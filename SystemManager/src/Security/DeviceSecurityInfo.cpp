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

/**
 * @author catalin.pop, beniamin.tecar
 */
#include "Security/DeviceSecurityInfo.h"
#include "Misc/Convert/Convert.h"

using namespace NE::Model;

namespace Isa100 {
namespace Security {

DeviceSecurityInfo::DeviceSecurityInfo() {

}

DeviceSecurityInfo::DeviceSecurityInfo(const Address64& address64, const SecurityKey& masterKey, Uint32 challenge) {
    this->address64 = address64;
    this->masterKey = masterKey;
    this->challenge = challenge;

    this->policies = 0xF0F0F0F0;
}

DeviceSecurityInfo::~DeviceSecurityInfo() {
}

SecurityKey DeviceSecurityInfo::getMasterKey() {
    return this->masterKey;
}

Uint32 DeviceSecurityInfo::getChallenge() {
    return this->challenge;
}

void DeviceSecurityInfo::addContractKey(Address64 address64, SecurityKey key) {
    contractKeys[address64] = key;
}

SecurityKey DeviceSecurityInfo::getContractKey(Address64 address64) {
    if (contractKeys.find(address64) == contractKeys.end()) {
        ostringstream stream;
        stream << "No session key for the device with address64=";
        stream << array2string(address64.value, address64.LENGTH);

        LOG_DEBUG(stream.str());

        throw NEException(stream.str());
    }
    return contractKeys[address64];
}

Uint16 DeviceSecurityInfo::getSubnetId() {
    return this->subnetId;
}

void DeviceSecurityInfo::setSubnetId(Uint16 subnetId) {
    this->subnetId = subnetId;
}

Uint32 DeviceSecurityInfo::getPolicies() {
    return this->policies;
}

void DeviceSecurityInfo::setPolicies(Uint32 policies) {
    this->policies = policies;
}
}
}
