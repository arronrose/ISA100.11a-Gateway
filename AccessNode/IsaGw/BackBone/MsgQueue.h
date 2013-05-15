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

#ifndef _MSG_QUEUE_H_
#define _MSG_QUEUE_H_

//////////////////////////////////////////////////////////////////////////////
/// @file	MsgQueue.h
/// @author	Marius Negreanu
/// @date	Wed Nov 28 12:26:59 2007 UTC
/// @brief	A Retransmission queue (MsgQueue,MsgQueue::AckMetaData,MsgQueue::Bucket).
/// @see	CTtyLink
//////////////////////////////////////////////////////////////////////////////

class ProtocolPacket ;

#define CONFIRM_OK	0x00
//#define CONFIRM_FAILED 1
//#define CONFIRM_OUT_OF_MEM 2
// According to Ticus and eduard Erdei (email from 18.24.2008), the code are as follows:
/*
typedef enum
{
  SUCCESS,
  UNSPECIFIED_ERROR,
  READ_ONLY,
  WRITE_ONLY,
  INVALID_PARAMETER,
  INVALID_ADDRESS,
  DUPLICATE,
  OUT_OF_MEMORY,
  UID_OUT_OF_RANGE,
  DATA_TOO_LONG,
  NO_CHANNEL,
  NO_TIMESLOT,
  NO_LINK,
  RX_LINK,
  TX_LINK,
  NO_ROUTE,
  NO_CONTRACT,
  NO_UID,
  TOO_BIG,
  QUEUE_FULL,
  NACK,
  TIMEOUT,
  MSG_NOT_FOUND,
  INVALID_SC
}SC;
*/
/// Transceiver Confirmation code: Out of memory.
#define CONFIRM_OUT_OF_MEMORY	0x07
/// Transceiver Confirmation code: Timeout while sending packet in 6lowPAN Net
#define CONFIRM_TIMEOUT		0x15

/// Maximum entries in the MsgQueue
#define MAX_PENDING_SIZE 256
/// Number of priority queues in MsgQueue
#define PRIO_QUEUES 2

///////////////////////////////////////////////////////////////////////////////
/// @class MsgQueue
/// @ingroup Backbone
/// @brief A bounded height priority queue.
///////////////////////////////////////////////////////////////////////////////
class MsgQueue {
friend class TMsgQueue ;

public:
	MsgQueue() ;
	~MsgQueue() ;

public:
	bool AddPending( ProtocolPacket *pkt, PRIO_LEVEL prio, Policy policy=NORMAL_LIFETIME_MSG ) ;
	bool Confirm( uint16_t msgHandler, uint16_t status ) ;
	bool DropExpired( );
	bool Reindex( void ) ;
	bool RetrySend( void ) ;
	void SetLink( IsaSocket * s ) ;
	void SetFlowControl( PacketMatcher::PKT_TYPE fc)
	{	m_bufState = fc ; }

public:
	/// @struct AckMetaData
	/// @ingroup Backbone
	/// @brief 6lowPAN Address
	struct AckMetaData {
		ProtocolPacket *pkt ;
		PRIO_LEVEL	priority ;
		time_t		initial_timestamp ;
		time_t		nextsend_timestamp ;
		uint16_t	msgHandle ;
		int8_t		reinsertions ;
		Policy		policy ;
	};
	/// @struct Bucket
	/// @ingroup Backbone
	/// @brief 6lowPAN Address
	struct Bucket {
		AckMetaData bucket[MAX_PENDING_SIZE];
		uint16_t lastPos ;	///< The end of the queue.
		uint16_t queueSz ;	///< The number of elements in queue.
		bool	 hasGaps ;	///< Elements were removed.
	};

protected:
	bool _AddPending( struct AckMetaData *it ) ;
	bool _Reindex( int lvl ) ;

protected:
	PacketMatcher::PKT_TYPE m_bufState;	///< The current state of the BufferReady FSM.
	ProtocolPacket	*m_pAckPkt ;		///< Only one ACK packet is kept in Queue.
	Bucket		m_prioQ[PRIO_QUEUES];	///< We have 2 lists, keeping HIGH_PRIO(0) and LOW_PRIO(1)
	IsaSocket	*m_trLink ;		///< The Transceiver Link.
	uint16_t	m_bufRdyCounter ;	///< Buffer Ready Timer.
};
#endif	//_MSG_QUEUE_H_

