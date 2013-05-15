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
#ifndef DEVICESECURITYINFO_H_
#define DEVICESECURITYINFO_H_

#include <map>

#include "Common/NETypes.h"
#include "Common/logging.h"
#include "Common/NEAddress.h"
#include "Model/SecurityKey.h"

using namespace NE::Model;
using namespace NE::Common;

namespace Isa100 {
namespace Security {

/**
 * Keep the security information related to a joined device
 * @author Ioan-Vasile Pocol, catalin.pop, beniamin.tecar
 * @version 1.0
 */
class DeviceSecurityInfo {

    public:
        LOG_DEF("I.S.DeviceSecurityInfo")

        DeviceSecurityInfo();

        /**
         * Initializes the device's address, master key and reply challenge
         */
        DeviceSecurityInfo(const Address64& address64, const SecurityKey& masterKey, Uint32 challenge);

        virtual ~DeviceSecurityInfo();

        /**
         * @returns SecurityKey device's master key
         */
        SecurityKey getMasterKey();

        /**
         * @returns Uint32 reply challange
         */
        Uint32 getChallenge();

        /**
         * @returns Uint32 device policies
         */
        Uint32 getPolicies();

        /**
         * Set the current policies
         * @param Uint32 policies
         * @returns
         */
        void setPolicies(Uint32 policies);

        /**
         * @returns set the current DLL subnet
         */
        Uint16 getSubnetId();

        /**
         * @returns set the current DLL subnet
         */
        void setSubnetId(Uint16 subnetId);

        /**
         * Set the session key for the received address
         * If the key already exists it is overwrited
         * @param address64 the address of the contract device pair
         * @param key the associated session key
         */
        void addContractKey(Address64 address64, SecurityKey key);

        /**
         * Finds the contract key associated to the received address
         * @param address64 contract pair address
         * @return SecurityKey
         * @throws Isa100Exception
         */
        SecurityKey getContractKey(Address64 address64);

        std::string toString() {
            std::ostringstream stream;
            std::string policiesString;
            Type::toString(challenge, policiesString);
            stream << "{ address=" << address64.toString()
				<< ", subnetId=" << (int)subnetId
				<< ", masterKey=" << masterKey.toString()
				<< ", challenge=" << policiesString;
            Type::toString(policies, policiesString);
			stream << ", policies=" << policiesString
				<< ", list session keys={";

            for(map<Address64, SecurityKey>::iterator it = contractKeys.begin();
				it != contractKeys.end(); it++) {

            	stream << it->first.toString() << " => " << it->second.toString()
					<< ", ";
            }

            stream << "} }";

            return stream.str();
        }

    private:
        //device address
        Address64 address64;

        //current device DLL subnet
        Uint16 subnetId;

        //master key
        SecurityKey masterKey;

        //reply challange
        Uint32 challenge;

        //policies
        Uint32 policies;

        //the list session keys
        map<Address64, SecurityKey> contractKeys;

};
}
}
#endif /*DEVICESECURITYINFO_H_*/
