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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "FileLink.h"


CFileLink::CFileLink() : m_handle(-1), m_eof(false)
{
}

CFileLink::~CFileLink()
{
}

int
CFileLink::OpenLink( const char* file, int flags)
{
	if( -1 == (m_handle=::open(file, flags)) )
	{
		ERR("Unable to open FileLink to [%s:%.2X] : %s", file, flags, strerror(errno) ) ;
	}
	return m_handle ;
}


// Not really optimized :D
int CFileLink::GetMsgLen( void )
{
	off_t off = lseek( m_handle, 0, SEEK_CUR );
	int len=0, rv=0;
	char buf[1]={0};
	while( true )
	{
		rv = ::read( m_handle, buf, 1 ) ;
		if( -1 == rv )
			break ;

		if( rv == EOF || buf[0] == '\r' || buf[0] == '\n' )
		{
			rv = len ;
			break ;
		}
		++len ;
	}
	lseek( m_handle, off, SEEK_SET );
	return rv ;
}

ssize_t
CFileLink::Read( void* msg, size_t count )
{
	ssize_t rv ;
	rv = ::read( m_handle, msg, count ) ;
	switch( rv )
	{
		case 0:
			if( !m_eof )
			{
				INFO("End of file encountered.") ;
				m_eof = true ;
			}
			break ;
		case -1:
			ERR("Error while reading from file: [%s].", strerror(errno) );
			break ;
		default:
			INFO("Read [%d] bytes.", rv );
			break ;
	}
	if( !msg || ((char*)msg)[0] != 'I' || ((char*)msg)[1] != ':' )
	{
		ERR("msg is null\n");
		return 0 ;
	}

	if( ((uint8_t*)msg)[rv-1] == 0x0A )
	{
		((uint8_t*)msg)[rv-1] = 0x00 ;
		rv -= 1 ;
	}

	int n=0, i=0;
	unsigned int word=0 ;
	uint8_t* bin = (uint8_t*)msg ;

	while ( sscanf( ((char*)msg)+3, "%X%n", &word, &n ) == 1 )
	{	msg = (uint8_t*)msg + n;
		bin[i++] = word ;
	}
	LOG_HEX( 0, "BIN:", bin, i) ;
	return i ;
}


ssize_t
CFileLink::Write( const uint8_t* msg, uint16_t count )
{
	ssize_t rv ;
	char* buf = (char*)malloc( count*5 );
	int n =0;

	::write( m_handle, "\nO: ", 4 );

	for( int i =0; i < count; ++i )
	{
		n += snprintf( buf+n, 4, "0x%.2X ", msg[i] );
	}

	rv = ::write( m_handle, buf, count*5) ;
	if( -1 == rv )
		ERR("Error writing in file: %s\n", strerror(errno) ) ;
	else
		INFO("Wrote [%d] bytes\n", rv );
	free( buf );
	return rv ;
}


int
CFileLink::CloseLink( )
{
	int rv = ::close( m_handle ) ;
	if( 0 != rv )
		ERR("Unable to close file [%s]", strerror(errno) );

	return rv ;
}
