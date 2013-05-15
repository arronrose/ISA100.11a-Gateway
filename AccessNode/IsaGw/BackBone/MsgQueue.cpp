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
/// @file	MsgQueue.cpp
/// @author	Marius Negreanu
/// @date	Wed Nov 28 12:26:59 2007 UTC
/// @brief	A Retransmission queue.
/// @see	TtyLink
//////////////////////////////////////////////////////////////////////////////

#include "Shared/h.h"
#include "common/ProtocolLayer.h"
#include "common/EncaseLayer.h"
#include "IsaSocket.h"
#include "LowPANTunnel.h"
#include "MsgQueue.h"
#include "BackBoneApp.h"
#include "BbrTypes.h"

#include <assert.h>
#include <sys/time.h>
#include <time.h>


// 5 seconds to wait until sending again
#define RETRY_TIMEOUT			5
// 60 seconds timeout to wait for an ACK
#define ACK_TIMEOUT				60
#define EXPIRED_TIMEOUT			(ACK_TIMEOUT-10)
#define START_MSG_TIMEOUT				5


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
MsgQueue::MsgQueue()
	: m_bufState(PacketMatcher::PKT_BUFF_RDY)
	, m_pAckPkt(0)
	, m_trLink(0)
	, m_bufRdyCounter(0)
{
	for ( int lvl=0; lvl<PRIO_QUEUES; ++lvl)
	{
		LOG("Bucket size:%d", sizeof(Bucket) );
		memset( m_prioQ+lvl, 0, sizeof(Bucket) );
	}
}


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
MsgQueue::~MsgQueue()
{
	unsigned int k ;
	for ( int lvl=0; lvl<PRIO_QUEUES; ++lvl)
	{
		// Claudiu: function _Reindex does not set to 0 old position of elements 
		for ( k=0; k < m_prioQ[lvl].lastPos; ++k )
		{
			delete m_prioQ[lvl].bucket[k].pkt;
			m_prioQ[lvl].bucket[k].pkt=0;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
/// @param [in] pkt Packet to be added in Queue
/// @param [in] prio Priority of pkt.
///////////////////////////////////////////////////////////////////////////////
bool MsgQueue::AddPending( ProtocolPacket* pkt, PRIO_LEVEL prio, enum Policy policy/*=NORMAL_LIFETIME_MSG*/ )
{

	if( prio == PRIO_ACK )
	{
		delete m_pAckPkt ; /// In case the previous ACK was not sent.
		m_pAckPkt =  pkt ;
		return true;
	}

	// regular packet -> build AckMetaData struct
	LayerMsgHdr* pHdr = (LayerMsgHdr*)pkt->Data();
	AckMetaData t;
	t.pkt = pkt ;
	t.priority = prio ;
	t.initial_timestamp = 0;
	t.nextsend_timestamp = 0;
	t.reinsertions = 0;
	t.msgHandle = pHdr->msg_handler;
	t.policy = policy ;

	return _AddPending( &t ) ;
}


///////////////////////////////////////////////////////////////////////////////
/// @param [in] it A packet and it's metadata (MsgQueue::AckMetaData).
///////////////////////////////////////////////////////////////////////////////
bool MsgQueue::_AddPending( struct MsgQueue::AckMetaData *it )
{
	PRIO_LEVEL prio = it->priority ;
	if ( m_prioQ[prio].lastPos >= MAX_PENDING_SIZE )
	{
		m_prioQ[prio].hasGaps = true ;
		_Reindex( prio ) ;
		if ( m_prioQ[prio].lastPos >= MAX_PENDING_SIZE )
		{
			WARN( "MsgQ._AddPending: HIGH buffer full, discard the message");
			return false ;
		}
	}
	m_prioQ[prio].bucket[ m_prioQ[prio].lastPos ] = *it ;
	m_prioQ[prio].lastPos++ ;
	m_prioQ[prio].queueSz++ ;
	uint16_t Qsz = 0;
	for ( int lvl=0; lvl< PRIO_QUEUES; ++lvl )
	{	Qsz += m_prioQ[lvl].queueSz;	}

	LOG( "MsgQ.Add: type x%02X handle x%04X Qsz %d pktSz %d"
		, ((LayerMsgHdr*)it->pkt->Data())->type, ntohs(it->msgHandle), Qsz, it->pkt->DataLen() );
	return true ;
}


///////////////////////////////////////////////////////////////////////////////
/// @param [in] msgHandle The message handle to be confirmed.
/// @param [in] status The status with which the message is confirmed
///////////////////////////////////////////////////////////////////////////////
bool MsgQueue::Confirm( uint16_t msgHandle, uint16_t status )
{

	for ( int lvl=0; lvl < PRIO_QUEUES; ++lvl )
	{
		for( int idx = 0; idx < m_prioQ[lvl].lastPos; ++idx )
		{
			if (m_prioQ[lvl].bucket[idx].msgHandle != msgHandle)
			{
				continue;
			}

			if (!m_prioQ[lvl].bucket[idx].pkt)
			{
				return true;
			}

			uint16_t u16Dest = 0;

			if (sizeof(LayerMsgHdr) >  m_prioQ[lvl].bucket[idx].pkt->DataLen())
			{
				LOG("MsgQ.Confirm: ERROR:  sizeof(LayerMsgHdr) + sizeof(NetMsgBbrToTrHeader) >  m_prioQ[lvl].bucket[idx].pkt->DataLen()");
				delete  m_prioQ[lvl].bucket[idx].pkt ;
				m_prioQ[lvl].bucket[idx].pkt = 0;
				m_prioQ[lvl].hasGaps = true ;
				m_prioQ[lvl].queueSz-- ;
				return true;
			}

			if (m_prioQ[lvl].bucket[idx].pkt->Data()[0] == PacketMatcher::PKT_NTWK_BBR2TR
				&& (sizeof(LayerMsgHdr) + sizeof(NetMsgBbrToTrHeader) <= m_prioQ[lvl].bucket[idx].pkt->DataLen())
				)
			{
				NetMsgBbrToTrHeader * pNetHeader = (NetMsgBbrToTrHeader*)(m_prioQ[lvl].bucket[idx].pkt->Data() + sizeof(LayerMsgHdr));
				u16Dest = pNetHeader->m_u16DstAddr;
			}


			if ( status == CONFIRM_OUT_OF_MEMORY )
			{
				m_prioQ[lvl].bucket[idx].nextsend_timestamp -= ACK_TIMEOUT;
				m_prioQ[lvl].bucket[idx].nextsend_timestamp += RETRY_TIMEOUT;
				LOG( "MsgQ.Add.Confirm:  handle x%04X dst %04X Qsz %d status 07 OUT_OF_MEM retry %d"
					, ntohs(msgHandle)
					, ntohs(u16Dest)
					, m_prioQ[lvl].queueSz
					, m_prioQ[lvl].bucket[idx].reinsertions ) ;

				return true;
			}

			if(status != CONFIRM_OK)
			{
				//we can dismantle the packet: it's deleted anyway
				LayerMsgHdr mh;
				m_prioQ[lvl].bucket[idx].pkt->PopFront(&mh, sizeof(LayerMsgHdr));
				if( mh.type == PacketMatcher::PKT_MESG_NET )
				{
					NetMsgBbrToTrHeader nh;
					unsigned short ccr;
					m_prioQ[lvl].bucket[idx].pkt->PopBack(0, CRC_LENGTH);
					m_prioQ[lvl].bucket[idx].pkt->PopFront(&nh, 5);
					m_prioQ[lvl].bucket[idx].pkt->PopBack(&ccr, 2);
					LOG("DROPMSG: reason=%02X, src:%04X, dst:%04X, mmic:%08X",
						status, ntohs(nh.m_u16SrcAddr), ntohs(nh.m_u16DstAddr), ntohs(ccr));
				}
			}

			time_t nCrtTime = time(0);
			LOG( "MsgQ.Del.Confirm:  handle x%04X dst %04X Qsz %d status %02X%s time total=%d last=%d"
				, ntohs(msgHandle)
				, ntohs(u16Dest)
				, m_prioQ[lvl].queueSz
				, status
				, 	(status==CONFIRM_OK)		? " (success)"
					: (status==CONFIRM_TIMEOUT)	? " (TIMEOUT)"
					: " (?)", nCrtTime - m_prioQ[lvl].bucket[idx].initial_timestamp, nCrtTime - m_prioQ[lvl].bucket[idx].nextsend_timestamp + ACK_TIMEOUT  ) ;

			delete  m_prioQ[lvl].bucket[idx].pkt ;
			m_prioQ[lvl].bucket[idx].pkt = 0;
			m_prioQ[lvl].hasGaps = true ;
			m_prioQ[lvl].queueSz-- ;

			return true;
		}
	}
	WARN( "MsgQ::Confirm handle x%04X not found !!", ntohs(msgHandle) );

	return false ;
}


///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
bool MsgQueue::DropExpired( )
{
	time_t seconds = time(0) - EXPIRED_TIMEOUT;

	for ( int lvl = 0; lvl < PRIO_QUEUES; ++lvl )
	{
		for( int idx = 0; idx < m_prioQ[lvl].lastPos; idx ++ )
		{
			/*
			 * If there is data to send
			 * and it was sent to TR
			 * and its lifetime expired
			 * and it has a normal lifetime policy
			 * then consider it for dropping
			 */
			if ( m_prioQ[lvl].bucket[idx].pkt
			&&   m_prioQ[lvl].bucket[idx].initial_timestamp
			&&   seconds >= m_prioQ[lvl].bucket[idx].initial_timestamp
			&&   m_prioQ[lvl].bucket[idx].policy == NORMAL_LIFETIME_MSG )
			{
				//we can dismantle the packet: it's deleted anyway
				LayerMsgHdr mh;
				m_prioQ[lvl].bucket[idx].pkt->PopFront(&mh, sizeof(LayerMsgHdr));
				if( mh.type == PacketMatcher::PKT_MESG_NET ){
					NetMsgBbrToTrHeader nh;
					unsigned short ccr;
					m_prioQ[lvl].bucket[idx].pkt->PopBack(0, CRC_LENGTH);
					m_prioQ[lvl].bucket[idx].pkt->PopFront(&nh, sizeof(NetMsgBbrToTrHeader));
					m_prioQ[lvl].bucket[idx].pkt->PopBack(&ccr, 2);
					LOG("DROPMSG: reason=timeout, src:%04X, dst:%04X, mmic:%08X",
						ntohs(nh.m_u16SrcAddr), ntohs(nh.m_u16DstAddr), ntohs(ccr));
				}
				LOG( "MsgQ.Del.Expired:  handle x%04X Qsz %d"
					, ntohs(m_prioQ[lvl].bucket[idx].msgHandle)
					, m_prioQ[lvl].queueSz);

				delete m_prioQ[lvl].bucket[idx].pkt ;
				m_prioQ[lvl].bucket[idx].pkt = 0;
				m_prioQ[lvl].hasGaps = true ;
				m_prioQ[lvl].queueSz-- ;
			}
		}
	}
	return true ;
}


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
bool MsgQueue::Reindex()
{
	bool rv = true ;
	for ( int lvl = 0; lvl < PRIO_QUEUES; ++lvl )
		rv &= _Reindex( lvl );
	return rv ;
}


bool MsgQueue::_Reindex( int lvl)
{
	if ( ! m_prioQ[lvl].hasGaps ) return false ;

	int r=0 ;

	m_prioQ[lvl].hasGaps = false ;
	for( int idx =0 ; idx < m_prioQ[lvl].lastPos; ++idx )
	{
		if( m_prioQ[lvl].bucket[idx].pkt )
		{
			m_prioQ[lvl].bucket[r++] = m_prioQ[lvl].bucket[idx] ;
		}
	}
	m_prioQ[lvl].queueSz = m_prioQ[lvl].lastPos = r;
	//LOG("REINDEX_DONE: lastPos=%d", r );
	return r ;
}


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
bool MsgQueue::RetrySend( )
{
	if ( m_pAckPkt )
	{
		m_trLink->Write( m_pAckPkt );
		delete m_pAckPkt ;
		m_pAckPkt = 0 ;
		m_bufState = PacketMatcher::PKT_BUFF_FULL ;
		return true ;
	}

	#define RDY_TIMER_CICLES 10

	time_t tv = time(0);
	assert( m_trLink ) ;
	if ( m_bufState != PacketMatcher::PKT_BUFF_RDY
	&&   m_bufState != PacketMatcher::PKT_BUFF_MIC_ERR )
	{
		if ( ++m_bufRdyCounter < RDY_TIMER_CICLES )
		{
			return false ;
		}
		m_bufState = PacketMatcher::PKT_BUFF_RDY ;
		m_bufRdyCounter = 0 ;
	}

	for ( int lvl =0; lvl < PRIO_QUEUES; ++lvl )
	{
		for ( int idx=0; idx < m_prioQ[lvl].lastPos; ++idx )
		{
			if ( m_prioQ[lvl].bucket[idx].pkt
			&&   m_prioQ[lvl].bucket[idx].nextsend_timestamp <= tv )
			{
				LOG( "MsgQ.Send:         handle x%04X Qsz %d"
					, ntohs(m_prioQ[lvl].bucket[idx].msgHandle)
					, m_prioQ[lvl].queueSz );

				if( g_pApp->m_cfg.UseRawLog() )
				{
					LOG_HEX( "TRNS_OUT:", m_prioQ[lvl].bucket[idx].pkt->Data(), m_prioQ[lvl].bucket[idx].pkt->DataLen());
				}
				m_trLink->Write( m_prioQ[lvl].bucket[idx].pkt );

				// this might be turned in a function
				if ( m_prioQ[lvl].bucket[idx].policy == NORMAL_LIFETIME_MSG )
					m_prioQ[lvl].bucket[idx].nextsend_timestamp = tv + ACK_TIMEOUT ; //don't resend, unless a confirm says out-of-mem
				else if ( m_prioQ[lvl].bucket[idx].policy == INF_LIFETIME_MSG )
					m_prioQ[lvl].bucket[idx].nextsend_timestamp = tv + START_MSG_TIMEOUT ;

				if( ! m_prioQ[lvl].bucket[idx].initial_timestamp ) //it's the first sending, record its time
				{
					m_prioQ[lvl].bucket[idx].initial_timestamp = tv;
				}
				++m_prioQ[lvl].bucket[idx].reinsertions ;
				m_bufState = PacketMatcher::PKT_BUFF_FULL ;
				return true ;
			}
		}
	}
	return false ;
}


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
void MsgQueue::SetLink( IsaSocket *sk )
{
	m_trLink = sk;
}
