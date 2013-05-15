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

#ifndef _FPT_TYPES_H
#define _FPT_TYPES_H

#include "../../Shared/Common.h"
#include <boost/shared_ptr.hpp>
#include <vector>

class CStreamLink;
typedef boost::shared_ptr<CStreamLink> CStreamLinkPtr;

class CTunnelItem
{
public:
	typedef boost::shared_ptr<CTunnelItem> Ptr;

public:
	TIPv6Address  m_oNetworkAddress ; //ipv6 in hex

	int m_nTunnelRemotePort;	//1-15
	int m_nTunnelRemoteObject;

	int	m_nTunnelLocalPort;  //1-15
	int	m_nTunnelLocalObject;  

	int	m_nLinkType;	// 0 - serial 3 -udp 

	char m_szConn[256];	//	serial = /dev/ttyS1
	int	m_nConnParam1;			// baud or port = B115200
	int	m_nConnParam2;	


	CStreamLinkPtr	m_pLinkPtr;	
}; 

typedef std::vector<CTunnelItem::Ptr> CTunnelVector;

#endif //_FPT_TYPES_H
