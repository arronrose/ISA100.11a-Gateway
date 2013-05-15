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

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Ion Ticus
/// Date:         January 2008
/// Description:  Implements transport layer data entity from ISA100 standard
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <netinet/in.h>

#include "tlde.h"
#include "tlme.h"
#include "slme.h"
#include "nlme.h"
#include "aslsrvc.h"
#include "mlde.h"
#include "dmap.h"
#include "provision.h"
#include "asm.h"
#include "porting.h"
#include "../../Shared/DurationWatcher.h"


#ifdef UT_ACTIVED  // unit testing support
#if( (UT_ACTIVED == UT_TL_ONLY) || (UT_ACTIVED == UT_NL_TL) )
#include "../UnitTests/unit_test_TLDE.h"
UINT_TEST_TLDE_STRUCT g_stUnitTestTLDE;
#endif
#endif // UT_ACTIVED


#define DUPLICATE_KEY_SIZE 26 // m_ucCounter[1],m_ucTimeAndCounter[1],srcaddr[16],senderTAI[4],srcport[2],dstport[2];
#define DUPLICATE_ARRAY_SIZE (0)

struct  TDuplicateHistEntry
{
	int		m_nTime;
	uint8	m_aKey[DUPLICATE_KEY_SIZE];
};

static int s_nDuplicateHistTimeWindow;		//60 seconds
static int s_nDuplicateHistSize;			// 60 seconds * 50 messages/second

static TDuplicateHistEntry* s_aDuplicateHistArray = NULL;
int		s_unDuplicateIdx = 0;

uint8  TLDE_IsDuplicate( const uint8 * p_pKey, int p_nCrtTime );



typedef struct
{
	uint8 m_aIpv6SrcAddr[16];    
	uint8 m_aIpv6DstAddr[16];    
	union
	{
		uint8 m_aPadding[3];
		uint16 m_unPadding;
	}__attribute__((packed));
	uint8 m_ucNextHdr; // 17
	union
	{
		uint8 m_aSrcPort[2];
		uint16 m_unSrcPort;
	}__attribute__((packed));
	union
	{
		uint8 m_aDstPort[2];
		uint16 m_unDstPort;
	}__attribute__((packed));
	uint8 m_aTLSecurityHdr[4];
}__attribute__((packed, aligned(4))) IPV6_PSEUDO_HDR; // encrypted will avoid check sum since is part of MIC

typedef struct
{
	uint16 m_unUdpSPort;
	uint16 m_unUdpDPort;
	uint16 m_unUdpLen;
	uint8 m_aUdpCkSum[2];
	uint8 m_ucSecurityCtrl;    // must be SECURITY_CTRL_ENC_NONE or SECURITY_ENC_MIC_32
	uint8 m_ucTimeAndCounter;    
	uint8 m_ucCounter;    
} __attribute__((packed))TL_HDR_PLAIN_OR_ENC_SINGLE_KEY; // plain request mandatory check sum

typedef struct
{
	uint16 m_unUdpSPort;
	uint16 m_unUdpDPort;
	uint16 m_unUdpLen;	
	uint16 m_unUdpCkSum;
	uint8 m_ucSecurityCtrl;   // must be SECURITY_CTRL_ENC_MIC_32
	uint8 m_ucKeyIdx;    
	uint8 m_ucTimeAndCounter;    
	uint8 m_ucCounter;    
} __attribute__((packed))TL_HDR_ENC_MORE_KEYS; // encrypted will avoid check sum since is part of MIC

typedef union
{
	TL_HDR_PLAIN_OR_ENC_SINGLE_KEY m_stPlain;
	TL_HDR_PLAIN_OR_ENC_SINGLE_KEY m_stEncOneKey;
	TL_HDR_ENC_MORE_KEYS           m_stEncMoreKeys;
} __attribute__((packed, aligned(4)))TL_HDR;

#define TLDE_TIME_MAX_DIFF 2


const uint8 c_aLocalLinkIpv6Prefix[8] = {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8 c_aIpv6ConstPseudoHdr[8] = { 0, 0, 0, UDP_PROTOCOL_CODE, 0xF0, 0xB0, 0xF0, 0xB0 };

uint8 TLDE_decryptTLPayload( const SLME_KEY * p_pKey,
							uint32   p_ulIssueTime,
							const IPV6_PSEUDO_HDR * p_pAuth,
							uint16   p_unSecHeaderLen,
							uint16   p_unTLDataLen,
							void *   p_pTLData );

void TLDE_encryptTLPayload( uint16                  p_unAppDataLen, 
						   void *                  p_pAppData, 
						   const IPV6_PSEUDO_HDR * p_pAuth, 
						   uint16                  p_unSecHeaderLen,
						   const uint8 *           p_pKey, 
						   uint32                  p_ulCrtTai );

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  It is used by the Application Layer to send an packet to one device
/// @param  p_pEUI64DestAddr     - final EUI64 destination
/// @param  p_unContractID       - contract ID
/// @param  p_ucPriorityAndFlags - message priority + DE + ECN
/// @param  p_unAppDataLen  - APP data length
/// @param  p_pAppData      - APP data buffer
/// @param  p_appHandle     - app level handler
/// @param  p_uc4BitSrcPort - source port (16 bit port supported only)
/// @param  p_uc4BitDstPort - destination port (16 bit port supported only)
/// @return none
/// @remarks
///      Access level: user level
///      Context: After calling of that function a NLDE_DATA_Confirm has to be received.
///      Obs: !!! p_pAppData can be altered by TLDE_DATA_Request and must have size at least TL_MAX_PAYLOAD+16
///      On future p_uc4BitSrcPort must be maped via a TSAP id to a port (something like socket id)
///      but that is not clear specified on ISA100
////////////////////////////////////////////////////////////////////////////////////////////////////
void TLDE_DATA_Request(   const uint8* p_pEUI64DestAddr,
					NLME_CONTRACT_ATTRIBUTES * p_pContract,
					   uint8        p_ucPriorityAndFlags,
					   uint16       p_unAppDataLen,
					   void *       p_pAppData,
					   HANDLE       p_appHandle,
					   uint8        p_uc4BitSrcPort,
					   uint8        p_uc4BitDstPort,
					   uint8        p_ucAllowEncryption)
{
	TL_HDR          stHdr;
	IPV6_PSEUDO_HDR stPseudoHdr;
	//char szLogString[100];

	uint16 unHdrLen;
	uint8 ucKeysNo;
	const SLME_KEY * pKey = NULL;
	bool bEvenHeader;

	//WATCH_DURATION_INIT_DEF(oDurationWatcher);

	memcpy( stPseudoHdr.m_aPadding, c_aIpv6ConstPseudoHdr, sizeof(c_aIpv6ConstPseudoHdr) ) ;
	stPseudoHdr.m_aSrcPort[1] |= p_uc4BitSrcPort;
	stPseudoHdr.m_aDstPort[1] |= p_uc4BitDstPort;

	stHdr.m_stPlain.m_unUdpSPort = htons(0xF0B0 | p_uc4BitSrcPort);
	stHdr.m_stPlain.m_unUdpDPort = htons(0xF0B0 | p_uc4BitDstPort);

	LOG_ISA100( LOGLVL_DBG, "TL-send: C %d Prio:%02X Ports:%04X,%04X Data(%d):%s", p_pContract->m_unContractID, p_ucPriorityAndFlags, 0xF0B0 | p_uc4BitSrcPort, 0xF0B0 | p_uc4BitDstPort, 
					p_unAppDataLen, GetHex( p_pAppData, p_unAppDataLen,' ') );
	
	if(p_pEUI64DestAddr)
	{
		LOG_HEX_ISA100( LOGLVL_ERR, "TL-send: ERR: Can't send by EUI64 address:", p_pEUI64DestAddr, 8);
		return;
	}

	memcpy( stPseudoHdr.m_aIpv6DstAddr, p_pContract->m_aDestAddress, 16 );
	TLME_SetPortInfo( stPseudoHdr.m_aIpv6DstAddr, stPseudoHdr.m_unSrcPort, TLME_PORT_TPDU_OUT );

	//WATCH_DURATION_DEF(oDurationWatcher);

	if( p_ucAllowEncryption && g_stDSMO.m_ucTLSecurityLevel != SECURITY_NONE )
	{
		pKey = SLME_FindTxKey( stPseudoHdr.m_aIpv6DstAddr, 0xF0B0 | p_uc4BitSrcPort, 0xF0B0 | p_uc4BitDstPort, &ucKeysNo );
		if( !pKey )
		{
			LOG_ISA100( LOGLVL_ERR, "TL-send: ERR: no key found for C %d, src %s SPort:%X DPort:%X",
				p_pContract->m_unContractID, GetHexC( stPseudoHdr.m_aIpv6DstAddr, 16 ), 0xF0B0 | p_uc4BitSrcPort, 0xF0B0 | p_uc4BitDstPort );
			TLDE_DATA_Confirm( p_appHandle, SFC_NO_KEY );
			return;
		}
	}

	TLME_SetPortInfo( stPseudoHdr.m_aIpv6DstAddr, stPseudoHdr.m_unSrcPort, TLME_PORT_TPDU_OUT_OK );
	memcpy( stPseudoHdr.m_aIpv6SrcAddr, g_aIPv6Address, 16 );

	uint32 ulCrtTai = MLSM_GetCrtTaiSec();
	uint8  ucTimeAndCounter = (uint8)((ulCrtTai << 2) | g_stTAI.m_uc250msStep); // add 250ms time sense

	g_ucMsgIncrement++;

	//WATCH_DURATION_DEF(oDurationWatcher);
	if( !pKey ) // not encryption
	{
		stPseudoHdr.m_aTLSecurityHdr[0] = stHdr.m_stPlain.m_ucSecurityCtrl = SECURITY_NONE;
		stPseudoHdr.m_aTLSecurityHdr[1] = stHdr.m_stPlain.m_ucTimeAndCounter = ucTimeAndCounter;
		stPseudoHdr.m_aTLSecurityHdr[2] = stHdr.m_stPlain.m_ucCounter = g_ucMsgIncrement;
		stPseudoHdr.m_aTLSecurityHdr[3] = (p_unAppDataLen ? *((uint8*)p_pAppData) : 0 );        

		unHdrLen = sizeof( stHdr.m_stPlain );
		bEvenHeader = false;
	}
	else // encryption
	{
		if( ucKeysNo > 1 )
		{
			stHdr.m_stEncMoreKeys.m_ucSecurityCtrl = SECURITY_CTRL_ENC_MIC_32;
			stHdr.m_stEncMoreKeys.m_ucKeyIdx = pKey->m_ucKeyID;
			stHdr.m_stEncMoreKeys.m_ucTimeAndCounter = ucTimeAndCounter; // add 250ms time sense
			stHdr.m_stEncMoreKeys.m_ucCounter = g_ucMsgIncrement;

			unHdrLen = sizeof( stHdr.m_stEncMoreKeys );
			bEvenHeader = true;
		}
		else
		{
			stHdr.m_stEncOneKey.m_ucSecurityCtrl = SECURITY_ENC_MIC_32;
			stHdr.m_stEncOneKey.m_ucTimeAndCounter = ucTimeAndCounter; // add 250ms time sense
			stHdr.m_stEncOneKey.m_ucCounter = g_ucMsgIncrement;

			unHdrLen = sizeof( stHdr.m_stEncOneKey );
			bEvenHeader = false;
		}

		memcpy( stPseudoHdr.m_aTLSecurityHdr, &stHdr.m_stEncMoreKeys.m_ucSecurityCtrl, unHdrLen-8 );

		TLDE_encryptTLPayload(p_unAppDataLen, p_pAppData, &stPseudoHdr, unHdrLen, pKey->m_aKey, ulCrtTai );

		p_unAppDataLen += 4; // add MIC_32 size
		if( !bEvenHeader ) // type SECURITY_ENC_MIC_32
		{
			stPseudoHdr.m_aTLSecurityHdr[3] = *((uint8*)p_pAppData);
		}
	}

	uint32 ulUdpCkSum = p_unAppDataLen + unHdrLen;

	//WATCH_DURATION_DEF(oDurationWatcher);

	// make happy check sum over UDP and add twice udp payload size
	stHdr.m_stPlain.m_unUdpLen = stPseudoHdr.m_unPadding = htons((uint16)ulUdpCkSum);

	ulUdpCkSum = TLDE_IcmpInterimChksum( (const uint8*)&stPseudoHdr, sizeof(stPseudoHdr), ulUdpCkSum );

	if( p_unAppDataLen > !bEvenHeader )
	{
		ulUdpCkSum = TLDE_IcmpInterimChksum( (uint8*)p_pAppData+!bEvenHeader, p_unAppDataLen-!bEvenHeader, ulUdpCkSum );
	}
	ulUdpCkSum = TLDE_IcmpGetFinalCksum( ulUdpCkSum );

	//LOG_HEX_ISA100( LOGLVL_DBG, "TL-send-raw: Pseudoheader:", (const uint8*)&stPseudoHdr, sizeof(stPseudoHdr));
	//LOG_HEX_ISA100( LOGLVL_DBG, "TL-send-raw: AppData:", (const uint8*)p_pAppData, p_unAppDataLen );

	stHdr.m_stPlain.m_aUdpCkSum[0] = ulUdpCkSum >> 8;
	stHdr.m_stPlain.m_aUdpCkSum[1] = ulUdpCkSum & 0xFF;

	if( !bEvenHeader ) // not encryption
	{
		LOG_ISA100( LOGLVL_DBG, "TL-send-hdr: Security:%02X TC:%02X C:%02X UDP:%s", stHdr.m_stPlain.m_ucSecurityCtrl, 
						stHdr.m_stPlain.m_ucTimeAndCounter, stHdr.m_stPlain.m_ucCounter, 
						GetHex((uint8*)&stHdr, 8, ' '));
	}else{// encryption
		LOG_ISA100( LOGLVL_DBG, "TL-send-hdr: Security:%02X Key:%d TC:%02X C:%02X UDP:%s", stHdr.m_stEncMoreKeys.m_ucSecurityCtrl, 
						stHdr.m_stEncMoreKeys.m_ucKeyIdx, stHdr.m_stEncMoreKeys.m_ucTimeAndCounter, stHdr.m_stEncMoreKeys.m_ucCounter,
						GetHex((uint8*)&stHdr, 8, ' '));
	}
	//LOG_HEX_ISA100( LOGLVL_DBG, szLogString, (uint8*)&stHdr, 8 );

	//WATCH_DURATION_DEF(oDurationWatcher);

	NLDE_DATA_RequestMemOptimzed(
		stPseudoHdr.m_aIpv6DstAddr,
		p_pContract,
		p_ucPriorityAndFlags,
		unHdrLen,
		&stHdr,
		p_unAppDataLen,
		p_pAppData,
		p_appHandle
		);
	//WATCH_DURATION_DEF(oDurationWatcher);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  It is invoked by Network Layer to notify about result code of a data request
/// @param  p_hTLHandle - transport layer handler (used to find correspondent request)
/// @param  p_ucStatus  - request status
/// @return none
/// @remarks
///      Access level: Interrupt level
///      Context: After any NLDE_DATA_Request
////////////////////////////////////////////////////////////////////////////////////////////////////
void NLDE_DATA_Confirm (  HANDLE       p_hTLHandle,
						uint8        p_ucStatus )
{
	TLDE_DATA_Confirm ( p_hTLHandle, p_ucStatus );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  It is invoked by Network Layer to notify transport layer about new packet
/// @param  p_pSrcAddr  - The IPv6 or EUI64 source address (if first byte is 0xFF -> last 8 bytes are EUI64)  
/// @param  p_unTLDataLen - network layer payload length of received message
/// @param  p_pTLData     - network layer payload data of received message
/// @param  m_ucPriorityAndFlags - message priority + DE + ECN
/// @return none
/// @remarks
///      Access level: User level
///      Obs: p_pTLData will be altered on this function
////////////////////////////////////////////////////////////////////////////////////////////////////
void NLDE_DATA_Indication (   const uint8 * p_pSrcAddr,
						   uint16        p_unTLDataLen,
						   void *        p_pTLData,
						   uint8         m_ucPriorityAndFlags )

{
	uint8  ucTimeTraveling;
	struct timeval	tvTxTime;
	const SLME_KEY * pKey = NULL;
	uint32 ulCrtTime;
	//char szLogString[100];
	uint8 msgKey[DUPLICATE_KEY_SIZE]; 

	IPV6_PSEUDO_HDR stPseudoHdr;

	memcpy( stPseudoHdr.m_aPadding, c_aIpv6ConstPseudoHdr, sizeof(c_aIpv6ConstPseudoHdr) ) ;
	stPseudoHdr.m_unSrcPort = ((TL_HDR*)p_pTLData)->m_stPlain.m_unUdpSPort;
	stPseudoHdr.m_unDstPort = ((TL_HDR*)p_pTLData)->m_stPlain.m_unUdpDPort;

	memcpy( stPseudoHdr.m_aIpv6SrcAddr, p_pSrcAddr, 16 );
	memcpy( stPseudoHdr.m_aIpv6DstAddr, g_aIPv6Address, 16 );
	TLME_SetPortInfo( p_pSrcAddr, stPseudoHdr.m_unDstPort, TLME_PORT_TPDU_IN );

	if( (stPseudoHdr.m_aDstPort[1] & 0x0F) >= MAX_NB_OF_PORTS ) // invalid application port
	{
		TLME_Alert( TLME_ILLEGAL_USE_OF_PORT, stPseudoHdr.m_aDstPort, 2 );
		return;
	}

	ulCrtTime = MLSM_GetCrtTaiSec() + TLDE_TIME_MAX_DIFF;
	ucTimeTraveling = ( ulCrtTime & 0x3F);

	//  LOG_HEX_ISA100( LOGLVL_DBG, "TL-recv from:", p_pSrcAddr, 16);
	LOG_ISA100( LOGLVL_DBG, "TL-recv %db, containing header:", p_unTLDataLen, GetHex(p_pTLData, sizeof(TL_HDR), ' '));
	//LOG_HEX_ISA100( LOGLVL_DBG, szLogString, p_pTLData, sizeof(TL_HDR) );

	if( p_unTLDataLen <= sizeof(((TL_HDR*)p_pTLData)->m_stPlain) ) 
	{
		LOG_ISA100( LOGLVL_ERR, "ERROR: datagram length too small: %d", p_unTLDataLen);
		return;
	}

	stPseudoHdr.m_unPadding = htons( p_unTLDataLen );

	uint32 ulUdpCkSum = TLDE_IcmpInterimChksum( (const uint8*)&stPseudoHdr, sizeof(stPseudoHdr)-sizeof(stPseudoHdr.m_aTLSecurityHdr), p_unTLDataLen );
	stPseudoHdr.m_unPadding = 0; // if this is encrypted, the MIC wants 0

	//	LOG_HEX_ISA100( LOGLVL_DBG, "Pseudoheader:", (const uint8*)&stPseudoHdr, sizeof(stPseudoHdr)-sizeof(stPseudoHdr.m_aTLSecurityHdr));
	//	LOG_HEX_ISA100( LOGLVL_DBG, "cksum:", &ulUdpCkSum, 2);
	// compute the checksum on NL payload but escape UDP hdr (src port, dst port, len and checksum bytes)
	ulUdpCkSum = TLDE_IcmpInterimChksum( &((TL_HDR*)p_pTLData)->m_stPlain.m_ucSecurityCtrl, p_unTLDataLen-8, ulUdpCkSum );

	ulUdpCkSum = TLDE_IcmpGetFinalCksum( ulUdpCkSum );

	//	LOG_HEX_ISA100( LOGLVL_DBG, "data:", &((TL_HDR*)p_pTLData)->m_stPlain.m_ucSecurityCtrl, p_unTLDataLen-8);
	//	LOG_HEX_ISA100( LOGLVL_DBG, "cksum:", &ulUdpCkSum, 2);
	if(     ((TL_HDR*)p_pTLData)->m_stPlain.m_aUdpCkSum[0] != (ulUdpCkSum >> 8)
		||  ((TL_HDR*)p_pTLData)->m_stPlain.m_aUdpCkSum[1] != (ulUdpCkSum & 0xFF) )
	{
		LOG_ISA100( LOGLVL_ERR, "WARNING: Invalid UDP checksum: computed %04X, in packet %02X%02X", ulUdpCkSum,
			((TL_HDR*)p_pTLData)->m_stPlain.m_aUdpCkSum[0], ((TL_HDR*)p_pTLData)->m_stPlain.m_aUdpCkSum[1]);
		return;
	}

	if( g_stDSMO.m_ucTLSecurityLevel != SECURITY_NONE && ((TL_HDR*)p_pTLData)->m_stPlain.m_ucSecurityCtrl == SECURITY_NONE )
	{
		LOG_ISA100( LOGLVL_DBG,
			"WARNING: security mismatch: TLSecurityLevel=0x%02X but packet security=0x%02X", 
			g_stDSMO.m_ucTLSecurityLevel, ((TL_HDR*)p_pTLData)->m_stPlain.m_ucSecurityCtrl);
		TLME_Alert( TLME_TPDU_OUT_OF_SEC_POL, (uint8*)p_pTLData, (p_unTLDataLen < 40 ? p_unTLDataLen : 40) );
	}

	memcpy( stPseudoHdr.m_aTLSecurityHdr, &((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_ucSecurityCtrl, sizeof(stPseudoHdr.m_aTLSecurityHdr) );
	switch(((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_ucSecurityCtrl){
		case SECURITY_ENC_MIC_32:
		case SECURITY_CTRL_ENC_MIC_32:
			uint16 unHeaderLen;
			if( ((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_ENC_MIC_32 )
			{
				unHeaderLen = sizeof(((TL_HDR*)p_pTLData)->m_stEncOneKey);
				ucTimeTraveling -= ((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucTimeAndCounter >> 2;
				tvTxTime.tv_usec = (((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucTimeAndCounter & 0x3)<<8 | ((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucCounter;
				uint8 ucTmp;
				//LOG_ISA100( LOGLVL_DBG, "NLDE_DATA_Indication checkpoint: calling SLME_FindKeyTx");
				pKey = SLME_FindTxKey( stPseudoHdr.m_aIpv6SrcAddr,
					ntohs(((TL_HDR*)p_pTLData)->m_stEncOneKey.m_unUdpDPort),//take care: on RX the ports are reversed!
					ntohs(((TL_HDR*)p_pTLData)->m_stEncOneKey.m_unUdpSPort),
					&ucTmp );
				msgKey[0] = ((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucCounter;
				msgKey[1] = ((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucTimeAndCounter;
			}
			else // ((TL_HDR*)p_pTLData)->m_stEncOneKey.m_ucSecurityCtrl == SECURITY_CTRL_ENC_MIC_32
			{
				unHeaderLen = sizeof(((TL_HDR*)p_pTLData)->m_stEncMoreKeys);
				ucTimeTraveling -= ((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_ucTimeAndCounter >> 2;
				tvTxTime.tv_usec = (((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_ucTimeAndCounter & 0x3)<<8 | ((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_ucCounter;
				//LOG_ISA100( LOGLVL_DBG, "NLDE_DATA_Indication checkpoint: calling SLME_FindKey");
				pKey = SLME_FindKey( stPseudoHdr.m_aIpv6SrcAddr,
					ntohs(((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_unUdpDPort),//take care: on RX the ports are reversed!
					ntohs(((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_unUdpSPort),
					((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_ucKeyIdx,
					SLM_KEY_USAGE_SESSION);
				msgKey[0] = ((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_ucCounter;
				msgKey[1] = ((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_ucTimeAndCounter;
			}
			if( !pKey )
			{
				LOG_ISA100( LOGLVL_ERR, "ERROR: security key not found");
				TLME_Alert( TLME_TPDU_OUT_OF_SEC_POL, (uint8*)p_pTLData, (p_unTLDataLen < 40 ? p_unTLDataLen : 40) );
				return;
			}
			if( p_unTLDataLen <= unHeaderLen+4 ) //  +MIC
			{
				LOG_ISA100( LOGLVL_ERR, "ERROR: encrypted datagram length too small: %d", p_unTLDataLen);
				return;
			}
			if( ucTimeTraveling & 0x80 )
			{
				ucTimeTraveling += 0x40;
			}
			//LOG_HEX_ISA100( LOGLVL_DBG, "NLDE_DATA_Indication checkpoint: key=",pKey->m_aKey, sizeof(pKey->m_aKey) );
			if( AES_SUCCESS != TLDE_decryptTLPayload( pKey, ulCrtTime-ucTimeTraveling, &stPseudoHdr, unHeaderLen, p_unTLDataLen, p_pTLData ) )
			{
				SLME_TL_FailReport();
				LOG_ISA100( LOGLVL_ERR, "ERROR: invalid MIC");
				return;
			}
			p_pTLData = ((uint8*)p_pTLData) + unHeaderLen;
			p_unTLDataLen -= unHeaderLen+4;
			break;
		case SECURITY_NONE:
			ucTimeTraveling -= ((TL_HDR*)p_pTLData)->m_stPlain.m_ucTimeAndCounter >> 2;
			tvTxTime.tv_usec = (((TL_HDR*)p_pTLData)->m_stPlain.m_ucTimeAndCounter & 0x3)<<8 | ((TL_HDR*)p_pTLData)->m_stPlain.m_ucCounter;
			if( ucTimeTraveling & 0x80 )
			{
				ucTimeTraveling += 0x40;
			}
			msgKey[0] = ((TL_HDR*)p_pTLData)->m_stPlain.m_ucCounter;
			msgKey[1] = ((TL_HDR*)p_pTLData)->m_stPlain.m_ucTimeAndCounter;
			p_pTLData = ((uint8*)p_pTLData) + sizeof( ((TL_HDR*)p_pTLData)->m_stPlain );
			p_unTLDataLen -= sizeof( ((TL_HDR*)p_pTLData)->m_stPlain );
			break;
		default:
			LOG_ISA100( LOGLVL_ERR, "WARNING: Unsupported security 0x%02X", ((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_ucSecurityCtrl);
			LOG_ISA100( LOGLVL_ERR, "SPort: %X DPort: %X, len: 0x%02X, cksum: 0x%02X", ntohs(((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_unUdpSPort),
				ntohs(((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_unUdpDPort), ntohs(((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_unUdpLen), 
				ntohs(((TL_HDR*)p_pTLData)->m_stEncMoreKeys.m_unUdpCkSum));
			TLME_Alert( TLME_TPDU_OUT_OF_SEC_POL, (uint8*)p_pTLData, (p_unTLDataLen < 40 ? p_unTLDataLen : 40) );
			return;
	}
	tvTxTime.tv_sec = ulCrtTime-ucTimeTraveling;
	if( ucTimeTraveling > TLDE_TIME_MAX_DIFF )
	{
		ucTimeTraveling -= TLDE_TIME_MAX_DIFF;
	}
	else
	{
		ucTimeTraveling = 0;
	}
	memcpy( msgKey+2, p_pSrcAddr, 16 );
	ulCrtTime -= ucTimeTraveling;
	long remainder = 0;
	if(tvTxTime.tv_usec >= 1000)
	{
		remainder = 1025 - tvTxTime.tv_usec;
		tvTxTime.tv_usec = 1000;
	}
	tvTxTime.tv_usec *= 1000;
	tvTxTime.tv_usec -= remainder;

	memcpy( msgKey+18, &ulCrtTime, sizeof(ulCrtTime));
	memcpy( msgKey+22, stPseudoHdr.m_aSrcPort, 2*sizeof(uint16));

	if( TLDE_IsDuplicate( msgKey, ulCrtTime ))
	{
		LOG_HEX_ISA100( LOGLVL_INF, "WARNING TL discarding DUPLICATE from:", p_pSrcAddr, 16);
		LOG_HEX_ISA100( LOGLVL_DBG, "WARNING TL duplicate key:", msgKey, sizeof(msgKey));
		return;
	}
	TLME_SetPortInfo( p_pSrcAddr, stPseudoHdr.m_unDstPort, TLME_PORT_TPDU_IN_OK );

	TLDE_DATA_Indication(	p_pSrcAddr,
		stPseudoHdr.m_aSrcPort[1] & 0x0F,
		m_ucPriorityAndFlags,
		p_unTLDataLen,
		(uint8*)p_pTLData,
		stPseudoHdr.m_aDstPort[1] & 0x0F,
		ucTimeTraveling,
		&tvTxTime,
		stPseudoHdr.m_aTLSecurityHdr[0] );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  decrypt transport layer payload in same buffer
/// @param  p_pKey        - key entry
/// @param  p_ulIssueTime - time when message was issued on remote source
/// @param  p_pAuth       - auth data (IPv6 pseudo header + security header)
/// @param  p_unTLDataLen - transport layer data length
/// @param  p_pTLData     - transport layer data buffer
/// @return none
/// @remarks
///      Access level: User level
///      Obs: !! use 1.2k stack !! p_pTLData will be altered on this function
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 TLDE_decryptTLPayload( const SLME_KEY * p_pKey,
							uint32   p_ulIssueTime,
							const IPV6_PSEUDO_HDR * p_pAuth,
							uint16                  p_unSecHeaderLen,
							uint16   p_unTLDataLen,
							void *   p_pTLData )
{
	uint8  aNonce[13];
	uint16 unAuthLen = sizeof(IPV6_PSEUDO_HDR) - 4 + p_unSecHeaderLen - 8;

	memcpy(aNonce, p_pKey->m_aIssuerEUI64, 8);    
	aNonce[8] = (uint8)(p_ulIssueTime >> 14);
	aNonce[9] = (uint8)(p_ulIssueTime >> 6);
	aNonce[10] = ((uint8*)p_pAuth)[unAuthLen-2];
	aNonce[11] = ((uint8*)p_pAuth)[unAuthLen-1];
	aNonce[12] = 0xFF;

	// skip UDP section from AUTH because that section may be altered by BBR
	return AES_Decrypt_User( p_pKey->m_aKey,
		aNonce,
		(uint8*)p_pAuth,
		unAuthLen,
		((uint8*)p_pTLData) + p_unSecHeaderLen,
		p_unTLDataLen -  p_unSecHeaderLen );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  encrypt transport layer payload in same buffer
/// @param  p_unAppDataLen  - application layer buffer
/// @param  p_pAppData      - application layer data buffer
/// @param  p_pAuth         - auth data (IPv6 pseudo header + security header)
/// @param  p_pKey          - encryption key
/// @param  p_ulCrtTai      - current TAI
/// @return none
/// @remarks
///      Access level: User level
////////////////////////////////////////////////////////////////////////////////////////////////////
void TLDE_encryptTLPayload( uint16 p_unAppDataLen, void * p_pAppData, const IPV6_PSEUDO_HDR * p_pAuth, uint16 p_unSecHeaderLen, const uint8 * p_pKey, uint32 p_ulCrtTai )
{
	uint8  aNonce[13];
	uint16 unAuthLen = sizeof(IPV6_PSEUDO_HDR) - 4 + p_unSecHeaderLen - 8;

	memcpy(aNonce, c_oEUI64BE, 8);
	aNonce[8]  = (uint8)(p_ulCrtTai >> 14);
	aNonce[9]  = (uint8)(p_ulCrtTai >> 6);
	aNonce[10] = ((uint8*)p_pAuth)[unAuthLen-2];
	aNonce[11] = ((uint8*)p_pAuth)[unAuthLen-1];
	aNonce[12] = 0xFF;

	// skip UDP section from AUTH because that section can be altered by BBR
	AES_Crypt_User( p_pKey,
		aNonce,
		(uint8 *) p_pAuth,
		unAuthLen,
		(uint8*)p_pAppData,
		p_unAppDataLen );
}

//-----------------------------------------------------------------------------
/// Process & return the ICMP checksum
/// \param p Pointer to the buffer to process
/// \param len The length of the bufferred data
//-----------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ionut Avram, Ion Ticus
/// @brief  Compute ICMP check sum 
/// @param  p_pData          - buffer
/// @param  p_unDataLen      - buffer length
/// @param  p_ulPrevCheckSum - previous check sum
/// @return interimate check sum
/// @remarks
///      Access level: User level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint32 TLDE_IcmpInterimChksum(const uint8 *p_pData, uint16 p_unDataLen, uint32 p_ulPrevCheckSum )
{
	while(  p_unDataLen--  ) 
	{        
		p_ulPrevCheckSum += ((uint16)*(p_pData++)) << 8;        

		if( !(p_unDataLen --) )
			break;

		p_ulPrevCheckSum += *(p_pData++);        
	}

	return p_ulPrevCheckSum;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ionut Avram, Ion Ticus
/// @brief  Compute final ICMP check sum 
/// @param  p_ulPrevCheckSum - previous check sum
/// @return ICMP check sum
/// @remarks
///      Access level: User level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint16 TLDE_IcmpGetFinalCksum( uint32 p_ulPrevCheckSum )
{
    uint16 unResult = ~((uint16)p_ulPrevCheckSum + (uint16)(p_ulPrevCheckSum >> 16)); 
    return ( !unResult ? 0xFFFF : unResult);
}


int TLDE_Duplicate_Init (int p_nSize, int p_nSeconds)
{
	if (s_aDuplicateHistArray)
	{
		LOG_ISA100(LOGLVL_ERR, "TLDE_Duplicate_Init: already init -> resize");
		
		delete []s_aDuplicateHistArray;

		s_aDuplicateHistArray = NULL;
	}
	s_aDuplicateHistArray = new TDuplicateHistEntry [p_nSize];

	memset(s_aDuplicateHistArray,0,p_nSize*sizeof(TDuplicateHistEntry));
	s_nDuplicateHistSize = p_nSize;
	s_nDuplicateHistTimeWindow = p_nSeconds;

	LOG_ISA100(LOGLVL_ERR, "TLDE_Duplicate_Init: to size=%d timeout=%d",s_nDuplicateHistSize, s_nDuplicateHistTimeWindow);
	return 1;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Ion Ticus
/// @brief  Check if message is duplicate at trasnport layer (on security enable only) 
/// @param  p_pKey          - concatenation of significant fields of the message
/// @return 1 if is duplicate, 0 if not
/// @remarks
///      Access level: User level
///       As an alternative keep time generation + IPV6 (6+16 = 22 bytes instead of 4 bytes)
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8  TLDE_IsDuplicate( const uint8 * p_pKey, int p_nCrtTime )
{
	if (s_aDuplicateHistArray == NULL)
	{
		LOG_ISA100(LOGLVL_ERR, "TLDE_IsDuplicate: s_aDuplicateHistArray==NULL probably TLDE_Duplicate_Init not called ");
		return 0;
	}

	int  uIdx;

	for( uIdx = s_unDuplicateIdx-1; uIdx != s_unDuplicateIdx; uIdx -- )
	{
		if (uIdx < 0)
		{
			uIdx = s_nDuplicateHistSize;
			continue;
		}

		if (s_aDuplicateHistArray[uIdx].m_nTime < p_nCrtTime )
		{
			break;
		}

		if( memcmp(p_pKey, s_aDuplicateHistArray [uIdx].m_aKey, DUPLICATE_KEY_SIZE) == 0)
			return 1;
	}

	s_aDuplicateHistArray[s_unDuplicateIdx].m_nTime = p_nCrtTime + s_nDuplicateHistTimeWindow;

	memcpy(s_aDuplicateHistArray[s_unDuplicateIdx].m_aKey, p_pKey, DUPLICATE_KEY_SIZE);

	if( (++s_unDuplicateIdx) >= s_nDuplicateHistSize )
		s_unDuplicateIdx = 0;

	return 0;
}

