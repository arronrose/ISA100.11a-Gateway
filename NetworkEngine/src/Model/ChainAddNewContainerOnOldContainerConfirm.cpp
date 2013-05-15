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
 * ChainAddNewContainerOnOldContainerConfirm.cpp
 *
 *  Created on: Mar 4, 2010
 *      Author: flori.parauan
 */
#include <boost/bind.hpp>
#include "ChainAddNewContainerOnOldContainerConfirm.h"
#include "ChainWaitForConfirmOnEvalGraph.h"
#include "TheoreticEngine.h"


namespace NE {

namespace Model {


ChainAddNewContainerOnOldContainerConfirm::ChainAddNewContainerOnOldContainerConfirm(Subnet::PTR subnet_,  TheoreticEngine * theoreticEngine_, NE::Model::Operations::OperationsProcessor* operationsProcessor_,  Address32 deviceAddress32_, Address32 newParentAddress32_, Address32 oldParentAddress32_)
:subnet(subnet_), theoreticEngine(theoreticEngine_),operationsProcessor(operationsProcessor_),
deviceAddress32(deviceAddress32_), newParentAddress32(newParentAddress32_), oldParentAddress32(oldParentAddress32_){
}

ChainAddNewContainerOnOldContainerConfirm::~ChainAddNewContainerOnOldContainerConfirm() {
}

void ChainAddNewContainerOnOldContainerConfirm::process(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status) {
    if (status == ResponseStatus::SUCCESS){
        Device* device = subnet->getDevice(deviceAddress32);
        if(!device) {
            LOG_ERROR("Device not found" << Address_toStream(deviceAddress32));
            return;
        }

        Device* newParent = subnet->getDevice(newParentAddress32);
        if(!newParent) {
            LOG_ERROR("Device not found" << Address_toStream(newParentAddress32));
            return;
        }

        Device* oldParent = subnet->getDevice(oldParentAddress32);
        if(!oldParent) {
            LOG_ERROR("Device not found" << Address_toStream(oldParentAddress32));
            return;
        }

		char reason[128];
		sprintf(reason, "redirect device's parent to an old child, device=%x, neighbor=%x", device->address32, newParent->address32);
		ChainWaitForConfirmOnEvalGraphPointer chainConfirmGraphOperations(new ChainWaitForConfirmOnEvalGraph(subnet, DEFAULT_GRAPH_ID));
		HandlerResponseList handlersConfirmGraphOperations;
		handlersConfirmGraphOperations.push_back(boost::bind(&ChainWaitForConfirmOnEvalGraph::process, chainConfirmGraphOperations, _1, _2, _3));

		Operations::OperationsContainerPointer operationsContainer(new Operations::OperationsContainer(device->address32, 0, handlersConfirmGraphOperations, reason));
		theoreticEngine->redirectDeviceToNewParent(subnet,  device, oldParent, newParent, operationsContainer);
		operationsProcessor->addOperationsContainer(operationsContainer);
    } else {
        LOG_DEBUG("GRAPH EVAL enable in ChainAddNewContainerOnOldContainerConfirm");
        subnet->enableInboundGraphEvaluation();
    }
}

}
}
