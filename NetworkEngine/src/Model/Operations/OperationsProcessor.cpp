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

#include <boost/bind.hpp>
#include "OperationsProcessor.h"
#include "boost/shared_ptr.hpp"
#include "Common/NEException.h"
#include "WriteAttributeOperation.h"
#include "DeleteAttributeOperation.h"
#include "UpdateGraphOperation.h"
#include "SMState/SMStateLog.h"
#include "Model/SubnetsContainer.h"
#include "Model/ChainWaitForConfirmOnEvalGraph.h"

namespace NE {
namespace Model {
namespace Operations {

using namespace NE::Model::Operations;

#define CAN_SEND_ATTRIBUTE(attribute)\
    if (device->phyAttributes.attribute.isOnPending()) {\
        return ProcessedOperationStatus::EXISTS_DEPENDENCIES;\
    }\

/**
 * Checks an indexed attribute.
 */
#define CAN_SEND(attributeTable, AttributeTableType) {\
    AttributeTableType::iterator it = device->phyAttributes.attributeTable.find(entityIndex);\
    if (it == device->phyAttributes.attributeTable.end() ) {\
        return statusIfNotFound;\
    } \
    if (it->second.isOnPending() ) {\
         return ProcessedOperationStatus::EXISTS_DEPENDENCIES;\
    } \
}

/**
 * Creates an indexed attribute if does not exist or sets the pending value.
 */
#define UPDATE_ENTITY(attributeTable, AttributeTableType, PhyAttribute, AddPendingMethod){\
    AttributeTableType::iterator it = device->phyAttributes.attributeTable.find(entityIndex);\
    if (it == device->phyAttributes.attributeTable.end()) {\
        if (operation->getType() == EngineOperationType::WRITE_ATTRIBUTE) {\
            device->phyAttributes.AddPendingMethod(entityIndex, (PhyAttribute*) operation->getPhysicalEntity(), true);\
        } else if (operation->getType() == EngineOperationType::READ_ATTRIBUTE) {\
            /*device->phyAttributes.AddPendingMethod(entityIndex, NULL, true);*/\
            return false;/* read something that does not exist; do nothing ... yet*/\
        } else if (operation->getType() == EngineOperationType::DELETE_ATTRIBUTE) {\
            return false;/* delete something that does not exist; do nothing ...*/\
        }\
    } else {\
        if (operation->getType() == EngineOperationType::WRITE_ATTRIBUTE) {\
            delete it->second.previousValue; \
            it->second.previousValue = it->second.currentValue; \
            it->second.currentValue = (PhyAttribute*) operation->getPhysicalEntity();\
        } \
        it->second.isPending = true;\
    }}

/**
 * Updates an unindexed attribute.
 */
#define UPDATE_ENTITY_ATTRIBUTE(attribute,  PhyAttribute) {\
    if (operation->getType() == EngineOperationType::WRITE_ATTRIBUTE) {\
        delete device->phyAttributes.attribute.previousValue; \
        device->phyAttributes.attribute.previousValue = device->phyAttributes.attribute.currentValue; \
        device->phyAttributes.attribute.currentValue = (PhyAttribute*) operation->getPhysicalEntity();\
    } else if (operation->getType() == EngineOperationType::DELETE_ATTRIBUTE) {\
        LOG_WARN("You can not delete an unindexed attribute!");\
        return false; \
    } \
   device->phyAttributes.attribute.isPending = true; \
}

/**
 * Commit operation on an un indexed operation.
 */
#define COMMIT_OPERATION_ATTRIBUTE(attribute, PhyAttribute) { \
    device->phyAttributes.attribute.isPending = false;\
    if (operation->getType() == EngineOperationType::WRITE_ATTRIBUTE) {\
        delete device->phyAttributes.attribute.previousValue;\
        device->phyAttributes.attribute.previousValue = NULL;\
    } else if (operation->getType() == EngineOperationType::READ_ATTRIBUTE) {\
        delete device->phyAttributes.attribute.currentValue;\
        device->phyAttributes.attribute.currentValue = (PhyAttribute*) operation->getPhysicalEntity();\
    } \
}

/**
 *
 * Commit operation on an indexed operation.
 */
#define COMMIT_OPERATION(attributeTable, AttributeTableType, PhyAttribute) {\
    AttributeTableType::iterator it;\
    it = device->phyAttributes.attributeTable.find(operation->getEntityIndex());\
    if (it == device->phyAttributes.attributeTable.end()) {\
        if (!device->capabilities.isManager()) {\
            LOG_ERROR("Attribute not found; entityIndex=" << std::hex << operation->getEntityIndex() << "; Can not commit.");\
        } else {\
            LOG_INFO("Attribute not found; entityIndex=" << std::hex << operation->getEntityIndex() << "; Can not commit.");\
        }\
        if (operation->getType() == EngineOperationType::DELETE_ATTRIBUTE) {\
            return true;\
        } else {\
            return false;\
        }\
    }\
    it->second.isPending = false;\
    if (operation->getType() == EngineOperationType::WRITE_ATTRIBUTE) {\
        delete it->second.previousValue;\
        it->second.previousValue = NULL;\
    } else if (operation->getType() == EngineOperationType::READ_ATTRIBUTE) {\
        delete it->second.currentValue;\
        it->second.currentValue = (PhyAttribute*) operation->getPhysicalEntity();\
    } else if (operation->getType() == EngineOperationType::DELETE_ATTRIBUTE) {\
        delete it->second.currentValue;\
        it->second.currentValue = NULL;\
        delete it->second.previousValue;\
        it->second.previousValue = NULL;\
        device->phyAttributes.attributeTable.erase(it);\
    } \
}

#define ADD_OPERATION_WAITING_LIST_INDEXED(attributeTable, AttributeTableType) {\
    AttributeTableType::iterator it;\
    it = device->phyAttributes.attributeTable.find(operation.getEntityIndex());\
    if (it != device->phyAttributes.attributeTable.end()) {\
        it->second.addWaitingOperation(operation.getOpID());\
    }\
}

#define ADD_OPERATION_WAITING_LIST(attributeTable, AttributeTableType) {\
    device->phyAttributes.attributeTable.addWaitingOperation(operation.getOpID());\
}
#define IS_FIRST_IN_WAITING_LIST_INDEXED(attributeTable, AttributeTableType) {\
    AttributeTableType::iterator it;\
    it = device->phyAttributes.attributeTable.find(operation.getEntityIndex());\
    if (it != device->phyAttributes.attributeTable.end()) {\
    	bool isFirst = it->second.isFirstOperation(operation.getOpID());\
        return isFirst;\
    }\
    return true;\
}

#define IS_FIRST_IN_WAITING_LIST(attributeTable, AttributeTableType) {\
    return device->phyAttributes.attributeTable.isFirstOperation(operation.getOpID());\
}



OperationsProcessor::OperationsProcessor(NE::Model::SubnetsContainer& subnetsContainer_) :
    deviceRemover(NULL), alertSender(NULL), subnetsContainer(subnetsContainer_) {
}

OperationsProcessor::~OperationsProcessor() {
}

void OperationsProcessor::addManagerOperation(NE::Model::Operations::IEngineOperationPointer operation) {
    assert(operation->getOwner() == subnetsContainer.manager->address32
                && "addManagerOperation called for other device than manager.");

    operation->setContainerId(0);
    addOperationToWaitingList(*operation);
    sendSingleOperation(operation);
}

void OperationsProcessor::addOperationsContainer(NE::Model::Operations::OperationsContainerPointer container) {

    if (container->isContainerEmpty()) {
        container->handleEndContainer(true);
        return;
    }

    char message[35 + container->getReason().length()];
    sprintf(message, "REASON containerId: %d %s", container->getContainerId(), container->getReason().c_str());
    SMState::SMStateLog::logOperationReason(message);
    LOG_INFO("addOperationsContainer() : added container " << container->getContainerId() << " " << message);
    LOG_DEBUG(*container);

    // must be here because in case the container contains an operation that can not be sent the container is invalidated
    // and it should not be added to the list of containers
    listOfContainers.push_back( container );

    OperationsList& unsentOperations = container->getUnsentOperations();
    OperationsList::iterator it = unsentOperations.begin();
    for (; it != unsentOperations.end(); ++it) {
        addOperationToWaitingList(**it);
        addDirectDependencies( **it );
    }

    LOG_DEBUG("addOperationsContainer() : after dependency " << container->getContainerId() << " " << message);
    LOG_DEBUG(*container);

    OperationsList confirmedOperations;

    bool containerFinished = sendAllContainerOperations( *container, confirmedOperations );

            // clean confirmed operations
    OperationsList::iterator itOperation;
    for ( itOperation = confirmedOperations.begin(); itOperation != confirmedOperations.end();) {
        if ((*itOperation)->getOwner() != subnetsContainer.manager->address32) {//operations for manager are already confirmed
            confirm( *itOperation, true ); // escape sending here
        }
        itOperation = confirmedOperations.erase( itOperation );
    }

    if( containerFinished )
    {
        //listOfContainers.pop_back();
        listOfContainers.remove(container);
        container->handleEndContainer();
    }
}

void OperationsProcessor::addOperationToWaitingList(NE::Model::Operations::IEngineOperation & operation ){
    Device * device = subnetsContainer.getDevice(operation.getOwner());
    if (device == NULL){
        return;
    }
    EntityType::EntityTypeEnum entityType = getEntityType(operation.getEntityIndex());

    switch( entityType ) {

    case EntityType::Link:
        ADD_OPERATION_WAITING_LIST_INDEXED(linksTable, LinkIndexedAttribute);
        break;
    case EntityType::Superframe:
        ADD_OPERATION_WAITING_LIST_INDEXED(superframesTable, SuperframeIndexedAttribute);
        break;
    case EntityType::Neighbour:
        ADD_OPERATION_WAITING_LIST_INDEXED(neighborsTable, NeighborIndexedAttribute);
        break;
    case EntityType::Candidate:
        ADD_OPERATION_WAITING_LIST_INDEXED(candidatesTable, CandidateIndexedAttribute);
        break;
    case EntityType::Route:
        ADD_OPERATION_WAITING_LIST_INDEXED(routesTable, RouteIndexedAttribute);
        break;
    case EntityType::NetworkRoute:
        ADD_OPERATION_WAITING_LIST_INDEXED(networkRoutesTable, NetworkRouteIndexedAttribute);
        break;
    case EntityType::Graph:
        ADD_OPERATION_WAITING_LIST_INDEXED(graphsTable, GraphIndexedAttribute);
        break;
    case EntityType::Contract:
        ADD_OPERATION_WAITING_LIST_INDEXED(contractsTable, ContractIndexedAttribute);
        break;
    case EntityType::NetworkContract:
        ADD_OPERATION_WAITING_LIST_INDEXED(networkContractsTable, NetworkContractIndexedAttribute);
        break;
    case EntityType::AdvJoinInfo:
        ADD_OPERATION_WAITING_LIST(advInfo, PhyAdvJoinInfo);
        break;
    case EntityType::AdvSuperframe:
        ADD_OPERATION_WAITING_LIST(advSuperframe, PhyUint16);
        break;
    case EntityType::SessionKey:
        ADD_OPERATION_WAITING_LIST_INDEXED(sessionKeysTable, SessionKeyIndexedAttribute);
        break;
    case EntityType::MasterKey:
        ADD_OPERATION_WAITING_LIST_INDEXED(masterKeysTable, SpecialKeyIndexedAttribute);
        break;
    case EntityType::SubnetKey:
        ADD_OPERATION_WAITING_LIST_INDEXED(subnetKeysTable, SpecialKeyIndexedAttribute);
        break;
    case EntityType::PowerSupply:
        ADD_OPERATION_WAITING_LIST(powerSupplyStatus, PhyUint8);
        break;
    case EntityType::AssignedRole:
        ADD_OPERATION_WAITING_LIST(assignedRole, PhyUint16);
        break;
    case EntityType::SerialNumber:
        ADD_OPERATION_WAITING_LIST(serialNumber, PhyString);
        break;
    case EntityType::AddressTranslation:
        ADD_OPERATION_WAITING_LIST_INDEXED(addressTranslationTable, AddressTranslationIndexedAttribute);
        break;
    case EntityType::ClientServerRetryTimeout:
        ADD_OPERATION_WAITING_LIST(retryTimeout, PhyUint16);
        break;
    case EntityType::ClientServerRetryMaxTimeoutInterval:
        ADD_OPERATION_WAITING_LIST(max_Retry_Timeout_Interval, PhyUint16);
        break;
    case EntityType::ClientServerRetryCount:
        ADD_OPERATION_WAITING_LIST(retryCount, PhyUint8);
        break;
    case EntityType::Channel:
        throw NE::Common::NEException("Not implemented yet!");
        break;
    case EntityType::BlackListChannels:
        ADD_OPERATION_WAITING_LIST_INDEXED(blacklistChannelsTable, BlacklistChannelsIndexedAttribute);
        break;
    case EntityType::ChannelHopping:
        ADD_OPERATION_WAITING_LIST_INDEXED(channelHoppingTable, ChannelHoppingIndexedAttribute);
        break;
    case EntityType::Vendor_ID:
        ADD_OPERATION_WAITING_LIST(vendorID, PhyString);
        break;
    case EntityType::Model_ID:
        ADD_OPERATION_WAITING_LIST(modelID, PhyString);
        break;
    case EntityType::Software_Revision_Information:
        ADD_OPERATION_WAITING_LIST(softwareRevisionInformation, PhyString);
        break;
    case EntityType::PackagesStatistics:
        ADD_OPERATION_WAITING_LIST(packagesStatistics, PhyBytes);
        break;
    case EntityType::JoinReason:
        ADD_OPERATION_WAITING_LIST(joinReason, PhyBytes);
        break;
    case EntityType::ContractsTable_MetaData:
        ADD_OPERATION_WAITING_LIST(contractsTableMetadata, PhyMetaData);
        break;
    case EntityType::Neighbor_MetaData:
        ADD_OPERATION_WAITING_LIST(neighborMetadata, PhyMetaData);
        break;
    case EntityType::Superframe_MetaData:
        ADD_OPERATION_WAITING_LIST(superframeMetadata, PhyMetaData);
        break;
    case EntityType::Graph_MetaData:
        ADD_OPERATION_WAITING_LIST(graphMetadata, PhyMetaData);
        break;
    case EntityType::Link_MetaData:
        ADD_OPERATION_WAITING_LIST(linkMetadata, PhyMetaData);
        break;
    case EntityType::Route_MetaData:
        ADD_OPERATION_WAITING_LIST(routeMetadata, PhyMetaData);
        break;
    case EntityType::Diag_MetaData:
        ADD_OPERATION_WAITING_LIST(diagMetadata, PhyMetaData);
        break;
    case EntityType::ChannelDiag:
        throw NE::Common::NEException("Not implemented yet!");
        break;
    case EntityType::NeighborDiag:
        throw NE::Common::NEException("Not implemented yet!");
        break;
    case EntityType::HRCO_CommEndpoint:
        ADD_OPERATION_WAITING_LIST(hrcoCommEndpoint, PhyCommunicationAssociationEndpoint);
        break;
    case EntityType::HRCO_Publish:
        ADD_OPERATION_WAITING_LIST(hrcoEntityIndexListAttribute, PhyEntityIndexList);
        break;
    case EntityType::DeviceCapability:
        ADD_OPERATION_WAITING_LIST(deviceCapability, PhyDeviceCapability);
        break;
    case EntityType::QueuePriority:
    	ADD_OPERATION_WAITING_LIST(queuePriority, PhyQueuePriority);
    	break;
    case EntityType::ARMO_CommEndpoint:
        ADD_OPERATION_WAITING_LIST(armoCommEndpoint, PhyAlertCommunicationEndpoint);
        break;
    case EntityType::DLMO_MaxLifetime:
        ADD_OPERATION_WAITING_LIST(dlmoMaxLifeTime, PhyUint16);
        break;
    case EntityType::DLMO_IdleChannels:
        ADD_OPERATION_WAITING_LIST(dlmoIdleChannels, PhyUint16);
        break;
    case EntityType::PingInterval:
        ADD_OPERATION_WAITING_LIST(pingInterval, PhyUint16);
        break;
    case EntityType::DLMO_DiscoveryAlert:
        ADD_OPERATION_WAITING_LIST(dlmoDiscoveryAlert, PhyUint8);
        break;

    default:
        throw NE::Common::NEException("Entity type not treated!");
    }
}

bool OperationsProcessor::isFirstInWaitingList(NE::Model::Operations::IEngineOperation & operation ){
    Device * device = subnetsContainer.getDevice(operation.getOwner());
    if (device == NULL){
        return true;
    }
    EntityType::EntityTypeEnum entityType = getEntityType(operation.getEntityIndex());

    switch( entityType ) {

    case EntityType::Link:
        IS_FIRST_IN_WAITING_LIST_INDEXED(linksTable, LinkIndexedAttribute);
        break;
    case EntityType::Superframe:
        IS_FIRST_IN_WAITING_LIST_INDEXED(superframesTable, SuperframeIndexedAttribute);
        break;
    case EntityType::Neighbour:
        IS_FIRST_IN_WAITING_LIST_INDEXED(neighborsTable, NeighborIndexedAttribute);
        break;
    case EntityType::Candidate:
        IS_FIRST_IN_WAITING_LIST_INDEXED(candidatesTable, CandidateIndexedAttribute);
        break;
    case EntityType::Route:
        IS_FIRST_IN_WAITING_LIST_INDEXED(routesTable, RouteIndexedAttribute);
        break;
    case EntityType::NetworkRoute:
        IS_FIRST_IN_WAITING_LIST_INDEXED(networkRoutesTable, NetworkRouteIndexedAttribute);
        break;
    case EntityType::Graph:
        IS_FIRST_IN_WAITING_LIST_INDEXED(graphsTable, GraphIndexedAttribute);
        break;
    case EntityType::Contract:
        IS_FIRST_IN_WAITING_LIST_INDEXED(contractsTable, ContractIndexedAttribute);
        break;
    case EntityType::NetworkContract:
        IS_FIRST_IN_WAITING_LIST_INDEXED(networkContractsTable, NetworkContractIndexedAttribute);
        break;
    case EntityType::AdvJoinInfo:
        IS_FIRST_IN_WAITING_LIST(advInfo, PhyAdvJoinInfo);
        break;
    case EntityType::AdvSuperframe:
        IS_FIRST_IN_WAITING_LIST(advSuperframe, PhyUint16);
        break;
    case EntityType::SessionKey:
        IS_FIRST_IN_WAITING_LIST_INDEXED(sessionKeysTable, SessionKeyIndexedAttribute);
        break;
    case EntityType::MasterKey:
        IS_FIRST_IN_WAITING_LIST_INDEXED(masterKeysTable, SpecialKeyIndexedAttribute);
        break;
    case EntityType::SubnetKey:
        IS_FIRST_IN_WAITING_LIST_INDEXED(subnetKeysTable, SpecialKeyIndexedAttribute);
        break;
    case EntityType::PowerSupply:
        IS_FIRST_IN_WAITING_LIST(powerSupplyStatus, PhyUint8);
        break;
    case EntityType::AssignedRole:
        IS_FIRST_IN_WAITING_LIST(assignedRole, PhyUint16);
        break;
    case EntityType::SerialNumber:
        IS_FIRST_IN_WAITING_LIST(serialNumber, PhyString);
        break;
    case EntityType::AddressTranslation:
        IS_FIRST_IN_WAITING_LIST_INDEXED(addressTranslationTable, AddressTranslationIndexedAttribute);
        break;
    case EntityType::ClientServerRetryTimeout:
        IS_FIRST_IN_WAITING_LIST(retryTimeout, PhyUint16);
        break;
    case EntityType::ClientServerRetryMaxTimeoutInterval:
        ADD_OPERATION_WAITING_LIST(max_Retry_Timeout_Interval, PhyUint16);
        break;
    case EntityType::ClientServerRetryCount:
        IS_FIRST_IN_WAITING_LIST(retryCount, PhyUint8);
        break;
    case EntityType::Channel:
        throw NE::Common::NEException("Not implemented yet!");
        return false;
        break;
    case EntityType::BlackListChannels:
        IS_FIRST_IN_WAITING_LIST_INDEXED(blacklistChannelsTable, BlacklistChannelsIndexedAttribute);
        break;
    case EntityType::ChannelHopping:
        IS_FIRST_IN_WAITING_LIST_INDEXED(channelHoppingTable, ChannelHoppingIndexedAttribute);
        break;
    case EntityType::Vendor_ID:
        IS_FIRST_IN_WAITING_LIST(vendorID, PhyString);
        break;
    case EntityType::Model_ID:
        IS_FIRST_IN_WAITING_LIST(modelID, PhyString);
        break;
    case EntityType::Software_Revision_Information:
        IS_FIRST_IN_WAITING_LIST(softwareRevisionInformation, PhyString);
        break;
    case EntityType::PackagesStatistics:
        IS_FIRST_IN_WAITING_LIST(packagesStatistics, PhyBytes);
        break;
    case EntityType::JoinReason:
        IS_FIRST_IN_WAITING_LIST(joinReason, PhyBytes);
        break;
    case EntityType::ContractsTable_MetaData:
        IS_FIRST_IN_WAITING_LIST(contractsTableMetadata, PhyMetaData);
        break;
    case EntityType::Neighbor_MetaData:
        IS_FIRST_IN_WAITING_LIST(neighborMetadata, PhyMetaData);
        break;
    case EntityType::Superframe_MetaData:
        IS_FIRST_IN_WAITING_LIST(superframeMetadata, PhyMetaData);
        break;
    case EntityType::Graph_MetaData:
        IS_FIRST_IN_WAITING_LIST(graphMetadata, PhyMetaData);
        break;
    case EntityType::Link_MetaData:
        IS_FIRST_IN_WAITING_LIST(linkMetadata, PhyMetaData);
        break;
    case EntityType::Route_MetaData:
        IS_FIRST_IN_WAITING_LIST(routeMetadata, PhyMetaData);
        break;
    case EntityType::Diag_MetaData:
        IS_FIRST_IN_WAITING_LIST(diagMetadata, PhyMetaData);
        break;
    case EntityType::ChannelDiag:
        throw NE::Common::NEException("Not implemented yet!");
        return false;
        break;
    case EntityType::NeighborDiag:
        throw NE::Common::NEException("Not implemented yet!");
        return false;
        break;
    case EntityType::HRCO_CommEndpoint:
        IS_FIRST_IN_WAITING_LIST(hrcoCommEndpoint, PhyCommunicationAssociationEndpoint);
        break;
    case EntityType::HRCO_Publish:
        IS_FIRST_IN_WAITING_LIST(hrcoEntityIndexListAttribute, PhyEntityIndexList);
        break;
    case EntityType::DeviceCapability:
        IS_FIRST_IN_WAITING_LIST(deviceCapability, PhyDeviceCapability);
        break;
    case EntityType::QueuePriority:
        IS_FIRST_IN_WAITING_LIST(queuePriority, PhyQueuePriority);
        break;
    case EntityType::ARMO_CommEndpoint:
        IS_FIRST_IN_WAITING_LIST(armoCommEndpoint, PhyAlertCommunicationEndpoint);
        break;
    case EntityType::DLMO_MaxLifetime:
        IS_FIRST_IN_WAITING_LIST(dlmoMaxLifeTime, PhyUint16);
        break;
    case EntityType::DLMO_IdleChannels:
        IS_FIRST_IN_WAITING_LIST(dlmoIdleChannels, PhyUint16);
        break;
    case EntityType::PingInterval:
        IS_FIRST_IN_WAITING_LIST(pingInterval, PhyUint16);
        break;
    case EntityType::DLMO_DiscoveryAlert:
        IS_FIRST_IN_WAITING_LIST(dlmoDiscoveryAlert, PhyUint8);
        break;

    default:
        throw NE::Common::NEException("Entity type not treated!");
        return false;
    }
    return false;
}



SentOperationStatus::SentOperationStatusEnum OperationsProcessor::sendSingleOperation(
            NE::Model::Operations::IEngineOperationPointer operation) {

    LOG_INFO("send operation: " << *operation);
    Device * ownerDevice = subnetsContainer.getDevice(operation->getOwner());
    if( ownerDevice == NULL) {
        LOG_ERROR("Operation skipped for inexistent device: " << Address_toStream(operation->getOwner()));
    } else {
        ownerDevice->removeUnsentOperation(operation);
        if ( updatePhyEntity(operation, ownerDevice) ) {
            std::string sign = EngineOperationType::getSign(operation->getType());
            SMState::SMStateLog::logOperation(sign, operation);

            assert(sender != NULL && "No operations sender registered");
            bool sendEnded = sender->sendOperation(operation);
            updateMetadata(ownerDevice, operation);

            if( operation->getType() != EngineOperationType::READ_ATTRIBUTE )
            {
                operation->setPhysicalEntity( NULL ); // clean operation pointer assignement (because is already assigned on PHY model)
            }

            if ( !sendEnded ) {
                return SentOperationStatus::OPERATION_SENT;
            }

    		LOG_INFO("confirm operation: " << std::hex << operation->getEntityIndex());

            // the response for the operation is processed in the call back
            commitChanges(operation, ownerDevice);

        }

    }
    operation->destroyPhysicalEntity();
    return SentOperationStatus::OPERATION_SENT_AND_CONFIRMED;
}

void OperationsProcessor::updateMetadata(Device * device, NE::Model::Operations::IEngineOperationPointer& operation) {

    EntityType::EntityTypeEnum entityType = getEntityType(operation->getEntityIndex());

    switch( entityType )
    {
    case EntityType::Link:
        if (device->phyAttributes.linkMetadata.currentValue) {
            device->phyAttributes.linkMetadata.currentValue->used = device->phyAttributes.linksTable.size();
        }
        break;
    case EntityType::Superframe:
        if (device->phyAttributes.superframeMetadata.currentValue) {
            device->phyAttributes.superframeMetadata.currentValue->used = device->phyAttributes.superframesTable.size();
        }
        break;
    case EntityType::Neighbour:
        if (device->phyAttributes.neighborMetadata.currentValue) {
            device->phyAttributes.neighborMetadata.currentValue->used = device->phyAttributes.neighborsTable.size();
        }
        break;
    case EntityType::Route:
        if (device->phyAttributes.routeMetadata.currentValue) {
            device->phyAttributes.routeMetadata.currentValue->used = device->phyAttributes.routesTable.size();
        }
        break;
    case EntityType::NetworkRoute:
        if (device->phyAttributes.routeMetadata.currentValue) {
            device->phyAttributes.routeMetadata.currentValue->used = device->phyAttributes.routesTable.size();
        }
        break;
    case EntityType::Graph:
        if (device->phyAttributes.graphMetadata.currentValue) {
            device->phyAttributes.graphMetadata.currentValue->used = device->phyAttributes.graphsTable.size();
        }
        break;
    case EntityType::Contract:
        if (device->phyAttributes.contractsTableMetadata.currentValue) {
            device->phyAttributes.contractsTableMetadata.currentValue->used = device->phyAttributes.contractsTable.size();
        }
        break;
    case EntityType::NetworkContract:
        if (device->phyAttributes.contractsTableMetadata.currentValue) {
            device->phyAttributes.contractsTableMetadata.currentValue->used = device->phyAttributes.contractsTable.size();
        }
        break;
    default:
        break;
    }
}

/*
1.	Link
    a.	Superframe
        i.	Channel hopping sequence
    b.	Timeslot Template
    c.	If Tx Link: Neighbour or Group  or Graph

2.	Superframe
    a.	Channel Hopping Sequence
    Obs: an active superframe needs at least one link

3.	Graph
    a.	List of neighbours

4.	Route
    a.	Graph and/or Neighbor (depends on first entry in route table)
    b.	Contract

5.	Neighbour
    a.	Link (only if LinkBacklogIndex field is valid)

6.	Timeslot Templates                        - none
7.	Channel Hopping Sequence        - none
*/
void OperationsProcessor::addDirectDependencies(NE::Model::Operations::IEngineOperation & operation ) {

    if( operation.getContainerId() && operation.getType() == EngineOperationType::WRITE_ATTRIBUTE )
    {
        EntityIndex entityIndex = operation.getEntityIndex();
        Address32 deviceAddress = getDeviceAddress(entityIndex);
        Device* device = subnetsContainer.getDevice(deviceAddress);
        if( device )
        {
            EntityType::EntityTypeEnum entityType = getEntityType(entityIndex);
            EntityDependency & entDependency = operation.getEntityDependency();
            void * pEntity = operation.getPhysicalEntity();

            switch( entityType )
            {
            case EntityType::Link:
                entDependency.push_back( createEntityIndex( deviceAddress, EntityType::Superframe, ((PhyLink *)pEntity)->superframeIndex ) );
                if( ((PhyLink *)pEntity)->type & Tdma::LinkType::TRANSMIT )
                {
                    if( ((PhyLink *)pEntity)->neighborType == Tdma::NeighbourTypes::NEIGHBOUR_BY_ADDRESS )
                    {
                        entDependency.push_back( createEntityIndex( deviceAddress, EntityType::Neighbour, ((PhyLink *)pEntity)->neighbor ) );
                    }
                }
                // to do check for graph links
                break;
            case EntityType::Superframe:
//                not mandatory because if fail anyway will delete the device
//                entDependency.push_back( createEntityIndex( deviceAddress, EntityType::ChannelHopping, ((PhySuperframe *)pEntity)->chIndex ) );
                break;

            case EntityType::Route:
                if( ((PhyRoute *)pEntity)->alternative == 0) // route based on contract and srcAddress. used on BBR for configuring
                {
                    // to check here
                }
                else if( ((PhyRoute *)pEntity)->alternative == 1) // route based on the contract ID from selector field
                {
                    entDependency.push_back( createEntityIndex( deviceAddress, EntityType::NetworkContract, ((PhyRoute *)pEntity)->selector ) );
                }
                else if( ((PhyRoute *)pEntity)->alternative == 2) // select this route based on Address16 from selector
                {
                    Uint16 nextHop = ((PhyRoute *)pEntity)->route[0];
                    if( (nextHop & 0xF000) == 0xA000 ) // it is graph
                    {
                        entDependency.push_back( createEntityIndex( deviceAddress, EntityType::Graph, nextHop & 0x0FFF ) );
                    }
                    else // it is neighbor
                    {
                        entDependency.push_back( createEntityIndex( deviceAddress, EntityType::Neighbour, nextHop ) );
                    }
                }
                break;

            default:
                break;
            }
        }
    }
}


void OperationsProcessor::confirm(NE::Model::Operations::IEngineOperationPointer operation, bool escapeSend /*= false*/ ) {
    LOG_INFO("confirm operation: " << std::hex << operation->getEntityIndex() << ", containerID=" << operation->getContainerId() << ", escapeSend=" << escapeSend);
    std::string signConfOperation = "*";
    signConfOperation += EngineOperationType::getSign(operation->getType());
    SMState::SMStateLog::logOperationConfirm(operation);

    OperationsContainer::removeOperationInWaitingList(*operation, &subnetsContainer);

    int opContainerId = operation->getContainerId();

    if (opContainerId == 0){
        //see OperationsContainer.removeAllSentOperationsForOwner()
        //Operations marked on 0 in order to not be found on confirm in case of a fast rejoin
        //contract is removed from stack but detection of a packet that is on a inexistent contract is made only when the
        //packet has the time to be sent (when the retry time is due)
        //in this time if the device performed rejoin it will produce a false error in OperationProcessor
        LOG_WARN("ContainerId=0 on confirm. Possibly confirm on an operation for a removed device.");
        return;
    }

    Address32 devAddress = NE::Model::getDeviceAddress(operation->getEntityIndex());

    ListOfOperationsContainer::iterator itContainer = listOfContainers.begin();
    for (;;itContainer++) {
        if (itContainer == listOfContainers.end()) {
            LOG_ERROR("confirm() : Confirm received for operation from unknown container " << opContainerId );
            operation->destroyPhysicalEntity();
            return;
        }
        if( (*itContainer)->containerId == opContainerId ) // container found
        {
            break;
        }
    }


    OperationErrorCode::OperationErrorCodeEnum errCode = operation->getErrorCode();


    Device* device = this->subnetsContainer.getDevice(devAddress);
    OperationsContainerPointer container = *itContainer;

    bool bCommited = false;
    bool removeDevice = false;
    if (device ) {
        if (    (errCode == OperationErrorCode::SUCCESS)
            || ((errCode == OperationErrorCode::ERROR) && (operation->getType() == EngineOperationType::DELETE_ATTRIBUTE)) )
        {
            LOG_DEBUG("confirm() : success: code " << errCode << ",opType " << operation->getType());
            bCommited = commitChanges(operation, device);
        } else if (deviceRemover != NULL) { // error -> delete the device with the address of the operation's owner
            removeDevice = true;
//            LOG_INFO("confirm() : error : Deletes the device with the address " << Address::toString(devAddress));
//            char reason[50];
//            sprintf(reason, "remove device - confirm with error, device=%x", devAddress);
//
//            HandlerResponseList handlersConfirmGraphOperations;
//            OperationsContainerPointer containerRemDevOperations(new OperationsContainer(devAddress, 0, handlersConfirmGraphOperations, reason));
//            deviceRemover->removeDeviceOnError(devAddress, containerRemDevOperations);
//            // LOG_DEBUG("confirm() - CONTAINER(" << (int)containerRemDevOperations->containerId << ") : " << *containerRemDevOperations);
//            listOfContainers.push_back( containerRemDevOperations );
        } else {
            LOG_ERROR("confirm() : Untreated error code!");
        }
    }

    if ( !bCommited ) { // not commited operation -> destroy it
        container->setAsFail( &subnetsContainer );
        operation->destroyPhysicalEntity();
    }

    if( !escapeSend ) {
        OperationsList& sentOperations = container->getSentOperations();
        sentOperations.remove(operation);

        // try to send the cached operations
        sendOperations();
    }

    if ( !escapeSend && removeDevice ){
        LOG_INFO("confirm() : error : Deletes the device with the address " << Address::toString(devAddress));
        char reason[128];
        sprintf(reason, "remove device - confirm with error, device=%x", devAddress);

        OperationsContainerPointer containerRemDevOperations(new OperationsContainer(reason));
        if (device && device->statusForReports != StatusForReports::JOINED_AND_CONFIGURED) {
            deviceRemover->removeDeviceOnError(devAddress, containerRemDevOperations, RemoveDeviceReason::joinTimeout);
        } else {
            deviceRemover->removeDeviceOnError(devAddress, containerRemDevOperations, RemoveDeviceReason::timeout);
        }
        addOperationsContainer(containerRemDevOperations);
    }

}

bool OperationsProcessor::commitChanges(IEngineOperationPointer operation, Device * device) {

	OperationsContainer::removeOperationInWaitingList(*operation, &subnetsContainer);

    EntityType::EntityTypeEnum entityType = getEntityType(operation->getEntityIndex());

    switch( entityType )
    {
    case EntityType::Link:
        COMMIT_OPERATION(linksTable, LinkIndexedAttribute, PhyLink);
        break;
    case EntityType::Superframe:
        COMMIT_OPERATION(superframesTable, SuperframeIndexedAttribute, PhySuperframe);
        break;
    case EntityType::Neighbour:
        COMMIT_OPERATION(neighborsTable, NeighborIndexedAttribute, PhyNeighbor);
        break;
    case EntityType::Candidate:
        COMMIT_OPERATION(candidatesTable, CandidateIndexedAttribute, PhyCandidate);
        break;
    case EntityType::Route:
        COMMIT_OPERATION(routesTable, RouteIndexedAttribute, PhyRoute);
        break;
    case EntityType::NetworkRoute:
        COMMIT_OPERATION(networkRoutesTable, NetworkRouteIndexedAttribute, PhyNetworkRoute);
        break;
    case EntityType::Graph:
        COMMIT_OPERATION(graphsTable, GraphIndexedAttribute, PhyGraph);
        break;
    case EntityType::Contract:
        COMMIT_OPERATION(contractsTable, ContractIndexedAttribute, PhyContract);
        break;
    case EntityType::NetworkContract:
        COMMIT_OPERATION(networkContractsTable, NetworkContractIndexedAttribute, PhyNetworkContract);
        break;
    case EntityType::AdvJoinInfo:
        COMMIT_OPERATION_ATTRIBUTE(advInfo, PhyAdvJoinInfo);
        break;
    case EntityType::AdvSuperframe:
        COMMIT_OPERATION_ATTRIBUTE(advSuperframe, PhyUint16);
        break;
    case EntityType::SessionKey:
        COMMIT_OPERATION(sessionKeysTable, SessionKeyIndexedAttribute, PhySessionKey);
        break;
    case EntityType::MasterKey:
        COMMIT_OPERATION(masterKeysTable, SpecialKeyIndexedAttribute, PhySpecialKey);
        break;
    case EntityType::SubnetKey:
        COMMIT_OPERATION(subnetKeysTable, SpecialKeyIndexedAttribute, PhySpecialKey);
        break;
    case EntityType::PowerSupply:
        COMMIT_OPERATION_ATTRIBUTE(powerSupplyStatus, PhyUint8);
        break;
    case EntityType::AssignedRole:
        COMMIT_OPERATION_ATTRIBUTE(assignedRole, PhyUint16);
        break;
    case EntityType::SerialNumber:
        COMMIT_OPERATION_ATTRIBUTE(serialNumber, PhyString);
        break;
    case EntityType::AddressTranslation:
        COMMIT_OPERATION(addressTranslationTable, AddressTranslationIndexedAttribute, PhyAddressTranslation);
        break;
    case EntityType::ClientServerRetryTimeout:
        COMMIT_OPERATION_ATTRIBUTE(retryTimeout, PhyUint16);
        break;
    case EntityType::ClientServerRetryMaxTimeoutInterval:
        COMMIT_OPERATION_ATTRIBUTE(max_Retry_Timeout_Interval, PhyUint16);
        break;
    case EntityType::ClientServerRetryCount:
        COMMIT_OPERATION_ATTRIBUTE(retryCount, PhyUint8);
        break;
    case EntityType::Channel:
        throw NE::Common::NEException("Not implemented yet!");
        //COMMIT_OPERATION(candidatesTable, CandidateIndexedAttribute, PhyCandidate);
        break;
    case EntityType::BlackListChannels:
        COMMIT_OPERATION(blacklistChannelsTable, BlacklistChannelsIndexedAttribute, PhyBlacklistChannels);
        break;
    case EntityType::ChannelHopping:
        COMMIT_OPERATION(channelHoppingTable, ChannelHoppingIndexedAttribute, PhyChannelHopping);
        break;
    case EntityType::Vendor_ID:
        COMMIT_OPERATION_ATTRIBUTE(vendorID, PhyString);
        break;
    case EntityType::Model_ID:
        COMMIT_OPERATION_ATTRIBUTE(modelID, PhyString);
        break;
    case EntityType::Software_Revision_Information:

        COMMIT_OPERATION_ATTRIBUTE(softwareRevisionInformation, PhyString);
        break;
    case EntityType::PackagesStatistics:
        COMMIT_OPERATION_ATTRIBUTE(packagesStatistics, PhyBytes);
        break;
    case EntityType::JoinReason:
        COMMIT_OPERATION_ATTRIBUTE(joinReason, PhyBytes);
        break;
    case EntityType::ContractsTable_MetaData:
        COMMIT_OPERATION_ATTRIBUTE(contractsTableMetadata, PhyMetaData);
        break;
    case EntityType::Neighbor_MetaData:
        COMMIT_OPERATION_ATTRIBUTE(neighborMetadata, PhyMetaData);
        break;
    case EntityType::Superframe_MetaData:
        COMMIT_OPERATION_ATTRIBUTE(superframeMetadata, PhyMetaData);
        break;
    case EntityType::Graph_MetaData:
        COMMIT_OPERATION_ATTRIBUTE(graphMetadata, PhyMetaData);
        break;
    case EntityType::Link_MetaData:
        COMMIT_OPERATION_ATTRIBUTE(linkMetadata, PhyMetaData);
        break;
    case EntityType::Route_MetaData:
        COMMIT_OPERATION_ATTRIBUTE(routeMetadata, PhyMetaData);
        break;
    case EntityType::Diag_MetaData:
        COMMIT_OPERATION_ATTRIBUTE(diagMetadata, PhyMetaData);
        break;
    case EntityType::ChannelDiag:
        throw NE::Common::NEException("Not implemented yet!");
        break;
    case EntityType::NeighborDiag:
        throw NE::Common::NEException("Not implemented yet!");
        break;
    case EntityType::HRCO_CommEndpoint:
        COMMIT_OPERATION_ATTRIBUTE(hrcoCommEndpoint, PhyCommunicationAssociationEndpoint);
        break;
    case EntityType::HRCO_Publish:
        COMMIT_OPERATION_ATTRIBUTE(hrcoEntityIndexListAttribute, PhyEntityIndexList);
        break;
    case EntityType::DeviceCapability:
        COMMIT_OPERATION_ATTRIBUTE(deviceCapability, PhyDeviceCapability);
        break;
    case EntityType::QueuePriority:
        COMMIT_OPERATION_ATTRIBUTE(queuePriority, PhyQueuePriority);
        break;
    case EntityType::ARMO_CommEndpoint:
        COMMIT_OPERATION_ATTRIBUTE(armoCommEndpoint, PhyAlertCommunicationEndpoint);
        break;
    case EntityType::DLMO_MaxLifetime:
        COMMIT_OPERATION_ATTRIBUTE(dlmoMaxLifeTime, PhyUint16);
        break;
    case EntityType::DLMO_IdleChannels:
        COMMIT_OPERATION_ATTRIBUTE(dlmoIdleChannels, PhyUint16);
        break;
    case EntityType::PingInterval:
        COMMIT_OPERATION_ATTRIBUTE(pingInterval, PhyUint16);
        break;
    case EntityType::DLMO_DiscoveryAlert:
        COMMIT_OPERATION_ATTRIBUTE(dlmoDiscoveryAlert, PhyUint8);
        break;

    default:
        throw NE::Common::NEException("Entity type not treated!");
    }

    // after committing the phyEntity mark it as NULL in operation in case it gets accessed after the entity(pending or current)
    // from Device gets destroyed
    operation->setPhysicalEntity(NULL);
    if(device->capabilities.isGateway() || device->capabilities.isManager()) {
        device->hasChanged = true;
    }
    else {
        Subnet::PTR subnet = subnetsContainer.getSubnet(device->address32);
        if(subnet) {
            subnet->changedDevices.push_back(Address::getAddress16(device->address32));
            device->hasChanged = true;
        }
    }

    updateMetadata(device, operation);

    return true;
}

bool OperationsProcessor::sendAllContainerOperations( NE::Model::Operations::OperationsContainer & container, OperationsList & confirmedOperations )
{
    OperationsList& unsentOperations = container.getUnsentOperations();
    Uint32 currentTime = time(NULL);

    OperationsList::iterator itOperation = unsentOperations.begin();

    for (; itOperation != unsentOperations.end();) {
        IEngineOperationPointer oper = *itOperation;
        ProcessedOperationStatus::ProcessedOperationStatusEnum canSendStatus = canSendOperation(oper, container );

        if (canSendStatus == ProcessedOperationStatus::EXISTS_DEPENDENCIES) {
            ++itOperation;
        } else {
            if (canSendStatus == ProcessedOperationStatus::CAN_BE_SENT) {
                Address32 opDeviceAddress = getDeviceAddress((*itOperation)->getEntityIndex());
                Device* opDevice = subnetsContainer.getDevice(opDeviceAddress);
                if (!opDevice) {
                    LOG_ERROR("sendAllContainerOperations() : Device with address " << Address::toString(opDeviceAddress) << " does not exist!");
                    return ProcessedOperationStatus::DEVICE_DELETED;
                }

                opDevice->setLastTimeAccessed(currentTime);
                Device * parent = subnetsContainer.getDevice(opDevice->parent32);
                while (parent != NULL && !parent->capabilities.isBackbone()){
                    parent->setLastTimeAccessed(currentTime);
                    parent = subnetsContainer.getDevice(parent->parent32);
                }

                const SentOperationStatus::SentOperationStatusEnum status = sendSingleOperation(*itOperation);
                container.updateTimer(currentTime);

                if (status == SentOperationStatus::OPERATION_SENT) {
                    // move it to sent operations only if it was not already confirmed on send
                    container.addToSentOperations(*itOperation);
                } else if (status == SentOperationStatus::OPERATION_SENT_AND_CONFIRMED) {
                    confirmedOperations.push_back( *itOperation );
                }
            }
            else // that operation was not sent and must be destroyed
            {
                LOG_INFO("The operation was not sent and must be destroyed! operation=" << **itOperation);
                (*itOperation)->setErrorCode( OperationErrorCode::ERROR );
                confirmedOperations.push_back( *itOperation );
            }

            itOperation = unsentOperations.erase(itOperation); // unsent operation was used
        }
    }

    return container.isContainerEmpty();
}

void OperationsProcessor::sendOperations() {
    LOG_DEBUG("sendOperations()");
    OperationsList confirmedOperations;

    ListOfOperationsContainer::iterator itContainer = listOfContainers.begin();
    for (; itContainer != listOfContainers.end();) {
        //LOG_DEBUG("sendOperations() - CONTAINER(" << (int)(*itContainer)->containerId << ") : " << **itContainer);

    	if( sendAllContainerOperations( **itContainer, confirmedOperations ) )
    	{
    	    //for cases when handleEndContainer() inserts another container and this method gets called
    	    // to avoid a recursive call then: create a copy of container(pointer), erase current container from list and then call handler.
    	    OperationsContainerPointer container = *itContainer;
    	    //LOG_DEBUG("sendOperations() - erase CONTAINER(" << (int)(*itContainer)->containerId << ") : " << **itContainer);
    		itContainer = listOfContainers.erase(itContainer);
    		container->handleEndContainer();
    	}
    	else
    	{
    		++itContainer;
    	}
    }
            // clean confirmed operations
    OperationsList::iterator itOperation;
    for ( itOperation = confirmedOperations.begin(); itOperation != confirmedOperations.end();) {
        if ((*itOperation)->getOwner() != subnetsContainer.manager->address32) {//operations for manager are already confirmed
            confirm( *itOperation, true ); // escape sending here
        }
        itOperation = confirmedOperations.erase( itOperation );
    }


}

ProcessedOperationStatus::ProcessedOperationStatusEnum getSendStatusForEntity(Device* device,
                                                                              EntityIndex entityIndex,
                                                                              ProcessedOperationStatus::ProcessedOperationStatusEnum statusIfNotFound ){
    EntityType::EntityTypeEnum entityType = getEntityType(entityIndex);

    switch( entityType )
    {
    case EntityType::Link:
        CAN_SEND(linksTable, LinkIndexedAttribute);
        break;
    case EntityType::Superframe:
        CAN_SEND(superframesTable, SuperframeIndexedAttribute);
        break;
    case EntityType::Neighbour:
        CAN_SEND(neighborsTable, NeighborIndexedAttribute);
        break;
    case EntityType::Candidate:
        CAN_SEND(candidatesTable, CandidateIndexedAttribute);
        break;
    case EntityType::Route:
        CAN_SEND(routesTable, RouteIndexedAttribute);
        break;
    case EntityType::NetworkRoute:
        CAN_SEND(networkRoutesTable, NetworkRouteIndexedAttribute);
        break;
    case EntityType::Graph:
        CAN_SEND(graphsTable, GraphIndexedAttribute);
        break;
    case EntityType::Contract:
        CAN_SEND(contractsTable, ContractIndexedAttribute);
        break;
    case EntityType::NetworkContract:
        CAN_SEND(networkContractsTable, NetworkContractIndexedAttribute);
        break;
    case EntityType::AdvJoinInfo:
        CAN_SEND_ATTRIBUTE(advInfo);
        break;
    case EntityType::AdvSuperframe:
        CAN_SEND_ATTRIBUTE(advSuperframe);
        break;
    case EntityType::SessionKey:
        CAN_SEND(sessionKeysTable, SessionKeyIndexedAttribute);
        break;
    case EntityType::MasterKey:
        CAN_SEND(masterKeysTable, SpecialKeyIndexedAttribute);
        break;
    case EntityType::SubnetKey:
        CAN_SEND(subnetKeysTable, SpecialKeyIndexedAttribute);
        break;
    case EntityType::PowerSupply:
        CAN_SEND_ATTRIBUTE(powerSupplyStatus);
        break;
    case EntityType::AssignedRole:
        CAN_SEND_ATTRIBUTE(assignedRole);
        break;
    case EntityType::SerialNumber:
        CAN_SEND_ATTRIBUTE(serialNumber);
        break;
    case EntityType::AddressTranslation:
        CAN_SEND(addressTranslationTable, AddressTranslationIndexedAttribute);
        break;
    case EntityType::ClientServerRetryTimeout:
        CAN_SEND_ATTRIBUTE(retryTimeout);
        break;
    case EntityType::ClientServerRetryMaxTimeoutInterval:
        CAN_SEND_ATTRIBUTE(max_Retry_Timeout_Interval);
        break;
    case EntityType::ClientServerRetryCount:
        CAN_SEND_ATTRIBUTE(retryCount);
        break;
    case EntityType::Channel:
        throw NE::Common::NEException("Not implemented yet!");
        break;
    case EntityType::BlackListChannels:
        CAN_SEND(blacklistChannelsTable, BlacklistChannelsIndexedAttribute);
        break;
    case EntityType::ChannelHopping:
        CAN_SEND(channelHoppingTable, ChannelHoppingIndexedAttribute);
        break;
    case EntityType::Vendor_ID:
        CAN_SEND_ATTRIBUTE(vendorID);
        break;
    case EntityType::Model_ID:
        CAN_SEND_ATTRIBUTE(modelID);
        break;
    case EntityType::Software_Revision_Information:
        CAN_SEND_ATTRIBUTE(softwareRevisionInformation);
        break;
    case EntityType::PackagesStatistics:
        CAN_SEND_ATTRIBUTE(packagesStatistics);
        break;
    case EntityType::JoinReason:
        CAN_SEND_ATTRIBUTE(joinReason);
        break;
    case EntityType::ContractsTable_MetaData:
        CAN_SEND_ATTRIBUTE(contractsTableMetadata);
        break;
    case EntityType::Neighbor_MetaData:
        CAN_SEND_ATTRIBUTE(neighborMetadata);
        break;
    case EntityType::Superframe_MetaData:
        CAN_SEND_ATTRIBUTE(superframeMetadata);
        break;
    case EntityType::Graph_MetaData:
        CAN_SEND_ATTRIBUTE(graphMetadata);
        break;
    case EntityType::Link_MetaData:
        CAN_SEND_ATTRIBUTE(linkMetadata);
        break;
    case EntityType::Route_MetaData:
        CAN_SEND_ATTRIBUTE(routeMetadata);
        break;
    case EntityType::Diag_MetaData:
        CAN_SEND_ATTRIBUTE(diagMetadata);
        break;
    case EntityType::ChannelDiag:
        throw NE::Common::NEException("Not implemented yet!");
        break;
    case EntityType::NeighborDiag:
        throw NE::Common::NEException("Not implemented yet!");
        break;
    case EntityType::HRCO_CommEndpoint:
        CAN_SEND_ATTRIBUTE(hrcoCommEndpoint);
        break;
    case EntityType::HRCO_Publish:
        CAN_SEND_ATTRIBUTE(hrcoEntityIndexListAttribute);
        break;
    case EntityType::DeviceCapability:
        CAN_SEND_ATTRIBUTE(deviceCapability);
        break;
    case EntityType::QueuePriority:
        CAN_SEND_ATTRIBUTE(queuePriority);
        break;
    case EntityType::ARMO_CommEndpoint:
        CAN_SEND_ATTRIBUTE(armoCommEndpoint);
        break;
    case EntityType::DLMO_MaxLifetime:
        CAN_SEND_ATTRIBUTE(dlmoMaxLifeTime);
        break;
    case EntityType::DLMO_IdleChannels:
        CAN_SEND_ATTRIBUTE(dlmoIdleChannels);
        break;
    case EntityType::PingInterval:
        CAN_SEND_ATTRIBUTE(pingInterval);
        break;
    case EntityType::DLMO_DiscoveryAlert:
        CAN_SEND_ATTRIBUTE(dlmoDiscoveryAlert);
        break;

    default:
//        LOG_ERROR("canSendOperation() : Unknown physical entity type : " << (int) entityType);
        return ProcessedOperationStatus::DEVICE_DELETED;
    }

    return ProcessedOperationStatus::CAN_BE_SENT;
}


ProcessedOperationStatus::ProcessedOperationStatusEnum OperationsProcessor::canSendOperation(
                                NE::Model::Operations::IEngineOperationPointer operation,
                                OperationsContainer & container ) {
    // IF the device with address of the owner of the operation does not exist
    // THEN this operation must not be sent and deleted from the list of operations.

    Address32 opDeviceAddress = getDeviceAddress(operation->getEntityIndex());
    Device* opDevice = subnetsContainer.getDevice(opDeviceAddress);
    if (!opDevice) {
        LOG_ERROR("canSendOperation() : Device with address " << Address::toString(opDeviceAddress) << " does not exist!");
        return ProcessedOperationStatus::DEVICE_DELETED;
    }

    if (opDevice->capabilities.isManager()){
        return ProcessedOperationStatus::CAN_BE_SENT;
    }


    // check operations dependency
    if (operation->getContainerId() != 0) {
        OperationsList& sentOperations = container.getSentOperations();
        OperationDependency & operationDependencies = operation->getOperationDependency();
        OperationDependency::iterator itOpDep = operationDependencies.begin();

        for (; itOpDep != operationDependencies.end(); ) {

            NE::Model::Operations::IEngineOperation * dependencyPointer = itOpDep->get();
            // check depenedency with sent operations
            OperationsList::iterator itSentOperation = sentOperations.begin();
            for (; itSentOperation != sentOperations.end(); ++itSentOperation) {
                if ( dependencyPointer == itSentOperation->get() ) { // point to same operation
                    return ProcessedOperationStatus::EXISTS_DEPENDENCIES;
                }
            }

            // check if depenedency with unsent operations
            OperationsList& operations = container.getUnsentOperations();
            OperationsList::iterator itUnsentOperation = operations.begin();
            for (; itUnsentOperation != operations.end(); ++itUnsentOperation) {
                if ( dependencyPointer == itUnsentOperation->get() ) {
                    return ProcessedOperationStatus::EXISTS_DEPENDENCIES;
                }
            }

            itOpDep = operationDependencies.erase( itOpDep ); // depedency resolved
        }
    }

    ProcessedOperationStatus::ProcessedOperationStatusEnum entityStatus
            = getSendStatusForEntity(opDevice, operation->getEntityIndex(),ProcessedOperationStatus::CAN_BE_SENT);

    if (entityStatus != ProcessedOperationStatus::CAN_BE_SENT){
        return entityStatus;
    }

    // CHECK entity dependencies
    EntityDependency & dependencies = operation->getEntityDependency();
    EntityDependency::iterator itDep = dependencies.begin();
    for (; itDep != dependencies.end(); ++itDep) {
        Address32 deviceAddress = getDeviceAddress(*itDep);
        Device* device = subnetsContainer.getDevice(deviceAddress);
        if (!device) {
            LOG_ERROR("canSendOperation() : Device with address " << deviceAddress << " does not exist!");
            return ProcessedOperationStatus::DEVICE_DELETED;
        }
        entityStatus = getSendStatusForEntity(device, *itDep,ProcessedOperationStatus::EXISTS_DEPENDENCIES);
        if (entityStatus != ProcessedOperationStatus::CAN_BE_SENT){
            return entityStatus;
        }
    }

    if (!isFirstInWaitingList(*operation)){
        return ProcessedOperationStatus::EXISTS_DEPENDENCIES;
    }

    return ProcessedOperationStatus::CAN_BE_SENT;
}

bool OperationsProcessor::updatePhyEntity(IEngineOperationPointer operation, Device * device) {
    EntityIndex entityIndex = operation->getEntityIndex();

    EntityType::EntityTypeEnum entityType = getEntityType(entityIndex);

    EngineOperationType::EngineOperationTypeEnum opType = operation->getType();
    if (    (opType == EngineOperationType::WRITE_ATTRIBUTE)
        ||  (opType == EngineOperationType::READ_ATTRIBUTE)
        ||  (opType == EngineOperationType::DELETE_ATTRIBUTE) )
    {
        LOG_DEBUG("Device CHANGED - " << Address::toString(device->address32));
        if(device->capabilities.isGateway() || device->capabilities.isManager()) {
            device->hasChanged = true;
        }

        else {
            Subnet::PTR subnet = subnetsContainer.getSubnet(device->address32);
            if(subnet) {
                subnet->changedDevices.push_back(Address::getAddress16(device->address32));
                device->hasChanged = true;
            }
        }

        switch( entityType )
        {
        case EntityType::Link:
            UPDATE_ENTITY(linksTable, LinkIndexedAttribute, PhyLink, createLink);
            break;
        case EntityType::Superframe:
            UPDATE_ENTITY(superframesTable, SuperframeIndexedAttribute, PhySuperframe, createSuperframe);
            break;
        case EntityType::Neighbour:
            UPDATE_ENTITY(neighborsTable, NeighborIndexedAttribute, PhyNeighbor, createNeighbor);
            break;
        case EntityType::Candidate:
            UPDATE_ENTITY(candidatesTable, CandidateIndexedAttribute, PhyCandidate, createCandidate);
            break;
        case EntityType::Route:
            UPDATE_ENTITY(routesTable, RouteIndexedAttribute, PhyRoute, createRoute);
            break;
        case EntityType::NetworkRoute:
            UPDATE_ENTITY(networkRoutesTable, NetworkRouteIndexedAttribute, PhyNetworkRoute, createNetworkRoute);
            break;
        case EntityType::Graph:
            if (operation->getType() == EngineOperationType::DELETE_ATTRIBUTE) {
                if (getIndex(entityIndex) == DEFAULT_GRAPH_ID){
                    LOG_ERROR("DELETION OF GRAPH 1 NOT ALLOWED. Oper: " << *operation);
                }
            }

            UPDATE_ENTITY(graphsTable, GraphIndexedAttribute, PhyGraph, createGraph);
            break;
        case EntityType::Contract:
            UPDATE_ENTITY(contractsTable, ContractIndexedAttribute, PhyContract, createContract);
            break;
        case EntityType::NetworkContract:
            UPDATE_ENTITY(networkContractsTable, NetworkContractIndexedAttribute, PhyNetworkContract, createNetworkContract);
            break;
        case EntityType::AdvJoinInfo:
            UPDATE_ENTITY_ATTRIBUTE(advInfo, PhyAdvJoinInfo);
            break;
        case EntityType::AdvSuperframe:
            UPDATE_ENTITY_ATTRIBUTE(advSuperframe, PhyUint16);
            break;
        case EntityType::SessionKey:
            UPDATE_ENTITY(sessionKeysTable, SessionKeyIndexedAttribute, PhySessionKey, createSessionKey);
            break;
        case EntityType::MasterKey:
            UPDATE_ENTITY(masterKeysTable, SpecialKeyIndexedAttribute, PhySpecialKey, createMasterKey);
            break;
        case EntityType::SubnetKey:
            UPDATE_ENTITY(subnetKeysTable, SpecialKeyIndexedAttribute, PhySpecialKey, createSubnetKey);
            break;
        case EntityType::PowerSupply:
            UPDATE_ENTITY_ATTRIBUTE(powerSupplyStatus, PhyUint8);
            break;
        case EntityType::AssignedRole:
            UPDATE_ENTITY_ATTRIBUTE(assignedRole, PhyUint16);
            break;
        case EntityType::SerialNumber:
            UPDATE_ENTITY_ATTRIBUTE(serialNumber, PhyString);
            break;
        case EntityType::AddressTranslation:
            UPDATE_ENTITY(addressTranslationTable, AddressTranslationIndexedAttribute, PhyAddressTranslation, createAddressTranslation);
            break;
        case EntityType::ClientServerRetryTimeout:
            UPDATE_ENTITY_ATTRIBUTE(retryTimeout, PhyUint16);
            break;
        case EntityType::ClientServerRetryMaxTimeoutInterval:
            UPDATE_ENTITY_ATTRIBUTE(max_Retry_Timeout_Interval, PhyUint16);
            break;
        case EntityType::ClientServerRetryCount:
            UPDATE_ENTITY_ATTRIBUTE(retryCount, PhyUint8);
            break;
        case EntityType::Channel:
            throw NE::Common::NEException("Not implemented yet!");
            break;
        case EntityType::BlackListChannels:
            UPDATE_ENTITY(blacklistChannelsTable, BlacklistChannelsIndexedAttribute, PhyBlacklistChannels, createBlacklistChannel);
            break;
        case EntityType::ChannelHopping:
            UPDATE_ENTITY(channelHoppingTable, ChannelHoppingIndexedAttribute, PhyChannelHopping, createChannelHopping);
            break;
        case EntityType::Vendor_ID:
            UPDATE_ENTITY_ATTRIBUTE(vendorID, PhyString);
            break;
        case EntityType::Model_ID:
            UPDATE_ENTITY_ATTRIBUTE(modelID, PhyString);
            break;
        case EntityType::Software_Revision_Information:
            UPDATE_ENTITY_ATTRIBUTE(softwareRevisionInformation, PhyString);
            break;
        case EntityType::PackagesStatistics:
            UPDATE_ENTITY_ATTRIBUTE(packagesStatistics, PhyBytes);
            break;
        case EntityType::JoinReason:
            UPDATE_ENTITY_ATTRIBUTE(joinReason, PhyBytes);
            break;
        case EntityType::ContractsTable_MetaData:
            UPDATE_ENTITY_ATTRIBUTE(contractsTableMetadata, PhyMetaData);
            break;
        case EntityType::Neighbor_MetaData:
            UPDATE_ENTITY_ATTRIBUTE(neighborMetadata, PhyMetaData);
            break;
        case EntityType::Superframe_MetaData:
            UPDATE_ENTITY_ATTRIBUTE(superframeMetadata, PhyMetaData);
            break;
        case EntityType::Graph_MetaData:
            UPDATE_ENTITY_ATTRIBUTE(graphMetadata, PhyMetaData);
            break;
        case EntityType::Link_MetaData:
            UPDATE_ENTITY_ATTRIBUTE(linkMetadata, PhyMetaData);
            break;
        case EntityType::Route_MetaData:
            UPDATE_ENTITY_ATTRIBUTE(routeMetadata, PhyMetaData);
            break;
        case EntityType::Diag_MetaData:
            UPDATE_ENTITY_ATTRIBUTE(diagMetadata, PhyMetaData);
            break;
        case EntityType::ChannelDiag:
            throw NE::Common::NEException("Not implemented yet!");
            break;
        case EntityType::NeighborDiag:
            throw NE::Common::NEException("Not implemented yet!");
            break;
        case EntityType::HRCO_CommEndpoint:
            UPDATE_ENTITY_ATTRIBUTE(hrcoCommEndpoint, PhyCommunicationAssociationEndpoint);
            break;
        case EntityType::HRCO_Publish:
            UPDATE_ENTITY_ATTRIBUTE(hrcoEntityIndexListAttribute, PhyEntityIndexList);
            break;
        case EntityType::DeviceCapability:
            UPDATE_ENTITY_ATTRIBUTE(deviceCapability, PhyDeviceCapability);
            break;
        case EntityType::QueuePriority:
            UPDATE_ENTITY_ATTRIBUTE(queuePriority, PhyQueuePriority);
            break;
        case EntityType::ARMO_CommEndpoint:
            UPDATE_ENTITY_ATTRIBUTE(armoCommEndpoint, PhyAlertCommunicationEndpoint);
            break;
        case EntityType::DLMO_MaxLifetime:
            UPDATE_ENTITY_ATTRIBUTE(dlmoMaxLifeTime, PhyUint16);
            break;
        case EntityType::DLMO_IdleChannels:
            UPDATE_ENTITY_ATTRIBUTE(dlmoIdleChannels, PhyUint16);
            break;
        case EntityType::PingInterval:
            UPDATE_ENTITY_ATTRIBUTE(pingInterval, PhyUint16);
            break;
        case EntityType::DLMO_DiscoveryAlert:
            UPDATE_ENTITY_ATTRIBUTE(dlmoDiscoveryAlert, PhyUint8);
            break;

        default:
            LOG_ERROR("updatePhyEntity() : Unknown physical entity type : " << entityType);
            throw NEException("updatePhyEntity() : Unknown physical entity type.");
        }

        return true;
    }

    LOG_ERROR("invalid operation type" << opType );
    return false;
}

void OperationsProcessor::cleanEmptyContainers(Uint32 currentTime, Uint16 timeoutValue)
{
    ListOfOperationsContainer::iterator itContainer = listOfContainers.begin();
    Address32Set devicesToBeRemoved;

    for (; itContainer != listOfContainers.end();) {
    	// empty remained container
    	if( (*itContainer)->isContainerEmpty() ){
    	    //for cases when handleEndContainer() inserts another container and this method gets called
            // to avoid a recursive call then: create a copy of container(pointer), erase current container from list and then call handler.
    	    OperationsContainerPointer container = *itContainer;
            //LOG_DEBUG("cleanEmptyContainers() - erase CONTAINER(" << (int)(*itContainer)->containerId << ") : " << **itContainer);
    		itContainer = listOfContainers.erase(itContainer);
    		container->handleEndContainer();
    	} else if ((*itContainer)->isExpiredContainer(currentTime, timeoutValue)){
    	    OperationsContainerPointer container = *itContainer;

            std::ostringstream stream;
            stream << "Found expired container: removing containerID=" << (int)container->getContainerId() << ", reason=" << container->getReason();

    	    LOG_WARN(stream.str() << ", container=" << *container);
    	    SMState::SMStateLog::logOperationsContainer(stream.str(), *container);

            //LOG_DEBUG("cleanEmptyContainers() - erase CONTAINER(" << (int)(*itContainer)->containerId << ") : " << **itContainer);
            itContainer = listOfContainers.erase(itContainer);
//            container->setAsFail( &subnetsContainer );
//            container->handleEndContainer();
            container->expireContainer( &subnetsContainer, devicesToBeRemoved);

    	} else {
    		++itContainer;
    	}
    }

    if (!devicesToBeRemoved.empty() && deviceRemover != NULL){
        LOG_INFO("Deletes the devices in expired container ");
        Address32Set::const_iterator itAddress = devicesToBeRemoved.begin();
        Subnet::PTR subnet = subnetsContainer.getSubnet(*itAddress);
        char reason[128];
        sprintf(reason, "remove devices- expired container");

        OperationsContainerPointer containerRemDevOperations(new OperationsContainer(reason));

        for (; itAddress != devicesToBeRemoved.end(); ++itAddress) {
            Device *device = this->subnetsContainer.getDevice(*itAddress);
            if (device && device->statusForReports != StatusForReports::JOINED_AND_CONFIGURED) {
                deviceRemover->removeDeviceOnError(*itAddress, containerRemDevOperations, RemoveDeviceReason::joinTimeout);
            } else {
                deviceRemover->removeDeviceOnError(*itAddress, containerRemDevOperations, RemoveDeviceReason::timeout);
            }
        }
        addOperationsContainer(containerRemDevOperations);
    }

}

bool OperationsProcessor::existsOperationsForDevice(Address32 deviceAddress32) {

    for (ListOfOperationsContainer::iterator itContainer = listOfContainers.begin(); itContainer != listOfContainers.end(); ++itContainer) {
        OperationsList& unsentOperations = (*itContainer)->getUnsentOperations();
        for(OperationsList::iterator itUnsentOperations = unsentOperations.begin(); itUnsentOperations != unsentOperations.end(); ++itUnsentOperations) {
            //LOG_DEBUG("existsOperationsForDevice operation=" << **itUnsentOperations);
            if ((*itUnsentOperations)->getOwner() == deviceAddress32) {
                //LOG_DEBUG("existsOperationsForDevice unsent return true");
                return true;
            }
        }

        OperationsList& sentOperations = (*itContainer)->getSentOperations();
        for(OperationsList::iterator itSentOperations = sentOperations.begin(); itSentOperations != sentOperations.end(); ++itSentOperations) {
            //LOG_DEBUG("existsOperationsForDevice operation=" << **itSentOperations);
            if ((*itSentOperations)->getOwner() == deviceAddress32) {
                //LOG_DEBUG("existsOperationsForDevice sent return true");
                return true;
            }
        }
    }
    //LOG_DEBUG("existsOperationsForDevice return false");
    return false;
}

void OperationsProcessor::deviceDeletedCallback(Address32 deletedDevAddr32, Uint16 deviceType) {

    LOG_INFO("deviceDeletedCallback - device=" << Address::toString(deletedDevAddr32) << ", deviceType=" << (int)deviceType);

    ListOfOperationsContainer::iterator itContainer = listOfContainers.begin();

    Subnet::PTR subnet = subnetsContainer.getSubnet(deletedDevAddr32);
    bool isBackbone = deviceType & NE::Model::DeviceType::BACKBONE;

    for (; itContainer != listOfContainers.end();) {

        bool needToClean = false;
        if ( isBackbone ) {
            needToClean = (*itContainer)->removeAllSentOperationsForSubnet(subnet, &subnetsContainer);
            if (Address::extractSubnet((*itContainer)->getRequesterAddress32()) == subnet->getSubnetId()) {
                needToClean = true;
            }
        } else {
            needToClean = (*itContainer)->removeAllSentOperationsForOwner(subnet, deletedDevAddr32, &subnetsContainer);
            if ((*itContainer)->getRequesterAddress32() == deletedDevAddr32) {
                needToClean = true;
            }
        }
        if (!needToClean) { // maybe is on pending list
            OperationsList& unsentOperations = (*itContainer)->getUnsentOperations();
            OperationsList::iterator itOperation = unsentOperations.begin();
            for (; itOperation != unsentOperations.end(); ++itOperation) {
                if (isBackbone) {
                    if (Address::extractSubnet((*itOperation)->getOwner()) == subnet->getSubnetId()) {
                        needToClean = true;
                        break;
                    }
                } else if ((*itOperation)->getOwner() == deletedDevAddr32) {
                    needToClean = true;
                    break;
                }
            }
        }
        if (needToClean) { // destroy all operations since a neighbor was deleted
            (*itContainer)->setAsFail( &subnetsContainer );
    	    if( (*itContainer)->isContainerEmpty() )
    	    {
    	        //for cases when handleEndContainer() inserts another container and this method gets called
                // to avoid a recursive call then: create a copy of container(pointer), erase current container from list and then call handler.
    	        OperationsContainerPointer container = *itContainer;
                //LOG_DEBUG("deviceDeletedCallback() - erase CONTAINER(" << (int)(*itContainer)->containerId << ") : " << **itContainer);
    		    itContainer = listOfContainers.erase(itContainer);
    		    container->handleEndContainer();
    	    }
    	    else
    	    {
    		    ++itContainer;
    	    }
    	} else {
    		++itContainer;
    	}
    }
}

std::ostream& operator<<(std::ostream& stream, const OperationsProcessor& operationsProcessor) {

    //stream << " ##################### BEGIN ############################" << std::endl;
    for (ListOfOperationsContainer::const_iterator itContainer = operationsProcessor.listOfContainers.begin();
            itContainer != operationsProcessor.listOfContainers.end(); ++itContainer) {

        stream << "container: " << **itContainer << std::endl;
    }
    //stream << " ##################### END ############################" << std::endl;

    return stream;
}


}
}
}
