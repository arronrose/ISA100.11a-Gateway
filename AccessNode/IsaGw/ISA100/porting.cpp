/*
* Copyright (C) 2013 Nivis LLC.
* Email:   opensource@nivis.com
* Website: http://www.nivis.com
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, version 3 of the License.
* 
* Redistribution and use in source and binary forms must retain this
* copyright notice.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

////////////////////////////////////////////////////////////////////////////////
/// @file porting.cpp
/// @brief Glue to link the Freescale Nivis ISA stack written in C to the gateway logic written in C++
/// What was done consists of replacing the network layer part of the stack with something that composes backbone
/// network headers and interfaces with the C++ part of the application. All the code placed below the network
/// layer in the Freescale Nivis ISA stack has been deleted since it's not used in the gateway.
////////////////////////////////////////////////////////////////////////////////



#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

#include "../../Shared/log_callback.h"
#include "../../Shared/UdpSocket.h"
#include "../../Shared/UtilsSolo.h"
#include "../../Shared/MicroSec.h"
#include "../../Shared/DurationWatcher.h"

#include "sfc.h"
#include "Ccm.h"
#include "porting.h"
#include "callbacks.h"
#include "rijndael-alg-fst.h"

#include "uap.h"
#include "uap_mo.h"


int	g_nDeviceType = DEVICE_TYPE_GW;

/// these are global variables which correspond to some members crom CIsaConfig
#define	smIP6addr g_stDMO.m_aucSysMng128BitAddr
extern uint8	g_ucSMLinkTimeout;
extern uint8	g_ucSMLinkTimeoutConf;

/// Callbacks
static PFNUAP_DataTimeout			g_pfnDataTimeout = NULL;
static PFNCompleteContractRequest	g_pfnCompleteContractRequest = NULL;
static PFNUAP_OnJoin				g_pfnOnJoin = NULL;
static PFNAlertAck			g_pfnAlertAck = NULL;

////////////////////////////////////////////////////////////////////////////////
/// Local methods


void FowardIPv6Message (IPv6Packet* p_pIpv6, int p_nLen);
void handlePacketForBBR(IPv6Packet* p_pIpv6, int p_nLen);

/// Extract IPv4 IP from an IPv6 IP (Nivis convention: IPv4 is in bytes 10-13 of IPv6, port in bytes 15&16)
static unsigned int cExtractIPv4IP( const uint8* p_aIpv6Addr );
/// Extract IPv4 port from an IPv6 IP (Nivis convention: IPv4 is in bytes 10-13 of IPv6, port in bytes 15&16)
static int cExtractIPv4Port( const uint8* p_aIpv6Addr );

/// C++ objects which support the stack
static Ccm	      ccm;
static CUdpSocket g_oUdpServer;

///ISA100 stuff here
uint8  g_ucDllDeleteSmibs = 0;
uint8  g_ucDllRestoreSfOffset;
TAI_STRUCT  g_stTAI;
struct timeval tvNow; /// TAKE CARE: TAI time is stored here; tv_sec is offset with TAI_OFFSET + CurrentUTCAdjustment from UNIX time
uint16 g_unDllSubnetId;
extern uint16 g_nSLMEKeysNo;
uint16 g_unGEspecialTimeout;

///in ISA100 world everything happens relative to TAI
/// TODO: replace MLSM_GetCrtTaiSec with MLSM_GetCrtTaiTime(NULL, NULL)
uint32 MLSM_GetCrtTaiSec(void)
{
	return MLSM_GetCrtTaiTime( );
}

uint32 MLSM_GetCrtTaiTime( struct timeval * p_ptCrtTAI /*= NULL*/)
{
	if( !gettimeofday(&tvNow, NULL) )
	{	g_stTAI.m_uc250msStep = tvNow.tv_usec / 250000;
		tvNow.tv_sec += TAI_OFFSET + g_stDPO.m_nCurrentUTCAdjustment;
	}
	else	/// cannot use gettimeofday, attempt to fallback to time(NULL)
	{	LOG_ISA100(LOGLVL_INF,"Gettimeofday works not. Subsecond precision you have not.");
		g_stTAI.m_uc250msStep = 0;
		tvNow.tv_sec = time(NULL) + TAI_OFFSET + g_stDPO.m_nCurrentUTCAdjustment;
	}
	
	if(p_ptCrtTAI)
	{
		*p_ptCrtTAI = tvNow;
	}

	return tvNow.tv_sec;
}

void MLSM_GetCrtTaiTime(uint32 *p_pulTaiSec, uint16 *p_punTaiFract)
{
	struct timeval tv;
	MLSM_GetCrtTaiTime( &tv );	/// also updates g_stTAI.m_uc250msStep
	
	if(p_pulTaiSec)
		*p_pulTaiSec = tv.tv_sec;

	if(p_punTaiFract)
		*p_punTaiFract = ((uint16)g_stTAI.m_uc250msStep << 13); //TODO: + TMR0_TO_FRACTION2( TMR_Get250msOffset() );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Inserts (binarizes) an extDLUint into a specified buffer by p_pData
/// @param  p_pData     - (output) a pointer to the data buffer
/// @param  p_unValue   - the 15 bit uint data to be binarized
/// @return p_pData     - pointer at end of inserted data
/// @remarks  don't check inside if value fits into 15 bits. this check must be done by caller\n
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 * DLMO_InsertExtDLUint( uint8 * p_pData, uint16  p_unValue)
{

  if (p_unValue < 0x80) 
  {
      *(p_pData++) = p_unValue << 1;
  }
  else
  {
      *(p_pData++) = (p_unValue << 1) | 0x01;
      *(p_pData++) = p_unValue >> 7;      
  }

  return (p_pData);
}

uint8 DLMO_ReadDeviceCapability(uint16* p_punSize, uint8* p_pBuf)
{
  uint8* pStart = p_pBuf;  
  
  int8   cRadioTxPower = 0;
  uint16 unEnergyLeft = 0x7FFF; //A value of 0x7FFF indicates that the feature is not supported
  DLL_MIB_DEV_CAPABILITIES stDevCap = c_stCapability;
  DLL_MIB_ENERGY_DESIGN    stEnergyDesign = c_stEnergyDesign;

  p_pBuf = DLMO_InsertExtDLUint(p_pBuf, stDevCap.m_unQueueCapacity);
  
  *(p_pBuf++) = stDevCap.m_ucClockAccuracy;  
  p_pBuf = DMAP_InsertUint16( p_pBuf, stDevCap.m_unChannelMap);    
  *(p_pBuf++) = stDevCap.m_ucDLRoles;
  
  p_pBuf = DMAP_InsertUint16( p_pBuf, stEnergyDesign.m_nEnergyLife);
  
  p_pBuf = DLMO_InsertExtDLUint(p_pBuf, stEnergyDesign.m_unListenRate);
  p_pBuf = DLMO_InsertExtDLUint(p_pBuf, stEnergyDesign.m_unTransmitRate);
  p_pBuf = DLMO_InsertExtDLUint(p_pBuf, stEnergyDesign.m_unAdvRate);  
  
  p_pBuf = DMAP_InsertUint16( p_pBuf, unEnergyLeft );    

  p_pBuf = DLMO_InsertExtDLUint(p_pBuf, stDevCap.m_unAckTurnaround);
  p_pBuf = DLMO_InsertExtDLUint(p_pBuf, stDevCap.m_unNeighDiagCapacity);
  
  *(p_pBuf++) = cRadioTxPower;
  *(p_pBuf++) = stDevCap.m_ucOptions;

  *p_punSize = p_pBuf - pStart;
  return SFC_SUCCESS;
}

void LibIsaSetUTCAdj(int p_nCrtUTCAdj)
{
	g_stDPO.m_nCurrentUTCAdjustment = p_nCrtUTCAdj;
}

/*
#define AES_SUCCES              0
#define AES_ERROR               1
uint8 AES_Crypt ( const uint8 * p_pucKey,
                          const uint8 * p_pucNonce,
                          const uint8 * p_pucToAuthOnly,
                          uint16        p_unToAuthOnlyLen,
                          const uint8 * p_pucToEncrypt,
                          uint16        p_unToEncryptLen,
                          uint8 *       p_pucEncryptedBuff,
                          uint8         p_ucInterruptFlag )
{//README: the 3 buffers indicated by the params will be the same
	uint8* work = malloc(p_unToEncryptLen+4);//add 4 len of MIC32
	memcpy(work, p_pucToEncrypt, p_unToEncryptLen);
	memcpy(p_pucEncryptedBuff, work, p_unToEncryptLen+4);
	free(work);
	return AES_SUCCES;
}

uint8 AES_Decrypt ( const uint8 * p_pucKey,
                          const uint8 * p_pucNonce,
                          const uint8 * p_pucToAuthOnly,
                          uint16        p_unToAuthOnlyLen,
                          const uint8 * p_pucToDecrypt,
                          uint16        p_unToDecryptLen,
                          uint8 *       p_pucDecryptedBuff ,
                          uint8         p_ucInterruptFlag )
{//README: the 3 buffers indicated by the params will be the same
	uint8* work = malloc(p_unToDecryptLen);//the 4 len of MIC32 are included
	memcpy( work, p_pucToDecrypt, p_unToDecryptLen);
	memcpy( p_pucDecryptedBuff, work, p_unToDecryptLen-4);
	free(work);
	return AES_SUCCES;
}
*/
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Implementation of the Matyas-Meyer-Oseas hash function with 16 byte block length
/// @param  p_pucInputBuff - the input buffer with length in bytes not smaller than 16 bytes
/// @param  p_unInputBuffLen - length in bytes of the input buffer
/// @return SFC_FAILURE if fail, SFC_SUCCESS if success
/// @remarks
///      This was originally in phy.c. Imported to this file, so we can use the software AES
///      Obs: result data replaces the first "MAX_HMAC_INPUT_SIZE" bytes from original source buffer
uint8 Crypto_Hash_Function( uint8* p_pucInputBuff,
                            uint16 p_unInputBuffLen
                           )
{
  int iKeyNr;
  u32 rguiKeyRk[60];

  uint8* pucBuffRes = p_pucInputBuff;
  uint8 aucInput[HASH_DATA_BLOCK_LEN];
  uint8 ucIdx;
  uint16 unInitialLen = p_unInputBuffLen;
  uint8 ucNextBlock = 0;
  uint8 ucLastBlock = 0;

  //the result having HASH_DATA_BLOCK_LEN length will overwrite the input buffer
  if( HASH_DATA_BLOCK_LEN > p_unInputBuffLen )
    return SFC_FAILURE;

  //the next condition need to be validated: 0 <= StringLengthInBits < (1 << HASH_DATA_BLOCK_LEN)
  if( p_unInputBuffLen > (1 << (HASH_DATA_BLOCK_LEN - 3)))
    return SFC_FAILURE;

  //the key for the first algorithm iteration need to be full 0
  memset(aucInput, 0x00, HASH_DATA_BLOCK_LEN);

  while(!ucLastBlock)
  {
    iKeyNr = rijndaelKeySetupEnc( rguiKeyRk, aucInput, HASH_DATA_BLOCK_LEN * 8 ) ;

    if( p_unInputBuffLen >= HASH_DATA_BLOCK_LEN )
    {
        memcpy(aucInput, p_pucInputBuff, HASH_DATA_BLOCK_LEN);

        p_unInputBuffLen -= HASH_DATA_BLOCK_LEN;
        p_pucInputBuff += HASH_DATA_BLOCK_LEN;
    }
    else
    {
        if( !p_unInputBuffLen )
        {
            // p_unInputBuffLenis multiple by 16
            aucInput[0] = 0x80;   //the bit7 of the padding's first byte must be "1"
            memset(aucInput + 1, 0x00, HASH_DATA_BLOCK_LEN - 3);
            //last iteration
            ucLastBlock = 1;
        }
        else
        {
            //+1 - concatenation of the input buffer with first byte 0x80
            //+2 - adding the last 2 bytes representing the buffer length info - particular case for 16 bytes data block len
            if( HASH_DATA_BLOCK_LEN - 3 >= p_unInputBuffLen)
            {
                memcpy(aucInput, p_pucInputBuff, p_unInputBuffLen);
                aucInput[p_unInputBuffLen++] = 0x80;   //the bit7 of the padding's first byte must be "1"
                memset(aucInput + p_unInputBuffLen, 0x00, (HASH_DATA_BLOCK_LEN - 2) - p_unInputBuffLen);
                //last iteration
                ucLastBlock = 1;
            }
            else
            {
                if( !ucNextBlock && (HASH_DATA_BLOCK_LEN - 2 <= p_unInputBuffLen) )
                {
                    memcpy(aucInput, p_pucInputBuff, p_unInputBuffLen);
                    aucInput[p_unInputBuffLen] = 0x80;   //the bit7 of the padding's first byte must be "1"
                    if( HASH_DATA_BLOCK_LEN - 2 == p_unInputBuffLen )
                    {
                        //1 byte zero padding
                        aucInput[HASH_DATA_BLOCK_LEN - 1] = 0x00;
                    }
                    ucNextBlock = 1;
                }
                else
                {
                    memset(aucInput, 0x00, HASH_DATA_BLOCK_LEN - 2);
                    ucLastBlock = 1;
                }
            }
        }

        if( ucLastBlock )
        {
            //it's the last block
            aucInput[HASH_DATA_BLOCK_LEN - 2] = unInitialLen >> 5;  //the MSB of bits number inside the input buffer
            aucInput[HASH_DATA_BLOCK_LEN - 1] = unInitialLen << 3;  //the LSB of bits number inside the input buffer
        }

    }

    // encrypt the block using the expandedKey
    rijndaelEncrypt( rguiKeyRk, iKeyNr, aucInput, pucBuffRes ) ;

    // xor-ing with Mi
    for (ucIdx = 0; ucIdx < HASH_DATA_BLOCK_LEN; ucIdx++)
    {
      aucInput[ucIdx] ^= pucBuffRes[ucIdx];
    }
  }


    //save the result data over the original buffer
  memcpy(pucBuffRes, aucInput, HASH_DATA_BLOCK_LEN);
  return SFC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Implementation of Keyed-Hash Message Authentication Code algorithm
/// @param  p_pucKey - cryptographic key(16 bytes length)
/// @param  p_pucInputBuff - the input buffer with length in bytes not smaller than 16 bytes
/// @param  p_unInputBuffLen - length in bytes of the input buffer
/// @return SFC_FAILURE if fail, SFC_SUCCESS if success
/// @remarks
///      This function was originally in phy.c
///      Obs: result data replaces the first "MAX_HMAC_INPUT_SIZE" bytes from original source buffer
uint8 Keyed_Hash_MAC(const uint8* p_pucKey,
                  uint8* const p_pucInputBuff,
                  uint16 p_unInputBuffLen
                  )
{
  uint8 aucK0[2* HASH_DATA_BLOCK_LEN + MAX_HMAC_INPUT_SIZE];
  uint8 ucIdx;

  if(MAX_HMAC_INPUT_SIZE < p_unInputBuffLen )
    return SFC_FAILURE;

  if( HASH_DATA_BLOCK_LEN > p_unInputBuffLen )
    return SFC_FAILURE;

  memcpy(aucK0 + HASH_DATA_BLOCK_LEN, p_pucKey, HASH_DATA_BLOCK_LEN);

  //XOR-ing with inner pad 0x36
  for(ucIdx = HASH_DATA_BLOCK_LEN; ucIdx < 2 * HASH_DATA_BLOCK_LEN; ucIdx++)
  {
    aucK0[ucIdx] ^= 0x36;
  }

  memcpy(aucK0 + 2 * HASH_DATA_BLOCK_LEN, p_pucInputBuff, p_unInputBuffLen);

  if( SFC_SUCCESS == Crypto_Hash_Function(aucK0 + HASH_DATA_BLOCK_LEN, HASH_DATA_BLOCK_LEN + p_unInputBuffLen) )
  {
    //XOR-ing with outer pad 0x5C
    for(ucIdx = 0; ucIdx < HASH_DATA_BLOCK_LEN; ucIdx++)
    {
      aucK0[ucIdx] = p_pucKey[ucIdx] ^ 0x5C;
    }

    if( SFC_SUCCESS == Crypto_Hash_Function(aucK0, 2 * HASH_DATA_BLOCK_LEN) )
    {
      memcpy(p_pucInputBuff, aucK0, HASH_DATA_BLOCK_LEN);
      return SFC_SUCCESS;
    }

  }

  return SFC_FAILURE;
}

/// This function replaces the corresponding function in the real stack
/// It uncompresses the UDP ports from the 6LoWPAN header, prepends the IPv6 addresses for the source and destination
/// and sends it to the CGwApp object for further processing
/// The parameters are the same as in the corresponding function, even though some of them are not really used here
/// At the end, it reports success to the Transport Layer.
void NLDE_DATA_RequestMemOptimzed
		(   				const uint8* p_aIpv6DstAddr,
                            NLME_CONTRACT_ATTRIBUTES * p_pContract,
                            uint8        p_ucPriorityAndFlags,
                            uint16       p_unNsduHeaderLen,
                            const void * p_pNsduHeader,
                            uint16       p_unAppDataLen,
                            const void * p_pAppData,
                            HANDLE       p_nsduHandle )
{

/*      uint8   m_ucVersionAndTrafficClass;
        uint8   m_aFlowLabel[3];
        uint8   m_aPayloadSize[2];
        uint8   m_ucNextHeader;
        uint8   m_ucHopLimit;
        uint8   m_ucSrcAddr[16];        ///< IPv6 address of the source of the packet
        uint8   m_ucDstAddr[16];        ///< IPv6 address of the destination of the packet
        uint8   m_aPayload[MAX_DATAGRAM_SIZE];  ///< IP payload

typedef struct^M
{  ^M
  uint16          m_unContractID;^M
  IPV6_ADDR       m_aSourceAddress;^M
  IPV6_ADDR       m_aDestAddress; ^M
  uint8           m_bUnused : 4;             ^M
  uint8           m_bIncludeContractFlag : 1;               // 0 = Don't Include, 1 = Include^M
  uint8           m_bDiscardEligible : 1;                   // 0 = Not Eligible, 1 = Eligible (default)^M
  uint8           m_bPriority : 2;                          // Valid values: 00, 01, 10, 11^M
}NLME_CONTRACT_ATTRIBUTES;^M
*/
	IPv6Packet	stPkt;/* = { { 0x60, // uint8   m_ucVersionAndTrafficClass; (version 6)
							{0,0,0},// uint8   m_aFlowLabel[3];
							0,	// uint8   m_u16PayloadSize;
							17,	// uint8   m_ucNextHeader; (UDP)
							64	} };	// uint8   m_ucHopLimit; (TTL=64)*/
	//[TODO]: Claudiu Hobeanu: assure that not entire 64k are set to 0 on every call, it is not necessary

	//WATCH_DURATION_INIT_DEF(oDurationWatcher);

	memset(&stPkt, 0, offsetof(IPv6Packet,m_stIPPayload.m_aUDPPayload) );
	stPkt.m_stIPHeader.m_ucVersionAndTrafficClass = 0x60;
	stPkt.m_stIPHeader.m_ucNextHeader = 17; //UDP
	stPkt.m_stIPHeader.m_ucHopLimit = 64;
	
	uint16		unNsduDataLen = p_unAppDataLen+p_unNsduHeaderLen;

	LOG_ISA100( LOGLVL_DBG, "NLDE_DATA_RequestMemOptimzed (cid=%d, hdrl=%d,appl=%d,h=%d",p_pContract->m_unContractID, p_unNsduHeaderLen, p_unAppDataLen, p_nsduHandle );

	if( unNsduDataLen > p_pContract->m_unAssignedMaxNSDUSize )
	{
		NLDE_DATA_Confirm( p_nsduHandle, DATA_TOO_LONG );
		return;
	}
//Contract priority establishes a base priority for all messages sent using that contract.
//  Four contract priorities are supported using 2 bits
//Message priority establishes priority within a contract.
//  Two message priorities are supported using 1 bit, low = 0 and high = 1.
//  Another 1 bit is reserved for future releases of this standard.
	stPkt.m_stIPHeader.m_ucVersionAndTrafficClass |= (0x03 & p_ucPriorityAndFlags) | (((uint8)p_pContract->m_bPriority) << 2 & 0x0C);
//	stPkt.m_stIPHeader.m_aFlowLabel[0] |= bDiscardEligible ? 0x60 : 0x20; // 0 | DE=0x40 | ECT(0)=0x20 | 0000
//	stPkt.m_stIPHeader.m_aFlowLabel[0] |= 0x20; // 0 | !DE | ECT(0) | 0000
	//set the destination address
	memcpy( stPkt.m_stIPHeader.m_ucDstAddr, p_pContract->m_aDestAddress, sizeof(stPkt.m_stIPHeader.m_ucDstAddr));
	stPkt.m_stIPHeader.m_aFlowLabel[1] = (uint8)(p_pContract->m_unContractID >> 8);
	stPkt.m_stIPHeader.m_aFlowLabel[2] = (uint8)p_pContract->m_unContractID;
	stPkt.m_stIPHeader.m_u16PayloadSize = htons(unNsduDataLen);
	//set the source address
	memcpy( stPkt.m_stIPHeader.m_ucSrcAddr, g_aIPv6Address, sizeof(stPkt.m_stIPHeader.m_ucSrcAddr));
	memcpy( stPkt.m_stIPHeader.m_ucDstAddr, p_aIpv6DstAddr, sizeof(stPkt.m_stIPHeader.m_ucDstAddr));

//	LOG_HEX_ISA100( LOGLVL_DBG, "ip6hdr:", (uint8*)&stPkt, 40);
//	LOG_HEX_ISA100( LOGLVL_DBG, "tlhdr:", (const unsigned char*)p_pNsduHeader, p_unNsduHeaderLen);
	memcpy( &stPkt.m_stIPPayload, p_pNsduHeader, p_unNsduHeaderLen);
	memcpy( (uint8_t*)&stPkt.m_stIPPayload+p_unNsduHeaderLen, p_pAppData, p_unAppDataLen);

	//WATCH_DURATION_DEF(oDurationWatcher);
	//send stPkt to UDP Socket. 40 =  sizeof ip6 header
	uint8 ucStatus = SendUDP( &stPkt, p_unNsduHeaderLen+p_unAppDataLen+40 )
			? SFC_SUCCESS : SFC_FAILURE; // maybe SFC_TX_LINK or SFC_NACK on failure?

	//WATCH_DURATION_DEF(oDurationWatcher);
	//tell them we're done
	NLDE_DATA_Confirm( p_nsduHandle, ucStatus );
	//WATCH_DURATION_DEF(oDurationWatcher);
}

/// @brief Initializes the global variables from this file to the proper values
/// @param p_cOwnIPv6 - 16 bytes with IPV6 address (last 4 bytes are the IPV4 address)
/// @param p_aSMIPv6
/// @param p_ushPort port to listen on
/// @param p_ucPingTimeoutConf
/// @param p_nDeviceType - the device type (DEVICE_TYPE_BBR / DEVICE_TYPE_GW / DEVICE_TYPE_SM)
/// @retval 1 init was successfull, port bound ok
/// @retval 0 init failed, cannot bind porting
/// @remarks return false is FATAL error, the application should stop in this case
int LibIsaInit(const uint8* p_aOwnIPv6, const uint8* p_aSMIPv6, unsigned short p_ushPort,
		const uint8* p_aOwnEUI64, const uint8* p_aSecManEUI64, const uint8* p_aJoinAppKey,
		uint8 p_ucPingTimeoutConf, int p_nDeviceType, unsigned short p_unDllSubnetId, int p_nCrtUTCAdj)
{
	srandom(time(NULL));

	if ( p_nDeviceType == DEVICE_TYPE_SM)
		TLDE_Duplicate_Init(3000,60);		// 3000 messages and 60 seconds
	else
		TLDE_Duplicate_Init(100,60);		// 100 messages and 60 seconds

	ASLDE_Init();
	TLME_Init();
	SLME_Init();
	LOG_ISA100(LOGLVL_INF,"Initializing libisa version: "LIBISA_VERSION);
	g_nDeviceType = p_nDeviceType; //[TODO]: Claudiu Hobeanu: maybe set also the c_stDevInfo.m_ucDeviceType to p_nDeviceType
	ccm.SetAuthenFldSz( MIC_SIZE );//MIC32
	ccm.SetLenFldSz(2);//nonce on 13 bytes
	memcpy(g_aIPv6Address, (const unsigned char*)p_aOwnIPv6, 16);
	memcpy(c_oEUI64BE, p_aOwnEUI64, sizeof(EUI64_ADDR));
	memcpy(c_oSecManagerEUI64BE, p_aSecManEUI64, sizeof(EUI64_ADDR));
	if(p_aSMIPv6)	  memcpy(smIP6addr, p_aSMIPv6, 16); //this is only used in our DMAP, so the SM can NULL it
	if(p_aJoinAppKey) memcpy(g_aJoinAppKey, p_aJoinAppKey, 16); //this is only used in our DMAP, so the SM can NULL it
	LibIsaConfig(p_ucPingTimeoutConf);
	g_unDllSubnetId = p_unDllSubnetId;
	LibIsaSetUTCAdj( p_nCrtUTCAdj );
	if(!g_oUdpServer.Create())
		return 0;

	return g_oUdpServer.Bind( p_ushPort );
}

/// @brief Configures on the fly some global variables of the stack. Useful to tune the UNJOIN detection timeout and the number of retries without unjoining :)
/// @param p_ucSMLinkTimeoutConf - the detection timeout limit for UNJOINing the stack's DMAP. The detection timeout is measured but cannot get lower than this.
/// @param p_ucMaxRetries - how many retries should the stack do when trying to send reliable requests
/// @param p_unRetryTimeout - retry timer to use when something happens to the RTO in the contract (not really useful: no contract = no sending !)
/// @remarks This can be called on the fly, to reconfigure the variables without rejoining: they take effect at next packets
void LibIsaConfig( uint8 p_ucSMLinkTimeoutConf, uint8 p_ucMaxRetries, uint16 p_unRetryTimeout, uint16 p_unGEspecialTimeout )
{
	g_ucMaxUAPRetries = g_ucDmapMaxRetry = p_ucMaxRetries;
	g_ucSMLinkTimeout = g_ucSMLinkTimeoutConf = p_ucSMLinkTimeoutConf;
	g_unDmapRetryTout = p_unRetryTimeout;
	g_unGEspecialTimeout = p_unGEspecialTimeout;
}

/// @brief Configures (provisions) various DMAP variables (signature subject to change)
/// @brief Should be used only by devices using DMAP (GW and eventually BBR)
/// @param p_szRevision - Device software version number
/// @param p_szModel - Device model
/// @note MUST call it before device first join
void LibIsaProvisionDMAP( const char * p_szRevision, const char * p_szModel, const char * p_szTag )
{	// No need to null-terminate these strings
	size_t sizeRev   = _Min(sizeof(c_aucSWRevInfo), strlen(p_szRevision));
	size_t sizeModel = _Min(sizeof(c_aucModelID), strlen(p_szModel));
	size_t sizeTag   = _Min(sizeof(g_stDMO.m_aucTagName), strlen(p_szTag));

	memset( c_aucSWRevInfo, ' ' , sizeof(c_aucSWRevInfo) );
	memcpy( c_aucSWRevInfo, p_szRevision, sizeRev);
	c_ucSWRevInfoSize = 16;
	c_aDMOFct[22].m_ucSize = c_ucSWRevInfoSize;		// DMAP_SW_REVISION_INFO

	memset( c_aucModelID, ' ' , sizeof(c_aucModelID) );
	memcpy( c_aucModelID, p_szModel, sizeModel );
	c_aDMOFct[ 7].m_ucSize = 16;					// DMO_MODEL_ID

	memset( g_stDMO.m_aucTagName, ' ' , sizeof(g_stDMO.m_aucTagName) );
	memcpy( (char*)g_stDMO.m_aucTagName, p_szTag, sizeTag);
	c_aDMOFct[ 8].m_ucSize = 16;					// DMO_TAG_NAME
}

/// @brief Deletes all UdpSockets
void LibIsaShutdown(void)
{
	g_oUdpServer.Close();
}

/// @brief Wait p_nTimeoutUSec microSec (10^-6) for activity on all UDP socket
/// @param p_nTimeoutUSec - the max timeout, specified in microseconds (sec*10^-6)
/// @retval false on timeout,
/// @retval true when there is activity on socket
int LibIsaCheckRecv( int p_nTimeoutUSec )
{
	return g_oUdpServer.CheckRecv( p_nTimeoutUSec );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Register callbacks - add here all necessary callbacks
/// @param p_ucReqId the request id
/// @param p_ucStatus error status - MUST be some kind of failure
/// @remarks SM MUST pass only p_pfnDataTimeout - callback for REQuest timeouts.
/// SM MUST NOT pass parameter p_pfnCompleteContractRequest
/// TODO: this function should be joind with LibIsaInit
////////////////////////////////////////////////////////////////////////////////
void LibIsaRegisterCallbacks( PFNUAP_DataTimeout p_pfnTimeout, PFNUAP_OnJoin p_pfnOnjoin /*= NULL*/,PFNCompleteContractRequest p_pfnContractComplete /*= NULL*/ )
{
	g_pfnDataTimeout				= p_pfnTimeout;
	g_pfnOnJoin						= p_pfnOnjoin;
	g_pfnCompleteContractRequest 	= p_pfnContractComplete;
	LOG_ISA100(LOGLVL_DBG,"LibIsaRegisterCallbacks: pfn %p %p %p", g_pfnDataTimeout, p_pfnOnjoin, g_pfnCompleteContractRequest);
}

#define AES_SUCCESS             0
#define AES_ERROR               1

/// SPEED_FORMAT expect 3 int values: SIZE in bytes, TIME in usec, SPEED in bytes/sec
#define SPEED_FORMAT "%4u bytes in %4u usec, %6u b/s"
/// CCM encryption C-to-C++ wrapper
uint8 AES_Crypt_User ( const uint8 * p_pucKey,
		const uint8 *	p_pucNonce,
		uint8 *		p_pucToAuthOnly,
		uint16		p_unToAuthOnlyLen,
		uint8 *		p_pucToEncrypt,
		uint16		p_unToEncryptLen )
{
	uint8 work[ p_unToAuthOnlyLen + p_unToEncryptLen + MIC_SIZE ];
	LOG_HEX_ISA100( LOGLVL_DBG, "encrypt<=", p_pucToEncrypt, p_unToEncryptLen);
	
	CMicroSec uSec;
	if( !ccm.AuthEncrypt( p_pucKey, p_pucNonce, p_pucToAuthOnly, p_unToAuthOnlyLen, p_pucToEncrypt, p_unToEncryptLen, work) )
	{	/// assert
		LOG_ISA100(LOGLVL_ERR, "ERROR AES_Crypt_User: AuthEncrypt(%p %u %p %u | %u %u)",
			p_pucToAuthOnly, p_unToAuthOnlyLen, p_pucToEncrypt, p_unToEncryptLen, ccm.GetAuthenFldSz(), ccm.GetLenFldSz() );
		LOG_HEX_ISA100( LOGLVL_ERR,"encrypt", work, sizeof(work));
		LOG_HEX_ISA100( LOGLVL_ERR,"key:", p_pucKey, 16);
		LOG_HEX_ISA100( LOGLVL_ERR,"nonce:", p_pucNonce, 13);
		LOG_HEX_ISA100( LOGLVL_ERR,"aad:", p_pucToAuthOnly, p_unToAuthOnlyLen);
		return AES_ERROR;
	}

	memcpy(p_pucToEncrypt, work+p_unToAuthOnlyLen, p_unToEncryptLen + MIC_SIZE);

	int nElapsed = uSec.GetElapsedUSec();
	if( nElapsed )
	{
		LOG_ISA100(LOGLVL_INF, "STATS: encrypt=> " SPEED_FORMAT,
			p_unToEncryptLen + MIC_SIZE, nElapsed, MICROSEC_IN_SEC * (p_unToEncryptLen + MIC_SIZE) / nElapsed);
	}
//		LOG_ISA100(LOGLVL_DBG, "encrypt=>", p_pucEncryptedBuff, p_unToEncryptLen+MIC_SIZE);
	return AES_SUCCESS;
}

/// CCM decryption C-to-C++ wrapper
uint8 AES_Decrypt_User ( const uint8 *	p_pucKey,
				const uint8 *	p_pucNonce,
				uint8 *		p_pucToAuthOnly,
				uint16		p_unToAuthOnlyLen,
				uint8 *		p_pucToDecrypt,
				uint16		p_unToDecryptLen )
{
	uint8 work[ p_unToDecryptLen ];//the 4 len of MIC32 are included
	CMicroSec uSec;
	
	memcpy( work, p_pucToDecrypt, p_unToDecryptLen);
	
	bool bRet = ccm.CheckAuthDecrypt( p_pucKey, p_pucNonce,	p_pucToAuthOnly, p_unToAuthOnlyLen, work, p_unToDecryptLen, p_pucToDecrypt) ;
	if( !bRet )
	{	/// assert
		LOG_ISA100( LOGLVL_ERR,"ERROR AES_Decrypt_User: authOnlyLen %u len %u", p_unToAuthOnlyLen, p_unToDecryptLen );
		LOG_ISA100( LOGLVL_ERR,"ERROR AES_Decrypt_User: CheckAuthDecrypt(%p %p %p %u %p %u %p | %u %u)",
			p_pucKey, p_pucNonce, p_pucToAuthOnly, p_unToAuthOnlyLen, work, p_unToDecryptLen, p_pucToDecrypt,
			ccm.GetAuthenFldSz(), ccm.GetLenFldSz() );
		LOG_HEX_ISA100( LOGLVL_ERR,"  decrypt", work, p_unToDecryptLen);
		LOG_HEX_ISA100( LOGLVL_ERR,"  key:", p_pucKey, 16);
		LOG_HEX_ISA100( LOGLVL_ERR,"  nonce:", p_pucNonce, 13);
		LOG_HEX_ISA100( LOGLVL_ERR,"  aad:", p_pucToAuthOnly, p_unToAuthOnlyLen);
		//SLME_PrintKeys();
	}
	unsigned unElapsed = uSec.GetElapsedUSec();
	if( unElapsed )
	{
		LOG_ISA100(LOGLVL_INF, "STATS: decrypt=> " SPEED_FORMAT,
			p_unToDecryptLen-MIC_SIZE, unElapsed, MICROSEC_IN_SEC * (p_unToDecryptLen-MIC_SIZE) / unElapsed);
	}
	LOG_HEX_ISA100( LOGLVL_DBG, "decrypt=>", p_pucToDecrypt, p_unToDecryptLen-MIC_SIZE);

	return bRet ? AES_SUCCESS : AES_ERROR;
}

/// CCM* decryption C-to-C++ wrapper with no message integrity check
uint8 AES_Decrypt_User_NoMIC ( const uint8 *	p_pucKey,
				const uint8 *	p_pucNonce,
				uint8 *		p_pucToAuthOnly,
				uint16		p_unToAuthOnlyLen,
				uint8 *		p_pucToDecrypt,
				uint16		p_unToDecryptLen )
{
	uint8 work[ p_unToDecryptLen ];
	CMicroSec uSec;
	
	memcpy( work, p_pucToDecrypt, p_unToDecryptLen);
	
	ccm.SetAuthenFldSz( 0 );
	bool bRet = ccm.CheckAuthDecrypt( p_pucKey, p_pucNonce, p_pucToAuthOnly, p_unToAuthOnlyLen, work, p_unToDecryptLen, p_pucToDecrypt );
	if( !bRet )
	{	/// assert
		LOG_ISA100( LOGLVL_ERR,"ERROR AES_Decrypt_User_NoMIC: CheckAuthDecrypt(%p %p %p %u %p %u %p | %u %u)",
			p_pucKey, p_pucNonce, p_pucToAuthOnly, p_unToAuthOnlyLen, work, p_unToDecryptLen, p_pucToDecrypt,
			ccm.GetAuthenFldSz(), ccm.GetLenFldSz() );
		LOG_HEX_ISA100( LOGLVL_ERR,"  decrypt", work, p_unToDecryptLen);
		LOG_HEX_ISA100( LOGLVL_ERR,"  key:", p_pucKey, 16);
		LOG_HEX_ISA100( LOGLVL_ERR,"  nonce:", p_pucNonce, 13);
		LOG_HEX_ISA100( LOGLVL_ERR,"  aad:", p_pucToAuthOnly, p_unToAuthOnlyLen);
		//SLME_PrintKeys();
	}
	ccm.SetAuthenFldSz( MIC_SIZE );//MIC32
	
	unsigned unElapsed = uSec.GetElapsedUSec();
	if( unElapsed )
	{
		LOG_ISA100(LOGLVL_INF, "STATS: decrypt=> " SPEED_FORMAT,
			p_unToDecryptLen, unElapsed, MICROSEC_IN_SEC * (p_unToDecryptLen) / unElapsed);
	}
	LOG_HEX_ISA100( LOGLVL_DBG, "decrypt=>", p_pucToDecrypt, p_unToDecryptLen );
	return bRet ? AES_SUCCESS : AES_ERROR;
}

static NLME_ROUTE_ATTRIBUTES* FindRouteHelper (const uint8_t* p_pDestAddr)
{
	NLME_ROUTE_ATTRIBUTES * pRoute = NLME_FindDestinationRoute( (uint8_t*)p_pDestAddr );

	if( !pRoute ) // not found
	{
		LOG_HEX_ISA100( LOGLVL_ERR, "ERROR FindRouteHelper: no route to ", p_pDestAddr, 16);
		return 0;
	}

//	LOG_ISA100( LOGLVL_DBG, "FindRouteHelper: route nextHop=%s Out_%s", GetHex(pRoute->m_aNextHopAddress, sizeof(pRoute->m_aNextHopAddress)), pRoute->m_bOutgoingInterface ? "BBR" : "DL" );
	return pRoute;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief The C-to-C++ wrapper which glues the "network" layer from ISA100 to the actual network
/// @brief layer of the system. It is called once for every ISA100 datagram and it does the EUI64-to-IPv6
/// @brief translation then sends the packet down the UDP Socket
/// @param p_pAddr64 the EUI64 address of the destination node
/// @param msg the buffer containing the data ready to send
/// @param len length of the buffer
/// @retval non-zero: success
/// @retval 0 - failed to send
////////////////////////////////////////////////////////////////////////////////
int SendUDP( IPv6Packet *msg, size_t len )
{
	//WATCH_DURATION_INIT_DEF(oDurationWatcher);

	LOG_HEX_ISA100( LOGLVL_INF, "ISA-Send:", msg->m_stIPHeader.m_ucDstAddr, 16 );

	NLME_ROUTE_ATTRIBUTES * pRoute = FindRouteHelper (msg->m_stIPHeader.m_ucDstAddr);

	//WATCH_DURATION_DEF(oDurationWatcher);
	if (!pRoute)
	{
		return 0;
	}

	if ( memcmp (msg->m_stIPHeader.m_ucSrcAddr, g_aIPv6Address, sizeof(msg->m_stIPHeader.m_ucSrcAddr)) == 0 )
	{	
		msg->m_stIPHeader.m_ucHopLimit = pRoute->m_ucNWK_HopLimit;
	}
	else
	{
		msg->m_stIPHeader.m_ucHopLimit --;
		if (msg->m_stIPHeader.m_ucHopLimit > pRoute->m_ucNWK_HopLimit)	
		{
			msg->m_stIPHeader.m_ucHopLimit = pRoute->m_ucNWK_HopLimit;
		}
	}

	if (msg->m_stIPHeader.m_ucHopLimit == 0)
	{
		LOG_ISA100( LOGLVL_ERR, "SendUDP: hop limit reach 0 -> drop ");
		return 0;
	}

	if (pRoute->m_bOutgoingInterface == OutgoingInterfaceBBR)
	{
		//WATCH_DURATION_DEF(oDurationWatcher);
		in_addr sIpv4 = { cExtractIPv4IP(pRoute->m_aNextHopAddress) };
		int nPort = cExtractIPv4Port(pRoute->m_aNextHopAddress);
		LOG_ISA100(    LOGLVL_INF, "SendUDP (%s:%u len %u)", inet_ntoa(sIpv4), nPort, len);
		LOG_HEX_ISA100( LOGLVL_DBG, "    Send:", (const unsigned char*)msg, len);
		//WATCH_DURATION_DEF(oDurationWatcher);
		bool ret = g_oUdpServer.SendTo( sIpv4.s_addr, nPort, (const char*)msg, len);
		//WATCH_DURATION_DEF(oDurationWatcher);
		return ret;
	}

	//send to DL
	if (!g_pCallbackSendToDL)
	{
		LOG_ISA100( LOGLVL_ERR, "WARN: route set to DL but no callback registered");
		return 0;
	}

	return g_pCallbackSendToDL (msg, len);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Attempts to receive a UDP message.
/// @brief If message is received in specified timeout, it is being sent up the stack(NL/TL/ASL)
/// @param p_nTimeoutUSec - the max timeout, specified in microseconds (sec*10^-6)
////////////////////////////////////////////////////////////////////////////////
void RecvUDP( int p_unTimeoutUSec )
{
	IPv6Packet 	stPkt;
	size_t		nPktSize = sizeof(IPv6Packet);
	char 		szSrcIP[ 16 ];
	int 		nSrcPort;

	if (!g_oUdpServer.IsValid())
	{
		if (!g_oUdpServer.Create())
		{
			return;
		}
		if (!g_oUdpServer.Bind(g_oUdpServer.GetPort()))
		{
			return;
		}	
	}

	if( ! g_oUdpServer.CheckRecv( p_unTimeoutUSec ))	// if zero timeout: return immediately
	{
		return;
	}

	if (!g_oUdpServer.RecvFrom( (char*)&stPkt, &nPktSize, 0, szSrcIP, &nSrcPort))
	{
		return;
	}

	if(nPktSize <= sizeof(IPv6IPHeader) + sizeof(IPv6UDPHeader)) // TODO: map a strucutre here, get sizeof()
	{  //ip6 header size
			LOG_ISA100( LOGLVL_ERR, "ERROR RecvUDP: packet too short from %s:%u", szSrcIP, nSrcPort);
			return;  // dump message
	}

	LOG_ISA100(    LOGLVL_INF, "RecvUDP (%s:%u len %u)", szSrcIP, nSrcPort, nPktSize);
	LOG_HEX_ISA100( LOGLVL_INF, "    IN-IPv6hdr:", (unsigned char*)&stPkt, sizeof(IPv6IPHeader));
	LOG_HEX_ISA100( LOGLVL_INF, "    UDP/IPv6: ", (unsigned char*)&stPkt.m_stIPPayload, sizeof(IPv6UDPHeader));
	LOG_ISA100(    LOGLVL_DBG, "    SPort %X,DPort %X", ntohs(stPkt.m_stIPPayload.m_stUDPHeader.m_unUdpSPort),ntohs(stPkt.m_stIPPayload.m_stUDPHeader.m_unUdpDPort) );
	LOG_HEX_ISA100( LOGLVL_DBG, "    Recv:",  (uint8_t*)&stPkt, nPktSize);
	if(memcmp(stPkt.m_stIPHeader.m_ucDstAddr, g_aIPv6Address, 16))
	{
		FowardIPv6Message(&stPkt,nPktSize);
		return;
	}

	uint8_t pTmpBuff[nPktSize];

	if (g_nDeviceType == DEVICE_TYPE_BBR)
	{	memcpy (pTmpBuff, &stPkt, nPktSize);
	}
	
	//send to upper layers
	NLDE_DATA_Indication ( stPkt.m_stIPHeader.m_ucSrcAddr,
				nPktSize - sizeof(IPv6IPHeader),
				&stPkt.m_stIPPayload,
				(stPkt.m_stIPHeader.m_aFlowLabel[0] & 0xF0) | (stPkt.m_stIPHeader.m_ucVersionAndTrafficClass & 0x0F) );

	if (g_nDeviceType == DEVICE_TYPE_BBR)
	{
		handlePacketForBBR ((IPv6Packet*)pTmpBuff, nPktSize);
	}
}

void FowardIPv6Message (IPv6Packet* p_pIpv6, int p_nLen)
{
	LOG_HEX_ISA100( LOGLVL_INF, "Not for me -> try fwd destAdr=", (unsigned char*)p_pIpv6->m_stIPHeader.m_ucDstAddr, sizeof(p_pIpv6->m_stIPHeader.m_ucDstAddr));

	if (p_pIpv6->m_stIPHeader.m_ucHopLimit == 0)
	{
		LOG_ISA100( LOGLVL_ERR, "Hop Limit reached -> drop");
		return;
	}

	SendUDP(p_pIpv6, p_nLen);
}

void handlePacketForBBR(IPv6Packet* p_pIpv6, int p_nLen)
{
	//we are on BBR

	if (DMAP_BBR_Sniffer())
	{
		//the linux BBR is the final destination
		return;
	}

	//send to DL
	if (!g_pCallbackSendToDL)
	{
		LOG_ISA100( LOGLVL_ERR, "WARN: msg for TR but no callback registered");
		return;
	}

	g_pCallbackSendToDL (p_pIpv6, p_nLen);
}

/// @brief Extract IPv4 IP from an IPv6 IP (Nivis convention: IPv4 is in bytes 13-16 of IPv6, port in bytes 11&12)
static unsigned int cExtractIPv4IP( const uint8* p_aIpv6Addr )
{
	unsigned int unIPv4;

	memcpy(&unIPv4,p_aIpv6Addr + 12,4);
	return unIPv4;
}

/// @brief Extract IPv4 port from an IPv6 IP (Nivis convention: IPv4 is in bytes 13-16 of IPv6, port in bytes 11&12)
static int cExtractIPv4Port( const uint8* p_aIpv6Addr )
{
	uint16_t u16Port;

	memcpy(&u16Port,p_aIpv6Addr + 10,2);
	return ntohs(u16Port);
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Callback to indicate request timeout. Called from ISA100 stack
/// @param p_ushAppHandle the application - provided request handle.
/// @param p_ucSFC error status - MUST be some kind of failure
/// @remarks TAKE CARE: we may need some additional parameters - device address?
////////////////////////////////////////////////////////////////////////////////
void UAP_DataTimeout( uint16 p_ushAppHandle, uint8 p_ucSrcSAP, uint8 p_ucDstSAP, uint8 p_ucSFC )
{
	if( g_pfnDataTimeout )
		g_pfnDataTimeout( p_ushAppHandle, p_ucSrcSAP, p_ucDstSAP, p_ucSFC );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu, Mihai Buha
/// @brief Search original APDU by application handle and Source TSAP ID (to be used on timeout)
/// @param p_ushAppHandle the application - provided request handle.
/// @param p_ucTSAPID the TSAP ID of the application (provides context for p_ushAppHandle)
/// @param p_pIdtf [out] the APDU identifier of the TX APDU returned
/// @return pointer to the APDU or NULL if not found
/// @remarks This is a hybrid between ASLDE_GetMyAPDU and ASLDE_SearchOriginalRequest, with the difference
///	that those are used for normal RX flow, while this is on the timeout flow. Also note that the APDU_IDTF
///	is filled with TX information, not RX.
////////////////////////////////////////////////////////////////////////////////
uint8 * ASLDE_GetMyOriginalTxAPDU( uint16 p_ushAppHandle, uint8 p_ucTSAPID, APDU_IDTF* p_pIdtf )
{
	CApduListTx::iterator it;
	for (it = g_oAslMsgListTx.begin(); it != g_oAslMsgListTx.end(); it++)
	{
		CAslTxMsg::Ptr pTxMsg = *it;
		
		
		if ( p_ushAppHandle == pTxMsg->m_stQueueHdr.m_unAppHandle && pTxMsg->m_stQueueHdr.m_ucSrcSAP == p_ucTSAPID )
		{
			memcpy( p_pIdtf->m_aucAddr, pTxMsg->m_stQueueHdr.m_aucDest, sizeof p_pIdtf->m_aucAddr );
			p_pIdtf->m_ucSrcTSAPID		= pTxMsg->m_stQueueHdr.m_ucSrcSAP;
			p_pIdtf->m_ucDstTSAPID		= pTxMsg->m_stQueueHdr.m_ucDstSAP;
			p_pIdtf->m_ucPriorityAndFlags	= pTxMsg->m_stQueueHdr.m_ucPriorityAndFlags;
			p_pIdtf->m_ucTransportTime	= 0;
			p_pIdtf->m_ucSecurityCtrl	= pTxMsg->m_stQueueHdr.m_ucAllowEncryption;
			p_pIdtf->m_unDataLen		= pTxMsg->m_stQueueHdr.m_unAPDULen;
			memset( &p_pIdtf->m_tvTxTime, 0, sizeof p_pIdtf->m_tvTxTime );

			LOG_ISA100(LOGLVL_DBG,"Unrecoverable timeout: delete the TX APDU handle %u", pTxMsg->m_stQueueHdr.m_unAppHandle);
			pTxMsg->m_stQueueHdr.m_ucStatus = APDU_READY_TO_CLEAR;
			return &pTxMsg->m_oTxApduData[0];
		}
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Mihai Buha
/// @brief Wrapper to call CGwUAP::ContractNotification. Called from DMAP
/// @param p_pContract
////////////////////////////////////////////////////////////////////////////////
void UAP_NotifyAddContract( DMO_CONTRACT_ATTRIBUTE * p_pContract )
{
	if( g_pfnCompleteContractRequest )
		g_pfnCompleteContractRequest( p_pContract->m_unContractID, p_pContract );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Mihai Buha
/// @brief Wrapper to call CGwUAP::ContractNotification. Called from DMAP
/// @param p_nContractId
////////////////////////////////////////////////////////////////////////////////
void UAP_NotifyContractDeletion(uint16 p_unContractID)
{
	if( g_pfnCompleteContractRequest )
		g_pfnCompleteContractRequest( p_unContractID, NULL );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Mihai Buha
/// @brief Wrapper to call CGwUAP::ContractNotification. Called from DMAP
/// @param p_pContract
////////////////////////////////////////////////////////////////////////////////
void UAP_NotifyContractFailure( DMO_CONTRACT_ATTRIBUTE * p_pContract )
{
	if( g_pfnCompleteContractRequest )
		g_pfnCompleteContractRequest( INVALID_CONTRACTID, p_pContract );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Mihai Buha
/// @brief Wrapper to call CGwUAP::AlertNotification. Called from DMAP
/// @param p_ucAlertID
////////////////////////////////////////////////////////////////////////////////
void UAP_NotifyAlertAcknowledge(uint8 p_ucAlertID)
{
	if( g_pfnAlertAck )
		g_pfnAlertAck( p_ucAlertID );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Wrapper to call CGwUAP::OnJoin. Called from DMAP
/// @param p_nJoined - 0 on unjoin, != 0 on join
////////////////////////////////////////////////////////////////////////////////
void UAP_OnJoin( void )
{
	if( g_pfnOnJoin )
		g_pfnOnJoin( 1 );
}

void UAP_OnJoinReset( void )
{
	if( g_pfnOnJoin )
		g_pfnOnJoin( 0 );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief format a IPv6 as XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX
/// @param p_pIPv6 pointer to IPv6 address. Must have at least 16 bytes available
/// @param p_pDstBuf the destination buffer, allocated by the acller. Must be at least 40 bytes (recommend 64)
////////////////////////////////////////////////////////////////////////////////
void FormatIPv6( const byte * p_pIPv6, char * p_pDstBuf )
{	int i;
	char tmp[3];
	p_pDstBuf[0] = 0;
	for( i=0; i<16; ++i)
	{
		sprintf(tmp,"%02X", p_pIPv6[i]);
		strcat(p_pDstBuf, tmp);
		if( i%2 && i<15 )
		{
			strcat(p_pDstBuf,":");
		}
	}
}
