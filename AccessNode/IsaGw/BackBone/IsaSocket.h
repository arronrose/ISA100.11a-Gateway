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

#ifndef _ISA_SOCKET_H_
#define _ISA_SOCKET_H_
//////////////////////////////////////////////////////////////////////////////
/// @file	IsaSocket.h
/// @author	Marius Negreanu
/// @date	Wed Nov 28 12:26:59 2007 UTC
/// @brief	The 6lowPAN Socket (IsaSocket) .
//////////////////////////////////////////////////////////////////////////////



class ProtocolLayer ;

//////////////////////////////////////////////////////////////////////////////
/// @class IsaSocket
/// @ingroup Backbone
/// @brief A Isa socket.
//////////////////////////////////////////////////////////////////////////////
class IsaSocket
{
public:
	IsaSocket( bool rawLog ) : m_first(0), m_rawLog(rawLog) { } ;
	~IsaSocket() 
	{
		delete m_first ;
		
	} ;
public:

	int Write( ProtocolPacket *pkt );
	ProtocolPacket* Read( int tout ) ;
	int Close() { return 0; } ;


	template <class Encasing>
	int OpenLink( const char* device, uint32_t flags, uint32_t local_port=0, bool escapeMsg=false)
	{
		EncaseLayer<Encasing>* el = new EncaseLayer<Encasing>(m_rawLog) ;

		int nRet = el->OpenLink(device, flags, local_port, true);
		
		/// For 6lowpan to IPv6 conversion we reserve a 36 spare bytes.
		/// For now is hardcoded, but it will use the sizeof(headers)
		el->SetOffsets( 36, 0 );
		m_first = el ;

		m_first->SetBelow(0);
		m_first->SetAbove(0);

		return  nRet; 
	}

protected:
	ProtocolLayer*	m_first ;
	bool		m_rawLog ;
};
#endif	//_ISA_SOCKET_H_
