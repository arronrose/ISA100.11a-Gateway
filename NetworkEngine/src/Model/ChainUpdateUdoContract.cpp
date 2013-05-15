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
 * @author beniamin.tecar
 */
#include "ChainUpdateUdoContract.h"
#include "TheoreticEngine.h"
#include "Operations/OperationsContainer.h"
#include "SubnetsContainer.h"

#include <boost/bind.hpp>


using namespace NE::Model::Operations;

namespace NE {
namespace Model {


ChainUpdateUdoContract::ChainUpdateUdoContract(Uint16 subnetID_, EntityIndex & eiContract_, TheoreticEngine * theoreticEngine_)
    : subnetID(subnetID_), eiContract(eiContract_), theoreticEngine(theoreticEngine_) {

}

ChainUpdateUdoContract::~ChainUpdateUdoContract() {
}

void ChainUpdateUdoContract::process(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status) {

    Subnet::PTR subnet = theoreticEngine->getSubnetsContainer()->getSubnet(subnetID);

    if (status != ResponseStatus::SUCCESS) {
        theoreticEngine->addUdoContractToBeEvaluated(subnet, eiContract);
        theoreticEngine->updateUdoContract(subnet, eiContract, -8);

        LOG_WARN("handler called with status: " << (int) status);

        return;
    }

    theoreticEngine->updateUdoContract(subnet, eiContract, 1);

}

}
}
