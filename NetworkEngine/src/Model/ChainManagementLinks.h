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
 * ChainManagementLinks.h
 *
 *  Created on: Oct 13, 2009
 *      Author: Catalin Pop
 */

#ifndef CHAINMANAGEMENTLINKS_H_
#define CHAINMANAGEMENTLINKS_H_

#include "Model/Subnet.h"
#include "Model/Device.h"
#include "TheoreticEngine.h"
#include "Operations/OperationsProcessor.h"
#include "Common/logging.h"

namespace NE {

namespace Model {

class ChainManagementLinks {
    LOG_DEF("N.M.ChainManagementLinks");
    Subnet::PTR subnet;
    TheoreticEngine& theoreticEngine;
    Operations::OperationsProcessor& operationsProcessor;
    Address32 addressDevice;
    Address32 addressParent;
    std::list<HandlerResponse> handlerResponses;
    public:
        ChainManagementLinks(Subnet::PTR subnet,
                    TheoreticEngine & theoreticEngine,
                    Operations::OperationsProcessor & operationsProcessor,
                    Address32 addressDevice,
                    Address32 addressParent,
                    std::list<HandlerResponse>& handlerResponses);
        virtual ~ChainManagementLinks();

        void process(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status);
};
typedef boost::shared_ptr<ChainManagementLinks> ChainManagementLinksPointer;

}

}

#endif /* CHAINMANAGEMENTLINKS_H_ */
