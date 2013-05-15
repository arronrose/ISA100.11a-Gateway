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
 * @author radu.pop, sorin.bidian, beniamin.tecar, catalin.pop
 */
#include "DMSO.h"
#include "AL/ObjectsIDs.h"
#include "ASL/PDUUtils.h"
#include "ASL/PDU/ExecuteRequestPDU.h"
#include "Common/ClockSource.h"
#include "Common/NEAddress.h"
#include "Common/MethodsIDs.h"
#include "Common/SmSettingsLogic.h"
#include "Misc/Marshall/NetworkOrderStream.h"
#include "Model/Capabilities.h"
#include "Model/EngineProvider.h"
#include "Model/ModelUtils.h"
#include "Model/Contracts/NewDeviceContractResponse.h"
#include "SMState/SMStateLog.h"
#include "AL/Types/AlertTypes.h"

#include <boost/bind.hpp>

using namespace Isa100::ASL::Services;
using namespace Isa100::ASL;
using namespace Isa100::AL;
using namespace Isa100::Common::Objects;
using namespace Isa100::Common;
using namespace NE::Misc::Marshall;
using namespace Isa100::Model;
using namespace NE::Model::Operations;

namespace Isa100 {
namespace AL {
namespace SMAP {

DMSO::DMSO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine_) :
    Isa100Object(tsap, engine_) {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: created object:" << objectIdString);

    expectingProxySecurityJoinResponse = false;

    joinStartTime = 0;
}

DMSO::~DMSO() {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: destroyed object:" << objectIdString);
}

void DMSO::execute(Uint32 currentTime) {
}

void DMSO::indicateExecute(PrimitiveIndicationPointer indication) {

    LOG_DEBUG(LOG_OI << "indicateExecute() : " + indication->toString());

    ASL::PDU::ExecuteRequestPDUPointer executeRequest = PDUUtils::extractExecuteRequest(indication->apduRequest);

    if (executeRequest->methodID == Isa100::Common::MethodsID::DMSOMethodID::System_Manager_Join) {
        processSystemManagerJoinRequest(indication, executeRequest);
    } else if (executeRequest->methodID == Isa100::Common::MethodsID::DMSOMethodID::System_Manager_Contract) {
        processSystemManagerContractRequest(indication, executeRequest);
        //    } else if (executeRequest->methodID == Isa100::Common::MethodsID::DMSOMethodID::Proxy_Security_Sym_Join) {
        //        processSecuritySymJoinRequest(indication, executeRequest);
    } else {
        LOG_ERROR(LOG_OI << "InvokeMethod: unknown method id: " << (int) executeRequest->methodID << "Discarding packet");
        sendExecuteResponseToRequester(indication, SFC::invalidMethod, BytesPointer(new Bytes()), true);
    }
}

void DMSO::confirmExecute(PrimitiveConfirmationPointer confirmation) {
    LOG_DEBUG(LOG_OI << "confirmExecute() : Confirmation received in DMO from " << confirmation->serverNetworkAddress.toString());

    if (confirmation->apduResponse->appHandle == indication->apduRequest->appHandle) {
        if (expectingProxySecurityJoinResponse) {
            forwardSecurityJoinResponse(confirmation);
            expectingProxySecurityJoinResponse = false;
        } else {
            LOG_WARN(LOG_OI << "confirmExecute() : Invalid confirmExecute!");
        }
    }
}

void DMSO::processSystemManagerJoinRequest(PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer executeRequest) {

    LOG_DEBUG(LOG_OI << "processSecuritySymJoinRequest...PARAMS=" << bytes2string(*executeRequest->parameters));

    // parse SmJoinRequest
    NetworkOrderStream stream(executeRequest->parameters);
    smJoinRequest.unmarshall(stream);

    LOG_INFO(LOG_OI << "DMSO JOIN REQUEST :" << smJoinRequest);

    LOG_DEBUG("DevCapability: chMap=" << smJoinRequest.deviceCapability.channelMap <<
				", dlRoles=" << (int) smJoinRequest.deviceCapability.dlRoles <<
				", energyLeft=" << smJoinRequest.deviceCapability.energyLeft <<
				", neiDiagCapacity=" << smJoinRequest.deviceCapability.neighborDiagCapacity);

    // check if this address is provisioned
    //    try {
    SmSettingsLogic& smSettings = Isa100::Common::SmSettingsLogic::instance();
    ProvisioningItem * provisioningItem = smSettings.getProvisioningForDevice(smJoinRequest.capabilities.euidAddress);

    if (!provisioningItem) {
        LOG_ERROR(LOG_OI << "The device " << smJoinRequest.capabilities.euidAddress.toString()
                    << " is not provisioned. Join refused!");
        jobFinished = true;
        return;
    }

    if (provisioningItem->subnetId != smJoinRequest.capabilities.dllSubnetId) {
        LOG_ERROR(LOG_OI << "Device " << smJoinRequest.capabilities.euidAddress.toString() << " provisioned in subnet: "
                    << provisioningItem->subnetId << " but tries to join in subnet: " << smJoinRequest.capabilities.dllSubnetId
                    << ". Join refused.");

        //send join failure alert
        //LOG_INFO("[SORIN] alert join subnet_incorrectly_provisioned");
        Uint32 currentTAI = ClockSource::getTAI(engine->getSettingsLogic());
        AlertJoinFailed *alertJoinFailed = new AlertJoinFailed(smJoinRequest.capabilities.euidAddress,
                                                                (Uint8) StatusForReports::SM_JOIN_RECEIVED,
                                                                (Uint8) JoinFailureReason::subnet_provisioning_mismatch);
        AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoinFailed, alertJoinFailed, currentTAI));
        engine->getOperationsProcessor().sendAlert(alertOperation);

        jobFinished = true;
        return;
    }
    Address32 parentAddress32 = engine->getAddress32(indication->clientNetworkAddress);

    engine->requestJoinSystemManager( //
                smJoinRequest.capabilities.euidAddress, //
                parentAddress32, //
                (int) indication->apduRequest->appHandle, //
                smJoinRequest.capabilities, //standard join request capabilities
                smJoinRequest.softwareRevisionInformation,
                smJoinRequest.deviceCapability, //dlmo.18 capability, custom added on join
                boost::bind(&DMSO::responseJoinSystemManager, this, _1, _2, _3));
}

void DMSO::responseJoinSystemManager(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status) {
    LOG_DEBUG(LOG_OI << "Callback responseJoinSystemManager - device:" << deviceAddress << " reqID=" << requestID << " status="
                << status);
    if (status == ResponseStatus::REQUEST_DISCARDED) {//discard and DO not respond
        jobFinished = true;
        return;
    } else if (status == ResponseStatus::REFUSED_INSUFICIENT_RESOURCES) {
        //send join failure alert
        //LOG_INFO("[SORIN] alert insufficient resources");
        Uint32 currentTAI = ClockSource::getTAI(engine->getSettingsLogic());
        AlertJoinFailed *alertJoinFailed = new AlertJoinFailed(smJoinRequest.capabilities.euidAddress,
                                                                (Uint8) StatusForReports::SM_JOIN_RECEIVED,
                                                                (Uint8) JoinFailureReason::insufficient_parent_resources);
        AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoinFailed, alertJoinFailed, currentTAI));
        engine->getOperationsProcessor().sendAlert(alertOperation);

        sendExecuteResponseToRequester(indication, Isa100::Common::Objects::SFC::insufficientDeviceResources,
                    BytesPointer(new Bytes()), true);
        return;
    }

    BytesPointer joinResponsePayload(new Bytes());
    SFC::SFCEnum sfc = SFC::success;

    if (status == ResponseStatus::SUCCESS) {
        NE::Model::Device * device = engine->getDevice(deviceAddress);
        if (!device) {
            LOG_ERROR(LOG_OI << "Device " << Address::toString(deviceAddress) << " not found." );
            return;
        }

        SmJoinResponse smJoinResponse;
        smJoinResponse.deviceAddress128 = device->address128;
        smJoinResponse.deviceAddress16 = Address::getAddress16(device->address32);
        smJoinResponse.deviceType = smJoinRequest.capabilities.deviceType;
        smJoinResponse.systemManagerAddress128 = SmSettingsLogic::instance().managerAddress128;

        Address32 managerAddress32 = engine->getManagerAddress32();
        smJoinResponse.managerAddress16 = engine->getAddress16(managerAddress32);
        smJoinResponse.systemManagerAddress64 = SmSettingsLogic::instance().managerAddress64;

        NE::Misc::Marshall::NetworkOrderStream outStream;
        smJoinResponse.marshall(outStream);
        joinResponsePayload.reset(new Bytes(outStream.ostream.str()));

    } else {
        sfc = Isa100::Common::Objects::SFC::failure;
    }
    if (indication == NULL){
        LOG_ERROR(LOG_OI << "Indication is null. Trying to respond: " << (int) sfc);
        return;
    }
    sendExecuteResponseToRequester(indication, sfc, joinResponsePayload, true);
}

void DMSO::processSystemManagerContractRequest(PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer executeRequest) {

    LOG_DEBUG(LOG_OI << "processSystemManagerContractRequest() : PARAMS=" << bytes2string(*executeRequest->parameters));

    Isa100::Model::SmJoinContractRequest smContractRequest;
    NetworkOrderStream stream(executeRequest->parameters);
    smContractRequest.unmarshall(stream);

    LOG_INFO(LOG_OI << " DMSO CONTRACT REQUEST from:" << smContractRequest.euidAddress.toString());

    Address32 parentAddress32 = engine->getAddress32(indication->clientNetworkAddress);

    engine->requestJoinContract( //
                smContractRequest.euidAddress, //
                parentAddress32, //
                (int) indication->apduRequest->appHandle, //
                boost::bind(&DMSO::responseJoinContract, this, _1, _2, _3));
}

void DMSO::responseJoinContract(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status) {
    LOG_DEBUG(LOG_OI << "Callback responseJoinContract - device:" << Address::toString(deviceAddress) << " reqID=" << requestID
                << " status=" << status);

    if (status == ResponseStatus::REQUEST_DISCARDED){//discard and do not respond
        LOG_INFO(LOG_OI << " requestID " << requestID << " discarded!");
        jobFinished = true;
        return;
    }

    BytesPointer joinResponsePayload(new Bytes());
    SFC::SFCEnum sfc = SFC::success;

    NE::Model::Device * device = engine->getDevice(deviceAddress);
    if (!device) {
        std::ostringstream stream;
        stream << "Device " << Address::toString(deviceAddress) << " not found.";
        LOG_ERROR(LOG_OI << stream.str());
        throw NEException(stream.str());
    }

    if (status == ResponseStatus::SUCCESS) {

        NewDeviceContractResponse newDeviceContractResponse;

        //get the management contract from device
        NE::Model::PhyContract * managementContract = NE::Model::ModelUtils::findManagementContract(device);
        if (!managementContract) {
            LOG_ERROR(LOG_OI << "Management contract on device " << Address::toString(deviceAddress) << " not found." );
            return;
        }

        newDeviceContractResponse.contractID = managementContract->contractID;
        newDeviceContractResponse.assignedMaxNSDUSize = managementContract->assigned_Max_NSDU_Size;
        newDeviceContractResponse.assignedCommittedBurst = managementContract->assignedCommittedBurst;
        newDeviceContractResponse.assignedExcessBurst = managementContract->assignedExcessBurst;
        newDeviceContractResponse.assignedMaxSendWindowSize = managementContract->assigned_Max_Send_Window_Size;

        //get the network route with manager from device
        NE::Model::PhyNetworkRoute * networkRouteToManager = NE::Model::ModelUtils::findNetworkRouteWithManager(device,
                    SmSettingsLogic::instance().managerAddress128);
        if (!networkRouteToManager) {
            LOG_ERROR(LOG_OI << "Network route with manager on device " << Address::toString(deviceAddress) << " not found." );
            return;
        }

        newDeviceContractResponse.nlNextHop = networkRouteToManager->nextHop;
        newDeviceContractResponse.nlHopsLimit = networkRouteToManager->nwkHopLimit;
        newDeviceContractResponse.nlOutgoingInterface = networkRouteToManager->outgoingInterface;
        //newDeviceContractResponse.nlHeaderIncludeContractFlag = 0;

        NE::Model::PhyNetworkContract* phyNetworkContract = NE::Model::ModelUtils::findNetworkContract(device,
                    managementContract->contractID);
        if (!phyNetworkContract) {
            LOG_ERROR(LOG_OI << "Network contract" << (int) managementContract->contractID << " on device " << Address::toString(
                        deviceAddress) << ", not found." );
            return;
        }

        newDeviceContractResponse.nlHeaderIncludeContractFlag = phyNetworkContract->include_Contract_Flag;

        LOG_DEBUG(LOG_OI << "newDeviceContractResponse= " << newDeviceContractResponse.toString());

        NE::Misc::Marshall::NetworkOrderStream outStream;
        newDeviceContractResponse.marshall(outStream);
        joinResponsePayload.reset(new Bytes(outStream.ostream.str()));

    } else {
        sfc = Isa100::Common::Objects::SFC::failure;

    }

    sendExecuteResponseToRequester(indication, sfc, joinResponsePayload, true);
}

void DMSO::forwardSecurityJoinResponse(PrimitiveConfirmationPointer confirmation) {

    ExecuteResponsePDUPointer responsePDU = PDUUtils::extractExecuteResponse(confirmation->apduResponse);

    // if there is an error don't send the response anymore - we have no contract to do it
    if (responsePDU->feedbackCode != SFC::success) {
        LOG_WARN(LOG_OI << "forwardSecurityJoinResponse() - received a response with sfc=" << (int) responsePDU->feedbackCode
                    << " client=" << indication->clientNetworkAddress.toString() << ". Dropping it..");
        return;
    }

    ClientServerPDUPointer apdu(new ClientServerPDU( //
                Isa100::Common::PrimitiveType::response, //
                Isa100::Common::ServiceType::execute, //
                indication->serverObject, //
                indication->clientObject, //
                indication->apduRequest->appHandle));

    LOG_DEBUG(LOG_OI << "forwardSecurityJoinResponse() : to client=" << indication->clientNetworkAddress.toString());

    ClientServerPDUPointer apduResponse = Isa100::ASL::PDUUtils::appendExecuteResponse(apdu, responsePDU);

    Address32 destAddress32 = engine->getAddress32(indication->clientNetworkAddress);
    NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(destAddress32,
                TSAP::TSAP_DMAP, TSAP::TSAP_DMAP);

    if (!contract_SM_Dev) {
        std::ostringstream stream;
        stream << "Contract DMAP-DMAP between manager and " << Address::toString(destAddress32) << " not found.";
        LOG_ERROR(LOG_OI << stream.str());
        throw NEException(stream.str());
    }

    PrimitiveResponsePointer primitiveResponse(new PrimitiveResponse( //
                contract_SM_Dev->contractID, //
                destAddress32, //
                ServicePriority::high, //
                false, //
                indication->forwardCongestionNotification, //fwdNotifEcho
                indication->serverTSAP_ID, //serverTSAPID
                indication->serverObject, //serverObj
                indication->clientTSAP_ID,//clientTSAPID
                indication->clientObject,//clientObj
                apduResponse, //apdu
                false//true // force unencrypted
                ));

    messageDispatcher->Response(primitiveResponse);
    jobFinished = true;
}

}
}
}
