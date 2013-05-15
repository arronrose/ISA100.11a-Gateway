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

#ifndef FIRMWARE_H_
#define FIRMWARE_H_

#include "../model/MAC.h"
#include "../model/VersionFW.h"

#include <boost/format.hpp>

namespace nisa100 {
namespace hostapp {

class GetFirmwareVersion
{
public:
	int DeviceID;
	MAC DeviceAddress;
	VersionFW FirmwareVersion;
	boost::uint8_t OperationStatus;
public:
	const std::string ToString() const
	{
		return boost::str(boost::format(
				"GetFirmwareVersion[ DeviceID=%1%, OpStatus=%2% ]") % DeviceID % (int)OperationStatus);
	}
};

class FirmwareUpdate
{
public:
	int DeviceID;
	MAC DeviceAddress;
	std::string FirmwareFileName;
	boost::uint8_t OperationStatus;
public:
	const std::string ToString() const
	{
		return boost::str(boost::format(
				"FirmwareUpdate[ DeviceID=%1%, OpStatus=%2% ]") % DeviceID % (int)OperationStatus);
	}
};

class GetFirmwareUpdateStatus
{
public:
	int DeviceID;
	MAC DeviceAddress;
	boost::uint8_t CurrentPhase;
	boost::uint8_t PercentDone;
	boost::uint8_t OperationStatus;
public:
	const std::string ToString() const
	{
		return boost::str(boost::format(
				"GetFirmwareUpdateStatus[ DeviceID=%1%, CurrentPhase=%2%, PercentDone=%3%, OpStatus=%4% ]")
				% DeviceID % (int)CurrentPhase % (int)PercentDone % (int)OperationStatus);
	}
};

class CancelFirmwareUpdate
{
public:
	int DeviceID;
	MAC DeviceAddress;
	boost::uint8_t OperationStatus;
public:
	const std::string ToString() const
	{
		return boost::str(boost::format(
				"CancelFirmwareUpdate[ DeviceID=%1%, OpStatus=%2% ]") % DeviceID % (int)OperationStatus);
	}
};

} //namespace hostapp
} //namespace nisa100
#endif /*FIRMWARE_H_*/
