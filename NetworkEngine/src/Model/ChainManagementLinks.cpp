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
 * ChainManagementLinks.cpp
 *
 *  Created on: Oct 13, 2009
 *      Author: Catalin Pop
 */

#include "ChainManagementLinks.h"

namespace NE {
namespace Model {

ChainManagementLinks::ChainManagementLinks(Subnet::PTR subnet_, TheoreticEngine & theoreticEngine_,
            Operations::OperationsProcessor & operationsProcessor_, Address32 addressDevice_, Address32 addressParent_,
            std::list<HandlerResponse>& handlerResponses_) :
    subnet(subnet_), theoreticEngine(theoreticEngine_), operationsProcessor(operationsProcessor_), addressDevice(addressDevice_),
                addressParent(addressParent_), handlerResponses(handlerResponses_) {

}

ChainManagementLinks::~ChainManagementLinks() {
}

void ChainManagementLinks::process(Address32 deviceAddress32, int requestID, ResponseStatus::ResponseStatusEnum status) {
    if (status != ResponseStatus::SUCCESS) {
        callHandlerResponsesList(handlerResponses, deviceAddress32, requestID, status);
        return;
    }
    Device * device = subnet->getDevice(addressDevice);
    if (device == NULL) {
        LOG_ERROR("Device " << Address_toStream(addressDevice) << " does not exist.");
        callHandlerResponsesList(handlerResponses, deviceAddress32, requestID, ResponseStatus::FAIL);
        return;
    }
    Device * parentDevice = subnet->getDevice(addressParent);
    if (parentDevice == NULL) {
        LOG_ERROR("Device " << Address_toStream(addressParent) << " does not exist.");
        callHandlerResponsesList(handlerResponses, deviceAddress32, requestID, ResponseStatus::FAIL);
        return;
    }

    char reason[128];
    sprintf(reason, "allocateManagementJoinLinks for device %x, parent=%x", addressDevice, addressParent);
    Operations::OperationsContainerPointer operationsContainer(
                new Operations::OperationsContainer(deviceAddress32, requestID, handlerResponses, reason));

    if (theoreticEngine.allocateManagementJoinLinks(device, parentDevice, operationsContainer)){
        operationsProcessor.addOperationsContainer(operationsContainer);
    } else {
        LOG_WARN("Allocation of management links failed for:" << Address_toStream(device->address32) << " parent:" << Address_toStream(parentDevice->address32));
        operationsContainer->setAsFail(theoreticEngine.getSubnetsContainer());
        callHandlerResponsesList(handlerResponses, deviceAddress32, requestID, ResponseStatus::REFUSED_INSUFICIENT_RESOURCES);
    }
}

}
}
