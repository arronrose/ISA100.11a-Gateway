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
 * DevicePrinter.cpp
 *
 *  Created on: Sep 29, 2009
 *      Author: mulderul
 */

#include "DevicePrinter.h"
#include <iostream>
#include "Model/ModelPrinter.h"
#include <iomanip>
namespace NE {

namespace Model {

void printCommonDevice(std::ostream& stream, Device * device, int slotsPerSec, int childsRouters = 0, int childsIO = 0, int level=0, float outManagement = 0){
    stream << "DEV:" << std::setw(14) << DeviceType::toString(device->capabilities.deviceType) << " ";
    stream << std::hex << device->address128.toString() << " " << device->address64.toString() << " " << std::setw(8) << device->address32 << " ";
    stream << "parent=" << std::setw(8) << device->parent32;
    stream << " RA=" << (int)device->hasRoleActivated;
    stream << " AC=" << device->getAcceleratedFlag();
    stream << " join=" << std::setw(2) << (int)device->joinsCount;
    stream << "/" << std::setw(2) << device->fullJoinsCount;
    stream << std::dec ;
    stream << " L" << level;
    stream << " cR=" << TYPE_FORMAT(2, childsRouters);
    stream << " cI=" << TYPE_FORMAT(2, childsIO);
    stream << " iM=" << FLOAT_FORMAT(5, 2, device->getAllocatedInboundLink(Address::getAddress16(device->parent32), Tdma::TdmaLinkTypes::MANAGEMENT));
    stream << " oM=" << FLOAT_FORMAT(5, 2, outManagement);
    stream << " iA=" << FLOAT_FORMAT(5, 2, device->getInboundAppTraffic(slotsPerSec));
    stream << " oA=" << FLOAT_FORMAT(5, 2, device->getOutboundAppTraffic(slotsPerSec));
    stream << " Tag:" << TYPE_FORMAT(16, device->capabilities.tagName);
    stream << " " << StatusForReports::toString(device->statusForReports);
}

std::ostream& operator<<(std::ostream& stream, const DeviceShortPrinter& printer){
	Device * device = printer.device;
	printCommonDevice(stream, device, printer.slotsPerSec, printer.childsRouters, printer.childsIO, printer.level, printer.outboundManagement);
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const DeviceDetailedPrinter& printer){
    Device * device = printer.device;
    printCommonDevice(stream, device, printer.slotsPerSec);
    stream << std::endl << device->phyAttributes;
    stream << std::endl << device->theoAttributes;

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const LevelDeviceDetailedPrinter& printer){
    Device * device = printer.device;

//    stream << std::endl <<  "------------------" << std::endl;
    printCommonDevice(stream, device, printer.slotsPerSec);
    time_t startRawtime = device->startTime;
    time_t confRawtime = device->joinConfirmTime;
    time_t lastRawtime = device->getLastTimeAccessed();
    char startTime [40];
    char confTime [40];
    char lastTime [40];
    strftime (startTime,40,"%H:%M:%S.", gmtime( &startRawtime ));
    strftime (confTime,40,"%H:%M:%S.", gmtime( &confRawtime ));
    strftime (lastTime,40,"%H:%M:%S.", gmtime( &lastRawtime ));
    stream << " Times: Start=" << startTime << " Conf=" << confTime << " Last=" << lastTime;
    LevelPrinterPhyAttributes levelPhyPrinter(device->phyAttributes, printer.level);
    stream << std::endl << levelPhyPrinter;

    if (printer.level.logTheoreticAttributes && (printer.level.entityType == ENTITY_TYPE_NOT_USED)) {
        stream << std::endl << device->theoAttributes;
    }
    return stream;
}

}

}
