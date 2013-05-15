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

#include "BackBoneCfg.h"
#include <ctype.h>
#include <netinet/in.h>
#include <termios.h>

#include "../Shared/Utils.h"

CBackBoneCfg::CBackBoneCfg()
{
	//memset( this, 0, sizeof(*this) ); // not virtual functions present so we can do in that way ...
}

CBackBoneCfg::~CBackBoneCfg()
{
	;
}


#define READ_BUF_SIZE 256

int CBackBoneCfg::Init()
{
	/// @todo check for potential conflicts between  this CIniParser::Load
	/// and the CIniParser::Load which occurs in CConfig::Init
	if(!CConfig::Init("BACKBONE", INI_FILE))
	{
		return 0;
	}


	if (! GetVar( "TTY_DEV", m_TtyDev, READ_BUF_SIZE ))
	{
		return 0 ;
	}
	//---------
	//---------
	if (!CConfig::GetVarAsBaudRate("TTY_BAUDS", &m_TtyBauds ))
	{

		return 0 ;
	}

	memset (m_szBBRTag, 0, sizeof(m_szBBRTag));

	//rv = CIniParser::GetVar("BACKBONES", "RAW_LOG", m_useRawLog, READ_BUF_SIZE );
	READ_DEFAULT_VARIABLE_YES_NO( "RAW_LOG", m_useRawLog, "NO" );

	if (!GetVar("BACKBONE_TAG", m_szBBRTag, sizeof(m_szBBRTag)))
	{
		memset(m_szBBRTag, 0xff, sizeof(m_szBBRTag));
		m_szBBRTag[sizeof(m_szBBRTag) - 1] = 0;
	}
	else
	{
		if (strcasecmp(m_szBBRTag, "Backbone_TAG_disabled") == 0)
		{
			memset(m_szBBRTag, 0xff, sizeof(m_szBBRTag));
			m_szBBRTag[sizeof(m_szBBRTag) - 1] = 0;
		}		
	}
	//READ_DEFAULT_VARIABLE_STRING("BACKBONE_TAG", m_szBBRTag, "Nivis Backbone" )

	char szIPv4[16];
	if(!GetVar("BACKBONE_IPv6", &m_oBBIPv6 ))
	{
		return 0;
	}

	if(!GetVar("BACKBONE_IPv4", szIPv4, sizeof(szIPv4)))
	{
		return 0;
	}

	if(!GetVar("BACKBONE_Port", &m_nBBPort))
	{
		return 0;
	}

	if (!AdjustIPv6(&m_oBBIPv6,szIPv4, m_nBBPort))
	{
		return 0;
	}

	LOG("BB IPv6=%s IPv4=%s Port=%d", m_oBBIPv6.GetPrintIPv6(), szIPv4, m_nBBPort);


	if(!CIniParser::GetVar("SYSTEM_MANAGER", "SYSTEM_MANAGER_IPv6", &m_oSMIPv6 ))
	{
		return 0;
	}

	if(!CIniParser::GetVar("SYSTEM_MANAGER", "SYSTEM_MANAGER_IPv4", szIPv4, sizeof(szIPv4)))
	{
		return 0;
	}

	if(!CIniParser::GetVar("SYSTEM_MANAGER", "SYSTEM_MANAGER_Port", &m_nSMPort))
	{
		return 0;
	}
	if(!CIniParser::GetVar("SYSTEM_MANAGER", "CURRENT_UTC_ADJUSTMENT", &m_nCrtUTCAdj))
	{	return 0;
	}

	if (!AdjustIPv6(&m_oSMIPv6,szIPv4, m_nSMPort))
	{
		return 0;
	}

	LOG("SM IPv6=%s IPv4=%s Port=%d", m_oSMIPv6.GetPrintIPv6(), szIPv4, m_nSMPort);

	//read provisioning information

	if(!CIniParser::GetVar("SYSTEM_MANAGER", "SECURITY_MANAGER_EUI64", m_u8SecurityManager, sizeof(m_u8SecurityManager)))
	{
		return 0;
	}

	if (!CIniParser::FindGroup("BACKBONE"))
	{	return 0;
	}

	if(!GetVar("BACKBONE_EUI64", (uint8_t*)m_pu8BbrEUI64, sizeof(m_pu8BbrEUI64)))
	{
		return 0;
	}

	if(!GetVar("ProvisionFormatVersion", (uint8_t*)&m_u16ProvisionFormatVersion, sizeof(m_u16ProvisionFormatVersion)))
	{
		return 0;
	}

	//if(!GetVar("SubnetID", (uint8_t*)&m_u16SubnetID, sizeof(m_u16SubnetID)))
	//{
	//	return 0;
	//}

	if(!GetVar("FilterBitMask", (uint8_t*)&m_u16FilterBitMask, sizeof(m_u16FilterBitMask)))
	{
		return 0;
	}

	if(!GetVar("FilterTargetID", (uint8_t*)&m_u16FilterTargetID, sizeof(m_u16FilterTargetID)))
	{
		return 0;
	}

	if(!GetVar("AppJoinKey", m_u8AppJoinKey, sizeof(m_u8AppJoinKey)))
	{
		return 0;
	}
	//if(!GetVar("DllJoinKey", m_u8DllJoinKey, sizeof(m_u8DllJoinKey)))
	//{
	//	return 0;
	//}
	//if(!GetVar("ProvisionKey", m_u8ProvisionKey, sizeof(m_u8ProvisionKey)))
	//{
	//	return 0;
	//}
	READ_DEFAULT_VARIABLE_INT("TR_MAX_INACTIVITY",m_nTrMaxInactivity, 30);
	READ_DEFAULT_VARIABLE_INT("TR_POWER_ID",m_nTrPowerID, 1);
	READ_DEFAULT_VARIABLE_INT("RUN_TR_TEST", runTRTest, 0);
	READ_DEFAULT_VARIABLE_INT("RUN_TR_TEST_INTERVAL", m_nTrTestInterval, 1000);

	READ_DEFAULT_VARIABLE_INT("MAX_6LOWPAN_NET_PAYLOAD",m_nMax6lowPanNetPayload, 98);
	m_n6lowPanNetFragSize = m_nMax6lowPanNetPayload / 8 * 8;
	
	if(!GetVar("CCA_Limit", &m_u8CCA_Limit, sizeof(m_u8CCA_Limit)))
	{
		m_u8CCA_Limit = 0;
	}

	Reload();

	LOG("Config loaded");
	//--------------
	return 1;
}

int CBackBoneCfg::Reload()
{
	if (!CIniParser::FindGroup("BACKBONE"))
	{	return 0;
	}

	READ_DEFAULT_VARIABLE_INT("LOG_LEVEL_STACK", m_nLogLevelStack, 3 );
	if( m_nLogLevelStack < 1 ) m_nLogLevelStack = 1;
	if( m_nLogLevelStack > 3 ) m_nLogLevelStack = 3;

	READ_DEFAULT_VARIABLE_INT ("TLDE_HLIST_SIZE",  m_nTldeHListSize, 100);
	READ_DEFAULT_VARIABLE_INT ("TLDE_HLIST_TIME_WINDOW",  m_nTldeHListTimeWindow, 60);

	return 1;
}
