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
/// @file	IsaSocket.cpp
/// @author	Marius Negreanu
/// @date	Wed Nov 28 12:26:59 2007 UTC
/// @brief	The 6lowPAN Socket.
//////////////////////////////////////////////////////////////////////////////

#include "common/EncaseLayer.h"
#include "LowPANTunnel.h"
#include "IsaSocket.h"
#include <sys/time.h>

int IsaSocket::Write( ProtocolPacket* pkt )
{
	return m_first->Write( pkt );
}


ProtocolPacket* IsaSocket::Read( int tout )
{
	assert(m_first);

	ProtocolPacket* pkt = m_first->Read( tout ) ;
	if( !pkt ) return 0 ;
	return pkt ;
}

