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

#ifndef _PACKET_MATCHER_H_
#define _PACKET_MATCHER_H_

//////////////////////////////////////////////////////////////////////////////
/// @file	PacketMatcher.h
/// @author	Marius Negreanu
/// @date	Wed Nov 28 12:26:59 2007 UTC
/// @brief	Implements a packet matcher (PacketMatcher).
//////////////////////////////////////////////////////////////////////////////

#include "Shared/ProtocolPacket.h"


//////////////////////////////////////////////////////////////////////////////
/// @class PacketMatcher
/// @ingroup Backbone
/// @brief Maps an incomming integer value to an enum as the Packet type.
//////////////////////////////////////////////////////////////////////////////
class PacketMatcher
{
public:
	enum PKT_TYPE {
		PKT_BUFF_RDY		=0x00,
		PKT_BUFF_FULL		=0x01,
		PKT_BUFF_CHK		=0x02,
		PKT_BUFF_MIC_ERR	=0x03,
		PKT_MESG_ACK		=0x10,
		PKT_MESG_NET		=0x80,
		PKT_MESG_APP		=0x81,
		PKT_NTWK_CNF		=0x82,
		PKT_NTWK_TR2BBR		=0x83,
		PKT_NTWK_BBR2TR		=0x84,
		PKT_LOCAL_CMD		=0x85,	
		PKT_FRAG_START,
		PKT_FRAG_NEXT,
		PKT_INVALID,
		PKT_TR_DEBUG		=0xFF
	};

	enum LOCAL_CMD_IDS
	{
		LOCAL_CMD_BBR_PWR			= 1,
		LOCAL_CMD_PROV				= 2,
		LOCAL_CMD_RT_ATT			= 3,
		LOCAL_CMD_KEY				= 4,
		LOCAL_CMD_ALERT				= 5,
		LOCAL_CMD_ENG_MODE_SET		= 6,
		LOCAL_CMD_ENG_MODE_GET		= 7
	};
	inline static PacketMatcher::PKT_TYPE GetType(uint8_t* msg, uint16_t len ) ;
	inline static const char* GetTypeString(uint8_t* msg, uint16_t len ) ;
protected:
	PacketMatcher() {} ;
};

PacketMatcher::PKT_TYPE
PacketMatcher::GetType( uint8_t* pkt, uint16_t len )
{
	if( ! pkt || !len ) return PKT_INVALID ;

	switch( *pkt ) {
		case 0x00:
			//LOG("PKT_TYPE: PKT_BUFF_RDY (%d bytes)", len);
			return PKT_BUFF_RDY;
		case 0x01:
			WARN("PKT_TYPE: PKT_BUFF_FULL (%d bytes)", len);
			return PKT_BUFF_FULL;
		case 0x02:
			//LOG("PKT_TYPE: PKT_BUFF_CHK (%d bytes)", len);
			return PKT_BUFF_CHK ;
		case 0x03:
			ERR("PKT_TYPE: PKT_BUFF_MIC_ERR (%d bytes)", len);
			return PKT_BUFF_MIC_ERR ;
		case 0x10:
			//LOG("PKT_TYPE: PKT_MESG_ACK (%d bytes)", len);
			return PKT_MESG_ACK ;
		case 0x80:
			// Do not log periodical time sync messages
			//	(messages with no payload, only type+handler+ccm)
			//if( len != (3+4 /*sizeof(LayerMsgHdr) + CCM_MIC_LENGTH*/) )
			//{
			//LOG("PKT_TYPE: PKT_MESG_NET (%d bytes)", len);
			//}
			return PKT_MESG_NET ;
		case 0x81:
			//LOG("PKT_TYPE: PKT_MESG_APP (%d bytes)", len);
			return PKT_MESG_APP ;
		case 0x82:
			//LOG("PKT_TYPE: PKT_NTWK_CNF (%d bytes)", len);
			return PKT_NTWK_CNF ;
	
		case PKT_NTWK_TR2BBR:
			//LOG("PKT_TYPE: PKT_NTWK_CNF (%d bytes)", len);
			return PKT_NTWK_TR2BBR ;
		case PKT_NTWK_BBR2TR:
			//LOG("PKT_TYPE: PKT_NTWK_CNF (%d bytes)", len);
			return PKT_NTWK_BBR2TR ;
		case PKT_LOCAL_CMD:
			//LOG("PKT_TYPE: PKT_NTWK_CNF (%d bytes)", len);
			return PKT_LOCAL_CMD ;
		case PKT_TR_DEBUG:
			//LOG("PKT_TYPE: PKT_NTWK_CNF (%d bytes)", len);
			return PKT_TR_DEBUG ;
		default:
			//LOG("PKT_TYPE: PKT_INVALID (%d bytes)", len);
			LOG_HEX("PKT_INVALID", pkt,len);
			return PKT_INVALID ;
	}
	//return PKT_INVALID ; // never reached.
}

const char * PacketMatcher::GetTypeString( uint8_t* pkt, uint16_t len )
{
	if( ! pkt || !len ) return "PKT_INVALID" ;

	switch( *pkt ) {
		case 0x00:	return "PKT_BUFF_RDY ";
		case 0x01:	return "PKT_BUFF_FULL";
		case 0x02:	return "PKT_BUFF_CHK ";
		case 0x03:	return "PKT_BUFF_MIC_ERR";
		case 0x10:	return "PKT_MESG_ACK ";
		case 0x80:	return "PKT_MESG_NET ";
		case 0x81:	return "PKT_MESG_APP ";
		case 0x82:	return "PKT_NTWK_CNF ";
		case 0x83:	return "PKT_NTWK_TR2BBR ";
		case 0x84:	return "PKT_NTWK_BBR2TR ";
		case 0x85:	return "PKT_LOCAL_CMD ";
		case 0xFF:	return "PKT_TR_DEBUG ";
		default:	return "PKT_INVALID  ";
	}
}
#endif	//_PACKET_MATCHER_H_
