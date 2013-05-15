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
 * @author Beniamin Tecar, sorin.bidian
 */
#include "PSMO.h"

#include <algorithm>
#include <functional>
#include <boost/bind.hpp>
#include <ASL/PDUUtils.h>
#include <ASL/PDU/ExecuteRequestPDU.h>
#include <ASL/PDU/ExecuteResponsePDU.h>
#include <Common/CCM/AuthenticationFailException.h>
#include <Security/SecuritySymJoinRequest.h>
#include <Security/SecuritySymJoinResponse.h>
#include <Security/SecurityUpdateSessionKeyRequest.h>
#include <Common/Objects/SFC.h>
#include <Model/model.h>
#include <Model/EngineProvider.h>
#include <AL/DMAP/NLMO.h>
#include <AL/DMAP/TLMO.h>
#include <Security/SecurityManager.h>
#include <Common/NETypes.h>
#include <Common/MethodsIDs.h>
#include <Common/SmSettingsLogic.h>
#include <Common/Objects/SFC.h>
#include "Common/Utils/ContractUtils.h"
#include "Common/HandleFactory.h"
#include "Security/KeyUtils.h"
#include "Model/ModelPrinter.h"
#include "AL/Types/AlertTypes.h"
#include "Common/HandleFactory.h"

using namespace Isa100::ASL;
using namespace Isa100::ASL::PDU;
using namespace NE::Common;
using namespace NE::Model::Operations;
using namespace NE::Model;
using namespace Isa100::Model;
using namespace Isa100::Common;
using namespace Isa100::Common::Objects;
using namespace Isa100::Security;

namespace Isa100 {
namespace AL {
namespace SMAP {

PSMO::PSMO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine_) :
    Isa100Object(tsap, engine_) {

    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: created object:" << objectIdString);
    currentState = Initial;
    confirmedSetKeyA = false;
    confirmedSetKeyB = false;
    errorOnConfirmKeys = false;
    expectingSetKeyGWResponse = false;
}

PSMO::~PSMO() {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: destroyed object:" << objectIdString);
}

void PSMO::execute(Uint32 currentTime) {
}

bool PSMO::isJobFinished() {
    return jobFinished || ((currentState == Finished) && !expectingSetKeyGWResponse);
}

bool PSMO::expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    return false;
}

bool PSMO::expectConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
    if (currentState == WaitingForSetKeyTo) {
        if (confirm->serverObject == ObjectID::ID_DSMO && confirm->serverTSAP_ID == TSAP::TSAP_DMAP
        //&& confirm->apduResponse->primitiveType == Isa100::Common::PrimitiveType::response
                    && confirm->apduResponse->serviceInfo == Isa100::Common::ServiceType::execute) {

            return (!confirmedSetKeyB && (setKeyB && (confirm->apduResponse->appHandle == setKeyB->apduRequest->appHandle)));
        }
    } else if (currentState == WaitingForSetKeyFrom) {
        if (confirm->serverObject == ObjectID::ID_DSMO && confirm->serverTSAP_ID == TSAP::TSAP_DMAP
                    && confirm->apduResponse->serviceInfo == Isa100::Common::ServiceType::execute) {

            return (!confirmedSetKeyA && (setKeyA && (confirm->apduResponse->appHandle == setKeyA->apduRequest->appHandle)));
        }
    }

    return false;
}

void PSMO::indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    try {
        ASL::PDU::ExecuteRequestPDUPointer executeRequest = Isa100::ASL::PDUUtils::extractExecuteRequest(indication->apduRequest);

        switch (executeRequest->methodID) {
            case Isa100::Common::MethodsID::PSMOMethodsId::SECURITY_SYM_JOIN_REQUEST: {
                processSecuritySymJoinRequest(indication, executeRequest);
                break;
            }
            case Isa100::Common::MethodsID::PSMOMethodsId::SECURITY_CONFIRM: {
                processSecurityConfirmRequest(indication, executeRequest);
                break;
            }

            case Isa100::Common::MethodsID::PSMOMethodsId::SECURITY_NEW_SESSION: {
                processSecurityNewSessionRequest(indication, executeRequest);
                break;
            }

            default: {
                LOG_ERROR(LOG_OI << "InvokeMethod: unknown method id: " << (int) executeRequest->methodID << "Discarding packet");
                sendExecuteResponseToRequester(indication, SFC::invalidMethod, BytesPointer(new Bytes()), false);
            }
        }
    } catch (Isa100::Common::AuthenticationFailException& ex) {
        LOG_ERROR(LOG_OI << ex.what());
        try {

            sendExecuteResponseToRequester(indication, SFC::inconsistentContent, BytesPointer(new Bytes()), true);

        } catch (NE::Common::NEException& ex) {
            LOG_ERROR(LOG_OI << ex.what());
        } catch (...) {
            LOG_ERROR(LOG_OI << "Unknown exception trying to send error code to the client object.");
        }
        LOG_ERROR(LOG_OI << "All requests are cleared for object=" << getObjectID());
    }
}

void PSMO::processSecuritySymJoinRequest(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer& executeRequest) {

    LOG_DEBUG(LOG_OI << "processSecuritySymJoinRequest - device: " << indication->clientNetworkAddress.toString());

    NE::Misc::Marshall::NetworkOrderStream stream(executeRequest->parameters);
    joinRequest.unmarshall(stream);

    //check provisioning first and then the challenge; (retries with the same challenge can arrive for a device with provisioning removed)

    Isa100::Common::ProvisioningItem * provisioningItem =
        SmSettingsLogic::instance().getProvisioningForDevice(joinRequest.EUI64JoiningDevice);

    //No provisioning found for device
    if (!provisioningItem) {
        std::ostringstream stream;
        stream << "Provisioning for " << joinRequest.EUI64JoiningDevice.toString() << " not found. Join refused.";
        LOG_ERROR(stream.str());

        //send join failure alert
        //LOG_INFO("[SORIN] alert join not_provisioned");
        Uint32 currentTAI = ClockSource::getTAI(engine->getSettingsLogic());
        AlertJoinFailed *alertJoinFailed = new AlertJoinFailed(joinRequest.EUI64JoiningDevice,
                                                                (Uint8) StatusForReports::SEC_JOIN_REQUEST_RECEIVED,
                                                                (Uint8) JoinFailureReason::not_provisioned);
        AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoinFailed, alertJoinFailed, currentTAI));
        engine->getOperationsProcessor().sendAlert(alertOperation);

        sendExecuteResponseToRequester(indication, Isa100::Common::Objects::SFC::configurationMismatch, BytesPointer(new Bytes()), true);
        return;
    }

    //*******debug
    if (SmSettingsLogic::instance().disableAuthentication) {
    	LOG_INFO("challenge check skipped");
    } else
    //*******
    if (!securityManager->checkChallenge(joinRequest)) {
        LOG_ERROR(LOG_OI
                    << "This is a security check. Received a join request containing a challenge that has already been used;"
                    << " device=" << joinRequest.EUI64JoiningDevice.toString() << ". Request discarded for security safety.");
        //By Cata: do not send any response: in case of a retry (while we process first request) the response with failure will be sent before response with success
        // and the response with success sent after will be useless.
        //        sendExecuteResponseToRequester(indication, SFC::incompatibleMode, BytesPointer(new Bytes()), true);

        //send join failure alert
        //LOG_INFO("[SORIN] alert join invalid challenge");
        Uint32 currentTAI = ClockSource::getTAI(engine->getSettingsLogic());
        AlertJoinFailed *alertJoinFailed = new AlertJoinFailed(joinRequest.EUI64JoiningDevice,
                                                                (Uint8) StatusForReports::SEC_JOIN_REQUEST_RECEIVED,
                                                                (Uint8) JoinFailureReason::challenge_check_failure);
        AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoinFailed, alertJoinFailed, currentTAI));
        engine->getOperationsProcessor().sendAlert(alertOperation);

        jobFinished = true;
        return;
    }

    Address32 parentAddress32;

    if (provisioningItem->isGateway() || provisioningItem->isBackbone()) {
        parentAddress32 = engine->getManagerAddress32();
    } else {
        parentAddress32 = engine->getAddress32(indication->clientNetworkAddress);
    }

    //The Key ID of the master key and session key shall be set implicitly (not transmitted but inferred) as 0x00.
    //join key index is 1; attention: this index is used in responseJoinSecurity
    Uint16 joinKeyID = 0;
    Uint16 index = 1;

    const NE::Model::PhySessionKey& joinKey = securityManager->createJoinKey(joinRequest.EUI64JoiningDevice, joinKeyID, index);
    LOG_INFO("createJoinKey device=" << joinRequest.EUI64JoiningDevice.toString() << ", joinKey=" << joinKey);

    engine->requestJoinSecurity( //
                joinRequest.EUI64JoiningDevice, //
                parentAddress32, //
                (int) indication->apduRequest->appHandle, //
                (DeviceType::DeviceTypeEnum) provisioningItem->deviceType, //
                provisioningItem->subnetId, //
                joinKey, //
                boost::bind(&PSMO::responseJoinSecurity, this, _1, _2, _3));

}

void PSMO::responseJoinSecurity(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status) {
    LOG_DEBUG(LOG_OI << "Callback responseSecurityJoin - device:" << Address::toString(deviceAddress) << " reqID=" << requestID
                << " status=" << status);

    if (status == ResponseStatus::FAIL) {
        //send join failure alert
        //LOG_INFO("[SORIN] alert fail");
        Uint32 currentTAI = ClockSource::getTAI(engine->getSettingsLogic());
        AlertJoinFailed *alertJoinFailed = new AlertJoinFailed(joinRequest.EUI64JoiningDevice,
                                                                (Uint8) StatusForReports::SEC_JOIN_REQUEST_RECEIVED,
                                                                (Uint8) JoinFailureReason::other);
        AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoinFailed, alertJoinFailed, currentTAI));
        engine->getOperationsProcessor().sendAlert(alertOperation);

        sendExecuteResponseToRequester(indication, Isa100::Common::Objects::SFC::internalError, BytesPointer(new Bytes()), true);
        return;
    } else if (status == ResponseStatus::REQUEST_DISCARDED) {
        jobFinished = true;
        return;
    } else if (status == ResponseStatus::REFUSED_INSUFICIENT_RESOURCES) {
        //send join failure alert
        //LOG_INFO("[SORIN] alert insufficient resources");
        Uint32 currentTAI = ClockSource::getTAI(engine->getSettingsLogic());
        AlertJoinFailed *alertJoinFailed = new AlertJoinFailed(joinRequest.EUI64JoiningDevice,
                                                                (Uint8) StatusForReports::SEC_JOIN_REQUEST_RECEIVED,
                                                                (Uint8) JoinFailureReason::insufficient_parent_resources);
        AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoinFailed, alertJoinFailed, currentTAI));
        engine->getOperationsProcessor().sendAlert(alertOperation);

        sendExecuteResponseToRequester(indication, Isa100::Common::Objects::SFC::insufficientDeviceResources,
                    BytesPointer(new Bytes()), true);
        return;
    }

    NE::Model::Device * device = engine->getDevice(deviceAddress);

    if (!device) {
        LOG_ERROR( "Device " << Address::toString(deviceAddress) << " not found.");
        return;
    }

	//perform no security job if auth is disabled
	if (SmSettingsLogic::instance().disableAuthentication) {
		this->securitySymJoinResponse(true, SecuritySymJoinResponse(), SecurityJoinFailureReason::success);
		return;
	}

    //key index for join key is 1
    Uint16 index = 1;
    EntityIndex eIndex = createEntityIndex(deviceAddress, EntityType::SessionKey, index);
    SessionKeyIndexedAttribute::iterator it = device->phyAttributes.sessionKeysTable.find(eIndex);

    if (it != device->phyAttributes.sessionKeysTable.end() && it->second.currentValue) {
        //currentState = WaitingForSecManagerJoinResp;

        NE::Model::PhySpecialKey masterKey;
        NE::Model::PhySpecialKey subnetKey;

        securityManager->SecurityJoinRequest( //
                    joinRequest, //
                    it->second.currentValue->key, //
                    it->second.currentValue->keyID, //
                    masterKey, //out
                    subnetKey, //out
                    boost::bind(&PSMO::securitySymJoinResponse, this, _1, _2, _3));
        //set stack keys
        securityManager->setStackKeys(joinRequest, *it->second.currentValue, device->address128);
        //set in model the master key and subnet key of device
        EntityIndex eIndexMaster = createEntityIndex(deviceAddress, EntityType::MasterKey, masterKey.keyID);
        device->phyAttributes.createMasterKey(eIndexMaster, new NE::Model::PhySpecialKey(masterKey));
        EntityIndex eIndexSubnet = createEntityIndex(deviceAddress, EntityType::SubnetKey, subnetKey.keyID);
        device->phyAttributes.createSubnetKey(eIndexSubnet, new NE::Model::PhySpecialKey(subnetKey));
    } else {
        LOG_ERROR("Could not find the join key eIndex=" << std::hex << eIndex << " on the device=" << Address::toString(device->address32) );
        return;
    }
}

void PSMO::securitySymJoinResponse(bool status, Isa100::Security::SecuritySymJoinResponse response, int failureReason) {
    if (status) {

        NE::Misc::Marshall::NetworkOrderStream outStream;
        response.marshall(outStream);

        BytesPointer bytes(new Bytes(outStream.ostream.str()));

        LOG_DEBUG(LOG_OI << "Security join succeeded size=" << bytes->size() << ", payload=" << response.toString());

        sendExecuteResponseToRequester(indication, Isa100::Common::Objects::SFC::success, bytes, true);

    } else {
        LOG_WARN(LOG_OI << "Security Join Request failed.");

        if (failureReason == SecurityJoinFailureReason::bad_join_key) {
            //send join failure alert
            //LOG_INFO("[SORIN] alert join bad join key");
            Uint32 currentTAI = ClockSource::getTAI(engine->getSettingsLogic());
            AlertJoinFailed *alertJoinFailed = new AlertJoinFailed(joinRequest.EUI64JoiningDevice,
                                                                    (Uint8) StatusForReports::SEC_JOIN_REQUEST_RECEIVED,
                                                                    (Uint8) JoinFailureReason::invalid_join_key);
            AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoinFailed, alertJoinFailed, currentTAI));
            engine->getOperationsProcessor().sendAlert(alertOperation);

        } else if (failureReason == SecurityJoinFailureReason::subnet_not_foud) {
            //send join failure alert
            //LOG_INFO("[SORIN] alert join subnet_incorrectly_provisioned");
            Uint32 currentTAI = ClockSource::getTAI(engine->getSettingsLogic());
            AlertJoinFailed *alertJoinFailed = new AlertJoinFailed(joinRequest.EUI64JoiningDevice,
                                                                    (Uint8) StatusForReports::SEC_JOIN_REQUEST_RECEIVED,
                                                                    (Uint8) JoinFailureReason::subnet_provisioning_mismatch);
            AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoinFailed, alertJoinFailed, currentTAI));
            engine->getOperationsProcessor().sendAlert(alertOperation);

        } else if (failureReason == SecurityJoinFailureReason::unprovisioned_device) {
            //send join failure alert
            //LOG_INFO("[SORIN] alert join not_provisioned");
            Uint32 currentTAI = ClockSource::getTAI(engine->getSettingsLogic());
            AlertJoinFailed *alertJoinFailed = new AlertJoinFailed(joinRequest.EUI64JoiningDevice,
                                                                    (Uint8) StatusForReports::SEC_JOIN_REQUEST_RECEIVED,
                                                                    (Uint8) JoinFailureReason::not_provisioned);
            AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoinFailed, alertJoinFailed, currentTAI));
            engine->getOperationsProcessor().sendAlert(alertOperation);
        } else {
            //send join failure alert
            //LOG_INFO("[SORIN] alert join other reason");
            Uint32 currentTAI = ClockSource::getTAI(engine->getSettingsLogic());
            AlertJoinFailed *alertJoinFailed = new AlertJoinFailed(joinRequest.EUI64JoiningDevice,
                                                                    (Uint8) StatusForReports::SEC_JOIN_REQUEST_RECEIVED,
                                                                    (Uint8) JoinFailureReason::other);
            AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoinFailed, alertJoinFailed, currentTAI));
            engine->getOperationsProcessor().sendAlert(alertOperation);
        }

        currentState = Finished;
        sendExecuteResponseToRequester(indication, Isa100::Common::Objects::SFC::failure, BytesPointer(new Bytes()), true);
    }
}

void PSMO::processSecurityConfirmRequest(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer& executeRequest) {

    LOG_DEBUG(LOG_OI << "processJoinConfirmRequest - device: " << indication->clientNetworkAddress.toString());

    Address32 deviceAddress32 = engine->getAddress32(indication->clientNetworkAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        std::ostringstream stream;
        stream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_ERROR(stream.str());
        throw NEException(stream.str());
    }

    Isa100::Security::SecurityConfirmRequest joinConfirm;
    NE::Misc::Marshall::NetworkOrderStream stream(executeRequest->parameters);
    joinConfirm.unmarshall(stream);

    if (SmSettingsLogic::instance().disableAuthentication
    		|| securityManager->SecurityJoinConfirm(device->address64, joinConfirm)) {
        //joinConfirmReqPrimitive = indication;

        engine->requestConfirmJoin( //
                    deviceAddress32, //
                    (int) indication->apduRequest->appHandle, //
                    boost::bind(&PSMO::responseSecurityConfirm, this, _1, _2, _3));

    } else {
        LOG_ERROR(LOG_OI << "Join of device " << indication->clientNetworkAddress.toString()
                    << " failed on HMACK security check - result equal to previous join !");
        currentState = Finished;
    }
}

void PSMO::responseSecurityConfirm(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status) {
    LOG_DEBUG(LOG_OI << "Callback responseSecurityConfirm - device:" << Address::toString(deviceAddress)
                     << " reqID=" << requestID << " status=" << status);

    if (status == ResponseStatus::SUCCESS) {
        sendExecuteResponseToRequester(indication, SFC::success, BytesPointer(new Bytes()), false);

    } else if (status == ResponseStatus::FAIL) {
        //alert on FAIL is skipped because if there's an error an alert will be generated in another place

        sendExecuteResponseToRequester(indication, SFC::failure, BytesPointer(new Bytes()), true);
        return;

    } else if (status == ResponseStatus::REFUSED_INSUFICIENT_RESOURCES) {
        //send join failure alert
        Device *device = engine->getDevice(deviceAddress);
        if (device) {
            //LOG_INFO("[SORIN] alert insufficient resources");
            Uint32 currentTAI = ClockSource::getTAI(engine->getSettingsLogic());
            AlertJoinFailed *alertJoinFailed = new AlertJoinFailed(device->address64,
                                                                    (Uint8) StatusForReports::SEC_CONFIRM_RECEIVED,
                                                                    (Uint8) JoinFailureReason::insufficient_parent_resources);
            AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoinFailed, alertJoinFailed, currentTAI));
            engine->getOperationsProcessor().sendAlert(alertOperation);
        }

        sendExecuteResponseToRequester(indication, SFC::insufficientDeviceResources, BytesPointer(new Bytes()), true);
        return;
    }
    //else the request is discarded and no response must be sent(possibly a retry).

    //at GW join create session DMAP-UAP2 for alert reports
    Device *device = engine->getDevice(deviceAddress);
    if (status == ResponseStatus::SUCCESS && device && device->capabilities.isGateway()) {

        LOG_INFO("create session sm-gw, tsaps 0-2");

        Device *manager = engine->getDevice(engine->getManagerAddress32());

        if (manager) {
            Uint16 keyID = manager->getGreatestKeyIDwithPeer(device, TSAP::TSAP_DMAP, TSAP::TSAP_UAP2);

            Uint16 index = manager->getNextKeysTableIndex();
            const NE::Model::PhySessionKey& keyFrom = securityManager->createSecurityKey(manager->address64, device->address64,
                        manager->address128, device->address128, TSAP::TSAP_DMAP, TSAP::TSAP_UAP2, keyID, index);

            index = device->getNextKeysTableIndex();

            const NE::Model::PhySessionKey& keyTo = KeyUtils::getAsPhySessionKey(
                        keyFrom.key, //
                        device->address64, manager->address64, device->address128, manager->address128,
                        TSAP::TSAP_UAP2, TSAP::TSAP_DMAP, keyFrom.sessionKeyPolicy, keyID, index);

            AppHandle reqID = HandleFactory().CreateHandle();

            engine->requestSecurityNewSession(manager->address32, //
                        (int) reqID,
                        keyFrom, keyTo, boost::bind(&PSMO::responseSecurityNewSession, this, _1, _2, _3));
            return;
        } else {
            LOG_ERROR("Oops..no manager !?");
        }
    }

    currentState = Finished;
    jobFinished = true;
}

//obsolete
void PSMO::processJoinConfirmation(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer& executeRequest) {
    LOG_WARN(LOG_OI << "obsolete method called - processJoinConfirmation()");
}

void PSMO::processDMSOFinishOnConfirm() {
    LOG_WARN(LOG_OI << "obsolete method called - processDMSOFinishOnConfirm()");
}

void PSMO::responseSecurityNewSession(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status) {
    if (status == ResponseStatus::SUCCESS) {
        LOG_DEBUG(LOG_OI << "Callback responseSecurityNewSession - reqID=" << requestID << " status= SUCCESS");
    } else if (status == ResponseStatus::REQUEST_DISCARDED) {
        LOG_ERROR(LOG_OI << "Response handler for SecurityNewSession called with status REQUEST_DISCARDED");
    } else {
        LOG_ERROR(LOG_OI << "Response handler for SecurityNewSession called with status FAIL");
        //        sendExecuteResponseToRequester(indication, SFC::failure, BytesPointer(new Bytes()), true);
    }

    currentState = Finished;
    jobFinished = true;
}

void PSMO::processSecurityNewSessionRequest(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer& executeRequest) {

    NE::Misc::Marshall::NetworkOrderStream stream(executeRequest->parameters);
    newSessionRequest.unmarshall(stream);

    Address32 deviceAddress32 = engine->getAddress32(indication->clientNetworkAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        LOG_ERROR("Received request from " << indication->clientNetworkAddress.toString()
                    << ". Device " << Address::toString(deviceAddress32) << " not found.");
        sendExecuteResponseToRequester(indication, SFC::deviceNotFound, BytesPointer(new Bytes()), true);
        return;
    }

    //authenticate request
    if (!securityManager->validateNewSessionRequest(newSessionRequest, device)) {
        LOG_ERROR("Invalid MIC in new session request.");
        sendExecuteResponseToRequester(indication, Isa100::Common::Objects::SFC::failure, BytesPointer(new Bytes()), true);
        return;
    }

    //if the request is for a master key or DL key, the address128A is 0000:0000...:0000
    Address128 emptyAddress;
    if (newSessionRequest.address128B == emptyAddress
                || newSessionRequest.address128A == emptyAddress) {

        LOG_DEBUG(LOG_OI << "new master/DL key request");

        //send new session response to requester
        SecurityNewSessionResponse newSessionResponse;
        securityManager->getNewSessionResponse(device, 0x01, newSessionResponse); // 0x1 = SECURITY_SESSION_GRANTED
        NE::Misc::Marshall::NetworkOrderStream newSessionResponseStream;
        newSessionResponse.marshall(newSessionResponseStream);
        sendExecuteResponseToRequester(indication, SFC::success,
                    BytesPointer(new Bytes(newSessionResponseStream.ostream.str())), false);

        bool newKeyCreated = securityManager->createNewSpecialKey(device);

        if (!newKeyCreated) {
            LOG_INFO("Setting no key...");
        }
        return;
    }

    LOG_DEBUG(LOG_OI << "new session from: " << newSessionRequest.address128A.toString() << " to:"
                << newSessionRequest.address128B.toString());

    Address32 destAddress32 = engine->getAddress32(newSessionRequest.address128B);
    NE::Model::Device * destDevice = engine->getDevice(destAddress32);
    if (!destDevice) {
        std::ostringstream errStream;
        errStream << "New session key with device " << newSessionRequest.address128B.toString()
                    << " not created. Device not found.";
        LOG_ERROR(errStream.str());
        //sendExecuteResponseToRequester(indication, SFC::deviceNotFound, BytesPointer(new Bytes()), true);
        //send new session response to requester - with status DENIED
        SecurityNewSessionResponse newSessionResponse;
        securityManager->getNewSessionResponse(device, 0x00, newSessionResponse); //0x0 = SECURITY_SESSION_DENIED
        NE::Misc::Marshall::NetworkOrderStream newSessionResponseStream;
        newSessionResponse.marshall(newSessionResponseStream);
        sendExecuteResponseToRequester(indication, SFC::success,
                    BytesPointer(new Bytes(newSessionResponseStream.ostream.str())), true);
        return;
    }

    Address32 srcAddress32 = engine->getAddress32(newSessionRequest.address128A);
    NE::Model::Device * srcDevice = engine->getDevice(srcAddress32);
    if (!srcDevice) {
        std::ostringstream errStream;
        errStream << "Device " << Address::toString(srcAddress32) << " not found.";
        LOG_ERROR(errStream.str());
        //send new session response to requester - with status DENIED
        SecurityNewSessionResponse newSessionResponse;
        securityManager->getNewSessionResponse(device, 0x00, newSessionResponse); //0x0 = SECURITY_SESSION_DENIED
        NE::Misc::Marshall::NetworkOrderStream newSessionResponseStream;
        newSessionResponse.marshall(newSessionResponseStream);
        sendExecuteResponseToRequester(indication, SFC::success,
                    BytesPointer(new Bytes(newSessionResponseStream.ostream.str())), true);
        return;
    }

    //deny session if a peer is not JOINED_AND_CONFIGURED
    if ((destDevice->statusForReports != StatusForReports::JOINED_AND_CONFIGURED)
                || (srcDevice->statusForReports != StatusForReports::JOINED_AND_CONFIGURED)) {
        LOG_ERROR("One of the peer devices for the requested session is not JOINED_AND_CONFIGURED. Refusing session..");
        //send new session response to requester - with status DENIED
        SecurityNewSessionResponse newSessionResponse;
        securityManager->getNewSessionResponse(device, 0x00, newSessionResponse); //0x0 = SECURITY_SESSION_DENIED
        NE::Misc::Marshall::NetworkOrderStream newSessionResponseStream;
        newSessionResponse.marshall(newSessionResponseStream);
        sendExecuteResponseToRequester(indication, SFC::success,
                    BytesPointer(new Bytes(newSessionResponseStream.ostream.str())), true);
        return;
    }

    //sendExecuteResponseToRequester(indication, SFC::success, BytesPointer(new Bytes()), false);
    //send new session response to requester
    SecurityNewSessionResponse newSessionResponse;
    securityManager->getNewSessionResponse(device, 0x01, newSessionResponse); // 0x1 = SECURITY_SESSION_GRANTED
    NE::Misc::Marshall::NetworkOrderStream newSessionResponseStream;
    newSessionResponse.marshall(newSessionResponseStream);
    sendExecuteResponseToRequester(indication, SFC::success,
                BytesPointer(new Bytes(newSessionResponseStream.ostream.str())), false);

    //search if there already exists a session
    bool existsKeyA = false;
    bool existsKeyB = false;
    NE::Model::PhySessionKey existingKeyA;
    NE::Model::PhySessionKey existingKeyB;
    for (SessionKeyIndexedAttribute::iterator itKeys = srcDevice->phyAttributes.sessionKeysTable.begin(); itKeys
                != srcDevice->phyAttributes.sessionKeysTable.end(); ++itKeys) {

        if (itKeys->second.currentValue
        			&& (itKeys->second.currentValue->destination128 == newSessionRequest.address128B)
                    && ((TSAP::TSAP_Enum) itKeys->second.currentValue->destinationTSAP == newSessionRequest.endPortB)
                    && ((TSAP::TSAP_Enum) itKeys->second.currentValue->sourceTSAP == newSessionRequest.endPortA)
                    && !itKeys->second.currentValue->markedAsExpiring) {

            existsKeyA = true;
            existingKeyA = *itKeys->second.currentValue;
            break;
        }
    }
    for (SessionKeyIndexedAttribute::iterator itKeys = destDevice->phyAttributes.sessionKeysTable.begin(); itKeys
                != destDevice->phyAttributes.sessionKeysTable.end(); ++itKeys) {

        if (itKeys->second.currentValue
        			&& (itKeys->second.currentValue->destination128 == newSessionRequest.address128A)
                    && ((TSAP::TSAP_Enum) itKeys->second.currentValue->destinationTSAP == newSessionRequest.endPortA)
                    && ((TSAP::TSAP_Enum) itKeys->second.currentValue->sourceTSAP == newSessionRequest.endPortB)
                    && !itKeys->second.currentValue->markedAsExpiring) {

            existsKeyB = true;
            existingKeyB = *itKeys->second.currentValue;
            break;
        }
    }

    if (existsKeyA && existsKeyB) { //send the old keys
        LOG_INFO("Request for a session that already exists. Sending the existent keys: " << "keyID=" << (int) existingKeyA.keyID
                    << ", srcKeyIndex=" << (int) existingKeyA.index << ", destKeyIndex=" << (int) existingKeyB.index);
        engine->requestSecurityNewSession(srcAddress32, //
                    (int) indication->apduRequest->appHandle, //
                    existingKeyA, existingKeyB, boost::bind(&PSMO::responseSecurityNewSession, this, _1, _2, _3));
        return;
    }

    Uint16 keyID = srcDevice->getGreatestKeyIDwithPeer(destDevice, newSessionRequest.endPortA, newSessionRequest.endPortB);
    //++keyID; //next id

    Uint16 index = srcDevice->getNextKeysTableIndex();
    const NE::Model::PhySessionKey& keyFrom = securityManager->createSecurityKey(srcDevice->address64, destDevice->address64,
                srcDevice->address128, destDevice->address128, newSessionRequest.endPortA, newSessionRequest.endPortB, keyID,
                index);

    index = destDevice->getNextKeysTableIndex();
    const NE::Model::PhySessionKey& keyTo = KeyUtils::getAsPhySessionKey(
                keyFrom.key, //
                destDevice->address64, srcDevice->address64, destDevice->address128, srcDevice->address128,
                newSessionRequest.endPortB, newSessionRequest.endPortA, keyFrom.sessionKeyPolicy, keyID, index);

    engine->requestSecurityNewSession(srcAddress32, //
                (int) indication->apduRequest->appHandle, //
                keyFrom, keyTo, boost::bind(&PSMO::responseSecurityNewSession, this, _1, _2, _3));

}

SFC::SFCEnum PSMO::sendSessionKey(const Address128& to, Isa100::Security::SecurityKeyAndPolicies& sessionKey,
            Isa100::ASL::Services::PrimitiveRequestPointer &setKey) {

    NE::Misc::Marshall::NetworkOrderStream stream;

    ASL::PDU::ExecuteRequestPDUPointer forwardedRequest(
                new ASL::PDU::ExecuteRequestPDU(Isa100::Common::MethodsID::DSMOMethodId::NEW_KEY, BytesPointer(
                            new Bytes(stream.ostream.str()))));

    Isa100::ASL::PDU::ClientServerPDUPointer pdu(new Isa100::ASL::PDU::ClientServerPDU( //
                Isa100::Common::PrimitiveType::request, //
                Isa100::Common::ServiceType::execute, //
                ObjectID::ID_PSMO, //
                ObjectID::ID_DSMO, //
                HandleFactory().CreateHandle()));
    ASL::PDUUtils::appendExecuteRequest(pdu, forwardedRequest);

    Address32 destAddress32 = engine->getAddress32(to);
    NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(destAddress32, TSAP::TSAP_SMAP,
                TSAP::TSAP_DMAP);

    if (!contract_SM_Dev) {
        std::ostringstream stream;
        stream << "Contract SMAP-DMAP between manager and " << Address::toString(destAddress32) << " not found.";
        LOG_ERROR(stream.str());
        return SFC::failure;
    }

    setKey.reset(new Isa100::ASL::Services::PrimitiveRequest( //
                contract_SM_Dev->contractID, //
                destAddress32, //
                ServicePriority::high, // priority
                false, // discard eligible
                TSAP::TSAP_DMAP, // serverTSAP
                ObjectID::ID_DSMO, // server Obj
                TSAP::TSAP_SMAP, // client SAP
                ObjectID::ID_PSMO, // client Obj
                pdu));

    messageDispatcher->Request(setKey);

    return SFC::success;
}

std::string PSMO::stateString() {
    switch (currentState) {
        case Initial:
            return "Initial";
        case WaitingForSecManagerJoinResp:
            return "WaitingForSecManagerJoinResp";
        case WaitingForSecJoinConfirmReq:
            return "WaitingForSecJoinConfirmReq";
        case WaitingForDMSOConfirm:
            return "WaitingForDMSOConfirm";
        case WaitingForSetKeyTo:
            return "WaitingForSetKeyTo";
        case WaitingForSetKeyFrom:
            return "WaitingForSetKeyFrom";
        case Finished:
            return "Finished";
        default:
            std::ostringstream str;
            str << "Unknown(" << (int) currentState << ")";
            return str.str();
    }
}

std::string PSMO::toString() {
    std::ostringstream stream;

    //
    stream << "TODO";
    //

    return stream.str();
}

void PSMO::NotifyOfJoinRequest(NE::Common::Address64& joiningAddress) {
    LOG_WARN("obsolete method called - NotifyOfJoinRequest()");
}

}
}
}
