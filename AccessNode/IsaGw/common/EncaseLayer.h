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

#ifndef _ENCASE_LAYER_H_
#define _ENCASE_LAYER_H_

#include "Shared/h.h"
#include "ProtocolLayer.h"
#include "TtyLink.h"
#include "FileLink.h"
#include "Shared/StreamLink.h"


///////////////////////////////////////////////////////////////////////////////
//! @class EncaseLayer
//! @brief Exposes a single interface to TtyLink, FileLink and UdpLink.
///////////////////////////////////////////////////////////////////////////////
template<class BaseClass>
class EncaseLayer : public ProtocolLayer, BaseClass
{
public:
	EncaseLayer( bool rawLog )
		: ProtocolLayer(ENCASE_LAYER)
		, BaseClass(rawLog)
		, m_uiHeadSpare(0)
		, m_uiTailSpare(0)
		, m_bEscMsg(false)
		{
		}
	~EncaseLayer() {} ;
public:
	bool		Create( ) ;
	int		OpenLink( const char* p_szDev, uint32_t p_uiDstPort, uint32_t p_uiLocalPort=0, bool p_bEscMsg=false ) ;
	void		SetOffsets( uint16_t p_uiHeadSpare , uint16_t p_uiTailSpare) ;
	ProtocolPacket* Read( int tout = 1000 ) ;
	ProtocolPacket* Write( const uint8_t* p_prguiMsg, uint16_t p_uiMsgSz ) ;
	int SendTo( const char *p_szIP, int p_iRemotePort, const uint8_t *p_rguiMsg, uint16_t p_uiMsgSz) ;

protected:
	int	Write( ProtocolPacket* pkt ) ;
private:
	uint16_t	m_uiHeadSpare ;	///< A ProtocolPacket head spare.
	uint16_t	m_uiTailSpare;	///< A ProtocolPacket tail spare.
	bool		m_bEscMsg ;	///< Do character escaping.
	CNivisFrameParser_v2	m_oParse ;
};

#include "common/EncaseLayer.cpp"
#endif	//_ENCASE_LAYER_H_

