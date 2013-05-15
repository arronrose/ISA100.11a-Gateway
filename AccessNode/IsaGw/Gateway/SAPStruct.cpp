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
/// @file SAPStruct.cpp
/// @author Marcel Ionescu
/// @brief SAP structures: GSAP and YGSAP
////////////////////////////////////////////////////////////////////////////////
#include "../../Shared/Common.h"

#include "SAPStruct.h"

////////////////////////////////////////////////////////////////////////////////
/// DeviceListRsp (DEVICE_LIST response)
////////////////////////////////////////////////////////////////////////////////
bool SAPStruct::DeviceListRsp::Device::VALID( int p_nPayloadSize ) const
{	int deviceSize = sizeof(Device);
	if(	(p_nPayloadSize <  deviceSize ) )
	{	LOG("ERROR DeviceListRsp::Device::VALID(%d) < %u", p_nPayloadSize, deviceSize);
		return false;	// Avoid GetHex( negative )
	}
	if(	(p_nPayloadSize < (deviceSize += getManufacturerPTR()->SIZE()) )
	||	(p_nPayloadSize < (deviceSize += getModelPTR()->SIZE()) )
	||	(p_nPayloadSize < (deviceSize += getRevisionPTR()->SIZE()) )
	||	(p_nPayloadSize < (deviceSize += getDeviceTagPTR()->SIZE()) )
	||	(p_nPayloadSize < (deviceSize += getSerialNoPTR()->SIZE()) ) )
	{	LOG("ERROR DeviceListRsp::Device::VALID(%d) %u %s", p_nPayloadSize, deviceSize, GetHex( this, _Min(p_nPayloadSize, 1000)) );
		return false;
	}
	return true;
}

bool SAPStruct::DeviceListRsp::VALID( int p_nPayloadSize ) const
{	const Device* pD = (const Device* )&deviceList[0];
	if( (p_nPayloadSize < (int)sizeof(SAPStruct::DeviceListRsp)) )
	{	LOG("ERROR DeviceListRsp::VALID(%d) < %u", p_nPayloadSize, sizeof(SAPStruct::DeviceListRsp));
		return false;
	}
	for( int i = 0; i < ntohs(numberOfDevices); ++i )
	{	if( !pD->VALID( p_nPayloadSize - ( (byte*)pD - (byte*)this )))
		{	LOG("ERROR DeviceListRsp::VALID(%d) idx %u nr %u %s", p_nPayloadSize, i, ntohs(numberOfDevices), GetHex(this, _Min(p_nPayloadSize, 1000)));
			return false;
		}
		pD = pD->NEXT();
	}
	return true;
}

size_t SAPStruct::DeviceListRsp::SIZE( void ) const
{	const Device* pD = (const Device*)&deviceList[0];
	for( int i = 0; i < ntohs(numberOfDevices); ++i )
	{	pD = pD->NEXT();
	}
	return (byte*)pD - (byte*)this;
}
////////////////////////////////////////////////////////////////////////////////
/// TopologyReportRsp (TOPOLOGY_REPORT response)
////////////////////////////////////////////////////////////////////////////////
bool SAPStruct::TopologyReportRsp::Device::Graph::VALID( int p_nPayloadSize ) const
{	if(p_nPayloadSize < (int)sizeof(Graph))
	{	LOG("ERROR TopologyReportRsp::Device::Graph::VALID(%d) < %u", p_nPayloadSize, sizeof(Graph));
		return false;	// Avoid GetHex( negative )
	}
	if(p_nPayloadSize < (int)SIZE())
	{	LOG("ERROR TopologyReportRsp::Device::Graph::VALID(%d) %u %s", p_nPayloadSize,  sizeof(Graph), GetHex(this, _Min(p_nPayloadSize, 1000)));
		return false;
	}
	return true;
}

bool SAPStruct::TopologyReportRsp::Device::VALID( int p_nPayloadSize ) const
{	if( p_nPayloadSize < (int)sizeof(Device) )
	{	LOG("ERROR v::Device::VALID(%d) < %u", p_nPayloadSize,  sizeof(Device));
		return false;	// Avoid GetHex( negative )
	}
	const Graph * pG = GET_Graph_PTR();
	for( int i = 0; i < ntohs(numberOfGraphs); ++i )
	{	if( !pG->VALID( p_nPayloadSize - ( (byte*)pG - (byte*)this) ) )/// substract from payload data already parsed
		{	LOG("ERROR TopologyReportRsp::Device::VALID(%d) idx %u nrGraphs %u %s", p_nPayloadSize, i, ntohs(numberOfGraphs), GetHex(this, _Min(p_nPayloadSize, 1000)));
			return false;
		}
		pG = pG->NEXT();
	}
	return true;
}

size_t SAPStruct::TopologyReportRsp::Device::SIZE( void ) const
{
	const Graph * pG = GET_Graph_PTR();
	for( int i = 0; i < ntohs(numberOfGraphs); ++i )
	{ 	pG = pG->NEXT();
	}
	return (byte*)pG - (byte*)this;
}

bool SAPStruct::TopologyReportRsp::VALID( int p_nPayloadSize ) const
{	if( p_nPayloadSize < (int)sizeof(TopologyReportRsp) )
	{	LOG("ERROR TopologyReportRsp::VALID(%d) %u", p_nPayloadSize,  sizeof(TopologyReportRsp));
		return false;	// Avoid GetHex( negative )
	}
	const Device * pD = &deviceList[0];
	for( int i = 0; i < ntohs(numberOfDevices); ++i )
	{	if( !pD->VALID( p_nPayloadSize - ( (byte*)pD - (byte*)this) ) )/// substract from payload data already parsed
		{	LOG("ERROR TopologyReportRsp::VALID(%d) idx %u nrDev %u %s", p_nPayloadSize, i, ntohs(numberOfDevices), GetHex(this, _Min(p_nPayloadSize, 1000)));
			return false;
		}
		pD = pD->NEXT();
	}
	if( p_nPayloadSize < (ntohs(numberOfBackbones) * sizeof(Backbone) + (byte*)pD - (byte*)this) )
	{	LOG("ERROR TopologyReportRsp::VALID(%d) nrDev %u nrBBR %u off %u %s", p_nPayloadSize,
			ntohs(numberOfDevices), ntohs(numberOfBackbones), (byte*)pD - (byte*)this, GetHex(this, _Min(p_nPayloadSize, 1000)));
		return false;
	}
	return true;
}

size_t SAPStruct::TopologyReportRsp::SIZE( void ) const
{	const Device * pD = &deviceList[0];
	for( int i = 0; i < ntohs(numberOfDevices); ++i )
	{	pD = pD->NEXT();
	}
	return ntohs(numberOfBackbones) * sizeof(Backbone) + (byte*)pD - (byte*)this;
}

////////////////////////////////////////////////////////////////////////////////
/// ScheduleReportRsp (SCHEDULE response)
////////////////////////////////////////////////////////////////////////////////
bool SAPStruct::ScheduleReportRsp::DeviceSchedule::Superframe::VALID( int p_nPayloadSize ) const
{	if( p_nPayloadSize < (int)sizeof(Superframe) )
	{	LOG("ERROR ScheduleReportRsp::DeviceSchedule::Superframe::VALID(%d) < %u", p_nPayloadSize,  sizeof(Superframe) );
		return false;	// Avoid GetHex( negative )
	}
	if( p_nPayloadSize < (int)SIZE())
	{	LOG("ERROR ScheduleReportRsp::DeviceSchedule::Superframe::VALID(%d) %u %s", p_nPayloadSize,  sizeof(Superframe), GetHex(this, _Min(p_nPayloadSize, 1000)));
		return false;
	}
	return true;
}

bool SAPStruct::ScheduleReportRsp::DeviceSchedule::VALID( int p_nPayloadSize ) const
{	const Superframe * pS = (const Superframe *) &superframeList[0];
	if( p_nPayloadSize < (int)sizeof(DeviceSchedule) )
	{	LOG("ERROR ScheduleReportRsp::DeviceSchedule::VALID(%d) < %u", p_nPayloadSize,  sizeof(DeviceSchedule));
		return false;	// Avoid GetHex( negative )
	}
	for( int i = 0; i < ntohs(nrOfSuperframes); ++i )
	{	if( !pS->VALID( p_nPayloadSize - ( (byte*)pS - (byte*)this) ) )/// substract from payload data already parsed
		{	LOG("ERROR ScheduleReportRsp::DeviceSchedule::VALID(%d) idx %u nr %u %s", p_nPayloadSize, i, ntohs(nrOfSuperframes), GetHex(this, _Min(p_nPayloadSize, 1000)));
			return false;
		}
		pS = pS->NEXT();
	}
	return true;
}

size_t SAPStruct::ScheduleReportRsp::DeviceSchedule::SIZE( void ) const
{	const Superframe * pS = (const Superframe *)&superframeList[0];
	for( int i = 0; i < ntohs(nrOfSuperframes); ++i )
	{ 	pS = pS->NEXT();
	}
	return (byte*)pS - (byte*)this;
}

bool SAPStruct::ScheduleReportRsp::VALID( int p_nPayloadSize ) const
{	if(p_nPayloadSize < (int)sizeof(ScheduleReportRsp))
	{	LOG("ERROR ScheduleReportRsp::VALID(%d) < %u", p_nPayloadSize, sizeof(ScheduleReportRsp) );
		return false;	// Avoid GetHex( negative )
	}
	const DeviceSchedule * pD = GET_DeviceSchedule_PTR();
	for( int i = 0; i < ntohs(numberOfDevices); ++i )
	{
		if( !pD->VALID( p_nPayloadSize - ((byte*)pD - (byte*)this) ) )	/// substract from payload data already parsed
		{	LOG("ERROR ScheduleReportRsp::VALID(%d) idx %u nrDev %u %s", p_nPayloadSize, i, ntohs(numberOfDevices), GetHex(this, _Min(p_nPayloadSize, 1000)) );
			return false;
		}
		pD = pD->NEXT();
	}
	return true;
}

/// @note works with structure in network order
size_t SAPStruct::ScheduleReportRsp::SIZE( void ) const
{	const DeviceSchedule * pD = GET_DeviceSchedule_PTR();
	for( int i = 0; i < ntohs(numberOfDevices); ++i )
	{	pD = pD->NEXT();
	}
	return (byte*)pD - (byte*)this;
}

////////////////////////////////////////////////////////////////////////////////
/// DeviceHealthRsp (DEVICEHEALTH response)
////////////////////////////////////////////////////////////////////////////////
bool SAPStruct::DeviceHealthRsp::VALID( int p_nPayloadSize ) const
{	if(p_nPayloadSize < (int)sizeof(DeviceHealthRsp))
	{	LOG("ERROR DeviceHealthRsp::VALID(%d) < %u", p_nPayloadSize, sizeof(DeviceHealthRsp));
		return false;	// Avoid GetHex( negative )
	}
	if( p_nPayloadSize < (int)SIZE() )
	{	LOG("ERROR DeviceHealthRsp::VALID(%d) %u %s", p_nPayloadSize, sizeof(DeviceHealthRsp), GetHex(this, _Min(p_nPayloadSize, 1000)) );
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// NeighborHealthRsp (NEIGHBOR_HEALTH_REPORT response)
////////////////////////////////////////////////////////////////////////////////
bool SAPStruct::NeighborHealthRsp::VALID( int p_nPayloadSize ) const
{ 	if(p_nPayloadSize < (int)sizeof(NeighborHealthRsp))
	{	LOG("ERROR NeighborHealthRsp::VALID(%d) < %u", p_nPayloadSize, sizeof(NeighborHealthRsp) );
		return false;	// Avoid GetHex( negative )
	}
	if(p_nPayloadSize < (int)SIZE())
	{	LOG("ERROR NeighborHealthRsp::VALID(%d) %u %s", p_nPayloadSize, sizeof(NeighborHealthRsp), GetHex(this, _Min(p_nPayloadSize, 1000)) );
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// NetworkHealthReportRsp (NETWORK_HEALTH_REPORT response)
////////////////////////////////////////////////////////////////////////////////
bool SAPStruct::NetworkHealthReportRsp::NetworkHealth::VALID( int p_nPayloadSize ) const
{ if( p_nPayloadSize < (int)sizeof(NetworkHealth) )
	{	LOG("ERROR TNetworkHealth::VALID(%d) %u", p_nPayloadSize, sizeof(NetworkHealth) );
		return false;	// Avoid GetHex( negative )
	}
	return true;
}

bool SAPStruct::NetworkHealthReportRsp::VALID( int p_nPayloadSize ) const
{ 	if(p_nPayloadSize < (int)sizeof(NetworkHealthReportRsp))
	{	LOG("ERROR NetworkHealthReportRsp::VALID(%d) < %u", p_nPayloadSize, sizeof(NetworkHealthReportRsp));
		return false;	// Avoid GetHex( negative )
	}
	if(p_nPayloadSize < (int)SIZE())
	{	LOG("ERROR NetworkHealthReportRsp::VALID(%d) %u %s", p_nPayloadSize, sizeof(NetworkHealthReportRsp), GetHex(this, _Min(p_nPayloadSize, 1000)) );
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// NetworkResourceReportRsp (NETWORK_RESOURCE_REPORT response)
////////////////////////////////////////////////////////////////////////////////
bool SAPStruct::NetworkResourceReportRsp::VALID( int p_nPayloadSize ) const
{ 	if(p_nPayloadSize < (int)sizeof(NetworkResourceReportRsp))
	{	LOG("ERROR NetworkResourceReportRsp::VALID(%d) < %u", p_nPayloadSize, sizeof(NetworkResourceReportRsp));
		return false;	// Avoid GetHex( negative )
	}
	if(p_nPayloadSize < (int)SIZE())
	{	LOG("ERROR NetworkResourceReportRsp::VALID(%d) %u %s", p_nPayloadSize, sizeof(NetworkResourceReportRsp), GetHex(this, _Min(p_nPayloadSize, 1000)) );
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// EXEC SMO.getContractsAndRoutes RESPONSE
////////////////////////////////////////////////////////////////////////////////
bool SAPStruct::ContractsAndRoutes::DeviceContractsAndRoutes::Route::RouteElement::VALID( int p_nPayloadSize ) const
{	if(p_nPayloadSize < (int)sizeof(RouteElement))
	{	LOG("ERROR RouteElement::VALID(%d) < %u", p_nPayloadSize, sizeof(RouteElement));
		return false;	// Avoid GetHex( negative )
	}
	if(p_nPayloadSize < (int)SIZE() || isGraph>1)
	{	LOG("ERROR RouteElement::VALID(%d) %u %s", p_nPayloadSize, SIZE(), GetHex(this, _Min(p_nPayloadSize, 1000)) );
		return false;
	}
	return true;
}

bool SAPStruct::ContractsAndRoutes::DeviceContractsAndRoutes::Route::VALID( int p_nPayloadSize ) const
{	if(p_nPayloadSize < (int)sizeof(Route))
	{	LOG("ERROR Route::VALID(%d) < %u", p_nPayloadSize, sizeof(Route));
		return false;	// Avoid GetHex( negative )
	}

	const RouteElement *pR = &route[ 0 ];
	for( int i = 0; i < size; ++i )
	{	if( !pR->VALID( p_nPayloadSize - ((byte*)pR - (byte*)this) ) )	/// substract from payload data already parsed
		{	LOG("ERROR Route::VALID(%d) idx %u size %u %s", p_nPayloadSize, i, size, GetHex(this, _Min(p_nPayloadSize, 1000)) );
			return false;
		}
		pR = pR->NEXT();
	}
	/// use SIZE instead of recomputing selector, it is safe now
	if(p_nPayloadSize < (int)SIZE() || alternative>3)
	{	LOG("ERROR Route::VALID(%d) %u %s", p_nPayloadSize, SIZE(), GetHex(this, _Min(p_nPayloadSize, 1000)) );
		return false;
	}
	return true;
}

size_t SAPStruct::ContractsAndRoutes::DeviceContractsAndRoutes::Route::SIZE( void ) const
{	const RouteElement *pR = &route[ 0 ];
	for( int i = 0; i < size; ++i )
	{	pR = pR->NEXT();
	}
	/// now add selector
	uint8_t *offset = (uint8_t*)pR;
	switch(alternative)
	{	case 0: offset += sizeof(uint16_t) + 16; break;
		case 1: offset += sizeof(uint16_t); break;
		case 2: offset += 16; break;
		case 3: break;
		default:LOG("WARNING Route::SIZE(): parse error (index %04X size %02X alternative %02X fwdLimit %02X route[0] %02X%02X%02X", index, size, alternative, forwardLimit, route[0].isGraph, route[1].isGraph, route[2].isGraph);
	}
	return offset - (uint8_t*)this;
}

bool SAPStruct::ContractsAndRoutes::DeviceContractsAndRoutes::VALID( int p_nPayloadSize ) const
{	if(p_nPayloadSize < (int)sizeof(DeviceContractsAndRoutes))
	{	LOG("ERROR DeviceContractsAndRoutes::VALID(%d) < %u", p_nPayloadSize, sizeof(DeviceContractsAndRoutes));
		return false;	// Avoid GetHex( negative )
	}
	const Route * pR = GET_Route_PTR();
	for( int i = 0; i < numberOfRoutes; ++i )
	{	if( !pR->VALID( p_nPayloadSize - ( (byte*)pR - (byte*)this) ) )/// substract from payload data already parsed
		{	LOG("ERROR DeviceContractsAndRoutes::VALID(%d) idx %u nr %u %s", p_nPayloadSize, i, numberOfRoutes, GetHex(this, _Min(p_nPayloadSize, 1000)) );
			return false;
		}
		pR = pR->NEXT();
	}
	return true;
}

size_t SAPStruct::ContractsAndRoutes::DeviceContractsAndRoutes::SIZE( void ) const
{	const Route * pR = GET_Route_PTR();
	for( int i = 0; i < numberOfRoutes; ++i )
	{	pR = pR->NEXT();
	}
	return (uint8_t*)pR - (uint8_t*)this;
}

bool SAPStruct::ContractsAndRoutes::VALID( int p_nPayloadSize ) const
{	const DeviceContractsAndRoutes * pR = (const DeviceContractsAndRoutes*) &deviceList[0];
	if(p_nPayloadSize < (int)sizeof(ContractsAndRoutes))
	{	LOG("ERROR ContractsAndRoutes::VALID(%d) < %u", p_nPayloadSize, sizeof(ContractsAndRoutes));
		return false;	// Avoid GetHex( negative )
	}
	for( int i = 0; i < numberOfDevices; ++i )
	{	if( !pR->VALID( p_nPayloadSize - ( (byte*)pR - (byte*)this) ) )/// substract from payload data already parsed
		{	LOG("ERROR ContractsAndRoutes::VALID(%d) idx %u nr %u %s", p_nPayloadSize, i, numberOfDevices, GetHex(this, _Min(p_nPayloadSize, 1000)) );
			return false;
		}
		pR = pR->NEXT();
	}
	return true;
}
