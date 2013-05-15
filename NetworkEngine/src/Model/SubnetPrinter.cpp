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

/*
 * SubnetPrinter.cpp
 *
 *  Created on: Sep 29, 2009
 *      Author: mulderul(catalin.pop)
 */

#include "SubnetPrinter.h"
#include "DevicePrinter.h"
#include "Common/NEAddress.h"
#include <iostream>
#include <iomanip>

namespace NE {

namespace Model {

std::ostream& operator<<(std::ostream& stream, const SubnetShortPrinter& printer){
    using namespace NE::Common;
	const Subnet::PTR& subnet = printer.subnet;
	const NE::Common::SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    const Address16Set & activeDevices = subnet->getActiveDevices();

	stream << "Subnet " <<  std::setw(5) << (int)subnet->getSubnetId(); //<< " Nr. devices: " << activeDevices.size();

	time_t brRawtime = 0;
	int totalDevicesCount = 0;
	int confirmedDevicesCount = 0;
	int nrOfIN_OUT = 0;
	int nrOfROUTER = 0;
	int nrOfROUTER_IO = 0;

	for (Address16Set::iterator it = activeDevices.begin(); it != activeDevices.end(); ++it) {

	    Device * device = subnet->getDevice(*it);
	    if (device == NULL) {
	        continue;
	    }

        switch (device->capabilities.deviceType) {
            case DeviceType::ROUTER_IO :
                ++nrOfROUTER_IO;
                break;
            case DeviceType::ROUTER :
                ++nrOfROUTER;
                break;
            case DeviceType::IN_OUT :
                ++nrOfIN_OUT;
                break;
            default:
                break;
        }

        ++totalDevicesCount;
        if (device->statusForReports == StatusForReports::JOINED_AND_CONFIGURED) {
            ++confirmedDevicesCount;
        }

        Uint8 childsRouters = 0;
        Uint8 childsIO = 0;
        subnet->getCountDirectChildsAndRouters(device, childsRouters, childsIO);
        int level = subnet->getDeviceLevel(device->address32);
        float outboundMan = 0;

        Device * parent = subnet->getDevice(device->parent32);
        if (parent != NULL) {
            outboundMan = parent->getAllocatedOutboundLink(Address::getAddress16(device->address32), Tdma::TdmaLinkTypes::MANAGEMENT);
        }
        stream << std::endl << DeviceShortPrinter(device, childsRouters, childsIO, level, outboundMan, subnetSettings.getSlotsPerSec());
        if (device->capabilities.isBackbone()) {
            brRawtime = device->startTime;
        }
	}

	int ocupied = 0;
	for (int i = 0 ; i < AppSlotsLength; ++i) {
	    if (subnet->appSlots[i] != 0) {
	        ++ocupied;
	    }
	}
	char brTime [40];
	memset(brTime, ' ', 40);
	struct tm * timeinfo;
	timeinfo = gmtime( &brRawtime );
	strftime (brTime,40,"%H:%M:%S.",timeinfo);

	time_t smRawtime = subnet->getManager()->startTime;
	char smTime [40];
	timeinfo = gmtime( &smRawtime );
	strftime (smTime,40,"%H:%M:%S.",timeinfo);

	int diff = difftime (time(NULL), brRawtime);
	int diffS = (diff % 60);
	int diffM = (diff / 60) % 60;
	int diffH = (diff / 3600);

	stream << std::endl << "Nr. devices: " << totalDevicesCount << ", Confirmed: " << confirmedDevicesCount
        << ", Nr. ROUTER-IO: " << nrOfROUTER_IO << ", Nr. ROUTER: " << nrOfROUTER  << ", Nr. IN_OUT: " << nrOfIN_OUT;

	stream << std::endl << "AppOcup: " << std:: dec << ocupied
	            << " SM start:" << smTime
	            << ", BR start:" << brTime
	            << ", FromBR: " << diffH << ":" << diffM << ":" << diffS;

	return stream;
}

std::ostream& operator<<(std::ostream& stream, const SubnetDetailPrinter& printer){
	const Subnet::PTR& subnet = printer.subnet;
	const NE::Common::SubnetSettings& subnetSettings = subnet->getSubnetSettings();

	stream << "Subnet " << (int)subnet->getSubnetId();
	stream << std::endl << "------------------";
	stream << std::endl << DeviceDetailedPrinter(subnet->getManager(), subnetSettings.getSlotsPerSec());

	for (Address16 i = 2; i < MAX_NUMBER_OF_DEVICES; ++i){
		Device * device = subnet->getDevice(i);
		if (device != NULL){
		    stream << std::endl << "------------------";
			stream << std::endl << DeviceDetailedPrinter(device, subnetSettings.getSlotsPerSec());
		}
	}

	return stream;
}

std::ostream& operator<<(std::ostream& stream, const LevelSubnetDetailPrinter& printer) {
    const Subnet::PTR& subnet = printer.subnet;
    const NE::Common::SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    stream << "Subnet " << (int)subnet->getSubnetId();
    stream << std::endl << "------------------";
    stream << std::endl << LevelDeviceDetailedPrinter(subnet->getManager(), printer.level, subnetSettings.getSlotsPerSec());

    for (Address16 i = 2; i < MAX_NUMBER_OF_DEVICES; ++i){
        Device * device = subnet->getDevice(i);
        if (device != NULL){
            stream << std::endl << "------------------";
            stream << std::endl << LevelDeviceDetailedPrinter(device, printer.level, subnetSettings.getSlotsPerSec());
        }
    }

    return stream;
}

}
}
