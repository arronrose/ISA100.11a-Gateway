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

////////////////////////////////////////////////////////////////////////////////
/// @file YDevList.cpp
/// @author 
/// @brief Device tag/Device address mapping, used by YGSAP - implementation
////////////////////////////////////////////////////////////////////////////////

//#include <arpa/inet.h>	// ntoh
//#include "../../Shared/Utils.h"
//#include "../../Shared/MicroSec.h"
#include "YDevList.h"
#include "GwApp.h"	// g_stApp
////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Re-create the association map
/// @param p_unDevListReportSize Size of device list report
/// @param p_pDevListReport Device list report
/// @remarks Erase old associations and re-create the list
/// TODO Possible optimisation: parse the list:
///		erase only devices unjoining (previously in the list, currently not)
/// 	add new devices (not previously in the list)
///		(optional) update devices already in the list, only if the information chaged (not likely)
////////////////////////////////////////////////////////////////////////////////
void CYDevList::Refresh( uint32_t p_unDevListReportSize, const SAPStruct::DeviceListRsp* p_pDevListReport )
{
	const SAPStruct::DeviceListRsp::Device * pD = (const SAPStruct::DeviceListRsp::Device*) &p_pDevListReport->deviceList[0];
	m_mapDeviceList.clear();
	if(!p_pDevListReport->VALID( p_unDevListReportSize ))
	{	LOG("WARNING CYDevList::Refresh: Invalid device list report, do not use");
		return;
	}

	for( int i = 0; i < ntohs(p_pDevListReport->numberOfDevices); ++i )
	{
		if( pD->joinStatus < 20 /*JOINED_AND_CONFIGURED*/ )
		{
			pD = pD->NEXT();
			continue;
		}
		YGSAP_IN::DeviceTag::Ptr pDevTag( new YGSAP_IN::DeviceTag( pD->getDeviceTagPTR()->size, pD->getDeviceTagPTR()->data) );
		TDeviceListMap::iterator it = m_mapDeviceList.find( pDevTag );
		if( it == m_mapDeviceList.end() )
		{	//LOG("DEBUG CYDevList Add [%16s] -> %s", pDevTag->m_szDeviceTag, GetHex(pD->networkAddress, 16) );
			YGSAP_IN::DeviceAddr::Ptr pDevAddr( new YGSAP_IN::DeviceAddr( pD->networkAddress ) );
			m_mapDeviceList.insert( std::make_pair(pDevTag, pDevAddr) );
		}
		else
		{	LOG("WARNING CYDevList Duplicate tag [%16s] -> %s ignored", it->first->m_szDeviceTag, GetHex( pD->networkAddress, 16 ) );
		}
		pD = pD->NEXT();
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Return device address function of device tag
/// @return device address if found
/// @retval NULL if not found
////////////////////////////////////////////////////////////////////////////////
uint8_t * CYDevList::Find( uint8_t p_uchDeviceTagSize, const uint8_t * p_aucDeviceTag )
{	/// convert to sz from auc
	YGSAP_IN::DeviceTag::Ptr pDevTag( new YGSAP_IN::DeviceTag( p_uchDeviceTagSize, p_aucDeviceTag ) );
	return Find( pDevTag );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Return device address function of device tag
/// @return device address if found
/// @retval NULL if not found
////////////////////////////////////////////////////////////////////////////////
uint8_t * CYDevList::Find( YGSAP_IN::DeviceTag::Ptr p_pDeviceTag )
{
	TDeviceListMap::iterator it = m_mapDeviceList.find( p_pDeviceTag );
	return ( it == m_mapDeviceList.end() ) ? NULL : it->second->m_aucAddr;
}


void CYDevList::Dump( void )
{
	LOG("YGSAP: Device Tag -> Device Addr association table (%u tags):", m_mapDeviceList.size() );
	for( TDeviceListMap::const_iterator it = m_mapDeviceList.begin(); it != m_mapDeviceList.end(); ++it )
	{
		LOG("\t[%16s] -> %s ",it->first->m_szDeviceTag, GetHex(it->second->m_aucAddr, 16) );
	}

}

