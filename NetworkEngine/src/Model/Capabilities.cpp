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
 * Capabilities.cpp
 *
 *  Created on: May 22, 2008
 *      Author: catalin.pop
 */
#include "Capabilities.h"
#include <iomanip>

namespace NE {
namespace Model {

using namespace NE::Common;
using namespace NE::Misc::Marshall;

Capabilities::Capabilities() :
    euidAddress(),
    dllSubnetId(0),
    deviceType(DeviceType::NOT_SET),
    tagName() {
}

std::string Capabilities::getRoleAsString() {
    if (isManager()) {
        return "Manager";
    }
    if (isSecurityManager()) {
        return "SecMan";
    } else if (isBackbone()) {
        return "BR";
    } else if (isGateway()) {
        return "GW";
    } else if (isRouting()) {
        return "Rout";
    } else if (isFieldDevice()) {
        return "FD";
    } else if (isFieldDevice()) {
        return "FD";
    } else if (isDevice()) {
        return "Dev";
    } else if (isTimeSource()) {
        return "TimSrc";
    } else if (isProvisioning()) {
        return "Prov";
    }

    return "Unknown role";
}

void Capabilities::toString( std::ostringstream& stream) {
    stream << "Capabilities {";
    stream << "euidAddr=" << euidAddress.toString();
    stream << ", dllSubId=" << std::hex << (int) dllSubnetId;
    stream << ", devType=" << std::setw(18) << DeviceType::toString(deviceType);
    stream << "}";

}

void Capabilities::toTableIndentString(std::ostringstream& stream) {
    stream << std::setw(20) << euidAddress.toString();
    stream << std::setw(5)  << std::hex << (int) dllSubnetId << std::dec;
    stream << std::setw(18) << DeviceType::toString(deviceType);

}

std::ostream& operator<<(std::ostream& stream, const Capabilities& capabilities) {

    stream << "euidAddr=" << capabilities.euidAddress.toString();
    stream << ", dllSubId=" << std::hex << (int) capabilities.dllSubnetId;
    stream << ", devType=" << std::setw(18) << DeviceType::toString(capabilities.deviceType);

    return stream;
}


}
}
