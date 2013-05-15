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
 * @author Beniamin Tecar, radu.pop, catalin.pop
 */
#include "SOO.h"
#include <boost/bind.hpp>

#include "AL/DMAP/NLMO.h"
#include "ASL/PDUUtils.h"
#include "Model/EngineProvider.h"
#include "Model/OperationPDUsVisitor.h"
#include "Model/EntitiesHelper.h"
#include "Model/ModelPrinter.h"
#include "Model/ModelUtils.h"
#include "Security/SecurityKeyAndPolicies.h"
#include "Model/_signatureDeleteDeviceCallback.h"

using namespace Isa100::AL::DMAP;
using namespace Isa100::ASL::PDU;
using namespace Isa100::Common;
using namespace Isa100::Common::Objects;
using namespace Isa100::Model;
using namespace Isa100::Security;

namespace Isa100 {
namespace AL {
namespace SMAP {

SOO::SOO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine_) :
    Isa100Object(tsap, engine_) {

    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: created object:" << objectIdString);

    engine->registerSendOperationCallback(this);
    SubnetsMap& subnetsMap = engine->getSubnetsList();
    for (SubnetsMap::iterator itSubnet = subnetsMap.begin(); itSubnet != subnetsMap.end(); ++itSubnet) {
        itSubnet->second->registerDeleteDeviceCallback(this);
    }
}

SOO::~SOO() {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: destroyed object:" << objectIdString);
}

void SOO::execute(Uint32 currentTime) {
    resetLifeTime(currentTime); //resets the timeout alarm. This object has forever life time.
}

bool SOO::expectConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
    Address32 serverAddress32 = engine->getAddress32(confirm->serverNetworkAddress);
    OperationReqID reqID = createOperationReqID(serverAddress32, confirm->apduResponse->appHandle);
    return reqIDsMap.find(reqID) != reqIDsMap.end();
}

bool SOO::sendOperation(NE::Model::Operations::IEngineOperationPointer& operation) {

    // LOG_DEBUG("sendOperation op=" << *operation);

    bool ownerManager = operation->getOwner() == engine->getManagerAddress32();

    if (ownerManager) {
        setStackManagerSettings(operation);

    } else {
        Isa100::AL::ObjectID::ObjectIDEnum clientObject = ObjectID::ID_SOO;

        OperationPDUsVisitor pduVisitor(clientObject, *securityManager);
        operation->accept(pduVisitor);
        const ClientServerPDUList& pduList = pduVisitor.getPDUs();

        Address32 destinationAddress32 = operation->getOwner();


        Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID = Isa100::Common::TSAP::TSAP_SMAP;
        Isa100::Common::TSAP::TSAP_Enum serverTSAP_ID = Isa100::Common::TSAP::TSAP_DMAP;

        NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(
                    destinationAddress32, clientTSAP_ID, serverTSAP_ID);
        if (!contract_SM_Dev) {
            LOG_ERROR(LOG_OI << " Operation for device without contract: " << Address_toStream(destinationAddress32) << ", tsap "
                        << (int) clientTSAP_ID << "->" << (int) serverTSAP_ID);
            return true;
        }

        for (ClientServerPDUList::const_iterator itPdu = pduList.begin(); itPdu != pduList.end(); ++itPdu) {
            Isa100::ASL::Services::PrimitiveRequestPointer primitiveRequest(new Isa100::ASL::Services::PrimitiveRequest( //
                        contract_SM_Dev->contractID, //
                        destinationAddress32, ServicePriority::medium, false,
                        serverTSAP_ID, //TSAP::TSAP_DMAP,
                        (*itPdu)->destinationObjectID, clientTSAP_ID, clientObject, *itPdu));
            messageDispatcher->Request(primitiveRequest);

            OperationReqID reqID = createOperationReqID(operation->getOwner(), (*itPdu)->appHandle);
            reqIDsMap[reqID] = operation;
        }
    }
    return ownerManager;
}

void SOO::deviceDeletedCallback(Address32 deletedDevAddr32, Uint16 deviceType) {

    LOG_INFO("deviceDeletedCallback - device=" << Address::toString(deletedDevAddr32) << ", deviceType=" << (int)deviceType);

    Uint16 deletedSubnetID = engine->getSubnetId(deletedDevAddr32);
    bool isBackbone = (deviceType & NE::Model::DeviceType::BACKBONE);

    for (ReqIDsMap::iterator itOperations = reqIDsMap.begin(); itOperations != reqIDsMap.end();) {
        bool removeOperation = false;
        if (!isBackbone) {
            removeOperation = (itOperations->second->getOwner() == deletedDevAddr32);
        } else {
            Uint16 operationSubnetID = engine->getSubnetId(itOperations->second->getOwner());
            removeOperation = (operationSubnetID == deletedSubnetID);
        }
        if (removeOperation) {
            LOG_INFO("deleteDeviceCallback deletedDevAddr32=" << Address::toString(deletedDevAddr32)
                << ", deviceType=" << DeviceType::toString(deviceType) << ", op=" << *itOperations->second);

            reqIDsMap.erase(itOperations++);
            continue;
        }
        ++itOperations;
    }
}

void SOO::confirmExecute(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
    ExecuteResponsePDUPointer response = ASL::PDUUtils::extractExecuteResponse(confirm->apduResponse);

    //check error status for DSMO.SetKey responses
    if (confirm->serverTSAP_ID == TSAP::TSAP_DMAP && confirm->serverObject == ObjectID::ID_DSMO) {
        if (response->parameters->size() > 0) { //responses for delete key come from DSMO but parameters field is empty
            if ((int) response->parameters->at(0) != 0) {
                LOG_ERROR("Received response for DSMO.SetKey with status=" << (int) response->parameters->at(0)
                            << ". Probably keys queue full on device.");
            }
        }
    }

    processConfirm(confirm->serverNetworkAddress, confirm->apduResponse->appHandle, response->feedbackCode, BytesPointer(
                new Bytes()));
}

void SOO::confirmWrite(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
    WriteResponsePDUPointer response = ASL::PDUUtils::extractWriteResponse(confirm->apduResponse);
    processConfirm(confirm->serverNetworkAddress, confirm->apduResponse->appHandle, response->feedbackCode, BytesPointer(
                new Bytes()));
}

void SOO::confirmRead(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
    ReadResponsePDUPointer response = ASL::PDUUtils::extractReadResponse(confirm->apduResponse);
    processConfirm(confirm->serverNetworkAddress, confirm->apduResponse->appHandle, response->feedbackCode, response->value);
}

void SOO::processConfirm(const Address128& serverAddress128, AppHandle appHandle, SFC::SFCEnum feedbackCode,
            const BytesPointer& value) {

    //Address32 serverAddress32 = SmAddressTable::instance().getAddress32(serverAddress128);
    Address32 serverAddress32 = engine->getAddress32(serverAddress128);
    if (!engine->existsDevice(serverAddress32)) {
        LOG_WARN(LOG_OI << "Confirm received from a removed device=" << serverAddress128.toString() << "; ReqID="
                    << (int) appHandle);
        return;
    }

    OperationReqID reqID = createOperationReqID(serverAddress32, appHandle);
    ReqIDsMap::iterator it = reqIDsMap.find(reqID);

    if (it == reqIDsMap.end()) {
        LOG_ERROR("Unexpected confirm received : address=" << Address::toString(serverAddress32) << ", appH=" << (int) appHandle);
        return;
    }

    // SFC to OperationError
    NE::Model::Operations::OperationErrorCode::OperationErrorCodeEnum errorCode = NE::Model::Operations::OperationErrorCode::ERROR;
    if (feedbackCode == SFC::success || feedbackCode == SFC::inappropriateProcessMode || feedbackCode == SFC::invalidAtrribute) {
        if (feedbackCode == SFC::inappropriateProcessMode){//this can happen on the HRCO configuration
            LOG_WARN("Confirm with " << feedbackCode << " considered success.");
        } else if (feedbackCode == SFC::invalidAtrribute){ // this can happen when reading custom attribute (DMO.64, DMO.67 for honeywell devices)
            LOG_WARN("Confirm with " << feedbackCode << " considered success.");
        }
        errorCode = NE::Model::Operations::OperationErrorCode::SUCCESS;
        if (it->second->getType() == NE::Model::Operations::EngineOperationType::READ_ATTRIBUTE) {
            unmarshallReadResponse(value, it->second);
        }
    } else {
        LOG_ERROR("confirm received : address=" << Address::toString(serverAddress32)
            << ", appH=" << (int) appHandle << ", sfc=" << (int)feedbackCode);

        if (feedbackCode == SFC::timeout) {
            errorCode = NE::Model::Operations::OperationErrorCode::TIMEOUT;
        }
    }

    it->second->setErrorCode(errorCode);

    NE::Model::Operations::IEngineOperationPointer confirmedOperation = it->second;
    // must be here - because on confirm with sfc=fail the engine->getOperationsProcessor().confirm(confirmedOperation)
    // call SOO::deleteDeviceCallback that delete all operations with owner = confirmedOperation
    reqIDsMap.erase(it);

    // because SOO::sendOperation - return bool !!!
    if (serverAddress32 != engine->getManagerAddress32()) {
        engine->getOperationsProcessor().confirm(confirmedOperation);
    }
}

void SOO::unmarshallReadResponse(const BytesPointer& value, NE::Model::Operations::IEngineOperationPointer operation) {

    NetworkOrderStream stream(*value);

    EntityType::EntityTypeEnum entityType = getEntityType(operation->getEntityIndex());

    switch (entityType) {
        case EntityType::Neighbour: { // Add Neighbour
            PhyNeighbor* neighbor = new PhyNeighbor();
            unmarshallEntity(stream, *neighbor);
            operation->setPhysicalEntity(neighbor);
            break;
        }
        case EntityType::Route: { // Add Route
            PhyRoute* route = new PhyRoute();
            unmarshallEntity(stream, *route);
            operation->setPhysicalEntity(route);
            break;
        }
        case EntityType::Link: { // Add Link
            PhyLink* link = new PhyLink();
            unmarshallEntity(stream, *link);
            operation->setPhysicalEntity(link);
            break;
        }
        case EntityType::Superframe: { // Add Superframe
            PhySuperframe* superframe = new PhySuperframe();
            unmarshallEntity(stream, *superframe);
            operation->setPhysicalEntity(superframe);
            break;
        }
        case EntityType::Graph: { // Add Graph

            PhyGraph* graph = new PhyGraph();
            unmarshallEntity(stream, *graph);
            operation->setPhysicalEntity(graph);
            break;
        }
        case EntityType::SerialNumber:
        case EntityType::Vendor_ID:
        case EntityType::Model_ID:
        case EntityType::Software_Revision_Information:
        case EntityType::JoinReason: {
            PhyBytes* phyBytes = new PhyBytes();
            unmarshallEntity(*value, *phyBytes);
            operation->setPhysicalEntity(phyBytes);
            break;
        }
        case EntityType::PackagesStatistics: {
            PhyBytes* phyBytes = new PhyBytes();
            unmarshallEntity(*value, *phyBytes);
            operation->setPhysicalEntity(phyBytes);
            break;
        }
        case EntityType::ContractsTable_MetaData:
        case EntityType::Neighbor_MetaData:
        case EntityType::Superframe_MetaData:
        case EntityType::Graph_MetaData:
        case EntityType::Link_MetaData:
        case EntityType::Route_MetaData:
        case EntityType::Diag_MetaData: {
            PhyMetaData* phyMetaData = new PhyMetaData();
            unmarshallEntity(stream, *phyMetaData);
            operation->setPhysicalEntity(phyMetaData);
            break;
        }
        case EntityType::PowerSupply: {
            PhyUint8* phyUint8 = new PhyUint8();
            unmarshallEntity(stream, *phyUint8);
            operation->setPhysicalEntity(phyUint8);
            break;
        }
        case EntityType::AssignedRole: {
            PhyUint16* phyUint16 = new PhyUint16();
            unmarshallEntity(stream, *phyUint16);
            operation->setPhysicalEntity(phyUint16);
            break;
        }
        case EntityType::DLMO_MaxLifetime: {
            PhyUint16* phyUint16 = new PhyUint16();
            unmarshallEntity(stream, *phyUint16);
            operation->setPhysicalEntity(phyUint16);
            break;
        }
        case EntityType::DLMO_DiscoveryAlert: {
            PhyUint8* phyUint8 = new PhyUint8();
            unmarshallEntity(stream, *phyUint8);
            operation->setPhysicalEntity(phyUint8);
            break;
        }
        case EntityType::DeviceCapability: {
            PhyDeviceCapability* phyDeviceCapability = new PhyDeviceCapability();
            unmarshallEntity(stream, *phyDeviceCapability);
            operation->setPhysicalEntity(phyDeviceCapability);
            break;
        }
        case EntityType::DLMO_IdleChannels: {
            PhyUint16* phyUint16 = new PhyUint16();
            unmarshallEntity(stream, *phyUint16);
            operation->setPhysicalEntity(phyUint16);
            break;
        }
        case EntityType::PingInterval: {
            PhyUint16* phyUint16 = new PhyUint16();
            unmarshallEntity(stream, *phyUint16);
            operation->setPhysicalEntity(phyUint16);
            break;
        }
        default: {
            std::ostringstream streamEx;
            streamEx << "Unknown operation=" << operation;
            throw NEException(streamEx.str());
        }
    }


}

void SOO::setStackManagerSettings(NE::Model::Operations::IEngineOperationPointer& operation) {

    Isa100::Common::Objects::SFC::SFCEnum sfc = SFC::success;
    Isa100::AL::DMAP::NLMO nlmo;

    NE::Model::Device * managerDevice = engine->getDevice(operation->getOwner());

    std::ostringstream hexEntityIndexStream;
    hexEntityIndexStream << std::hex << operation->getEntityIndex();

    std::ostringstream stream;
    EntityType::EntityTypeEnum entityType = getEntityType(operation->getEntityIndex());
    NE::Model::Operations::EngineOperationType::EngineOperationTypeEnum operationType = operation->getType();

    if (entityType == EntityType::Contract) { //

        NE::Model::PhyContract* phyContract = (NE::Model::PhyContract*)operation->getPhysicalEntity();

        if (operationType == NE::Model::Operations::EngineOperationType::WRITE_ATTRIBUTE) { // Add Contract

            LOG_DEBUG("add dll Contract: " << *phyContract << "; Manager does nothing!");

        } else if (operationType == NE::Model::Operations::EngineOperationType::DELETE_ATTRIBUTE) { // Delete Contract

            LOG_DEBUG("delete dll Contract! the entity index " << hexEntityIndexStream.str() << "; Manager does nothing!");
        }
    } else if (entityType == EntityType::NetworkContract) { //

        NE::Model::PhyNetworkContract* phyNetworkContract = (NE::Model::PhyNetworkContract*)operation->getPhysicalEntity();

        if (operationType == NE::Model::Operations::EngineOperationType::WRITE_ATTRIBUTE) { // Add Contract

            sfc = nlmo.AddContract(
                        phyNetworkContract->contractID, //
                        phyNetworkContract->sourceAddress, //
                        phyNetworkContract->destinationAddress, //
                        phyNetworkContract->contract_Priority, //
                        phyNetworkContract->include_Contract_Flag, //
                        phyNetworkContract->assigned_Max_NSDU_Size, //
                        phyNetworkContract->assigned_Max_Send_Window_Size, //
                        phyNetworkContract->assignedCommittedBurst, //
                        phyNetworkContract->assignedExcessBurst);

            stream << "add Contract: " << *phyNetworkContract << ", SFC=" << sfc;
            if (sfc == Isa100::Common::Objects::SFC::success) {
                LOG_DEBUG(stream.str());
            } else {
                LOG_ERROR(stream.str());
            }

        } else if (operationType == NE::Model::Operations::EngineOperationType::DELETE_ATTRIBUTE) { // Delete Contract

            NetworkContractIndexedAttribute::iterator it = managerDevice->phyAttributes.networkContractsTable.find(operation->getEntityIndex());
            if (it != managerDevice->phyAttributes.networkContractsTable.end() && it->second.getValue())
            {
                NE::Model::PhyNetworkContract* phyNetworkContract = it->second.getValue();
                sfc = nlmo.DeleteContract(phyNetworkContract->contractID);
                stream << "delete Network Contract: " << *phyNetworkContract << ", SFC=" << sfc;
                if (sfc == Isa100::Common::Objects::SFC::success) {
                    LOG_DEBUG(stream.str());
                } else {
                    LOG_ERROR(stream.str());
                }
            } else {
                LOG_ERROR("delete Network Contract fail! the entity index " << hexEntityIndexStream.str() << " not found!");
            }
        }
    } else if (entityType == EntityType::NetworkRoute) { // NetworkRoute

        NE::Model::PhyNetworkRoute* route = (NE::Model::PhyNetworkRoute*) operation->getPhysicalEntity();
        if (operationType == NE::Model::Operations::EngineOperationType::WRITE_ATTRIBUTE) { // Add NetworkRoute

            sfc = nlmo.AddRoute(route->destination, route->nextHop, route->nwkHopLimit,
                        route->outgoingInterface ? NLMO::oiBackbone : NLMO::oiDL);

            stream << "add Route: dst=" << route->destination.toString() << ", SFC=" << sfc;
            if (sfc == Isa100::Common::Objects::SFC::success) {
                LOG_DEBUG(stream.str());
            } else {
                LOG_ERROR(stream.str());
            }
        } else if (operationType == NE::Model::Operations::EngineOperationType::DELETE_ATTRIBUTE) { // Delete NetworkRoute

            NetworkRouteIndexedAttribute::iterator it = managerDevice->phyAttributes.networkRoutesTable.find(operation->getEntityIndex());
            if (it != managerDevice->phyAttributes.networkRoutesTable.end() && it->second.getValue())
            {
                NE::Model::PhyNetworkRoute* route = it->second.getValue();
                sfc = nlmo.DeleteRoute(route->destination);

                stream << "delete Route: dst=" << route->destination.toString() << ", SFC=" << sfc;
                if (sfc == Isa100::Common::Objects::SFC::success) {
                    LOG_DEBUG(stream.str());
                } else {
                    LOG_ERROR(stream.str());
                }
            } else {
                LOG_ERROR("delete Network Route fail! the entity index " << hexEntityIndexStream.str() << " not found!");
            }
        }
    } else if (entityType == EntityType::SessionKey) { // SessionKey

        NE::Model::PhySessionKey* phySessionKey = (NE::Model::PhySessionKey*) operation->getPhysicalEntity();
        if (operationType == NE::Model::Operations::EngineOperationType::WRITE_ATTRIBUTE) { // Add SessionKey

            sfc = securityManager->setStackKeyWithPeer(phySessionKey->destination64, phySessionKey->destination128,
                        (TSAP::TSAP_Enum) phySessionKey->sourceTSAP, (TSAP::TSAP_Enum) phySessionKey->destinationTSAP,
                        phySessionKey->key, phySessionKey->keyID, phySessionKey->sessionKeyPolicy);

            stream << "add SessionKey: " << *phySessionKey << ", SFC=" << sfc;
            if (sfc == Isa100::Common::Objects::SFC::success) {
                LOG_DEBUG(stream.str());
            } else {
                LOG_ERROR(stream.str());
            }
        } else if (operationType == NE::Model::Operations::EngineOperationType::DELETE_ATTRIBUTE) { // Delete SessionKey

            SessionKeyIndexedAttribute::iterator it = managerDevice->phyAttributes.sessionKeysTable.find(operation->getEntityIndex());
            if (it != managerDevice->phyAttributes.sessionKeysTable.end() && it->second.getValue())
            {
                phySessionKey = it->second.getValue();
                sfc = securityManager->deleteStackKey(phySessionKey->destination128, (TSAP::TSAP_Enum) phySessionKey->sourceTSAP,
                            (TSAP::TSAP_Enum) phySessionKey->destinationTSAP, phySessionKey->keyID);

                stream << "delete SessionKey: " << *phySessionKey << ", SFC=" << sfc;

                if (sfc == Isa100::Common::Objects::SFC::success) {
                    LOG_DEBUG(stream.str());
                } else {
                    LOG_ERROR(stream.str());
                }
            } else {
                LOG_ERROR("delete SessionKey fail! the entity index " << hexEntityIndexStream.str() << " not found!");
            }
        }
    } else {
        LOG_ERROR("Wrong operation for SM : " << *operation);
    }
}

}
}
}
