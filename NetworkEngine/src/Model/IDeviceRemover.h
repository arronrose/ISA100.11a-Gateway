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
 * IDeviceRemover.h
 *
 *  Created on: Oct 23, 2009
 *      Author: Catalin Pop
 */

#ifndef IDEVICEREMOVER_H_
#define IDEVICEREMOVER_H_
#include "Model/Operations/OperationsContainer.h"
#include "Common/NETypes.h"

namespace NE {

namespace Model {

class IDeviceRemover {
    public:
        virtual ~IDeviceRemover() {};
        /**
         *
         * Call back for the remove device on error.
         * Parameters: Address32 of the device that must be deleted.
         * The function returns the a container of operations for the deleted device:
         *  1. If the removed device is BBR or SM generates:
         *      - on SM remove key
         *      - on SM remove contract
         *  2. If the removed device is a field device generates:
         *      - on BR remove route with the removed device
         *      - for every device's neighbor generates delete neighbor
         *      - for every device's neighbor generates delete links (T and R)
         *      - for every device's graph check to see if there is a route on BBR that uses the graph. If there is then
         *      and this route id to Theoretical Engine list of routes to be evaluated. If there is no route then add the
         *      graph to the Theoretical Engine list of routes to be evaluated.
         * @param address32
         * @param container
         */
        virtual void removeDeviceOnError(Address32 address32, Operations::OperationsContainerPointer container, int reason) = 0;
};

} // namespace Model

} // namespace NE

#endif /* IDEVICEREMOVER_H_ */
