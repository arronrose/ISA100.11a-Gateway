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

#ifndef _BACKBONE_CONFIG_H_
#define _BACKBONE_CONFIG_H_

//////////////////////////////////////////////////////////////////////////////
/// @file	BackBoneCfg.h
/// @author	Marius Negreanu
/// @date	Wed Nov 28 12:26:59 2007 UTC
/// @brief	Backbone Configuration Interface.
//////////////////////////////////////////////////////////////////////////////

#include "Shared/Config.h"
#include "Shared/h.h"


//////////////////////////////////////////////////////////////////////////////
/// @class	CBackBoneCfg
/// @ingroup	Backbone
/// @brief	Backbone Configuration Interface.
//////////////////////////////////////////////////////////////////////////////
class CBackBoneCfg : CConfig
{
public:
	CBackBoneCfg();
	virtual ~CBackBoneCfg();
public:
	int Init();
	int Reload();
	//char* getSM_IPv4()		  { return m_SM_IPv4; }
	const uint8_t* getSM_IPv6() const { return  m_oSMIPv6.m_pu8RawAddress; }
	uint16_t getSM_Port()		  { return m_nSMPort; }


	const uint8_t* getBB_IPv6() const { return  m_oBBIPv6.m_pu8RawAddress; }
	uint16_t getBB_Port()		  { return m_nBBPort ; }

	const char	*getTtyDev()	const { return m_TtyDev ; }
	unsigned int	getTtyBauds()	const { return m_TtyBauds ; }
	bool		UseRawLog()	const { return m_useRawLog; }



public:

	TIPv6Address	m_oBBIPv6;	///< BackBone IPv6 Address
	int				m_nBBPort ;		///< BackBone UDP Port

	TIPv6Address	m_oSMIPv6;	///< BackBone IPv6 Address
	int				m_nSMPort ;		///< BackBone UDP Port
	int		m_nCrtUTCAdj;	///< Current UTC Adjustment
	int		m_TtyBauds ;		///< Serial Link Bauds
	char	m_TtyDev[256] ;		///< Serial Link Device
	bool	m_useRawLog ;		///< To use or not to use RawLog.
	int		m_nLogLevelStack;

public:
	uint8_t	 m_pu8BbrEUI64[8];
	uint16_t m_u16ProvisionFormatVersion;
	//uint16_t m_u16SubnetID;
	uint16_t m_u16FilterBitMask;
	uint16_t m_u16FilterTargetID;

	uint8_t	m_u8AppJoinKey[16];
	//uint8_t	m_u8DllJoinKey[16];
	//uint8_t	m_u8ProvisionKey[16];

	uint8_t m_u8SecurityManager[8];

	uint8_t	m_u8CCA_Limit;

	int		m_nMax6lowPanNetPayload;
	int		m_n6lowPanNetFragSize;
	char	m_szBBRTag[24];	// 16 + terminator to include Backbone_TAG_disabled

	int		m_nTrMaxInactivity;
	int		m_nTrPowerID;
	int		runTRTest ;
	int		m_nTrTestInterval;

	int		m_nTldeHListSize;
	int		m_nTldeHListTimeWindow;	
};


#endif // _BACKBONE_CONFIG_H_
