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
// Name:        DeFrgmTbl.cpp
// Author:      Marius Negreanu
// Date:        Wed Nov 28 12:26:59 2007 UTC
// Description: `6lowPAN Fragment` assembling class.
//////////////////////////////////////////////////////////////////////////////

#include "Shared/ProtocolPacket.h"
#include "DeFrgmTbl.h"
#include "BbrTypes.h" 

#define EXPIRATION_TIMEOUT 120	/// Seconds.

DeFrgmTbl::DeFrgmTbl()
	: m_uiLength(0)
{
}


DeFrgmTbl::~DeFrgmTbl()
{
	;
}


///////////////////////////////////////////////////////////////////////////////
/// @brief Called when the first fragment is received. It will allocate the
///	needed memory to keep the final assembled message.
/// @param [in] dgramTag Datagram tag, as obtained from the 6lowPAN packet.
/// @param [in] srcAddr The 6lowPAN Source Address of this Fragment.
/// @param [in] dstAddr The 6lowPAN Destination Address of this Fragment.
/// @param [in] msgSz The final assembled message size.
/// @param [in] frgSz The fragment size.
/// @retval true A new entry is added in the MsgHandle table.
/// @retval false Error encountered.
///////////////////////////////////////////////////////////////////////////////
bool DeFrgmTbl::startMsg(uint16_t dgramTag, uint16_t srcAddr, uint16_t dstAddr, uint8_t p_u8TrafficClass
			, uint16_t msgSz, uint16_t frgSz, uint8_t* p_pu8Ipv6 )
{
	int nMsgIdx = 0;
	uint32_t key = FRGM_KEY(dgramTag,srcAddr) ;

	LOG( "DeFrgmTbl.startMsg(dgramTag %d, srcAddr %04X, dstAddr %04X, msgSz %d, frgSz %d)",
				dgramTag, srcAddr, dstAddr, msgSz, frgSz );

	if( frgSz < (1280/32) )
	{
		ERR( "DeFrgmTbl.startMsg:Invalid fragment size (%d)", frgSz );
		return false;
	}

	if( frgSz > msgSz )
	{
		ERR( "DeFrgmTbl.startMsg:Invalid fragment size (%d) with msg size (%d)", frgSz, msgSz );
		return false;
	}

	if( m_uiLength >= sizeof(m_aFrgTbl)/sizeof(m_aFrgTbl[0]) )
	{
		ERR( "DeFrgmTbl.startMsg: fragment table full!" );
		return false;
	}

	bool found = binSearch( m_uiLength, key, m_aFrgTbl, compareTagAddr, nMsgIdx );

	/// allready called startMsg
	if( found )
	{
		WARN( " DeFrgmTbl::startMsg : duplicated");
		return false;
	}

	MsgHandle * pMsg = new MsgHandle;

	pMsg->msgPtr =  new uint8_t [msgSz];
	pMsg->timestamp = time(0) ;
	pMsg->TagShortAddr = key;
	pMsg->dstAddr = dstAddr;
	pMsg->u8TrafficClass = p_u8TrafficClass;

	pMsg->pu8IPv6 = NULL;
	
	if (p_pu8Ipv6)
	{
		pMsg->pu8IPv6 = new uint8_t[IPv6_ADDR_LENGTH];
		memcpy(pMsg->pu8IPv6, p_pu8Ipv6, IPv6_ADDR_LENGTH);
	}
	

	/// set first/unused bits to 1
	pMsg->bitfield = (0xFFFFFFFE << ((msgSz-1)/frgSz));

	pMsg->msgSz = msgSz ;
	pMsg->frgSz = frgSz ;
	LOG_BIN(pMsg->bitfield, "startMsg");

	memmove( m_aFrgTbl+nMsgIdx+1, m_aFrgTbl+nMsgIdx, sizeof(m_aFrgTbl[0]) * (m_uiLength - nMsgIdx) );

	m_aFrgTbl[ nMsgIdx ] = pMsg;

	++m_uiLength;
	return true ;
}


///////////////////////////////////////////////////////////////////////////////
/// @brief Add a fragment in a partly assembled message.
/// @param [in] dgramTag Datagram tag, as obtained from the 6lowPAN packet.
/// @param [in] srcAddr The 6lowPAN Source Address of this Fragment.
/// @param [in] dgramOff The 6lowPAN Datagram offset.
/// @param [in] frg The fragment data.
/// @param [in] frgSz The fragment size.
/// @retval true A new fragment is added in the MsgHandle table.
/// @retval false Error encountered.
///////////////////////////////////////////////////////////////////////////////
bool DeFrgmTbl::addFragment( uint16_t dgramTag, uint16_t srcAddr
			, uint16_t dgramOff, const uint8_t* frg, uint16_t frgSz)
{
	if( !m_uiLength ) return false;

	LOG( "DeFrgmTbl.addFragment(dgramTag %d, srcAddr %04X,dgramOff %d, frgSz %d)"
	    , dgramTag, srcAddr, dgramOff, frgSz );

	uint32_t key = FRGM_KEY(dgramTag,srcAddr) ;
	int nMsgIdx;
	bool found = binSearch( m_uiLength, key, m_aFrgTbl, compareTagAddr, nMsgIdx );

	/// Return cause there was no startMsg before.
	if( !found  )
	{
		ERR("DeFrgmTbl::addFragment (dgramTag = %X) not found", dgramTag );
		return false;
	}

	MsgHandle * pMsg = m_aFrgTbl[nMsgIdx];

	if( dgramOff + frgSz > pMsg->msgSz )
	{
		ERR( "DeFrgmTbl.addFragment:invalid datagram offset(%d + %d > %d"
			, dgramOff, frgSz, pMsg->msgSz );
		return false ;
	}

	/// compute corespondent fragment position bit.
	uint32_t ulFrgBit = 1UL << (dgramOff/pMsg->frgSz);
	if( pMsg->bitfield & ulFrgBit )
	{
		WARN( "DeFrgmTbl.addFragment:duplicate fragment");
		return false ;
	}

	pMsg->bitfield |= ulFrgBit;

	memcpy( pMsg->msgPtr + dgramOff, frg, frgSz );

	return true ;
}


///////////////////////////////////////////////////////////////////////////////
/// @brief Drop the Messages that we're not assembled in EXPIRATION_TIMEOUT.
///////////////////////////////////////////////////////////////////////////////
void DeFrgmTbl::dropExpired()
{
	time_t tExpirationTime = time(0) - EXPIRATION_TIMEOUT;
	uint16_t i = m_uiLength;

	/// start from end because drop can perfrom memcpy
	while( i-- )
	{
		/// message expired
		if( tExpirationTime > m_aFrgTbl[i]->timestamp )
		{
			dropMsg(i) ;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
/// @brief Looks in the MsgHandle table and extract the first assembled message.
/// @return Assembled message, having a 6lowPAN header.
/// @details The 6lowPAN header on the returned Packet is needed because
/// LowPANTunnel::lowPanToIPv6 will need it to convert to an IPv6 Packet.
///////////////////////////////////////////////////////////////////////////////
ProtocolPacket* DeFrgmTbl::hasFullMsg( )
{
	for( uint16_t i=0; i< m_uiLength; ++i)
	{
		LOG("i=[%d] m_uiLength=[%d]", i, m_uiLength );
		LOG_BIN( m_aFrgTbl[i]->bitfield, "hasFulMg" ) ;
		if( 0xFFFFFFFF == m_aFrgTbl[i]->bitfield )
		{
			int nStartReservedLen = sizeof(NetMsgTrToBbrHeader) + (m_aFrgTbl[i]->pu8IPv6 ? IPv6_ADDR_LENGTH : 0) ;

			ProtocolPacket* pk_out = new ProtocolPacket() ;
			pk_out->AllocateCopy( m_aFrgTbl[i]->msgPtr, m_aFrgTbl[i]->msgSz, nStartReservedLen) ;

			/// serial header
			
			NetMsgTrToBbrHeader stSerial;
			stSerial.m_u16DstAddr = m_aFrgTbl[i]->dstAddr;
			stSerial.m_u16SrcAddr = (uint16_t)(m_aFrgTbl[i]->TagShortAddr);
			stSerial.m_u8TrafficClass = 0;
			
			if (m_aFrgTbl[i]->pu8IPv6)
			{
				pk_out->PushFront(m_aFrgTbl[i]->pu8IPv6,IPv6_ADDR_LENGTH);
			}
			pk_out->PushFront((uint8_t*)&stSerial, sizeof(NetMsgTrToBbrHeader));
			dropMsg( i ) ;
			LOG("Has FULL MESSAGE.") ;
			return pk_out ;
		}
	}
	LOG("No message yet.") ;
	return 0 ;
}


///////////////////////////////////////////////////////////////////////////////
/// @brief Drop the contents handled by a MsgHandle and erase the MsgHandle
///	entry from the table.
/// @param [in] idx The message index to be dropped.
/// @see DeFrgmTbl::dropExpired.
///////////////////////////////////////////////////////////////////////////////
void DeFrgmTbl::dropMsg( uint16_t idx )
{

	delete [] m_aFrgTbl[idx]->msgPtr;
	delete [] m_aFrgTbl[idx]->pu8IPv6;	
	delete m_aFrgTbl[idx] ;

	--m_uiLength;
	/// (using memcpy instead of memmove)
	memcpy( m_aFrgTbl+idx, m_aFrgTbl+idx+1, sizeof(m_aFrgTbl[0]) * (m_uiLength-idx) ) ;
}

