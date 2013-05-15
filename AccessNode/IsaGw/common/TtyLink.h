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

#ifndef _TTY_LINK_H_
#define _TTY_LINK_H_

//////////////////////////////////////////////////////////////////////////////
/// @file	TtyLink.h
/// @author	Marius Negreanu
/// @date	Wed Nov 28 12:26:59 2007 UTC
/// @brief	TtyLink.
//////////////////////////////////////////////////////////////////////////////

#include "Shared/link.h"

#define IN
#define OUT


class DummySock
{
public:
	DummySock() {};
	~DummySock(){};
	int SendTo(...) { return true ; }

} ;

//////////////////////////////////////////////////////////////////////////////
/// @class CTtyLink
//////////////////////////////////////////////////////////////////////////////
class CTtyLink : CLink {
public:
	CTtyLink() {m_nothingReadTime=0;m_NoOpTimeout = 60 ;}
	CTtyLink(bool p_bRawLog):CLink(p_bRawLog)  {m_nothingReadTime=0;m_NoOpTimeout=60;}
	~CTtyLink() {} ;
public:
	int     OpenLink( const char *file, int flags, int ) { m_file = file ; m_flags = flags ; return CLink::OpenLink( m_file, m_flags) ;}
	void    CloseLink() { return CloseLink() ; }
	int		GetMsgLen( int tout=1000 ) ;
	ssize_t Read( void* msg, int msgSz ) ;
	ssize_t Write( const uint8_t* msg, uint16_t msgSz ) ;

protected:
	unsigned char	m_buf[512];
	uint16_t	m_bufLen ;
	DummySock	m_oSock;
	const char	*m_file ;
	int		m_flags ;
	CNivisFrameParser_v2	m_oParse ;
	int		reopenLink() ;
	time_t		m_nothingReadTime ;
	time_t		m_NoOpTimeout ;
};

#endif	//_TTY_LINK_H_
