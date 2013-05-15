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
 * NewDeviceContractResponse.h
 *
 *  Created on: Feb 19, 2009
 *      Author: sorin.bidian
 */

#ifndef NEWDEVICECONTRACTRESPONSE_H_
#define NEWDEVICECONTRACTRESPONSE_H_

#include "Common/NETypes.h"
#include "Common/Address128.h"
#include "Misc/Marshall/Stream.h"

namespace Isa100 {
namespace Model {

class NewDeviceContractResponse {

    public:

        /**
         * This element is related to the argument in ContractResponse.
         * Valid value set: 1 � 0xFFFF (0 reserved to mean no contract.
         */
        Uint16 contractID;

        /**
         * This element is related to the argument in ContractResponse.
         * Valid value set:70 � 1280.
         */
        Uint16 assignedMaxNSDUSize;

        /**
         * This element is related to the argument in ContractResponse.
         * Valid value set: 0 � 0xFFFF
         * (units in APDUs per second for positive values, units in seconds per APDU for negative values).
         */
        Int16 assignedCommittedBurst;

        /**
         * This element is related to the argument in ContractResponse.
         * Valid value set: 0 � 0xFFFF
         * (units in APDUs per second for positive values, units in seconds per APDU for negative values).
         */
        Int16 assignedExcessBurst;

        /**
         * This element is related to the argument in ContractResponse.
         * Valid value set: 0 � 0xFF.
         */
        Uint8 assignedMaxSendWindowSize;

        /**
         * Used for configuring the NL of the new device to use the contract assigned to the new device.
         * Valid value set: 0 = don�t include, 1 = include.
         */
        bool nlHeaderIncludeContractFlag;

        /**
         * Used for configuring the NL of the new device to use the contract assigned to the new device.
         */
        NE::Common::Address128 nlNextHop;

        /**
         * Used for configuring the NL of the new device to use the contract assigned to the new device.
         * Valid value set: 0 � 255.
         */
        Uint8 nlHopsLimit;

        /**
         * Used for configuring the NL of the new device to use the contract assigned to the new device.
         * Valid value set: 0-DL, 1-Backbone.
         */
        bool nlOutgoingInterface;

    public:

        NewDeviceContractResponse();

        ~NewDeviceContractResponse();

        void marshall(NE::Misc::Marshall::OutputStream& stream);

        void unmarshall(NE::Misc::Marshall::InputStream& stream);

        std::string toString();
};

}
}

#endif /* NEWDEVICECONTRACTRESPONSE_H_ */
