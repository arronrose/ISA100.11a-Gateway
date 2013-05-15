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

#ifndef CAPABILITIES_H_
#define CAPABILITIES_H_

#include "Common/NETypes.h"
#include "Common/NEAddress.h"

using namespace NE::Common;
using namespace NE::Misc::Marshall;

namespace NE {
namespace Model {
namespace DeviceType {

/**
 * Bit 0 - IO
 * Bit 1 - router
 * Bit 2 - backbone router
 * Bit 3 - gateway
 * Bit 4 - system manager
 * Bit 5 - security manager
 * Bit 6 - system time source
 * Bit 7 - provisioning device
 */
enum DeviceTypeEnum {
    NOT_SET = 0,
    IN_OUT = 1, // if device has IO role (has a sensor attached to it)
    ROUTER = 2,
    ROUTER_IO = 3,
    BACKBONE = 4,
    GATEWAY = 8,
    MANAGER = 16,
    SECURITY_MANAGER = 32,
    TIME_SOURCE = 64,
    PROVISIONING = 128

};

inline
std::string toString(Uint16 deviceType) {

    if (deviceType & NE::Model::DeviceType::BACKBONE) {
        return "BACKBONE";
    } else if (deviceType & NE::Model::DeviceType::MANAGER) {
        return "MANAGER";
    } else if (deviceType & NE::Model::DeviceType::GATEWAY) {
        return "GATEWAY";
    } else if (deviceType & NE::Model::DeviceType::ROUTER) {
        if (deviceType & NE::Model::DeviceType::IN_OUT){
            return "ROUTER-IO";
        }
        return "ROUTER";
    } else if (deviceType & NE::Model::DeviceType::IN_OUT) {
        return "IN_OUT";
    } else if (deviceType & NE::Model::DeviceType::PROVISIONING) {
        return "PROVISIONING";
    } else if (deviceType & NE::Model::DeviceType::SECURITY_MANAGER) {
        return "SECURITY_MANAGER";
    } else if (deviceType & NE::Model::DeviceType::TIME_SOURCE) {
        return "TIME_SOURCE";
    }
    return "N/A";
}

} // namespace DeviceType


class Capabilities {
    public:

    	/**
    	 * DMO attribute EUI64.
    	 */
    	Address64 euidAddress;

    	/**
    	 * DL subnet that the new device is trying to join; this is also the DL subnet of the advertising router.
    	 */
        Uint16 dllSubnetId;

        /**
         * DMO attribute Device_Role_Capability.
         */
        Uint16 deviceType; //to verify the role of device use the provided methods "isXXX"

        /**
         * DMO attribute Tag_Name.
         * Max length: 16 characters.
         */
        std::string tagName;


        Capabilities();

        /**
         * Returns true if the device type indicates a SYSTEM_MANAGER device type.
         */
        bool isManager() const { return deviceType & NE::Model::DeviceType::MANAGER; }

        /**
         * Returns true if the device type indicates a SECURITY_MANAGER device type.
         */
        bool isSecurityManager() const {return deviceType & NE::Model::DeviceType::SECURITY_MANAGER;}
        /**
         * Returns true if the device type indicates a GATEWAY device type.
         */
        bool isGateway() const { return deviceType & NE::Model::DeviceType::GATEWAY; }
        /**
         * Returns true if the device type indicates a BACKBONE device type.
         */
        bool isBackbone() const { return deviceType & NE::Model::DeviceType::BACKBONE; }
        /**
         * Returns true if the device type indicates a ROUTING device type.
         */
        bool isRouting() const { return deviceType & NE::Model::DeviceType::ROUTER; }

        /**
         * Returns true if the device type indicates a FIELD_DEVICE device type.
         */
        bool isFieldDevice() const { return deviceType & NE::Model::DeviceType::IN_OUT; }

        /**
         * Returns true if the device type indicates a NON-ROUTING  but IO (only) device type.
         */
        bool isNonRouting() const { return !isRouting() && isFieldDevice(); }

        /**
         * Returns true if the device type indicates a FIELD_DEVICE or ROUTER device type.
         */
        bool isDevice() const { return isFieldDevice() || isRouting(); }

        /**
         * Returns true if the device type indicates a TIME_SOURCE device type.
         */
        bool isTimeSource() const {return deviceType & NE::Model::DeviceType::TIME_SOURCE;}

        /**
         * Returns true if the device type indicates a PROVISIONING device type.
         */
        bool isProvisioning() const {return deviceType & NE::Model::DeviceType::PROVISIONING;}

        // std::string getDeviceTypeDescription() const;

    public:

        /**
         * Returns a string representation of this Capabilities.
         */
        friend std::ostream& operator<<(std::ostream&, const Capabilities& capabilities);

        /**
         * Returns a string representation of these capabilities.
         */
        void toString( std::ostringstream& stream);

        /**
         * Returns a string representation of these capabilities
         * (for the table output format).
         */
       void toTableIndentString(std::ostringstream& stream);

        std::string getRoleAsString();

};
}
}

#endif /*CAPABILITIES_H_*/
