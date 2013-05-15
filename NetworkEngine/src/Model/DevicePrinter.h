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
 * DevicePrinter.h
 *
 *  Created on: Sep 29, 2009
 *      Author: mulderul
 */

#ifndef DEVICEPRINTER_H_
#define DEVICEPRINTER_H_
#include "Model/Device.h"
#include "Model/ModelPrinter.h"
#include "Common/SubnetSettings.h"
namespace NE {

namespace Model {

struct DeviceShortPrinter {
	NE::Model::Device * device;
	int childsRouters;
	int childsIO;
	int level;
	float outboundManagement;
	int slotsPerSec;
	DeviceShortPrinter(NE::Model::Device * device_, int childsRouters_, int childsIO_, int level_, float outboundManagement_, int slotsPerSec_)
	: device(device_), childsRouters(childsRouters_), childsIO(childsIO_), level(level_), outboundManagement(outboundManagement_), slotsPerSec(slotsPerSec_){}
	virtual ~DeviceShortPrinter(){}
};

std::ostream& operator<<(std::ostream& stream, const DeviceShortPrinter& printer);

struct DeviceDetailedPrinter {
	NE::Model::Device * device;
	int slotsPerSec;
	DeviceDetailedPrinter(NE::Model::Device * device_, int slotsPerSec_) : device(device_), slotsPerSec(slotsPerSec_){}
	virtual ~DeviceDetailedPrinter(){}
};

struct LevelDeviceDetailedPrinter {
	NE::Model::Device * device;
	LogDeviceDetail& level;
	int slotsPerSec;
	LevelDeviceDetailedPrinter(NE::Model::Device * device_, LogDeviceDetail& level_, int slotsPerSec_)
	: device(device_), level(level_), slotsPerSec(slotsPerSec_){}
	virtual ~LevelDeviceDetailedPrinter(){}
};

std::ostream& operator<<(std::ostream& stream, const DeviceDetailedPrinter& printer);

std::ostream& operator<<(std::ostream& stream, const LevelDeviceDetailedPrinter& printer);

}

}

#endif /* DEVICEPRINTER_H_ */
