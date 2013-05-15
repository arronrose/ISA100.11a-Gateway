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

#ifndef _ISACONFIG_H_
#define _ISACONFIG_H_

#include "../../Shared/Config.h"
#include "../ISA100/typedef.h"


/// the size of text variables read from the config.ini file
#define READ_BUF_SIZE 256

/// Configuration class for the gateway
///
/// It may be useful in the future to unify it with the configuration class for the backbone router,
/// and for that reason also includes functionality pertaining to the backbone router
class CISAConfig : CConfig
{
public:
	CISAConfig();
	virtual ~CISAConfig();
public:
	/// Initialisation of the object: read the config.ini file
	int Init();

	/// Re-read here variables which may change at run-time
	/// Will call this on USR2
	bool Reload( void );
	
	const uint8* getSM_IPv6() const { return  m_oSMIPv6.m_pu8RawAddress; }
	uint16 getSM_Port()		{ return m_nSMPort; }	///< Getter for UDP SM port

	const uint8* getGW_IPv6() const { return  m_oGWIPv6.m_pu8RawAddress; }
	uint16 getGW_UDPPort() 	{ return m_nGW_UDPPort; } ///< Getter for UDP GW port

	uint16 getGSAP_TCPPort()	{ return m_nGSAP_TCPPort;  } ///< Getter for TCP GW port
	uint16 getGSAP_TCPPort_SSL()	{ return m_nGSAP_TCPPort_SSL;  } ///< Getter for Ssl TCP GW port

	uint16 getYGSAP_TCPPort()	{ return m_nYGSAP_TCPPort; } ///< Getter for TCP GW port
	uint16 getYGSAP_TCPPort_SSL()	{ return m_nYGSAP_TCPPort_SSL; } ///< Getter for TCP GW port

	/// Accessor function for the Ping timeout
	int	getPingTimeout() const	{ return m_nPingTimeout; }
	/// Accessor function for the IP address/TCP port of the host application
	const net_address& getHostAddr() const { return m_HostAddr; }
	LOGLVL getStackLogLevel( void ) const { return (LOGLVL)m_nLogLevelStack; };
	
	/// true if log level allow DBG logging
	bool AppLogAllowDBG( void ) const { return (LOGLVL)m_nLogLevelApp >= LOGLVL_DBG; };
	/// true if log level allow INF logging
	bool AppLogAllowINF( void ) const { return (LOGLVL)m_nLogLevelApp >= LOGLVL_INF; };


	bool AppLogLevelEnabled( LOGLVL p_nLogLevel ) const { return (LOGLVL)m_nLogLevelApp <= p_nLogLevel; };
private:
	TIPv6Address	m_oSMIPv6;	///< System Manager IPv6 Address
	int				m_nSMPort ;		///< SM UDP Port

	TIPv6Address	m_oGWIPv6;	///< Gateway IPv6 Address
	int				m_nGW_UDPPort ;		///< GW UDP Port

	/// the IP address/TCP port of the host application, if we are the TCP client; zero if we are the server
	struct net_address	m_HostAddr;

	int			m_nGSAP_TCPPort;	///< TCP port used by the GateWay / Client. Use int just because there is no cfg getter for uint16
	int			m_nGSAP_TCPPort_SSL;	///< TCP ssl port used by the GateWay / Client. Use int just because there is no cfg getter for uint16

	int			m_nYGSAP_TCPPort;	///< TCP port used by the GateWay to expose YGSAP. Use int just because there is no cfg getter for uint16
	int			m_nYGSAP_TCPPort_SSL;	///< TCP ssl port used by the GateWay to expose YGSAP. Use int just because there is no cfg getter for uint16
	
	/// the Ping timeout
	int		m_nPingTimeout;
	int 	m_nLogLevelApp;		///< APP log level
	int 	m_nLogLevelStack;	///< Stak log level

public:
	uint8_t	 m_pu8GWEUI64[8];	
	int	m_nSubnetID;
	int		m_nCrtUTCAdj;	///< Current UTC Adjustment
	
	uint8_t	m_u8AppJoinKey[16];

	uint8_t m_u8SecurityManager[8];
	int 	m_nTCPInactivity;	///< Number of seconds of inactivity until SAP is closed
	int		m_nTCPMaxSize;		/// Max accepted message payload on a GSAP
	
	int		m_nSystemReportsCacheTimeout;	///< Number of seconds to consider any System Report as valid and return from cache against asking the SM
	int		m_nClientServerCacheTimeout;	///< Number of seconds to consider any Cient/server response as valid and return from cache against asking the device
	int		m_nDeviceListRefresh;	///< Number of seconds between auto-requests for device list needed for YGSAP
	int 	m_nContractListRefresh;	///< Number of seconds between auto-requests for contract list needed for GPDU* fields in Network Health

	int		m_nDisplayReports;	///<indicator to display relevant system reports: device list and network health even in log level INF
			
	int m_nMaxSapInDataLog;		///< max raw bytes to log on SAP_IN
	int m_nMaxSapOutDataLog;	///< max raw bytes to log on SAP_OUT
	int m_nMaxPublishDataLog;	///< max raw bytes to log on PUBLISH
	int m_nMaxCSDataLog;		///< max raw bytes to log on CS

	char m_szGWTag[17];	// 16 + terminator

	char m_szSslServerCertif[READ_BUF_SIZE];
	char m_szSslServerKey[READ_BUF_SIZE];
	char m_szSslCaCertif[READ_BUF_SIZE];

	int m_nGPDUStatisticsPeriod;	/// interval used to average GS_GPDU_Path_Reliability/GS_GPDU_Latency

	int m_nTldeHListTimeWindow;
	int m_nTldeHListSize;
};

#endif // _ISACONFIG_H_

