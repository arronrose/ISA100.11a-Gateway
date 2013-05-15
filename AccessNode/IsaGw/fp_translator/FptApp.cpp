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

#include "FptApp.h"
#include "Shared/Utils.h"
//#include "Shared/StreamLink.h"
#include "Shared/SignalsMgr.h"


#include "../ISA100/porting.h"
#include "Shared/log_callback.h"
#include "../ISA100/callbacks.h"
#include "../ISA100/nlme.h"



CFptApp::CFptApp()
:CApp( "FPT" )
{
	strcpy(m_gwAddress, "10.32.0.121");
	m_gwPort = 4900;
	m_reqTimedOut = 180;
	m_logLevel = 3;
}

CFptApp::~CFptApp()
{
	;
}

int CFptApp::Init()
{

	if( !CApp::Init( NIVIS_TMP"fp_translator.log") )
	{	ERR( "CFptApp.Init: CApp.Init failed.");
		return 0 ;
	}


	CSignalsMgr::Install(SIGUSR1);
	CSignalsMgr::Ignore(SIGUSR2);

	
	if( !m_cfg.Init() )
	{	ERR( "CFptApp.Init: Config.Init failed");
		return false ;
	}	
	

	//m_oSerialLink.OpenLink (m_cfg.m_TtyDev, m_cfg.m_TtyBauds);

	//m_oSerialLink.SetRawLog(m_cfg.m_useRawLog);

	if (!loadTunnelIOConfig())
	{
		return 0;
	}

	if (!loadTunnelsConfig())
	{
		return 0;
	}
	LOG("CFptApp::Init: done");

	return true ;
}

#define FP_TRANSLATOR_FILE NIVIS_PROFILE"./config.ini"
int CFptApp::loadTunnelIOConfig()
{
	CIniParser oIniParser;
	if( !oIniParser.Load (FP_TRANSLATOR_FILE) )
		return 0;

	if (!oIniParser.GetVarRawString("fp_translator", "GW_IP", m_gwAddress, sizeof(m_gwAddress)))
		return 0;
	
	char szLogVarString[256] = "";
	if (!oIniParser.GetVarRawString("fp_translator", "GW_PORT", szLogVarString, sizeof(szLogVarString)))
		return 0;
	m_gwPort = atoi(szLogVarString);
	
	if (!oIniParser.GetVarRawString("fp_translator", "REQ_TIMEDOUT", szLogVarString, sizeof(szLogVarString)))
		return 0;
	m_reqTimedOut = atoi(szLogVarString);
	
	if (!oIniParser.GetVarRawString("fp_translator", "LOG_LEVEL", szLogVarString, sizeof(szLogVarString)))
		return 0;
	m_logLevel = atoi(szLogVarString);

	return 1;
}

#define TUNNELS_FILE NIVIS_PROFILE"./tunnels_config.ini"

int CFptApp::loadTunnelsConfig()
{
	m_oTunnelVector.clear();
	CIniParser oIniParser;

	if( !oIniParser.Load (TUNNELS_FILE) )
		return 0;

	for( int i = 0; oIniParser.FindGroup("tunnel", i); i++ )
	{
		CTunnelItem::Ptr pTunnel(new CTunnelItem);

		if (!oIniParser.GetVar(NULL, "Network_Address", &pTunnel->m_oNetworkAddress))
		{	continue;
		}

		if (!oIniParser.GetVar(NULL, "TunnelRemotePort", &pTunnel->m_nTunnelRemotePort))
		{	continue;
		}
		if (!oIniParser.GetVar(NULL, "TunnelRemoteObject", &pTunnel->m_nTunnelRemoteObject))
		{	continue;
		}

		if (!oIniParser.GetVar(NULL, "TunnelLocalPort", &pTunnel->m_nTunnelLocalPort))
		{	continue;
		}
		if (!oIniParser.GetVar(NULL, "TunnelLocalObject", &pTunnel->m_nTunnelLocalObject))
		{	continue;
		}

		if (!oIniParser.GetVar(NULL, "LinkType", &pTunnel->m_nLinkType))
		{	continue;
		}

		pTunnel->m_nConnParam1 = 0;
		pTunnel->m_nConnParam2 = 0;
		pTunnel->m_szConn[0] = 0;
		
		char szBaud[256];
		switch(pTunnel->m_nLinkType)
		{
		case LINK_SERIAL:
			if (!oIniParser.GetVar(NULL, "serial", pTunnel->m_szConn, sizeof(pTunnel->m_szConn)))
			{	continue;
			}

			if (!oIniParser.GetVar(NULL, "baud", szBaud, sizeof(szBaud)))
			{	continue;
			}
			pTunnel->m_nConnParam1 = CConfig::GetBaudRate(szBaud);

			if (pTunnel->m_nConnParam1 < 0)
			{
				continue;
			}
			break;
		case LINK_TCP_CLIENT:			
		case LINK_TCP_SERVER:			
		case LINK_UDP:
			if (!oIniParser.GetVar(NULL, "PortLocal", &pTunnel->m_nConnParam1))
			{	continue;
			}

			oIniParser.GetVar(NULL, "IP_Remote", pTunnel->m_szConn, sizeof(pTunnel->m_szConn), 0, false);
			oIniParser.GetVar(NULL, "PortRemote", &pTunnel->m_nConnParam2);
			break;

		default:
			LOG("CFptApp::loadTunnelsConfig: unknown link type %d", pTunnel->m_nLinkType );
			continue;
		}


		switch(pTunnel->m_nLinkType)
		{
		case LINK_SERIAL:
			pTunnel->m_pLinkPtr.reset (new CSerialLink);
			break;
		case LINK_TCP_CLIENT:
			pTunnel->m_pLinkPtr.reset (new CTcpClientLink);
			break;
		case LINK_TCP_SERVER:
			pTunnel->m_pLinkPtr.reset (new CTcpServerLink);
			break;
		case LINK_UDP:
			pTunnel->m_pLinkPtr.reset (new CUdpLink);
			break;

		default:
			LOG("CFptApp::loadTunnelsConfig: unknown link type %d", pTunnel->m_nLinkType );
			continue;
		}
		pTunnel->m_pLinkPtr->SetRawLog(m_cfg.m_useRawLog);
		m_oTunnelVector.push_back(pTunnel);
	}

	//tunnel 
	m_tunnelPtr.reset(new CTunnelIO(&m_oTunnelVector, m_oTunnelVector.size(), m_gwPort, m_gwAddress, m_reqTimedOut));
	SetLogLevel(m_logLevel);

	return m_oTunnelVector.size();
}


int CFptApp::readFromTunnels()
{
	for (int i=0; i < (int)m_oTunnelVector.size(); i++)
	{
		CTunnelItem::Ptr pTunnel = m_oTunnelVector[i]; //just for easy of use
		CStreamLinkPtr pTunnelLink = pTunnel->m_pLinkPtr; //just for easy of use

		if (!pTunnelLink->IsLinkOpen())
		{				
			if ( !pTunnelLink->OpenLink(pTunnel->m_szConn, pTunnel->m_nConnParam1, pTunnel->m_nConnParam2))
			{
				continue;
			}
		}
		
		u_char pSerialBuffer[10*1024];
		int nReadLen = pTunnelLink->Read(	pSerialBuffer, sizeof(pSerialBuffer));

		if (nReadLen<0)
		{
			pTunnelLink->CloseLink();
			continue;
		}

		if (nReadLen > 0)
		{
			//send nReadLen from pSerialBuffer to device			
			
			//tunnel
			m_tunnelPtr->PushReqData(i, pSerialBuffer, nReadLen);
		}
	}

	return 1;
}



int CFptApp::writeToTunnels()
{
	for (int i=0; i < (int)m_oTunnelVector.size(); i++)
	{
		CTunnelItem::Ptr pTunnel = m_oTunnelVector[i]; //just for easy of use
		CStreamLinkPtr pTunnelLink = pTunnel->m_pLinkPtr; //just for easy of use

		if (!pTunnelLink->IsLinkOpen())
		{				
			if ( !pTunnelLink->OpenLink(pTunnel->m_szConn, pTunnel->m_nConnParam1, pTunnel->m_nConnParam2))
			{
				continue;
			}
		}
		
		while (m_tunnelPtr->IsRespData(i))
		{
			u_char pSerialBuffer[10*1024];
			int nDeviceDataLen = 0;

			//tunnel
			m_tunnelPtr->WriteRespData(i, pSerialBuffer, &nDeviceDataLen);
			
			if (pTunnelLink && nDeviceDataLen > 0)
			{
				pTunnelLink->Write(pSerialBuffer,nDeviceDataLen);
			}

			//tunnel
			m_tunnelPtr->PopRespData(i);
		}
	}

	return 1;
}




void CFptApp::Run()
{
	timer1s.SetTimer(1000);
	
	LOG("inainte de start");

	//tunnel
	m_tunnelPtr->StartIO();


	while( !CApp::IsStop() )
	{	
		readFromTunnels();

		// suppose that in pSerialBuffer there are nDeviceDataLen bytes received from device

		//tunnel
		m_tunnelPtr->PerformIO();
		
		writeToTunnels();

		if (timer1s.IsSignaling()) 
		{
			TouchPidFile(m_szAppPidFile);

			timer1s.SetTimer(1000);	
		}	
		

		if (CSignalsMgr::IsRaised(SIGUSR1)) 
		{
			//m_obTunnel.LogRoutes();
			CSignalsMgr::Reset(SIGUSR1);
		}
	}

	//tunnel
	m_tunnelPtr->StopIO();
}

