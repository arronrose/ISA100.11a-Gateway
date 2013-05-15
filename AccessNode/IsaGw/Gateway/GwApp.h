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

/// @file GwApp.h
/// @brief Declarations for the entire gateway application
#ifndef GWAPP_H
#define GWAPP_H

#include "../../Shared/app.h"
#include "../../Shared/SimpleTimer.h"

#include "ISAConfig.h"
#include "GwUAP.h"
#include "TcpGSAP.h"

#include "../../Shared/ServerSocket.h"

/// Main application class
class CGwApp : public CApp
{
public:
	CGwApp();
	~CGwApp();
	void USR2_Handler( void );
	void HUP_Handler( void );
	int Init( void );	///< Mandatory initialisations

	int Run( void );	///< Main loop. this should never end,  unless rebooted/killed, of course
	void Dump( void );	///< Dump to log the gateway status
	void Close();

	/// TODO: MAKE m_oGwUAP private, forward methods currently used trough g_stApp.m_oGwUAP.
	/// Do not use operators like operator CGwUAP*
	CGwUAP		m_oGwUAP;		///< the gateway user application process
	CISAConfig 	m_stCfg;			///< the gateway configuration



private:
	CTcpGSAP * lookForNewTcpSession(CServerSocket& p_rServer, bool p_bSsl = false);

	void lookForNewTcpSessions( void );

	CSimpleTimer timer100ms;	///< 100ms timer
	CSimpleTimer timer1s;		///< 1 sec timer
	
private:
	CServerSocket m_oGSAPServer;
	CServerSocket m_oYGSAPServer;

	CServerSocket m_oGSAPServer_SSL;
	CServerSocket m_oYGSAPServer_SSL;
};

extern CGwApp g_stApp;

#endif
