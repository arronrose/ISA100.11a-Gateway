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

#ifndef _MISC_IMPL_H_
#define _MISC_IMPL_H_

#include <stdint.h>
#include <vector>

#include "../../../Shared/IniParser.h" /* MAX_LINE_LEN */

static const int MultiplyDeBruijnBitPosition[32] = 	 
{
  0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 	 
  31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9 	 
};

//////////////////////////////////////////////////////////////////////
/// @class CValidator
//////////////////////////////////////////////////////////////////////

class CValidator {
 public:
         // return 0 if p_szIp is invalid 	 
         // return uint32_t of p_szIp 	 
         uint32_t Ip(const char* p_szIp, uint32_t p_uiMask); 	 
         uint32_t Mask( const char *p_szMask ); 	 
         uint32_t Gateway( const char *p_szGw, uint32_t mask, uint32_t ip32 ); 	 
};

//////////////////////////////////////////////////////////////////////
/// @class CMiscImpl
//////////////////////////////////////////////////////////////////////

class CMiscImpl
{
	int m_nFd;
public:
	struct StatusSummary
	{
		uint16_t totalDevices;
		uint16_t onlineDevices;
		uint16_t offlineDevices;
		uint32_t alerts;
		uint16_t devicesNotCommissioned;
		time_t lastAlertReportTimestamp;
		time_t lastAuditReportTimestamp;
		uint8_t memoryUsagePercent ;
		const char*  batteryCharger ;
	};
	struct GatewayNetworkInfo
	{
		char	ip[MAX_LINE_LEN];
		char	mask[MAX_LINE_LEN];
		char	defgw[MAX_LINE_LEN];
		bool	bUpdateMAC;
		char	mac0[MAX_LINE_LEN];
		char	mac1[MAX_LINE_LEN];
		bool	bDhcpEnabled;
		char*	nameservers[256];
	};

	struct GprsProviderInfo
	{
		char	m_szMCC[16];
		char	m_szMNC[16];
		char	m_szProviderTag	[MAX_LINE_LEN];
		char	m_szCountry	[MAX_LINE_LEN];
		char	m_szAPN[MAX_LINE_LEN];
		char	m_szDialNo[MAX_LINE_LEN];
		char	m_szUser[MAX_LINE_LEN];
		char	m_szPass[MAX_LINE_LEN];
	};

public:
	char*getVersion(void);
	bool fileUpload(const char *p_szCmd, const char*p_kszCmdParam, char *& p_szCmdRsp) ;
	bool fileDownload(const char *p_szCmd, const char*p_kszCmdParam, char *& p_szCmdRsp) ;
	bool unpackTgz(const char *p_szScript, std::vector<char*> & p_vOut);

	bool getGatewayNetworkInfo( GatewayNetworkInfo& p_rGatewayNetworkInfo );
	bool setGatewayNetworkInfo( GatewayNetworkInfo& p_rGatewayNetworkInfo, int*status );
	// return the ntp servers
	bool getNtpServers( std::vector<char*>& servers ) ;
	// set an array of ntp servers
	bool setNtpServers( const std::vector<const char*>& servers ) ;

	bool restartApp( const char * p_szAppName, const char * p_szAppParams );
    bool softwareReset( );
	bool hardReset( );
	bool applyConfigChanges( const char * p_szModule ) ;
	bool execCmd(const char *p_szCmd, char *& p_szCmdRsp);

private:
	bool lockUpgradeFile(void);
	bool unlockUpgradeFile(void);

} ;
#endif	/* _MISC_IMPL_H_ */
