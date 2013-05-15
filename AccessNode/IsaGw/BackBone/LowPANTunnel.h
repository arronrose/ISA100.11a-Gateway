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

#ifndef _LOW_WPAN_TUN_H_
#define _LOW_WPAN_TUN_H_

//////////////////////////////////////////////////////////////////////////////
/// @file	LowPANTunnel.h
/// @author	Marius Negreanu
/// @date	Wed Nov 28 12:26:59 2007 UTC
/// @brief	Implements the Tunneling class between IPv6 <-> 6lowPAN.
/// @see	CTtyLink
//////////////////////////////////////////////////////////////////////////////

#include "PacketMatcher.h"
#include "WaitingQueues.h"
#include "../../Shared/SimpleTimer.h"
#include "../ISA100/porting.h"


#include <map>
#include <signal.h>
#include <netinet/in.h>

class	AddrMap ;
class	DeFrgmTbl ;
class	ProtocolStack ;
class   MsgQueue ;
class	IPv6Socket;
class	IsaSocket;

/// @enum PRIO_LEVEL
/// @brief MsgQueue Message Priority Level.
enum PRIO_LEVEL {
	PRIO_HIGH=0,			///< High priority: Usually a single fragment.
	PRIO_LOW=1,			///< Low priority: Usually a fragmented message.
	PRIO_ACK=2			///< Highes priority: Ack is sent right away.
};

/// Maximum UDP queue size
#define UDP_QUEUE_SIZE 512

/// Length of the CRC signature at the end of the serial packet
#define CRC_LENGTH 2

//////////////////////////////////////////////////////////////////////////////
/// @struct UdpMsgHandle
/// @ingroup Backbone
//////////////////////////////////////////////////////////////////////////////
struct UdpMsgHandle {
	time_t timestamp ;		///< The time of add.
	ProtocolPacket *pkt ;		///< Cargo to be queued.
};




//////////////////////////////////////////////////////////////////////////////
/// @struct ACKMsgHdr
/// @ingroup Backbone
//////////////////////////////////////////////////////////////////////////////
struct ACKMsgHdr {
	uint8_t		type ;		///< Packet Type
	uint16_t	msg_handler ;	///< Packet Message Handler
	uint8_t		tai[4] ;		///< TAI time. little endian
	uint8_t		sec_frac[3];	///< seconds fraction. little endian
	ACKMsgHdr()
		: type(0x10)
		, msg_handler(0)
	{ }
}__attribute__((__packed__));


//////////////////////////////////////////////////////////////////////////////
/// @struct LayerMsgHdr
/// @ingroup Backbone
//////////////////////////////////////////////////////////////////////////////
struct LayerMsgHdr {
	uint8_t		type ;		///< Packet Type
	uint16_t	msg_handler ;	///< Packet Message Handler
}__attribute__((__packed__));


//////////////////////////////////////////////////////////////////////////////
/// @struct NWKConfirmElm
/// @ingroup Backbone
//////////////////////////////////////////////////////////////////////////////
struct NWKConfirmElm {
	uint16_t        msg_handler ;	///< Packet Message Handler
	uint8_t         status ;	///< Confirmation status
} __attribute__((__packed__));

//////////////////////////////////////////////////////////////////////////////
/// @struct NWKConfirmHdr
/// @ingroup Backbone
//////////////////////////////////////////////////////////////////////////////
struct NWKConfirmHdr {
	uint8_t         type ;		///< Packet type
	uint16_t        radio_handler ;	///< Radio Handler.
	NWKConfirmHdr()
		: type(0x82)
		, radio_handler(0)
	{   }
}__attribute__((__packed__));



struct TFileCmdEngModeSet
{
	uint8_t		m_u8SubCmd	;
	uint8_t		m_u8EngMode	;
	uint8_t		m_u8Channel	;
	uint8_t		m_u8PwrLevel;
}__attribute__((__packed__));

// Encode special treatment for certain packets.
// @see MsgQueue::RetrySend, MsgQueue::AddPending MsgQueue::DropExpired
enum Policy {
	NORMAL_LIFETIME_MSG,
	INF_LIFETIME_MSG
} ;


#define FILE_CMD_ENG_MODE				NIVIS_TMP"tr_cmd_eng_mode.txt"
#define FILE_CMD_ENG_MODE_PENDING		NIVIS_TMP"tr_cmd_eng_mode.txt.pending"
#define FILE_CMD_ENG_MODE_ACKED			NIVIS_TMP"tr_cmd_eng_mode.txt.acked"
#define FILE_CMD_ENG_MODE_RESPONDED		NIVIS_TMP"tr_cmd_eng_mode.txt.responded"


//////////////////////////////////////////////////////////////////////////////
/// @class LowPANTunnel
/// @ingroup Backbone
/// @brief The Tunneling class between IPv6 <-> 6lowPAN.
//////////////////////////////////////////////////////////////////////////////
class LowPANTunnel
{
public:
	/// Default Constructor
	LowPANTunnel() ;
	/// Destructor
	~LowPANTunnel() ;

public:
	int	Start() ;		///<
	int	Run() ;			///<
	void	LogRoutes(void) ;	///<

	ProtocolPacket*		IpV6ToLowPAN( IPv6Packet* p_pIPv6Packet, int p_nLen, uint16_t p_u16SrcAddr, uint16_t p_u16DstAddr );
	int					AddToTty (ProtocolPacket* pkt);
private:
	int		lowPanToIPv6( ProtocolPacket* pkt, IPv6Packet* p_pIPv6Pkt,  const uint8_t* p_pSrcIpv6,  const uint8_t* p_pDstIpv6);
	void	ResetStack();
	int runTRTest() ;

	void	addFragment( ProtocolPacket* pkt ) ;

	int		addStartStopMsg( int p_nStart);
	void	sendAttRt ();

	bool	handleDown( int tout) ;
	bool	handleNetPkt( ProtocolPacket*&pkt ) ;
	bool	handleFull6lowPANMsg(ProtocolPacket *pkt) ;
	bool	handleLocalCmd(ProtocolPacket*pkt);
	bool	handleLocalCmdProvisioning(ProtocolPacket*pkt);
	bool	handleLocalCmdKeyMgr(ProtocolPacket*pkt);
	bool	handleLocalCmdEngModeGet(ProtocolPacket*pkt);

	bool	handleNetConfirm( ProtocolPacket* pkt ) ;
	void	sendAck(int handler) const ;

	void	checkNeedAddSMContract();
	void	checkWaitAttListForSend();
	void	checkWaitAttListForTimeout();
	int		addPktToTty( ProtocolPacket* pkt, uint8_t p_ucMsgType, PRIO_LEVEL p_ucPriority, enum Policy policy=NORMAL_LIFETIME_MSG );

public:
	CMsgWaitATTList* GetWaitListToTr() { return &m_oMsgWaitingList; }
	//CMsgWaitATTList* GetWaitListFromTr() { return &m_oMsgWaitingQFromTr; }

	int SendAlertToTR( const uint8_t* p_pAlertHeader, int p_nHeaderLen, const uint8_t* p_pData, int p_nDataLen);
	void ReadTmpCmd();
private:
	uint16_t udp_last_idx;
	DeFrgmTbl	*m_deFrgmTbl ;
	uint16_t	m_myShortAddr ;
	const uint8_t	*m_myIPv6Addr ;
	const uint8_t	*m_SMIPv6Addr ;

	MsgQueue	*m_ttyQ ;
	IsaSocket	*m_trLink ;
	timer_t		m_timerid ;
	uint8_t		m_key[16];
	uint8_t		m_nonce[13] ;
	uint16_t	m_dgram_tag ;
	uint16_t	m_hHandle;
	uint16_t	m_counter ;
	int			m_nLastCmdEngModeHandle;

	int				m_nLastValidMsgTime;
	CMsgWaitATTList		m_oMsgWaitingList;

	CSimpleTimer		m_oWaitListTimer;




	struct UdpMsgHandle udpQ[UDP_QUEUE_SIZE] ;


};


#endif	//_LOW_WPAN_TUN_H_
