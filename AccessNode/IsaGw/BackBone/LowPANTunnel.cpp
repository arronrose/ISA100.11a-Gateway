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

//////////////////////////////////////////////////////////////////////////////
/// @file	LowPANTunnel.cpp
/// @author	Marius Negreanu
/// @date	Wed Nov 28 12:26:59 2007 UTC
/// @brief	Implements the Tunneling class between IPv6 <-> 6lowPAN.
/// @see	CTtyLink
//////////////////////////////////////////////////////////////////////////////

#include "Shared/h.h"
#include "Shared/log_callback.h"
#include "../ISA100/typedef.h"
#include "../ISA100/porting.h"
#include "../ISA100/slme.h"
#include "../ISA100/nlme.h"
#include "../ISA100/Ccm.h"


#include "common/ProtocolLayer.h"
#include "common/EncaseLayer.h"
#include "BackBoneApp.h"

#include "DeFrgmTbl.h"
#include "IsaSocket.h"
#include "BbrTypes.h"
#include "BbrGlobal.h"

#include "LowPANTunnel.h"
#include "MsgQueue.h"
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include "../../Shared/DevMem.h"



///////////////////////////////////////////////////////////////////////////////
/// @brief Default Constructor.
///////////////////////////////////////////////////////////////////////////////
LowPANTunnel::LowPANTunnel( )
	: udp_last_idx(0)
	, m_deFrgmTbl(0)
	, m_myShortAddr(0)
	, m_myIPv6Addr(0)
	, m_SMIPv6Addr(0)
	, m_ttyQ(0)
	, m_trLink(0)
	, m_dgram_tag(0)
	, m_hHandle(0)
	, m_counter(0)
	, m_nLastCmdEngModeHandle(-1)

{
	m_nLastValidMsgTime = time(NULL);
}


///////////////////////////////////////////////////////////////////////////////
/// @brief Destructor.
///////////////////////////////////////////////////////////////////////////////
LowPANTunnel::~LowPANTunnel()
{

	delete m_deFrgmTbl ;


	m_trLink->Close() ;
	delete m_trLink ;

	delete m_ttyQ ;
}

void blink()
{
	CDevMem dev ;
	//devmem 0xF0000A0A 8 0x0F
	sleep (1);
	dev.WriteByte(0xF0000A0A, 0x0f);
	usleep(10000);
	dev.WriteByte(0xF0000A0A, 0x00);
	usleep(10000);
	dev.WriteByte(0xF0000A0A, 0x0f);
	usleep(10000);
	dev.WriteByte(0xF0000A0A, 0x00);
	usleep(10000);
	dev.WriteByte(0xF0000A0A, 0x0f);
	usleep(10000);
	dev.WriteByte(0xF0000A0A, 0x00);
	usleep(10000);
}

int LowPANTunnel::runTRTest()
{
	LOG("runTRTest: start restart TR");
	CDevMem dev ;
	dev.WriteInt(0xF0000810,0x00000000); // off
	sleep(1) ;
	dev.WriteInt(0xF0000810,0x00000024); // on
	sleep(5) ;// the TR takes 5 seconds to boot

	///blink() ; // This might be removed. It's here only to see if the LED blinks

	m_trLink->OpenLink<CTtyLink>(
		g_pApp->m_cfg.getTtyDev()
		, (int)g_pApp->m_cfg.getTtyBauds()
		, 0
		, true) ;

	m_ttyQ = new MsgQueue() ;
	m_ttyQ->SetLink( m_trLink ) ;

	//ResetStack(); // reset on provision request
	//LOG("Backbone up. -> send START to TR");

	clock_t beforeTtyOpen = GetClockTicks() + g_pApp->m_cfg.m_nTrTestInterval *sysconf( _SC_CLK_TCK ) / 1000;
	addStartStopMsg(1);
	m_ttyQ->RetrySend() ;
	bool testOK = false ;
	while ( GetClockTicks() < beforeTtyOpen )
	{
		ProtocolPacket * pkt = m_trLink->Read( 10000 );
		if ( pkt ) { testOK= true ; break ; }
	}
	if ( ! testOK )
	{
		LOG("blink: TR ERROR");
		blink() ;
	}

	m_trLink->Close() ;

	delete m_ttyQ ;
	m_ttyQ=NULL;

	return testOK ;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Create a LowPANTunnel.
/// @details Opens a UDP link to SystemManager and a Serial Link to
/// Transceiver. Creates the Retransmission Queues.
/// @retval 0 success. The links communication links were openned.
/// @retval !0 failure.
///////////////////////////////////////////////////////////////////////////////
int LowPANTunnel::Start( )
{
	int rv =0;
	extern CBackBoneApp* g_pApp;
	memset( m_key, 0,  sizeof(m_key) );	///< @deprecated CRC is used instead of LowPANTunnel::m_key.
	memset( m_nonce, 0, sizeof(m_nonce) );	///< @deprecated CRC is used instead of LowPANTunnel::m_nonce.

	m_deFrgmTbl = new DeFrgmTbl() ;

	m_myIPv6Addr = g_pApp->m_cfg.getBB_IPv6() ;
	m_SMIPv6Addr = g_pApp->m_cfg.getSM_IPv6() ;

	m_trLink = new IsaSocket( g_pApp->m_cfg.UseRawLog() ) ;

	// activate using RUN_TR_TEST=1 in config.ini
	if (g_pApp->m_cfg.runTRTest)
	{
		runTRTest() ;
		return 0;
	}

	# if defined( USE_ENCASE_TTY )
	rv = m_trLink->OpenLink<CTtyLink>(
		g_pApp->m_cfg.getTtyDev()
		, (int)g_pApp->m_cfg.getTtyBauds()
		, 0
		, true) ;
	#elif defined( USE_ENCASE_UDP )
	rv = m_trLink->OpenLink<CUdpLink>(
		"10.32.0.28"
		, g_pApp->m_cfg.getBB_Port()+1
		, g_pApp->m_cfg.getSM_Port()+1
		, true);
	ProtocolPacket *p= new ProtocolPacket();
	p->AllocateCopy( 0, 4 );
	p->PushFront(0x01);
	m_trLink->Write(p);/// Use to trigger listening netcat
	delete p;
	#elif defined( USE_ENCASE_FILE )
	rv = m_trLink->OpenLink<CFileLink>( "/tmp/bone_simfile", O_RDWR, 0, true) ;
	#endif
	//if( !rv )
	//{
	//	ERR("LowPANTunnel.Start: Failed to Open transceiver Link");
	//	return false ;
	//}

	m_ttyQ = new MsgQueue() ;
	m_ttyQ->SetLink( m_trLink ) ;

	//ResetStack(); // reset on provision request
	LOG("Backbone up. -> send START to TR");

	addStartStopMsg(1);

	m_oWaitListTimer.SetTimer(10*1000);

	return true ;
}

void LowPANTunnel::checkNeedAddSMContract()
{
	if(g_unSysMngContractID != INVALID_CONTRACTID)
	{
		return;
	}

	//add fake contract
	g_unSysMngContractID = 1000;
	LOG("checkNeedAddSMContract: add fake SysMngContract %d", g_unSysMngContractID);


	NLME_AddContract(1000, g_pApp->m_cfg.getBB_IPv6(), g_pApp->m_cfg.getSM_IPv6(), 0, 0, MAX_DATAGRAM_SIZE, 0, 0, 0 );
	NLME_CONTRACT_ATTRIBUTES_FOR_SETROW stContractInfo = {{0,},};

	stContractInfo.m_stIdx.m_nContractId = ntohs(g_unSysMngContractID);
	memcpy(stContractInfo.m_stIdx.m_aOwnIPv6, g_pApp->m_cfg.getBB_IPv6(), IPv6_ADDR_LENGTH);

	stContractInfo.m_stData.m_nContractId = stContractInfo.m_stIdx.m_nContractId;
	memcpy(stContractInfo.m_stData.m_aOwnIPv6, g_pApp->m_cfg.getBB_IPv6(), IPv6_ADDR_LENGTH);
	memcpy(stContractInfo.m_stData.m_aPeerIPv6, g_pApp->m_cfg.getSM_IPv6(), IPv6_ADDR_LENGTH);
	stContractInfo.m_stData.m_cFlags = 0;

	NLME_setContract(0, (uint8 *)&stContractInfo, MAX_DATAGRAM_SIZE, 0, 0, 0);

	//if (NLME_FindContract(g_unSysMngContractID))
	//{
	//	LOG("WARN: contract exist");
	//	return;
	//}

	LOG("checkNeedAddSMContract: add fake SysMngContract %d", g_unSysMngContractID);
}

void LowPANTunnel::ResetStack()
{
	DMAP_DLMO_ResetStack();

	//add default route to field
	g_stNlme.m_ucEnableDefaultRoute = 1;
	memset(g_stNlme.m_aDefaultRouteEntry, 0, IPv6_ADDR_LENGTH);
	NLME_AddRoute ( g_stNlme.m_aDefaultRouteEntry, g_stNlme.m_aDefaultRouteEntry, 1, OutgoingInterfaceDL );

	//uint8_t pGw[] = {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4E, 0x7C, 0x0A, 0x20, 0x00, 0x1C};

	//NLME_AddRoute ( pGw, pGw, 1, OutgoingInterfaceBBR );
	//add route to SM
	NLME_AddRoute ( g_pApp->m_cfg.getSM_IPv6(), g_pApp->m_cfg.getSM_IPv6(), 64, OutgoingInterfaceBBR );

	g_nATT_TR_Changed = 1;
	// add contract to SM
	checkNeedAddSMContract();

}

#define LinuxBbbrStart	1
#define LinuxBbbrStop	2


int LowPANTunnel::addStartStopMsg (int p_nStart)
{
	uint8_t u8Restart[2];

	u8Restart[0] = PacketMatcher::LOCAL_CMD_BBR_PWR;
	u8Restart[1] = p_nStart ? LinuxBbbrStart : LinuxBbbrStop;

	ProtocolPacket* pPkt = new ProtocolPacket;
	pPkt->AllocateCopy(u8Restart,sizeof(u8Restart), sizeof(LayerMsgHdr), CRC_LENGTH );
	addPktToTty(pPkt, PacketMatcher::PKT_LOCAL_CMD,PRIO_HIGH, INF_LIFETIME_MSG);
	m_ttyQ->RetrySend() ;
	return 1;
}

void LowPANTunnel::sendAttRt ()
{
	LOG("sendAttRt to TR");
	uint16_t pDest[g_stNlme.m_oNlmeRoutesMap.size() + 2 ];
	int nPos = 0;

	uint8_t u8DefaultRoute = OutgoingInterfaceBBR; //BBR

	//[TODO] add code to find default route when new stack is integrated

	if (g_stNlme.m_ucEnableDefaultRoute)
	{
		NLME_ROUTE_ATTRIBUTES * pRoute = NLME_findRoute( g_stNlme.m_aDefaultRouteEntry );
		if (pRoute)
		{
			LOG_HEX("sendAttRt: default route", g_stNlme.m_aDefaultRouteEntry, IPv6_ADDR_LENGTH);
			u8DefaultRoute = pRoute->m_bOutgoingInterface;
		}
	}

	CNlmeRoutesMap::iterator it = g_stNlme.m_oNlmeRoutesMap.begin();

	for( ; it != g_stNlme.m_oNlmeRoutesMap.end(); it++ )
	{
		CNlmeRoutePtr pRoutePtr = it->second;

		if (pRoutePtr->m_bOutgoingInterface == u8DefaultRoute)
		{
			continue;
		}

		NLME_ADDR_TRANS_ATTRIBUTES *pAtt = NLME_FindATT (pRoutePtr->m_aDestAddress);
		if (!pAtt)
		{
			WARN("LowPANTunnel.sendAttRt: no ATT entry for: %s", GetHex(pRoutePtr->m_aDestAddress,sizeof(pRoutePtr->m_aDestAddress))) ;
			continue;
		}
		memcpy(pDest+nPos, pAtt->m_aShortAddress, 2);
		nPos++;
	}

	LOG("sendAttRt: default_route=%s, opposite entry=%d", (u8DefaultRoute==OutgoingInterfaceBBR) ? "BBR": "DL", nPos);
	ProtocolPacket* pPkt = new ProtocolPacket;

	pPkt->AllocateCopy( (uint8_t*)pDest, nPos*2, sizeof(LayerMsgHdr) + 1 + 1, CRC_LENGTH );
	pPkt->PushFront(&u8DefaultRoute,1);

	uint8_t u8LocalCmd = PacketMatcher::LOCAL_CMD_RT_ATT;
	pPkt->PushFront(&u8LocalCmd,1);

	addPktToTty(pPkt, PacketMatcher::PKT_LOCAL_CMD,PRIO_HIGH);
	m_ttyQ->RetrySend() ;
}

void LowPANTunnel::checkWaitAttListForTimeout()
{
	for (;!m_oMsgWaitingList.empty();)
	{
		CMsgWaitATTList::iterator it = m_oMsgWaitingList.begin();
		CMsgWaitATT::Ptr & pMsg = *it;

		#define MAX_TTL_FOR_MSG 60
		if (pMsg->m_nRecvTime + MAX_TTL_FOR_MSG <= time(NULL))
		{
			return;
		}

		LOG("LowPANTunnel::checkWaitAttListForTimeout: time expired -> drop msg, direction=%s", (pMsg->m_nDirection == CMsgWaitATT::DIR_TR_TO) ? "to TR" : "from TR" );
		m_oMsgWaitingList.erase(it);
	}


}

void LowPANTunnel::checkWaitAttListForSend()
{
	CMsgWaitATTList::iterator itNext = m_oMsgWaitingList.begin();

	for (;itNext != m_oMsgWaitingList.end();)
	{
		CMsgWaitATTList::iterator it = itNext;
		itNext++;

		CMsgWaitATT::Ptr & pMsg = *it;

		if (pMsg->m_nDirection == CMsgWaitATT::DIR_TR_TO)
		{
			uint16_t u16SrcAddr;
			uint16_t u16DstAddr;
			bool bHasAtt = Get6LowPanAddrs( (IPv6Packet*)pMsg->m_pPkt->Data(), pMsg->m_pPkt->DataLen(), u16SrcAddr, u16DstAddr );
			if (bHasAtt)
			{
				ProtocolPacket* pPktForTR = g_pApp->m_obTunnel.IpV6ToLowPAN((IPv6Packet*)pMsg->m_pPkt->Data(), pMsg->m_pPkt->DataLen(), u16SrcAddr, u16DstAddr);

				if (pPktForTR && g_pApp->m_obTunnel.AddToTty(pPktForTR))
				{
					//pMsg->Disown();
					delete pPktForTR;
				}

				m_oMsgWaitingList.erase(it);
				continue;
			}
		}
		else if (pMsg->m_nDirection == CMsgWaitATT::DIR_TR_FROM)
		{
			IPv6Packet stIPv6Packet;
			int nLen = 0;

			const uint8_t* pSrcIPv6;
			const uint8_t* pDstIPv6;;

			if (GetIPv6Addrs(pMsg->m_pPkt,pSrcIPv6,pDstIPv6))
			{
				nLen = lowPanToIPv6(pMsg->m_pPkt, &stIPv6Packet,pSrcIPv6,pDstIPv6);

				if (nLen <= 0)
				{	return;
				}

				SendUDP(&stIPv6Packet,nLen);
				m_oMsgWaitingList.erase(it);
				continue;
			}
		}


	}
}

///////////////////////////////////////////////////////////////////////////////
/// @brief The main busy loop.
/// @details When the ack timer expires, the Retransmission Queue is refreshed
/// by dropping the expired packets and reindexing the remaining ones.
/// It waits for incomming UDP messages followed by Transceiver data.
/// A retransmission toward the serial link is performed in the end.
/// @return 0 Allways returns 0.
///////////////////////////////////////////////////////////////////////////////
int LowPANTunnel::Run()
{
	#define ACK_TIMER_CICLES 500

	if ( m_counter == 0 )
	{
		//INFO("TimerHandler called.");
		m_ttyQ->DropExpired( );
		m_ttyQ->Reindex( ) ;
		m_counter = ACK_TIMER_CICLES ;
	}
	--m_counter ;
	//handleUp() ;
	if(handleDown(1000))
	{
		for (int i = 0;i < 2 && handleDown(0);i++)
		;
	}

	m_ttyQ->RetrySend() ;

	if (g_nATT_TR_Changed)
	{
		//send routes to TR
		sendAttRt();
		checkWaitAttListForSend();

		g_nATT_TR_Changed = 0;
	}

	if(m_oWaitListTimer.IsSignaling())
	{
		checkWaitAttListForTimeout();
		m_oWaitListTimer.SetTimer(10*1000);
	}

	//usleep(10000);
	return 0 ;
}



///////////////////////////////////////////////////////////////////////////////
/// @brief Handle the TTY link.
/// @retval (void*)0
///////////////////////////////////////////////////////////////////////////////
bool LowPANTunnel::handleDown( int tout)
{
	ProtocolPacket * pkt = m_trLink->Read( tout);

	int nCrtTime = time(NULL);
	if( !pkt )
	{
		if ( g_pApp->m_cfg.m_nTrPowerID > 0 && nCrtTime - m_nLastValidMsgTime > g_pApp->m_cfg.m_nTrMaxInactivity)
		{
			systemf_to(30, "tr_ctl.sh restart %d ", g_pApp->m_cfg.m_nTrPowerID);
			m_nLastValidMsgTime = nCrtTime;
		}

		return false;
	}

	m_nLastValidMsgTime = nCrtTime;

	if( g_pApp->m_cfg.UseRawLog() )
	{
		LOG_HEX(  "TRNS_IN :", pkt->Data(), pkt->DataLen() );
	}



	PacketMatcher::PKT_TYPE pt = PacketMatcher::GetType(pkt->Data(), pkt->DataLen() ) ;

	if (	pt == PacketMatcher::PKT_BUFF_FULL
		||	pt == PacketMatcher::PKT_BUFF_MIC_ERR
		||	pt == PacketMatcher::PKT_BUFF_RDY
		)
	{
		if ( ( g_pApp->m_cfg.UseRawLog() && (pt == PacketMatcher::PKT_BUFF_RDY) )	// Log everything is raw logging is enabled
			||	( pt != PacketMatcher::PKT_BUFF_RDY ) ) // Do not log PKT_BUFF_RDY, it's too noisy
		{
			LOG("TR PKT_TYPE: %s (%d bytes)", PacketMatcher::GetTypeString(pkt->Data(), pkt->DataLen() ) , pkt->DataLen());
		}

		m_ttyQ->SetFlowControl(pt);
		delete pkt;
		return true;
	}

	uint16_t msgHandler;
	uint16_t myCrc;
	uint8_t in_mic[CRC_LENGTH] ;

	//minim len: type(1b) + handler(2b) + crc(2b)
	if (pkt->DataLen() < (sizeof(LayerMsgHdr) + CRC_LENGTH))
	{
		LOG("serial pkt len too small %d < %d", pkt->DataLen(),  (sizeof(LayerMsgHdr) + CRC_LENGTH));
		delete pkt;
		return true;
	}

	pkt->PopBack( in_mic, CRC_LENGTH );
	myCrc = computeCRC( pkt->Data(), pkt->DataLen() ) ;
	if ( memcmp( in_mic, (void*)&myCrc, CRC_LENGTH ) )
	{
		LOG_HEX(  "CRC ERROR", (const unsigned char*)&myCrc, CRC_LENGTH );
		delete pkt;
		return true;
	}

	msgHandler  = ((LayerMsgHdr*)pkt->Data())->msg_handler ;

	// Do not log periodical time sync messages
	//	(messages with no payload, only type+handler+ccm)
	if (pkt->DataLen() > sizeof(LayerMsgHdr))
	{	LOG("TR PKT_TYPE: %s (handle x%04X %d bytes)", PacketMatcher::GetTypeString(pkt->Data(), pkt->DataLen() ),	ntohs(msgHandler), pkt->DataLen());
	}

	if (pt != PacketMatcher::PKT_NTWK_CNF) //the previous code did not send ack but ticus says that should
	{	sendAck( msgHandler );
		m_ttyQ->RetrySend();
	}

	pkt->PopFront( 0, sizeof(LayerMsgHdr) ) ;

	switch( pt )
	{
		case PacketMatcher::PKT_NTWK_TR2BBR:

			if ( handleNetPkt(pkt) )
			{
				if (!handleFull6lowPANMsg(pkt))
				{	return true;
				}

			}
			break ;


		case PacketMatcher::PKT_NTWK_CNF:
			handleNetConfirm(pkt) ;
			break;

		case PacketMatcher::PKT_LOCAL_CMD:

			handleLocalCmd(pkt);

			break ;
		case PacketMatcher::PKT_TR_DEBUG:
			break;
		default:
			ERR("TR PKT_TYPE %d invalid: PKT_INVALID %d bytes", pt, pkt->DataLen());
	}

	//LOG("TRNS_FIN: ok");
	delete pkt;
	return true;
}

#define KeyOperationSet 1
#define KeyOperationDel 2
bool LowPANTunnel::handleLocalCmdKeyMgr(ProtocolPacket*pkt)
{
	uint8_t u8Operation;

	if (!pkt->PopFront(&u8Operation,1))
	{
		LOG("handleLocalCmdKeyMgr: missing Operation field");
		return false;
	}

	if (u8Operation != KeyOperationSet && u8Operation != KeyOperationDel)
	{
		LOG("handleLocalCmdKeyMgr: key operation invalid %d ", u8Operation);
		return false;
	}
	TIPv6Address	oPeerIPv6Address; //	16
	uint16_t		u16UdpSPort;//	2
	uint16_t		u16UdpDPort;//	2
	uint8_t			u8KeyID;//	1
	uint8_t			u8Type;//	1
	uint8_t			u8TLenc;

	if (!pkt->PopFront(&u8TLenc,sizeof(u8TLenc)))
	{
		LOG("handleLocalCmdKeyMgr: TLenc field");
		return false;
	}

	if (!pkt->PopFront(oPeerIPv6Address.m_pu8RawAddress,sizeof(oPeerIPv6Address.m_pu8RawAddress)))
	{
		LOG("handleLocalCmdKeyMgr: PeerIPv6Address field");
		return false;
	}

	if (!pkt->PopFront(&u16UdpSPort,sizeof(u16UdpSPort)))
	{
		LOG("handleLocalCmdKeyMgr: UdpSPort field");
		return false;
	}
	if (!pkt->PopFront(&u16UdpDPort,sizeof(u16UdpDPort)))
	{
		LOG("handleLocalCmdKeyMgr: UdpDPort field");
		return false;
	}

	if (!pkt->PopFront(&u8KeyID,sizeof(u8KeyID)))
	{
		LOG("handleLocalCmdKeyMgr: KeyID field");
		return false;
	}

	if (!pkt->PopFront(&u8Type,sizeof(u8Type)))
	{
		LOG("handleLocalCmdKeyMgr: Type field");
		return false;
	}
	u16UdpSPort = ntohs(u16UdpSPort);
	u16UdpDPort = ntohs(u16UdpDPort);
	if (u8Operation == KeyOperationDel)
	{
		LOG("handleLocalCmdKeyMgr: op=del, TLenc=%02X, peer=%s, sport=%d, dport=%d, key=%d, type=%d",
				u8TLenc, GetHex(oPeerIPv6Address.m_pu8RawAddress, sizeof(oPeerIPv6Address.m_pu8RawAddress)),
				 u16UdpSPort, u16UdpDPort, u8KeyID, u8Type);
		return SLME_DeleteKey(oPeerIPv6Address.m_pu8RawAddress, u16UdpSPort, u16UdpDPort, u8KeyID, u8Type) == SFC_SUCCESS;
	}

	//set key operation
	uint8_t			pu8Key[16];//	16
	uint8_t			u8IssuerEUI64[8];//	8
	uint32_t		u32ValidNotBefore;//	4
	uint32_t		u32SoftLifetime;//	4
	uint32_t		u32HardLifetime;//	4

	uint8_t			u8Policy;//	1


	if (!pkt->PopFront(pu8Key,sizeof(pu8Key)))
	{
		LOG("handleLocalCmdKeyMgr: Key field");
		return false;
	}

	if (!pkt->PopFront(u8IssuerEUI64,sizeof(u8IssuerEUI64)))
	{
		LOG("handleLocalCmdKeyMgr: IssuerEUI64 field");
		return false;
	}

	if (!pkt->PopFront(&u32ValidNotBefore,sizeof(u32ValidNotBefore)))
	{
		LOG("handleLocalCmdKeyMgr: ValidNotBefore field");
		return false;
	}

	if (!pkt->PopFront(&u32SoftLifetime,sizeof(u32SoftLifetime)))
	{
		LOG("handleLocalCmdKeyMgr: SoftLifetime field");
		return false;
	}

	if (!pkt->PopFront(&u32HardLifetime,sizeof(u32HardLifetime)))
	{
		LOG("handleLocalCmdKeyMgr: uHardLifetime field");
		return false;
	}


	if (!pkt->PopFront(&u8Policy,sizeof(u8Policy)))
	{
		LOG("handleLocalCmdKeyMgr: Policy field");
		return false;
	}

	u32ValidNotBefore = htonl(u32ValidNotBefore);
	u32HardLifetime = htonl(u32HardLifetime);
	u32SoftLifetime = htonl(u32SoftLifetime);

	LOG("handleLocalCmdKeyMgr: op=set, TLenc=%02X, peer=%s, sport=%d, dport=%d, key=%d, type=%d, key=%s, issuer=%s, ValidNotBefore=%d, SoftLifetime=%d, HardLifetime=%d",
		u8TLenc, GetHex(oPeerIPv6Address.m_pu8RawAddress, sizeof(oPeerIPv6Address.m_pu8RawAddress)),
		 u16UdpSPort, u16UdpDPort, u8KeyID, u8Type, GetHex(pu8Key, sizeof(pu8Key)), GetHex(u8IssuerEUI64, sizeof(u8IssuerEUI64)),
		u32ValidNotBefore, u32SoftLifetime, u32HardLifetime
		);

	bool bRet = (SLME_SetKey(oPeerIPv6Address.m_pu8RawAddress, u16UdpSPort, u16UdpDPort, u8KeyID, pu8Key, u8IssuerEUI64,
								u32ValidNotBefore, u32SoftLifetime, u32HardLifetime, u8Type,u8Policy) == SFC_SUCCESS );

	if ( bRet)
	{
		checkNeedAddSMContract();
		//contract and session key available - update the TL security level
		uint8 ucTmp;
		if( SLME_FindTxKey(g_pApp->m_cfg.getSM_IPv6(),ISA100_START_PORTS,ISA100_SMAP_PORT, &ucTmp ) )// have key to SM's UAP
		{
			g_stDSMO.m_ucTLSecurityLevel = SECURITY_ENC_MIC_32;
			LOG("LowPANTunnel::handleLocalCmdKeyMgr: set TL security to SECURITY_ENC_MIC_32");
		}
	}
	return bRet;
}

bool LowPANTunnel::handleLocalCmdProvisioning(ProtocolPacket*pkt)
{
	LOG("TR requested provisioning info -> send prov info -> reset all and wait rejoin");
	if (pkt->DataLen())
	{
		LOG_HEX("TR requested provisioning info: extra in data: ", pkt->Data(),pkt->DataLen());
	}

	// [TODO] reset all stack info
	ResetStack();

	TProvisioningInfo stProv;

	//memset(stProv.m_aBbrEUI64, 7, sizeof())
	memcpy(stProv.m_aBbrEUI64, g_pApp->m_cfg.m_pu8BbrEUI64, sizeof(stProv.m_aBbrEUI64));

	stProv.m_unFormatVersion	=	g_pApp->m_cfg.m_u16ProvisionFormatVersion;
	//stProv.m_unSubnetID			=	g_pApp->m_cfg.m_u16SubnetID;
	stProv.m_unFilterBitMask	=	g_pApp->m_cfg.m_u16FilterBitMask;
	stProv.m_unFilterTargetID	=	g_pApp->m_cfg.m_u16FilterTargetID;
	//stProv.m_u8CCA_Limit		=	g_pApp->m_cfg.m_u8CCA_Limit;

	memcpy (stProv.m_aAppJoinKey, g_pApp->m_cfg.m_u8AppJoinKey, sizeof(g_pApp->m_cfg.m_u8AppJoinKey));
	//memcpy (stProv.m_aDllJoinKey, g_pApp->m_cfg.m_u8DllJoinKey, sizeof(g_pApp->m_cfg.m_u8DllJoinKey));
	//memcpy (stProv.m_aProvisionKey, g_pApp->m_cfg.m_u8ProvisionKey, sizeof(g_pApp->m_cfg.m_u8ProvisionKey));
	memcpy (stProv.m_aSecMngrEUI64, g_pApp->m_cfg.m_u8SecurityManager, sizeof(g_pApp->m_cfg.m_u8SecurityManager));
	memcpy (stProv.m_aSysMngrIPv6, g_pApp->m_cfg.getSM_IPv6(), sizeof(stProv.m_aSysMngrIPv6));
	memcpy (stProv.m_aIpv6BBR, g_pApp->m_cfg.getBB_IPv6(), sizeof(stProv.m_aIpv6BBR));
	memcpy (stProv.m_aBbrTag, g_pApp->m_cfg.m_szBBRTag, sizeof(stProv.m_aBbrTag));

	ProtocolPacket* pPkt = new ProtocolPacket;
	pPkt->AllocateCopy((uint8_t*)&stProv, sizeof(stProv), sizeof(LayerMsgHdr) + 4, CRC_LENGTH );

	uint8_t u8LocalCmdID = PacketMatcher::LOCAL_CMD_PROV;
	pPkt->PushFront(&u8LocalCmdID,1);

	//LOG("pkt len=%d",pPkt->DataLen());
	//LOG_HEX("handleLocalCmdProvisioning: ", pPkt->Data(), pPkt->DataLen());

	addPktToTty(pPkt, PacketMatcher::PKT_LOCAL_CMD,PRIO_HIGH);
	m_ttyQ->RetrySend() ;
	return true;
}

bool LowPANTunnel::handleLocalCmdEngModeGet(ProtocolPacket*pkt)
{
	uint8_t u8EngMode;

	if (!pkt->PopFront(&u8EngMode,1))
	{
		LOG("Received EngModeGet response invalid");
		return false;
	}

	LOG("Received EngModeGet response = %d", u8EngMode);

	if (FileExist(FILE_CMD_ENG_MODE_ACKED))
	{
		LOG("EngMode unlink" FILE_CMD_ENG_MODE_ACKED);
		unlink(FILE_CMD_ENG_MODE_ACKED);
	}

	if (FileExist(FILE_CMD_ENG_MODE_PENDING))
	{
		LOG("EngMode unlink" FILE_CMD_ENG_MODE_PENDING);
		unlink(FILE_CMD_ENG_MODE_PENDING);
	}

	char szResp[128];
	sprintf(szResp,"cmd=get\nengMode=%d\n",  (int)u8EngMode);

	LOG("EngMode write to file %s engMode=%d", FILE_CMD_ENG_MODE_RESPONDED, u8EngMode);
	WriteToFile (FILE_CMD_ENG_MODE_RESPONDED, szResp, true);


	return true;
}

bool LowPANTunnel::handleLocalCmd(ProtocolPacket*pkt)
{
	uint8_t u8LocalCmdID;

	if (!pkt->PopFront(&u8LocalCmdID,1))
	{
		return false;
	}

	//[TODO]: set key
	switch(u8LocalCmdID)
	{
	case PacketMatcher::LOCAL_CMD_PROV:
		return handleLocalCmdProvisioning(pkt);
	case PacketMatcher::LOCAL_CMD_KEY:
	    return handleLocalCmdKeyMgr(pkt);
	case PacketMatcher::LOCAL_CMD_ENG_MODE_GET:
		return handleLocalCmdEngModeGet(pkt);
	default:
		LOG("handleLocalCmd: cmd_id=%d -> nothing to be done",u8LocalCmdID);
	    break;
	}


	return false;
}

///////////////////////////////////////////////////////////////////////////////
/// Handles a Network layer packet.
///////////////////////////////////////////////////////////////////////////////
bool LowPANTunnel::handleNetPkt(ProtocolPacket*&pkt)
{
	// Zero sized packet
	if( pkt->DataLen() < sizeof(NetMsgTrToBbrHeader) + 1 ) //
	{
		// to synchronize, the transceiver sends a NetPkt with empty payload
		return false ;
	}

	NetMsgTrToBbrHeader* pNetHeader = (NetMsgTrToBbrHeader*)pkt->Data();

	int dpatch = pkt->Data()[sizeof(NetMsgTrToBbrHeader)];

	if (pNetHeader->m_u16DstAddr == 0 && pNetHeader->m_u16SrcAddr == 0)
	{
		if (pkt->DataLen() < IPv6_ADDR_LENGTH + sizeof(NetMsgTrToBbrHeader) + 1 )
		{
			// to synchronize, the transceiver sends a NetPkt with empty payload
			return false;
		}
		dpatch = pkt->Data()[sizeof(NetMsgTrToBbrHeader) + IPv6_ADDR_LENGTH ];
	}
	//NetHeader* h = (NetMsgTrToBbrHeader*)pkt->Data() +1  ;

	int dpatch3b = dpatch >> 5;

	NLOG_DBG("LowPANTunnel::handleNetPkt: dispatch=%02X|%02X", dpatch, dpatch3b);
	if (	dpatch3b != NetDispatch_FragFirst
		&&	dpatch3b != NetDispatch_FragNext )
	{
		//LOG("Full Message");
		return true ;
	}

	addFragment( pkt ) ;
	ProtocolPacket *full_msg = m_deFrgmTbl->hasFullMsg() ;
	if( ! full_msg )
	{
		LOG("No full messages yet!");
		return false ; // Break loop: No full message yet!
	}
	delete pkt ;
	pkt = full_msg ;
	return true ;
}


///////////////////////////////////////////////////////////////////////////////
/// Handles a full 6lowpan message
/// return true -> the pkt should be deleted
///			false -> the pkt should not be deleted
///////////////////////////////////////////////////////////////////////////////
bool LowPANTunnel::handleFull6lowPANMsg(ProtocolPacket *pkt)
{
	IPv6Packet stIPv6Packet;
	int nLen = 0;

	const uint8_t* pSrcIPv6;
	const uint8_t* pDstIPv6;;

	if (!GetIPv6Addrs(pkt,pSrcIPv6,pDstIPv6))
	{
		//return true; // quick fix
		//add to wait queue
		CMsgWaitATT::Ptr pPtr (new CMsgWaitATT);

		pPtr->Load(pkt,CMsgWaitATT::DIR_TR_FROM);

		g_pApp->m_obTunnel.GetWaitListToTr()->push_back(pPtr);
		return false;
	}

	nLen = lowPanToIPv6(pkt, &stIPv6Packet,pSrcIPv6,pDstIPv6);

	if (nLen <= 0)
	{	return true;
	}

	SendUDP(&stIPv6Packet,nLen);

	return true ;
}


///////////////////////////////////////////////////////////////////////////////
/// @brief Handles a Network confirmation Packet.
///////////////////////////////////////////////////////////////////////////////
bool LowPANTunnel::handleNetConfirm( ProtocolPacket* pkt )
{
	if( pkt->DataLen() <  sizeof(NWKConfirmElm) )
		 return false ;

	NWKConfirmElm *idx = (NWKConfirmElm*)  pkt->Data()  ;
	while (  idx < (NWKConfirmElm*) (pkt->Data() + pkt->DataLen()) )
	{
		if (m_nLastCmdEngModeHandle == ntohs(idx->msg_handler))
		{
			LOG("EngMode rename %s -> %s handle=%04X", FILE_CMD_ENG_MODE_PENDING, FILE_CMD_ENG_MODE_ACKED, m_nLastCmdEngModeHandle);
			rename(FILE_CMD_ENG_MODE_PENDING, FILE_CMD_ENG_MODE_ACKED);
			m_nLastCmdEngModeHandle = -1;
		}

		m_ttyQ->Confirm( idx->msg_handler, idx->status ) ;
		++idx ;
	}
	return true ;
}



///////////////////////////////////////////////////////////////////////////////
// Mesh Type | Mesh Header | HC1 Dispatch | HC1 Header | Payload
// Mesh Type | Mesh Header | Fragmentation Type | Fragmentation Header | HC1 Dispatch | HC1 Header | Payload |
///////////////////////////////////////////////////////////////////////////////
ProtocolPacket* LowPANTunnel::IpV6ToLowPAN( IPv6Packet* p_pIPv6Packet, int p_nLen, uint16_t p_u16SrcAddr, uint16_t p_u16DstAddr )
{
	NetMsgBbrToTrHeader stSerialHeader;

	if (p_nLen < (int)offsetof(IPv6Packet,m_stIPPayload.m_aUDPPayload) + 1 ) //the payload should have at least 1b
	{
		WARN("LowPANTunnel.ipV6ToLowPAN: IPv6 size too low %db", p_nLen);
		return false;
	}

	stSerialHeader.m_u16DstAddr = p_u16DstAddr;
	stSerialHeader.m_u16SrcAddr = p_u16SrcAddr;

	memcpy(&stSerialHeader.m_u16ContractID, p_pIPv6Packet->m_stIPHeader.m_aFlowLabel + 1, 2);

	stSerialHeader.m_u8TrafficClass = p_pIPv6Packet->m_stIPHeader.m_ucVersionAndTrafficClass << 4 | p_pIPv6Packet->m_stIPHeader.m_aFlowLabel[0] >> 4;

	uint16_t sPort   = ntohs(p_pIPv6Packet->m_stIPPayload.m_stUDPHeader.m_unUdpSPort);//m_smLink->SPort(pkt);
	uint16_t dPort   = ntohs(p_pIPv6Packet->m_stIPPayload.m_stUDPHeader.m_unUdpDPort);//m_smLink->DPort(pkt);

	sPort -= 61616 ;
	dPort -= 61616 ;

	// Checking for limits.
	if( sPort >= 16 || dPort >=16 )
	{
		WARN("sPort=%d dPort=%d", sPort, dPort );
		sPort=dPort=0;
	}
	uint8_t u8Ports = (sPort << 4) | dPort ;



	ProtocolPacket* pkt = new ProtocolPacket;

	int nStartReservedLen = sizeof(NetMsgBbrToTrHeader) + 10 +  ((stSerialHeader.m_u16DstAddr == 0) ? IPv6_ADDR_LENGTH : 0);
	pkt->AllocateCopy(p_pIPv6Packet->m_stIPPayload.m_aUDPPayload,
				p_nLen - offsetof(IPv6Packet,m_stIPPayload.m_aUDPPayload), nStartReservedLen, 4 ); // 5 - 1b IP, 1b - udp, 1- porturi, 2 - checksum

	if (p_pIPv6Packet->m_stIPPayload.m_aUDPPayload[0] == SECURITY_NONE)
	{
		LOG("IpV6ToLowPAN: no security -> use UDp checksum");

		pkt->PushFront((uint8_t*)&p_pIPv6Packet->m_stIPPayload.m_stUDPHeader.m_u16UdpCkSum, 2);
		pkt->PushFront (&u8Ports, 1);

		uint8_t u8UdpEnc = ( uint8_t)B8(11110011); // checksum not included
		pkt->PushFront (&u8UdpEnc, 1);

		uint8_t u8IPdipatch = B8(01111101); //contract enable without contract
		uint8_t u8IPenc = B8(01110111);


		pkt->PushFront(&u8IPenc,1);
		pkt->PushFront (&u8IPdipatch, 1);
	}
	else
	{
		uint8_t u8IPdipatch = ( uint8_t)B8(00000001); //basic header

		pkt->PushFront (&u8Ports, 1);

		pkt->PushFront (&u8IPdipatch, 1);
	}

	int nNetPayloadLen = pkt->DataLen();


	//LOG_HEX("IpV6ToLowPAN: 6lowpan: ", pkt->Data(),pkt->DataLen());
	if (stSerialHeader.m_u16DstAddr == 0)
	{
		pkt->PushFront (p_pIPv6Packet->m_stIPHeader.m_ucSrcAddr, IPv6_ADDR_LENGTH);

		//LOG_HEX("IpV6ToLowPAN: push IPv6: ", pkt->Data(),pkt->DataLen());
	}

	pkt->PushFront ((const uint8_t*)&stSerialHeader, sizeof(NetMsgBbrToTrHeader));

	LOG("IpV6ToLowPAN: from=%s %s to=%s %s total=%3db payload=%db", GetHex(p_pIPv6Packet->m_stIPHeader.m_ucSrcAddr,IPv6_ADDR_LENGTH), isSMipv6(p_pIPv6Packet->m_stIPHeader.m_ucSrcAddr) ? "SM": "  ",
		GetHex(p_pIPv6Packet->m_stIPHeader.m_ucDstAddr,IPv6_ADDR_LENGTH), isTRipv6(p_pIPv6Packet->m_stIPHeader.m_ucDstAddr) ? "TR" : "  ", pkt->DataLen(), nNetPayloadLen );

	return pkt;
}

// return	1/0 - pkt should be deleted
int LowPANTunnel::AddToTty (ProtocolPacket* pkt)
{
	uint16_t nDataGramSize  =  pkt->DataLen() - sizeof(NetMsgBbrToTrHeader) ;

	if (pkt->DataLen() < sizeof(NetMsgBbrToTrHeader))
	{
		LOG("AddToTty: ERROR size incorect");
		return 1;
	}

	nDataGramSize -= (((NetMsgBbrToTrHeader*)pkt->Data())->m_u16DstAddr == 0 ? IPv6_ADDR_LENGTH : 0);

	//LOG("DataGramSize=%d", nDataGramSize);
	if( nDataGramSize  <= g_pApp->m_cfg.m_nMax6lowPanNetPayload) // not need fragmentation
	{
		addPktToTty( pkt, PacketMatcher::PKT_NTWK_BBR2TR, PRIO_HIGH );
		m_ttyQ->RetrySend() ;
		//LOG("LowPANTunnel.ipV6ToLowPAN: Done");
		return 0 ;
	}

	LOG("AddToTty: net payload too big %d > %d -> frag", nDataGramSize, g_pApp->m_cfg.m_nMax6lowPanNetPayload);
	NetMsgBbrToTrHeader stSerialHeader;

	if (!pkt->PopFront(&stSerialHeader,sizeof(NetMsgBbrToTrHeader)))
	{
		//should not happen
		return 1;
	}

	uint8_t pSrcIPv6[IPv6_ADDR_LENGTH];

	if (stSerialHeader.m_u16DstAddr == 0)
	{
		if (!pkt->PopFront(pSrcIPv6,IPv6_ADDR_LENGTH))
		{
			return 1;
		}
	}


	m_dgram_tag++;

	int nStartReservedLen = sizeof(NetMsgBbrToTrHeader) + 10 +  ((stSerialHeader.m_u16DstAddr == 0) ? IPv6_ADDR_LENGTH : 0) + 1 + sizeof(FragmHdr) + 1;
	// fragmented
	//LOG("LowPANTunnel.ipV6ToLowPAN: Fragment Begin");
	uint16_t nDatagramOffset =  0 ;

	ProtocolPacket * pFragment = new ProtocolPacket( ) ;
	pFragment->AllocateCopy( pkt->Data(), g_pApp->m_cfg.m_n6lowPanNetFragSize, nStartReservedLen, 4 ) ;
	// replace 5 with sizeof meshheader+type
	//nDataGramSize += HEADER_WITHOUT_TC_AND_FL - 5; //

	// create first fragment
	FragmHdr fh ;
	fh.dgramSz[0] = nDataGramSize>>8 ;
	fh.dgramSz[1] = nDataGramSize & 0xFF ;
	fh.dgramSz[0] |= 0xC0 ;

	fh.dgramTag[0]= m_dgram_tag>>8 ;
	fh.dgramTag[1]= m_dgram_tag&0xFF ;
	pFragment->PushFront( fh );

	if (stSerialHeader.m_u16DstAddr == 0)
	{
		pkt->PushFront(pSrcIPv6,IPv6_ADDR_LENGTH);
	}
	pFragment->PushFront((const uint8_t*)&stSerialHeader, sizeof(NetMsgBbrToTrHeader));

	addPktToTty( pFragment, PacketMatcher::PKT_NTWK_BBR2TR, PRIO_LOW );

	nDatagramOffset += g_pApp->m_cfg.m_n6lowPanNetFragSize ;

	fh.dgramSz[0] |= 0x20 ;
	// create remaining fragments
	while( nDatagramOffset < nDataGramSize )
	{
		uint16_t frgmSz=nDataGramSize-nDatagramOffset;
		if ( frgmSz>g_pApp->m_cfg.m_n6lowPanNetFragSize ) frgmSz = g_pApp->m_cfg.m_n6lowPanNetFragSize ;
		pFragment = new ProtocolPacket( ) ;

		pFragment->AllocateCopy( pkt->Data()+nDatagramOffset, frgmSz, nStartReservedLen, 4 ) ;

		pFragment->PushFront( (uint8_t)(nDatagramOffset/8) );
		pFragment->PushFront( fh );

		if (stSerialHeader.m_u16DstAddr == 0)
		{
			pkt->PushFront(pSrcIPv6,IPv6_ADDR_LENGTH);
		}

		pFragment->PushFront((const uint8_t*)&stSerialHeader, sizeof(NetMsgBbrToTrHeader));
		/// @todo replace 0x80 with PKT_TYPE
		addPktToTty( pFragment, PacketMatcher::PKT_NTWK_BBR2TR, PRIO_LOW );

		nDatagramOffset+=g_pApp->m_cfg.m_n6lowPanNetFragSize ;
	}
	//LOG("LowPANTunnel.ipV6ToLowPAN: Done.");
	return 1 ;
}





///////////////////////////////////////////////////////////////////////////////
/// @brief Add a new packet in the Transceiver Retransmission Queue.
/// @param [in] pkt The packet to be enqueued.
/// @param [in] p_ucMsgType  Packet type.
/// @return the message handler, reserved -1 for future use.
/// @see PKT_TYPE, PacketMatcher
///////////////////////////////////////////////////////////////////////////////
int LowPANTunnel::addPktToTty( ProtocolPacket* pkt, uint8_t p_ucMsgType, PRIO_LEVEL p_ucPriority, Policy policy/*=NORMAL_LIFETIME_MSG*/ )
{

	m_hHandle = (m_hHandle + 1) & 0x7FFF;

	LayerMsgHdr lh = { p_ucMsgType, htons( m_hHandle ) };
	pkt->PushFront( lh );
	uint16_t crc = computeCRC( pkt->Data(), pkt->DataLen() ) ;
	pkt->PushBack( (const void*)&crc, CRC_LENGTH ) ;

	m_ttyQ->AddPending( pkt, p_ucPriority, policy );
	return m_hHandle;
}



///////////////////////////////////////////////////////////////////////////////
//   Source IPv6 address
//   Destination IPv6 address
//   Source port
//   Destination port
///////////////////////////////////////////////////////////////////////////////
int LowPANTunnel::lowPanToIPv6( ProtocolPacket* pkt, IPv6Packet* p_pIPv6Pkt,  const uint8_t* p_pSrcIpv6,  const uint8_t* p_pDstIpv6)
{
	if( pkt->DataLen() < sizeof(NetMsgTrToBbrHeader) + 1 ) //at least dispatch octet
	{
		WARN("LowPANTunnel.lowPanToIPv6:Invalid Packet Size.");
		return 0 ;
	}

	NetMsgTrToBbrHeader stNetMsgTrToBbr;

	pkt->PopFront(&stNetMsgTrToBbr, sizeof(NetMsgTrToBbrHeader) );

	uint8_t pTrOrigDest[IPv6_ADDR_LENGTH];

	if (stNetMsgTrToBbr.m_u16DstAddr == 0 && stNetMsgTrToBbr.m_u16SrcAddr == 0)
	{
		if( !pkt->PopFront(pTrOrigDest,IPv6_ADDR_LENGTH))
		{
			LOG("lowPanToIPv6: short src and dest 0 -> too few butes for dest IPv6");
			return 0;
		}
		p_pDstIpv6 = pTrOrigDest;
		p_pSrcIpv6 = g_pApp->m_cfg.getBB_IPv6();
	}
	//else //

	uint8_t u8IPDispatch;
	if(!pkt->PopFront(&u8IPDispatch, 1 ))
	{
		return 0;
	}

	if ( (u8IPDispatch & B8(11100000)) == B8(01000000))
	{
		//is uncompressed IPv6

		if (pkt->DataLen() < (int)offsetof(IPv6Packet,m_stIPPayload.m_aUDPPayload))
		{
			WARN("lowPanToIPv6: uncompressed IPv6 pkt len too small %db", pkt->DataLen() - 1);
			return 0;
		}

		memcpy(p_pIPv6Pkt,pkt->Data(), pkt->DataLen());
		return pkt->DataLen();
	}

	uint8_t u8UdpDispatch = B8(11110111);

	if (u8IPDispatch == (B8(00000001)) )
	{
		//basic header
		p_pIPv6Pkt->m_stIPHeader.m_ucHopLimit = 64;
		memset(p_pIPv6Pkt->m_stIPHeader.m_aFlowLabel,0,3);
	}
	else if ((u8IPDispatch & B8(11100000)) == B8(01100000))
	{
		//contract enable
		//LOG_TRACK_POINT();
		pkt->PopFront(0, 1); // encoding 0-7

		if ( (u8IPDispatch & B8(11111100)) == B8(01101100) )
		{
			pkt->PopFront(0, 1); //Octet alignment FlowLabel (bits16-19)

			pkt->PopFront (p_pIPv6Pkt->m_stIPHeader.m_aFlowLabel + 1,2);
		}
		else if ( (u8IPDispatch & B8(11111100)) == B8(01111100) )
		{
			//nothing to do
		}
		else
		{
			WARN("lowPanToIPv6: invalid dispatch byte %02X, invalid contract enable", u8IPDispatch );
			return 0;
		}

		int nHopLimitType = u8IPDispatch & B8(00000011);
		if (nHopLimitType == 0)
		{
			if(!pkt->PopFront( &p_pIPv6Pkt->m_stIPHeader.m_ucHopLimit, 1))
			{
				WARN("lowPanToIPv6: contract based header len no HopLimit octet");
			}
		}

		switch (nHopLimitType)
		{	case 1: p_pIPv6Pkt->m_stIPHeader.m_ucHopLimit = 1; break;
			case 2: p_pIPv6Pkt->m_stIPHeader.m_ucHopLimit = 64; break;
			case 3: p_pIPv6Pkt->m_stIPHeader.m_ucHopLimit = 255; break;
		}

		if (!pkt->PopFront(&u8UdpDispatch,1))
		{
			return 0;
		}

		//we support so far only UDP short 4bits ports
		if ( (u8UdpDispatch & B8(11111011)) != (uint8_t)B8(11110011) )
		{
			WARN("lowPanToIPv6: invalid UDP dispatch byte %02X", u8UdpDispatch );
			return 0;
		}
	}
	else
	{
		WARN("lowPanToIPv6: invalid dispatch byte %02X, (first 3bits)", u8IPDispatch );
		return 0;
	}

	memcpy(p_pIPv6Pkt->m_stIPHeader.m_ucSrcAddr, p_pSrcIpv6, sizeof(p_pIPv6Pkt->m_stIPHeader.m_ucSrcAddr));
	memcpy(p_pIPv6Pkt->m_stIPHeader.m_ucDstAddr, p_pDstIpv6, sizeof(p_pIPv6Pkt->m_stIPHeader.m_ucDstAddr));

	p_pIPv6Pkt->m_stIPHeader.m_ucNextHeader = B8(00010001);

	p_pIPv6Pkt->m_stIPHeader.m_ucVersionAndTrafficClass = (6 << 4) | (stNetMsgTrToBbr.m_u8TrafficClass >> 4);
	p_pIPv6Pkt->m_stIPHeader.m_aFlowLabel[0] = (stNetMsgTrToBbr.m_u8TrafficClass << 4);

	uint8_t u8UdpPorts;

	if (!pkt->PopFront(&u8UdpPorts,1))
	{
		return 0;
	}

	bool bPktHasCheckSum = ! (u8UdpDispatch & 0x04);

	NLOG_DBG("UdpDispatch=%02X hasCheckSum=%d", u8UdpDispatch, bPktHasCheckSum);
	uint16_t u16PktCheckSum = 0;

	p_pIPv6Pkt->m_stIPPayload.m_stUDPHeader.m_u16UdpCkSum = 0;

	if (bPktHasCheckSum && !pkt->PopFront (&u16PktCheckSum, 2))
	{
		return 0;
	}

	p_pIPv6Pkt->m_stIPPayload.m_stUDPHeader.m_unUdpSPort = htons(61616+(u8UdpPorts >> 4));
	p_pIPv6Pkt->m_stIPPayload.m_stUDPHeader.m_unUdpDPort = htons(61616+(u8UdpPorts & 0x0f));

	p_pIPv6Pkt->m_stIPHeader.m_u16PayloadSize = htons (sizeof(IPv6UDPHeader) + pkt->DataLen());
	p_pIPv6Pkt->m_stIPPayload.m_stUDPHeader.m_u16UdpLen =  htons (sizeof(IPv6UDPHeader) + pkt->DataLen());

	memcpy(p_pIPv6Pkt->m_stIPPayload.m_aUDPPayload, pkt->Data(), pkt->DataLen());

	if (!bPktHasCheckSum)
	{
		UdpIPv6PseudoHeader stPseudoHeader;

		memcpy(stPseudoHeader.m_aIpv6SrcAddr, p_pIPv6Pkt->m_stIPHeader.m_ucSrcAddr, sizeof(p_pIPv6Pkt->m_stIPHeader.m_ucSrcAddr));
		memcpy(stPseudoHeader.m_aIpv6DstAddr, p_pIPv6Pkt->m_stIPHeader.m_ucDstAddr, sizeof(p_pIPv6Pkt->m_stIPHeader.m_ucDstAddr));
		stPseudoHeader.m_u16PayloadSize = p_pIPv6Pkt->m_stIPHeader.m_u16PayloadSize;
		stPseudoHeader.m_ucNextHeader = p_pIPv6Pkt->m_stIPHeader.m_ucNextHeader;
		memset (stPseudoHeader.m_aPadding, 0, 3);

		uint32_t u32PktCheckSum = 0;
		u32PktCheckSum =  TLDE_IcmpInterimChksum((uint8_t*)&stPseudoHeader, sizeof(stPseudoHeader), 0 );
		u32PktCheckSum =  TLDE_IcmpInterimChksum((uint8_t*)&p_pIPv6Pkt->m_stIPPayload, sizeof(IPv6UDPHeader) + pkt->DataLen(), u32PktCheckSum);

		u16PktCheckSum = htons(TLDE_IcmpGetFinalCksum(u32PktCheckSum));
	}
	p_pIPv6Pkt->m_stIPPayload.m_stUDPHeader.m_u16UdpCkSum = u16PktCheckSum;

	// Destination is clearly SM or GW, no need to log

	LOG("lowPanToIPv6: from=%s %s to=%s %s %3d bytes", GetHex(p_pSrcIpv6,IPv6_ADDR_LENGTH), isTRipv6(p_pSrcIpv6) ? "TR": "  ",
							GetHex(p_pDstIpv6,IPv6_ADDR_LENGTH), isSMipv6(p_pDstIpv6) ? "SM" : "  ",	pkt->DataLen() );

	return (int)offsetof(IPv6Packet,m_stIPPayload.m_aUDPPayload) + pkt->DataLen();
}





///////////////////////////////////////////////////////////////////////////////
/// @brief Add a fragment in the fragment assembly table.
/// @retval 0 failure
/// @retval !0 success
/// @param[in] pt Packet type
/// @param[in] pkt Network Layer Protocol Packet.
///////////////////////////////////////////////////////////////////////////////
void LowPANTunnel::addFragment( ProtocolPacket* pkt )
{
	NetMsgTrToBbrHeader stSerial;

	if (!pkt->PopFront(&stSerial, sizeof(NetMsgTrToBbrHeader)))
	{
		LOG("addFragment: size too small %d header", pkt->DataLen());
		return;
	}

	uint8_t* pIpv6 = NULL;
	if (stSerial.m_u16DstAddr == 0 && stSerial.m_u16SrcAddr == 0)
	{
		pIpv6 = pkt->Data();

		if( !pkt->PopFront(0,IPv6_ADDR_LENGTH))
		{
			LOG("addFragment: size too small %d ipv6", pkt->DataLen());
			return;
		}
	}

	uint8_t u8Dpatch;

	if (!pkt->PopFront(&u8Dpatch, 1))
	{
		LOG("addFragment: size too small %d - dispatch", pkt->DataLen());
		return;
	}


	//NetMsgTrToBbrHeader * p = (NetMsgTrToBbrHeader*) pkt->Data();
	FragmHdr  stFrgm;

	if (!pkt->PopFront(&stFrgm, sizeof(FragmHdr)))
	{
		LOG("addFragment: size too small %d - frag", pkt->DataLen());
		return;
	}


	uint16_t dgramSz   = ((uint16_t)(stFrgm.dgramSz[0] & 0x07) << 8) | stFrgm.dgramSz[1];
	uint16_t dgramTag  = ((uint16_t)(stFrgm.dgramTag[0]) << 8) | stFrgm.dgramTag[1];
	LOG("LowPANTunnel.addFragment: dpatch=%02X, dgramSz=[%d] dgramTag=%X", u8Dpatch, dgramSz, dgramTag ) ;

	u8Dpatch >>= 5;

	if( NetDispatch_FragFirst == u8Dpatch )
	{
		LOG("LowPANTunnel.addFragment: FRGM_START") ;
		//pkt->PopFront( 0,  sizeof(NetMsgTrToBbrHeader) + sizeof(FragmHdr)) ; // Pop all the above parsed header.
		if( ! m_deFrgmTbl->startMsg( dgramTag, stSerial.m_u16SrcAddr, stSerial.m_u16DstAddr, stSerial.m_u8TrafficClass, dgramSz, pkt->DataLen(), pIpv6 ) )
		{
			ERR("LowPANTunnel.addFragment:Unable to start fragment.");
			return;
		}

		m_deFrgmTbl->addFragment( dgramTag, stSerial.m_u16SrcAddr, 0, pkt->Data(), pkt->DataLen() );
	}
	else if( NetDispatch_FragNext == u8Dpatch )
	{
		uint8_t dgramOff;
		if (!pkt->PopFront(&dgramOff,1))
		{
			LOG("addFragment: size too small -- no offset byte");
			return;
		}
		LOG("LowPANTunnel.addFragment:dgramOff=[%d]", dgramOff) ;
		//pkt->PopFront( 0, sizeof(NetMsgTrToBbrHeader) + sizeof(FragmHdr) + 1 ) ; // Pop all the above parsed header.
		m_deFrgmTbl->addFragment( dgramTag, stSerial.m_u16SrcAddr, (uint16_t)dgramOff * 8, pkt->Data(), pkt->DataLen() );
	}
}

void LowPANTunnel::sendAck( int handler ) const
{
	struct timeval tv ;
	ACKMsgHdr msg ;

	MLSM_GetCrtTaiTime( &tv );
	msg.msg_handler = handler ;
	msg.tai[0] = tv.tv_sec & 0xFF ;
	msg.tai[1] = (tv.tv_sec >> 8) & 0xFF ;
	msg.tai[2] = (tv.tv_sec >> 16) & 0xFF ;
	msg.tai[3] = (tv.tv_sec >> 24) & 0xFF ;

	msg.sec_frac[0] = tv.tv_usec ;
	msg.sec_frac[1] = tv.tv_usec >> 8 ;
	msg.sec_frac[2] = tv.tv_usec >> 16 ;

	ProtocolPacket * pk_out = new ProtocolPacket() ;
	pk_out->AllocateCopy( (uint8_t*)&msg, sizeof(ACKMsgHdr), 0, 4);
	uint16_t crc = computeCRC( (uint8_t*)&msg, sizeof( struct ACKMsgHdr) );
	pk_out->PushBack( &crc, CRC_LENGTH );

	m_ttyQ->AddPending(pk_out, PRIO_ACK) ;
}



void LowPANTunnel::LogRoutes( void )
{/*
	if(m_pATTbl)
	{
		m_pATTbl->LogRoutes();
	}*/
}


int LowPANTunnel::SendAlertToTR( const uint8_t* p_pAlertHeader, int p_nHeaderLen, const uint8_t* p_pData, int p_nDataLen)
{

	ProtocolPacket* pPkt = new ProtocolPacket;
	pPkt->AllocateCopy((uint8_t*)p_pAlertHeader, p_nHeaderLen, sizeof(LayerMsgHdr) + 4, CRC_LENGTH + p_nDataLen);

	pPkt->PushBack(p_pData,p_nDataLen);

	uint8_t u8LocalCmdID = PacketMatcher::LOCAL_CMD_ALERT;
	pPkt->PushFront(&u8LocalCmdID,1);

	//LOG("pkt len=%d",pPkt->DataLen());
	//LOG_HEX("handleLocalCmdProvisioning: ", pPkt->Data(), pPkt->DataLen());

	addPktToTty(pPkt, PacketMatcher::PKT_LOCAL_CMD,PRIO_HIGH);
	m_ttyQ->RetrySend() ;
	return 1;
}


void  LowPANTunnel::ReadTmpCmd()
{

	if(!FileExist( FILE_CMD_ENG_MODE))
	{
		LOG("ReadTmpCmd: no file "FILE_CMD_ENG_MODE);
		return;
	}

	LOG("ReadTmpCmd: %s", FILE_CMD_ENG_MODE);
	int nTmp;

	CIniParser oIniParser;

	if( !oIniParser.Load (FILE_CMD_ENG_MODE) )
		return;


	TFileCmdEngModeSet stEngModeSet;

//cmd = get
	char szCmd[16];

	if (!oIniParser.GetVar(NULL, "cmd", szCmd, sizeof(szCmd)))
		return;

	if (strcasecmp(szCmd,"get") == 0)
	{
		stEngModeSet.m_u8SubCmd = PacketMatcher::LOCAL_CMD_ENG_MODE_GET;
	}
	else if (strcasecmp(szCmd,"set") == 0)
	{
		stEngModeSet.m_u8SubCmd = PacketMatcher::LOCAL_CMD_ENG_MODE_SET;
	}
	else

	{
		LOG("LowPANTunnel::ReadTmpCmd %s UNK cmd", szCmd);
		return;
	}

	if (stEngModeSet.m_u8SubCmd == PacketMatcher::LOCAL_CMD_ENG_MODE_GET)
	{
		ProtocolPacket* pPkt = new ProtocolPacket;

		pPkt->AllocateCopy((uint8_t*)&stEngModeSet.m_u8SubCmd, 1, 1 + 4, CRC_LENGTH );


		m_nLastCmdEngModeHandle = addPktToTty(pPkt, PacketMatcher::PKT_LOCAL_CMD,PRIO_HIGH);
		m_ttyQ->RetrySend() ;


		LOG("EngMode GET rename %s -> %s handle=%04X", FILE_CMD_ENG_MODE, FILE_CMD_ENG_MODE_PENDING, m_nLastCmdEngModeHandle);
		rename(FILE_CMD_ENG_MODE, FILE_CMD_ENG_MODE_PENDING);
		return;
	}

	//set
	nTmp = 0;
	if (!oIniParser.GetVar(NULL, "engMode", &nTmp))
		return;

	stEngModeSet.m_u8EngMode = nTmp;

	if (!oIniParser.GetVar(NULL, "channel", &nTmp))
		nTmp = 0;


	stEngModeSet.m_u8Channel = (uint8_t)nTmp;

	if (!oIniParser.GetVar(NULL, "power_level ", &nTmp))
		nTmp = 0xff;

	stEngModeSet.m_u8PwrLevel = (uint8_t)nTmp;

	ProtocolPacket* pPkt = new ProtocolPacket;

	pPkt->AllocateCopy((uint8_t*)&stEngModeSet, sizeof(TFileCmdEngModeSet), sizeof(LayerMsgHdr) + 4, CRC_LENGTH );

	m_nLastCmdEngModeHandle = addPktToTty(pPkt, PacketMatcher::PKT_LOCAL_CMD,PRIO_HIGH);
	m_ttyQ->RetrySend() ;

	LOG("EngMode SET rename %s -> %s handle=%04X engMode=%d channel=%d pwr_level=%02X", FILE_CMD_ENG_MODE, FILE_CMD_ENG_MODE_PENDING,
					m_nLastCmdEngModeHandle, stEngModeSet.m_u8EngMode,  stEngModeSet.m_u8Channel, stEngModeSet.m_u8PwrLevel);
	rename(FILE_CMD_ENG_MODE, FILE_CMD_ENG_MODE_PENDING);




}
