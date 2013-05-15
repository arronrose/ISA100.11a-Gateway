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
 * IDeletedDeviceListener.h
 *
 *  Created on: Nov 4, 2009
 *      Author: Catalin Pop
 */

#ifndef IDELETEDDEVICELISTENER_H_
#define IDELETEDDEVICELISTENER_H_
namespace NE {

namespace Model {

class IDeletedDeviceListener{
    public:
        virtual ~IDeletedDeviceListener() {};
        /**
         * Notify method called when a device is deleted. The deviceType parameter contains the role of the removed device.
         * It must be verified against DeviceTypeEnum (from Capabilities.h)
         * @param deletedDeviceAddress
         * @param deviceType
         */
        virtual void deviceDeletedCallback(Address32 deletedDeviceAddress, Uint16 deviceType) = 0;
};

}  // namespace Model

}  // namespace NE

#endif /* IDELETEDDEVICELISTENER_H_ */
