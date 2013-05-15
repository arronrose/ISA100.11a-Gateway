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

#ifndef _FILE_LINK_H_
#define _FILE_LINK_H_

#include "Shared/h.h"

class CFileLink {
public:
    CFileLink() ;
    ~CFileLink() ;
    int OpenLink( const char* file, int flags ) ;
    ssize_t Read( void* msg, size_t msgSz ) ;
    ssize_t Write( const uint8_t* msg, uint16_t msgSz ) ;
    int GetMsgLen( void ) ;
    int CloseLink() ;
protected:
    int m_handle ;
    bool m_eof ;
};

#endif	// _FILE_LINK_H_
