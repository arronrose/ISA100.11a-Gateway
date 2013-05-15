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
 * NewDeviceContractResponse.cpp
 *
 *  Created on: Feb 19, 2009
 *      Author: sorin.bidian
 */

#include "NewDeviceContractResponse.h"
#include "Misc/Convert/Convert.h"

using namespace NE::Misc::Marshall;
using namespace NE::Misc::Convert;

namespace Isa100 {
namespace Model {

NewDeviceContractResponse::NewDeviceContractResponse() :
    contractID(0), assignedMaxNSDUSize(0), assignedCommittedBurst(0), assignedExcessBurst(0),
                assignedMaxSendWindowSize(0), nlHeaderIncludeContractFlag(0)/*, nlHopsLimit(0), nlOutgoingInterface(0)*/{
}

NewDeviceContractResponse::~NewDeviceContractResponse() {
}

void NewDeviceContractResponse::marshall(NE::Misc::Marshall::OutputStream& stream) {
    stream.write(contractID);
    stream.write(assignedMaxNSDUSize);
    stream.write(assignedCommittedBurst);
    stream.write(assignedExcessBurst);
    stream.write(assignedMaxSendWindowSize);
    stream.write(nlHeaderIncludeContractFlag);
    nlNextHop.marshall(stream);
    stream.write(nlHopsLimit);
    stream.write(nlOutgoingInterface);
}

void NewDeviceContractResponse::unmarshall(NE::Misc::Marshall::InputStream& stream) {
    stream.read(contractID);
    stream.read(assignedMaxNSDUSize);
    stream.read(assignedCommittedBurst);
    stream.read(assignedExcessBurst);
    stream.read(assignedMaxSendWindowSize);
    stream.read(nlHeaderIncludeContractFlag);
    nlNextHop.unmarshall(stream);
    stream.read(nlHopsLimit);
    stream.read(nlOutgoingInterface);
}

std::string NewDeviceContractResponse::toString() {
    std::ostringstream stream;
    stream << "NewDeviceContractResponse {";
    stream << "contractID: " << std::hex << (int) contractID;
    stream << ", assignedMaxNSDUSize=" << (int) assignedMaxNSDUSize;
    stream << ", assignedCommittedBurst=" << (int) assignedCommittedBurst;
    stream << ", assignedExcessBurst=" << (int) assignedExcessBurst;
    stream << ", assignedMaxSendWindowSize=" << (int) assignedMaxSendWindowSize;
    stream << ", nlHeaderIncludeContractFlag=" << (int) nlHeaderIncludeContractFlag;
    stream << ", nlNextHop=" << nlNextHop.toString();
    stream << ", nlHopsLimit=" << (int) nlHopsLimit;
    stream << ", nlOutgoingInterface=" << (int) nlOutgoingInterface;
    stream << "}";

    return stream.str();
}

}
}
