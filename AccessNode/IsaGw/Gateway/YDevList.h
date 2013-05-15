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
/// @file YDevList.h
/// @author Marcel Ionescu
/// @brief Device tag/Device address mapping, used by YGSAP
////////////////////////////////////////////////////////////////////////////////
#ifndef YDEVLIST_H
#define YDEVLIST_H

#include <stdint.h>
#include <map>

#include "../../Shared/Common.h"
#include "SAPStruct.h"

class CYDevList{	/// the association device tag/device address used by YGSAP
	/// @brief Functor used by TDeviceListMap as comparator
	struct deviceTagCompare {	/// cannot use memcmp, the struct is not packed
		bool operator()( YGSAP_IN::DeviceTag::Ptr p_pK1, YGSAP_IN::DeviceTag::Ptr p_pK2 ) const
		{	return (strcmp((const char*)p_pK1->m_szDeviceTag, (const char*)p_pK2->m_szDeviceTag) < 0);
		};
	} ;
	typedef std::map<YGSAP_IN::DeviceTag::Ptr, YGSAP_IN::DeviceAddr::Ptr, deviceTagCompare> TDeviceListMap;

	TDeviceListMap		m_mapDeviceList;	/// Association between device tag and device address

public:
	/// Erase old associations and re-create the list
	/// possible optimisation: parse the list:
	///		erase only devices unjoining (previously in the list, currently not)
	/// 	add new devices (not previously in the list)
	///		(optional)update devices already in the list, only if the information chaged (not likely)
	void Refresh( uint32_t p_unDevListReportSize, const SAPStruct::DeviceListRsp* p_pDevListReport );

	/// Find a device address by it's device Tag. Return NULL if not found
	uint8_t * Find( uint8_t p_uchDeviceTagSize, const uint8_t * p_aucDeviceTag );

	/// Find a device address by it's device Tag. Return NULL if not found
	uint8_t * Find( YGSAP_IN::DeviceTag::Ptr p_pDeviceTag );
	
	/// dump to log the tag2addr mapping - the m_mapDeviceList content
	void Dump( void );
};

#endif	//YDEVLIST_H
