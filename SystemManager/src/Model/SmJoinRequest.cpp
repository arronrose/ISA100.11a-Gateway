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
 * SmJoinRequest.cpp
 *
 *  Created on: May 18, 2010
 *      Author: Sorin.Bidian
 */

#include "SmJoinRequest.h"

namespace Isa100 {
namespace Model {

void SmJoinRequest::marshall(OutputStream& stream) {
	capabilities.euidAddress.marshall(stream);
	stream.write(capabilities.dllSubnetId);
	stream.write((Uint16) capabilities.deviceType);
	stream.write(tagNameLength);
	stream.write(Bytes(capabilities.tagName.begin(), capabilities.tagName.end()));
	stream.write(com_SW_major_version);
	stream.write(com_SW_minor_version);

	stream.write(softwareRevisionInformationLength);
	stream.write(Bytes(softwareRevisionInformation.begin(), softwareRevisionInformation.end()));

}

void SmJoinRequest::unmarshall(InputStream& stream) {
	capabilities.euidAddress.unmarshall(stream);

	stream.read(capabilities.dllSubnetId); //DL_Subnet_ID

	stream.read(capabilities.deviceType);//Device_Role_Capability

	stream.read(tagNameLength);
	Bytes tagName2;
	stream.read(tagName2, tagNameLength);
	capabilities.tagName.assign(tagName2.begin(), tagName2.end());

	stream.read(com_SW_major_version);

	stream.read(com_SW_minor_version);

	stream.read(softwareRevisionInformationLength);
	Bytes softwareRevisionInformation2;
	stream.read(softwareRevisionInformation2, softwareRevisionInformationLength);
	softwareRevisionInformation.assign(softwareRevisionInformation2.begin(), softwareRevisionInformation2.end());

	try {
		deviceCapability.queueCapacity = Isa100::Common::Utils::unmarshallExtDLUint(stream);
		stream.read(deviceCapability.clockAccuracy);
		stream.read(deviceCapability.channelMap);
		stream.read(deviceCapability.dlRoles);

		stream.read(deviceCapability.energyDesign.energyLife);
		deviceCapability.energyDesign.listenRate = Isa100::Common::Utils::unmarshallExtDLUint(stream);
		deviceCapability.energyDesign.transmitRate = Isa100::Common::Utils::unmarshallExtDLUint(stream);
		deviceCapability.energyDesign.advRate = Isa100::Common::Utils::unmarshallExtDLUint(stream);

		stream.read(deviceCapability.energyLeft);
		deviceCapability.ack_Turnaround = Isa100::Common::Utils::unmarshallExtDLUint(stream);
		deviceCapability.neighborDiagCapacity = Isa100::Common::Utils::unmarshallExtDLUint(stream);
		stream.read(deviceCapability.radioTransmitPower);
		stream.read(deviceCapability.options);

    } catch (NE::Misc::Marshall::StreamException& ex) {
        LOG_ERROR("Reading device capability failed.. " << ex.what());
        throw NE::Misc::Marshall::StreamException("Invalid join request content");
    }
}

}
}

