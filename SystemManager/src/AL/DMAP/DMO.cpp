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
 * @author  Radu Pop, sorin.bidian, beniamin.tecar
 */
#include <fstream>
#include <stdio.h>
#include <vector>
#include <boost/bind.hpp>

#include "DMO.h"
#include "AL/InvalidALpayloadException.h"
#include "AL/DMAP/NLMO.h"
#include "ASL/PDUUtils.h"
#include "Common/NEAddress.h"
#include "Common/MethodsIDs.h"
#include "Common/SmSettingsLogic.h"
#include "Common/Utils/ContractUtils.h"
#include "Misc/Marshall/NetworkOrderStream.h"
#include "Model/EngineProvider.h"
#include "Security/SecuritySymJoinRequest.h"
#include "Model/SmJoinResponse.h"
#include "Model/SmJoinContractRequest.h"
#include "Model/Contracts/NewDeviceContractResponse.h"
#include "Model/ModelUtils.h"
#include "AL/Types/AlertTypes.h"

using namespace Isa100::ASL;
using namespace Isa100::ASL::Services;
using namespace Isa100::Common;
using namespace Isa100::Common::Objects;
using namespace Isa100::Common::MethodsID;
using namespace Isa100::Model;
using namespace NE::Model::Operations;

namespace Isa100 {
namespace AL {
namespace DMAP {

DMO::DMO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine_) :
    Isa100Object(tsap, engine_) {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: created object:" << objectIdString);

    expectingProxySecurityJoinResponse = false;
    expectingJoinResponse = false;
    expectingContractJoinResponse = false;
}

DMO::~DMO() {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: destroyed object:" << objectIdString);
}

bool DMO::expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    return false;

}

bool DMO::expectConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {

    if (expectingProxySecurityJoinResponse /*|| expectingJoinResponse || expectingContractJoinResponse*/) {
        return confirm->apduResponse->appHandle == indication->apduRequest->appHandle;
    }

    return false;
}

void DMO::execute(Uint32 currentTime) {

}

void DMO::indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {

    LOG_DEBUG(LOG_OI << "indicateExecute() : " + indication->toString());

    ASL::PDU::ExecuteRequestPDUPointer executeRequest = PDUUtils::extractExecuteRequest(indication->apduRequest);

    if (executeRequest->methodID == Isa100::Common::MethodsID::DMOMethodID::Proxy_System_Manager_Join) {
        proxySystemManagerJoin(indication, executeRequest);
    } else if (executeRequest->methodID == Isa100::Common::MethodsID::DMOMethodID::Proxy_System_Manager_Contract) {
        proxySystemManagerContract(indication, executeRequest);
    } else if (executeRequest->methodID == Isa100::Common::MethodsID::DMOMethodID::Proxy_Security_Sym_Join) {
        proxySecuritySymJoin(indication, executeRequest);
    } else {
        LOG_ERROR(LOG_OI << "InvokeMethod: unknown method id: " << (int) executeRequest->methodID << "Discarding packet");
        sendExecuteResponseToRequester(indication, SFC::invalidMethod, BytesPointer(new Bytes()), true);
    }
}

void DMO::proxySystemManagerJoin(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer executeRequest) {

    LOG_DEBUG(LOG_OI << "proxySystemManagerJoin...PARAMS=" << bytes2string(*executeRequest->parameters));

    // parse SmJoinRequest
    NetworkOrderStream stream(executeRequest->parameters);
    smJoinRequest.unmarshall(stream);

    LOG_INFO(LOG_OI << "JOIN REQUEST from:" << smJoinRequest.capabilities);

    LOG_INFO(" DeviceCapability: chMap=" << smJoinRequest.deviceCapability.channelMap <<
    		", dlRoles=" << (int) smJoinRequest.deviceCapability.dlRoles <<
    		", energyLeft=" << smJoinRequest.deviceCapability.energyLeft <<
    		", neighborDiagCapacity=" << smJoinRequest.deviceCapability.neighborDiagCapacity);

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
                    << ". Join refused");

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
    Address32 parentAddress32 = engine->getManagerAddress32();

    engine->requestJoinSystemManager( //
                smJoinRequest.capabilities.euidAddress, //
                parentAddress32, //
                (int) indication->apduRequest->appHandle, //
                //(DeviceType::DeviceTypeEnum) provisioningItem.deviceType, //
                //provisioningItem.subnetId, //
                smJoinRequest.capabilities, //standard join request capabilities
                smJoinRequest.softwareRevisionInformation,
                smJoinRequest.deviceCapability, //dlmo.18 capability, custom added on join
                boost::bind(&DMO::responseJoinSystemManager, this, _1, _2, _3));
}

void DMO::responseJoinSystemManager(Address32 deviceAddress, int requestID, int status) {
    LOG_DEBUG(LOG_OI << "Callback responseJoinSystemManager - device:" << Address::toString(deviceAddress) << " reqID="
                << requestID << " status=" << status);

    if (status == ResponseStatus::REFUSED_INSUFICIENT_RESOURCES) {
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
            std::ostringstream stream;
            stream << "Device " << Address::toString(deviceAddress) << " not found.";
            LOG_ERROR(LOG_OI << stream.str());
            throw NEException(stream.str());
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

    //    sendExecuteResponseToRequester(indication, Isa100::Common::Objects::SFC::success, joinResponsePayload, true);
    // create the response here instead of calling sendExecuteResponseToRequester - because 'forceUnencrypted' flag needs
    // to be set in primitive

    ExecuteResponsePDUPointer responsePDU(new ExecuteResponsePDU( //
                indication->forwardCongestionNotification, //
                sfc, //
                joinResponsePayload));

    ClientServerPDUPointer apdu(new ClientServerPDU( //
                Isa100::Common::PrimitiveType::response, //
                Isa100::Common::ServiceType::execute, //
                indication->serverObject, indication->clientObject, indication->apduRequest->appHandle));

    std::string objectIdString;
    getObjectIDString(objectIdString);

    LOG_DEBUG(LOG_OI << objectIdString << ": sendJoinResponseToRequester: client=" << indication->clientNetworkAddress.toString()
                << ", size=" << joinResponsePayload->size());

    ClientServerPDUPointer apduResponse = PDUUtils::appendExecuteResponse(apdu, responsePDU);

    Address32 destAddress32 = engine->getAddress32(indication->clientNetworkAddress);

    NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(
                destAddress32, indication->serverTSAP_ID, indication->clientTSAP_ID);

    if (!contract_SM_Dev) {
        std::ostringstream stream;
        stream << "Couldn't find contract between manager and " << Address_toStream(destAddress32) << ", tsap " << (int) indication->serverTSAP_ID
                    << "->" << (int) indication->clientTSAP_ID;
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
                true // force unencrypted
                ));

    messageDispatcher->Response(primitiveResponse);
    jobFinished = true;
}

void DMO::proxySystemManagerContract(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer executeRequest) {


    LOG_DEBUG(LOG_OI << "process SystemManagerContractRequest() : PARAMS=" << bytes2string(*executeRequest->parameters));

    Isa100::Model::SmJoinContractRequest smContractRequest;
    NetworkOrderStream stream(executeRequest->parameters);
    smContractRequest.unmarshall(stream);

    LOG_INFO(LOG_OI << " CONTRACT REQUEST from:" << smContractRequest.euidAddress.toString());

    //ProvisioningItem & provisioningItem = SmSettingsLogic::instance().getProvisioningForDevice(smContractRequest.euidAddress);
    //if (provisioningItem.isGateway() || provisioningItem.isBackbone()) {
    Address32 parentAddress32 = engine->getManagerAddress32();

    engine->requestJoinContract( //
                smContractRequest.euidAddress, //
                parentAddress32, //
                (int) indication->apduRequest->appHandle, //
                boost::bind(&DMO::responseJoinContract, this, _1, _2, _3));
}

void DMO::responseJoinContract(Address32 deviceAddress, int requestID, int status) {
    LOG_DEBUG(LOG_OI << "Callback responseJoinContract - device:" << Address::toString(deviceAddress) << " reqID=" << requestID
                << " status=" << status);

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
            std::ostringstream stream;
            stream << "Management contract on device " << Address::toString(deviceAddress) << " not found.";
            LOG_ERROR(LOG_OI << stream.str());
            throw NEException(stream.str());
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
            std::ostringstream stream;
            stream << "Network route with manager on device " << Address::toString(deviceAddress) << " not found.";
            LOG_ERROR(LOG_OI << stream.str());
            throw NEException(stream.str());
        }

        newDeviceContractResponse.nlNextHop = networkRouteToManager->nextHop;
        newDeviceContractResponse.nlHopsLimit = networkRouteToManager->nwkHopLimit;
        newDeviceContractResponse.nlOutgoingInterface = networkRouteToManager->outgoingInterface;
        //newDeviceContractResponse.nlHeaderIncludeContractFlag = 0;

        NE::Model::PhyNetworkContract* phyNetworkContract = NE::Model::ModelUtils::findNetworkContract(device,
                    managementContract->contractID);
        if (!phyNetworkContract) {
            std::ostringstream stream;
            stream << "Network contract" << (int) managementContract->contractID << " on device " << Address::toString(
                        deviceAddress) << ", not found.";
            LOG_ERROR(LOG_OI << stream.str());
            throw NEException(stream.str());
        }

        newDeviceContractResponse.nlHeaderIncludeContractFlag = phyNetworkContract->include_Contract_Flag;

        LOG_DEBUG(LOG_OI << "newDeviceContractResponse= " << newDeviceContractResponse.toString());

        NE::Misc::Marshall::NetworkOrderStream outStream;
        newDeviceContractResponse.marshall(outStream);
        joinResponsePayload.reset(new Bytes(outStream.ostream.str()));

    } else {
        sfc = Isa100::Common::Objects::SFC::failure;


    }

    //sendExecuteResponseToRequester(indication, SFC::success, joinResponsePayload, true);
    // create the response here instead of calling sendExecuteResponseToRequester - because 'forceUnencrypted' flag needs
    // to be set in primitive

    ExecuteResponsePDUPointer responsePDU(new ExecuteResponsePDU( //
                indication->forwardCongestionNotification, //
                sfc, //
                joinResponsePayload));

    ClientServerPDUPointer apdu(new ClientServerPDU( //
                Isa100::Common::PrimitiveType::response, //
                Isa100::Common::ServiceType::execute, //
                indication->serverObject, indication->clientObject, indication->apduRequest->appHandle));

    std::string objectIdString;
    getObjectIDString(objectIdString);

    LOG_DEBUG(LOG_OI << objectIdString << ": sendJoinContractResponseToRequester: client="
                << indication->clientNetworkAddress.toString() << ", size=" << joinResponsePayload->size());

    ClientServerPDUPointer apduResponse = PDUUtils::appendExecuteResponse(apdu, responsePDU);

    Address32 destAddress32 = engine->getAddress32(indication->clientNetworkAddress);

    NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(
                destAddress32, indication->serverTSAP_ID, indication->clientTSAP_ID);

    if (!contract_SM_Dev) {
        std::ostringstream stream;
        stream << "Couldn't find contract between manager and " << Address_toStream(destAddress32) << ", tsap " << (int) indication->serverTSAP_ID
                    << "->" << (int) indication->clientTSAP_ID;
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
                true // force unencrypted
                ));

    messageDispatcher->Response(primitiveResponse);
    jobFinished = true;
}

void DMO::proxySecuritySymJoin(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer executeRequest) {

    LOG_DEBUG(LOG_OI << "proxySecuritySymJoin()");

    //set the DMO to wait for a confirm from PSMO and forward the confirm to the requester
    //responseExpectingIndication = indication;
    expectingProxySecurityJoinResponse = true;

    executeRequest->methodID = Isa100::Common::MethodsID::PSMOMethodsId::SECURITY_SYM_JOIN_REQUEST;

    NE::Misc::Marshall::NetworkOrderStream stream(executeRequest->parameters);
    Isa100::Security::SecuritySymJoinRequest joinRequest;
    joinRequest.unmarshall(stream);

    // check if this address is provisioned
    //    try {
    SmSettingsLogic& smSettings = Isa100::Common::SmSettingsLogic::instance();
    Isa100::Common::ProvisioningItem * provisioningItem = smSettings.getProvisioningForDevice(joinRequest.EUI64JoiningDevice);
    if (!provisioningItem) {
        LOG_ERROR(LOG_OI << "The device " << joinRequest.EUI64JoiningDevice.toString() << " is not provisioned. Join refused!");
        jobFinished = true;
        return;
    }
    LOG_DEBUG(LOG_OI << "Provisioning found for device " << joinRequest.EUI64JoiningDevice.toString() << " subnet="
                << provisioningItem->subnetId);

    // add 64-128 address mapping in model
    engine->addManagerNeighbor(joinRequest.EUI64JoiningDevice,
                indication->clientNetworkAddress);

    // forward request to PSMO
    Isa100::ASL::PDU::ClientServerPDUPointer pdu(new Isa100::ASL::PDU::ClientServerPDU( //
                indication->apduRequest->primitiveType, //
                indication->apduRequest->serviceInfo, //
                indication->apduRequest->sourceObjectID, //
                indication->apduRequest->destinationObjectID, //
                indication->apduRequest->appHandle));
    PDUUtils::appendExecuteRequest(pdu, executeRequest);

    Isa100::ASL::Services::PrimitiveIndicationPointer forwardedIndication(
                new Isa100::ASL::Services::PrimitiveIndication(indication->elapsedMsec, //
                            indication->detailedTxTime, //
                            indication->endToEndTransmissionTime, //
                            indication->forwardCongestionNotification, //
                            TSAP::TSAP_SMAP, //
                            ObjectID::ID_PSMO, //
                            SmSettingsLogic::instance().managerAddress128, //
                            TSAP::TSAP_DMAP, //
                            ObjectID::ID_DMO, //
                            pdu));

    LOG_DEBUG(LOG_OI << "proxySecuritySymJoin() : Indication forwarded to PSMO: " << forwardedIndication->toString());
    messageDispatcher->LoopbackIndicate(forwardedIndication);
}

void DMO::forwardJoinResponse(Isa100::ASL::Services::PrimitiveConfirmationPointer confirmation) {

    ExecuteResponsePDUPointer responsePDU = PDUUtils::extractExecuteResponse(confirmation->apduResponse);

    // [hack] sorin - if there is an error don't send the response anymore - we have no contract to do it
    if (responsePDU->feedbackCode != SFC::success) {
        LOG_WARN(LOG_OI << "forwardJoinResponse() - received a response with sfc=" << (int) responsePDU->feedbackCode << " client="
                    << indication->clientNetworkAddress.toString() << ". Dropping it..");
        return;
    }

    ClientServerPDUPointer apdu(new ClientServerPDU( //
                Isa100::Common::PrimitiveType::response, //
                Isa100::Common::ServiceType::execute, //
                indication->serverObject, //
                indication->clientObject, //
                indication->apduRequest->appHandle));

    LOG_DEBUG(LOG_OI << "forwardJoinResponse() : to client=" << indication->clientNetworkAddress.toString());

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
                true // force unencrypted
                ));

    messageDispatcher->Response(primitiveResponse);
    jobFinished = true;
}

void DMO::confirmExecute(Isa100::ASL::Services::PrimitiveConfirmationPointer confirmation) {
    LOG_DEBUG(LOG_OI << "confirmExecute() : Confirmation received in DMO from " << confirmation->serverNetworkAddress.toString());

    //if (requestID == responseExpectingIndication->apduRequest->requestID) {
    if (confirmation->apduResponse->appHandle == indication->apduRequest->appHandle) {
        if (expectingProxySecurityJoinResponse) {
            forwardJoinResponse(confirmation);
            expectingProxySecurityJoinResponse = false;
        } else {
            LOG_WARN(LOG_OI << "confirmExecute() : Invalid confirmExecute!");
        }
    }
}

Isa100::AL::ObjectID::ObjectIDEnum DMO::getObjectID() const {
    return ObjectID::ID_DMO;
}

}
}
}
