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
 * model.cpp
 *
 *  Created on: Apr 13, 2010
 *      Author: Catalin Pop, beniamin.tecar, sorin.bidian
 */
#include "model.h"

namespace NE {
namespace Model {

#define DEALOCATE_ATTRIBUTE(attribute){\
        delete attribute.currentValue;\
        attribute.currentValue = NULL;\
        delete attribute.previousValue;\
        attribute.previousValue = NULL;\
}
#define DEALOCATE_ATTRIBUTE_TABLE(attributeType, attributeTable){\
    for (attributeType::iterator itAttribute = attributeTable.begin(); itAttribute != attributeTable.end(); ++itAttribute){\
        DEALOCATE_ATTRIBUTE(itAttribute->second);\
    }\
}

#define CREATE_INDEXED_ATTRIBUTE(IndexedAttributeType, AttributeType, table, index, attribute){\
    typedef std::pair<IndexedAttributeType::iterator, bool> InsertedPair;\
    InsertedPair insertedPair = table.insert(IndexedAttributeType::value_type(index, AttributeType(attribute, onPending)));\
    if (!insertedPair.second){\
        DEALOCATE_ATTRIBUTE(insertedPair.first->second);\
        table.erase(insertedPair.first);\
        table.insert(IndexedAttributeType::value_type(index, AttributeType(attribute, onPending)));\
    }\
}


PhyAttributes::PhyAttributes() :

contractsTableMetadata(new PhyMetaData(0, 3)),//min fo Router
neighborMetadata(new PhyMetaData(0, 2)), // min for IO
superframeMetadata(new PhyMetaData(0, 3)),//min for IO
graphMetadata(new PhyMetaData(0, 2)),//min for IO
linkMetadata(new PhyMetaData(0, 9)),//min for IO
routeMetadata(new PhyMetaData(0, 2)),//min for IO
diagMetadata(new PhyMetaData(0, 2))//min for IO
{
}

PhyAttributes::~PhyAttributes() {
    DEALOCATE_ATTRIBUTE(advInfo);
    DEALOCATE_ATTRIBUTE(advSuperframe);
    DEALOCATE_ATTRIBUTE(serialNumber);
    DEALOCATE_ATTRIBUTE(powerSupplyStatus);
    DEALOCATE_ATTRIBUTE(vendorID);
    DEALOCATE_ATTRIBUTE(modelID);
    DEALOCATE_ATTRIBUTE(softwareRevisionInformation);
    DEALOCATE_ATTRIBUTE(packagesStatistics);
    DEALOCATE_ATTRIBUTE(joinReason);
    DEALOCATE_ATTRIBUTE(contractsTableMetadata);
    DEALOCATE_ATTRIBUTE(neighborMetadata);
    DEALOCATE_ATTRIBUTE(superframeMetadata);
    DEALOCATE_ATTRIBUTE(graphMetadata);
    DEALOCATE_ATTRIBUTE(linkMetadata);
    DEALOCATE_ATTRIBUTE(routeMetadata);
    DEALOCATE_ATTRIBUTE(diagMetadata);
    DEALOCATE_ATTRIBUTE(hrcoCommEndpoint);
    DEALOCATE_ATTRIBUTE(hrcoEntityIndexListAttribute);
    DEALOCATE_ATTRIBUTE(deviceCapability);
    DEALOCATE_ATTRIBUTE(queuePriority);
    DEALOCATE_ATTRIBUTE(armoCommEndpoint);
    DEALOCATE_ATTRIBUTE(retryTimeout);
    DEALOCATE_ATTRIBUTE(max_Retry_Timeout_Interval);
    DEALOCATE_ATTRIBUTE(retryCount);
    DEALOCATE_ATTRIBUTE(dlmoMaxLifeTime);
    DEALOCATE_ATTRIBUTE(dlmoIdleChannels);
    DEALOCATE_ATTRIBUTE(pingInterval);
    DEALOCATE_ATTRIBUTE(dlmoDiscoveryAlert);
    DEALOCATE_ATTRIBUTE_TABLE(BlacklistChannelsIndexedAttribute, blacklistChannelsTable);
    DEALOCATE_ATTRIBUTE_TABLE(ChannelHoppingIndexedAttribute, channelHoppingTable);
    DEALOCATE_ATTRIBUTE_TABLE(LinkIndexedAttribute, linksTable);
    DEALOCATE_ATTRIBUTE_TABLE(SuperframeIndexedAttribute, superframesTable);
    DEALOCATE_ATTRIBUTE_TABLE(NeighborIndexedAttribute, neighborsTable);
    DEALOCATE_ATTRIBUTE_TABLE(CandidateIndexedAttribute, candidatesTable);
    DEALOCATE_ATTRIBUTE_TABLE(RouteIndexedAttribute, routesTable);
    DEALOCATE_ATTRIBUTE_TABLE(NetworkRouteIndexedAttribute, networkRoutesTable);
    DEALOCATE_ATTRIBUTE_TABLE(GraphIndexedAttribute, graphsTable);
    DEALOCATE_ATTRIBUTE_TABLE(ContractIndexedAttribute, contractsTable);
    DEALOCATE_ATTRIBUTE_TABLE(NetworkContractIndexedAttribute, networkContractsTable);
    DEALOCATE_ATTRIBUTE_TABLE(SessionKeyIndexedAttribute, sessionKeysTable);
    DEALOCATE_ATTRIBUTE_TABLE(SpecialKeyIndexedAttribute, masterKeysTable);
    DEALOCATE_ATTRIBUTE_TABLE(SpecialKeyIndexedAttribute, subnetKeysTable);
    DEALOCATE_ATTRIBUTE_TABLE(AddressTranslationIndexedAttribute, addressTranslationTable);
}

void PhyAttributes::createBlacklistChannel(EntityIndex index, PhyBlacklistChannels * blacklistChannels, bool onPending) {
    blacklistChannelsTable.insert(BlacklistChannelsIndexedAttribute::value_type(index, BlacklistChannelsAttribute(
                blacklistChannels, onPending)));
}

void PhyAttributes::createChannelHopping(EntityIndex index, PhyChannelHopping * channelHopping, bool onPending) {
    channelHoppingTable.insert(ChannelHoppingIndexedAttribute::value_type(index, ChannelHoppingAttribute(
                channelHopping, onPending)));
}

void PhyAttributes::createLink(EntityIndex index, PhyLink * link, bool onPending) {
    CREATE_INDEXED_ATTRIBUTE(LinkIndexedAttribute, LinkAttribute, linksTable, index, link);
}

void PhyAttributes::createSuperframe(EntityIndex index, PhySuperframe * superframe, bool onPending) {
    CREATE_INDEXED_ATTRIBUTE(SuperframeIndexedAttribute, SuperframeAttribute, superframesTable, index, superframe);
}

void PhyAttributes::createNeighbor(EntityIndex index, PhyNeighbor * neighbor, bool onPending) {
    CREATE_INDEXED_ATTRIBUTE(NeighborIndexedAttribute, NeighborAttribute, neighborsTable, index, neighbor);
}

void PhyAttributes::createCandidate(EntityIndex index, PhyCandidate * candidate, bool onPending) {
    CREATE_INDEXED_ATTRIBUTE(CandidateIndexedAttribute, CandidateAttribute, candidatesTable, index, candidate);
}

void PhyAttributes::removeCandidate(EntityIndex candidateIndex) {
    CandidateIndexedAttribute::iterator itCandidate = candidatesTable.find(candidateIndex);
    if (itCandidate != candidatesTable.end()) {
        DEALOCATE_ATTRIBUTE(itCandidate->second);
        candidatesTable.erase(itCandidate);
    }
}

void PhyAttributes::createRoute(EntityIndex index, PhyRoute * route, bool onPending) {
    CREATE_INDEXED_ATTRIBUTE(RouteIndexedAttribute, RouteAttribute, routesTable, index, route);
}

void PhyAttributes::createNetworkRoute(EntityIndex index, PhyNetworkRoute * networkRoute, bool onPending) {
    CREATE_INDEXED_ATTRIBUTE(NetworkRouteIndexedAttribute, NetworkRouteAttribute, networkRoutesTable, index,
                networkRoute);
}

void PhyAttributes::createGraph(EntityIndex index, PhyGraph * graph, bool onPending) {
    CREATE_INDEXED_ATTRIBUTE(GraphIndexedAttribute, GraphAttribute, graphsTable, index, graph);
}

void PhyAttributes::createContract(EntityIndex index, PhyContract * contract, bool onPending) {
    CREATE_INDEXED_ATTRIBUTE(ContractIndexedAttribute, ContractAttribute, contractsTable, index, contract);
}

void PhyAttributes::createNetworkContract(EntityIndex index, PhyNetworkContract * networkContract, bool onPending) {
    CREATE_INDEXED_ATTRIBUTE(NetworkContractIndexedAttribute, NetworkContractAttribute, networkContractsTable, index,
                networkContract);
}

void PhyAttributes::createSessionKey(EntityIndex index, PhySessionKey * sessionKey, bool onPending) {
    CREATE_INDEXED_ATTRIBUTE(SessionKeyIndexedAttribute, SessionKeyAttribute, sessionKeysTable, index, sessionKey);
}

void PhyAttributes::createMasterKey(EntityIndex index, PhySpecialKey * specialKey, bool onPending) {
    //using insert the pair is not added if index already exists in map
    CREATE_INDEXED_ATTRIBUTE(SpecialKeyIndexedAttribute, SpecialKeyAttribute, masterKeysTable, index, specialKey);
}

void PhyAttributes::createSubnetKey(EntityIndex index, PhySpecialKey * specialKey, bool onPending) {
    //using insert the pair is not added if index already exists in map
    CREATE_INDEXED_ATTRIBUTE(SpecialKeyIndexedAttribute, SpecialKeyAttribute, subnetKeysTable, index, specialKey);
}

void PhyAttributes::createAddressTranslation(EntityIndex index, PhyAddressTranslation * addressTranslation, bool onPending) {
    CREATE_INDEXED_ATTRIBUTE(AddressTranslationIndexedAttribute, AddressTranslationAttribute, addressTranslationTable,
                index, addressTranslation);
}

void PhyAttributes::setSoftwareRevisionInformation(const std::string& softwareRevision) {
    if (softwareRevisionInformation.currentValue == NULL) {
        softwareRevisionInformation.currentValue = new PhyString(softwareRevision);
    } else {
        softwareRevisionInformation.currentValue->value = softwareRevision;
    }
}

std::string PhyAttributes::getSoftwareRevisionInformation() {
    if (softwareRevisionInformation.getValue() == NULL) {
        return "";
    }
    return softwareRevisionInformation.getValue()->value;
}

void PhyAttributes::setDeviceCapability(const PhyDeviceCapability& deviceCapability_) {
    if (deviceCapability.currentValue == NULL) {
    	deviceCapability.currentValue = new PhyDeviceCapability(deviceCapability_);
    } else {
    	*deviceCapability.currentValue = deviceCapability_;
    }
}

/**
 * THis function clears all persistent data from a join to another.
 */
void PhyAttributes::clearPersistentData(){
    DEALOCATE_ATTRIBUTE(vendorID);
    DEALOCATE_ATTRIBUTE(modelID);
    DEALOCATE_ATTRIBUTE(softwareRevisionInformation);
    DEALOCATE_ATTRIBUTE(packagesStatistics);
    DEALOCATE_ATTRIBUTE(joinReason);
    DEALOCATE_ATTRIBUTE(serialNumber);
    DEALOCATE_ATTRIBUTE(contractsTableMetadata);
    DEALOCATE_ATTRIBUTE(neighborMetadata);
    DEALOCATE_ATTRIBUTE(superframeMetadata);
    DEALOCATE_ATTRIBUTE(graphMetadata);
    DEALOCATE_ATTRIBUTE(linkMetadata);
    DEALOCATE_ATTRIBUTE(routeMetadata);
    DEALOCATE_ATTRIBUTE(diagMetadata);
    DEALOCATE_ATTRIBUTE(armoCommEndpoint);
}

/**
 * Copy the persistent data from sourceDevice.
 * @param sourceDevice
 */
void PhyAttributes::movePersistentData(PhyAttributes& sourcePhyAttributes){
    delete this->vendorID.currentValue;
    this->vendorID.currentValue = sourcePhyAttributes.vendorID.currentValue;
    sourcePhyAttributes.vendorID.currentValue = NULL;

    delete this->modelID.currentValue;
    this->modelID.currentValue = sourcePhyAttributes.modelID.currentValue;
    sourcePhyAttributes.modelID.currentValue = NULL;

    delete this->softwareRevisionInformation.currentValue;
    this->softwareRevisionInformation.currentValue = sourcePhyAttributes.softwareRevisionInformation.currentValue;
    sourcePhyAttributes.softwareRevisionInformation.currentValue = NULL;

    delete this->packagesStatistics.currentValue;
    this->packagesStatistics.currentValue = sourcePhyAttributes.packagesStatistics.currentValue;
    sourcePhyAttributes.packagesStatistics.currentValue = NULL;

    delete this->joinReason.currentValue;
    this->joinReason.currentValue = sourcePhyAttributes.joinReason.currentValue;
    sourcePhyAttributes.joinReason.currentValue = NULL;

    delete this->serialNumber.currentValue;
    this->serialNumber.currentValue = sourcePhyAttributes.serialNumber.currentValue;
    sourcePhyAttributes.serialNumber.currentValue = NULL;

    delete this->contractsTableMetadata.currentValue;
    this->contractsTableMetadata.currentValue
                = sourcePhyAttributes.contractsTableMetadata.currentValue;
    sourcePhyAttributes.contractsTableMetadata.currentValue = NULL;

    delete this->neighborMetadata.currentValue;
    this->neighborMetadata.currentValue = sourcePhyAttributes.neighborMetadata.currentValue;
    sourcePhyAttributes.neighborMetadata.currentValue = NULL;

    delete this->superframeMetadata.currentValue;
    this->superframeMetadata.currentValue = sourcePhyAttributes.superframeMetadata.currentValue;
    sourcePhyAttributes.superframeMetadata.currentValue = NULL;

    delete this->graphMetadata.currentValue;
    this->graphMetadata.currentValue = sourcePhyAttributes.graphMetadata.currentValue;
    sourcePhyAttributes.graphMetadata.currentValue = NULL;

    delete this->linkMetadata.currentValue;
    this->linkMetadata.currentValue = sourcePhyAttributes.linkMetadata.currentValue;
    sourcePhyAttributes.linkMetadata.currentValue = NULL;

    delete this->routeMetadata.currentValue;
    this->routeMetadata.currentValue = sourcePhyAttributes.routeMetadata.currentValue;
    sourcePhyAttributes.routeMetadata.currentValue = NULL;

    delete this->diagMetadata.currentValue;
    this->diagMetadata.currentValue = sourcePhyAttributes.diagMetadata.currentValue;
    sourcePhyAttributes.diagMetadata.currentValue = NULL;

    delete this->armoCommEndpoint.currentValue;
    this->armoCommEndpoint.currentValue = sourcePhyAttributes.armoCommEndpoint.currentValue;
    sourcePhyAttributes.armoCommEndpoint.currentValue = NULL;
}

} // namespace model
} // namespace NE
