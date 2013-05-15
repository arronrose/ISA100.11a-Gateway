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

#ifndef _FPT_APP_H_
#define _FPT_APP_H_


#include "../../Shared/app.h"

#include "Shared/SimpleTimer.h"

#include "FptTypes.h"

#include "../../Shared/StreamLink.h"


#include "../ISA100/typedef.h"
#include "../ISA100/porting.h" 


//tunnel
#include "./tunnel/TunnelIO.h"

#include "FptCfg.h"

//////////////////////////////////////////////////////////////////////////////
/// @class CFptApp
/// @ingroup fp_translator
//////////////////////////////////////////////////////////////////////////////
class CFptApp : public CApp
{
public:
	CFptApp() ;
	~CFptApp() ;
public:
	int Init() ;
	void Run() ;



public:
	CFptCfg			m_cfg;
	
	CTunnelVector	m_oTunnelVector;
	//CSerialLink		m_oSerialLink;

private:
	int loadTunnelsConfig();
	int readFromTunnels();
	int writeToTunnels();
	CSimpleTimer timer1s;

	//tunnel 
	int loadTunnelIOConfig();
	CTunnelIO::Ptr m_tunnelPtr;
	char    m_gwAddress[256];
	int		m_gwPort;
	int		m_reqTimedOut;
	int		m_logLevel;
};

extern CFptApp* g_pApp ;

#endif	// _FPT_APP_H_
