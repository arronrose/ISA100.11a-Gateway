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
 * SmContractRequest.cpp
 *
 *  Created on: Mar 4, 2009
 *      Author: sorin.bidian
 */

#include "SmContractRequest.h"
#include <Model/EngineProvider.h>

using namespace NE::Misc::Marshall;
using namespace NE::Model;

namespace Isa100 {
namespace Model {


NE::Model::ContractRequestPointer SmContractRequest::unmarshall(InputStream& stream) {
    NE::Model::ContractRequestPointer contractRequest(new ContractRequest());
    Uint8 octet;
    stream.read(octet);
    contractRequest->contractRequestId = octet;

    stream.read(octet);
    contractRequest->contractRequestType = (ContractRequestType::ContractRequestType) octet;
    if (contractRequest->contractRequestType != 0) {
        stream.read(contractRequest->contractId);
    }

    stream.read(octet);
    contractRequest->communicationServiceType = (CommunicationServiceType::CommunicationServiceType) octet;

    Uint16 port;
    stream.read(port);
    contractRequest->sourceSAP = port - 0xF0B0;

    //stream contains destinationAddress as Address128; Address32 needed for ContractRequest
    Address128 destinationAddress;
    destinationAddress.unmarshall(stream);
    contractRequest->destinationAddress = Isa100::Model::EngineProvider::getEngine()->getAddress32(destinationAddress);

    stream.read(port);
    contractRequest->destinationSAP = port - 0xF0B0;

    stream.read(octet);
    contractRequest->contractNegotiability = (ContractNegotiability::ContractNegotiability) octet;

    stream.read(contractRequest->contractLife);

    stream.read(octet);
    contractRequest->contractPriority = (ContractPriority::ContractPriority) octet;

    stream.read(contractRequest->payloadSize);
    stream.read(contractRequest->reliability_And_PublishAutoRetransmit);

    if (contractRequest->communicationServiceType == CommunicationServiceType::Periodic) {
        stream.read(contractRequest->requestedPeriod);
        stream.read(contractRequest->requestedPhase);
        stream.read(contractRequest->requestedDeadline);
    } else if (contractRequest->communicationServiceType == CommunicationServiceType::NonPeriodic) {
        stream.read(contractRequest->committedBurst);
        stream.read(contractRequest->excessBurst);
        stream.read(contractRequest->maxSendWindowSize);
    }
    return contractRequest;
}


}
}

