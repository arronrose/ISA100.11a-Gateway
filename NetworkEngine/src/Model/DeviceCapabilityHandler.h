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

#ifndef DEVICECAPABILITYHANDLER_H_
#define DEVICECAPABILITYHANDLER_H_

#include "Model/Subnet.h"
#include "Model/Device.h"
#include "TheoreticEngine.h"
#include "Operations/OperationsProcessor.h"
#include "Common/logging.h"

namespace NE {
namespace Model {

class DeviceCapabilityHandler {
    LOG_DEF("N.M.DeviceCapabilityHandler");

    Subnet::PTR subnet;
    TheoreticEngine& theoreticEngine;
    Operations::OperationsProcessor& operationsProcessor;
    Address32 addressDevice;
    Address32 addressParent;
    HandlerResponseList handlerResponses;
    public:
        DeviceCapabilityHandler(Subnet::PTR subnet,
                    TheoreticEngine & theoreticEngine,
                    Operations::OperationsProcessor & operationsProcessor,
                    Address32 addressDevice,
                    Address32 addressParent,
                    HandlerResponseList& handlerResponses);
        virtual ~DeviceCapabilityHandler();

        void process(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status);
};

typedef boost::shared_ptr<DeviceCapabilityHandler> DeviceCapabilityHandlerPointer;

}
}

#endif /* DEVICECAPABILITYHANDLER_H_ */
