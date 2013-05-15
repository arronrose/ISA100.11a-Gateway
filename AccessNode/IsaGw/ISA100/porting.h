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
/// @file porting.h
/// @brief Declarations for various functions and structures needed in porting.cpp
/// @remarks
/// @remarks This is the ONLY HEADER FILE from ISA100 folder which needs to be included by stack users
/// @remarks
////////////////////////////////////////////////////////////////////////////////

#ifndef _PORTING_H_
#define _PORTING_H_

#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include "../../Shared/UtilsSolo.h"

#include "typedef.h"
#include "nlme.h"
#include "tlde.h"
#include "tlme.h"
#include "slme.h"
#include "dmap.h"
#include "dmap_dmo.h"
#include "dmap_armo.h"
#include "aslde.h"
#include "aslsrvc.h"

#define TAI_OFFSET (0x16925E80)

extern int g_nDeviceType;
//extern DLL_SMIB_DATA_STRUCT g_stDll;

#define GE_SPECIAL_TIMEOUT 60

void DMAP_DMO_CheckSMLink(void);
uint8 DLME_SMIB_Set_Request(uint8 p_ucTable, const void* pValue);
uint8 DLME_SMIB_Get_Request(uint8 p_ucTable, uint16 p_unUID, uint8 p_bFuture, void* p_pValue);
uint8 DLME_MIB_Set_Request(uint8 p_ucAttribute, const void* p_pValue);
uint8 DLMO_ReadDeviceCapability(uint16* p_punSize, uint8* p_pBuf);
void DLL_Init(void);
void DLME_init();
uint32 MLSM_GetCrtTaiSec(void); ///TODO: replace MLSM_GetCrtTaiSec() with MLSM_GetCrtTaiTime()

/// TAKE CARE: p_punTaiFract is in units of 10^-15
void   MLSM_GetCrtTaiTime(uint32 *p_pulTaiSec, uint16 *p_punTaiFract);

/// this return the time as normal, vith p_ptCrtTAI->tv_usec being actual microseconds
uint32 MLSM_GetCrtTaiTime( struct timeval * p_ptCrtTAI = NULL );

void NLDE_DATA_RequestMemOptimzed
        (   const uint8* p_pEUI64DestAddr,
            NLME_CONTRACT_ATTRIBUTES * p_pContract,
            uint8        p_ucPriorityAndFlags,
            uint16       p_unNsduHeaderLen,
            const void * p_pNsduHeader, 
            uint16       p_unAppDataLen,
            const void * p_pAppData, 
            HANDLE       p_nsduHandle );

#define MIC_SIZE 4
uint8 AES_Crypt_User ( const uint8 * p_pucKey,
                const uint8 *   p_pucNonce,
                uint8 *         p_pucToAuthOnly,
                uint16          p_unToAuthOnlyLen,
                uint8 *         p_pucToEncrypt,
                uint16          p_unToEncryptLen );

uint8 AES_Decrypt_User ( const uint8 *  p_pucKey,
                                const uint8 *   p_pucNonce,
                                uint8 *         p_pucToAuthOnly,
                                uint16          p_unToAuthOnlyLen,
                                uint8 *         p_pucToDecrypt,
                                uint16          p_unToDecryptLen );
uint8 AES_Decrypt_User_NoMIC ( const uint8 *    p_pucKey,
                                const uint8 *   p_pucNonce,
                                uint8 *         p_pucToAuthOnly,
                                uint16          p_unToAuthOnlyLen,
                                uint8 *         p_pucToDecrypt,
                                uint16          p_unToDecryptLen );

#define g_unRandValue random()
#ifdef DLL_KEY_ID_NOT_AUTHENTICATED
	#define MAX_HMAC_INPUT_SIZE     2*16+2*8+2+2*2+2*16
#else
	#define MAX_HMAC_INPUT_SIZE     2*16+2*8+2+2*2+1+2*16 //according with the Security Join Response Specs
#endif
#define HASH_DATA_BLOCK_LEN     (uint8)16           //lenght in bytes of the hash data block
uint8 Keyed_Hash_MAC(const uint8* p_pucKey,
                  uint8* const p_pucInputBuff,
                  uint16 p_unInputBuffLen
                  );

uint8 DMO_requestNewContract(  const uint8 * p_pDstAddr128,
                                      uint16        p_unDstSAP,
                                      uint16        p_unSrcSAP,
                                      uint8         p_ucSrcvType,
                                      uint16        p_unMaxNSDUSize,
                                      uint8         p_ucPriority,
                                      uint8         p_ucPeriodOrSendWindow,
                                      uint16        p_unPhaseOrCommitedBurst,
                                      uint16        p_unDeadlineOrExcessBurst,
                                      uint32        p_ulContractLife);

typedef struct
{
	uint8		m_aIpv6SrcAddr[16];    
	uint8		m_aIpv6DstAddr[16];    
	uint16_t	m_u16PayloadSize;
	uint8		m_aPadding[3]; 
	uint8		m_ucNextHeader; // 17
	//uint16		m_unUdpSPort;
	//uint16		m_unUdpDPort;
	//uint16		m_u16UdpLen;
	//uint16		m_u16UdpCkSum;
}__attribute__((packed)) UdpIPv6PseudoHeader; 


typedef struct  
{
	uint8		m_ucVersionAndTrafficClass;
	uint8		m_aFlowLabel[3];
	uint16_t	m_u16PayloadSize;
	uint8		m_ucNextHeader;	
	uint8		m_ucHopLimit;
	uint8		m_ucSrcAddr[16];	///< IPv6 address of the source of the packet
	uint8		m_ucDstAddr[16];	///< IPv6 address of the destination of the packet
} __attribute__((__packed__)) IPv6IPHeader;

typedef struct  
{
	uint16		m_unUdpSPort;
	uint16		m_unUdpDPort;
	uint16		m_u16UdpLen;
	uint16		m_u16UdpCkSum;
} __attribute__((__packed__)) IPv6UDPHeader;

typedef struct
{
    uint8  m_uc250msStep;
    uint8  m_ucSecondsFromLastSync; //m_ucSecondsFromLastSync still needed for filtering DAUX syncs 
    int16  m_nCorrection;
    
    // for use with ISA100 clock correction mechanism    
    uint16 m_unMaxClkTimeout;     
    uint16 m_unClkTimeout;
    uint8  m_ucActiveClkInterogationCtr;
} TAI_STRUCT;

/// this is the structure which holds a packet about to go on the IsaSocket
typedef struct
{
	IPv6IPHeader m_stIPHeader;
	struct TIPPayload
	{
		IPv6UDPHeader	m_stUDPHeader;	
		uint8			m_aUDPPayload[64*1024];	///< max possible UDP payload: 64k. TBD: can we use a smaller value? 
	} __attribute__((__packed__)) m_stIPPayload;	
}__attribute__((__packed__)) IPv6Packet;

/// Attempts to receive a UDP message. If message is received in specified timeout, it is being sent up the stack
void	RecvUDP( int p_unTimeoutUSec );

/// Send a UDP message. Return non-zero if the messages was sent, 0 if the send failed
int		SendUDP( IPv6Packet *msg, size_t len );

int		DMAP_BBR_Sniffer( void );

/// Callback to indicate request timeout. Called from ISA100 stack
/// TAKE CARE: we may need some additional parameters - device address?
void UAP_DataTimeout( uint16 p_ushAppHandle, uint8 p_ucSrcSAP, uint8 p_ucDstSAP, uint8 p_ucSFC );

void SLME_PrintKeys(void);

/// Format a IPv6 as XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX
/// @param p_pIPv6 must allow reading 16 bytes
/// @param p_pDstBuf must allow writing 40 bytes
/// @note used by SLME_PrintKeys, DMO_PrintContracts, NLME_PrintContracts NLME_PrintRoutes
void FormatIPv6( const byte * p_pIPv6, char * p_pDstBuf );
		
extern struct timeval tvNow;
extern TAI_STRUCT  g_stTAI;
extern uint16 g_unDllSubnetId;
extern uint16 g_unGEspecialTimeout;

////////////////////////////////////////////////////////////////////////////////
/// @brief C++ linkage methods, unknonwn to the stack
////////////////////////////////////////////////////////////////////////////////

/// The callbacks have C++ linkage, being called from porting.cpp
typedef void (*PFNCompleteContractRequest)( uint16 p_nContractId, DMO_CONTRACT_ATTRIBUTE* p_pNetAddr );
typedef void (*PFNUAP_DataTimeout)( uint16 p_ushAppHandle, uint8 p_ucSrcSAP, uint8 p_ucDstSAP, uint8 p_ucSFC );
typedef void (*PFNUAP_OnJoin)( bool p_bJoined );
typedef void (*PFNAlertAck)( uint8 p_ucAlertID );

/// Initializes the global variables from this file to the proper values
/// Start listening on p_ushPort UDP
/// Return true if init was successfull,
/// Return false on error (example: bind error)
/// LibIsaInit failure is FATAL error, the application should stop in this case
int LibIsaInit( const uint8* p_aOwnIPv6, const uint8* p_aSMIPv6, unsigned short p_ushPort,
		const uint8* p_aOwnEUI64, const uint8* p_aSecManEUI64, const uint8* p_aJoinAppKey,
		uint8 p_ucPingTimeoutConf, int p_nDeviceType, unsigned short p_unDllSubnetId, int p_nCrtUTCAdj );

/// Configures on the fly some global variables of the stack. Useful to tune the UNJOIN detection timeout and the number of retries without unjoining :)
void LibIsaConfig( uint8 p_ucSMLinkTimeoutConf, uint8 p_ucMaxRetries = DMO_DEF_MAX_CLNT_SRV_RETRY, uint16 p_unRetryTimeout = DMO_DEF_MAX_RETRY_TOUT_INT, uint16 p_unGEspecialTimeout = GE_SPECIAL_TIMEOUT);

/// Configures on the fly the current UTC adjustment in the stack
void LibIsaSetUTCAdj(int p_nCrtUTCAdj);

/// Provision the DMAP. Only GW/BBR needs to call it
void LibIsaProvisionDMAP( const char * p_szRevision, const char * p_szModel, const char * p_szTag );
		
/// Register callbacks - add here all necessary callbacks
/// SM MUST pass only p_pfnDataTimeout - callback for REQuest timeouts.
/// SM MUST NOT pass parameter p_pfnCompleteContractRequest
/// TODO: this function should be joind with LibIsaInit
void LibIsaRegisterCallbacks( PFNUAP_DataTimeout p_pfnTimeout, PFNUAP_OnJoin p_pfnOnjoin = NULL, PFNCompleteContractRequest p_pfnContractComplete = NULL );

/// Wait p_nTimeoutUSec microSec (10^-6) for activity on UDP socket
/// Returns false on timeout, true when there is activity on socket
int LibIsaCheckRecv( int p_unTimeoutUSec );

void LibIsaShutdown( void );

/// Search original APDU by application handle and Source TSAP ID (to be used on timeout)
/// This is a hybrid between ASLDE_GetMyAPDU and ASLDE_SearchOriginalRequest, with the difference
/// that those are used for normal RX flow, while this is on the timeout flow. Also note that the
/// APDU_IDTF is filled with TX information, not RX.
uint8 * ASLDE_GetMyOriginalTxAPDU( uint16 p_ushAppHandle, uint8 p_ucTSAPID, APDU_IDTF* p_pIdtf );

#endif	// _PORTING_H_
