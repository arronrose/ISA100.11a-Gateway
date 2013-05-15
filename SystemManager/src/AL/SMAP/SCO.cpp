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
 *  @author ioan.pocol, catalin.pop, flori.parauan, radu.pop, Beniamin Tecar, sorin.bidian
 */
#include "SCO.h"
#include "Common/ClockSource.h"
#include "Common/MethodsIDs.h"
#include "Common/SmSettingsLogic.h"
#include "Misc/Marshall/NetworkOrderBytesWriter.h"
#include "Misc/Marshall/NetworkOrderStream.h"
#include "Model/Contracts/SmContractRequest.h"
#include "Model/Contracts/ContractTermination.h"
#include "Model/ContractsHelper.h"
#include "Model/Device.h"
#include "Common/HandleFactory.h"
#include "Model/EngineProvider.h"
#include "Model/ModelUtils.h"
#include "Stats/Cmds.h"
#include "AL/DMAP/NLMO.h"
#include "AL/DMAP/TLMO.h"
#include "ASL/PDUUtils.h"

#include <boost/bind.hpp>

using namespace NE::Model;
using namespace NE::Misc::Convert;
using namespace Isa100::ASL::Services;
using namespace Isa100::ASL;
using namespace Isa100::AL;
using namespace Isa100::Common;
using namespace Isa100::Common::Objects;
using namespace Isa100::Model;

namespace Isa100 {
namespace AL {
namespace SMAP {

SCO::SCO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine_) :
    Isa100Object(tsap, engine_) {

    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: created object:" << objectIdString);

}

SCO::~SCO() {

    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: destroyed object:" << objectIdString);
}

bool SCO::expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {


    return false;
}

void SCO::indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {

    ASL::PDU::ExecuteRequestPDUPointer executeRequest = PDUUtils::extractExecuteRequest(indication->apduRequest);

    if (executeRequest->methodID == Isa100::Common::MethodsID::SCOMethodID::Contract_Establishment_Modification_Renewal) {
        NetworkOrderStream stream(executeRequest->parameters);
        ContractRequestPointer currentContractRequest;

        try {
            currentContractRequest = SmContractRequest::unmarshall(stream);
        } catch (NE::Common::AddressNotFoundException& ex) {
            LOG_ERROR(LOG_OI << "Invalid ContractRequest. Address not found! contractRequest="
                        << bytes2string(*executeRequest->parameters));
            sendExecuteResponseToRequester(indication, SFC::deviceNotFound, BytesPointer(new Bytes()), true);
            return;
        } catch (std::exception& ex) {
            LOG_ERROR(LOG_OI << "Invalid ContractRequest. contractRequest=" << bytes2string(*executeRequest->parameters));
            sendExecuteResponseToRequester(indication, SFC::invalidData, BytesPointer(new Bytes()), true);
            return;
        }

        currentContractRequest->sourceAddress = engine->getAddress32(indication->clientNetworkAddress);

         contractRequest = *currentContractRequest;

        if (contractRequest.contractRequestType == ContractRequestType::NewContract) {
            createNewContract(contractRequest);
        } else if (contractRequest.contractRequestType == ContractRequestType::ContractModification) {
            modifyContract(contractRequest);
        } else if (contractRequest.contractRequestType == ContractRequestType::ContractRenewal) {
            contractRenewal(contractRequest);
        } else {
            sendExecuteResponseToRequester(indication, SFC::invalidArgument, BytesPointer(new Bytes()), true);

            std::ostringstream stream;
            stream << "Invalid contractRequestType: " << (int) contractRequest.contractRequestType;
            stream << ". Discarding packet.";
            LOG_ERROR(LOG_OI << stream.str());
            return;
        }
    } else if (executeRequest->methodID == Isa100::Common::MethodsID::SCOMethodID::Contract_Termination_Deactivation_Reactivation) {
        NetworkOrderStream stream(executeRequest->parameters);

        Uint16 contractTermID;
        Uint8 contractOperation;
        stream.read(contractTermID);
        stream.read(contractOperation);

        if (contractOperation == SCOContractOperation::Termination) {
            terminateContract(contractTermID);
        } else if (contractOperation == SCOContractOperation::Deactivation) {
            //contractDeactivation();
        } else if (contractOperation == SCOContractOperation::Reactivation) {
            //contractReactivation();
        } else {
            sendExecuteResponseToRequester(indication, SFC::invalidArgument, BytesPointer(new Bytes()), true);

            std::ostringstream stream;
            stream << "Invalid contractOperation: " << (int) contractOperation << ". Discarding packet.";
            LOG_ERROR(LOG_OI << stream.str());
            return;
        }
    } else {
        sendExecuteResponseToRequester(indication, SFC::invalidMethod, BytesPointer(new Bytes()), true);

        std::ostringstream stream;
        stream << "InvokeMethod: unknown method id: " << (int) executeRequest->methodID << ". Discarding packet.";
        LOG_ERROR(LOG_OI << stream.str());
    }
}

bool SCO::checkContractRequestConstraints(const ContractRequest& contractReq) {

    bool checkConstraints = true;

    if (!engine->existsConfirmedDevice(contractReq.sourceAddress)) {
        LOG_ERROR(LOG_OI << "The requested source for contract no longer exists. source=" << Address::toString(
                    contractReq.sourceAddress));
        jobFinished = true;
        checkConstraints = false;
    }

    if (!engine->existsConfirmedDevice(contractReq.destinationAddress)) {
        LOG_ERROR(LOG_OI << "The requested destination for contract no longer exists. destination=" << Address::toString(
                    contractReq.destinationAddress));
        sendExecuteResponseToRequester(indication, SFC::deviceNotFound, BytesPointer(new Bytes()), true);
        checkConstraints = false;
    } else {
        if (contractReq.sourceAddress == contractReq.destinationAddress) {

            LOG_ERROR(LOG_OI << "ContractRequest source and destination are the same! source=" << Address::toString(
                        contractReq.sourceAddress));
            sendExecuteResponseToRequester(indication, SFC::invalidParameter, BytesPointer(new Bytes()), true);
            checkConstraints = false;
        }
    }

    return checkConstraints;
}

void SCO::createNewContract(const ContractRequest& contractReq) {
    if (checkContractRequestConstraints(contractReq)) {
        engine->requestCreateContract( //
                    contractReq, //
                    (int) indication->apduRequest->appHandle, //
                    boost::bind(&SCO::responseCreateNewContract, this, _1, _2, _3));
    }
}

void SCO::responseCreateNewContract(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status) {

    LOG_DEBUG(LOG_OI << "Callback responseCreateNewContract - device:" << deviceAddress << " reqID=" << requestID << " status="
                << status);

    NE::Model::Device * device = engine->getDevice(deviceAddress);
    if (!device) {
        LOG_ERROR(LOG_OI << "Device " << Address::toString(deviceAddress) << " not found." );
        jobFinished = true;
        return;
    }

    if (status == ResponseStatus::REFUSED_INSUFICIENT_RESOURCES) {
        sendExecuteResponseToRequester(indication, SFC::insufficientDeviceResources, BytesPointer(new Bytes()), true);
        return;
    }

    if (status == ResponseStatus::INAPPROPRIATE_PROCESS_MODE) {
        sendExecuteResponseToRequester(indication, SFC::inappropriateProcessMode, BytesPointer(new Bytes()), true);
        return;
    }

    if (status == ResponseStatus::REQUEST_DISCARDED) {
        ContractResponse contractResponse;
        contractResponse.contractRequestID = requestID; //contractRequestId
        contractResponse.responseCode = ContractResponseCode::FailureWithNoFurtherGuidance;//4- failure without further guidance
        contractResponse.contractID = 0;
        contractResponse.communicationServiceType = CommunicationServiceType::Periodic;
        contractResponse.contract_Activation_Time = 0;
        contractResponse.assigned_Contract_Life = 0;
        contractResponse.assigned_Contract_Priority = ContractPriority::BestEffortQueued;
        contractResponse.assigned_Max_NSDU_Size = 0;
        contractResponse.assigned_Reliability_And_PublishAutoRetransmit = 0;
        contractResponse.assignedPeriod = 0;
        contractResponse.assignedPhase = 0;
        contractResponse.assignedDeadline = 0;
        contractResponse.assignedCommittedBurst = 0;
        contractResponse.assignedExcessBurst = 0;
        contractResponse.assigned_Max_Send_Window_Size = 0;

        NetworkOrderStream streamResponse;
        contractResponse.marshall(streamResponse);

        sendExecuteResponseToRequester(indication, SFC::objectAccessDenied, BytesPointer(new Bytes(streamResponse.ostream.str())), true);


        return;
    }

    if (status != ResponseStatus::SUCCESS) {
        sendExecuteResponseToRequester(indication, SFC::failure, BytesPointer(new Bytes()), true);
        return;
    }


    NE::Model::PhyContract * newContract =
        ContractsHelper::findContractSource2Destination(device, contractRequest.destinationAddress,
                contractRequest.sourceSAP, contractRequest.destinationSAP, contractRequest.contractRequestId,
                contractRequest.communicationServiceType);

    if (!newContract) {
        LOG_ERROR(LOG_OI<< "Contract with contractRequestId=" << (int) contractRequest.contractRequestId << " on device "
                    << Address::toString(deviceAddress) << " - not found." );
        return;
    }

    ContractResponse contractResponse;
    contractResponse.contractRequestID = newContract->requestID; //contractRequestId
    contractResponse.responseCode = newContract->responseCode;
    contractResponse.contractID = newContract->contractID;
    contractResponse.communicationServiceType = newContract->communicationServiceType;
    contractResponse.contract_Activation_Time = newContract->contract_Activation_Time;
    contractResponse.assigned_Contract_Life = newContract->assigned_Contract_Life;
    contractResponse.assigned_Contract_Priority = newContract->assigned_Contract_Priority;
    contractResponse.assigned_Max_NSDU_Size = newContract->assigned_Max_NSDU_Size;
    contractResponse.assigned_Reliability_And_PublishAutoRetransmit = newContract->assigned_Reliability_And_PublishAutoRetransmit;
    contractResponse.assignedPeriod = newContract->assignedPeriod;
    contractResponse.assignedPhase = newContract->assignedPhase;
    contractResponse.assignedDeadline = newContract->assignedDeadline;
    contractResponse.assignedCommittedBurst = newContract->assignedCommittedBurst;
    contractResponse.assignedExcessBurst = newContract->assignedExcessBurst;
    contractResponse.assigned_Max_Send_Window_Size = newContract->assigned_Max_Send_Window_Size;

    NetworkOrderStream streamResponse;
    contractResponse.marshall(streamResponse);

    sendExecuteResponseToRequester(indication, SFC::success, BytesPointer(new Bytes(streamResponse.ostream.str())), true);
}

void SCO::modifyContract(const ContractRequest& contractReq) {

    if (checkContractRequestConstraints(contractReq)) {
        engine->modifyContract( //
                    contractReq, //
                    (int) indication->apduRequest->appHandle, //
                    boost::bind(&SCO::responseCreateNewContract, this, _1, _2, _3));
    }
}

void SCO::contractRenewal(const ContractRequest& contractReq) {
    if (checkContractRequestConstraints(contractReq)) {
        engine->contractRenewal( //
                    contractReq, //
                    (int) indication->apduRequest->appHandle, //
                    boost::bind(&SCO::responseCreateNewContract, this, _1, _2, _3));
    }
}

void SCO::terminateContract(const Uint16 contractID) {

    	Address32 clientAddress32 =  engine->getAddress32(indication->clientNetworkAddress);
        LOG_INFO(LOG_OI << "contract termination: source=" << Address_toStream(clientAddress32)
                    << ", contractID=" << (int)contractID);

        if (!engine->existsConfirmedDevice(clientAddress32)) {
            LOG_ERROR(LOG_OI << "The requested source for contract no longer exists. source=" << clientAddress32);
            jobFinished = true;
            return;
        }

        if (!engine->existsContract(clientAddress32, contractID)) {
            LOG_ERROR(LOG_OI << "Could not find contract for termination; device: " << Address_toStream(clientAddress32)
                        << ", contract: " << (int)contractID);
            sendResponseToRequesterOfContractTermination(SFC::invalidArgument);
        	return;
        }

        sendExecuteResponseToRequester(indication, SFC::success, BytesPointer(new Bytes()), true);
   		engine->terminateContract(engine->getAddress32(indication->clientNetworkAddress), contractID);
}

void SCO::sendResponseToRequesterOfContractTermination(SFC::SFCEnum sfc) {

    LOG_DEBUG(LOG_OI << "sendResponseToRequesterOfContractTermination: SFC::ServiceFeedbackCode=" << (int) sfc);

    sendExecuteResponseToRequester(indication, sfc, BytesPointer(new Bytes()), false);

    jobFinished = true;
}

}
}
}
