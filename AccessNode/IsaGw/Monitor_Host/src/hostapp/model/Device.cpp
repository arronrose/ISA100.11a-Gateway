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

#include "Device.h"

#include <boost/format.hpp>
#include <stdio.h>

namespace nisa100 {
namespace hostapp {

ChannelValue::ChannelValue()
{
	value.uint32 = 0;
	dataType = cdtUInt32;
}

double ChannelValue::GetDoubleValue()
{
	switch (dataType)
	{
	case cdtUInt8:
		return (double)value.uint8;
	case cdtInt8:
		return (double)value.int8;
	case cdtUInt16:
		return (double)value.uint16;
	case cdtInt16:
		return (double)value.int16;
	case cdtUInt32:
		return (double)value.uint32;
	case cdtInt32:
		return (double)value.int32;
	case cdtFloat32:
		return (double)value.float32;
	default:
		return (double)value.uint32;
	}
}

float ChannelValue::GetFloatValue()
{
	switch (dataType)
	{
	case cdtUInt8:
		return (float)value.uint8;
	case cdtInt8:
		return (float)value.int8;
	case cdtUInt16:
		return (float)value.uint16;
	case cdtInt16:
		return (float)value.int16;
	case cdtUInt32:
		return (float)value.uint32;
	case cdtInt32:
		return (float)value.int32;
	case cdtFloat32:
		return (float)value.float32;
	default:
		return (float)value.uint32;
	}
}

uint32_t ChannelValue::GetIntValue()
{
	switch (dataType)
	{
	case cdtUInt8:
		return (uint32_t)value.uint8;
	case cdtInt8:
		return (uint32_t)value.int8;
	case cdtUInt16:
		return (uint32_t)value.uint16;
	case cdtInt16:
		return (uint32_t)value.int16;
	case cdtUInt32:
		return (uint32_t)value.uint32;
	case cdtInt32:
		return (uint32_t)value.int32;
	case cdtFloat32:
		return (uint32_t) *(uint32_t*)&value.float32;
	default:
		return (uint32_t)value.uint32;
	}
}

const std::string ChannelValue::ToString() const
{
	
	static char szVal[50];
	
	switch (dataType)
	{
	case cdtUInt8:
		sprintf(szVal, "%d", value.uint8);
		return szVal;
	
	case cdtUInt16:
		sprintf(szVal, "%d", value.uint16);
		return szVal;

	case cdtUInt32:
		sprintf(szVal, "%d", (int)value.uint32);
		return szVal;

	case cdtInt8:
		sprintf(szVal, "%d", value.int8);
		return szVal;

	case cdtInt16:
		sprintf(szVal, "%d", value.int16);
		return szVal; 

	case cdtInt32:
		sprintf(szVal, "%d", (int)value.int32);
		return szVal; 

	case cdtFloat32:
		sprintf(szVal, "%f", value.float32);
		return szVal;
	}
	return "N/A";
}

const std::string DeviceChannel::ToString() const
{
	return boost::str(boost::format("DeviceChannel[Name=%1%, No=%2%, min=%3%, max=%4%, minRaw=%5%, maxRaw=%6%]")
	    % channelName % channelNumber % minValue % maxValue % minRawValue % maxValue);
}

const std::string Device::ToString() const
{
	//return boost::str(boost::format("Device[ DeviceID=%1% MAC=%2% Status=%3% IP=%4% ]") % id % mac.ToString()
	//    % (int) status % ip.ToString());
	char szVal[500];
	sprintf(szVal, "Device[ DeviceID=%d MAC=%s Status=%d IP=%s ]", id, mac.ToString().c_str(), status, ip.ToString().c_str());
	return szVal;
}

} //namespace hostapp
} //namespace nisa100
