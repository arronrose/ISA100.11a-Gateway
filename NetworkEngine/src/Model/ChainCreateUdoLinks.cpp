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

#include "ChainCreateUdoLinks.h"
#include "TheoreticEngine.h"
#include "ChainUpdateUdoContract.h"
#include "Operations/OperationsContainer.h"
#include "Operations/OperationsProcessor.h"
#include "SubnetsContainer.h"

#include <boost/bind.hpp>

using namespace NE::Model::Operations;

namespace NE {
namespace Model {

ChainCreateUdoLinks::ChainCreateUdoLinks(Uint16 subnetID_, EntityIndex & eiContract_, Address32 contractDestination_, TheoreticEngine * theoreticEngine_)
    : subnetID(subnetID_), eiContract(eiContract_), contractDestination(contractDestination_), theoreticEngine(theoreticEngine_) {
}

ChainCreateUdoLinks::~ChainCreateUdoLinks() {
}

void ChainCreateUdoLinks::process(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status){

    Subnet::PTR subnet = theoreticEngine->getSubnetsContainer()->getSubnet(subnetID);

    if (status != ResponseStatus::SUCCESS) {
        LOG_WARN("handler called with status: " << (int) status);
        theoreticEngine->addUdoContractToBeEvaluated(subnet, eiContract); //if not added here, when status is not success contract will not be evaluated anymore
        return;
    }

    char reason[128];
    sprintf(reason, "Create links for udo contract %x", getIndex(eiContract));

    ChainUpdateUdoContractPointer chainUpdateUdoContract(new ChainUpdateUdoContract(subnetID, eiContract, theoreticEngine));
    OperationsContainerPointer container(new OperationsContainer(0, 0, boost::bind(&ChainUpdateUdoContract::process, chainUpdateUdoContract, _1, _2, _3), reason));

    Device *destinationDevice = subnet->getDevice(contractDestination);
    RETURN_ON_NULL_MSG(destinationDevice, "Device not found: " << Address_toStream(contractDestination));

    Device *backbone = subnet->getBackbone();
    RETURN_ON_NULL_MSG(backbone, "Backbone not found for subnet " << (int) subnetID);

    Uint16 numberOfUdoContractsAtOneSecond = subnet->getManager()->theoAttributes.getNoOfUdoContractsAtOneSecond();
    if ( numberOfUdoContractsAtOneSecond >= subnet->getSubnetSettings().maxNoUdoContractsAtOneSecond) {
        LOG_WARN("Current UDO contractsAtOneSecond=" << (int) numberOfUdoContractsAtOneSecond << ", max number Of ContractsAtOneSecond=" << (int)subnet->getSubnetSettings().maxNoUdoContractsAtOneSecond);
        theoreticEngine->addUdoContractToBeEvaluated(subnet, eiContract); //if not added here, when status is not success contract will not be evaluated anymore
    	return;
    }

    for (RouteIndexedAttribute::iterator itRoutes = backbone->phyAttributes.routesTable.begin();
                itRoutes != backbone->phyAttributes.routesTable.end(); ++itRoutes) {

        PhyRoute * phyRoute = itRoutes->second.currentValue;
        if (phyRoute
                    && isRouteGraphElement(phyRoute->route[0])
                    && phyRoute->selector == Address::getAddress16(contractDestination) ) {

            Uint16 graphID = getRouteElement(phyRoute->route[0]);
            if (!theoreticEngine->allocateUdoLinksForOutboundGraph(container, subnet, graphID, eiContract)) {
                theoreticEngine->addUdoContractToBeEvaluated(subnet, eiContract);
                container->setAsFail(theoreticEngine->getSubnetsContainer());
                return;
            }
            break;
        }
    }

    if (!theoreticEngine->allocateUdoLinksForInboundGraph(container, subnet, destinationDevice, eiContract)) {
        theoreticEngine->addUdoContractToBeEvaluated(subnet, eiContract);
        container->setAsFail(theoreticEngine->getSubnetsContainer());
        return;
    }
    theoreticEngine->getOperationsProcessor()->addOperationsContainer(container);
}

}
}
