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

#ifndef _DEFRGM_TBL_H_
#define _DEFRGM_TBL_H_

//////////////////////////////////////////////////////////////////////////////
/// @file	DeFrgmTbl.h
/// @author	Marius Negreanu
/// @date	Date:Wed Nov 28 12:26:59 2007 UTC
/// @brief	`6lowPAN Fragment` assembling class (MsgHandle,DeFrgmTbl).
//////////////////////////////////////////////////////////////////////////////


class ProtocolPacket ;


//////////////////////////////////////////////////////////////////////////////
/// @struct MsgHandle
/// @ingroup Backbone
/// @brief Keeps metadata about a message that is beeing assembled.
//////////////////////////////////////////////////////////////////////////////
struct MsgHandle
{
	uint32_t bitfield ;	///< Keeps track of the fragments added so far.
	uint32_t TagShortAddr ;	///< Key for the metadata.
	time_t   timestamp ;	///< Time when the message was started.
	uint8_t* msgPtr ;	///< Data holding the final message.
	uint16_t dstAddr;	///< 6lowPAN Destination Address.

	uint16_t msgSz ;	///< Final message size.
	uint16_t frgSz ;	///< A fragment size.
	uint8_t		u8TrafficClass;
	uint8_t*	pu8IPv6;	
};


//////////////////////////////////////////////////////////////////////////////
/// @class DeFrgmTbl
/// @ingroup Backbone
/// @brief `6lowPAN Fragment` assembling class.
//////////////////////////////////////////////////////////////////////////////
class DeFrgmTbl
{
public:
	DeFrgmTbl() ;
	~DeFrgmTbl() ;

public:
	/// Computes the Fragmentation table index as TAG|ADDR.
	#define FRGM_KEY(TAG,ADDR) ( ((uint32_t)(TAG) <<16) | (ADDR) )

	/// sorted insert in the table
	bool startMsg( uint16_t dgramTag, uint16_t srcAddr, uint16_t dstAddr, uint8_t p_u8TrafficClass, uint16_t msgSz, uint16_t frgSz, uint8_t* p_pu8Ipv6 = NULL ) ;

	/// sorted add in the table
	bool addFragment( uint16_t dgramTag, uint16_t srcAddr, uint16_t dgramOff, const uint8_t* frg, uint16_t frgSz) ;

	/// drop expired packets
	void dropExpired() ;

	/// get, if any, completed message
	ProtocolPacket* hasFullMsg() ;

	/// in case of expired message, or overlaped fragments.
	void dropMsg( uint16_t idx ) ;

	/// Used as a parameter to binSearch
	static int compareTagAddr( uint32_t key, MsgHandle* pMsg) {
		return ( key < pMsg->TagShortAddr ? -1 : (key != pMsg->TagShortAddr) );
	}

protected:
	MsgHandle* m_aFrgTbl[256];
	uint16_t m_uiLength ;
};

#endif	//_DEFRGM_TBL_H_
