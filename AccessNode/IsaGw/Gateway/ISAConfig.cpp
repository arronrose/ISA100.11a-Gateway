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

#include "ISAConfig.h"
#include <ctype.h>
#include <netinet/in.h>
#include <termios.h>
#include "../../Shared/Utils.h"

CISAConfig::CISAConfig()
{
}

CISAConfig::~CISAConfig()
{
}

/// Load the config.ini file and parse it to extract various configurations out of it
/// @retval 1 success
/// @retval 0 failure
/// @note DO NOT read same variable in Init() and Reload(). Init calls Reload.
int CISAConfig::Init()
{
	if(!CConfig::Init("GATEWAY"))
	{
		return 0;
	}

	READ_DEFAULT_VARIABLE_INT ("TCP_YGSAP_PORT",  m_nYGSAP_TCPPort, 0);
	READ_DEFAULT_VARIABLE_INT ("TCP_SERVER_PORT", m_nGSAP_TCPPort, 4900);

	READ_DEFAULT_VARIABLE_INT ("TCP_SERVER_PORT_SSL", m_nGSAP_TCPPort_SSL, 0);
	READ_DEFAULT_VARIABLE_INT ("TCP_YGSAP_PORT_SSL", m_nYGSAP_TCPPort_SSL, 0);
	
	READ_DEFAULT_VARIABLE_INT ("SM_LINK_TIMEOUT", m_nPingTimeout, 50);
	READ_DEFAULT_VARIABLE_STRING("GATEWAY_TAG", m_szGWTag, "Nivis Gateway" );

	if ( m_nGSAP_TCPPort_SSL != 0 ||  m_nYGSAP_TCPPort_SSL != 0)
	{
		READ_DEFAULT_VARIABLE_STRING("SSL_SERVER_CERTIF_FILE", m_szSslServerCertif, "/access_node/activity_files/ssl_resources/servercert.pem" );
		READ_DEFAULT_VARIABLE_STRING("SSL_SERVER_KEY_FILE", m_szSslServerKey, "/access_node/activity_files/ssl_resources/serverkey.pem" );
		READ_DEFAULT_VARIABLE_STRING("SSL_CA_CERTIF_FILE", m_szSslCaCertif, "/access_node/activity_files/ssl_resources/cakey.pem" );
	}
	

	if(GetVar("HOST_APP", &m_HostAddr )){
		LOG_HEX( "m_HostAddr(hexdump):", (uint8*)&m_HostAddr, sizeof(net_address));
	}

	READ_MANDATORY_VARIABLE("GATEWAY_IPv6", m_oGWIPv6 );

	char szIPv4[16];
	READ_MANDATORY_VARIABLE_STRING("GATEWAY_IPv4", szIPv4);
	READ_MANDATORY_VARIABLE("GATEWAY_UDPPort", m_nGW_UDPPort)

	if (!AdjustIPv6(&m_oGWIPv6,szIPv4, m_nGW_UDPPort))
	{
		return 0;
	}

	LOG("GW IPv6=%s IPv4=%s UDPPort=%d", m_oGWIPv6.GetPrintIPv6(), szIPv4, m_nGW_UDPPort);

	if(!GetVar("GATEWAY_EUI64", (uint8_t*)m_pu8GWEUI64, sizeof(m_pu8GWEUI64)))
	{
		return 0;
	}

	READ_MANDATORY_VARIABLE("SubnetID", m_nSubnetID);
	LOG("SubnetID %u (0x%X)", m_nSubnetID, m_nSubnetID);
	READ_MANDATORY_VARIABLE_STRING("AppJoinKey", m_u8AppJoinKey);
	
	READ_DEFAULT_VARIABLE_INT ("TLDE_HLIST_SIZE",  m_nTldeHListSize, 100);
	READ_DEFAULT_VARIABLE_INT ("TLDE_HLIST_TIME_WINDOW",  m_nTldeHListTimeWindow, 60);

	Reload();

	if(!CIniParser::GetVar("SYSTEM_MANAGER", "SYSTEM_MANAGER_IPv6", &m_oSMIPv6 ))
	{	return 0;
	}
	if(!CIniParser::GetVar("SYSTEM_MANAGER", "SYSTEM_MANAGER_IPv4", szIPv4, sizeof(szIPv4)))
	{	return 0;
	}
	if(!CIniParser::GetVar("SYSTEM_MANAGER", "SYSTEM_MANAGER_Port", &m_nSMPort))
	{	return 0;
	}
	if(!CIniParser::GetVar("SYSTEM_MANAGER", "CURRENT_UTC_ADJUSTMENT", &m_nCrtUTCAdj))
	{	return 0;
	}
	if (!AdjustIPv6(&m_oSMIPv6,szIPv4, m_nSMPort))
	{	return 0;
	}
	LOG("SM IPv6=%s IPv4=%s Port=%d", m_oSMIPv6.GetPrintIPv6(), szIPv4, m_nSMPort);

	if(!CIniParser::GetVar("SYSTEM_MANAGER", "SECURITY_MANAGER_EUI64", m_u8SecurityManager, sizeof(m_u8SecurityManager)))
	{	return 0;
	}

	LOG("Config loaded");
	//--------------
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Re-read variables which may change at run-time
/// @retval true success
/// @retval false failure
/// @remarks Call it on USR2
/// @note TAKE CARE: DO NOT read same variable in Init and Reload. Init calls Reload.
/// @note The group get set to "GATEWAY" inside the method
/// @note TAKE CARE: put here only variables used directly
/// @note Do not put here varaibles used
///		- to initialise some other variables,
///		- as parameters to constructors
///		- as parameters for I/O subsystems (serials, USB, UDP, TCP)
////////////////////////////////////////////////////////////////////////////////
bool CISAConfig::Reload( void )
{
//	LOGLVL_ERR = 1,	/// User should ALWAYS log this log level.
//	LOGLVL_INF = 2,	/// User may choose to ignore this loglevel, to reduce the log noise. It is recommended to log it, however.
//	LOGLVL_DBG = 3	/// User should NOT log this level, except for stack debugging
/// @see ../ISA100/log_callback.h

	if (!CIniParser::FindGroup("GATEWAY"))
	{    return false;
	}

	READ_DEFAULT_VARIABLE_INT("LOG_LEVEL_APP",   m_nLogLevelApp, 3 );
	if( m_nLogLevelApp < 1 ) m_nLogLevelApp = 1;
	if( m_nLogLevelApp > 3 ) m_nLogLevelApp = 3;
	
	READ_DEFAULT_VARIABLE_INT("LOG_LEVEL_STACK", m_nLogLevelStack, 3 );
	if( m_nLogLevelStack < 1 ) m_nLogLevelStack = 1;
	if( m_nLogLevelStack > 3 ) m_nLogLevelStack = 3;

	READ_DEFAULT_VARIABLE_INT("TCP_INACTIVITY_SEC",	m_nTCPInactivity, 3600);
	READ_DEFAULT_VARIABLE_INT("TCP_MAX_MSG_SIZE", 	m_nTCPMaxSize, 10*1024*1024);
	READ_DEFAULT_VARIABLE_INT("SYSTEM_REPORTS_CACHE_TIMEOUT",	m_nSystemReportsCacheTimeout, 10 );
	READ_DEFAULT_VARIABLE_INT("CLIENTSERVER_CACHE_TIMEOUT",		m_nClientServerCacheTimeout, 120 );
	READ_DEFAULT_VARIABLE_INT("YGSAP_DEVICE_LIST_REFRESH",		m_nDeviceListRefresh, 30 );
	READ_DEFAULT_VARIABLE_INT("CONTRACT_LIST_REFRESH",			m_nContractListRefresh, 300 );

	READ_DEFAULT_VARIABLE_YES_NO("DISPLAY_REPORTS",				 m_nDisplayReports, "no");

	READ_DEFAULT_VARIABLE_INT("MAX_SAP_IN_DATA_LOG", 	m_nMaxSapInDataLog, 16);
	READ_DEFAULT_VARIABLE_INT("MAX_SAP_OUT_DATA_LOG", 	m_nMaxSapOutDataLog, 24);
	READ_DEFAULT_VARIABLE_INT("MAX_PUBLISH_DATA_LOG", 	m_nMaxPublishDataLog, 16);
	READ_DEFAULT_VARIABLE_INT("MAX_CS_DATA_LOG", 		m_nMaxCSDataLog, 12);

	READ_DEFAULT_VARIABLE_INT("GPDU_STATISTICS_PERIOD", m_nGPDUStatisticsPeriod, 600);   /// interval used to average GS_GPDU_Path_Reliability/GS_GPDU_Latency
// old method	READ_DEFAULT_VARIABLE_INT("GPDU_MAX_LATENCY_PERCENT", m_nGPDUMaxLatencyPercent, 10); /// percent of publication period. Above it a publish packet is considered late and added to GS_GPDU_Latency

	return true;
}
