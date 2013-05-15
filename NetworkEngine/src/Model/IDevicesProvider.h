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
 * IDevicesProvider.h
 *
 *  Created on: Mar 31, 2009
 *      Author: Catalin Pop
 */

#ifndef IDEVICESPROVIDER_H_
#define IDEVICESPROVIDER_H_
#include "Model/Device.h"

namespace NE {

namespace Model {

class IDevicesProvider {
    public:
        virtual ~IDevicesProvider() {};

        /**
         * Find a device by its 32 address. If the device is not found an exception is thrown.
         * @param Address32
         * @return Device&
         * @throws DeviceNotFoundException if a device with this address is not found.
         */
        virtual Device& getDevice(Address32 address) = 0;

        virtual bool existsDevice(Address32 address32) = 0;
};

} // namespace Model

} // namespace Isa100

#endif /* IDEVICESPROVIDER_H_ */
