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
 * Policy.h
 *
 *  Created on: Mar 18, 2009
 *      Author: Andy(andrei.petrut)
 */

#ifndef POLICY_H_
#define POLICY_H_

#include <string>
#include "Misc/Marshall/NetworkOrderStream.h"
#include <iomanip>

namespace NE {
namespace Model {

// Represents a policy
struct Policy {
        Uint8 Granularity;
        Uint8 KeyUsage;
        Uint8 KeyType;

        Uint32 NotValidBefore;

        Uint32 KeyHardLifetime; //the value from config.ini

        Uint8 PolicyPolicy;

        Policy() :
            Granularity(2), //hours (same as in Security Manager)
            KeyUsage(0),
            KeyType(0),
            NotValidBefore(0),
            KeyHardLifetime(0),
            PolicyPolicy(0) {
        }

        Policy(const Policy& other):
            Granularity(other.Granularity),
            KeyUsage(other.KeyUsage),
            KeyType(other.KeyType),
            NotValidBefore(other.NotValidBefore),
            KeyHardLifetime(other.KeyHardLifetime),
            PolicyPolicy(other.PolicyPolicy)
            {
        }

        std::string toString() const {
            std::ostringstream stream;

            stream << "Granularity=" << std::hex << std::setw(2) << (int)Granularity
                << ", KeyUsage=" << std::hex << std::setw(2) << (int) KeyUsage
                << ", KeyType=" << std::hex << std::setw(2) << (int)KeyType
                << ", NotValidBefore=" << std::hex << std::setw(8) << (int)NotValidBefore
                << ", KeyHardLifetime=" << std::hex << std::setw(8) << (int)KeyHardLifetime;
                        //<< ", PolicyPolicy=" << (int) PolicyPolicy; //not used

            return stream.str();
        }
};

// Compressed policy
struct CompressedPolicy {
        // expressed in hours
        Uint16 KeyHardLifetime;

        CompressedPolicy() :
            KeyHardLifetime(0) {
        }
};

void ToString(Policy policy, std::string &policyString);

void ToString(CompressedPolicy policy, std::string &compressedPolicyString);

}
}

#endif /* POLICY_H_ */
