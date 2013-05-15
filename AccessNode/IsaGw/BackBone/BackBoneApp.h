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

#ifndef _BACKBONE_APP_H_
#define _BACKBONE_APP_H_

#include "Shared/app.h"
#include "Shared/SimpleTimer.h"
#include "BackBoneCfg.h"
#include "LowPANTunnel.h"


#include "../ISA100/typedef.h"
#include "../ISA100/porting.h" 

class LowPANTunnel ;

//////////////////////////////////////////////////////////////////////////////
/// @class CBackBoneApp
/// @ingroup Backbone
//////////////////////////////////////////////////////////////////////////////
class CBackBoneApp : public CApp
{
public:
	CBackBoneApp() ;
	~CBackBoneApp() ;
public:
	int Init() ;
	void Run() ;

	static int SendToDL ( IPv6Packet* p_pIPv6, int p_nLen );
	static int SendAlertToTR( const uint8_t* p_pAlertHeader, int p_nHeaderLen, const uint8_t* p_pData, int p_nDataLen);
public:
	CBackBoneCfg	m_cfg ;
	LowPANTunnel	m_obTunnel;


private:
	void generateAlert();

	CSimpleTimer timer1s;
};

extern CBackBoneApp* g_pApp ;

#endif	// _BACKBONE_APP_H_
