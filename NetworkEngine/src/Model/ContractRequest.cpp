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
 * ContractRequest.cpp
 *
 *  Created on: Mar 3, 2009
 *      Author: sorin.bidian
 */
#include "ContractRequest.h"

using namespace NE::Misc::Marshall;

namespace NE {
namespace Model {


ContractRequest::ContractRequest():
    contractRequestId(0),
    contractRequestType(ContractRequestType::ContractModification),
    contractId(0),
    communicationServiceType(CommunicationServiceType::NonPeriodic),
    sourceSAP(0),
    destinationSAP(0),
    contractNegotiability(ContractNegotiability::NegotiableAndRevocable),
    contractLife(0),
    contractPriority(ContractPriority::BestEffortQueued),
    payloadSize(0),
    reliability_And_PublishAutoRetransmit(0),
    requestedPeriod(0),
    requestedPhase(0),
    requestedDeadline(0),
    committedBurst(0),
    excessBurst(0),
    maxSendWindowSize(0),
    isManagement(false){
}


std::ostream& operator<<(std::ostream& stream, const ContractRequest& request){

      stream << std::hex
          << "reqId=" << (int)request.contractRequestId
          << ", reqType=" << (int)request.contractRequestType
          << ", ID=" << (int)request.contractId
          << ", type=" << CommunicationServiceType::toString(request.communicationServiceType)
          << ", Address=" << Address_toStream(request.sourceAddress) << "->" << Address_toStream(request.destinationAddress)
          << ", TSAP=" << (int)request.sourceSAP << "->" << (int)request.destinationSAP
          << ", negot=" << (int)request.contractNegotiability
          << ", life=" << (int)request.contractLife
          << ", priority=" << (int)request.contractPriority
          << ", payloadSize=" << (int)request.payloadSize
          << ", reliability" << (int)request.reliability_And_PublishAutoRetransmit;

      if (request.communicationServiceType == CommunicationServiceType::Periodic) {
          stream << ", Period=" << (int)request.requestedPeriod
                 << ", Phase=" << (int)request.requestedPhase
                 << ", Deadline=" << (int)request.requestedDeadline;
      }
      if (request.communicationServiceType == CommunicationServiceType::NonPeriodic) {
          stream << ", CBurst=" << (int)request.committedBurst
                 << ", EBurst=" << (int)request.excessBurst
                 << ", WindowSize=" << (int)request.maxSendWindowSize;
      }
      stream << ", isManagement=" << request.isManagement;

      return stream;
 }

}
}

