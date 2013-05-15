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
 * @author catalin.pop, ion.ticus, sorin.bidian
 */
#include "DeviceCapabilityHandler.h"
#include "Operations/ReadAttributeOperation.h"
#include "Operations/WriteAttributeOperation.h"
#include "Operations/DeleteAttributeOperation.h"

namespace NE {
namespace Model {

DeviceCapabilityHandler::DeviceCapabilityHandler(Subnet::PTR subnet_, TheoreticEngine & theoreticEngine_,
            Operations::OperationsProcessor & operationsProcessor_, Address32 addressDevice_, Address32 addressParent_,
            std::list<HandlerResponse>& handlerResponses_) :
    subnet(subnet_), theoreticEngine(theoreticEngine_), operationsProcessor(operationsProcessor_), addressDevice(addressDevice_),
                addressParent(addressParent_), handlerResponses(handlerResponses_) {

}

DeviceCapabilityHandler::~DeviceCapabilityHandler() {
}

void DeviceCapabilityHandler::process(Address32 deviceAddress32, int requestID, ResponseStatus::ResponseStatusEnum status) {
    if (status != ResponseStatus::SUCCESS) {
        callHandlerResponsesList(handlerResponses, deviceAddress32, requestID, status);
        return;
    }

    Device * device = subnet->getDevice(addressDevice);
    if (device == NULL) {
        LOG_ERROR("Device is NULL.");
        callHandlerResponsesList(handlerResponses, deviceAddress32, requestID, ResponseStatus::FAIL);
        return;
    }

    char reason[128];
    sprintf(reason, "Read_Write_Attributes device=%x", deviceAddress32);
    Operations::OperationsContainerPointer operationsContainer(
                new Operations::OperationsContainer(deviceAddress32, requestID, handlerResponses, reason));

    // 2.2 Power_Supply
    if (!device->capabilities.isBackbone()) {

        // 1. READ DMO.Power_Supply_Status
        EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::PowerSupply, 0);
        Operations::IEngineOperationPointer operation(new Operations::ReadAttributeOperation(entityIndex));
        operationsContainer->addOperation(operation, device);
    }

    if(device->capabilities.isDevice() && device->roleChanged) {
        // 2. READ DMO.Power_Supply_Status
        EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::AssignedRole, 0);
//        PhyAssignedRole * assignedRole = new PhyAssignedRole();
        PhyUint16 * assignedRole = new PhyUint16();
        assignedRole->value = device->capabilities.deviceType;
        Operations::IEngineOperationPointer operation(new Operations::WriteAttributeOperation(assignedRole, entityIndex));
        operationsContainer->addOperation(operation, device);
    }

    {
        // 3. WRITE DLMO.MaxLifetime
        EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::DLMO_MaxLifetime, 0);
        PhyUint16 * attributeValue = new PhyUint16();
        attributeValue->value = 184; // (45 (3(retries) x 15(contract at 15 sec)) + 1(marja)) * 4(units = 1/4 second)
        Operations::IEngineOperationPointer operation(new Operations::WriteAttributeOperation(attributeValue, entityIndex));
        operationsContainer->addOperation(operation, device);
    }

    {
        // 4. Delete Channel Hopping Sequence with ID = 0x7F (provisioned by device)
        PhyChannelHopping *  provisionedChannelHopping = new PhyChannelHopping();
        provisionedChannelHopping->index = 0x7F;

        EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::ChannelHopping, 0x7F);
        device->phyAttributes.createChannelHopping(entityIndex, provisionedChannelHopping, false);

        Operations::IEngineOperationPointer operation(new Operations::DeleteAttributeOperation(entityIndex));
        operationsContainer->addOperation(operation, device);
    }

    operationsProcessor.addOperationsContainer(operationsContainer);
}

}
}
