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
 * Device.cpp
 *
 *  Created on: Sep 16, 2009
 *      Author: Catalin Pop
 */

#include "Model/Device.h"
#include <iomanip>
#include <boost/unordered_set.hpp>
#include <list>

#include "Common/NEAddress.h"
#include "Common/NETypes.h"
#include "Common/logging.h"
#include "Model/ModelPrinter.h"
#include "Model/Capabilities.h"
#include "model.h"
#include "Model/Subnet.h"
#include "Model/Tdma/TdmaTypes.h"
#include "Common/ClockSource.h"

namespace NE {
namespace Model {

#define MAX_16BIT_ID 0x00003FFF

Device::Device(const Capabilities& deviceCapabilities) :
    hasVistited(false),
        dirtyInboundFlag(true),
        inboundAppTraffic(0.0),
        outboundAppTraffic(0.0),
        lastContractID(0),
        lastLinkID(3),//links id 1-3 are used on defaultInit for default links
        lastIndexID(0),
        lastNetworkRouteID(0),
        lastATTID(0),
        lastRouteID(0),
        address64(deviceCapabilities.euidAddress),
        parent32(0),
        capabilities(deviceCapabilities),
        status(DeviceStatus::NOT_JOINED),
        //startTime(time(NULL)),
        joinConfirmTime(0xFFFFFFFF),//configure it at long time in future to avoid false operations
        hasChanged(true),
        hasRoleActivated(RoleActivationStatus::NOT_ACTIVE),
        hasMetadataUpdated(false),
        statusForReports(StatusForReports::SEC_JOIN_REQUEST_RECEIVED), //device instance is created at security join
        lastPacketTAI(0),
        joinsCount(0),
        fullJoinsCount(0),
        nrOfwrongPublishReceived(0),
        fastDiscoveryTime(0)
 {

    acceleratedFlag = false;
    Uint32 currentTime = NE::Common::ClockSource::getCurrentTime();
    setLastTimeAccessed(currentTime);
    startTime = currentTime;
    roleChanged = false;

    transmissionsSinceLastReport = 0;
    failedTransmissionsSinceLastReport = 0;
    lastLinkID = rand() % (MAX_16BIT_ID / 2 );
    lastIndexID = lastLinkID < 3 ? 3 : lastIndexID;

}

void Device::setStatistics(Uint8 transmited, Uint8 received, Uint8 failedTransmission, Uint8 failedReception) {
    deviceStatistics.DPDUsTransmitted = transmited;

    deviceStatistics.DPDUsReceived = received;

    deviceStatistics.DPDUsFailedTransmission = failedTransmission;

    deviceStatistics.DPDUsFailedReception = failedReception;

}

void Device::updateStatistics(Uint8 transmited, Uint8 received, Uint8 failedTransmission, Uint8 failedReception) {
    if ((MAX_32BITS_VALUE - deviceStatistics.DPDUsTransmitted) <= transmited) {
        deviceStatistics.DPDUsTransmitted = MAX_32BITS_VALUE;
    } else {
        deviceStatistics.DPDUsTransmitted += transmited;
    }

    if ((MAX_32BITS_VALUE - deviceStatistics.DPDUsReceived) <= received) {
        deviceStatistics.DPDUsReceived = MAX_32BITS_VALUE;
    } else {
        deviceStatistics.DPDUsReceived += received;
    }

    if ((MAX_32BITS_VALUE - deviceStatistics.DPDUsFailedTransmission) <= failedTransmission) {
        deviceStatistics.DPDUsFailedTransmission = MAX_32BITS_VALUE;
    } else {
        deviceStatistics.DPDUsFailedTransmission += failedTransmission;
    }

    if ((MAX_32BITS_VALUE - deviceStatistics.DPDUsFailedReception) <= failedReception) {
        deviceStatistics.DPDUsFailedReception = MAX_32BITS_VALUE;
    } else {
        deviceStatistics.DPDUsFailedReception += failedReception;
    }

    if (((MAX_32BITS_VALUE - transmissionsSinceLastReport) <= transmited)
                || ((MAX_32BITS_VALUE - failedTransmissionsSinceLastReport) <= failedTransmission)) {

        resetStatisticsSinceLastReport(); //keep them synchronized
    }

    transmissionsSinceLastReport += transmited;
    failedTransmissionsSinceLastReport += failedTransmission;
}

void Device::updateStatistics(const std::vector<Uint8>& channelsCCABackoffs) {
    for (int i = 0; i <= 15; ++i) { //16 channels
        if (deviceStatistics.CCABackoffStatistics[i] != 0) {
            deviceStatistics.CCABackoffStatistics[i] = (deviceStatistics.CCABackoffStatistics[i] + channelsCCABackoffs[i]) / 2;
        } else {
            deviceStatistics.CCABackoffStatistics[i] = channelsCCABackoffs[i];
        }
    }
}

void Device::resetStatisticsSinceLastReport() {
    transmissionsSinceLastReport = 0;
    failedTransmissionsSinceLastReport = 0;
}

bool isContractIDFree(int contractId, NE::Model::Device * device) {
    EntityIndex contractEntityIndex = createEntityIndex(device->address32, EntityType::Contract, contractId);
    return device->phyAttributes.contractsTable.find(contractEntityIndex) == device->phyAttributes.contractsTable.end();
}

#define GET_NEXT_ID(lastVariable, table, entityType, maximumValue){ \
    int numberOfFullSerachLoops = 0;                                \
    do {                                                            \
        if ((++lastVariable) > maximumValue) {                      \
            if ((++numberOfFullSerachLoops) == 2) {                 \
        	    THROW_EX2(NE::Common::NEException, "getNextID() : All IDs are occupied for Entity:" << entityType);\
            }                                                       \
            lastVariable = 1;                                       \
        }                                                           \
        EntityIndex entityIndex = createEntityIndex(address32, entityType, lastVariable); \
        if (phyAttributes.table.find(entityIndex) == phyAttributes.table.end()) break;  \
    } while( true );                                                \
    return (Uint16) lastVariable;                                   \
}

Uint16 Device::getNextContractID() {
    GET_NEXT_ID(lastContractID, contractsTable, EntityType::Contract, MAX_16BIT_ID);
}

Uint16 Device::getNextLinkID() {
    GET_NEXT_ID(lastLinkID, linksTable, EntityType::Link, MAX_16BIT_ID);
}

Uint16 Device::getNextNetworkRouteID() {
    GET_NEXT_ID(lastNetworkRouteID, networkRoutesTable, EntityType::NetworkRoute, MAX_16BIT_ID);
}

Uint16 Device::getNextRouteID() {
    GET_NEXT_ID(lastRouteID, routesTable, EntityType::Route, MAX_16BIT_ID);
}

Uint16 Device::getNextAddressTranslationID() {
    GET_NEXT_ID(lastATTID, addressTranslationTable, EntityType::AddressTranslation, MAX_16BIT_ID);
}

void Device::getEdges(Address16Set &targetsList) {
    for (std::map<EntityIndex, NeighborAttribute>::iterator it = phyAttributes.neighborsTable.begin(); it
                != phyAttributes.neighborsTable.end(); ++it) {
        if (getEntityType(it->first) == EntityType::Neighbour) {
            targetsList.insert(getIndex(it->first));
        }
    }

    for (std::map<EntityIndex, CandidateAttribute>::iterator it = phyAttributes.candidatesTable.begin(); it
                != phyAttributes.candidatesTable.end(); ++it) {
        if (getEntityType(it->first) == EntityType::Candidate) {
            targetsList.insert(getIndex(it->first));
        }
    }
}

Uint8 Device::getEdgesNo() {

    Uint8 neighborsNo = 0;

    for (std::map<EntityIndex, NeighborAttribute>::iterator it = phyAttributes.neighborsTable.begin(); it
                != phyAttributes.neighborsTable.end(); ++it) {
        if (getEntityType(it->first) == EntityType::Neighbour) {
            neighborsNo++;
        }
    }

    for (std::map<EntityIndex, CandidateAttribute>::iterator it = phyAttributes.candidatesTable.begin(); it
                != phyAttributes.candidatesTable.end(); ++it) {
        if (getEntityType(it->first) == EntityType::Candidate) {
            neighborsNo++;
        }
    }

    return neighborsNo;
}

bool isIndexIDFree(int indexID, NE::Model::Device * device) {
    EntityIndex keyEntityIndex = createEntityIndex(device->address32, EntityType::SessionKey, indexID);
    return device->phyAttributes.sessionKeysTable.find(keyEntityIndex) == device->phyAttributes.sessionKeysTable.end();
}

Uint16 Device::getNextKeysTableIndex() {
    int numberOfFullSerachLoops = 0;
    do {
        ++lastIndexID;
        if (lastIndexID > MAX_16BIT_ID) {
            ++numberOfFullSerachLoops;
            if (numberOfFullSerachLoops == 2) {
                //means that a full search for ALL possible IDs was made 1 time
                throw NE::Common::NEException("getNextKeyID() : All key IDs between 0 and 0x3FFF are occupied. "
                    "NO MORE IDs AVAILABLE!");
            }
            lastIndexID = 1;
        }
    }
    while (!isIndexIDFree(lastIndexID, this));

    return (Uint16) lastIndexID;
}

Uint16 Device::getGreatestKeyIDwithPeer(Device *peerDevice, int tsapSrc, int tsapDest) {

    Uint16 newID = 1;
    while (newID <= 0xFF) { //8-bit ID
        bool exists = false;

        //search if there's a key with this ID on the source device
        for (SessionKeyIndexedAttribute::iterator itKeys = this->phyAttributes.sessionKeysTable.begin(); itKeys
                    != this->phyAttributes.sessionKeysTable.end(); ++itKeys) {
            if (itKeys->second.getValue() == NULL) {
                continue;
            }
            if ((itKeys->second.getValue()->destination64 == peerDevice->address64)
                        && (itKeys->second.getValue()->sourceTSAP == tsapSrc)
                        && (itKeys->second.getValue()->destinationTSAP == tsapDest)
                        && itKeys->second.getValue()->keyID == newID) {
                exists = true;
                break;
            }
        }

        if (!exists) {
            //search if there's a key with this ID on the destination device
            for (SessionKeyIndexedAttribute::iterator itKeys = peerDevice->phyAttributes.sessionKeysTable.begin(); itKeys
                        != peerDevice->phyAttributes.sessionKeysTable.end(); ++itKeys) {
                if (itKeys->second.getValue() == NULL) {
                    continue;
                }
                if ((itKeys->second.getValue()->destination64 == this->address64) //
                            && (itKeys->second.getValue()->sourceTSAP == tsapDest) //
                            && (itKeys->second.getValue()->destinationTSAP == tsapSrc) //
                            && itKeys->second.getValue()->keyID == newID) {
                    exists = true;
                    break;
                }
            }
        }

        if (!exists) {
            return newID;
        }

        ++newID;
    }

    LOG_ERROR("Could not find a free ID for key between " << this->address64.toString() << " and "
                << peerDevice->address64.toString() << ", tsaps " << tsapSrc << " and " << tsapDest);

    return 0xFFFF;
}

Uint8 Device::getMinInboundBC_Router() {

    Uint8 minSet = MAX_NUMBER_OF_BANDWIDTH_CHUNCKS - 1;
    LinkIndexedAttribute::iterator itLink;
    for (itLink = phyAttributes.linksTable.begin(); itLink != phyAttributes.linksTable.end(); ++itLink) {

        const PhyLink * link = itLink->second.getValue();
        if (  link && (link->role == NE::Model::Tdma::TdmaLinkTypes::MANAGEMENT || link->role == NE::Model::Tdma::TdmaLinkTypes::UDO_FIRMWARE)
            && (link->type == NE::Model::Tdma::LinkType::TRANSMIT)
            &&  (link->direction == NE::Model::Tdma::TdmaLinkDir::INBOUND) ){
            Uint8 linkSet = link->schedule.offset % 100;
            if( linkSet < minSet )
                minSet = linkSet;
        }
    }

    return minSet;
}

Uint8 Device::getMaxOutBoundBC(Uint8 joinReservedSet) {

    Uint8 max = joinReservedSet;
    BCList::iterator it;
    for (it = theoAttributes.mngChunckOutbound.begin(); it != theoAttributes.mngChunckOutbound.end(); ++it) {
        if ((*it).setNo > max) {
            max = (*it).setNo;
        }
    }

    return max;
}

bool Device::isUsedBcSetNumber(Uint8 setNumber) {
    BCList::iterator it;
    for (it = theoAttributes.mngChunckInboundNonRouter.begin(); it != theoAttributes.mngChunckInboundNonRouter.end(); ++it) {
        if ((*it).setNo == setNumber) {
            return true;
        }
    }

    for (it = theoAttributes.mngChunckInboundRouter.begin(); it != theoAttributes.mngChunckInboundRouter.end(); ++it) {
        if ((*it).setNo == setNumber) {
            return true;
        }
    }
    for (it = theoAttributes.mngChunckOutbound.begin(); it != theoAttributes.mngChunckOutbound.end(); ++it) {
        if ((*it).setNo == setNumber) {
            return true;
        }
    }

    if (!capabilities.isBackbone()) // not backbone, search on links table to guarantee the availability
    {
        LinkIndexedAttribute::iterator itLink;
        for (itLink = phyAttributes.linksTable.begin(); itLink != phyAttributes.linksTable.end(); ++itLink) {

            const PhyLink * link = itLink->second.getValue();
            if (link && (link->role == NE::Model::Tdma::TdmaLinkTypes::MANAGEMENT || link->role == NE::Model::Tdma::TdmaLinkTypes::UDO_FIRMWARE)
            		&& ((link->schedule.offset % 100) == setNumber)) {

                return true;
            }
        }
    }

    return false;
}

float Device::getAllocatedInboundLink(Address16 peerDevice, Tdma::TdmaLinkTypes::TdmaLinkTypesEnum role) {
    float allocatedInboundLink = 0.0;
    LinkIndexedAttribute::iterator itLink;
    for (itLink = phyAttributes.linksTable.begin(); itLink != phyAttributes.linksTable.end(); ++itLink) {

        const PhyLink * link = itLink->second.getValue();
        if ( link && (link->neighbor == peerDevice)
            && (link->direction == Tdma::TdmaLinkDir::INBOUND )
            && (link->type == NE::Model::Tdma::LinkType::TRANSMIT)
            && (link->role == role) ) {

            allocatedInboundLink += ((float)SUPERFRAME_APPLICATION_LENGTH / link->schedule.interval);
        }
    }
    return allocatedInboundLink;

}

float Device::getAllocatedInboundLink2Roles(Address16 peerDevice, Tdma::TdmaLinkTypes::TdmaLinkTypesEnum role1, Tdma::TdmaLinkTypes::TdmaLinkTypesEnum role2) {
    float allocatedInboundLink = 0.0;
    LinkIndexedAttribute::iterator itLink;
    for (itLink = phyAttributes.linksTable.begin(); itLink != phyAttributes.linksTable.end(); ++itLink) {

        const PhyLink * link = itLink->second.getValue();
        if ( link && (link->neighbor == peerDevice)
            && (link->direction == Tdma::TdmaLinkDir::INBOUND )
            && (link->type == NE::Model::Tdma::LinkType::TRANSMIT)
            && ( (link->role == role1) || (link->role == role2) ) ) {

            allocatedInboundLink += ((float)SUPERFRAME_APPLICATION_LENGTH / link->schedule.interval);
        }
    }

    NE::Model::Operations::OperationsList::iterator itOperation;
    for (itOperation = unsentOperations.begin(); itOperation != unsentOperations.end(); ++itOperation){
        if ((*itOperation)->getType() != Operations::EngineOperationType::WRITE_ATTRIBUTE){
            continue;
        }
        if (getEntityType((*itOperation)->getEntityIndex()) != EntityType::Link ){
            continue;
        }
        if ((*itOperation)->getPhysicalEntity() == NULL){
            continue;
        }
        const Operations::IEngineOperationPointer& operation = *itOperation;
        const PhyLink * link = (PhyLink *)(operation->getPhysicalEntity());
        if (    (link->neighbor == peerDevice)
            && (link->direction == Tdma::TdmaLinkDir::INBOUND )
            && (link->type == NE::Model::Tdma::LinkType::TRANSMIT)
            && ( (link->role == role1) || (link->role == role2) ) ) {

            allocatedInboundLink += ((float)SUPERFRAME_APPLICATION_LENGTH / link->schedule.interval);
        }

    }

    return allocatedInboundLink;

}

PhyLink * Device::getNotFullTxMngLink( Address16 peerDevice, Tdma::TdmaLinkDir::TdmaLinkDirEnum direction ) {
    LinkIndexedAttribute::iterator itLink;
    for (itLink = phyAttributes.linksTable.begin(); itLink != phyAttributes.linksTable.end(); ++itLink) {

        PhyLink * linkTx = itLink->second.getValue();
        if ( linkTx && (linkTx->neighbor == peerDevice) ) {

            if ((linkTx->direction == direction )
                && (linkTx->type == NE::Model::Tdma::LinkType::TRANSMIT)
                && (linkTx->role == Tdma::TdmaLinkTypes::MANAGEMENT)
                && (linkTx->schedule.interval > 100 ) ) {

                return linkTx;
            }
        }
    }

    return NULL;
}


PhyLink * Device::getTxMngLink( Address16 peerDevice, Tdma::TdmaLinkDir::TdmaLinkDirEnum direction ) {
    LinkIndexedAttribute::iterator itLink;
    for (itLink = phyAttributes.linksTable.begin(); itLink != phyAttributes.linksTable.end(); ++itLink) {

        PhyLink * linkTx = itLink->second.getValue();
        if ( linkTx && (linkTx->neighbor == peerDevice) ) {

            if ((linkTx->direction == direction )
                && (linkTx->type == NE::Model::Tdma::LinkType::TRANSMIT)
                && (linkTx->role == Tdma::TdmaLinkTypes::MANAGEMENT)) {
                 return linkTx;
            }
        }
    }

    return NULL;
}

PhyLink * Device::getFirstTxMngLink(Address16 peerDevice, Tdma::TdmaLinkDir::TdmaLinkDirEnum direction, Tdma::TdmaLinkTypes::TdmaLinkTypesEnum role) {
    LinkIndexedAttribute::iterator itLink;
    for (itLink = phyAttributes.linksTable.begin(); itLink != phyAttributes.linksTable.end(); ++itLink) {

        PhyLink * linkTx = itLink->second.getValue();
        if ( linkTx && (linkTx->neighbor == peerDevice) ) {

            if ((linkTx->direction == direction )
                && (linkTx->type == NE::Model::Tdma::LinkType::TRANSMIT)
                && (linkTx->role == role) ) {

                return linkTx;
            }
        }
    }

    return NULL;
}


PhyLink * Device::getPeerRxLink( const PhyLink * txLink ) {
    LinkIndexedAttribute::iterator itLink;
    for (itLink = phyAttributes.linksTable.begin(); itLink != phyAttributes.linksTable.end(); ++itLink) {

        PhyLink * link = itLink->second.getValue();
        if (  link && (link->direction == txLink->direction )
            && (link->type == NE::Model::Tdma::LinkType::RECEIVE)
            && (link->role == txLink->role)
            && (link->chOffset == txLink->chOffset)
            && (link->schedule.interval == txLink->schedule.interval)
            && (link->schedule.offset == txLink->schedule.offset) ) {

            return link;
        }
    }

    return NULL;
}

PhyLink * Device::findLink(Uint8 direction, Uint8 type, Tdma::TdmaLinkTypes::TdmaLinkTypesEnum role, Uint8 chOffset, Uint16 offset, EntityIndex& entityIndexLink){
    LinkIndexedAttribute::iterator itLink;
    for (itLink = phyAttributes.linksTable.begin(); itLink != phyAttributes.linksTable.end(); ++itLink) {

        PhyLink * link = itLink->second.getValue();
        if (  link && (link->direction == direction )
            && (link->type == type)
            && (link->role == role)
            && (link->chOffset == chOffset)
            && (link->schedule.offset == offset) ) {
            entityIndexLink = itLink->first;
            return link;
        }
    }

    return NULL;
}


float Device::getAllocatedOutboundLink(Address16 peerDevice, NE::Model::Tdma::TdmaLinkTypes::TdmaLinkTypesEnum role ) {
    float allocatedOutbound = 0.0;
    LinkIndexedAttribute::iterator itLink;
    for (itLink = phyAttributes.linksTable.begin(); itLink != phyAttributes.linksTable.end(); ++itLink) {

        const PhyLink * link = itLink->second.getValue();
        if ( link && (link->neighbor == peerDevice)
            && (link->direction == Tdma::TdmaLinkDir::OUTBOUND )
            && (link->type == NE::Model::Tdma::LinkType::TRANSMIT)
            && (link->role == role ) ) {

            allocatedOutbound += ((float)SUPERFRAME_APPLICATION_LENGTH / link->schedule.interval);
        }
    }
    return allocatedOutbound;
}

float Device::getInboundAppTraffic(Uint16 slotsPerSec)
{
    if( inboundAppTraffic == 0 )
    {
        ContractIndexedAttribute::iterator itPhyContract = phyAttributes.contractsTable.begin();
        for (; itPhyContract != phyAttributes.contractsTable.end(); ++itPhyContract) {
            PhyContract * phyContract = itPhyContract->second.getValue();
//            if (!phyContract->isManagement || phyContract->usedForFirmwareUpdate) {
            if (phyContract && (phyContract->destination32 != ADDRESS16_MANAGER)) {//exclude any contract with SM
                Int16 period = ( phyContract->communicationServiceType == CommunicationServiceType::Periodic ?
                                        phyContract->assignedPeriod
                                     : -phyContract->assignedCommittedBurst );

                #warning the contracts for alarm formward are not counted for inbound traffic.. MUST verify the efect
                //HACK: contracts used to send alarm from device to gateway are not used for calculation of inbound traffic
                // this in case of a contract C/S with 1pkt / 15 sec
                // the alarms should be supported by management links
                //this is needed to avoid the creation of a backup link in case of a device with 2 C/S contracts
                // with the GW.. generate that no free slots found on creation of publish contracts.
                if (phyContract->communicationServiceType == CommunicationServiceType::NonPeriodic
                   && phyContract->sourceSAP == 0
                   && phyContract->assignedCommittedBurst <=-15
                   && phyContract->destination32 == ADDRESS16_GATEWAY ){
                    continue;
                }

                if( period > 0 ) {
                    inboundAppTraffic += (float)SUPERFRAME_APPLICATION_LENGTH / (period * slotsPerSec);
                } else {
                    inboundAppTraffic += (float)SUPERFRAME_APPLICATION_LENGTH * (-period) / slotsPerSec;
                }
            }
        }
    }
    return inboundAppTraffic;
}

float Device::getOutboundAppTraffic(Uint16 slotsPerSec)
{
    outboundAppTraffic = theoAttributes.getServedContractsTraffic();
    return outboundAppTraffic;
}

float Device::getOutboundAppTrafficLocalLoop(Uint16 slotsPerSec)
{
    if( outboundAppTraffic == 0 )
    {
        ContractIndexedAttribute::iterator itPhyContract = phyAttributes.contractsTable.begin();
        for (; itPhyContract != phyAttributes.contractsTable.end(); ++itPhyContract) {
            PhyContract * phyContract = itPhyContract->second.getValue();
            //verify is local loop contract
            if (phyContract && (!phyContract->isManagement && ((phyContract->destination32 != ADDRESS16_GATEWAY ) && ( phyContract->destination32 != ADDRESS16_MANAGER))) ) {
                Int16 period = ( -phyContract->assignedCommittedBurst );
                if( period > 0 ) {
                    outboundAppTraffic += (float)SUPERFRAME_APPLICATION_LENGTH / (period * slotsPerSec);
                } else {
                    outboundAppTraffic += (float)SUPERFRAME_APPLICATION_LENGTH * (-period) / slotsPerSec;
                }
            }
        }
        outboundAppTraffic += theoAttributes.getServedContractsTraffic();
    } else {
        outboundAppTraffic = theoAttributes.getServedContractsTraffic();
    }
    return outboundAppTraffic;
}


PhyLink * Device::getPhyLink(EntityIndex linkIndex, bool& isPendingStatus) {
    LinkIndexedAttribute::iterator iter = phyAttributes.linksTable.find(linkIndex);
    if (iter != phyAttributes.linksTable.end()) {
        isPendingStatus = iter->second.isPending;
        return iter->second.getValue();
    }

    return NULL;
}

PhyContract * Device::getPhyContract(EntityIndex contractIndex, bool& isPending) {
    ContractIndexedAttribute::iterator iter = phyAttributes.contractsTable.find(contractIndex);
    if (iter != phyAttributes.contractsTable.end()) {
        isPending = iter->second.isOnPending();
        return iter->second.getValue();
    }

    return NULL;
}

Uint16 Device::getOutBoundGraph(Address16 device) const {
    RouteIndexedAttribute::const_iterator itBrRoute = phyAttributes.routesTable.begin();
    Uint16 graphID = 0;
    for (; itBrRoute != phyAttributes.routesTable.end(); ++itBrRoute) {
        PhyRoute* route = (PhyRoute*) itBrRoute->second.getValue();
        if (route && (route->alternative == 2) && (route->selector == device) && (isRouteGraphElement(route->route[0]))) {
            graphID = getRouteElement(route->route[0]);
        }

    }

    return graphID;
}

bool  Device::deviceHasOutboundGraph(Address32 device) {
    RouteIndexedAttribute::iterator itBrRoute = phyAttributes.routesTable.begin();
    for (; itBrRoute != phyAttributes.routesTable.end(); ++itBrRoute) {
         PhyRoute* route = (PhyRoute*) itBrRoute->second.getValue();
         if (route && (route->alternative == 2) && (route->selector == Address::getAddress16(device))
                     && (isRouteGraphElement(route->route[0])) && (route->route.size() == 1)  ) {
            return true;
         }

     }
    return false;
}

Address16 Device::getGraphOwner(Uint16 graphId) {
    RouteIndexedAttribute::iterator itBrRoute = phyAttributes.routesTable.begin();
    for (; itBrRoute != phyAttributes.routesTable.end(); ++itBrRoute) {
        PhyRoute* route = (PhyRoute*) itBrRoute->second.getValue();
        if (!route) {
            continue;
        }
        if ((route->alternative == 2) && (isRouteGraphElement(route->route[0])) && (getRouteElement(route->route[0])== graphId)) {
            return route->selector;
        }

    }
    LOG_ERROR("The graph with id " << (int)graphId << " not found. Address16 0 will be returned.");
    return 0;
}

bool Device::existsPhyGraph(Uint64 entityIndexGraph) {
    GraphIndexedAttribute::iterator itGraph = phyAttributes.graphsTable.find(entityIndexGraph);
    return (itGraph != phyAttributes.graphsTable.end());

}

bool Device::existsPhyNeighbor(Uint64 entityIndexNeighbor) {
    NeighborIndexedAttribute::iterator itNeighbor = phyAttributes.neighborsTable.find(entityIndexNeighbor);
    return (itNeighbor != phyAttributes.neighborsTable.end());
}

bool Device::existsTransmisionManagementLink(Address16 destination, Tdma::TdmaLinkDir::TdmaLinkDirEnum direction) {
    LinkIndexedAttribute::iterator itLink;
    for (itLink = phyAttributes.linksTable.begin(); itLink != phyAttributes.linksTable.end(); ++itLink) {

        const PhyLink * link = itLink->second.getValue();
        if(   link &&  (link->neighbor == destination)
                &&  (direction == link->direction)
                &&  (link->type == NE::Model::Tdma::LinkType::TRANSMIT)
                &&  (NE::Model::Tdma::TdmaLinkTypes::MANAGEMENT == link->role)
                &&  (NE::Model::Tdma::NeighbourTypes::NEIGHBOUR_BY_ADDRESS == link->neighborType ) ) {
            return true;
        }
    }

    return false;
}

bool Device::hasNeighbor(Address16 neighbor) {
    for (std::map<EntityIndex, NeighborAttribute>::iterator it = phyAttributes.neighborsTable.begin();
                it != phyAttributes.neighborsTable.end(); ++it) {
        if (getIndex(it->first) == neighbor) {
            return true;
        }
    }

    return false;
}

bool Device::hasCandidate(Address16 candidate) {
    for (std::map<EntityIndex, CandidateAttribute>::iterator it = phyAttributes.candidatesTable.begin();
                it != phyAttributes.candidatesTable.end(); ++it) {
        if ((getEntityType(it->first) == EntityType::Candidate) && (getIndex(it->first) == candidate)) {
            return true;
        }
    }

    return false;
}

bool Device::hasOnlyOneContractWithPeer(Address32 peer, Uint16 contractId ) {
    for (std::map<EntityIndex, ContractAttribute>::iterator it = phyAttributes.contractsTable.begin();
                it != phyAttributes.contractsTable.end(); ++it) {
            PhyContract* contract = (PhyContract*) it->second.getValue();
            if(contract && contract->destination32 == peer && contract->contractID != contractId) {
                return false;
            }
    }
    return true;
}

bool Device::hasMultipleContractsWithPeerOnSameTSAP(Address32 peer , int sourceTSAP, int destinationTSAP) {
    Uint8 noContracts = 0;
    for (std::map<EntityIndex, ContractAttribute>::iterator it = phyAttributes.contractsTable.begin();
                it != phyAttributes.contractsTable.end(); ++it) {
            PhyContract* contract = (PhyContract*) it->second.getValue();
            if(contract && contract->destination32 == peer  &&
                        contract->sourceSAP == sourceTSAP  && contract->destinationSAP == destinationTSAP) {
                ++noContracts;
                if(noContracts == 2) {
                    return true;
                }
            }
    }
    return false;
}

Uint16 Device::getInboundGraphId() {
    for (std::map<EntityIndex, RouteAttribute>::iterator it = phyAttributes.routesTable.begin();
                it != phyAttributes.routesTable.end(); ++it) {
        if (getEntityType(it->first) == EntityType::Route) {
            PhyRoute* route = (PhyRoute*) it->second.getValue();
            if (!route) {
                continue;
            }
            if (isRouteGraphElement(route->route[0])) {
                return getRouteElement(route->route[0]);
            }
        }
    }

    return 0;
}

int Device::getLinkPeriod(const PhyLink * link) const{
    if (link->schedule.interval != 0){
        return link->schedule.interval;
    }
    EntityIndex index1 = createEntityIndex(this->address32, EntityType::Superframe, link->superframeIndex);
    SuperframeIndexedAttribute::const_iterator itSuperframe = this->phyAttributes.superframesTable.find(index1);
    if (itSuperframe == this->phyAttributes.superframesTable.end()) {
        return 0;
    }
    PhySuperframe * superframe1 = itSuperframe->second.getValue();
    return (superframe1 ? superframe1->sfPeriod : 0);
}

void Device::getClockSourceNeighbors(Address16 &prefered, Address16 &backup) {

    NeighborIndexedAttribute::iterator itPhyNeighbor = phyAttributes.neighborsTable.begin();
    for (; itPhyNeighbor != phyAttributes.neighborsTable.end(); ++itPhyNeighbor) {
        PhyNeighbor * phyNeighbor = itPhyNeighbor->second.getValue();
        if (phyNeighbor == NULL){
            continue;
        }
        if (phyNeighbor->clockSource == 2) {
            prefered = getIndex(itPhyNeighbor->first);
        }

        if (phyNeighbor->clockSource == 1) {
            backup = getIndex(itPhyNeighbor->first);
        }

    }


}

bool Device::existsContract(Uint16 contractID) {
    ContractIndexedAttribute::iterator itPhyContract = phyAttributes.contractsTable.begin();
    for (; itPhyContract != phyAttributes.contractsTable.end(); ++itPhyContract) {
        if(getIndex(itPhyContract->first) == contractID) {
            return true;
        }
    }

    return false;

}

std::ostream& operator<<(std::ostream& stream, const Device& device) {

    stream << "address64=" << device.address64.toString();
    stream << ", address32=" << Address::toString(device.address32);
    stream << ", address128=" << device.address128.toString();
    stream << ", joinedBackbone32=" << Address::toString(device.joinedBackbone32);
    stream << ", parent32=" << Address::toString(device.parent32);
    stream << ", capabilities=" << device.capabilities;
    stream << ", status=" << DeviceStatus::toString(device.status);
    stream << ", DPDUsTransmitted=" << (long long) device.deviceStatistics.DPDUsTransmitted;
    stream << ", DPDUsReceived=" << (long long) device.deviceStatistics.DPDUsReceived;
    stream << ", DPDUsFailedTransmission=" << (long long) device.deviceStatistics.DPDUsFailedTransmission;
    stream << ", DPDUsFailedReception=" << (long long) device.deviceStatistics.DPDUsFailedReception;
    stream << ", startTime=" << (long long) device.startTime;
    stream << ", lastPacketTAI=" << (long long) device.lastPacketTAI;
    stream << ", phyAttributes=" << device.phyAttributes;
    stream << ", theoAttributes=" << device.theoAttributes;

    return stream;
}

}
}
