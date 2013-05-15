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
 * ContractResponse.cpp
 *
 *  Created on: Mar 3, 2009
 *      Author: sorin.bidian
 */

#include "ContractResponse.h"
#include "Common/NEException.h"

using namespace NE::Misc::Marshall;

namespace Isa100 {
namespace Model {

ContractResponse::ContractResponse() {
    contractRequestID = 0;
    responseCode = ContractResponseCode::FailureWithNoFurtherGuidance;
    contractID = 0;
    communicationServiceType = CommunicationServiceType::NonPeriodic;
    contract_Activation_Time = 0;
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
    retry_Backoff_Time = 0;
    negotiationGuidance = ContractNegotiability::NegotiableAndRevocable;
    supportable_Contract_Priority = ContractPriority::BestEffortQueued;
    supportable_max_NSDU_Size = 70;
    supportable_Reliability_And_PublishAutoRetransmit = 0;
    supportable_Period = 0;
    supportablePhase = 0;
    supportableDeadline = 0;
    supportable_Committed_Burst = 0;
    supportable_Excess_Burst = 0;
    supportable_Max_Send_Window_Size = 0;

    //isManagement = false;
}

void ContractResponse::marshall(OutputStream& stream) {
    stream.write(contractRequestID);
    stream.write((Uint8)responseCode);

    if (responseCode > 6) {
        std::ostringstream stream;
        stream << "ContractResponse- unknown response code: " << (int)responseCode;
        //LOG_ERROR(stream.str());
        throw NE::Common::NEException(stream.str());
    }

    if ((responseCode != 4) && (responseCode != 5)) {
        if (responseCode != 6) {
            stream.write(contractID);
        }
        stream.write((Uint8)communicationServiceType);
        if ((responseCode != 0) && (responseCode != 2) && (responseCode != 6)) {
            //contract_Activation_Time.marshall(stream);
            stream.write(contract_Activation_Time);
        }
        if (responseCode != 6) {
            stream.write(assigned_Contract_Life);
            stream.write((Uint8)assigned_Contract_Priority);
            stream.write(assigned_Max_NSDU_Size);
            stream.write(assigned_Reliability_And_PublishAutoRetransmit);

            if (communicationServiceType == CommunicationServiceType::Periodic) {
                stream.write(assignedPeriod);
                stream.write(assignedPhase);
                stream.write(assignedDeadline);
            }

            if (communicationServiceType == CommunicationServiceType::NonPeriodic) {
                stream.write(assignedCommittedBurst);
                stream.write(assignedExcessBurst);
                stream.write(assigned_Max_Send_Window_Size);
            }
        }
    }

    if (responseCode > 4 ) {
        stream.write(retry_Backoff_Time);
        if (responseCode != 5) {
            stream.write((Uint8)negotiationGuidance);
            stream.write((Uint8)supportable_Contract_Priority);
            stream.write(supportable_max_NSDU_Size);
            stream.write(supportable_Reliability_And_PublishAutoRetransmit);

            if (communicationServiceType == CommunicationServiceType::Periodic) {
                stream.write(supportable_Period);
                stream.write(supportablePhase);
                stream.write(supportableDeadline);
            }

            if (communicationServiceType == CommunicationServiceType::NonPeriodic) {
                stream.write(supportable_Committed_Burst);
                stream.write(supportable_Excess_Burst);
                stream.write(supportable_Max_Send_Window_Size);
            }
        }
    }
}

void ContractResponse::unmarshall(InputStream& stream) {
    stream.read(contractRequestID);

    Uint8 responseCode2;
    stream.read(responseCode2);
    responseCode = (ContractResponseCode::ContractResponseCode)responseCode2;

    if (responseCode > 6) {
        std::ostringstream stream;
        stream << "ContractResponse- unknown response code: " << (int)responseCode;
        // LOG_ERROR(stream.str());
        throw NE::Common::NEException(stream.str());
    }

    if ((responseCode != 4) && (responseCode != 5)) {
        if (responseCode != 6) {
            stream.read(contractID);
        }
        Uint8 contractServiceType2;
        stream.read(contractServiceType2);
        communicationServiceType = (CommunicationServiceType::CommunicationServiceType)contractServiceType2;
        if ((responseCode != 0) && (responseCode != 2) && (responseCode != 6)) {
            //contract_Activation_Time.unmarshall(stream);
            stream.read(contract_Activation_Time);
        }
        if (responseCode != 6) {
            stream.read(assigned_Contract_Life);

            Uint8 assigned_Contract_Priority2;
            stream.read(assigned_Contract_Priority2);
            assigned_Contract_Priority = (ContractPriority::ContractPriority)assigned_Contract_Priority2;

            stream.read(assigned_Max_NSDU_Size);
            stream.read(assigned_Reliability_And_PublishAutoRetransmit);

            if (communicationServiceType == CommunicationServiceType::Periodic) {
                stream.read(assignedPeriod);
                stream.read(assignedPhase);
                stream.read(assignedDeadline);
            }

            if (communicationServiceType == CommunicationServiceType::NonPeriodic) {
                stream.read(assignedCommittedBurst);
                stream.read(assignedExcessBurst);
                stream.read(assigned_Max_Send_Window_Size);
            }
        }
    }

    if (responseCode > 4 ) { // case of failure response code
        stream.read(retry_Backoff_Time);
        if (responseCode != 5) {
            Uint8 negotiationGuidance2;
            stream.read(negotiationGuidance2);
            negotiationGuidance = (ContractNegotiability::ContractNegotiability)negotiationGuidance2;

            Uint8 supportable_Contract_Priority2;
            stream.read(supportable_Contract_Priority2);
            supportable_Contract_Priority = (ContractPriority::ContractPriority)supportable_Contract_Priority2;

            stream.read(supportable_max_NSDU_Size);
            stream.read(supportable_Reliability_And_PublishAutoRetransmit);

            if (communicationServiceType == CommunicationServiceType::Periodic) {
                stream.read(supportable_Period);
                stream.read(supportablePhase);
                stream.read(supportableDeadline);
            }

            if (communicationServiceType == CommunicationServiceType::NonPeriodic) {
                stream.read(supportable_Committed_Burst);
                stream.read(supportable_Excess_Burst);
                stream.read(supportable_Max_Send_Window_Size);
            }
        }
    }
}

 std::string ContractResponse::toString() {
     std::ostringstream stream;
     stream << "contractRequestID=" << (int)contractRequestID
         << ", responseCode=" << (int)responseCode;
     if ((responseCode != 4) && (responseCode != 5)) {
         stream << ", ServiceType=" << (int)communicationServiceType;
         if (responseCode != 6) {
             stream << ", contractID=" << (int)contractID;
             if ((responseCode != 0) && (responseCode != 2)) {
                 //stream << ", Activation_Time=" << contract_Activation_Time.toString();
                 stream << ", Activation_Time=" << (int)contract_Activation_Time;
             }
             std::string lifeString;
             Type::toString(assigned_Contract_Life, lifeString);
             stream << ", Contract_Life=" << lifeString
                 << ", Contract_Priority=" << (int)assigned_Contract_Priority
                 << ", Max_NSDU_Size=" << (int)assigned_Max_NSDU_Size
                 << ", Reliability_And_PublishAutoRetransmit=" << (int)assigned_Reliability_And_PublishAutoRetransmit;
             if (communicationServiceType == CommunicationServiceType::Periodic) {
                 stream << ", Period=" << (int)assignedPeriod
                        << ", Phase=" << (int)assignedPhase
                        << ", Deadline=" << (int)assignedDeadline;
             }
             if (communicationServiceType == CommunicationServiceType::NonPeriodic) {
                 stream << ", CommittedBurst=" << (int)assignedCommittedBurst
                        << ", ExcessBurst=" << (int)assignedExcessBurst
                        << ", Max_Send_Window_Size=" << (int)assigned_Max_Send_Window_Size;
             }
         }
     }
     if (responseCode > 4) {
         stream << ", retry_Backoff_Time=" << (int)retry_Backoff_Time;
         if (responseCode > 5) {
             stream << ", negotiationGuidance=" << (int)negotiationGuidance
                 << ", supportable_Contract_Priority=" << (int)supportable_Contract_Priority
                 << ", supportable_max_NSDU_Size=" << (int)supportable_max_NSDU_Size
                 << ", supportable_Reliability=" << (int)supportable_Reliability_And_PublishAutoRetransmit;
             if (communicationServiceType == CommunicationServiceType::Periodic) {
                 stream << ", supportable_Period=" << (int)supportable_Period
                        << ", supportablePhase=" << (int)supportablePhase
                        << ", supportableDeadline=" << (int)supportableDeadline;
             }
             if (communicationServiceType == CommunicationServiceType::NonPeriodic) {
                 stream << ", supportable_Committed_Burst=" << (int)supportable_Committed_Burst
                        << ", supportable_Excess_Burst=" << (int)supportable_Excess_Burst
                        << ", supportable_Max_Send_Window_Size=" << (int)supportable_Max_Send_Window_Size;
             }
         }
     }

     //stream <<", isManagement=" << isManagement;

     return stream.str();
 }

}
}
