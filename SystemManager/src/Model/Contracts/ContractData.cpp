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
 * ContractData.cpp
 *
 *  Created on: Mar 4, 2009
 *      Author: sorin.bidian
 */

#include "ContractData.h"

using namespace NE::Model;

namespace Isa100 {
namespace Model {

ContractData::ContractData() {
    contractID = 0;
    contractStatus = ContractStatus::SuccessWithDelayedEffect;
    serviceType = CommunicationServiceType::NonPeriodic;
    contractActivationTime = 0;
    sourceSAP = 0;
    // destinationAddress
    destinationSAP = 0;
    assigned_Contract_Life = 0;
    assigned_Contract_Priority = ContractPriority::BestEffortQueued;
    assigned_Max_NSDU_Size = 0;
    assigned_Reliability_And_PublishAutoRetransmit = 0;
    assignedPeriod = 0;
    assignedPhase = 0;
    assignedDeadline = 0;
    assignedCommittedBurst = 0;
    assignedExcessBurst = 0;
    assigned_Max_Send_Window_Size = 0;
}

void ContractData::marshall(OutputStream& stream) {
    stream.write(contractID);
    stream.write((Uint8)contractStatus);
    stream.write((Uint8)serviceType);
    stream.write(contractActivationTime);
    stream.write(sourceSAP);
    destinationAddress.marshall(stream);
    stream.write(destinationSAP);
    stream.write(assigned_Contract_Life);
    stream.write((Uint8)assigned_Contract_Priority);
    stream.write(assigned_Max_NSDU_Size);
    stream.write(assigned_Reliability_And_PublishAutoRetransmit);
    if (serviceType == CommunicationServiceType::Periodic) {
        stream.write(assignedPeriod);
        stream.write(assignedPhase);
        stream.write(assignedDeadline);
    } else if (serviceType == CommunicationServiceType::NonPeriodic) {
        stream.write(assignedCommittedBurst);
        stream.write(assignedExcessBurst);
        stream.write(assigned_Max_Send_Window_Size);
    }
}

void ContractData::unmarshall(InputStream& stream) {
    stream.read(contractID);

    Uint8 contractStatus2;
    stream.read(contractStatus2);
    contractStatus = (ContractStatus::ContractStatus)contractStatus2;

    Uint8 serviceType2;
    stream.read(serviceType2);
    serviceType = (CommunicationServiceType::CommunicationServiceType)serviceType2;

    stream.read(contractActivationTime);

    stream.read(sourceSAP);
    destinationAddress.unmarshall(stream);
    stream.read(destinationSAP);
    stream.read(assigned_Contract_Life);

    Uint8 assigned_Contract_Priority2;
    stream.read(assigned_Contract_Priority2);
    assigned_Contract_Priority = (ContractPriority::ContractPriority)assigned_Contract_Priority2;

    stream.read(assigned_Max_NSDU_Size);
    stream.read(assigned_Reliability_And_PublishAutoRetransmit);
    if (serviceType == CommunicationServiceType::Periodic) {
        stream.read(assignedPeriod);
        stream.read(assignedPhase);
        stream.read(assignedDeadline);
    } else if (serviceType == CommunicationServiceType::NonPeriodic) {
        stream.read(assignedCommittedBurst);
        stream.read(assignedExcessBurst);
        stream.read(assigned_Max_Send_Window_Size);
    }
}

std::string ContractData::toString() {
     std::ostringstream stream;
     std::string lifeString;
     Type::toString(assigned_Contract_Life, lifeString);
     stream << ", contractID=" << (int)contractID
         << ", contractStatus=" << (int)contractStatus
         << ", serviceType=" << (int)serviceType
         << ", contractActivationTime=" << contractActivationTime
         << ", sourceSAP=" << (int)sourceSAP
         << ", destinationAddress128=" << destinationAddress.toString()
         << ", destinationSAP=" << (int)destinationSAP
         << ", assigned_Contract_Life=" << lifeString
         << ", assigned_Contract_Priority=" << (int)assigned_Contract_Priority
         << ", assigned_Max_NSDU_Size=" << (int)assigned_Max_NSDU_Size
         << ", assigned_Reliability_And_PublishAutoRetransmit=" << (int)assigned_Reliability_And_PublishAutoRetransmit;
     if (serviceType == CommunicationServiceType::Periodic) {
         stream << ", assignedPeriod=" << (int)assignedPeriod
             << ", assignedPhase=" << (int)assignedPhase
             << ", assignedDeadline=" << (int)assignedDeadline;
     } else if (serviceType == CommunicationServiceType::NonPeriodic) {
         stream << ", assignedCommittedBurst=" << (int)assignedCommittedBurst
             << ", assignedExcessBurst=" << (int)assignedExcessBurst
             << ", assigned_Max_Send_Window_Size=" << (int)assigned_Max_Send_Window_Size;
     }
     return stream.str();
 }

}
}

