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
 * @author catalin.pop, sorin.bidian, ioan.pocol, ion.ticus, beniamin.tecar, eduard.budulea,
 */
#include "Subnet.h"
#include "Common/NETypes.h"
#include "SMState/SMStateLog.h"
#include "Model/ModelPrinter.h"
#include "Model/Tdma/AppAllocation.h"
#include "Model/Tdma/TdmaTypes.h"
#include "Model/modelDefault.h"
#include "Model/Operations/OperationsProcessor.h"
#include "Model/Operations/OperationsContainer.h"
#include "Model/Operations/DeleteAttributeOperation.h"
#include "Model/Operations/ReadAttributeOperation.h"
#include "Model/Operations/WriteAttributeOperation.h"
#include "Model/Routing/GraphPrinter.h"
#include "Model/IDeviceRemover.h"
#include "Model/AddressAllocator.h"
#include <arpa/inet.h>

using namespace NE::Common;
using namespace NE::Model;
using namespace NE::Model::Routing;
using namespace NE::Model::Operations;

#define MAX_GRAPH_ID 0x0FFF

namespace NE {

namespace Model {

namespace Predicates {

class EdgeDestinationPredicate {

        Address32 destination;

    public:

        EdgeDestinationPredicate(Address32 destination_) {
            destination = destination_;
        }

        bool operator()(EdgePointer edge) {
            return edge->getDestination() == destination;
        }
};

}

Subnet::Subnet(Uint16 _subnetId, Device * manager_, SettingsLogic *settingsLogic_) :
    subnetId(_subnetId), backbone(NULL), settingsLogic(settingsLogic_), lastGraphId(1), updateAdvPeriod(false) {

    memset(subnetNodes, 0, sizeof(subnetNodes));
    memset(subnetEdges, 0, sizeof(subnetEdges));
    //memset(nrJoinsPerDevice, 0, sizeof(nrJoinsPerDevice));
    mngChunksTable=new MngChunk* [MAX_NUMBER_OF_BANDWIDTH_CHUNCKS];
    for(int i = 0;i < MAX_NUMBER_OF_BANDWIDTH_CHUNCKS;++i) {
    	mngChunksTable[i]=new MngChunk[settingsLogic->getSubnetSettings(subnetId).numberOfFrequencies];
    }


    memset(advertiseChuncksTable, 0, sizeof(advertiseChuncksTable));
    memset(appSlots, 0, sizeof(appSlots));

    //default mark the slot 0 as used on all frequencies.
    //The slot 0 should be skiiped because the device sometime may skip this slot if to busy with calculations.
    appSlots[0] = 0xFFFF;

    subnetNodes[Address::getAddress16(manager_->address32)] = manager_;

    consistencyCheckIdx = 0;
    lastUnmatchDeviceId = MAX_NUMBER_OF_DEVICES - 1;
    lastLoggedDeviceId = 3;

    defaultGraphCanBeReevaluated = true;
    lastLockInboundGraphCanBeReevaluated = time(NULL);
    outBoundGraphCanBeEvaluated = true;
    lastLockOutboundGraphCanBeReevaluated = time(NULL);
}

Subnet::~Subnet() {

    // dealocate all devices from subnet. Must skip on manager or gateway this will be dealocated only by NetworkEngine.
    for (int i = 3; i < MAX_NUMBER_OF_DEVICES; ++i) {
        delete subnetNodes[i];
    }

    // deallocate
    for(int i = 0; i < MAX_NUMBER_OF_BANDWIDTH_CHUNCKS; ++i) {
      delete [] mngChunksTable[i];
    }
    delete [] mngChunksTable;
}

void Subnet::copySubnetShort(Subnet::PTR& subnet) {
    Device * gateway = subnet->getGateway();
    if (gateway != NULL) {
        Address16 address16 = Address::getAddress16(gateway->address32);
        subnetNodes[address16] = gateway;
        activeDevices.insert(address16);
    }
    this->lastGraphId = subnet->lastGraphId;
    this->lstDeleteDeviceCallbacks = subnet->lstDeleteDeviceCallbacks;
    this->lastLockInboundGraphCanBeReevaluated = subnet->lastLockInboundGraphCanBeReevaluated;
    this->lastLockOutboundGraphCanBeReevaluated = subnet->lastLockInboundGraphCanBeReevaluated;
    this->deviceJoinCounter = subnet->deviceJoinCounter;
}

Address32 Subnet::getAddress32(const Address64& address64) {

    Device* manager = getManager();
    if (manager && manager->address64 == address64) {
        return manager->address32;
    }

    Device* gateway = getGateway();
    if (gateway && gateway->address64 == address64) {
        return gateway->address32;
    }

    AddressMapping64_32::iterator mapping = addressMapping.find(address64);
    if (mapping == addressMapping.end()) {
        return 0;
    }

    return mapping->second;
}

Address32 Subnet::getAddress32(const  Address128& address128) {
//It is verified first if the 128 address is manager, GW or one of the backbones.
//These components already have a 128 address allocated and is used in join packets.
//For field devices the 128 address is configured by SM.
    Device* manager = getManager();
	if (manager && manager->address128 == address128) {
		return manager->address32;
	}

	Device* gateway = getGateway();
	if (gateway != NULL && gateway->address128 == address128) {
		return gateway->address32;
	}


    Device * backbone = getBackbone();
    if (backbone != NULL && backbone->address128 == address128) {
        return backbone->address32;
    }

	Address32 address32 = AddressAllocator::getAddress32(address128);
	Device* device = getDevice(address32);
	if (device == NULL){
		return 0;
	}
//this verification must be performed in case the address32 contained in address128 was assigned to a different device
//this can happen when the address128 is cached and device is removed and the address32 is assigned to a diffret device and then this
//method is called with the cached address.
	if (device->address128 == address128){
		return address32;
	} else {
		return 0;
	}

}

bool Subnet::isRouter(Address16 device) {
    if (subnetNodes[device]) {
        return ((subnetNodes[device])->capabilities.isRouting());
    }
    return false;
}

bool Subnet::isFieldDevice(Address16 device) {
    if (subnetNodes[device]) {
        return ((subnetNodes[device])->capabilities.isFieldDevice());
    }
    return false;
}

bool Subnet::isBackboneDevice(Address16 device) {
    if (subnetNodes[device]) {
        return ((subnetNodes[device])->capabilities.isBackbone());
    }
    return false;
}

bool Subnet::isManager(Address16 device) {
    if (subnetNodes[device]) {
        return ((subnetNodes[device])->capabilities.isManager());
    }
    return false;
}

bool Subnet::isGateway(Address16 device) {
    if (subnetNodes[device]) {
        return ((subnetNodes[device])->capabilities.isGateway());
    }
    return false;
}

//REQUEST: if changes are made referring to the content of activeDevices, please specify it in activeDevices description. (.h file)
void Subnet::addDevice(Device * device) {
    Address16 address16 = Address::getAddress16(device->address32);
    subnetNodes[address16] = device;
    activeDevices.insert(address16);

    JoinCounter& joinCounter = deviceJoinCounter[device->address64];
    ++joinCounter.joinCount;

    device->joinsCount = joinCounter.joinCount;
    device->fullJoinsCount = joinCounter.fullJoinCount;

    if (device->capabilities.isBackbone()) {
        if(device->capabilities.dllSubnetId != subnetId){
        	LOG_ERROR("Added backbone with capab subnet " << (int)device->capabilities.dllSubnetId << " to subnet " << (int)subnetId);
        }
        backbone = device;
    }
}

void Subnet::destroyDevice(const Address128 & deletedDevAddr128, const Address64 & deletedDevAddr64, const Address32 deletedDevAddr32) {

    LOG_INFO("destroyDevice : SubnetID=" << (int) subnetId
    		<< ", addr128=" << deletedDevAddr128.toString()
    		<< ", addr64=" << deletedDevAddr64.toString()
    		<< ", addr32=" << Address::toString(deletedDevAddr32));

    Uint16 address16 = getAddress16(deletedDevAddr32);
    assert(address16 < MAX_NUMBER_OF_DEVICES);

    Device * device = subnetNodes[address16];
    Uint16 deviceType = device->capabilities.deviceType;

    if (device == backbone) {//if teh destroyed device is backbone remove the shortcut
        backbone = NULL;
    }

    //pentru cazul in care se sterge device-ul si el este un peer al unui link din operationsProcessr.setAsFail...stergem device-ul mai tarziu
	try {
		for (Address16Set::iterator it = activeDevices.begin(); it != activeDevices.end(); ++it) {
			subnetEdges[address16][*it].reset();
			subnetEdges[*it][address16].reset();
		}

		DeletedDeviceListenersList::iterator itCallback = this->lstDeleteDeviceCallbacks.begin();
		for (; itCallback != this->lstDeleteDeviceCallbacks.end(); ++itCallback) {
			if (*itCallback) {
				(*itCallback)->deviceDeletedCallback(deletedDevAddr32, deviceType);
			}
		}
	} catch (std::exception& ex) {
		LOG_ERROR(ex.what());

		delete device;
		subnetNodes[address16] = NULL;
		activeDevices.erase(address16);

		throw ex;
	}

	delete device;
	subnetNodes[address16] = NULL;
	activeDevices.erase(address16);
}

Uint16 Subnet::getNextGraphId() {
    int numberOfFullSerachLoops = 0;
    do {
        if ((++lastGraphId) > MAX_GRAPH_ID) {
            if ((++numberOfFullSerachLoops) == 2) {
                THROW_EX2(NE::Common::NEException, "getNextGraphID() : All graph IDs are occupied ")
                ;
            }
            lastGraphId = 1;
        }
        if (graphs.find(lastGraphId) == graphs.end())
            break;
    } while (true);
    return (Uint16) lastGraphId;
}

Int8 Subnet::getDeviceLevel(Address32 address32) {
    Uint8 level = 0;
    Device * node = getDevice(address32);
    if (node == NULL) {
        LOG_ERROR("getDeviceLevel - device " << Address_toStream(address32) << " not found");
        return -1;
    }

    if (node->capabilities.isManager() || node->capabilities.isGateway()) {
        return 0;
    }

    while (!node->capabilities.isBackbone()) {
        node = getDevice(node->parent32);
        if (node == NULL){
            return level;
        }
        level++;
    }

    return level;
}

void Subnet::createGraph(Uint16 graphID, Address16 destination) {
    pair<GraphsMap::iterator, bool> ret;
    ret = graphs.insert(std::make_pair(graphID, new Graph(graphID, destination)));
    if (ret.second == false) {
        LOG_WARN("createGraph - trying to create a graph that already exists: " << (int) graphID
                    << ". The old graph remains unchanged.");
    }

    lastGraphId = graphID;
}

GraphPointer Subnet::getGraph(Uint16 graphID) {
    GraphsMap::iterator it = graphs.find(graphID);
    if (it != graphs.end()) {
        return it->second;
    }
    return GraphPointer();

}

void Subnet::registerDeleteDeviceCallback(NE::Model::IDeletedDeviceListener * deleteCallback) {

    if (deleteCallback) {
        if (std::find(lstDeleteDeviceCallbacks.begin(), lstDeleteDeviceCallbacks.end(), deleteCallback) == lstDeleteDeviceCallbacks.end()) {
            LOG_INFO("registerDeleteDeviceCallback - SubnetID=" << (int) subnetId);
            this->lstDeleteDeviceCallbacks.push_back(deleteCallback);
        } else {
            LOG_WARN("registerDeleteDeviceCallback already exist; SubnetID=" << (int) subnetId);
        }
    }
}

void  Subnet::addGraphToBeEvaluated(Uint16 graphId) {
    if(graphId != DEFAULT_GRAPH_ID) {//the outbound graph must be included only once in the graphsToBeEvaluated list
           GraphsList::iterator existsGraph = std::find(graphsToBeEvaluated.begin(), graphsToBeEvaluated.end(), graphId);
           if(existsGraph == graphsToBeEvaluated.end()) {
        	   graphsToBeEvaluated.push_back(graphId);
           }
    }  else {// the inbound graph must be included only 1 times in the graphsToBeEvaluated list
           if(graphsToBeEvaluated.empty() || graphsToBeEvaluated.front() != graphId) {
        	   graphsToBeEvaluated.push_front(graphId);
           }
       }
}

float Subnet::evalEdgeCost(Address32 source, Address32 destination, Uint16 evalGraphTraffic, bool isFieldDevice) {
    float evalEdgeCost = 0;
    if (isFieldDevice) {
        Address16 src16 = Address::getAddress16(source);
        Address16 dst16 = Address::getAddress16(destination);
        evalEdgeCost = (getEdge(src16, dst16)->getCostFactorBattery() * getEdge(src16, dst16)->getCostFactorRetry()
                    * (getEdge(src16, dst16)->getTraffic() + (evalGraphTraffic
                                * getEdge(src16, dst16)->getAllocationFactor()) / 100)
                    * getEdge(src16, dst16)->getCostFactorFail()) / 1000;
    }

    return evalEdgeCost;
}

float Subnet::getEdgeCost(Address16 source, Address16 destination) {
    if (subnetEdges[source][destination] != 0) {
        return subnetEdges[source][destination]->getCost();
    }
    return 0;
}

void Subnet::getOutBoundEdges(Address16 deviceAddress16, EdgesList &edges) {
    assert(deviceAddress16 < MAX_NUMBER_OF_DEVICES);
    Address16Set targetsList;
    subnetNodes[deviceAddress16]->getEdges(targetsList);

    for (Address16Set::iterator it = targetsList.begin(); it != targetsList.end(); it++) {
        if (subnetEdges[deviceAddress16][*it] != 0) {
            edges.push_back(subnetEdges[deviceAddress16][*it]);
        }
    }
}

void Subnet::getOutBoundEdgesTargets(Uint16 deviceAddress16, Address16Set &targets) {
    assert(deviceAddress16 < MAX_NUMBER_OF_DEVICES);
    Address16Set targetsList;
    if (subnetNodes[deviceAddress16]) {
        subnetNodes[deviceAddress16]->getEdges(targetsList);

        for (Address16Set::iterator it = targetsList.begin(); it != targetsList.end(); it++) {
            if (subnetEdges[deviceAddress16][*it] != 0) {
                LOG_DEBUG("out edge de" << deviceAddress16 << "=" << *it);
                targets.insert(*it);
            }
        }
    }
}

void Subnet::getInboundEdgesSources(Uint16 deviceAddress16, Address16Set &targets) {
    assert(deviceAddress16 < MAX_NUMBER_OF_DEVICES);
    Address16Set targetsList;
    subnetNodes[deviceAddress16]->getEdges(targetsList);

    for (Address16Set::iterator it = targetsList.begin(); it != targetsList.end(); it++) {
        if (subnetEdges[*it][deviceAddress16] != 0) {
            targets.insert(*it);
        }
    }
}

Uint8 Subnet::getDeviceNrOfNeighbors(Address16 device) {
    assert(device < MAX_NUMBER_OF_DEVICES);

    if (subnetNodes[device]) {
        return subnetNodes[device]->getEdgesNo();
    }

    return 0;
}

void Subnet::getInboundEdges(Address16 deviceAddress16, std::list<EdgePointer> &edges) {
    assert(deviceAddress16 < MAX_NUMBER_OF_DEVICES);
    Address16Set targetsList;
    subnetNodes[deviceAddress16]->getEdges(targetsList);

    for (Address16Set::iterator it = targetsList.begin(); it != targetsList.end(); it++) {
        if (subnetEdges[*it][deviceAddress16] != 0) {
            edges.push_back(subnetEdges[*it][deviceAddress16]);
        }
    }
}

void Subnet::getInboundEdgesToCycle(Address16Set devicesAddress16, Address16Set &cycle, std::list<EdgePointer> &edges) {
    for (Address16Set::iterator it = devicesAddress16.begin(); it != devicesAddress16.end(); it++) {
        for (Address16Set::iterator i = cycle.begin(); i != cycle.end(); i++) {
            if (subnetEdges[*it][*i] != 0) {
                edges.push_back(subnetEdges[*it][*i]);
            }
        }
    }
}

void Subnet::getChildrenList(Address16 device, Address16Set &childrenSet) {
    assert(device < MAX_NUMBER_OF_DEVICES);
    Address16Set inEdgesSources;
    getInboundEdgesSources(device, inEdgesSources);
    for (Address16Set::iterator it = inEdgesSources.begin(); it != inEdgesSources.end(); ++it) {
        if ((subnetNodes[*it] != 0) && (getAddress16((subnetNodes[*it])->parent32) == device)) {
            childrenSet.insert(*it);
        }
    }
}

void Subnet::getCountDirectChilds(Device * device, Uint8 &devices) {
    NeighborIndexedAttribute::iterator it = device->phyAttributes.neighborsTable.begin();
    for (; it != device->phyAttributes.neighborsTable.end(); ++it) {
        Device * child = getDevice(it->second.getValue()->index);
        if (child) {
            if ((child->parent32 == device->address32)
                        || (isBackupForDevice(Address::getAddress16(child->address32), Address::getAddress16(device->address32)))) {
                ++devices;
            }
        }
    }
}

void Subnet::getCountDirectChildsAndRouters(Device * device, Uint8 &routers, Uint8 &nonRouters) {
    NeighborIndexedAttribute::iterator it = device->phyAttributes.neighborsTable.begin();
    for (; it != device->phyAttributes.neighborsTable.end(); ++it) {
        Device * child = getDevice(it->second.getValue()->index);
        if (child) {
            if ((child->parent32 == device->address32)
                        || (isBackupForDevice(Address::getAddress16(child->address32), Address::getAddress16(device->address32)))) {
                if (child->capabilities.isRouting()) {
                    ++routers;
                } else {
                    ++nonRouters;
                }
            }
        }
    }
}

void Subnet::deleteEdgeFromGraph(Address16 source, Address16 destination, Uint16 graphID) {
    if (subnetEdges[source][destination] == NULL) {
        LOG_ERROR("Deletion of inexistent edge (skipped) :" << std::hex << source << "->" << destination);
        return;
    }
    subnetEdges[source][destination]->deleteGraph(graphID);
}

void Subnet::UpdateModelOnRemoveDevice(Address16 deviceToRemove) {
    assert(deviceToRemove < MAX_NUMBER_OF_DEVICES);
    Device* device = getDevice(deviceToRemove);
    RETURN_ON_NULL_MSG(device, "UpdateModelOnRemoveDevice: device is NULL");

    //remove devices from active devices
    activeDevices.erase(deviceToRemove);

    if (device->capabilities.isRouting()) {
        //deallocate mng chuncks
        for(int i=0;i < MAX_NUMBER_OF_BANDWIDTH_CHUNCKS;++i) {
            for(int j=0;j < settingsLogic->getSubnetSettings(subnetId).numberOfFrequencies;++j) {
            	if (mngChunksTable[i][j].owner == deviceToRemove) {
            		mngChunksTable[i][j].owner  = 0;
            	}
            }
        }

        for (Address16 * pAdv = (Address16 *) advertiseChuncksTable; (char*) pAdv < ((char*) advertiseChuncksTable)
                    + sizeof(advertiseChuncksTable); pAdv++) {
            if (*pAdv == deviceToRemove) {
                LOG_DEBUG("advertiseChuncksTable CLEAR " << Address_toStream(deviceToRemove));
                *pAdv = 0;
            }
        }

        std::ostringstream stream;
        for(int i = 0; i < MAX_NR_OF_ADV_BANDWIDTH_CHUNCKS; ++i) {
            for(int j = 0; j < MAX_NR_OF_ADV_PERIODS; ++j) {
                stream << std::hex << advertiseChuncksTable[i][j] << ", ";
            }
            stream << std::endl;
        }
		LOG_DEBUG("UpdateModelOnRemoveDevice : " << stream.str());
    }

    // release edges
    Address16Set sourceDevices;
    getInboundEdgesSources(deviceToRemove, sourceDevices);
    for (Address16Set::iterator it = sourceDevices.begin(); it != sourceDevices.end(); it++) {
        if (subnetEdges[*it][deviceToRemove] != 0) {
            subnetEdges[*it][deviceToRemove].reset();
        }
    }

    Address16Set targetsDevices;
    getOutBoundEdgesTargets(deviceToRemove, targetsDevices);
    for (Address16Set::iterator it = targetsDevices.begin(); it != targetsDevices.end(); it++) {
        if (subnetEdges[deviceToRemove][*it] != 0) {
            subnetEdges[deviceToRemove][*it].reset();
        }
    }


}

bool Subnet::getAvailableMngChunckInbound(Operations::OperationsContainer & operationsContainer, Device* device, Device* parent, Uint8 &setNo, Uint8 &freqOffset,
            Uint8 joinReservedSet) {

    Uint16Set usedChunks;
	if (parent->capabilities.isBackbone()) {
        for (Uint8 i = MAX_NUMBER_OF_BANDWIDTH_CHUNCKS - 1; i > joinReservedSet; i -= 2) {
            if (!parent->isUsedBcSetNumber(i) && ((device == parent) || !device->isUsedBcSetNumber(i))) {
                if (!isMngChunckReserved(i, 0)) {
                    setNo = i;
                    freqOffset = 0;
                    return true;
                }
            }
        }
    } else {
        Uint8 maxSetNo = parent->getMinInboundBC_Router();

        for (OperationsList::iterator it = operationsContainer.getUnsentOperations().begin(); it != operationsContainer.getUnsentOperations().end(); ++it) {
            if ((parent->address32 == (*it)->getOwner()) && (EntityType::Link == getEntityType((*it)->getEntityIndex()))) {
                PhyLink* link = (PhyLink*)(*it)->getPhysicalEntity();
                if ( link && (link->role == NE::Model::Tdma::TdmaLinkTypes::MANAGEMENT || link->role == NE::Model::Tdma::TdmaLinkTypes::UDO_FIRMWARE)) {
						Uint8 linkSet = link->schedule.offset % 100;
                        if ((link->type == NE::Model::Tdma::LinkType::TRANSMIT) &&  (link->direction == NE::Model::Tdma::TdmaLinkDir::INBOUND) ) {
                        	if( linkSet < maxSetNo ) {
									maxSetNo = linkSet;
                        	}
                        }
                        else if ((link->type == NE::Model::Tdma::LinkType::RECEIVE) &&  (link->direction == NE::Model::Tdma::TdmaLinkDir::OUTBOUND) ) {
                        	usedChunks.insert(linkSet);
                        }

                }
            }

        }

        for (Uint8 i = maxSetNo - 2; i > joinReservedSet; i -= 2) {
        	Uint16Set::iterator itUsed = usedChunks.find(i);
        	if (itUsed != usedChunks.end()) {
        		continue;
        	}
            if (!parent->isUsedBcSetNumber(i) && ((device == parent) || !device->isUsedBcSetNumber(i))) {
                for (Uint8 j = 1; j < getSubnetSettings().numberOfFrequencies; ++j) {
                    if (!isMngChunckReserved(i, j)) {
                        setNo = i;
                        freqOffset = j;
                        return true;
                    }
                }
            }
        }
        for (Uint8 i = MAX_NUMBER_OF_BANDWIDTH_CHUNCKS - 1; i >= maxSetNo; i -= 2) {
        	Uint16Set::iterator itUsed = usedChunks.find(i);
        	if (itUsed != usedChunks.end()) {
        		continue;
        	}
            if (!parent->isUsedBcSetNumber(i) && ((device == parent) || !device->isUsedBcSetNumber(i))) {
                for (Uint8 j = 1; j < getSubnetSettings().numberOfFrequencies; ++j) {
                    if (!isMngChunckReserved(i, j)) {
                        setNo = i;
                        freqOffset = j;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool Subnet::getAvailableMngChunckOutbound(Operations::OperationsContainer & operationsContainer, Device* parent, Device* device, Uint8 &setNo, Uint8 &freqOffset,
            Uint8 joinReservedSet) {

    Uint8 minSetNo = (parent->capabilities.isBackbone() ? joinReservedSet : parent->getMaxOutBoundBC(joinReservedSet));

    Uint16Set usedChunks;
    for (OperationsList::iterator it = operationsContainer.getUnsentOperations().begin(); it != operationsContainer.getUnsentOperations().end(); ++it) {
        if (EntityType::Link != getEntityType((*it)->getEntityIndex())) {
            continue;
        }
        if ((device->address32 == (*it)->getOwner())
                    || (parent->address32 == (*it)->getOwner()) ) {
            PhyLink* link = (PhyLink*)(*it)->getPhysicalEntity();
            if ( link && (link->role == NE::Model::Tdma::TdmaLinkTypes::MANAGEMENT || link->role == NE::Model::Tdma::TdmaLinkTypes::UDO_FIRMWARE)) {
						Uint8 linkSet = link->schedule.offset % 100;
                        if ((link->type == NE::Model::Tdma::LinkType::RECEIVE)&&  (link->direction == NE::Model::Tdma::TdmaLinkDir::OUTBOUND) ) {
							if( linkSet > minSetNo ) {
								minSetNo = linkSet;
							}
                        }
                        else if  ((link->type == NE::Model::Tdma::LinkType::TRANSMIT)&&  (link->direction == NE::Model::Tdma::TdmaLinkDir::INBOUND)) {
							usedChunks.insert(linkSet);
                        }
            }
        }
    }

    for (Uint8 i = minSetNo + 2; i <= MAX_NUMBER_OF_BANDWIDTH_CHUNCKS - 1; i += 2) {//find free chunk above minSetNo
    	Uint16Set::iterator itUsed = usedChunks.find(i);
    	if (itUsed != usedChunks.end()) {
    		continue;
    	}
        if (!parent->isUsedBcSetNumber(i) && ((device == parent) || !device->isUsedBcSetNumber(i))) {
            for (Uint8 j = 1; j < getSubnetSettings().numberOfFrequencies; ++j) {
                if (!isMngChunckReserved(i, j)) {
                    setNo = i;
                    freqOffset = j;
                    return true;
                }
            }
        }
    }
    if (!parent->capabilities.isBackbone()) {
        for (Uint8 i = joinReservedSet + 2; i <= minSetNo; i += 2) {//find free chunk below minSetNo
        	Uint16Set::iterator itUsed = usedChunks.find(i);
        	if (itUsed != usedChunks.end()) {
        		continue;
        	}
            if (!parent->isUsedBcSetNumber(i) && ((device == parent) || !device->isUsedBcSetNumber(i))) {
                for (Uint8 j = 1; j < getSubnetSettings().numberOfFrequencies; ++j) {
                    if (!isMngChunckReserved(i, j)) {
                        setNo = i;
                        freqOffset = j;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void Subnet::addEdge(EdgePointer edge) {
    assert(edge != NULL);
    LOG_DEBUG("addEdge " << std::hex << (int) edge->getSource() << "->" << (int) edge->getDestination() << " " << RsqiLevelEnum::toString(edge->getRadioQualityLevel()));

    subnetEdges[edge->getSource()][edge->getDestination()] = edge;

}

void Subnet::addEdgeToGraph(Address16 source, Address16 destination, Uint16 graphID) {
    GraphsMap::iterator itExistingGraph = graphs.find(graphID);
    if (itExistingGraph != graphs.end()) {
        RETURN_ON_NULL(itExistingGraph->second);
        itExistingGraph->second->addEdge(source, destination);
    }
    if (subnetEdges[source][destination]) {
        subnetEdges[source][destination]->addGraph(graphID);
    }
}

void Subnet::deleteGraphFromEdge(Address16 source, Address16 destination, Uint16 graphID) {
    if (source == 0 || destination == 0) {
        return;
    }
    if (subnetEdges[source][destination]) {
        subnetEdges[source][destination]->deleteGraph(graphID);
    }
}

void Subnet::addGraphToEdge(Address16 source, Address16 destination, Uint16 graphID) {
    if (source == 0 || destination == 0) {
        return;
    }
    if (subnetEdges[source][destination]) {
        subnetEdges[source][destination]->addGraph(graphID);
        subnetEdges[source][destination]->setEdgeStatus(Status::ACTIVE);
    }
}

bool Subnet::existsEdge(Address16 source, Address16 destination) {
    LOG_DEBUG("**source=" << std::hex << source);
    LOG_DEBUG("***destination=" << std::hex << destination);
    //    assert(source < MAX_NUMBER_OF_DEVICES);
    //    assert(destination < MAX_NUMBER_OF_DEVICES);
    if (source >= MAX_NUMBER_OF_DEVICES || destination >= MAX_NUMBER_OF_DEVICES) {
        LOG_ERROR("existsEdge called with outOfBounds addressess " << std::hex << source << "," << destination);
        return false;
    }

    if (subnetEdges[source][destination]) {
        return true;
    }

    return false;
}

void Subnet::removeDeviceFromGraph(Address16 deviceToRemove, Uint16 graphID) {
    GraphsMap::iterator itExistingGraph = graphs.find(graphID);
    if (itExistingGraph == graphs.end()) {
        LOG_ERROR("Remove device " << (int) deviceToRemove << " from graph " << (int) graphID << ". Graph not found.");
        return;
    }

    if (itExistingGraph->second) {
        itExistingGraph->second->removeDeviceFromGraph(deviceToRemove);
    }
}

Address16 Subnet::getAdvertiseChuncksTableElement(Uint8 advertiseSetId, Uint8 advPeriodId) {
    assert(advertiseSetId < MAX_NR_OF_ADV_BANDWIDTH_CHUNCKS);
    assert(advPeriodId < MAX_NR_OF_ADV_PERIODS);
    return advertiseChuncksTable[advertiseSetId][advPeriodId];
}

void Subnet::setAdvertiseChuncksTableElement(Uint8 advertiseSetId, Uint8 advPeriodId, Address16 address) {
    assert(advertiseSetId < MAX_NR_OF_ADV_BANDWIDTH_CHUNCKS);
    assert(advPeriodId < MAX_NR_OF_ADV_PERIODS);
    advertiseChuncksTable[advertiseSetId][advPeriodId] = address;
    LOG_DEBUG("advertiseChuncksTable SET: advertiseSetId=" << (int)advertiseSetId
                << ", advPeriodId=" << (int)advPeriodId << ", address=" << std::hex << address);

    std::ostringstream stream;
    for(int i = 0; i < MAX_NR_OF_ADV_BANDWIDTH_CHUNCKS; ++i) {
        for(int j = 0; j < MAX_NR_OF_ADV_PERIODS; ++j) {
            stream << std::hex << advertiseChuncksTable[i][j] << ", ";
        }
        stream << std::endl;
    }
    LOG_DEBUG("setAdvertiseChuncksTableElement : " << stream.str());
}

Uint32 Subnet::getInboundChildrenNo(Device * device) {
    Uint32 ulChildrenNo = 0;
    if (device && !device->isAlreadyVisited()) {
        Uint16 deviceAddress16 = Address::getAddress16(device->address32);
        device->setVisited();
        std::map<EntityIndex, NeighborAttribute>::iterator it;
        for (it = device->phyAttributes.neighborsTable.begin(); it != device->phyAttributes.neighborsTable.end(); ++it) {
            if (getEntityType(it->first) == EntityType::Neighbour) {
                Uint16 childShortAddr = getIndex(it->first);
                // LOG_DEBUG("childShortAddr=" << std::hex << (int)childShortAddr << ", deviceAddress16=" << (int)deviceAddress16);
                if (subnetEdges[childShortAddr][deviceAddress16]) {
                    Device * child = getDevice(childShortAddr);

                    if (child && (Address::getAddress16(child->parent32) == deviceAddress16)) {
                        if (child->capabilities.isRouting()) {
                            ulChildrenNo += 0x00010000;
                            ulChildrenNo += getInboundChildrenNo(child);
                        } else if (child->capabilities.isFieldDevice()) {
                            ulChildrenNo += 0x00000001;
                        }
                    }
                }
            }
        }
        device->unsetVisited();
    }
    return ulChildrenNo;
}
void Subnet::createGraphFromParent(Uint16 graphId, Uint16 parentGraph) {
    GraphsMap::iterator existsGraph = graphs.find(parentGraph);
    if (existsGraph != graphs.end()) {
        DoubleExitEdges parentGraphEdges = existsGraph->second->getGraphEdges();

        GraphsMap::iterator itGraph = graphs.find(graphId);
        if (itGraph == graphs.end()) {
            std::ostringstream stream;
            stream << "graphs doesn't contain graphId=" << (int) graphId;
            throw NEException(stream.str());
        }

        itGraph->second->setGraphEdges(parentGraphEdges);
        for (DoubleExitEdges::iterator it = parentGraphEdges.begin(); it != parentGraphEdges.end(); ++it) {
            if ((it->second.prefered != 0) && (subnetEdges[it->first][it->second.prefered])) {
                subnetEdges[it->first][it->second.prefered]->addGraph(graphId);
            }

            if ((it->second.backup != 0) && (subnetEdges[it->first][it->second.backup])) {
                subnetEdges[it->first][it->second.backup]->addGraph(graphId);
            }
        }
    } else {
        LOG_ERROR("Graph " << (int) parentGraph << " not found.");
    }
}

void Subnet::getNeighborsOnGraph(Uint16 graph, Uint16 device, Address16Set &neighbors) {
    GraphsMap::iterator existsGraph = graphs.find(graph);
    if (existsGraph != graphs.end()) {
        const DoubleExitEdges & edges = existsGraph->second->getGraphEdges();
        for (DoubleExitEdges::const_iterator it = edges.begin(); it != edges.end(); ++it) {
            if (it->first == device) {
                if (it->second.prefered != 0) {
                    neighbors.insert(it->second.prefered);
                }
                if (it->second.backup != 0) {
                    neighbors.insert(it->second.backup);
                }
            }
        }
    }
}

bool Subnet::existsConfirmedDevice(Address16 device) {
    assert(device < MAX_NUMBER_OF_DEVICES);
    if (subnetNodes[device]) {
        return (subnetNodes[device]->status == DeviceStatus::JOIN_CONFIRMED);
    }
    return false;
}

void Subnet::consistencyGraph(Device * device, std::string& consistencyCheck, bool& inconsistencyfound, Operations::OperationsContainer & operationsContainer) {
    GraphIndexedAttribute::iterator itPhyGraph = device->phyAttributes.graphsTable.begin();
    for (; itPhyGraph != device->phyAttributes.graphsTable.end(); ++itPhyGraph) {
        PhyGraph * phyGraph = itPhyGraph->second.getValue();
        if(!phyGraph) {
            continue;
        }

        if(!existsGraph(getIndex(itPhyGraph->first))) {
            LOG_INFO("GRAPH" << getIndex(itPhyGraph->first) << "doesn'y exist in the theoretical model..it will be deleted");
            DeleteAttributeOperationPointer deleteGraph(new DeleteAttributeOperation(itPhyGraph->first));
            operationsContainer.addOperation(deleteGraph, device);
            break;
        }

        for (std::vector<Uint16>::iterator itPhyGraphNeighbor = phyGraph->neighbors.begin(); itPhyGraphNeighbor
                    != phyGraph->neighbors.end(); ++itPhyGraphNeighbor) {

            NeighborIndexedAttribute::iterator itPhyNeighbor = device->phyAttributes.neighborsTable.begin();
            for (; itPhyNeighbor != device->phyAttributes.neighborsTable.end(); ++itPhyNeighbor) {
                PhyNeighbor * phyNeighbor = itPhyNeighbor->second.getValue();
                if(!phyNeighbor) {
                    continue;
                }
                if (phyNeighbor->index == *itPhyGraphNeighbor) {
                    break;
                }
            }
            if (itPhyNeighbor == device->phyAttributes.neighborsTable.end()) {
                LOG_WARN(consistencyCheck << " : Graph node " << Address::toString(*itPhyGraphNeighbor)
                            << " NOT in Neighbors Table. graph=" << *phyGraph);
                inconsistencyfound = true;
                addGraphToBeEvaluated(phyGraph->index);
            }
            Device * neighborDevice = getDevice(*itPhyGraphNeighbor);
            if (neighborDevice == NULL) {
                LOG_WARN(consistencyCheck << " : Graph node " << Address::toString(*itPhyGraphNeighbor)
                            << " is NULL. graph=" << *phyGraph);
                inconsistencyfound = true;
                addGraphToBeEvaluated(phyGraph->index);
            } else {//check for basic cycles
                EntityIndex indexNeighborGraph = createEntityIndex(neighborDevice->address32, EntityType::Graph,
                            phyGraph->index);
                GraphIndexedAttribute::iterator itGraphOnNeighbor = neighborDevice->phyAttributes.graphsTable.find(
                            indexNeighborGraph);
                if (itGraphOnNeighbor != neighborDevice->phyAttributes.graphsTable.end() &&
                            itGraphOnNeighbor->second.getValue()) {
                    std::vector<Uint16> & graphNodes = itGraphOnNeighbor->second.getValue()->neighbors;
                    std::vector<Uint16>::iterator itReverseNeighbor = std::find(graphNodes.begin(), graphNodes.end(),
                                getAddress16(device->address32));
                    if (itReverseNeighbor != graphNodes.end()) {
                        LOG_WARN(consistencyCheck << "Basic cycle with neighbor " << Address::toString(
                                    neighborDevice->address32) << " on graph:" << *phyGraph << " graph on neighbor " << *(itGraphOnNeighbor->second.getValue()));
                        GraphPointer subnetGraph = getGraph(phyGraph->index);
                        if (subnetGraph != NULL){
                            LOG_WARN(" Cycle on Graph " << (int)phyGraph->index << ": " << NE::Model::Routing::GraphPrinter(this, subnetGraph) );
                        }
                        inconsistencyfound = true;
                        addGraphToBeEvaluated(phyGraph->index);
                    }
                }
            }
        }
    }
}

void Subnet::periodicPhyConsistencyCheck(NE::Model::Operations::OperationsProcessor & operationsProcessor) {

    try {
        char reason[64];
        sprintf(reason, "Consistency check");
        OperationsContainerPointer containerConsistency(new OperationsContainer(reason));
        NE::Common::Address32Set inconsistentDevicesSet;

        std::string managerAddress32 = Address::toString(getManagerAddress32());
        const NE::Common::SubnetSettings& subnetSettings = getSubnetSettings();

        // check max one device at each periodicPhyConsistencyCheck
        bool checkDevice = false;
        bool inconsistencyfound = false;
        while (consistencyCheckIdx < MAX_NUMBER_OF_DEVICES && !checkDevice) {

            Device* device = subnetNodes[consistencyCheckIdx];
            inconsistencyfound = false;
            if (!device || device->statusForReports != StatusForReports::JOINED_AND_CONFIGURED || device->capabilities.isManager()
                        || device->capabilities.isGateway() || operationsProcessor.existsOperationsForDevice(
                        device->address32)) {
                ++consistencyCheckIdx;
                continue;
            }

            checkDevice = true;
            ++consistencyCheckIdx;

            std::string deviceAddress32 = Address::toString(device->address32);
            std::string consistencyCheck = "CONSISTENCY device=";
            consistencyCheck += deviceAddress32;

            LOG_DEBUG("PhyAttributes for device  " << deviceAddress32 << "(BEFORE periodicPhyConsistencyCheck): "/* << device->phyAttributes*/);
            LOG_INFO(consistencyCheck);

            // 1. check contract Device -> Manager
            {
                ContractIndexedAttribute::iterator itPhyContract = device->phyAttributes.contractsTable.begin();
                for (; itPhyContract != device->phyAttributes.contractsTable.end(); ++itPhyContract) {
                    PhyContract * phyContract = itPhyContract->second.getValue();
                    if (!phyContract) {
                        LOG_WARN(consistencyCheck
                                    << " : phyAttributes.contractsTable contains invalid(NULL) contracts!");
                        inconsistencyfound = true;
                        continue;
                    }

                    if (phyContract && phyContract->destination32 == getManagerAddress32()) {
                        break;
                    }
                }
                if (itPhyContract == device->phyAttributes.contractsTable.end()) {
                    LOG_WARN(consistencyCheck << " : contract " << deviceAddress32 << "->" << managerAddress32
                                << " not found!");
                    inconsistencyfound = true;
                }
            }

            // 2. for every contract a route must exist (route with selector=contactID OR a default route)
            if (!device->capabilities.isBackbone()) {
                for (ContractIndexedAttribute::iterator itPhyContract = device->phyAttributes.contractsTable.begin(); itPhyContract
                            != device->phyAttributes.contractsTable.end(); ++itPhyContract) {
                    PhyContract * phyContract = itPhyContract->second.getValue();

                    RouteIndexedAttribute::iterator itPhyRoute = device->phyAttributes.routesTable.begin();
                    PhyRoute * phyRoute = NULL;
                    for (; itPhyRoute != device->phyAttributes.routesTable.end(); ++itPhyRoute) {
                        itPhyRoute->second.getValue();
                        if (phyRoute && (phyRoute->alternative == 1 || phyRoute->alternative == 3)) { /*1=contractID, 3=default route*/
                            break;
                        }
                    }
                    if (phyRoute && (itPhyRoute == device->phyAttributes.routesTable.end())) {
                        LOG_WARN(consistencyCheck
                                    << " : route(based on contractID or default route) not found for contract="
                                    << *phyContract);
                        inconsistencyfound = true;
                    }
                }
            }

            // 3. for every route that is based on graph, the specified graph must exists.
            {
                RouteIndexedAttribute::iterator itPhyRoute = device->phyAttributes.routesTable.begin();
                for (; itPhyRoute != device->phyAttributes.routesTable.end(); ++itPhyRoute) {
                    PhyRoute * phyRoute = itPhyRoute->second.getValue();

                    if(!phyRoute) {
                        continue;
                    }
                    if (!((phyRoute->route.size() == 1 || phyRoute->route.size() == 2)
                                && isRouteGraphElement(phyRoute->route[0]))) { // based on graph/hybrid
                        continue;
                    }
                    Uint16 graphID = phyRoute->route[0];
                    graphID = (graphID & 0x0FFF);

                    GraphIndexedAttribute::iterator itPhyGraph = device->phyAttributes.graphsTable.begin();
                    for (; itPhyGraph != device->phyAttributes.graphsTable.end(); ++itPhyGraph) {
                        PhyGraph * phyGraph = itPhyGraph->second.getValue();
                        if(!phyGraph) {
                            continue;
                        }
                        if (phyGraph->index == graphID) {
                            break;
                        }
                    }
                    if (itPhyGraph == device->phyAttributes.graphsTable.end()) {
                        LOG_WARN(consistencyCheck << " : Route on graph and graph not found; route=" << *phyRoute);
                        inconsistencyfound = true;
                        if (phyRoute->alternative == 2) {
                            Address32 routeDestinationAddress = NE::Common::Address::createAddress32(subnetId,
                                        phyRoute->selector);
                            inconsistentDevicesSet.insert(routeDestinationAddress);
                            if (subnetNodes[phyRoute->selector] == NULL) {
                            	//delete route
                                DeleteAttributeOperationPointer deleteRoute(new DeleteAttributeOperation(itPhyRoute->first));
                                containerConsistency->addOperation(deleteRoute, device);
                            }
                        }
                    }
                }
            }

            // 4. for every route(source or hybrid) the first(source)/second(hybrid) node in routes vector must exist in NeighborsTable.
            {
                RouteIndexedAttribute::iterator itPhyRoute = device->phyAttributes.routesTable.begin();
                for (; itPhyRoute != device->phyAttributes.routesTable.end(); ++itPhyRoute) {
                    PhyRoute * phyRoute = itPhyRoute->second.getValue();
                    if(!phyRoute) {
                        continue;
                    }

                    if ((phyRoute->route.size() == 1 || phyRoute->route.size() == 2)
                                && isRouteGraphElement(phyRoute->route[0])) { // based on graph/hybrid
                        continue;
                    }

                    Uint16 neighborAddress16;
                    if (phyRoute->route.size() == 1) { // based on address
                        neighborAddress16 = phyRoute->route[0];
                    } else {
                        LOG_WARN(consistencyCheck << " : neighbor not found for route(source or hybrid); route="
                                    << *phyRoute);
                        inconsistencyfound = true;
                        continue;
                    }

                    NeighborIndexedAttribute::iterator itPhyNeighbor = device->phyAttributes.neighborsTable.begin();
                    for (; itPhyNeighbor != device->phyAttributes.neighborsTable.end(); ++itPhyNeighbor) {
                        PhyNeighbor * phyNeighbor = itPhyNeighbor->second.getValue();
                        if (phyNeighbor && (phyNeighbor->index == neighborAddress16)) {
                            break;
                        }
                    }
                    if (itPhyNeighbor == device->phyAttributes.neighborsTable.end()) {
                        LOG_WARN(consistencyCheck << " : neighbor not found for route(source or hybrid); route="
                                    << *phyRoute);
                        inconsistencyfound = true;
                    }
                }
            }

            // 5. in bbr exist a route with selector(destination) = current device(the outbound route)
            {
                if (!device->capabilities.isBackbone()) {
                    RouteIndexedAttribute::iterator itPhyRoute = backbone->phyAttributes.routesTable.begin();
                    for (; itPhyRoute != backbone->phyAttributes.routesTable.end(); ++itPhyRoute) {
                        PhyRoute * phyRoute = itPhyRoute->second.getValue();
                        if (!phyRoute) {
                            LOG_WARN(consistencyCheck << " : Device " << Address::toString(backbone->address32)
                                        << " : phyAttributes.routesTable contains invalid(NULL) routes!");
                            inconsistencyfound = true;
                            continue;
                        }
                        if ((phyRoute->alternative == 2 ) && (phyRoute->selector == getAddress16(device->address32)) /*&& (phyRoute->route.size() == 1)*/) {
                            break;
                        }
                    }
                    if (itPhyRoute == backbone->phyAttributes.routesTable.end()) {
                        LOG_WARN(consistencyCheck << " : Device " << Address::toString(backbone->address32)
                                    << " : not found a route with selector(destination)=" << deviceAddress32
                                    << "(the outbound route)");
                        inconsistencyfound = true;
                    }
                }
            }

            // 6. for every graph the nodes from neighbors vector must exists also in Neighbors Table.
            // Check basic cycle: neighbor in graph MUST not have current device as neighbor on the same graph()
            char reason[128];
            sprintf(reason, "Device %s -DELETE not existing graphs",
                        device->address64.toString().c_str());
            OperationsContainerPointer containerClean(new OperationsContainer(reason));
            consistencyGraph(device, consistencyCheck, inconsistencyfound, *containerClean);
            if(!containerClean->isContainerEmpty()) {
                operationsProcessor.addOperationsContainer(containerClean);
            }

            // 7. for every transmit link :
            //      -the destination device must exist in NeighborsTable
            //      -the destination device must exist (JOIN_CONFIRMED)
            //      -Optional: on the destination device must exist a Receive link on the current link slot
            {
                LinkIndexedAttribute::iterator itPhyLink = device->phyAttributes.linksTable.begin();
                for (; itPhyLink != device->phyAttributes.linksTable.end(); ++itPhyLink) {
                    PhyLink * phyLink = itPhyLink->second.getValue();
                    if(!phyLink) {
                        continue;
                    }

                    if (!(phyLink->type & Tdma::LinkType::TRANSMIT  && phyLink->neighborType == 1)) { // If dlmo11a.Link[].NeighborType=1, dlmo11a.Link[].Neighbor designates index into the dlmo11a.Neighbor attribute.
                        continue;
                    }

                    if (!device->hasNeighbor(phyLink->neighbor)) {
                        LOG_WARN(consistencyCheck << " : No neighbor for link=" << *phyLink);
                        inconsistencyfound = true;
                    }
                    if ((phyLink->type == Tdma::LinkType::TRANSMIT)
                    		&& ( phyLink->neighborType == 1)
                    		&& (subnetNodes[ phyLink->neighbor] == NULL)) {//neighbor device doesn't exist anymore
                    	//delete route
                        DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(itPhyLink->first));
                        containerConsistency->addOperation(deleteLink, device);
                    }

                    Device * neighborDevice = getDevice(phyLink->neighbor);
                    if (!(neighborDevice && neighborDevice->status == DeviceStatus::JOIN_CONFIRMED)) {
                        LOG_WARN(consistencyCheck << " : Destination device NULL or not confirmed for link="
                                    << *phyLink);
                        inconsistencyfound = true;
                    }

                    if (neighborDevice && !operationsProcessor.existsOperationsForDevice(neighborDevice->address32)) {
                        LinkIndexedAttribute::iterator itNeighborPhyLink =
                                    neighborDevice->phyAttributes.linksTable.begin();
                        for (; itNeighborPhyLink != neighborDevice->phyAttributes.linksTable.end(); ++itNeighborPhyLink) {
                            PhyLink * phyNeighborLink = itNeighborPhyLink->second.getValue();
                            if (!phyNeighborLink) {
                                LOG_WARN(consistencyCheck << " : Device " << Address::toString(neighborDevice->address32)
                                            << " : phyAttributes.linksTable contains invalid(NULL) links!");
                                inconsistencyfound = true;
                                continue;
                            }

                            if (!(phyNeighborLink->type == Tdma::LinkType::RECEIVE)) {
                                continue;
                            }
                            if (phyNeighborLink->schedType == Tdma::ScheduleType::RANGE) {
                                if ((phyLink->schedule.offset >= phyNeighborLink->schedule.offset)
                                            && (phyLink->schedule.offset <= phyNeighborLink->schedule.interval)) { //check if the ofset of T link is in range of R link
                                    break;
                                }//TODO should be verified all the repetitions
                            } else {
                                int period1 = device->getLinkPeriod(phyLink);//if period is 0 use the period from superframe of the link
                                int period2 = neighborDevice->getLinkPeriod(phyNeighborLink);//if period is 0 use the period from superframe of the link
                                if (period1 == 0) {
                                    LOG_WARN(consistencyCheck << Address_toStream(device->address32) << " : No superframe for link " << phyLink);
                                    inconsistencyfound = true;
                                    break;
                                }
                                if (period2 == 0) {
                                    LOG_WARN(consistencyCheck << Address_toStream(neighborDevice->address32) << " : No superframe for link " << phyNeighborLink);
                                    inconsistencyfound = true;
                                    break;
                                }
                                if (Tdma::isOverlapping(phyLink->schedule.offset, period1, phyNeighborLink->schedule.offset, period2, subnetSettings.getSlotsPer30Sec())) {
#warning "what happens if the 2 links are from diffrent superframes"
                                    break;
                                }
                            }
                        }
                        if (itNeighborPhyLink == neighborDevice->phyAttributes.linksTable.end()) {
                            LOG_WARN(consistencyCheck << " : On destination device:" << Address_toStream(neighborDevice->address32)
                                        << " there is no Receive on slot=" << std::hex << (int) phyLink->schedule.offset
                                        << " for link=" << *phyLink);
                            inconsistencyfound = true;
                        }
                    }
                }
            }

            // 8. check if exists pending entities and no pending operations(no operations in OperationProcessor).
            consistencyCheckForPendings(device, consistencyCheck, inconsistencyfound);


            //9. check clock source loop
            markAllDevicesUnVisited();
            consistencyCheckForClockSourceLoops(device, consistencyCheck, inconsistencyfound);
            markAllDevicesUnVisited();

            //10. check addressTranslation Table
            AddressTranslationIndexedAttribute::iterator itPhyAddress = device->phyAttributes.addressTranslationTable.begin();
            for (; itPhyAddress != device->phyAttributes.addressTranslationTable.end(); ++itPhyAddress) {
					PhyAddressTranslation * phyAddress = itPhyAddress->second.getValue();
					if(!phyAddress) {
						continue;
					}
                    if (subnetNodes[phyAddress->shortAddress] == NULL) {
                    	//delete ATT
                        DeleteAttributeOperationPointer deleteATT(new DeleteAttributeOperation(itPhyAddress->first));
                        containerConsistency->addOperation(deleteATT, device);
                    }
            }

            //11. check NEighborsTable
            NeighborIndexedAttribute::iterator itPhyNeighbor = device->phyAttributes.neighborsTable.begin();
            for (; itPhyNeighbor != device->phyAttributes.neighborsTable.end(); ++itPhyNeighbor) {
                PhyNeighbor * phyNeighbor = itPhyNeighbor->second.getValue();
                if ( !phyNeighbor) {
                	continue;
                }
                if (subnetNodes[phyNeighbor->index] == NULL) {
                	//delete Neighbor
                    DeleteAttributeOperationPointer deleteNeighbor(new DeleteAttributeOperation(itPhyNeighbor->first));
                    containerConsistency->addOperation(deleteNeighbor, device);
                }

            }

            //12. check graphs
            GraphIndexedAttribute::iterator itPhyGraph = device->phyAttributes.graphsTable.begin();
            for (; itPhyGraph != device->phyAttributes.graphsTable.end(); ++itPhyGraph) {
                PhyGraph * phyGraph = itPhyGraph->second.getValue();
                if ( !phyGraph) {
                	continue;
                }
                if (!existsGraph(phyGraph->index) && phyGraph->index != DEFAULT_GRAPH_ID) {
                	//delete Graph
                    DeleteAttributeOperationPointer deleteGraph(new DeleteAttributeOperation(itPhyGraph->first));
                    containerConsistency->addOperation(deleteGraph, device);
                }

            }

            //13. check network route
            NetworkRouteIndexedAttribute::iterator itNetRoute = device->phyAttributes.networkRoutesTable.begin();
            for (; itNetRoute != device->phyAttributes.networkRoutesTable.end(); ++itNetRoute) {
                PhyNetworkRoute* netRoute = itNetRoute->second.getValue();
                if (!netRoute) {
                	continue;
                }

                Address32 address32 = getAddress32(netRoute->destination);
                if (device->capabilities.isBackbone() && address32 == 0) {
                	continue;
                }
                Address16 destination = Address::getAddress16(address32);
                if ( subnetNodes[destination] == NULL ) {
                    DeleteAttributeOperationPointer deleteNetworkRoute(new DeleteAttributeOperation(itNetRoute->first));
                    containerConsistency->addOperation(deleteNetworkRoute, device);
                }
            }

            if (inconsistencyfound) {
                inconsistentDevicesSet.insert(device->address32);
            }
        }

        if ( !containerConsistency->isContainerEmpty() ) {
        	operationsProcessor.addOperationsContainer(containerConsistency);
        }

        char reasonCheckPath[64];
        sprintf(reasonCheckPath, "Inconsitent devices check path");
        OperationsContainerPointer container(new OperationsContainer(reasonCheckPath));
        for (NE::Common::Address32Set::iterator it = inconsistentDevicesSet.begin(); it != inconsistentDevicesSet.end(); ++it) {
            if ( (*it == ADDRESS16_GATEWAY) || (*it == ADDRESS16_MANAGER)) {
                continue;
            }

            Device* ownerDevice = getDevice(*it);
            if (ownerDevice == NULL) {
                LOG_WARN("Inconsistent device already removed: " << std::hex << *it);
                continue;
            }
            EntityIndex indexBatery = createEntityIndex(*it, EntityType::PowerSupply, 0);
            ReadAttributeOperationPointer readBatery(new ReadAttributeOperation(indexBatery));
            container->addOperation(readBatery, ownerDevice);
        }

        operationsProcessor.addOperationsContainer(container);


        if (consistencyCheckIdx == MAX_NUMBER_OF_DEVICES) {
            consistencyCheckIdx = 0;
        }
    } catch (std::exception& ex) {
        LOG_ERROR(ex.what());
    }
}
#define defineStr(x) #x

#define CHECK_PENDING_ATTRIBUTE(attribute, consistencyCheck)\
    if (device->phyAttributes.attribute.isOnPending() ) {\
        LOG_WARN(consistencyCheck << " : " << defineStr(attribute) << " is pending!");\
        inconsistencyfound = true;\
    }

#define CHECK_PENDING_ATTRIBUTE_TABLE(attributeTable, AttributeTableType, consistencyCheck)\
    for(AttributeTableType::iterator it = device->phyAttributes.attributeTable.begin(); it != device->phyAttributes.attributeTable.end(); ++it) {\
        if (it->second.isOnPending() ) {\
            LOG_WARN(consistencyCheck << " : " << defineStr(AttributeTableType) << ":" << std::hex << it->first << " is pending!");\
            inconsistencyfound = true;\
        }\
    }



void Subnet::consistencyCheckForPendings(Device* device, std::string& consistencyCheck, bool& inconsistencyfound) {

    std::string deviceAddress32 = Address::toString(device->address32);

    //Simple attributes
    CHECK_PENDING_ATTRIBUTE(advInfo, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(advSuperframe, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(powerSupplyStatus, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(vendorID, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(modelID, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(softwareRevisionInformation, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(packagesStatistics, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(joinReason, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(contractsTableMetadata, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(neighborMetadata, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(superframeMetadata, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(graphMetadata, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(linkMetadata, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(routeMetadata, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(diagMetadata, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(hrcoCommEndpoint, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(hrcoEntityIndexListAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(armoCommEndpoint, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(deviceCapability, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(retryTimeout, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(max_Retry_Timeout_Interval, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(retryCount, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE(dlmoDiscoveryAlert, consistencyCheck);
    //
    //    //Indexed Attributes
    CHECK_PENDING_ATTRIBUTE_TABLE(blacklistChannelsTable, BlacklistChannelsIndexedAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE_TABLE(channelHoppingTable, ChannelHoppingIndexedAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE_TABLE(linksTable, LinkIndexedAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE_TABLE(superframesTable, SuperframeIndexedAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE_TABLE(neighborsTable, NeighborIndexedAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE_TABLE(candidatesTable, CandidateIndexedAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE_TABLE(routesTable, RouteIndexedAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE_TABLE(networkRoutesTable, NetworkRouteIndexedAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE_TABLE(graphsTable, GraphIndexedAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE_TABLE(contractsTable, ContractIndexedAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE_TABLE(networkContractsTable, NetworkContractIndexedAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE_TABLE(sessionKeysTable, SessionKeyIndexedAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE_TABLE(masterKeysTable, SpecialKeyIndexedAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE_TABLE(subnetKeysTable, SpecialKeyIndexedAttribute, consistencyCheck);
    CHECK_PENDING_ATTRIBUTE_TABLE(addressTranslationTable, AddressTranslationIndexedAttribute, consistencyCheck);

    /* LOG_ERROR("Device " << deviceAddress32 << ": " << str(it->first) << " : " << str(AttributeTableType) << " is pending!");*/

}

void Subnet::consistencyCheckForClockSourceLoops(Device* device, std::string& consistencyCheck, bool& inconsistencyfound) {
    if(!device) {
        LOG_WARN("Device " << std::hex << (int)Address::getAddress16(device->address32) << "is null!");
        return ;
    }

    Address16 prefered = 0;
    Address16 secondary = 0;

    device->getClockSourceNeighbors(prefered, secondary);

    if(prefered) {
        bool isCycle = false;
        Address16 cycleDst = 0;
        checkClockSourceCycle(prefered, Address::getAddress16(device->address32), cycleDst, isCycle);
        if(isCycle) {
            LOG_WARN(consistencyCheck << " : ClockSource loop between " << std::hex << (int)Address::getAddress16(device->address32) << " and " <<
                        (int)cycleDst);
        }
    }

    if(secondary) {
        bool isCycle = false;
        Address16 cycleDst = 0;
        checkClockSourceCycle(secondary, Address::getAddress16(device->address32), cycleDst, isCycle);
        if(isCycle) {
            LOG_WARN(consistencyCheck << " : ClockSource loop between " << std::hex << (int)Address::getAddress16(device->address32) << " and " <<
                    (int)cycleDst);
        }
    }

}
void Subnet::checkClockSourceCycle(Address16 preferedClockSrc, Address16 cycleSrc, Address16 &cycleDst, bool& isCycle ) {

	Device * device = getDevice(preferedClockSrc);
    if(!preferedClockSrc || !device ) {
        return ;
    }

    if(device->isAlreadyVisited()) {
        return;
    }

    if(preferedClockSrc == cycleSrc) {
        cycleDst = preferedClockSrc;
        isCycle = true;
        return;
    }



    Address16 prefered = 0;
    Address16 secondary = 0;

    device->getClockSourceNeighbors(prefered, secondary);

    device->setVisited();

    checkClockSourceCycle( prefered, cycleSrc, cycleDst, isCycle );
    checkClockSourceCycle( secondary, cycleSrc, cycleDst, isCycle );

}

void Subnet::checkPreferedClockSourceCycle(Address16 src, Address16 dst, bool& isCycle ) {

    Device * srcDevice = getDevice(src);
    if(!srcDevice ) {
        return ;
    }

    if(src == dst) {
        isCycle = true;
        return;
    }

    Address16 prefered = 0;
    Address16 secondary = 0;

    srcDevice->getClockSourceNeighbors(prefered, secondary);
    checkPreferedClockSourceCycle( prefered, dst, isCycle );
}


bool Subnet::hasChildSpaceForDevice(Device * parent, Device * child) {
    if(!parent || !child) {
        return false;
    }

    int routerLimit = parent->capabilities.isBackbone() ? getSubnetSettings().nrRoutersPerBBR : getSubnetSettings().nrRoutersPerRouter;
    int ioLimit = parent->capabilities.isBackbone() ? getSubnetSettings().nrNonRoutersPerBBR : getSubnetSettings().nrNonRoutersPerRouter;

    Uint8 routers = 0;
    Uint8 nonRouters = 0;
    getCountDirectChildsAndRouters(parent, routers, nonRouters);
    bool hasChildSpace = false;
    if((child->parent32 == parent->address32) || isBackupForDevice(Address::getAddress16(child->address32), Address::getAddress16(parent->address32))) {
        if(child->capabilities.isRouting()) {
            --routers;
        } else {
            --nonRouters;
        }
    }

    if(child->capabilities.isRouting()) {
        hasChildSpace = (routers < routerLimit);
    } else {
        hasChildSpace = (nonRouters < ioLimit);
    }

    return hasChildSpace;
}

bool Subnet::hasOUTChunksSpaceForChild(Device * parent, Device * child) {

    bool hasOUTChunksSpace = false;

    int routerLimit = parent->capabilities.isBackbone() ? getSubnetSettings().nrRoutersPerBBR : getSubnetSettings().nrRoutersPerRouter;
    int ioLimit = parent->capabilities.isBackbone() ? getSubnetSettings().nrNonRoutersPerBBR : getSubnetSettings().nrNonRoutersPerRouter;

    Uint8 routers = 0;
    Uint8 nonRouters = 0;
    for ( BCList::iterator it = parent->theoAttributes.mngChunckOutbound.begin(); it != parent->theoAttributes.mngChunckOutbound.end(); ++it) {
        if ( it->usedForRouter) {
            routers++;
        } else {
            nonRouters++;
        }
    }

    if((child->parent32 == parent->address32) || isBackupForDevice(Address::getAddress16(child->address32), Address::getAddress16(parent->address32))) {
        if(child->capabilities.isRouting()) {
            --routers;
        } else {
            --nonRouters;
        }
    }

    if(child->capabilities.isRouting()) {
        hasOUTChunksSpace = (routers < routerLimit);
    } else {
        hasOUTChunksSpace = (nonRouters < ioLimit );
    }

    return hasOUTChunksSpace;
}

float Subnet::getChildsOccupationPercent(Device* device) {
    int routerLimit = device->capabilities.isBackbone() ? getSubnetSettings().nrRoutersPerBBR : getSubnetSettings().nrRoutersPerRouter;
    int ioLimit = device->capabilities.isBackbone() ? getSubnetSettings().nrNonRoutersPerBBR : getSubnetSettings().nrNonRoutersPerRouter;

    Uint8 routers = 0;
    Uint8 nonRouters = 0;

    getCountDirectChildsAndRouters(device, routers, nonRouters);

    float chlidSpacePercent = (float)(routers + nonRouters);
    chlidSpacePercent = (chlidSpacePercent * 100 ) / (routerLimit + ioLimit);

    return chlidSpacePercent;

}

bool Subnet::candidateIsEligibleAsNewParent(Address16 deviceAddress, Address16 candidate) {
    Device * device = getDevice(deviceAddress);
    Device * candidateDevice = getDevice(candidate);
    if(device && candidateDevice ) {
        bool hasChildSpace = hasChildSpaceForDevice(candidateDevice, device);
        bool hasLinksSpace = !deviceReachedTheMaxLinksNo(candidate, device->address64, false);
        const bool hasChunkSpace = hasOUTChunksSpaceForChild(candidateDevice, device);
        bool hasLinksSpaceOnEdge = ( checkMaxNumberOfLinksOnEdge(deviceAddress, candidate, 4 ) == ResponseStatus::SUCCESS) ; // 2 Management links + 1 AppLinks for publish + 1 for C/S from GW
        if ((candidateDevice->capabilities.isRouting() || candidateDevice->capabilities.isBackbone())
                    && hasChildSpace
                    && hasLinksSpace
                    && hasLinksSpaceOnEdge
                    && hasChunkSpace
                    && !device->theoAttributes.candidateIsBadRate(candidate)) {
            return true;
        }
    }

    return false;
}

bool Subnet::candidateIsEligibleAsBackup(Address16 deviceAddress, Address16 candidate, bool isFinalPhase) {
    Device * device = getDevice(deviceAddress);
    Device * candidateDevice = getDevice(candidate);
    if ( !device ) {
    	return false;
    }
    if ( !candidateDevice ) {
    	return false;
    }
    if ( !isFinalPhase ) {//if this is not the final step of backup find algorithm, check is new backup is on the prefered path
		if ( !candidateDevice->capabilities.isBackbone() && candidateNotValidOrIsOnPreferedPath(device, candidateDevice)) {
			return false;
		}
    }
    if(device && candidateDevice ) {
        bool hasChildSpace = hasChildSpaceForDevice(candidateDevice, device);
        bool hasLinksSpace = !deviceReachedTheMaxLinksNo(candidate, device->address64, false);
        const bool hasChunkSpace = hasOUTChunksSpaceForChild(candidateDevice, device);
        bool hasLinksSpaceOnEdge = ( checkMaxNumberOfLinksOnEdge(deviceAddress, candidate, 4 ) == ResponseStatus::SUCCESS) ; // 2 Management links + 1 AppLinks for publish + 1 for C/S from GW
        if ((candidateDevice->isAlreadyVisited())
                    && (candidateDevice->capabilities.isRouting() || candidateDevice->capabilities.isBackbone())
                    && hasChildSpace
                    && hasLinksSpace
                    && hasLinksSpaceOnEdge
                    && hasChunkSpace
                    && !device->theoAttributes.candidateIsBadRate(candidate)) {
            return true;
        }
    }

    return false;
}

bool Subnet::candidateNotValidOrIsOnPreferedPath( Device * device, Device * candidateDevice) {
    if ( !device ) {
    	return true;
    }
    if ( !candidateDevice ) {
    	return true;
    }

    if ( candidateDevice->address32 == device->parent32) {
    	return true;
    }

     if (subnetNodes[Address::getAddress16(device->parent32)]) {
    	 return candidateNotValidOrIsOnPreferedPath( subnetNodes[Address::getAddress16(device->parent32)] , candidateDevice);
     }

     return false;
}

void Subnet::getDevicesVisited(Address16List &targetsList) {
    for (Address16Set::iterator it = activeDevices.begin(); it != activeDevices.end(); ++it) {

        if (*it >= MAX_NUMBER_OF_DEVICES ) {
        	LOG_ERROR("Invalid address in active devices: " << std::hex << *it);
        	continue;
        }
        Device * device = getDevice(*it);
        if (device == NULL) {
            continue;
        }

        if (device->isAlreadyVisited()) {
            targetsList.push_back(*it);
        }
    }
}

float Subnet::getInboundAppTraffic_reentrant(Device * device, const DoubleExitEdges & edges) {
    float total = device->getInboundAppTraffic(getSubnetSettings().getSlotsPerSec());
    Address16 device16 = Address::getAddress16(device->address32);

    device->setVisited();

    // to be reviewed ... looks to be too long
    for (DoubleExitEdges::const_iterator it = edges.begin(); it != edges.end(); ++it) {
        if ((it->second.prefered == device16) ) { // compute the needed only generated by the preferred path

            Device * child = subnetNodes[it->first];
            if (child && !child->isAlreadyVisited()) {
                total += getInboundAppTraffic_reentrant(child, edges);
            }
        }
    }

    return total;
}

float Subnet::getOutboundAppTraffic_reentrant(Device * device, const DoubleExitEdges & edges) {
    float total = 0.0;

    total = device->getOutboundAppTraffic(getSubnetSettings().getSlotsPerSec());


    Address16 device16 = Address::getAddress16(device->address32);

    device->setVisited();

    // to be reviewed ... looks to be too long

    for (DoubleExitEdges::const_iterator it = edges.begin(); it != edges.end(); ++it) {
        // LOG_DEBUG("Edge first=" << std::hex << it->first << ", prefered=" << it->second.prefered << ", backup=" << it->second.backup);
        if ((it->second.prefered == device16) ) { //calculate he needed only generated by preferred edges.

            Device * child = subnetNodes[it->first];
            if (child && !child->isAlreadyVisited()) {
                total += getOutboundAppTraffic_reentrant(child, edges);
            }
        }
    }


    return total;
}

float Subnet::getInboundAppTraffic(Device * device, const DoubleExitEdges & edges) {

    markAllDevicesUnVisited();

    float total = getInboundAppTraffic_reentrant(device, edges);

    markAllDevicesUnVisited();

    return total;
}

float Subnet::getOutboundAppTraffic(Device * device) {

    float total = 0.0;

    GraphPointer graph = getGraph(DEFAULT_GRAPH_ID); // go to inbound graph
    if (graph) {
        const DoubleExitEdges & edges = graph->getGraphEdges();

        markAllDevicesUnVisited();

        total = getOutboundAppTraffic_reentrant(device, edges);

        markAllDevicesUnVisited();
    }

    return total;
}


void Subnet::unreserveSlotsOperation(Device * device, const NE::Model::Operations::IEngineOperationPointer& operation) {

    if ((operation->getType() == NE::Model::Operations::EngineOperationType::WRITE_ATTRIBUTE) //
                && (getEntityType(operation->getEntityIndex()) == EntityType::Link) && operation->getPhysicalEntity()) {
        unreserveLink(device, (PhyLink *) operation->getPhysicalEntity());
    }
}

bool Subnet::wasAddressAllocatedToOther(const Address64 & address64, const Address32 address32){
    for (AddressMapping64_32::iterator it = this->addressMapping.begin(); it != this->addressMapping.end(); ++it){
        if (it->second == address32 && !(it->first == address64) ) {
//            LOG_ERROR("CONFLICT ADDRESS ALLOCATION for " << address64.toString() << "--" << std::hex << address32 << " conflict with " << it->first.toString());
            return true;
        }
    }
    return false;
}

Address32 Subnet::getNextAddress32(const Address64 & address64, Address32 oldAddress32) {

    Address16 address16;
    if (oldAddress32 == 0) {
        address16 = address64.value[7] % MAX_NUMBER_OF_DEVICES;
    } else {
        address16 = Address::getAddress16(oldAddress32);
    }
    Address32 address32 = NE::Common::Address::createAddress32(getSubnetId(), address16);
    bool wasReservedToOther = wasAddressAllocatedToOther(address64, address32);

    if (!wasReservedToOther && 2 < address16 && address16 < MAX_NUMBER_OF_DEVICES) { // valid it, try to reallocate
        if (!getDevice(address16)) {
            return address32; // open position, return it
        }
        if (getDevice(address16)->address64 == address64) {
            return address32; // already present, return it
        }
    }

    for (Uint16 i = 2; i < MAX_NUMBER_OF_DEVICES; ++i) { // not found, search on reverse order

        if ((--lastUnmatchDeviceId) <= 2) {
            lastUnmatchDeviceId = MAX_NUMBER_OF_DEVICES - 1;
        }
        if (!getDevice(lastUnmatchDeviceId)) {
            address32 = NE::Common::Address::createAddress32(getSubnetId(), lastUnmatchDeviceId);
            wasReservedToOther = wasAddressAllocatedToOther(address64, address32);
            if (!wasReservedToOther){
                return address32; //NE::Common::Address::createAddress32(getSubnetId(), lastUnmatchDeviceId);
            }
        }
    }
    THROW_EX2(NE::Common::NEException, "No address 32 can be allocated for device "
                << address64.toString() << ", in subnet " << getSubnetId());
}

void Subnet::unreserveLinkForDevice(Address16 device, Address16 neighbor, Operations::OperationsContainer* container
            ,  NE::Model::Operations::OperationsList &operationsDependencies) {
    // generate remove LINK operations
    if(!subnetNodes[device]) {
        return;
    }

    LinkIndexedAttribute::iterator itDevLink = subnetNodes[device]->phyAttributes.linksTable.begin();
    for (; itDevLink != subnetNodes[device]->phyAttributes.linksTable.end(); ++itDevLink) {
        PhyLink* link = (PhyLink*) itDevLink->second.getValue();
        if(!link) {
            continue;
        }

        if (link->neighborType == Tdma::NeighbourTypes::NEIGHBOUR_BY_ADDRESS && neighbor == link->neighbor) {

            // destroy reservation
            unreserveLink(subnetNodes[device], link,  container);
            DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(itDevLink->first));
            if(!operationsDependencies.empty()) {
                deleteLink->setOperationDependency(operationsDependencies);
            }
            container->addOperation(deleteLink, subnetNodes[device]);
        }
    }

}


void Subnet::unreserveLink(Device * device, PhyLink * link, NE::Model::Operations::OperationsContainer * container /*= NULL*/ ) {

    LOG_INFO("unreserve link: " << *link);

//Attention: container has default value NULL
    if ((link->type & NE::Model::Tdma::LinkType::TRANSMIT) && ( link->neighborType == Tdma::NeighbourTypes::NEIGHBOUR_BY_ADDRESS )) {

        Device * peer =  getDevice(link->neighbor);

        if ( isAppLink( link->role ) ) { // APP deallocate TX
            for (Uint16 currSlot = link->schedule.offset; currSlot < AppSlotsLength; currSlot += link->schedule.interval) {
                appSlots[currSlot] &= ~(1 << link->chOffset);
                LOG_DEBUG("LINK: unreserve appSlots[:" << std::dec << (int)currSlot << "]="
                            << std:: hex << (int)appSlots[currSlot] << "; chOff:" << std::dec << (int)link->chOffset);
            }
            if( peer && container ) { // remove correspondent APP RX link
                LinkIndexedAttribute::iterator itRemDevLink = peer->phyAttributes.linksTable.begin();
                for (; itRemDevLink != peer->phyAttributes.linksTable.end(); ++itRemDevLink) {
                    PhyLink* peerLink = (PhyLink*) itRemDevLink->second.getValue();

                    if ( peerLink  &&  (peerLink->type == NE::Model::Tdma::LinkType::RECEIVE)
                          && (peerLink->role == link->role)
                          && (peerLink->chOffset  == link->chOffset) // not necessary since cannot be more links on same offset
                          && (peerLink->schedule.offset   == link->schedule.offset)
                          && (peerLink->schedule.interval == link->schedule.interval) ) {

                        DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(itRemDevLink->first));
                        container->addOperation(deleteLink, peer);
                        peer->theoAttributes.linkToContract.erase(itRemDevLink->first);
                        LOG_DEBUG("LINK: remove on peer Idx:" << std::hex << itRemDevLink->first << " Off: " << peerLink->schedule.offset << " Int:" << peerLink->schedule.interval);
                    }
                }
            }
        } else if ( peer ) {
            Uint16 setNo = link->schedule.offset % 100;
            if (link->role == NE::Model::Tdma::TdmaLinkTypes::MANAGEMENT || link->role == NE::Model::Tdma::TdmaLinkTypes::UDO_FIRMWARE) { // MAN deallocate peer management RX
                if (link->direction == NE::Model::Tdma::TdmaLinkDir::INBOUND) {
                    BCList::iterator it;
                    if (device->capabilities.isRouting()) {
                        for (it = peer->theoAttributes.mngChunckInboundRouter.begin(); it
                                    != peer->theoAttributes.mngChunckInboundRouter.end();) {
                            if (it->setNo == setNo) {
                                unreserveMngChunck(it->setNo, it->freqNo);
                                it = peer->theoAttributes.mngChunckInboundRouter.erase(it);
                                EntityIndex entityIndexLink = 0;
                                PhyLink * peerRx = peer->findLink(link->direction, NE::Model::Tdma::LinkType::RECEIVE, link->role, link->chOffset, link->schedule.offset, entityIndexLink);
                                if (peerRx && container){
                                    DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(entityIndexLink));
                                    container->addOperation(deleteLink, peer);
                                }
                                break;
                            } else {
                                ++it;
                            }
                        }
                    } else {
                        for (it = peer->theoAttributes.mngChunckInboundNonRouter.begin(); it
                                    != peer->theoAttributes.mngChunckInboundNonRouter.end(); ) {

                            if (it->setNo == setNo) {
                                it->unreserveSlot(link->schedule.offset / 100); // unreserve slot
                                if (it->isEmpty()){
                                    unreserveMngChunck(it->setNo, it->freqNo);
                                    it = peer->theoAttributes.mngChunckInboundNonRouter.erase(it);
                                }
                                EntityIndex entityIndexLink = 0;
                                PhyLink * peerRx = peer->findLink(link->direction, NE::Model::Tdma::LinkType::RECEIVE, link->role, link->chOffset, link->schedule.offset, entityIndexLink);
                                if (peerRx && container) {
                                    DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(entityIndexLink));
                                    container->addOperation(deleteLink, peer);
                                }
                                break;
                            }
                            else {
                                ++it;
                            }
                        }
                    }
                }
                else { // link->direction == NE::Model::Tdma::TdmaLinkDir::OUTBOUND

                }
            } else if (link->role == NE::Model::Tdma::TdmaLinkTypes::MANAGEMENT_ACC) {

            } else {
                return; // not MNG link -> do nothing
            }
            // remove allocated MNG RX links
            if( container && (link->direction == NE::Model::Tdma::TdmaLinkDir::OUTBOUND ) ) {
                LinkIndexedAttribute::iterator itRemDevLink = peer->phyAttributes.linksTable.begin();
                for (; itRemDevLink != peer->phyAttributes.linksTable.end(); ++itRemDevLink) {
                    PhyLink* peerLink = (PhyLink*) itRemDevLink->second.getValue();

                    if ( peerLink &&   (peerLink->type == NE::Model::Tdma::LinkType::RECEIVE)
                          && (peerLink->role == link->role )
                          && (peerLink->direction == link->direction) ) {

                        if( ( (peerLink->schedule.offset == link->schedule.offset)
                                && (peerLink->schedule.interval == link->schedule.interval) )
                            || (peer->capabilities.isRouting()
                                    && device->capabilities.isRouting()
                                    && ((peerLink->schedule.offset%100) == setNo)) ) {

                            DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(itRemDevLink->first));
                            container->addOperation(deleteLink, peer);
                        }
                    }
                }
            }

            if (link->direction == NE::Model::Tdma::TdmaLinkDir::OUTBOUND) {
                //find out chunk that link was part of
                Uint8 usedSlot = 0;
                if(peer && peer->capabilities.isNonRouting()) {
                    usedSlot = link->schedule.offset / 100;
                }
                BCList::iterator itChunk = device->theoAttributes.mngChunckOutbound.begin();
                for (; itChunk != device->theoAttributes.mngChunckOutbound.end(); ++itChunk){
                    if (itChunk->setNo == (link->schedule.offset % 100)){
                        itChunk->unreserveSlot(usedSlot);
                        break;
                    }
                }
                if ( itChunk != device->theoAttributes.mngChunckOutbound.end()) {
                    LOG_DEBUG("device=" << Address_toStream(device->address32) << ", chunk=" << *itChunk
                                << ", CHECK FOR ERASE chunk from mngChunckOutbound=" << device->theoAttributes.mngChunckOutbound);
                    if (itChunk->slotsReservationMap == 0) {
                        LOG_DEBUG("device=" << Address_toStream(device->address32) << ", chunk=" << *itChunk
                                    << ", BEFORE ERASE chunk from mngChunckOutbound=" << device->theoAttributes.mngChunckOutbound);
                        unreserveMngChunck(itChunk->setNo, itChunk->freqNo);
                        device->theoAttributes.mngChunckOutbound.erase(itChunk);
                    }
                }
            }
        }
    }
}

void Subnet::addDeviceCandidate(Address16 device16, Address16 candidate16) {
    Device* candidate = getDevice(candidate16);
    Device* dvc = getDevice(device16);
    if ((!dvc) || (!candidate)) {
        LOG_DEBUG("device or candidate is NULL");
        return;
    }


   if(dvc->theoAttributes.isCandidateOnList(candidate16)) {
       dvc->theoAttributes.deleteCandidate(candidate16);
   }

   if (!hasChildSpaceForDevice(candidate, dvc)) {
       LOG_WARN("cannot add candidate for device " << std::hex << device16 << " because candidate is full of children ,candidate: " << std::hex << candidate16);
       return;
   }

    EdgePointer edgeToCandidate = subnetEdges[device16][candidate16];
    if (!edgeToCandidate) {
        LOG_ERROR("there is no edge between " << std::hex << device16 << " and " << std::hex << candidate16);
        return;
    }
    int costCandidate = edgeToCandidate->getEvalEdgeCost(getSubnetSettings().k1factorOnEdgeCost, getNumberOfChildren(candidate16));

    if (!dvc->theoAttributes.candidates[0]) { //first candidate
        dvc->theoAttributes.candidates[0] = candidate16;
        return;
    }
    Uint8 i = 0;
    for (; i < DEFAULT_MAX_NO_CANDIDATES - 1;) {
        EdgePointer edgeToCandidateI = subnetEdges[device16][dvc->theoAttributes.candidates[i]];
        Device* candidateI = getDevice(dvc->theoAttributes.candidates[i]);
        if (!candidateI) {
            dvc->theoAttributes.deleteCandidateFromPosition(i);
            if (dvc->theoAttributes.candidates[i] == 0) {
                dvc->theoAttributes.candidates[i] = candidate16;
                break;
            }
        }
        if (!hasChildSpaceForDevice(candidateI, dvc)) {
            memmove(&dvc->theoAttributes.candidates[i],
                        &dvc->theoAttributes.candidates[i + 1], (DEFAULT_MAX_NO_CANDIDATES
                                    - 1 - (i )) * sizeof(Address16));
            dvc->theoAttributes.candidates[DEFAULT_MAX_NO_CANDIDATES - i] = 0;
        }

        if (edgeToCandidateI) {
            int costCandidateI = edgeToCandidateI->getEvalEdgeCost(getSubnetSettings().k1factorOnEdgeCost,getNumberOfChildren(dvc->theoAttributes.candidates[i]));
            if(costCandidateI <= costCandidate) {
                if (dvc->theoAttributes.candidates[i] == candidate16) {
                    sortCandidates(device16);
                    return;
                }
                if (!dvc->theoAttributes.candidates[i + 1]) { //there is not candidate set on the next position
                    dvc->theoAttributes.candidates[i + 1] = candidate16;
                    return;
                } else {

                    Device* candidateII = getDevice(dvc->theoAttributes.candidates[i + 1]);
                    EdgePointer edgeToCandidateII = subnetEdges[device16][dvc->theoAttributes.candidates[i + 1]];
                    if(!candidateII || !edgeToCandidateII) {
                        dvc->theoAttributes.candidates[i + 1] = candidate16;
                        return;
                    }
                    int costToCandidateII = edgeToCandidateII->getEvalEdgeCost(getSubnetSettings().k1factorOnEdgeCost, getNumberOfChildren(dvc->theoAttributes.candidates[i + 1]));
                    if ( costToCandidateII > costCandidate) {
                        memmove(&dvc->theoAttributes.candidates[i + 2],
                                    &dvc->theoAttributes.candidates[i + 1], (DEFAULT_MAX_NO_CANDIDATES
                                                - 1 - (i + 1)) * sizeof(Address16));
                        dvc->theoAttributes.candidates[i + 1] = candidate16;
                        return;
                    }
                }
                ++i;
            }

            else {
                memmove(&dvc->theoAttributes.candidates[i+1],
                            &dvc->theoAttributes.candidates[i], (DEFAULT_MAX_NO_CANDIDATES
                                        - 1 - (i )) * sizeof(Address16));
                dvc->theoAttributes.candidates[i ] = candidate16;
                return;
            }


        } else {
            dvc->theoAttributes.deleteCandidateFromPosition(i);
            if (dvc->theoAttributes.candidates[i] == 0) {
                dvc->theoAttributes.candidates[i] = candidate16;
                break;
            }
        }
    }
}


void Subnet::periodicStartPublish(Uint32 currentTime, NE::Model::Operations::OperationsProcessor & operationsProcessor){
    //This task is called periodicaly so:
    // - in one call only one device will have publish started
    // - in one call if device does not exist will be removed and no other device will be checked

    for (Address32Set::iterator it = notPublishingDevices.begin(); it != notPublishingDevices.end(); ++it){
        Device * device = getDevice(*it);
        if(device == NULL){
            notPublishingDevices.erase(it);
            break;
        }

        if (device->capabilities.isBackbone()){//for BBr
            if (configurePublish(device, operationsProcessor)) {
                notPublishingDevices.erase(it);
            }
            break;
        }

        if ( (currentTime - getSubnetSettings().delayConfigurePublish) >= device->joinConfirmTime){
        	if (configurePublish(device, operationsProcessor)) {
        	    notPublishingDevices.erase(it);
        	}
        	break;
        }
    }
}
void Subnet::periodicConfigureAlerts(Uint32 currentTime, NE::Model::Operations::OperationsProcessor & operationsProcessor){
    //This task is called periodicaly so:
    // - in one call only one device will have alerts configured
    // - in one call if device does not exist will be removed and no other device will be checked

    for (Address32Set::iterator it = notConfiguredForAlerts.begin(); it != notConfiguredForAlerts.end(); ++it){
        Device * device = getDevice(*it);
        if(device == NULL){
            notConfiguredForAlerts.erase(it);
            break;
        }

        if (device->capabilities.isBackbone()){//for BBr
            if (configureAlerts(device, operationsProcessor)) {
                notConfiguredForAlerts.erase(it);
            }
            break;
        }

        if ( (currentTime - getSubnetSettings().delayConfigureAlerts) >= device->joinConfirmTime){
        	if (configureAlerts(device, operationsProcessor)) {
        	    notConfiguredForAlerts.erase(it);
        	}
        	break;
        }
    }
}

void Subnet::evaluateIgnoredDevices(Uint32 currentTime, NE::Model::Operations::OperationsProcessor & operationsProcessor) {
    for (Address16Set::iterator  itDev = activeDevices.begin(); itDev != activeDevices.end(); ++itDev) {
        if(subnetNodes[*itDev] && !subnetNodes[*itDev]->theoAttributes.devicesToIgnore.empty()) {
            BadRateDevicesList::iterator it = subnetNodes[*itDev]->theoAttributes.devicesToIgnore.begin();
            if(it->timeIgnoreStared <= (currentTime - getSubnetSettings().timeToIgnoreBadRateDevice* 60)) {
                LOG_INFO("For device " << Address_toStream(subnetNodes[*itDev]->address32) << " erase candidate: " << Address_toStream(Address::createAddress32(subnetId, it->device16 )) << " from badRateList");
                subnetNodes[*itDev]->theoAttributes.devicesToIgnore.erase(it++);
            }
        }
    }
}

bool Subnet::configurePublish(Device * device, NE::Model::Operations::OperationsProcessor & operationsProcessor){

    LOG_INFO("Configure publish diag " << Address_toStream(device->address32));

    Device * parentDevice = getDevice(device->parent32);
    // RETURN_ON_NULL_MSG(parentDevice, "Parent is NULL " << Address_toStream(device->parent32) << " for dev " << Address_toStream(device->address32));
    if(parentDevice == NULL) {
        LOG_ERROR("Parent is NULL " << std::hex << device->parent32 << " for dev " << std::hex << device->address32);
        return true;
    }

    if (!device->phyAttributes.diagMetadata.getValue()) {
        LOG_ERROR("DiagMetadata is NULL(not read yet) for device " << Address_toStream(device->address32));
        return false;
    }

//    HandlerResponse handlerJoinedConfigured = boost::bind(&NetworkEngine::handlerJoinedConfigured, this, _1, _2, _3);

    char reason[64];
    sprintf(reason, "START publish, device=%x", device->address32);
    OperationsContainerPointer operationsContainerStartPublish(new OperationsContainer(reason));

    // 1. start Publish

    // 1.1 CommunicationEndpoint
    if (!device->phyAttributes.hrcoCommEndpoint.getValue()) {
        EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::HRCO_CommEndpoint, 0);
        PhyCommunicationAssociationEndpoint* phyCommEndpoint = new PhyCommunicationAssociationEndpoint();
        IEngineOperationPointer operation(new WriteAttributeOperation(phyCommEndpoint, entityIndex));

        operationsContainerStartPublish->addOperation(operation, device);
    }

    // 1.2
    PhyEntityIndexList *phyEntityIndexList = new PhyEntityIndexList();

    // 1.2.3.1 publish neighbor diag for parent(first)
    if (!device->capabilities.isBackbone()) {

        // config backup Neighbor of device (set diagLevel to 1)
        for (NeighborIndexedAttribute::const_iterator it = device->phyAttributes.neighborsTable.begin(); it != device->phyAttributes.neighborsTable.end(); ++it) {

            if (it->second.isOnPending()) {
                delete phyEntityIndexList;
                LOG_INFO("Delay configure publish for device " << Address_toStream(device->address32) << " because the neighbor " << std::hex << it->first << " is pending.");
                return false;
            }

            PhyNeighbor *neighbor = it->second.getValue();
            if (!neighbor) {
                continue;
            }

            Uint8 newDiagLevel = 0;
            if (neighbor->clockSource == 1 || neighbor->clockSource == 2) { // backup + parent
                newDiagLevel = 1;
                phyEntityIndexList->value.push_back(createEntityIndex(device->address32, EntityType::NeighborDiag, it->first));
            } else { // other
                newDiagLevel = 0;
            }

            if (neighbor->diagLevel == newDiagLevel) {
                // no changes
                continue;
            }

            Device * neighborDevice = getDevice(getIndex(it->first));
            if (!neighborDevice) {
                LOG_WARN("Config device " << Address_toStream(device->address32) << " to publish NeighborDiag for NULL neighbor="
                            << getIndex(it->first));
                continue;
            }
            LOG_INFO("Config device " << Address_toStream(device->address32) << " to publish NeighborDiag for "
                        << Address_toStream(neighborDevice->address32));

            // config device: backup Neighbour
            PhyNeighbor *neighborCopy = new PhyNeighbor(*neighbor);
            neighborCopy->diagLevel = newDiagLevel; // getDiagLevel(device->address32, backupDevice->address32);
            EntityIndex eiNeighbor = createEntityIndex(device->address32, EntityType::Neighbour, it->first);
            IEngineOperationPointer operationNeighborCopy(new WriteAttributeOperation(neighborCopy, eiNeighbor));
            operationsContainerStartPublish->addOperation(operationNeighborCopy, device);
        }
    }

    // 1.2.2 ChannelDiag
    EntityIndex channelDiag = createEntityIndex(device->address32, EntityType::ChannelDiag, 0);
    phyEntityIndexList->value.push_back(channelDiag);

    // 1.2.1 Candidates
    EntityIndex candidate = createEntityIndex(device->address32, EntityType::Candidate, 0);
    phyEntityIndexList->value.push_back(candidate);

    EntityIndex entityIndexHRCO = createEntityIndex(device->address32, EntityType::HRCO_Publish, 0);
    IEngineOperationPointer operationPublish(new WriteAttributeOperation(phyEntityIndexList, entityIndexHRCO));

    operationsContainerStartPublish->addOperation(operationPublish, device);

    // reset nrOfwrongPublishReceived
    device->nrOfwrongPublishReceived = 0;

    //after this call, no modification of the container should be made
    operationsProcessor.addOperationsContainer(operationsContainerStartPublish);

    return true;
}
bool Subnet::configureAlerts(Device * device, NE::Model::Operations::OperationsProcessor & operationsProcessor){

    LOG_INFO("Configure alerts " << Address_toStream(device->address32));

    Device * parentDevice = getDevice(device->parent32);
    // RETURN_ON_NULL_MSG(parentDevice, "Parent is NULL " << Address_toStream(device->parent32) << " for dev " << Address_toStream(device->address32));
    if(parentDevice == NULL) {
        LOG_ERROR("Parent is NULL " << std::hex << device->parent32 << " for dev " << std::hex << device->address32);
        return true;
    }

    char reason[64];
    sprintf(reason, "Config alerts, device=%x", device->address32);
    OperationsContainerPointer containerConfigAlerts(new OperationsContainer(reason));

    //configure alerts after the configuration of publication

    {
        // 1. WRITE DLMO.DiscoveryAlert
        EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::DLMO_DiscoveryAlert, 0);
        PhyUint8 * attributeValue = new PhyUint8();
        attributeValue->value = 0;//Disable sending Neighbor discovery through alerts.. this info comes via Publish Candidates
        Operations::IEngineOperationPointer operation(new Operations::WriteAttributeOperation(attributeValue, entityIndex));
        containerConfigAlerts->addOperation(operation, device);
    }

    //no need to write configurations at rejoin
    if (device->phyAttributes.armoCommEndpoint.getValue() == NULL) {

        EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::ARMO_CommEndpoint, 0);
        PhyAlertCommunicationEndpoint* phyCommEndpoint = new PhyAlertCommunicationEndpoint();

        IEngineOperationPointer operationAlerts(new WriteAttributeOperation(phyCommEndpoint, entityIndex));
        containerConfigAlerts->addOperation(operationAlerts, device);
    }


    //after this call, no modification of the container should be made
    operationsProcessor.addOperationsContainer(containerConfigAlerts);

    return true;
}

Uint8 Subnet::getDiagLevel(Address32 deviceAddress, Address32 neighborAddress) {
    //TODO this should be dinamicaly obtained from PhyNeighbor, in case is diffrent from neighbor to neighbor
    return 0x1;
}



bool Subnet::existsDirectPathBetween(Address16 source16, Address16 destination16, PhyRoute *route)  {
    if(!backbone) {
        return false;
    }
    if((!subnetNodes[source16]) || (!subnetNodes[destination16])) {
        return false;
    }
    Uint16 graphSource = 0;
    Uint16 graphDestination = 0;

    if(subnetNodes[source16]->hasNeighbor(destination16)) {
        route->route.push_back(destination16);
        return true;
    }

    RouteIndexedAttribute::iterator itBrRoute = backbone->phyAttributes.routesTable.begin();
    for (; itBrRoute != backbone->phyAttributes.routesTable.end(); ++itBrRoute) {
        PhyRoute* phyRoute = (PhyRoute*) itBrRoute->second.getValue();
        if(!phyRoute) {
            continue;
        }
        LOG_DEBUG("route=" << *route);
        if (phyRoute->alternative == 2 && phyRoute->selector == source16) {
            if (phyRoute->route.size() == 1 && isRouteGraphElement(phyRoute->route[0])) {//mark the graph for verification
                graphSource = getRouteElement(phyRoute->route[0]);
            }
        }
        if (phyRoute->alternative == 2 && phyRoute->selector == destination16) {
            if (phyRoute->route.size() == 1 && isRouteGraphElement(phyRoute->route[0])) {//mark the graph for verification
                graphDestination = getRouteElement(phyRoute->route[0]);
            }
        }
    }

    bool toRevert = false;
    Address16List routeNodes;
    if(graphSource != 0) {
        GraphsMap::iterator it = graphs.find(graphSource);
        if(it != graphs.end()) {
            if(it->second->graphPasesThroughNode(destination16)) {
                toRevert = true;
                it->second->getSourceRouteToNode(destination16, routeNodes, toRevert);
                if(!routeNodes.empty()) {
                    for(Address16List::iterator iter = routeNodes.begin(); iter != routeNodes.end(); ++iter) {
                        route->route.push_back(*iter);
                    }
                    return true;
                }
            }
        }
    }


    if(graphDestination != 0) {
        GraphsMap::iterator it = graphs.find(graphDestination);
        if(it != graphs.end()) {
            if(it->second->graphPasesThroughNode(source16)) {
                it->second->getSourceRouteToNode(source16, routeNodes, toRevert);
                if(!routeNodes.empty()) {
                    for(Address16List::iterator iter = routeNodes.begin(); iter != routeNodes.end(); ++iter) {
                        route->route.push_back(*iter);
                    }
                    return true;
                }
            }
        }
    }

    return false;
}

void Subnet::setUpdateAdvPeriod(bool value) {
    updateAdvPeriod = value;
}

bool Subnet::getUpdateAdvPeriod() {
    return updateAdvPeriod;
}

bool Subnet::neighborsAreValid(Address16 src,  DoubleExitDestinations &dst) {
    if(!subnetNodes[src]) {
        return false;
    }

    if((!subnetNodes[dst.prefered]) || (!dst.prefered) || !deviceHasNeighbor(src, dst.prefered) || !subnetNodes[dst.prefered]->isAlreadyVisited()) {
        return false;
    }

    if((dst.backup) && ((!subnetNodes[dst.backup]) || !deviceHasNeighbor(src, dst.backup) || !subnetNodes[dst.backup]->isAlreadyVisited())) {
        return false;
    }

    if ( dst.backup && subnetNodes[src]->theoAttributes.candidateIsBadRate(dst.backup)) {
        return false;
    }

    return true;
}

bool Subnet::nodeHasChangedDestinations(Address16 src,  DoubleExitDestinations &dst, Uint16 graphId) {
    if(!subnetNodes[src]) {
        return false;
    }

    Uint8 neighNo = 0;
    if(dst.prefered) {
        ++neighNo;
    }
    if(dst.backup) {
        ++neighNo;
    }


    GraphIndexedAttribute::iterator itPhyGraph = subnetNodes[src]->phyAttributes.graphsTable.begin();
    for (; itPhyGraph != subnetNodes[src]->phyAttributes.graphsTable.end(); ++itPhyGraph) {
        PhyGraph * phyGraph = itPhyGraph->second.getValue();
        if(!phyGraph) {
            continue;
        }
        if(phyGraph->index == graphId) {
            if(phyGraph->neighbors.size() == neighNo) {

                if(dst.prefered)  {
                    if ((std::find(phyGraph->neighbors.begin(), phyGraph->neighbors.end(), dst.prefered) == phyGraph->neighbors.end())) {
                        return true;
                    }

                    if((dst.backup) ) {
                        if (std::find(phyGraph->neighbors.begin(), phyGraph->neighbors.end(), dst.backup) != phyGraph->neighbors.end()) {
                            return false;
                        }
                        else {
                            return true;
                        }
                    }
                    return false;
                }
                return true;
            } else {
                return true;
            }
        }
    }
    return true;
}

void Subnet::markServedContractsDirty(Device * usedDevice) {
	EntityIndexList servedContractsList;
	usedDevice->theoAttributes.getServedContracts(servedContractsList);

	ContractToLinksMap::iterator it = usedDevice->theoAttributes.contractLinks.begin();
	for (; it != usedDevice->theoAttributes.contractLinks.end(); ++it){
	    moveToDirtyLinks(it->second);
	}

	for (EntityIndexList::iterator itEntity = servedContractsList.begin(); itEntity != servedContractsList.end(); ++itEntity){
	    addContractToBeEvaluated(*itEntity);
	}
}

void Subnet::moveToDirtyLinks(LinksList& contractLinks){
    dirtyLinks.insert(dirtyLinks.end(), contractLinks.begin(), contractLinks.end());
    contractLinks.clear();
}

void Subnet::removeFromDirtyLinks(Device * usedDevice) {
    if(!usedDevice) {
        return;
    }

    for(LinksList::iterator it = dirtyLinks.begin(); it != dirtyLinks.end(); ) {
        if(usedDevice->address32 == getDeviceAddress(*it)) {
            it = dirtyLinks.erase(it);
        }
        else {
            ++it;
        }
    }

}

void Subnet::addContractToBeEvaluated(EntityIndex contractEntityIndex){
    EntityIndexList::iterator itContract = std::find(contractsToBeEvaluated.begin(), contractsToBeEvaluated.end(), contractEntityIndex);
    if(itContract == contractsToBeEvaluated.end()){
        this->contractsToBeEvaluated.push_back(contractEntityIndex);
    }
}

float Subnet::getNumberOfUsedMngAndAdvBC() {
    float usedMng = 0.0;

    NE::Common::SubnetSettings settings = getSubnetSettings();
    for (Uint8 i = settings.join_reserved_set + 2; i <= MAX_NUMBER_OF_BANDWIDTH_CHUNCKS - 1; i += 2) {
        for (Uint8 j = 0; j < getSubnetSettings().numberOfFrequencies; ++j) {
            if(mngChunksTable[i][j].owner) {
                usedMng += 1.0;
                break;
            }
        }
    }
    for (Uint8 i = 0; i < MAX_NR_OF_ADV_BANDWIDTH_CHUNCKS; ++i) {
        for(Uint8 j = 0; j <  MAX_NR_OF_ADV_PERIODS; ++j) {
            if (advertiseChuncksTable[i][j]) {
                usedMng += 1.0;
                break;
            }
        }
    }
    return usedMng;

}

float Subnet::getNumberOfSlotsAPPperSecond(){
    int slotsOcupied = 0;
    for (int i = 0; i < AppSlotsLength; ++i){
        if (appSlots[i] != 0){
            slotsOcupied++;
        }
    }

    return ((float) slotsOcupied / (AppSlotsLength / getSubnetSettings().getSlotsPerSec() ));//slots per second
}

void Subnet::getNumberOfSlotsPerSecond(float& allocatedSlotsAdvMngPerSecond, float& allocatedSlotsAppPerSecond) {
	int allocatedSlotsApp = 0;
	int allocatedSlotsAdvMng = 0;

    for(Address16Set::const_iterator itDevice = activeDevices.begin(); itDevice != activeDevices.end(); ++itDevice) {
    	if(!existsConfirmedDevice(*itDevice)) {
    		continue;
    	}

		LinkIndexedAttribute::iterator itDevLink = subnetNodes[*itDevice]->phyAttributes.linksTable.begin();
		for (; itDevLink != subnetNodes[*itDevice]->phyAttributes.linksTable.end(); ++itDevLink) {
			PhyLink *link = (PhyLink*) itDevLink->second.getValue();
			if(!link) {
				continue;
			}

			if ( link->role == NE::Model::Tdma::TdmaLinkTypes::MANAGEMENT
					|| link->role == NE::Model::Tdma::TdmaLinkTypes::UDO_FIRMWARE
					|| link->role == NE::Model::Tdma::TdmaLinkTypes::APPLICATION_BACKUP) {

					if ( link->type == NE::Model::Tdma::LinkType::TRANSMIT && ( link->schedule.interval != 0 )) {
						allocatedSlotsAdvMng += (((AppSlotsLength - link->schedule.offset) / link->schedule.interval ) + 1 ) ;
					}
					continue;
			}


			//only routers have adv links
			if ( subnetNodes[*itDevice]->capabilities.isRouting() || subnetNodes[*itDevice]->capabilities.isBackbone() ) {
				if (( subnetNodes[*itDevice]->hasRoleActivated == RoleActivationStatus::ACTIVE_SLOW ) && (link->schedType = NE::Model::Tdma::ScheduleType::OFFSET)) {
					allocatedSlotsAdvMng += 1 ;
					continue;
				}

				if ( link->type == NE::Model::Tdma::LinkType::ADVERTISEMENT ) {
						if ( link->role == NE::Model::Tdma::TdmaLinkTypes::JOIN ) {
							if ( link->schedType == NE::Model::Tdma::ScheduleType::OFFSET_AND_INTERVAL  && (link->schedule.interval != 0 ) ) {
								allocatedSlotsAdvMng += (((AppSlotsLength - link->schedule.offset) / link->schedule.interval ) + 1 ) ;
							}
						}
						continue;
				}
			}


			if ( link->role == NE::Model::Tdma::TdmaLinkTypes::APPLICATION ) {
					if ( link->type == NE::Model::Tdma::LinkType::TRANSMIT && (link->schedule.interval != 0) ) {
						allocatedSlotsApp += (((AppSlotsLength - link->schedule.offset) / link->schedule.interval ) + 1 ) ;
					}
					continue;
			}
		}
	}

    allocatedSlotsAdvMngPerSecond = ((float) allocatedSlotsAdvMng / (AppSlotsLength / getSubnetSettings().getSlotsPerSec() ));//slots per second
    allocatedSlotsAppPerSecond = ((float) allocatedSlotsApp / (AppSlotsLength / getSubnetSettings().getSlotsPerSec() ));//slots per second
}

void Subnet::getNumberOfSlotsPerSecondOnBBR(float& allocatedSlotsAdvMngPerSecond, float& allocatedSlotsAppPerSecond) {
	int allocatedSlotsApp = 0;
	int allocatedSlotsAdvMng = 0;
	int neiDiscoveryLinksCount = 0;

	if (!backbone) {
		LOG_WARN("Backbone not present");
		return;
	}

	LinkIndexedAttribute::iterator itDevLink = backbone->phyAttributes.linksTable.begin();
	for (; itDevLink != backbone->phyAttributes.linksTable.end(); ++itDevLink) {
		PhyLink *link = (PhyLink*) itDevLink->second.getValue();
		if(!link) {
			continue;
		}

		//1. transmit links - M, a, J
		if (link->type == NE::Model::Tdma::LinkType::TRANSMIT && link->schedule.interval != 0 ) {
			if (link->role == NE::Model::Tdma::TdmaLinkTypes::MANAGEMENT
					|| link->role == NE::Model::Tdma::TdmaLinkTypes::UDO_FIRMWARE
					|| link->role == NE::Model::Tdma::TdmaLinkTypes::APPLICATION_BACKUP
					|| link->role == NE::Model::Tdma::TdmaLinkTypes::JOIN) {

				allocatedSlotsAdvMng += (((AppSlotsLength - link->schedule.offset) / link->schedule.interval) + 1) ;
			}
			continue;
		}

		//2. 'J' ADV links
		if (link->type == NE::Model::Tdma::LinkType::ADVERTISEMENT
				&& link->role == NE::Model::Tdma::TdmaLinkTypes::JOIN
				&& link->schedule.interval != 0) {

			allocatedSlotsAdvMng += (((AppSlotsLength - link->schedule.offset) / link->schedule.interval) + 1) ;

			continue;
		}

		if (link->role == NE::Model::Tdma::TdmaLinkTypes::NEIGHBOR_DISCOVERY) {
			neiDiscoveryLinksCount++;
			continue;
		}
	}

	//3. 'N' links - each link occurs only one time in sf -> one allocated slot/link
	float slotsNeiDiscoveryPerSecond = 0.0;
	int sfNeiDiscoveryLength = 0;

	EntityIndex indexSfNeiDiscovery = createEntityIndex(backbone->address32, EntityType::Superframe, DEFAULT_NEIGH_DISCOVERY_SUPERFRAME_ID);
    SuperframeIndexedAttribute::iterator itSf = backbone->phyAttributes.superframesTable.find(indexSfNeiDiscovery);

    if (itSf != backbone->phyAttributes.superframesTable.end() && itSf->second.getValue()) {
    	sfNeiDiscoveryLength = itSf->second.getValue()->sfPeriod;
    }

    if (sfNeiDiscoveryLength) {
    	slotsNeiDiscoveryPerSecond = (float) neiDiscoveryLinksCount / sfNeiDiscoveryLength;
    }

	//    LOG_INFO("[SORIN] sfNeiDiscoveryLength=" << sfNeiDiscoveryLength
	//    		<< " neiDiscoveryLinksCount=" << neiDiscoveryLinksCount
	//    		<< " slotsNeiDiscoveryPerSecond=" << slotsNeiDiscoveryPerSecond);

	//4. transmit links with "neighb=bbr" from BR's neighbors
    for(Address16Set::const_iterator itDevice = activeDevices.begin(); itDevice != activeDevices.end(); ++itDevice) {
    	if (!existsConfirmedDevice(*itDevice)) {
    		continue;
    	}

    	Device *device = subnetNodes[*itDevice];

    	if (device->parent32 != backbone->address32) {
    		continue;
    	}

		LinkIndexedAttribute::iterator itDevLink = device->phyAttributes.linksTable.begin();
		for (; itDevLink != device->phyAttributes.linksTable.end(); ++itDevLink) {
			PhyLink *link = (PhyLink*) itDevLink->second.getValue();
			if(!link) {
				continue;
			}

			if (link->type != NE::Model::Tdma::LinkType::TRANSMIT) {
				continue;
			}

			if(link->schedule.interval == 0) {
				continue;
			}

			Uint16 bbrAddress16 = Address::getAddress16(backbone->address32);
			if (bbrAddress16 != link->neighbor) {
				continue;
			}

			if (link->role == NE::Model::Tdma::TdmaLinkTypes::APPLICATION) {
				allocatedSlotsApp += (((AppSlotsLength - link->schedule.offset) / link->schedule.interval ) + 1 ) ;

				continue;

			} else {
				allocatedSlotsAdvMng += (((AppSlotsLength - link->schedule.offset) / link->schedule.interval) + 1) ;
			}
		}
    }

	allocatedSlotsAdvMngPerSecond = ((float) allocatedSlotsAdvMng / (AppSlotsLength / getSubnetSettings().getSlotsPerSec() ));//slots per second
	allocatedSlotsAdvMngPerSecond += slotsNeiDiscoveryPerSecond;
	allocatedSlotsAppPerSecond = ((float) allocatedSlotsApp / (AppSlotsLength / getSubnetSettings().getSlotsPerSec() ));//slots per second
}

void Subnet::checkExpiringBlacklists(Uint32 currentTime, NE::Model::Operations::OperationsProcessor & operationsProcessor)

{
    bool modified = false;
    for (std::vector<BlacklistChannel>::iterator it = channelsBlacklist.getBlacklistedChannels().begin();
            it != channelsBlacklist.getBlacklistedChannels().end(); ++it) {
        if ((currentTime - it->BlacklistedTime    >=  settingsLogic->channelBlacklistingKeepPeriod) && (it->BlacklistedTime > 0)) {
            LOG_DEBUG("Resetting blacklist for channel " << it->ChannelNo);
            modified = true;
            it->BlacklistedTime = 0;
        }
    }

    //Remove all channels that are "equal" to 0 in the meaning of the redefiend operator==(int)

    channelsBlacklist.removeUnselectedChannels();


    if (modified) {
        //TODO send new configuration
        generateChannelBlackListOperations(operationsProcessor);
    }
}


void Subnet::processChannelDiag(const Model::Tdma::ChannelDiagnostics& channelDiags, const Address32& source,
            NE::Model::Operations::OperationsProcessor & operationsProcessor) {
    bool modified = false;

    // only collect from backbones

    if (subnetNodes[Address::getAddress16(source)] && subnetNodes[Address::getAddress16(source)]->capabilities.isBackbone()) {
        Uint32 currentTime = NE::Common::ClockSource::getCurrentTime();
        for (std::map<Uint8, Model::Tdma::ChannelDiagnostics::Diag>::const_iterator it = channelDiags.Channels.begin();
            it != channelDiags.Channels.end(); ++it) {
            Int8 ccaBackoff = it->second.ccaBackoff;

            // negative values mean less than 5 attempts, so we will not blacklist channels like that
//              if (ccaBackoff < 0)
//                  ccaBackoff *= -1;

            // zero value means no transmissions done, so we skip that case (1 means 0% failure, 101 means 100% failure)
            ccaBackoff -= 1;

//                            settingsLogic->channelBlacklistingKeepPeriod);
            if (ccaBackoff > settingsLogic->channelBlacklistingThresholdPercent && ! (getSubnetSettings().advChannelsMask & (1 << it->first))) {
                if (!channelsBlacklist.IsBlacklistedChannel(it->first)) {
                    LOG_DEBUG("Blacklisted channel :" << (int)it->first);
                    BlacklistChannel blChannel;
                    blChannel.ChannelNo = it->first;
                    blChannel.BlacklistedTime = currentTime;

                    channelsBlacklist.insertChannel(blChannel);

                    modified = true;
                }
                else {
                    channelsBlacklist.modifyTimeToReset(it->first, currentTime);
                }
            }
        }
    }

    if (modified) {
        generateChannelBlackListOperations(operationsProcessor);
    }
}


void Subnet::generateChannelBlackListOperations(NE::Model::Operations::OperationsProcessor & operationsProcessor) {
    char reason[64];
    sprintf(reason, "Set Blacklists");
    OperationsContainerPointer operationsContainer(new OperationsContainer(reason));

    Uint16 channelMapMask = DEFAULT_CHANNEL_MAP;
    channelsBlacklist.createChannelMap(channelMapMask);
    for(Address16Set::const_iterator itDevice = activeDevices.begin(); itDevice != activeDevices.end(); ++itDevice) {
        if(existsConfirmedDevice(*itDevice) &&
                    !isManager(*itDevice) &&
                    !isGateway(*itDevice)) {
            if(!subnetNodes[*itDevice]) {
                continue;
            }

            EntityIndex entityIndex = createEntityIndex(subnetNodes[*itDevice]->address32, EntityType::DLMO_IdleChannels, 0);
            PhyUint16 * idleChannels = new PhyUint16();
            idleChannels->value = ~channelMapMask;
            Operations::IEngineOperationPointer opWriteIdleChannels(new Operations::WriteAttributeOperation(idleChannels, entityIndex));
            operationsContainer->addOperation(opWriteIdleChannels, subnetNodes[*itDevice]);
        }

    }
    operationsProcessor.addOperationsContainer(operationsContainer);

}


void Subnet::generateBackupPingOperation( OperationsContainer * operationsContainer, Address16 device) {
    if ( ! subnetNodes[device] ) {
        return;
    }

    EntityIndex entityIndex = createEntityIndex(subnetNodes[device]->address32, EntityType::PingInterval, 0);
    PhyUint16 * pingInterval = new PhyUint16();
    pingInterval->value = getSubnetSettings().pingInterval;
    Operations::IEngineOperationPointer opWritePingInterval(new Operations::WriteAttributeOperation(pingInterval, entityIndex));
    operationsContainer->addOperation(opWritePingInterval, subnetNodes[device]);

    LOG_INFO("Write PingInterval " << *opWritePingInterval);
}

bool Subnet::checkMaxNrOfChildren(const Address16 parentDeviceAddress16, const Address64 & childDeviceAddress64) {

    Device * parentDevice = subnetNodes[parentDeviceAddress16];
    if(!parentDevice) {
        return false;
    }

    Uint8 nrOfChildren = 0;
    for (Address64Set::iterator it = parentDevice->theoAttributes.children.begin(); it != parentDevice->theoAttributes.children.end(); ++it) {
        Device * child = subnetNodes[getAddress16(getAddress32(*it))];
        if (!child ) {
            continue;
        }
        if(!(getAddress16(child->parent32) == parentDeviceAddress16)
                    && !isBackupForDevice(getAddress16(child->parent32) , parentDeviceAddress16)) {
            continue;
        }

        ++nrOfChildren;
    }

    int routerChildsLimit = (subnetNodes[parentDeviceAddress16]->capabilities.isBackbone() ? getSubnetSettings().nrRoutersPerBBR
                : getSubnetSettings().nrRoutersPerRouter);
    int nonRouterChildsLimit = (subnetNodes[parentDeviceAddress16]->capabilities.isBackbone() ? getSubnetSettings().nrNonRoutersPerBBR
                : getSubnetSettings().nrNonRoutersPerRouter);
    int childrenLimit = routerChildsLimit + nonRouterChildsLimit;

    if (nrOfChildren >= childrenLimit) {
        LOG_WARN("Reached max number of children for parent " << Address_toStream(parentDevice->address32)
                    << ". Join refused for " << childDeviceAddress64.toString()
                    << ". Actual number of children is " << (int) nrOfChildren << "; limit is " << childrenLimit << ")");
        return false;
    }

    return true;
}

bool Subnet::addTheoAttributesChild(const Address16 parentDeviceAddress16, const Address16 deviceAddress16) {
    bool add = false;

    Device * parentDevice = subnetNodes[parentDeviceAddress16];
    if(!parentDevice) {
        return add;
    }

    // logging
    std::ostringstream childrenStream;
    for (Address64Set::iterator itChild = parentDevice->theoAttributes.children.begin();
            itChild != parentDevice->theoAttributes.children.end(); ++itChild) {
        childrenStream << itChild->toString() << ", ";
    }
    LOG_INFO("BEFORE : Device " << Address_toStream(parentDevice->address32) << ", ADD to theoAttributes::children: " << childrenStream.str());

    Device * device = subnetNodes[deviceAddress16];
    if(!device) {
        return add;
    }

    if (parentDevice->theoAttributes.children.find(device->address64) == parentDevice->theoAttributes.children.end()) {
        LOG_INFO("Device " << Address_toStream(parentDevice->address32) << ", add theoAttributes::children " << Address_toStream(device->address32));
    } else {
        LOG_INFO("Device " << Address_toStream(parentDevice->address32) << ", already exist theoAttributes::children " << Address_toStream(device->address32));
    }

    parentDevice->theoAttributes.children.insert(device->address64);
    add = true;

    return add;
}

bool Subnet::deleteTheoAttributesChild(const Address16 parentDeviceAddress16, const Address16 & deviceAddress16) {

    Device * device = subnetNodes[deviceAddress16];
    if(!device) {
        return false;
    }

    return deleteTheoAttributesChild(parentDeviceAddress16, device->address64);
}

bool Subnet::deleteTheoAttributesChild(const Address16 parentDeviceAddress16, const Address64 & deviceAddress64) {
    bool del = false;

    Device * parentDevice = subnetNodes[parentDeviceAddress16];
    if(!parentDevice) {
        return del;
    }

    // logging
    std::ostringstream childrenStream;
    for (Address64Set::iterator itChild = parentDevice->theoAttributes.children.begin();
            itChild != parentDevice->theoAttributes.children.end(); ++itChild) {
        childrenStream << itChild->toString() << ", ";
    }
    LOG_INFO("BEFORE : Device " << Address_toStream(parentDevice->address32) << ", DEL from theoAttributes::children:  " << childrenStream.str());


    if (parentDevice->theoAttributes.children.find(deviceAddress64) == parentDevice->theoAttributes.children.end()) {
        LOG_INFO("Device " << Address_toStream(parentDevice->address32) << ", theoAttributes::children: delete " << deviceAddress64.toString() << " not found");
    } else {
        LOG_INFO("Device " << Address_toStream(parentDevice->address32) << ", theoAttributes::children: delete " << deviceAddress64.toString());
        del = true;
        parentDevice->theoAttributes.children.erase(deviceAddress64);
    }

    return del;
}


bool Subnet::checkMaxNrOfChildren(Address16 parentDeviceAddress16, bool childIsRouting, bool printWarnMessage /*= true*/) {

    Device * parentDevice = subnetNodes[parentDeviceAddress16];
    if(!parentDevice) {
        return false;
    }

    Uint8 routerChildrenLimit = (subnetNodes[parentDeviceAddress16]->capabilities.isBackbone() ? settingsLogic->getSubnetSettings(subnetId).nrRoutersPerBBR
                : settingsLogic->getSubnetSettings(subnetId).nrRoutersPerRouter);
    Uint8 nonRouterChildrenLimit = (subnetNodes[parentDeviceAddress16]->capabilities.isBackbone() ? settingsLogic->getSubnetSettings(subnetId).nrNonRoutersPerBBR
                : settingsLogic->getSubnetSettings(subnetId).nrNonRoutersPerRouter);

    Uint8 nrOfRouters = 0;
    Uint8 nrOfNonRouters = 0;

    for (Address64Set::iterator it = parentDevice->theoAttributes.children.begin(); it != parentDevice->theoAttributes.children.end(); ++it) {
        Device * childDevice = subnetNodes[getAddress16(getAddress32(*it))];
        if (!childDevice ) {
            continue;
        }
        if(!(getAddress16(childDevice->parent32) == parentDeviceAddress16)
                    && !isBackupForDevice(getAddress16(childDevice->parent32) , parentDeviceAddress16)) {
            continue;
        }

        if (childDevice->capabilities.isRouting()) {
            ++nrOfRouters;
        } else {
            ++nrOfNonRouters;
        }
    }

    if((childIsRouting && (nrOfRouters >= routerChildrenLimit)) )
    {
        if(printWarnMessage) {
            LOG_WARN("Routers limit reached: " << Address_toStream(subnetNodes[parentDeviceAddress16]->address32) << " ("<< (int)nrOfRouters << ">=" << (int)routerChildrenLimit << ")");
        }
        return false;
    }

    if((!childIsRouting  && (nrOfNonRouters >= nonRouterChildrenLimit))) {
        if(printWarnMessage) {
            LOG_WARN("IO limit reached: " << Address_toStream(subnetNodes[parentDeviceAddress16]->address32) << " ("<< (int)nrOfNonRouters << ">=" << (int)nonRouterChildrenLimit << ")");
        }
        return false;
    }

    return true;
}

ResponseStatus::ResponseStatusEnum Subnet::checkMaxNumberOfLinksOnEdge(Address16 src, Address16 dst, Uint8 neededLinks, NE::Model::Operations::OperationsContainer *  pendingContainer ) {

    Device* srcDevice = subnetNodes[src];
    Device* dstDevice = subnetNodes[dst];

    if(!srcDevice || !dstDevice) {
        return ResponseStatus::FAIL;
    }

    Uint16 srcReservedLinks = srcDevice->phyAttributes.linkMetadata.currentValue ?
               srcDevice->phyAttributes.linkMetadata.currentValue->used + getSubnetSettings().linksMetadataThreshold : 0; // current device
    Uint16 srcTotalLinks= srcDevice->phyAttributes.linkMetadata.currentValue ? srcDevice->phyAttributes.linkMetadata.currentValue->total : 0;

    Uint16 dstReservedLinks = dstDevice->phyAttributes.linkMetadata.currentValue ?
                dstDevice->phyAttributes.linkMetadata.currentValue->used +  getSubnetSettings().linksMetadataThreshold : 0; // current device
    Uint16 dstTotalLinks= dstDevice->phyAttributes.linkMetadata.currentValue ? dstDevice->phyAttributes.linkMetadata.currentValue->total : 0;


    if ( pendingContainer ) {
        for (OperationsList::iterator it = pendingContainer->getUnsentOperations().begin(); it != pendingContainer->getUnsentOperations().end(); ++it) {
           if ((srcDevice->address32 == (*it)->getOwner()) && (EntityType::Link == getEntityType((*it)->getEntityIndex()))) {
               ++srcReservedLinks;
           }
           if ((dstDevice->address32 == (*it)->getOwner()) && (EntityType::Link == getEntityType((*it)->getEntityIndex()))) {
               ++dstReservedLinks;
           }
        }
    }

    if(srcReservedLinks + neededLinks > srcTotalLinks) {
       LOG_WARN("Subnet::checkMaxNumberOfLinksOnEdge" <<  Address_toStream(srcDevice->address32)
                      << " - Links : reserved=" << std::dec << (int)srcReservedLinks << ", total=" << (int)srcTotalLinks);
       return ResponseStatus::REFUSED_INSUFICIENT_RESOURCES;
    }

    if(dstReservedLinks + neededLinks > dstTotalLinks) {
       LOG_WARN("Subnet::checkMaxNumberOfLinksOnEdge" <<  Address_toStream(dstDevice->address32)
                      << " - Links : reserved=" << std::dec << (int)dstReservedLinks << ", total=" << (int)dstTotalLinks);
       return ResponseStatus::REFUSED_INSUFICIENT_RESOURCES;
    }

    return ResponseStatus::SUCCESS;
}

Uint16 Subnet::getNumberOfChildren(Address16 parentDeviceAddress16) {

    Device * parentDevice = subnetNodes[parentDeviceAddress16];
    if(!parentDevice) {
        return 0;
    }

    Uint8 nrChildren = 0;

    for (Address64Set::iterator it = parentDevice->theoAttributes.children.begin(); it != parentDevice->theoAttributes.children.end(); ++it) {
        Device * childDevice = subnetNodes[getAddress16(getAddress32(*it))];
        if (!childDevice) {
            continue;
        }
        if (!childDevice ) {
            continue;
        }
        if(!(getAddress16(childDevice->parent32) == parentDeviceAddress16)
                    && !isBackupForDevice(getAddress16(childDevice->parent32) , parentDeviceAddress16)) {
            continue;
        }
        ++nrChildren;
    }


    return nrChildren;
}


bool Subnet::deviceReachedTheMaxLinksNo(Address16 device, Address64 newDeviceAddress64, bool printWarnMessage /* = true*/) {
    if(!subnetNodes[device]) {
        return true;
    }

    Uint16 parentReservedLinks = subnetNodes[device]->phyAttributes.linkMetadata.currentValue ?
                                    subnetNodes[device]->phyAttributes.linkMetadata.currentValue->used
                                    +  settingsLogic->getSubnetSettings(subnetId).linksMetadataThreshold : 0; // current device

    Uint16 parentTotalLinks = subnetNodes[device]->phyAttributes.linkMetadata.currentValue ? subnetNodes[device]->phyAttributes.linkMetadata.currentValue->total : 0;

    Uint16 parentReservedNeighbors = subnetNodes[device]->phyAttributes.neighborMetadata.currentValue ?
                                    subnetNodes[device]->phyAttributes.neighborMetadata.currentValue->used
                                    +  settingsLogic->getSubnetSettings(subnetId).neighborsMetadataThreshold : 0; // current device

    Uint16 parentTotalNeighbors = subnetNodes[device]->phyAttributes.neighborMetadata.currentValue ?
                                    subnetNodes[device]->phyAttributes.neighborMetadata.currentValue->total : 0;

    Uint16 parentReservedGraphs = subnetNodes[device]->phyAttributes.graphMetadata.currentValue ?
                                    subnetNodes[device]->phyAttributes.graphMetadata.currentValue->used
                                    +  settingsLogic->getSubnetSettings(subnetId).graphsMetadataThreshold : 0; // current device

    Uint16 parentTotalGraphs = subnetNodes[device]->phyAttributes.graphMetadata.currentValue ?
                                    subnetNodes[device]->phyAttributes.graphMetadata.currentValue->total : 0;

    //consider possible other devices that are in progress of joining
    for (Address16Set::iterator itDevice = getActiveDevices().begin(); itDevice != getActiveDevices().end(); ++itDevice) {
        if (!subnetNodes[*itDevice]) {
            continue;
        }

        if (subnetNodes[*itDevice]->status < DeviceStatus::JOIN_CONFIRMED //
                    && subnetNodes[*itDevice]->parent32 == subnetNodes[device]->address32
                    && !(subnetNodes[*itDevice]->address64 == newDeviceAddress64)) //already added once (current device)
        {
            parentReservedLinks += getSubnetSettings().linksMetadataThreshold;
            parentReservedNeighbors +=getSubnetSettings().neighborsMetadataThreshold;
            parentReservedGraphs += getSubnetSettings().graphsMetadataThreshold;
        }
    }

    LOG_DEBUG("Device " << Address_toStream(subnetNodes[device]->address32)
                << " - Links : reserved=" << std::dec << (int)parentReservedLinks << ", total=" << (int)parentTotalLinks
                << ", Neighbors : reserved=" << (int)parentReservedNeighbors << ", total=" << (int)parentTotalNeighbors
                << ", Graphs : reserved=" << (int)parentReservedGraphs << ", total=" << (int)parentTotalGraphs);

    if (parentTotalLinks < parentReservedLinks) {
        if(printWarnMessage) {
            LOG_WARN("Reached max number of links for " << Address_toStream(subnetNodes[device]->address32)
                        << " (reserved=" << std::dec << (int) parentReservedLinks << ", total=" << (int) parentTotalLinks << ")");
        }
        return true;
    }

    if (parentTotalNeighbors < parentReservedNeighbors) {
        if(printWarnMessage) {
            LOG_WARN("Reached max number of neighbors for " << Address_toStream(subnetNodes[device]->address32)
                        << " (reserved=" << std::dec <<(int) parentReservedNeighbors << ", total=" << (int) parentTotalNeighbors << ")");
        }
        return true;
    }

    if (parentTotalGraphs < parentReservedGraphs) {
        if(printWarnMessage) {
            LOG_WARN("Reached max number of graphs for " << Address_toStream(subnetNodes[device]->address32)
                        << " (reserved=" << std::dec <<(int) parentReservedGraphs << ", total=" << (int) parentTotalGraphs << ")");
        }
        return true;
    }

    return false;
}

bool Subnet::isBackupForDevice(Address16 device, Address16 backup) {
    GraphPointer graph = getGraph(DEFAULT_GRAPH_ID);
    if(!graph ) {
        return false;
    }

    if(graph->isBackupForDevice(device, backup)) {
        return true;
    }

    return false;
}

void Subnet::removeDeviceFromAdvertiseChuncksTable(Device * device) {
    if (device) {
        Address16 removingAddress16 = Address::getAddress16(device->address32);

        for(int i = 0; i < MAX_NR_OF_ADV_BANDWIDTH_CHUNCKS; ++i) {
            for(int j = 0; j < MAX_NR_OF_ADV_PERIODS; ++j) {
                if (advertiseChuncksTable[i][j] == removingAddress16) {
                    advertiseChuncksTable[i][j] = 0;
                    LOG_INFO(" remove device " << Address_toStream(device->address32) << " from advertiseChuncksTable");
                }
            }
        }
    }
}

void Subnet::sortCandidates(Address16 device) {
    for(Uint8 i = 1; i< DEFAULT_MAX_NO_CANDIDATES; ++i) {
        for(Uint8 j = 0; j < i; ++j) {
            Device* candidateI = getDevice(subnetNodes[device]->theoAttributes.candidates[i]);
            Device* candidateJ = getDevice(subnetNodes[device]->theoAttributes.candidates[j]);
            if(candidateI && candidateJ && subnetEdges[device][subnetNodes[device]->theoAttributes.candidates[i]] && subnetEdges[device][subnetNodes[device]->theoAttributes.candidates[j]] &&
                        (subnetEdges[device][subnetNodes[device]->theoAttributes.candidates[i]]->getEvalEdgeCost(getSubnetSettings().k1factorOnEdgeCost, getNumberOfChildren(subnetNodes[device]->theoAttributes.candidates[i])) < subnetEdges[device][subnetNodes[device]->theoAttributes.candidates[j]]->getEvalEdgeCost(getSubnetSettings().k1factorOnEdgeCost, getNumberOfChildren(subnetNodes[device]->theoAttributes.candidates[j])))) {
                Address16 pivot = subnetNodes[device]->theoAttributes.candidates[i];
                subnetNodes[device]->theoAttributes.candidates[i] =  subnetNodes[device]->theoAttributes.candidates[j];
                subnetNodes[device]->theoAttributes.candidates[j] = pivot;
            }
        }
    }
}

bool Subnet::deviceHasHighFailRateWithNeighbor( Device * device, Device * neighbor ) {
    if ( !device || !neighbor) {
        return true;
    }

    EdgePointer edge = getEdge(device->address32, neighbor->address32);
    if ( !edge ) {
        return true;
    }

    GraphPointer graph = getGraph(DEFAULT_GRAPH_ID);
    if ( !graph ) {
        return true;
    }

    Uint16 nrPublishForFailRate = getSubnetSettings().noOfPublishForFailRate;
    Uint16 nrLastSentPackages = getSubnetSettings().numberOfSentPackagesForFailRate;
    Uint16 badTxRateThreshold = getSubnetSettings().badTransferRateThreshlod;

    Uint16 nrPublishForFailRateShort = getSubnetSettings().noOfPublishForFailRateShortPeriod;
    Uint16 nrLastSentPackagesShort = getSubnetSettings().numberOfSentPackagesForFailRateShort;
    Uint16 badTxRateThresholdShort = getSubnetSettings().badTransferRateThreshlodShort;

    if ( graph->getBackupFor(Address::getAddress16(device->address32)) == Address::getAddress16( neighbor->address32) ) {
        nrPublishForFailRate = getSubnetSettings().noOfPublishForFailRateOnBackup;
        nrPublishForFailRateShort = getSubnetSettings().noOfPublishForFailRateOnBackupShortPeriod;
        nrLastSentPackages = getSubnetSettings().numberOfSentPackagesForFailRateOnBackup;
        nrLastSentPackagesShort = getSubnetSettings().numberOfSentPackagesForFailRateOnBackupShort;
        badTxRateThreshold = getSubnetSettings().badTransferRateThreshlodOnBackup;
        badTxRateThresholdShort = getSubnetSettings().badTransferRateThreshlodOnBackupShort;
    }

    if (edge->getNumberOfLastSentPackages() == 0) { //is backup without ping
        LOG_INFO("There were not sent packages between device " <<  Address_toStream(device->address32)
					<< " and its neighbor " <<  Address_toStream(neighbor->address32));
        return false;
    }

    int packetErrorRate = edge->getFailedTxPercent();
    LOG_INFO("PER      ("<< Address_toStream(device->address32) << "->" << Address_toStream(neighbor->address32) << ")=" << packetErrorRate);
    packetErrorRate = edge->getFailedTxPercentShort();
    LOG_INFO("PER_short("<< Address_toStream(device->address32) << "->" << Address_toStream(neighbor->address32) << ")=" << packetErrorRate);

    if (( nrPublishForFailRate == edge->getPublishDiasgNo()
         && edge->getNumberOfLastSentPackages() >= nrLastSentPackages
         && edge->getFailedTxPercent() > badTxRateThreshold)
         ||
         ( nrPublishForFailRateShort <= edge->getPublishDiasgNo()
          && edge->getNumberOfLastSentPackagesShort() >= nrLastSentPackagesShort
          && edge->getFailedTxPercentShort() > badTxRateThresholdShort)){
        LOG_INFO("PER is higher then threshold between device " <<  Address_toStream(device->address32) << "and its neighbor"<<  Address_toStream(neighbor->address32) );
        return true;
    }

    return false;

}

bool Subnet::checkFailRateOnNewPath( Device * device) {
    if ( !device ) {
        return true;
    }

    if ( device->capabilities.isBackbone()) {
        return false;
    }

    if ( device->isAlreadyVisited() ) {
        return true; //is a clock cycle
    }

    device->setVisited();

    Device * parent = getDevice( device->parent32 );
    if ( !parent ) {
        return true;
    }

    if ( deviceHasHighFailRateWithNeighbor( device, parent )) {
        return true;
    }

    return checkFailRateOnNewPath( parent );
}

void Subnet::addBadRateCandidate( Device * device, Device * neighbor) {
    if ( device &&  neighbor) {
        LOG_INFO("addBadCandidate for device " <<  Address_toStream(device->address32) << ", badCandidate="<<  Address_toStream(neighbor->address32) );
        LOG_INFO("addBadCandidate for device " <<  Address_toStream(neighbor->address32) << ", badCandidate="<<  Address_toStream(device->address32) );
        device->theoAttributes.addBadRateCandidate(Address::getAddress16(neighbor->address32));
        neighbor->theoAttributes.addBadRateCandidate(Address::getAddress16(device->address32));
        device->theoAttributes.deleteCandidate(Address::getAddress16(neighbor->address32));
        neighbor->theoAttributes.deleteCandidate(Address::getAddress16(device->address32));
    }
}



}

}

