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

///
/// @file
/// @mainpage NISA 100 Backbone Description.
/// @section OverViewSec Overview.
/// This document provides a comprehensive architectural overview of the
/// Backbone Router, using a number of different architectural views to
/// depict different aspects of the system. It is intended to capture and
/// convey the significant architectural decisions which have been made on the
/// Gateway.
/// @section DataFlowSec Internal data flow
/// <pre>
/// +---------------------------+                  +---------------------------+
/// | ASL                       |<-Route Request---o Router                    |
/// |                           o--RouteAdd------->|                           |
/// +---------------------------+                  |                           |
///                                                |                           |
/// +---------------------------+                  |                           |
/// | NL                        |                  |                           |
/// |Confirmation {             |                  |                           |
/// | if x80000 then goUp       |                  |                           |
/// | if h>= 0x8000    o--------+-Add Confirm MSG->|                           |
/// |}                          |    (h-0x8000)    |                           |
/// |Data Request {             |                  |                           |
/// | if dstLen=8 then goDown   |                  |                           |
/// | if dstLen=2      o--------+--Add NWK MSG---->|                           |
/// |}                          |                  |                           |
/// |Indicate {                 |                  |                           |
/// | if srcLen=8               |                  |                           |
/// | or dest=MyAddr then goUp  |                  |                           |
/// | if srcLen=2               |                  |                           |
/// | and dest<>myAddr o--------+-Add NWK MSG----->|                           |
/// |}                          |                  | Rx UART MSG {             |
/// +---------------------------+<--Indicate-------+-o if dest =0              |
///                                                |    or dest=myAddr         |
/// +---------------------------+                  |                           |
/// |  DLL                      |<--Data Request---+-o if dest<>0              |
/// +---------------------------+   h|0x8000       |     and dest<>myAddr      |
///                                                +---------------------------+
/// Observation:
///	device application handles must be between 0 and 0x7FFF
///	Route messages handle must be betweeen 0 and 0x7FFF
/// </pre>
/// @subsection TransToRout Transceiver to Router.
/// @msc
/// "DLL", "Router","SM", "GW";
///
/// "DLL"->"Router" [label="1:BufferReady(pt)", URL="\ref LowPANTunnel::handleDown"] ;
/// "Router"=>"Router" [label="1.1:SetFlowControl(pt)", URL="\ref MsgQueue::SetFlowControl"];

/// "DLL"=>"Router" [label="2:NetworkMsg(P)", URL="\ref LowPANTunnel::handleDown"] ;
/// "DLL"<="Router" [label="2.1:sendAck", URL="\ref LowPANTunnel::sendAck"];
/// "Router"=>"Router" [label="2.2:handleNetPkt(P)", URL="\ref LowPANTunnel::handleNetPkt"];
/// "Router"=>"Router" [label="2.2.1:handleFull6lowPANMsg", URL="\ref LowPANTunnel::handleFull6lowPANMsg"];
/// "DLL"<="Router" [label="2.2.1.1:!src_addr then\nlookupIPv6Addr(src_addr)", URL="\ref LowPANTunnel::lookupIPv6Addr"];
/// "DLL"<="Router" [label="2.2.1.2:!dst_addr then\nlookupIPv6Addr(dst_addr)", URL="\ref LowPANTunnel::lookupIPv6Addr"];
/// "Router"=>"SM" [label="2.2.2.1:isIPv6ForSM\nSend", URL="\ref LowPANTunnel::lookupIPv6Addr"];
/// "Router"=>"GW" [label="2.2.2.2:!isIPv6ForSM\nSend", URL="\ref LowPANTunnel::lookupIPv6Addr"];
/// @endmsc
///
/// @subsection UDPToRout UDP to Router.
/// @defgroup Backbone ISA100 Backbone
/// @{ @}
#include "BackBoneApp.h"
CBackBoneApp* g_pApp;
int main( int argc, char** argv)
{
	log2flash("BBR started version %s",  CApp::GetVersion());

	g_pApp = new CBackBoneApp;
	
	if ( !g_pApp->Init() )
	{
		ERR("main: Failed to start BackBone.");
		log2flash("BBR ended <- init failed ");
		delete g_pApp ;
		return EXIT_FAILURE ;
	}
	g_pApp->Run();
	g_pApp->Close();
	log2flash("BBR ended");
	delete g_pApp ;
	return EXIT_SUCCESS ;
}
