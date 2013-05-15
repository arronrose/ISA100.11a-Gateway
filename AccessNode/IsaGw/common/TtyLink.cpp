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

#include "Shared/h.h"
#include "TtyLink.h"


///////////////////////////////////////////////////////////////////////////////
//! @retval 0 Error
//! @retval !0 Success */
///////////////////////////////////////////////////////////////////////////////
ssize_t CTtyLink::Read( void* omsg, int msgSz )
{
	memcpy( omsg, m_buf+1, msgSz ); // m_buf+1 means that STX isn't copied
	//LOG_HEX( 0, "CTtyLink.Read", (const unsigned char*)omsg, msgSz );

	int nBufLen = m_bufLen - (msgSz+1+1); // remove also ETX or STX
	if( nBufLen > 0 )
	{
		m_bufLen = nBufLen;
		memmove( m_buf, m_buf+msgSz+2, nBufLen);
	}
	else // nothing left
	{
		m_bufLen = 0;
	}
	return msgSz ;
}


///////////////////////////////////////////////////////////////////////////////
//! @retval 0 Error
//! @retval !0 ok */
///////////////////////////////////////////////////////////////////////////////
ssize_t CTtyLink::Write( const uint8_t* msg, uint16_t msgSz )
{
	if (!IsLinkOpen())
	{
		reopenLink();	
	}
	return writeBlock( msg, msgSz ) ;
}


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
int CTtyLink::GetMsgLen( int tout/*=1000*/ )
{
	if (!IsLinkOpen())
	{
		reopenLink();
		m_nothingReadTime = 0;
	}
	int length=m_oParse.ParseRxMessage( m_buf, m_bufLen ) ;
	if ( length > 0 ) return length ;

	if ( sizeof(m_buf) <= m_bufLen ) {
		//The buffer is full, and no ETX was received.
		//Drop the whole buffer and go on reading.
		m_bufLen = 0 ;
	}

	int rv = readBlock( m_buf + m_bufLen, sizeof(m_buf) - m_bufLen, 0, tout ) ;
	if ( rv < 0 )
	{
		ERR("CTtyLink.GetMsgLen: readBlock() error. ReOpening Link.");
		reopenLink() ;
		return 0;
	}

	if ( rv ==0 )
	{
		if ( !m_nothingReadTime ) m_nothingReadTime = time(NULL);
		else if ( (time(NULL) - m_nothingReadTime) > m_NoOpTimeout )
		{
			reopenLink() ;
			m_nothingReadTime = 0;
		}

	}else
	{
		m_nothingReadTime = 0;
	}
	m_bufLen += rv ;
	return m_oParse.ParseRxMessage( m_buf, m_bufLen ) ;
}
int CTtyLink::reopenLink()
{
	while ( true )
	{
		CLink::CloseLink() ;
		if ( CLink::OpenLink(m_file, m_flags) )
			return true ;
		sleep( 5 ) ;
	}
	return true ;
}
