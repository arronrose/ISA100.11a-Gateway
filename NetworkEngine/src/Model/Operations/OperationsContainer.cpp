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
 * EngineOperationsList.cpp
 *
 *  Created on: Jun 17, 2008
 *      Author: catalin.pop
 */
#include "OperationsContainer.h"
#include "Common/ClockSource.h"
#include "SMState/SMStateLog.h"
#include "Model/SubnetsContainer.h"

namespace NE {

namespace Model {

namespace Operations {

#define REMOVE_IN_WAITING_LIST_INDEXED(attributeTable, AttributeTableType) {\
    AttributeTableType::iterator it;\
    it = device->phyAttributes.attributeTable.find(operation.getEntityIndex());\
    if (it != device->phyAttributes.attributeTable.end()) {\
        it->second.removeOperation(operation.getOpID());\
        return true;\
    }\
    return false;\
}

#define REMOVE_IN_WAITING_LIST(attributeTable, AttributeTableType) {\
    device->phyAttributes.attributeTable.removeOperation(operation.getOpID());\
    return true;\
}


int OperationsContainer::nextAvailableContainerId = 1;

OperationsContainer::OperationsContainer(const char *reason) :
	requesterAddress32(0), taiCutover(time(NULL)), errorCode(OperationErrorCode::SUCCESS),
	proxyAddress(false), reasonOfOperations(reason) {

    containerId = nextAvailableContainerId;
    if ((++nextAvailableContainerId) == 0) {
        nextAvailableContainerId++;//do not used setIndex = 0
    }

    eventEngineType = NetworkEngineEventType::NONE;
    rspStatus = ResponseStatus::SUCCESS;

    LOG_DEBUG("Create container " << containerId << ", reason=" << reason);
}

OperationsContainer::OperationsContainer(Address32 requesterAddress_, Uint16 requestId_, HandlerResponse handlerResponse_,
            const char *reason) :
                requesterAddress32(requesterAddress_), taiCutover(time(NULL)), errorCode(
                OperationErrorCode::SUCCESS), proxyAddress(false), requestId(requestId_),
                reasonOfOperations(reason) {

    containerId = nextAvailableContainerId;
    if ((++nextAvailableContainerId) == 0) {
        nextAvailableContainerId++;//do not used setIndex = 0
    }

    handlerResponses.push_back(handlerResponse_);


    eventEngineType = NetworkEngineEventType::NONE;
    rspStatus = ResponseStatus::SUCCESS;

    LOG_DEBUG("Create container " << containerId << ", reason=" << reason);
}

OperationsContainer::OperationsContainer(Address32 requesterAddress_, Uint16 requestId_, HandlerResponseList& handlerResponses_,
            const char *reason) :
                requesterAddress32(requesterAddress_), taiCutover(time(NULL)), errorCode(
                OperationErrorCode::SUCCESS), proxyAddress(false), handlerResponses(handlerResponses_),
                requestId(requestId_), reasonOfOperations(reason) {

    containerId = nextAvailableContainerId;
    if ((++nextAvailableContainerId) == 0) {
        nextAvailableContainerId++;//do not used setIndex = 0
    }

    eventEngineType = NetworkEngineEventType::NONE;
    rspStatus = ResponseStatus::SUCCESS;

    LOG_DEBUG("Create container " << containerId << ", reason=" << reason);
}

OperationsContainer::~OperationsContainer() {
    LOG_DEBUG("Destroy container " << containerId << ", reason=" << reasonOfOperations);
}

void OperationsContainer::addOperation(const NE::Model::Operations::IEngineOperationPointer& operation, Device* ownerDevice) {
    operation->setContainerId(containerId);
    unsentOperations.push_back(operation);
    ownerDevice->addUnsentOperation(operation);
}

void OperationsContainer::addToSentOperations(const NE::Model::Operations::IEngineOperationPointer& operation) {
    this->sentOperations.push_back(operation);
    //    LOG_DEBUG("this->sentOperations.size() : " << (int) this->sentOperations.size());
}


bool OperationsContainer::removeAllSentOperationsForOwner(Subnet::PTR& subnet, Address32 owner, NE::Model::SubnetsContainer * subnetsContainers) {
	bool bOwnerFound = false;
    Device * device = subnet->getDevice( owner );
    for (OperationsList::iterator itOperation = sentOperations.begin(); itOperation != sentOperations.end();) {
        if ((*itOperation)->getOwner() == owner) {
            if( device )
            {
                subnet->unreserveSlotsOperation(device,*itOperation);
            }

            removeOperationInWaitingList(**itOperation, subnetsContainers);
            (*itOperation)->destroyPhysicalEntity();
            //marked on 0 in order to not be found on confirm in case of a fast rejoin
            //contract is removed from stack but detection of a packet that is on a inexistent contract is made only when the
            //packet has the time to be sent (when the retry time is due)
            //in this time if the device performed rejoin it will produce a false error in OperationProcessor
            (*itOperation)->setContainerId(0);
            itOperation = sentOperations.erase(itOperation);

            bOwnerFound = true;
        } else {
            ++itOperation;
        }
    }
    return bOwnerFound;
}

bool OperationsContainer::removeAllSentOperationsForSubnet(Subnet::PTR& subnet, NE::Model::SubnetsContainer * subnetsContainers) {
	bool bOwnerFound = false;

    for (OperationsList::iterator itOperation = sentOperations.begin(); itOperation != sentOperations.end();) {
        if ( Address::extractSubnet((*itOperation)->getOwner()) == subnet->getSubnetId()) {
            LOG_DEBUG("operation=" << **itOperation);
            Device * device = subnet->getDevice( (*itOperation)->getOwner() );
            if( device )
            {
                subnet->unreserveSlotsOperation(device,*itOperation);
            }
            removeOperationInWaitingList(**itOperation, subnetsContainers);
            (*itOperation)->destroyPhysicalEntity();
            //marked on 0 in order to not be found on confirm in case of a fast rejoin
            //contract is removed from stack but detection of a packet that is on a inexistent contract is made only when the
            //packet has the time to be sent (when the retry time is due)
            //in this time if the device performed rejoin it will produce a false error in OperationProcessor
            (*itOperation)->setContainerId(0);
            itOperation = sentOperations.erase(itOperation);

            bOwnerFound = true;
        } else {
            ++itOperation;
        }
    }
    return bOwnerFound;
}

void OperationsContainer::expireContainer(NE::Model::SubnetsContainer * pSubnetsContainers, Address32Set& devicesToBeRemoved){
    setAsFail( pSubnetsContainers );
    handleEndContainer();
    //remove all devices that had operations in sentList and no responce received up now
    if (!sentOperations.empty()) {
        for (OperationsList::const_iterator itOperation = sentOperations.begin(); itOperation != sentOperations.end(); ++itOperation) {
            Address32 device32 = (*itOperation)->getOwner() ;

            Subnet::PTR subnet = pSubnetsContainers->getSubnet( device32 );
            if( subnet ) {
                Device * device = subnet->getDevice( device32 );
                if( device ) {
                    device->removeUnsentOperation(*itOperation);
                    subnet->unreserveSlotsOperation(device,*itOperation);
                }
            }
            removeOperationInWaitingList(**itOperation, pSubnetsContainers);
            (*itOperation)->destroyPhysicalEntity();
            devicesToBeRemoved.insert((*itOperation)->getOwner());
        }
    }
}

void OperationsContainer::setAsFail(NE::Model::SubnetsContainer * pSubnetsContainers)
{

	// destroy all unsent operations
    for (OperationsList::iterator itOperation = unsentOperations.begin(); itOperation != unsentOperations.end();) {
        Address32 device32 = NE::Model::getDeviceAddress( (*itOperation)->getEntityIndex() );

        Subnet::PTR subnet = pSubnetsContainers->getSubnet( device32 );
        if( subnet ) {
            Device * device = subnet->getDevice( device32 );
            if( device ) {
                device->removeUnsentOperation(*itOperation);
                subnet->unreserveSlotsOperation(device,*itOperation);
            }
        }
        removeOperationInWaitingList(**itOperation, pSubnetsContainers);
        (*itOperation)->destroyPhysicalEntity();
        itOperation = unsentOperations.erase(itOperation);
    }

	rspStatus = ResponseStatus::FAIL;
}

void OperationsContainer::handleEndContainer(bool skipLogging)
{
    if (!skipLogging){
        char msg[255];
        sprintf(msg, "END containerId: %d, status:%d reason=%s", this->containerId, rspStatus, this->reasonOfOperations.c_str());
        LOG_DEBUG(msg);
        SMState::SMStateLog::logOperationReason(msg);
    }
    callHandlerResponsesList(handlerResponses, this->requesterAddress32, this->requestId, rspStatus);
}

bool OperationsContainer::removeOperationInWaitingList(NE::Model::Operations::IEngineOperation & operation, NE::Model::SubnetsContainer * pSubnetsContainers ){
    Device * device = pSubnetsContainers->getDevice(operation.getOwner());
    if (device == NULL){
        return true;
    }
    EntityType::EntityTypeEnum entityType = getEntityType(operation.getEntityIndex());

    switch( entityType ) {

    case EntityType::Link:
        REMOVE_IN_WAITING_LIST_INDEXED(linksTable, LinkIndexedAttribute);
        break;
    case EntityType::Superframe:
        REMOVE_IN_WAITING_LIST_INDEXED(superframesTable, SuperframeIndexedAttribute);
        break;
    case EntityType::Neighbour:
        REMOVE_IN_WAITING_LIST_INDEXED(neighborsTable, NeighborIndexedAttribute);
        break;
    case EntityType::Candidate:
        REMOVE_IN_WAITING_LIST_INDEXED(candidatesTable, CandidateIndexedAttribute);
        break;
    case EntityType::Route:
        REMOVE_IN_WAITING_LIST_INDEXED(routesTable, RouteIndexedAttribute);
        break;
    case EntityType::NetworkRoute:
        REMOVE_IN_WAITING_LIST_INDEXED(networkRoutesTable, NetworkRouteIndexedAttribute);
        break;
    case EntityType::Graph:
        REMOVE_IN_WAITING_LIST_INDEXED(graphsTable, GraphIndexedAttribute);
        break;
    case EntityType::Contract:
        REMOVE_IN_WAITING_LIST_INDEXED(contractsTable, ContractIndexedAttribute);
        break;
    case EntityType::NetworkContract:
        REMOVE_IN_WAITING_LIST_INDEXED(networkContractsTable, NetworkContractIndexedAttribute);
        break;
    case EntityType::AdvJoinInfo:
        REMOVE_IN_WAITING_LIST(advInfo, PhyAdvJoinInfo);
        break;
    case EntityType::AdvSuperframe:
        REMOVE_IN_WAITING_LIST(advSuperframe, PhyUint16);
        break;
    case EntityType::SessionKey:
        REMOVE_IN_WAITING_LIST_INDEXED(sessionKeysTable, SessionKeyIndexedAttribute);
        break;
    case EntityType::MasterKey:
        REMOVE_IN_WAITING_LIST_INDEXED(masterKeysTable, SpecialKeyIndexedAttribute);
        break;
    case EntityType::SubnetKey:
        REMOVE_IN_WAITING_LIST_INDEXED(subnetKeysTable, SpecialKeyIndexedAttribute);
        break;
    case EntityType::PowerSupply:
        REMOVE_IN_WAITING_LIST(powerSupplyStatus, PhyUint8);
        break;
    case EntityType::AssignedRole:
        REMOVE_IN_WAITING_LIST(assignedRole, PhyUint16);
        break;
    case EntityType::SerialNumber:
        REMOVE_IN_WAITING_LIST(serialNumber, PhyString);
        break;
    case EntityType::AddressTranslation:
        REMOVE_IN_WAITING_LIST_INDEXED(addressTranslationTable, AddressTranslationIndexedAttribute);
        break;
    case EntityType::ClientServerRetryTimeout:
        REMOVE_IN_WAITING_LIST(retryTimeout, PhyUint16);
        break;
    case EntityType::ClientServerRetryMaxTimeoutInterval:
        REMOVE_IN_WAITING_LIST(max_Retry_Timeout_Interval, PhyUint16);
        break;
    case EntityType::ClientServerRetryCount:
        REMOVE_IN_WAITING_LIST(retryCount, PhyUint8);
        break;
    case EntityType::Channel:
        throw NE::Common::NEException("Not implemented yet!");
        return false;
        break;
    case EntityType::BlackListChannels:
        REMOVE_IN_WAITING_LIST_INDEXED(blacklistChannelsTable, BlacklistChannelsIndexedAttribute);
        break;
    case EntityType::ChannelHopping:
        REMOVE_IN_WAITING_LIST_INDEXED(channelHoppingTable, ChannelHoppingIndexedAttribute);
        break;
    case EntityType::Vendor_ID:
        REMOVE_IN_WAITING_LIST(vendorID, PhyString);
        break;
    case EntityType::Model_ID:
        REMOVE_IN_WAITING_LIST(modelID, PhyString);
        break;
    case EntityType::Software_Revision_Information:
        REMOVE_IN_WAITING_LIST(softwareRevisionInformation, PhyString);
        break;
    case EntityType::PackagesStatistics:
        REMOVE_IN_WAITING_LIST(packagesStatistics, PhyBytes);
        break;
    case EntityType::JoinReason:
        REMOVE_IN_WAITING_LIST(joinReason, PhyBytes);
        break;
    case EntityType::ContractsTable_MetaData:
        REMOVE_IN_WAITING_LIST(contractsTableMetadata, PhyMetaData);
        break;
    case EntityType::Neighbor_MetaData:
        REMOVE_IN_WAITING_LIST(neighborMetadata, PhyMetaData);
        break;
    case EntityType::Superframe_MetaData:
        REMOVE_IN_WAITING_LIST(superframeMetadata, PhyMetaData);
        break;
    case EntityType::Graph_MetaData:
        REMOVE_IN_WAITING_LIST(graphMetadata, PhyMetaData);
        break;
    case EntityType::Link_MetaData:
        REMOVE_IN_WAITING_LIST(linkMetadata, PhyMetaData);
        break;
    case EntityType::Route_MetaData:
        REMOVE_IN_WAITING_LIST(routeMetadata, PhyMetaData);
        break;
    case EntityType::Diag_MetaData:
        REMOVE_IN_WAITING_LIST(diagMetadata, PhyMetaData);
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
        REMOVE_IN_WAITING_LIST(hrcoCommEndpoint, PhyCommunicationAssociationEndpoint);
        break;
    case EntityType::HRCO_Publish:
        REMOVE_IN_WAITING_LIST(hrcoEntityIndexListAttribute, PhyEntityIndexList);
        break;
    case EntityType::DeviceCapability:
        REMOVE_IN_WAITING_LIST(deviceCapability, PhyDeviceCapability);
        break;
    case EntityType::QueuePriority:
        REMOVE_IN_WAITING_LIST(queuePriority, PhyQueuePriority);
        break;
    case EntityType::ARMO_CommEndpoint:
        REMOVE_IN_WAITING_LIST(armoCommEndpoint, PhyAlertCommunicationEndpoint);
        break;
    case EntityType::DLMO_MaxLifetime:
        REMOVE_IN_WAITING_LIST(dlmoMaxLifeTime, PhyUint16);
        break;
    case EntityType::DLMO_IdleChannels:
        REMOVE_IN_WAITING_LIST(dlmoIdleChannels, PhyUint16);
        break;
    case EntityType::PingInterval:
        REMOVE_IN_WAITING_LIST(pingInterval, PhyUint16);
        break;
    case EntityType::DLMO_DiscoveryAlert:
        REMOVE_IN_WAITING_LIST(dlmoDiscoveryAlert, PhyUint8);
        break;

    default:
        throw NE::Common::NEException("Entity type not treated!");
        return false;
    }
}


void OperationsContainer::toShortString(std::ostringstream &stream) {

    int i = 0;
    bool isFirst = true;
    stream << std::endl;
    stream << " {";
    for (OperationsList::iterator it = unsentOperations.begin(); it != unsentOperations.end(); it++) {
        if (isFirst) {
            isFirst = false;
        } else {
            stream << ", ";
        }
        ++i;
        std::string ownerString;
        Address::toString((*it)->getOwner(), ownerString);
        stream << "0x" << ownerString;
    }
    stream << " }";

    i = 0;
    isFirst = true;
    stream << std::endl;
    stream << " {";
    for (OperationsList::iterator it = unsentOperations.begin(); it != unsentOperations.end(); it++) {
        if (isFirst) {
            isFirst = false;
        } else {
            stream << ", ";
        }
        ++i;
        stream << (*it)->getType();
    }
    stream << " }";

}

std::ostream& operator<<(std::ostream& stream, OperationsContainer& container) {
    OperationsList& operations = container.getUnsentOperations();
    OperationsList& sentOperations = container.getSentOperations();
    stream << std::endl;
    stream << " engineOperations : {";
    for (OperationsList::iterator it = operations.begin(); it != operations.end(); it++) {
        stream << std::endl;
        stream << (**it);
    }
    stream << std::endl;
    stream << " }";
    stream << std::endl;
    stream << " sentOperations : {";
    for (OperationsList::iterator it = sentOperations.begin(); it != sentOperations.end(); it++) {
        stream << std::endl;
        stream << (**it);
    }
    stream << std::endl;
    stream << " }";

    return stream;
}

}
}
}
