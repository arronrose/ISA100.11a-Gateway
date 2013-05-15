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
 * TheoreticEngine.cpp
 *
 *  Created on: Sep 18, 2009
 *      Author: Catalin Pop, beniamin.tecar
 */
#include <iomanip>
#include <boost/bind.hpp>
#include "modelDefault.h"
#include "TheoreticEngine.h"
#include "Model/RouteEngine.h"
#include "Model/Tdma/LinkEngine.h"
#include "Model/ChainAddDeviceToRoleActivation.h"
#include "Model/ChainWaitForConfirmOnEvalGraph.h"
#include "Model/ChainForceReevalRouteOnFail.h"
#include "Model/ChainForceReevalContractOnFail.h"
#include "Model/ChainAddNewContainerOnOldContainerConfirm.h"
#include "Model/ChainCreateUdoLinks.h"
#include "Model/Operations/DeleteAttributeOperation.h"
#include "Model/Operations/WriteAttributeOperation.h"
#include "Model/Operations/ReadAttributeOperation.h"
#include "Model/Routing/GraphPrinter.h"

using namespace NE::Common;
using namespace NE::Model::Operations;
using namespace NE::Model::Tdma;

namespace NE {
namespace Model {

TheoreticEngine::TheoreticEngine(SubnetsContainer * subnetsContainer_,
            NE::Model::Operations::OperationsProcessor * operationsProcessor_,
            NE::Model::IDeviceRemover * devicesRemover_,
            NE::Common::ISettingsProvider * settingsProvider_) :
    subnetsContainer(subnetsContainer_),
    operationsProcessor(operationsProcessor_),
    settingsProvider(settingsProvider_),
    devicesRemover(devicesRemover_){

    routeEngine = new NE::Model::RouteEngine();
    linkEngine = new NE::Model::Tdma::LinkEngine(subnetsContainer, operationsProcessor);
}

TheoreticEngine::~TheoreticEngine() {
    delete routeEngine;
    delete linkEngine;
}

void TheoreticEngine::createJoinEdge(Device * joiningDevice, Device * parentDevice, Subnet::PTR subnet) {
	//create edge between device and its parent(device->parent)
    Routing::EdgePointer edgeJoin1(new Routing::Edge(joiningDevice->address32, parentDevice->address32));
    edgeJoin1->setEdgeStatus(Status::ACTIVE);
    subnet->addEdge(edgeJoin1);

	//create edge between device's parent and device(parent->device)
    Routing::EdgePointer edgeJoin2(new Routing::Edge(parentDevice->address32, joiningDevice->address32));
    edgeJoin2->setEdgeStatus(Status::ACTIVE);
    subnet->addEdge(edgeJoin2);

    //if the joining device is gateway, return because gateway doesn't have to be added to the default graph
    if (joiningDevice->capabilities.isGateway()) {
        return;
    }

    // add edge(device->parent) to the inbound graph
    Uint16 graphId = DEFAULT_GRAPH_ID;
    subnet->addEdgeToGraph(Address::getAddress16(joiningDevice->address32), Address::getAddress16(parentDevice->address32), graphId);

    // add device to the list of devices to read join reason
    subnet->listDeviceToReadJoinReason.push_back(Address::getAddress16(joiningDevice->address32));
}

bool TheoreticEngine::allocateManagementJoinLinks(Device * joiningDevice, Device * parentDevice, Operations::OperationsContainerPointer& operationsContainer){
	//allocate first management links for a joining device ..the links with its parent
    return linkEngine->setMngFirstLinks(joiningDevice, parentDevice, *operationsContainer);

}

void TheoreticEngine::periodicEvaluation(Subnet::PTR subnet, NE::Common::EvaluationSignal evaluationSignal, Uint32 currentTime) {
    // based on values of signal flags perform the corresponding evaluation action
    if (NE::Common::isRoleActivationSignal(evaluationSignal)) {
        periodicRoleActivation(subnet, currentTime);
        periodicConfigureNeighborDiscovery(currentTime, subnet);
    } else if (NE::Common::isRoutesEvaluationSignal(evaluationSignal)) {
        periodicRoutesEvaluation(subnet);
        readJoinReason(subnet, currentTime);
    } else if (NE::Common::isGraphRedundancySignal(evaluationSignal)) {
        periodicGraphsEvaluation(subnet, currentTime);
    } else if (NE::Common::isGarbageCollectionSignal(evaluationSignal)) {
        periodicGraphsRedundancyEvaluation(subnet, currentTime);//TODO create another signal for this.. for tmp verification

        //remove one graph from list to be removed
        for(GraphsSet::iterator itRemove = subnet->graphsToBeRemoved.begin() ; itRemove != subnet->graphsToBeRemoved.end(); ++itRemove ) {
            deleteGraph(subnet, *itRemove);
            subnet->graphsToBeRemoved.erase(itRemove);
            break;
        }
    } else if (NE::Common::isRemoveAccLinksSignal(evaluationSignal)) {
        periodicRemoveAcceleratedLinks(subnet, currentTime);
    } else if (NE::Common::isUdoContractsSignal(evaluationSignal)) {
        periodicUdoContractsEvaluation(subnet);
    } else if (NE::Common::isClientServerContractsSignal(evaluationSignal)) {
        periodicClientServerContractsEvaluation(subnet);
    } else if (NE::Common::isDirtyContractsSignal(evaluationSignal)){
        if (!periodicDirtyLinksRemoval(subnet, currentTime)){//if no link removal was performed ..check dirty contracts
            periodicDirtyContractsEvaluation(subnet);
        }
    } else if (NE::Common::isFindBetterParentSignal(evaluationSignal) && subnet->isEnabledInboundGraphEvaluation()) {
//#warning CHANGE OF PARENT IS COMMENTED

		// Star Topology
        if (!subnet->getSubnetSettings().enableMultiPath
                    || subnet->getSubnetSettings().enableStarTopology
                    || subnet->getSubnetSettings().nrRoutersPerBBR <= 1) {
            return;
        }

        findBetterParentForDevices(subnet, currentTime);
    } else if(NE::Common::isDirtyEdgesSignal(evaluationSignal) && subnet->isEnabledInboundGraphEvaluation()) {
        periodicEvaluateDirtyEdges(subnet);
    } else if(NE::Common::isRoutersAdvertiseCheckSignal(evaluationSignal) ) {
        periodicRoutersAdvertiseCheck(subnet);
    } else if (NE::Common::isFastDiscoveryCheckSignal(evaluationSignal)) {
        periodicFastDiscoveryCheck(subnet, currentTime);
    }

    updateAdvertisePeriod(subnet);

}

void TheoreticEngine::readJoinReason(Subnet::PTR subnet, Uint32 currentTime) {
    if (subnet->listDeviceToReadJoinReason.empty()) {
        // LOG_DEBUG("No device to read join reason.");
        return;
    }

    for (Address16List::iterator itDev = subnet->listDeviceToReadJoinReason.begin();
                itDev != subnet->listDeviceToReadJoinReason.end(); ++itDev) {

        Device * device = subnet->getDevice(*itDev);
        if (device == NULL) {
            LOG_INFO("Read join reason: Device not found " << Address_toStream(*itDev));
            subnet->listDeviceToReadJoinReason.erase(itDev);
            return;
        }

        // read join reason for a device 7 minutes after its configuration
        if ( device->statusForReports == StatusForReports::JOINED_AND_CONFIGURED
                    && (device->joinConfirmTime + 7 * 60) < currentTime ) {

        	//don't read join reason for a honeyWell device
            PhyString * softwareRevisionInformation = device->phyAttributes.softwareRevisionInformation.getValue();
            if (!softwareRevisionInformation || softwareRevisionInformation->value.find("HR") != string::npos) {
                LOG_INFO("Device " << Address_toStream(device->address32) << ": softwareRevisionInformation is NULL or is a honeywell device.");
                subnet->listDeviceToReadJoinReason.erase(itDev);
                return;
            }

            char reason[64];
            sprintf(reason, "readJoinReason, device=%x", device->address32);
            OperationsContainerPointer operationsContainer(new OperationsContainer(reason));

            EntityIndex indexJoinReason = createEntityIndex(device->address32, EntityType::JoinReason, 0);
            ReadAttributeOperationPointer readJoinReason(new ReadAttributeOperation(indexJoinReason));
            operationsContainer->addOperation(readJoinReason, device);

            operationsProcessor->addOperationsContainer(operationsContainer);

            subnet->listDeviceToReadJoinReason.erase(itDev);
            return;

        } else if (device->statusForReports != StatusForReports::JOINED_AND_CONFIGURED) {
            //remove device if 5 minutes have elapsed since join start time and device is not yet JOINED_AND_CONFIGURED
            if ((currentTime - device->startTime) > settingsProvider->getSettingsLogic().joinMaxDuration ) {
                LOG_INFO("Join timeout! Elapsed time from join start: " << currentTime - device->startTime
                            << ". Removing device from listDeviceToReadJoinReason " << Address_toStream(device->address32));
                subnet->listDeviceToReadJoinReason.erase(itDev);
                return;
            }
        }
    }
}

void TheoreticEngine::periodicFastDiscoveryCheck(Subnet::PTR& subnet, Uint32 currentTime) {
    if (subnet->listDiscoveryToBeAccelerated.empty() && subnet->listOfFastDiscovery.empty()){
        return;
    }

    //default neighbor discovery superframe length is 5700(57 seconds). For the devices that don't have redundancy, neighbor discovery must be fastened
    // a fast neighbor discovery will have a superframe length of 800(8 seconds)
    char reasonAccelerate[64];
    sprintf(reasonAccelerate, "make fast neighbor discovery");
    OperationsContainerPointer operationsContainerAccelerate(new OperationsContainer(reasonAccelerate));

    while (!subnet->listDiscoveryToBeAccelerated.empty()){
        Address16 deviceAddress16 = subnet->listDiscoveryToBeAccelerated.front();
        subnet->listDiscoveryToBeAccelerated.pop_front();
        Device * device = subnet->getDevice(deviceAddress16);
        if (device == NULL){
            break;
        }
        if (device->fastDiscoveryTime != 0){//is allready at fast discovery
            break;
        }

        linkEngine->changeNeighborDiscovery(*operationsContainerAccelerate, subnet, device, true);
        subnet->addToFastDiscovery(device, currentTime);
    }

    if (!operationsContainerAccelerate->isContainerEmpty()){
        operationsProcessor->addOperationsContainer(operationsContainerAccelerate);
    }

    if (!subnet->listOfFastDiscovery.empty()){
        Address16 deviceAddress16 = subnet->listOfFastDiscovery.front();
        subnet->listOfFastDiscovery.pop_front();

        Device * device = subnet->getDevice(deviceAddress16);
        if (device != NULL){
            if (device->fastDiscoveryTime + subnet->getSubnetSettings().fastDiscoveryTimespan < currentTime
                        && !device->phyAttributes.candidatesTable.empty()){
                //make it slow discovery again
                char reasonSlow[128];
                sprintf(reasonSlow, "make slow neighbor discovery: device=%x, fastTime=%d, currentTime=%d", device->address32, device->fastDiscoveryTime, currentTime);
                OperationsContainerPointer operationsContainerSlow(new OperationsContainer(reasonSlow));
                linkEngine->changeNeighborDiscovery(*operationsContainerSlow, subnet, device, false);
                device->fastDiscoveryTime = 0;
                operationsProcessor->addOperationsContainer(operationsContainerSlow);
            } else {
                subnet->listOfFastDiscovery.push_back(deviceAddress16);
            }
        }
    }
}
void TheoreticEngine::periodicRoutersAdvertiseCheck(Subnet::PTR& subnet) {
	/**
	 * The default period for routers's advertise links is 7 seconds.
	 * Advertise links period for routers can be changed when needed:
	 * If the router is full of children, the advertise will be slowed. (30 seconds)
	 * If the router's level is >= maxNumberOfLayers, the advertise will be slowed. (30 seconds)
	 * If the router has reached the max number of links, the advertise will be slowed. (30 seconds)
	 */

    if (subnet->listOfAdvertisingRouters.empty()){
        return;
    }

    Address16 routerAddress16 = subnet->listOfAdvertisingRouters.front();
    subnet->listOfAdvertisingRouters.pop_front();
    Device * router = subnet->getDevice(routerAddress16);
	if (router == NULL){
		return;
	}
    subnet->listOfAdvertisingRouters.push_back(routerAddress16);//move router to back of list to force verification of all routers periodically


    const SubnetSettings& subnetSettings = subnet->getSubnetSettings();
    int advPeriod = subnetSettings.join_adv_period * subnetSettings.getSlotsPerSec();

    Uint8 childsRouters = 0;
    Uint8 childsIO = 0;
    subnet->getCountDirectChildsAndRouters(router, childsRouters, childsIO);
    int freeSpaceRouters = subnetSettings.nrRoutersPerRouter - childsRouters;
    int freeSpaceIO = subnetSettings.nrNonRoutersPerRouter - childsIO;
    PhyMetaData * linkMetadata = router->phyAttributes.linkMetadata.getValue();
    Int8 level = subnet->getDeviceLevel(router->address32);

    if (level >= subnetSettings.maxNumberOfLayers){//If the router's level is >= maxNumberOfLayers, the advertise will be slowed. (30 seconds)
        advPeriod = subnetSettings.getSlotsPer30Sec();
    } else if ((subnetSettings.nrRoutersPerRouter > 2 && freeSpaceRouters <= 1) || (subnetSettings.nrNonRoutersPerRouter > 2 && freeSpaceIO <= 1) ){
    	// If the router is full of children, the advertise will be slowed. (30 seconds)
        advPeriod = subnetSettings.getSlotsPer30Sec();
    } else if (linkMetadata && (linkMetadata->used + subnetSettings.linksMetadataThreshold >= linkMetadata->total)) {//if reached max number of links
        advPeriod = subnetSettings.getSlotsPer30Sec();
    } else if (freeSpaceRouters >= 2 || freeSpaceIO >= 2){
    	// If the router is full of children, the advertise will be slowed. (30 seconds)
        if (router->capabilities.isBackbone()) {
            advPeriod = subnetSettings.join_bbr_adv_period * subnetSettings.getSlotsPerSec();
        } else {
            advPeriod = subnetSettings.join_adv_period * subnetSettings.getSlotsPerSec();
        }
    } else {
        return;
    }

    char reason[64];
    sprintf(reason, "change adv period, device=%x, period=%d", router->address32, advPeriod);
    OperationsContainerPointer operationsContainer(new OperationsContainer(reason));
    linkEngine->changeAdvertiseLinkPeriod(router, advPeriod, subnet, operationsContainer);

    if (!operationsContainer->isContainerEmpty()){
        operationsProcessor->addOperationsContainer(operationsContainer);
    }

}

void TheoreticEngine::updateAdvertisePeriod(Subnet::PTR& subnet) {

    if (!subnet->getUpdateAdvPeriod()) {
        return;
    }

    const SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    for(Address32 i = 0; i < MAX_NUMBER_OF_DEVICES; ++i) {
        Device * device = subnet->getDevice(i);
        if (device && device->capabilities.isRouting()) {

            LOG_DEBUG("updateAdvPeriod - subnetID=" << subnet->getSubnetId()
                        << ", device=" << Address_toStream(device->address32)
                        << ", subnetSettings=" << subnetSettings);
            PhyAdvJoinInfo * phyAdvJoinInfo = device->phyAttributes.advInfo.getValue();
            if (!phyAdvJoinInfo) {
                LOG_INFO("updateAdvPeriod - NULL");
                continue;
            }
            LOG_DEBUG("updateAdvPeriod - phyAdvJoinInfo=" << *phyAdvJoinInfo);

            if (phyAdvJoinInfo->advRx.interval == subnetSettings.join_adv_period * subnetSettings.getSlotsPerSec()) {
                LOG_INFO("updateAdvPeriod - advRx.interval NOT CHANGED; advRx.interval=" << (int)phyAdvJoinInfo->advRx.interval << ", subnetSettings.join_adv_period=" << (int)subnetSettings.join_adv_period);
                continue;
            }
            LOG_INFO("updateAdvPeriod - advRx.interval CHANGED ########; advRx.interval=" << (int)phyAdvJoinInfo->advRx.interval << ", subnetSettings.join_adv_period=" << (int)subnetSettings.join_adv_period);

            char reason[64];
            sprintf(reason, "updateAdvPeriod, device=%x", device->address32);
            OperationsContainerPointer operationsContainer(new OperationsContainer(reason));
            if (!linkEngine->updateAdvPeriod(device, subnet, operationsContainer)) {
                LOG_ERROR("Fail to Update Adv Period for device " << Address_toStream(device->address32));
                continue;
            }
            operationsProcessor->addOperationsContainer(operationsContainer);
            return;
        }
    }
    subnet->setUpdateAdvPeriod(false);
}

void TheoreticEngine::periodicRemoveAcceleratedLinks(Subnet::PTR& subnet, Uint32 currentTime){
    // LOG_DEBUG("remove accelerated links.");

    char reason[64];
    sprintf(reason, "remove accelerated links");
    OperationsContainerPointer operationsContainer(new OperationsContainer(reason));


    GraphPointer graph = subnet->getGraph(DEFAULT_GRAPH_ID);
    RETURN_ON_NULL(graph);
    const DoubleExitEdges & edges = graph->getGraphEdges();
    DoubleExitEdges::const_iterator itEdge = edges.begin();

    for (; itEdge != edges.end(); ++itEdge){
        Device * device = subnet->getDevice(itEdge->first);
        if (device == NULL) continue;
        Device * parent = subnet->getDevice(itEdge->second.prefered);
        if (parent == NULL) continue;
        Device * parentBackup = subnet->getDevice(itEdge->second.backup);
        linkEngine->destroyAccMngLink(device, parent, parentBackup, *operationsContainer, subnet, currentTime);
    }

    if (!operationsContainer->isContainerEmpty()){
        operationsProcessor->addOperationsContainer(operationsContainer);
    }

}

void TheoreticEngine::periodicGraphsRedundancyEvaluation(Subnet::PTR subnet, Uint32 currentTime){

	/*
	 * each edge  from inbound graph is checked :
	 * 1. all the contracts that pass through that edge are summed in order to obtain the needed traffic
	 * 2. all the application links that passes through that edge are summed in order to obtain the allocated band
	 * 3. if the allocated band < needed traffic => need to allocate more band => the period of links will be fastened
	 * 4. if the allocated band > needed traffic => need to deallocate band => the period of links will be slowed.
	 *
	 */

	//if the inbound graph is in evaluation, the edge checking has no sense as long as an inbound graph evaluation can modify any inbound edge
	if(subnet->isEnabledInboundGraphEvaluation()) {
        char reason[128];
        sprintf(reason, "Evaluation of backup&retry trafic for graph %x", DEFAULT_GRAPH_ID);
        OperationsContainerPointer operationsContainer(new OperationsContainer(reason));
        linkEngine->updateMngAndAppRedundantInboundLinks(subnet, /*graph 1*/ DEFAULT_GRAPH_ID , operationsContainer, currentTime);
        operationsProcessor->addOperationsContainer(operationsContainer);

    } else {
        LOG_DEBUG("subnet " << (int)subnet->getSubnetId() << ", defaultGraphCanBeReevaluated is false!");
    }
}

void TheoreticEngine::periodicRoleActivation(Subnet::PTR subnet, Uint32 currentTime) {
	//after a router device is joined and configured, it will be added in a list of devicesToActivateRole
    if (subnet->devicesToActivateRole.empty()) {
        return;
    }

    for (Address32Set::iterator it = subnet->devicesToActivateRole.begin(); it != subnet->devicesToActivateRole.end(); ++it) {
        Device * device = subnet->getDevice(*it);
        if (device == NULL){
            subnet->devicesToActivateRole.erase(it);
            break;
        }
        if (device->capabilities.isGateway()) {
            subnet->devicesToActivateRole.erase(it);
            return;
        }
        if (device->capabilities.isBackbone() || device->capabilities.isRouting()) {

            if (!device->capabilities.isBackbone()) {//check if activate role delay has elapsed (except for BBR)
                if (currentTime - subnet->getSubnetSettings().delayActivateRouterRole < device->joinConfirmTime){
                    //delay not elapsed for device so continue to next device
                    continue;
                }
            }
            char reason[64];
            sprintf(reason, "activate router role, device=%x", device->address32);
            OperationsContainerPointer operationsContainer(new OperationsContainer(reason));
            linkEngine->activateRouterRole(device, subnet, operationsContainer);

            if (!operationsContainer->isContainerEmpty()){
                operationsProcessor->addOperationsContainer(operationsContainer);
            }

            subnet->devicesToActivateRole.erase(it);
            return;
        }
    }
}

void TheoreticEngine::periodicRoutesEvaluation(Subnet::PTR subnet) {

	/**
	 * right after device's join, the route on backbone to that device is a so-called "hybrid-route" : outbound_graph_of_device_parent + device's address
	 * The device's route is marked to be evaluated in order to become a graph-based route.
	 */
    if ( subnet->routesToBeEvaluated.empty() ) {
        return;
    }

    EntityIndex entityIndex = subnet->routesToBeEvaluated.front();
    subnet->routesToBeEvaluated.pop_front();
    Device* device = subnet->getDevice(getDeviceAddress(entityIndex));

    if (device == NULL) {
        return;
    }
//    LOG_DEBUG("device =" << *device << " entity index = " << std::hex << entityIndex);
//    LOG_DEBUG(device->phyAttributes.routesTable);

    RouteIndexedAttribute::iterator itRoutes = device->phyAttributes.routesTable.find(entityIndex);
    if(itRoutes == device->phyAttributes.routesTable.end()){
        LOG_WARN("Evaluation of allready removed route entIndex=" << std::hex << entityIndex);
        return;
    }

    if(itRoutes->second.currentValue == NULL){
        LOG_ERROR("Evaluation of an inconsistent route. current value NULL for entIndex=" << std::hex << entityIndex);
        return;
    }
    if (itRoutes->second.isOnPending() ){
        LOG_WARN("Evaluation of a pending route delayed. (entIndex=" << std::hex << entityIndex );
        subnet->routesToBeEvaluated.push_back(entityIndex);//ad route to be evaluated later when the pending status should change.
        return;
    }

    PhyRoute * phyRoute = itRoutes->second.getValue();
    Uint16 graphId;
    if (phyRoute && phyRoute->evaluationTime == 0) { // first time evaluated
        Device* joinedDevice = subnet->getDevice(phyRoute->selector);
        RETURN_ON_NULL(joinedDevice);
        Device* parent = subnet->getDevice(joinedDevice->parent32);
        RETURN_ON_NULL(parent);

        //create new out-bound graph
        graphId = subnet->getNextGraphId();
        subnet->createGraph(graphId, Address::getAddress16(joinedDevice->address32));

        ChainAddDeviceToRoleActivationPointer chainAddToRoleActivation(new ChainAddDeviceToRoleActivation(subnet));
        ChainForceReevalRouteOnFailPointer chainForceReeval(new ChainForceReevalRouteOnFail(subnet, entityIndex, graphId));
        HandlerResponseList handlersRouteEval;
        handlersRouteEval.push_back(boost::bind(&ChainAddDeviceToRoleActivation::process, chainAddToRoleActivation, _1, _2, _3));
        handlersRouteEval.push_back(boost::bind(&ChainForceReevalRouteOnFail::process, chainForceReeval, _1, _2, _3));


        if(parent->capabilities.isBackbone()) {//if parent is backbone, backbone doesn't have an outbound graph
            subnet->addEdgeToGraph(Address::getAddress16(parent->address32), Address::getAddress16(joinedDevice->address32), graphId);
        } else {
            Uint16 parentGraphID = 0;
            if (isRouteGraphElement(phyRoute->route[0])){
                parentGraphID = getRouteElement(phyRoute->route[0]);
            }
            //populate device's new outbound graph with the edges from its parent's outbound graph
            subnet->createGraphFromParent(graphId, parentGraphID);
            subnet->addEdgeToGraph(Address::getAddress16(parent->address32), Address::getAddress16(joinedDevice->address32), graphId);
        }

        //updateGraphOperation->source = phyRoute->selector;
        char reason[64];
        sprintf(reason, "First evaluation for route %llx", entityIndex);

        OperationsContainerPointer operationsContainer(new OperationsContainer(joinedDevice->address32, 0, handlersRouteEval, reason));

        EntityIndex entityIndexUpdateGraph = createEntityIndex(device->address32, EntityType::Graph, graphId);
        updateGraphOnJoin(operationsContainer, subnet, graphId, Address::getAddress16(joinedDevice->address32),
                    Address::getAddress16(parent->address32));

        PhyRoute * att_Route = new PhyRoute();
        att_Route->index = getIndex(entityIndex);
        att_Route->alternative = 2;
        att_Route->selector = phyRoute->selector;
        LOG_DEBUG("selector = " <<  phyRoute->selector);
        att_Route->route.push_back((ROUTE_GRAPH_MASK | graphId));
        att_Route->evaluationTime = 1;

        IEngineOperationPointer writeRoute(new Operations::WriteAttributeOperation(att_Route, entityIndex));
        OperationsList& operations = operationsContainer->getUnsentOperations();
        for (OperationsList::iterator it = operations.begin(); it != operations.end(); ++it) {
            if((*it)->getEntityIndex() ==  entityIndexUpdateGraph ) {
                writeRoute->addOperationDependency(*it);
            }
        }

        operationsContainer->addOperation(writeRoute, device);
        operationsProcessor->addOperationsContainer(operationsContainer);
    }
}

void TheoreticEngine::periodicGraphsEvaluation(Subnet::PTR& subnet, Uint32 currentTime) {

    if (subnet->graphsToBeEvaluated.empty()) {
    	return;
    }


    static int stepCheckGraphEvaluation = 0;

    //if inbound ot outbound graph evaluationm is disabled => check the disability period.
    if(!subnet->isEnabledInboundGraphEvaluation() ||  !subnet->isEnabledOutboundGraphEvaluation()) {
        if (stepCheckGraphEvaluation < 10) {
            ++stepCheckGraphEvaluation;
        } else {
            if (!subnet->isEnabledInboundGraphEvaluation() && (currentTime - subnet->getLastLockInboundGraphCanBeReevaluated() > 600)) {
                LOG_WARN("Force enable Inbound Graph Evaluation => true");
                subnet->enableInboundGraphEvaluation();
            }
            if (!subnet->isEnabledOutboundGraphEvaluation() && (currentTime - subnet->getLastLockOutboundGraphCanBeReevaluated() > 600)) {
                LOG_WARN("Force enable OutBound Graph Evaluation => true");
                subnet->enableOutBoundGraphEvaluation();
            }
            stepCheckGraphEvaluation = 0;
        }
        return;
    }


    char reason[128];
    sprintf(reason, "periodic Graphs Evaluation: ");

    OperationsContainerPointer operationsContainer(new OperationsContainer(reason));

    Uint8 noOfEvaluatedOutboundGraphs = 0;
    Uint16 graphId = 0;

    GraphsList::iterator it = subnet->graphsToBeEvaluated.begin();
    for( ; it != subnet->graphsToBeEvaluated.end();  ) {
    	bool isInboundGraph = ( *it == DEFAULT_GRAPH_ID);
        if(!subnet->existsGraph(*it)) {
            it = subnet->graphsToBeEvaluated.erase(it);
            continue; // it is invalid here
        }

        if (noOfEvaluatedOutboundGraphs >= subnet->getSubnetSettings().nrOfOutboundGraphsToBeEvaluated) {
        	break;
        }

        if( !isInboundGraph && !subnet->isEnabledOutboundGraphEvaluation() && ( noOfEvaluatedOutboundGraphs == 0)) {//some outbound graphs are already in evaluation
        	++it;
        	continue;
        }

        if (isInboundGraph && noOfEvaluatedOutboundGraphs !=0 ) {
        	break;
        }

        graphId = *it;
        it = subnet->graphsToBeEvaluated.erase(it);

        evaluateGraph(subnet, graphId, currentTime, operationsContainer); // evaluate one graph at a call

        sprintf(reason, "%s %d, ",reason, graphId);
        operationsContainer->reasonOfOperations = reason;

        if ( isInboundGraph ) {
            break; // it is invalid here
        }
        else {
            ++noOfEvaluatedOutboundGraphs;
        }
    }


    if (!operationsContainer->isContainerEmpty()){
        ChainWaitForConfirmOnEvalGraphPointer chainConfirmGraphOperations(new ChainWaitForConfirmOnEvalGraph(subnet, graphId));
        operationsContainer->addHandlerResponse(boost::bind(&ChainWaitForConfirmOnEvalGraph::process, chainConfirmGraphOperations, _1, _2, _3));
        if (noOfEvaluatedOutboundGraphs == 0){ //inbound graph evaluation
             LOG_DEBUG("INBOUND GRAPH EVAL ");
              subnet->disableInboundGraphEvaluation();
         }
        else {
            LOG_DEBUG("OUTBOUND GRAPH EVAL disable" );
             subnet->disableOutboundGraphEvaluation();
        }
    }

    operationsProcessor->addOperationsContainer(operationsContainer);
}

void TheoreticEngine::addUdoContractToBeEvaluated(Subnet::PTR subnet, EntityIndex & eiContract) {
    if (std::find(subnet->udoContractsToBeEvaluated.begin(), subnet->udoContractsToBeEvaluated.end(), eiContract)
            == subnet->udoContractsToBeEvaluated.end() ) {
        subnet->udoContractsToBeEvaluated.push_back(eiContract);
    }
}

void TheoreticEngine::updateUdoContract(Subnet::PTR subnet, EntityIndex & eiContract, Int16 newCommittedBurst) {

    LOG_INFO("updateUdoContract eiContract=" << std::hex << eiContract << ", newCommitedBurst=" << (int)newCommittedBurst);

    // dll contract
    ContractIndexedAttribute::iterator itContract = subnet->getManager()->phyAttributes.contractsTable.find(eiContract);
    if (itContract == subnet->getManager()->phyAttributes.contractsTable.end()) {
        LOG_WARN("Contract not in contracts table " << std::hex << eiContract);
        return;
    }

    PhyContract * phyContract = itContract->second.getValue();
    if (phyContract->assignedCommittedBurst == newCommittedBurst) {
        return;
    }

    phyContract->assignedCommittedBurst = newCommittedBurst;//change direcly in Phy


    // the associated network contract
    EntityIndex eiNetContract = createEntityIndex(getDeviceAddress(eiContract), EntityType::NetworkContract, getIndex(eiContract));
    NetworkContractIndexedAttribute::iterator itNetContract = subnet->getManager()->phyAttributes.networkContractsTable.find(eiNetContract);
    if (itNetContract == subnet->getManager()->phyAttributes.networkContractsTable.end()) {
        LOG_WARN("Network Contract not in contracts table " << std::hex << eiNetContract);
        return;
    }

    PhyNetworkContract * phyNetContract = itNetContract->second.getValue();
    if (phyNetContract->assignedCommittedBurst == newCommittedBurst) {
        return;
    }

    PhyNetworkContract * phyNetContractCopy = new PhyNetworkContract(*phyNetContract);
    phyNetContractCopy->assignedCommittedBurst = newCommittedBurst;

    IEngineOperationPointer updateNetContractOperation(new WriteAttributeOperation(phyNetContractCopy, itNetContract->first));
    operationsProcessor->addManagerOperation(updateNetContractOperation);
}

void TheoreticEngine::onChangeParentForDevice(Subnet::PTR & subnet, Device * changingDevice) {

    // mark the device for HRCO reconfigured
    subnet->addNotPublishingDevice(changingDevice->address32);
    changingDevice->setDirtyInbound();//mark APP inbound trafic as dirty.. to be reevaluated.

    // mark udo contracts that passing through changingDevice device to be evaluated
    for(LinkToContractMap::iterator it = changingDevice->theoAttributes.link2UdoContract.begin();
        it != changingDevice->theoAttributes.link2UdoContract.end(); ++it) {

        if (std::find(subnet->udoContractsToBeEvaluated.begin(), subnet->udoContractsToBeEvaluated.end(), it->second)
                == subnet->udoContractsToBeEvaluated.end()) {
            addUdoContractToBeEvaluated(subnet, it->second);
            updateUdoContract(subnet, it->second, -8);
        }
    }
}

void TheoreticEngine::evaluateGraph(Subnet::PTR subnet, Uint16 graphId, Uint32 currentTime, Operations::OperationsContainerPointer & operationsContainer) {

//     LOG_DEBUG("Evaluation graph " << (int)graphId);

     Device* backbone = subnet->getBackbone();
     if( !backbone ) {
         return; // was evaluated
     }

     GraphPointer graph = subnet->getGraph(graphId);
     if( !graph) {
         LOG_ERROR("evaluateGraph: in subnet:" << (int)subnet->getSubnetId() << " doesn't exists graph " << (int)graphId);
         return ; // was evaluated
     }

     DoubleExitEdges usedEdges;
     DoubleExitEdges notUsedEdges;

     bool isInbound = (graphId == DEFAULT_GRAPH_ID);

     LOG_INFO("Evaluation graph " << (int)graphId << " destination " << std::hex << graph->getDestination() << " backbone " << Address::getAddress16(backbone->address32));
     LOG_DEBUG("BEFORE GRAPH EVALUATION...GraphID=" << (int)graphId << ":" << NE::Model::Routing::GraphPrinter(subnet.get(), graph) );

     bool isEvaluationOnRemoveDevice = false;
     if( isInbound ) {// INBOUND graph

         if(!graph->removedDevices.empty()) {
             isEvaluationOnRemoveDevice = true;
         }
         routeEngine->periodicEvaluateInboundGraph(subnet, graph, subnet->getManagerAddress32(), usedEdges, notUsedEdges);
     }
     else { //outBound graph

         routeEngine->periodicEvaluateOutBoundGraph(subnet, graph, graph->getDestination(), usedEdges,  notUsedEdges);
     }

     std::ostringstream graphsToBeEval;
     for( GraphsList::iterator itGraphs =  subnet->graphsToBeEvaluated.begin(); itGraphs !=  subnet->graphsToBeEvaluated.end(); ++itGraphs) {
         graphsToBeEval <<  "," << (int)*itGraphs;
     }

     LOG_INFO("GRAPH_TO_BE_EVALUATED :" << graphsToBeEval.str());

     if(usedEdges.empty()) { //the evaluated graph is empty..must delete the outbound route..and the graph
         return ;
     }

     DoubleExitEdges changedEdges;

     for(DoubleExitEdges::iterator it = usedEdges.begin(); it != usedEdges.end(); ++it) {

         Device* srcDevice = subnet->getDevice(it->first);
         if(srcDevice == NULL) {
             LOG_INFO("Subnet " << std::hex << (int)subnet->getSubnetId() << ", Device " << std::hex << (int)it->first << " is null in evaluate!");
             continue;
         }
         if( srcDevice->statusForReports != StatusForReports::JOINED_AND_CONFIGURED) {
             LOG_INFO("Subnet " << std::hex << (int)subnet->getSubnetId() << ", Device " << std::hex << (int)it->first << " is not join confirmed in evaluate!");
             continue;
         }

         //if a node has changed one of its destinations(parent or backup), add it to changesEdges map
         if( subnet->nodeHasChangedDestinations(it->first, it->second, graphId)) {
             changedEdges.insert(std::make_pair(it->first, it->second));
             if(isInbound) {//if device has changed its destinations on the inbound graph=> outbound graphs that passes through that device must be reevaluated
                 onChangeParentsReevaluateGraphs(subnet, srcDevice, false);
                 if ( it->second.backup != 0) { // is called from physicalModelUpdate
                     onChangeParentForDevice(subnet, srcDevice);
                     LOG_INFO("Device " << Address_toStream(srcDevice->address32) << " changed inbound graph; reconfigure Publish...");
                 }
             }
         }
     }

     //update graph edges with usedEdges detected after current eval;uation
     graph->setGraphEdges(usedEdges);

     // changedEdges means new/updated edges
     // notUsedEdges means to delete edges
     theoreticModelUpdate(subnet, graph, changedEdges, notUsedEdges);

     physicalModelUpdate(operationsContainer, subnet,  graphId, changedEdges, notUsedEdges, isInbound, currentTime);

     LOG_INFO("AFTER GRAPH EVALUATION...GraphID=" << std::setw(3) << (int)graphId << ":" << NE::Model::Routing::GraphPrinter(subnet.get(), graph) );

}

void TheoreticEngine::createGraphG1(Subnet::PTR subnet, Address16 backboneAddress) {
    subnet->createGraph(DEFAULT_GRAPH_ID, backboneAddress);
}

void TheoreticEngine::addRouteToBeEvaluated(Uint16 subnetId, EntityIndex routeEntityIndex) {
    Subnet::PTR subnet = subnetsContainer->getSubnet(subnetId);
    subnet->routesToBeEvaluated.push_back(routeEntityIndex);
}

void TheoreticEngine::addGraphToGarbageCollection(Uint16 subnetId, Uint16 graphID) {
    Uint32 graphId32 = subnetId;
    graphId32 =(graphId32 << 16) + graphID;
    Subnet::PTR subnet = subnetsContainer->getSubnet(subnetId);
    subnet->garbageCollectionGraphs.insert(graphId32);
}

void TheoreticEngine::removeDevice(Address32 deviceToRemove, Subnet::PTR& subnet) {
    subnet->UpdateModelOnRemoveDevice(deviceToRemove);
}

void TheoreticEngine::redirectChildParent(Subnet::PTR& subnet, Device* removingDevice) {
    GraphPointer defaultGraph = subnet->getGraph(DEFAULT_GRAPH_ID);
    if(!defaultGraph) {
    	LOG_ERROR("Default graph not found (ID=" << DEFAULT_GRAPH_ID << ")");
        return ;
    }

    char reason[128];
    sprintf(reason, "redirectChhildren parent to backup, removingDevice=%x", removingDevice->address32);

    OperationsContainerPointer operationsContainer(new OperationsContainer(reason));

    NeighborIndexedAttribute::const_iterator itNeighbor = removingDevice->phyAttributes.neighborsTable.begin();
    for (; itNeighbor != removingDevice->phyAttributes.neighborsTable.end(); ++itNeighbor) {
        Device * device = subnet->getDevice( getIndex(itNeighbor->first) );
        if ((device ) && (device->parent32 == removingDevice->address32)){
            Address16 backup =  defaultGraph->getBackupFor(Address::getAddress16(device->address32));
            if(!backup) {
                continue;
            }
            Device* backupDevice = subnet->getDevice(backup);
            if(!backupDevice) {
                continue;
            }
            if(backupDevice->statusForReports != StatusForReports::JOINED_AND_CONFIGURED) {
                continue;
            }
            device->parent32 =backupDevice->address32;
        }
    }



    operationsProcessor->addOperationsContainer(operationsContainer);

}


bool  TheoreticEngine::detectNewParentForDevice(Subnet::PTR & subnet, Device * childDevice,  Device * avoidClockSourceDevice,
            Operations::OperationsContainerPointer & operationsContainer, bool isUrgentToChangeParent) {
    if(!childDevice) {
        return false;
    }

    Device* backbone = subnet->getBackbone();
    if(!backbone) {
        return false;
    }

    if(childDevice->statusForReports != StatusForReports::JOINED_AND_CONFIGURED) {
        return false;
    }

    GraphPointer defaultGraph = subnet->getGraph(DEFAULT_GRAPH_ID);
    if(!defaultGraph) {
    	LOG_ERROR("Default graph not found (ID=" << DEFAULT_GRAPH_ID << ")");
        return false;
    }


    //try to select backup as parent
    Address16 backupAddress = defaultGraph->getBackupFor(Address::getAddress16(childDevice->address32));
    Address32 backupAddress32 = Address::createAddress32(subnet->getSubnetId(), backupAddress);
    Device* newParent = subnet->getDevice(backupAddress);
    Device*  oldParent = subnet->getDevice(childDevice->parent32);
    bool hasPreferedClockSource = false;
    if(avoidClockSourceDevice && newParent) {
        subnet->checkPreferedClockSourceCycle(Address::getAddress16(newParent->address32), Address::getAddress16(avoidClockSourceDevice->address32), hasPreferedClockSource);
    }

    if(!backupAddress ) {
        newParent = 0;
    }

    if (newParent && newParent->statusForReports != StatusForReports::JOINED_AND_CONFIGURED) {
        newParent = 0;
    }

    if (( newParent && childDevice->theoAttributes.candidateIsBadRate(backupAddress) )
    		|| (hasPreferedClockSource == true)
    		|| (newParent && !isUrgentToChangeParent && subnet->getDeviceLevel(backupAddress32) > 1)) {
        newParent = 0;
    }

    if ( newParent && (subnet->deviceHasHighFailRateWithNeighbor(childDevice, newParent))) {
        newParent = 0;
    }

    //if backup was selected as new parent..but is not urgent for the device to change its parent, check if backup is on lower level..and with fewer children

    if(newParent && !isUrgentToChangeParent ) {
        if( subnet->getDeviceLevel(newParent->address32) > 1) {
            newParent = 0;
        }

        //change parent only if better cost
        if(newParent) {
            EdgePointer edgeToOldParent = subnet->getEdge(childDevice->address32, oldParent->address32);
            EdgePointer edgeToNewParent = subnet->getEdge(childDevice->address32, newParent->address32);
            if(edgeToOldParent && edgeToNewParent) {
                int costNewEdge = edgeToNewParent->getEvalEdgeCost(subnet->getSubnetSettings().k1factorOnEdgeCost, subnet->getNumberOfChildren(Address::getAddress16(newParent->address32)));
                int costOldEdge = edgeToOldParent->getEvalEdgeCost(subnet->getSubnetSettings().k1factorOnEdgeCost, subnet->getNumberOfChildren(Address::getAddress16(oldParent->address32)));
                if(costNewEdge > costOldEdge) {
                    newParent = 0;
                }
            }
        }
    }

    //backup not found..try to select a candidate as parent
    if(!newParent) {
        for(Address16 i = 0; i < DEFAULT_MAX_NO_CANDIDATES; ++i) {
            if(!childDevice->theoAttributes.candidates[i]) {
                continue;
            }
            Device* candidate = subnet->getDevice(childDevice->theoAttributes.candidates[i]);
            if(!candidate ) {
                continue;
            }
            if(candidate->statusForReports != StatusForReports::JOINED_AND_CONFIGURED) {
                continue;
            }

            if(!candidate->capabilities.isBackbone()
                        && !backbone->deviceHasOutboundGraph(candidate->address32)) {
                continue;
            }
            if(candidate->address32 ==  childDevice->parent32) { //if candidate is its old parent..skip it
                continue;
            }

            subnet->markAllDevicesUnVisited();

            if (subnet->checkFailRateOnNewPath(candidate)) {
                subnet->markAllDevicesUnVisited();
                continue;
            }

            subnet->markAllDevicesUnVisited();

            if ( subnet->deviceHasHighFailRateWithNeighbor(childDevice, candidate)) {
                continue;
            }

            if(!isUrgentToChangeParent ) {//if is not urgent to find new parent..get the best or leave it
                EdgePointer edgeToOldParent = subnet->getEdge(childDevice->address32, oldParent->address32);
                EdgePointer edgeToNewParent = subnet->getEdge(childDevice->address32, candidate->address32);

                if(edgeToOldParent && edgeToNewParent) {
                    int costNewEdge = edgeToNewParent->getEvalEdgeCost(subnet->getSubnetSettings().k1factorOnEdgeCost, subnet->getNumberOfChildren(Address::getAddress16(candidate->address32)));
                    int costOldEdge = edgeToOldParent->getEvalEdgeCost(subnet->getSubnetSettings().k1factorOnEdgeCost, subnet->getNumberOfChildren(Address::getAddress16(oldParent->address32)));
                    if(costNewEdge > costOldEdge) {
                        newParent = 0;
                        break;
                    }
                }

               if( subnet->getDeviceLevel(candidate->address32) > 1 ) {
                   continue;
               }
            }

            bool isPreferedClockSrcCycle = false;
            subnet->checkPreferedClockSourceCycle(childDevice->theoAttributes.candidates[i], Address::getAddress16(childDevice->address32) , isPreferedClockSrcCycle);
            hasPreferedClockSource = false;
            if(avoidClockSourceDevice) {
                subnet->checkPreferedClockSourceCycle(childDevice->theoAttributes.candidates[i], Address::getAddress16(avoidClockSourceDevice->address32), hasPreferedClockSource);
            }

            if (subnet->candidateIsEligibleAsNewParent(Address::getAddress16(childDevice->address32), childDevice->theoAttributes.candidates[i])
                        && !isPreferedClockSrcCycle
                        && !hasPreferedClockSource
                        && !childDevice->theoAttributes.candidateIsBadRate(Address::getAddress16(candidate->address32))
                        && !subnet->isBackupForDevice(Address::getAddress16(candidate->address32),Address::getAddress16(childDevice->address32))
                        && candidate->parent32 != childDevice->address32) {
                if(!newParent || (newParent &&  (subnet->getDeviceLevel(newParent->address32) > subnet->getDeviceLevel(candidate->address32)))) {
                    newParent = candidate;
                    break;
                }
            }
        } // for
    }

    if(newParent && oldParent){
        redirectDeviceToNewParent(subnet, childDevice, oldParent,  newParent, operationsContainer);
        return true;
    }

    return false;

}

void  TheoreticEngine::redirectDeviceToNewParent(Subnet::PTR & subnet, Device * device, Device * oldParent, Device * newParent,
            Operations::OperationsContainerPointer & operationsContainer) {

    if(!device || !newParent || !oldParent) {
        return;
    }

    GraphPointer defaultGraph = subnet->getGraph(DEFAULT_GRAPH_ID);
    if(!defaultGraph) {
    	LOG_ERROR("Default graph not found (ID=" << DEFAULT_GRAPH_ID << ")");
        return ;
    }
    Address16 const d16 = Address::getAddress16(device->address32);
    Address16 const np16 = Address::getAddress16(newParent->address32);
    OperationsList generatedOperations;

    if (defaultGraph->isBackupForDevice(d16, np16)) {
        Device * backbone = subnet->getBackbone();
        Uint16 const graphId = backbone->getOutBoundGraph(d16);
        removeDeviceFromOutBoundGraphs(subnet, backbone, oldParent, generatedOperations, graphId);
    }


    device->parent32 = newParent->address32;
    device->theoAttributes.deleteCandidate(Address::getAddress16(newParent->address32));
    newParent->theoAttributes.deleteCandidate(Address::getAddress16(device->address32));

    LOG_INFO("AddBadRateCandidate for device " << Address_toStream(device->address32)
				<< ", badRateCandidate=" << Address_toStream(oldParent->address32));
    subnet->addBadRateCandidate(device, oldParent);

    linkEngine->redirectChildParent(subnet, device, newParent, operationsContainer);

    DoubleExitDestinations oldDestinations;
    defaultGraph->getEdgesForDevice(Address::getAddress16(device->address32), oldDestinations);

    createNeighbors(subnet, operationsContainer, Address::getAddress16(device->address32),Address::getAddress16(newParent->address32), 2, true);
    createNeighbors(subnet, operationsContainer, Address::getAddress16(newParent->address32), Address::getAddress16(device->address32), 0, true );
    createNeighbors(subnet, operationsContainer, Address::getAddress16(device->address32),Address::getAddress16(oldParent->address32), 0, true);

    NE::Model::Operations::OperationsList generatedLinks;
    //if device is redirected to a candidate that was not backup before..delete links with its backup
    if(oldDestinations.backup
    		&& oldDestinations.backup != Address::getAddress16(newParent->address32)
			&& oldDestinations.backup != Address::getAddress16(oldParent->address32)) {

        if(subnet->getDevice(oldDestinations.backup ) && device->hasNeighbor(oldDestinations.backup)) {
            createNeighbors(subnet, operationsContainer, Address::getAddress16(device->address32),Address::getAddress16(oldParent->address32), 0, true);
            subnet->addDirtyEdge(Address::getAddress16(device->address32), oldDestinations.backup);
            subnet->deleteGraphFromEdge( Address::getAddress16(device->address32),  oldDestinations.backup, defaultGraph->getGraphId());
        }
    }

    onChangeParentForDevice(subnet, device);

    subnet->addTheoAttributesChild(Address::getAddress16(newParent->address32), Address::getAddress16(device->address32));
    subnet->deleteTheoAttributesChild(Address::getAddress16(oldParent->address32), device->address64);

    EdgePointer oldEdge = subnet->getEdge(Address::getAddress16(device->address32), Address::getAddress16(oldParent->address32));
    EdgePointer newEdge = subnet->getEdge(Address::getAddress16(device->address32), Address::getAddress16(newParent->address32));

    if(oldEdge) {
        oldEdge->resetEdgeDiagnosticsOnChangeParent();
    }
    if(newEdge) {
         newEdge->resetEdgeDiagnosticsOnChangeParent();
     }

    updateManagementLinksOnChangeParent( subnet, operationsContainer, device, newParent, oldParent, generatedLinks);

    DoubleExitDestinations destination;
    destination.prefered = Address::getAddress16(newParent->address32);
    defaultGraph->setDeviceDestination(Address::getAddress16(device->address32), destination);
    subnet->addGraphToEdge( Address::getAddress16(device->address32), Address::getAddress16(newParent->address32), defaultGraph->getGraphId());
    subnet->deleteGraphFromEdge( Address::getAddress16(device->address32), Address::getAddress16(oldParent->address32), defaultGraph->getGraphId());
    NE::Model::Operations::OperationsList generatedGraphOperations;
    createGraphOperation(subnet, operationsContainer, Address::getAddress16(device->address32), destination, defaultGraph->getGraphId(), generatedLinks, generatedGraphOperations);

    subnet->addDirtyEdge(Address::getAddress16(device->address32),Address::getAddress16(oldParent->address32));
    subnet->markServedContractsDirty(device);
    oldParent->decrementChildId();
    newParent->incrementChildId();

    LOG_INFO("Device " << Address_toStream(device->address32) << " has changed parent; oldParent="
                << Address_toStream(oldParent->address32) << ", newParent=" << Address_toStream(newParent->address32));

    onChangeParentsReevaluateGraphs(subnet, device, false);

    if(!generatedOperations.empty()) {
        for (OperationsList::iterator it = operationsContainer->getUnsentOperations().begin(); it != operationsContainer->getUnsentOperations().end(); ++it) {
            if ( Address::getAddress16((*it)->getOwner()) != ADDRESS16_GATEWAY
                    && Address::getAddress16((*it)->getOwner()) != ADDRESS16_MANAGER ) {
                (*it)->addOperationDependencies(generatedOperations);
            }
        }

        for(OperationsList::iterator itOper = generatedOperations.begin(); itOper != generatedOperations.end(); ++itOper) {
            Device* owner = subnet->getDevice((*itOper)->getOwner());
            if(owner) {
                operationsContainer->addOperation(*itOper, owner);
            }
            else {
                LOG_INFO("OWNER is null for operation=" << **itOper);
            }
        }
    }

}

void TheoreticEngine::retryToFindNewParent(Subnet::PTR & subnet, Device * device, Device * oldParent, Operations::OperationsContainerPointer & operationsContainer) {

    if (!device || !oldParent) {
        return;
    }

    if (!subnet->isEnabledInboundGraphEvaluation()){
        LOG_WARN("retryToFindNewParent and graph 1 is disabled");
        return;
    }

    NeighborIndexedAttribute::iterator itPhyNeighbor = device->phyAttributes.neighborsTable.begin();
    for (; itPhyNeighbor != device->phyAttributes.neighborsTable.end(); ++itPhyNeighbor) {
        PhyNeighbor * phyNeighbor = itPhyNeighbor->second.getValue();
        if (phyNeighbor == NULL){
            continue;
        }

        Device* neighbor = subnet->getDevice(getIndex(itPhyNeighbor->first));
        if(!neighbor) {
            continue;
        }

        if(neighbor->statusForReports != StatusForReports::JOINED_AND_CONFIGURED) {
            continue;
        }

        if(neighbor->address32 == device->parent32) {
            continue;
        }

        if(neighbor->parent32 != device->address32) {
            continue;

        }

        if ((neighbor->capabilities.isRouting() || neighbor->capabilities.isBackbone())
        		&& subnet->candidateIsEligibleAsNewParent(Address::getAddress16(device->address32), Address::getAddress16(neighbor->address32))
        		&& !device->theoAttributes.candidateIsBadRate(Address::getAddress16(neighbor->address32))) {

            //try to find
            char reason[128];
            sprintf(reason, "redirect device's parent to an old child, device=%x, neighbor=%x", device->address32, neighbor->address32);

            if (detectNewParentForDevice(subnet, neighbor, device, operationsContainer, true))  {
                return;
            }
        }
    }
}

ResponseStatus::ResponseStatusEnum TheoreticEngine::allocateApplicationTraffic(PhyContract * contract, Operations::OperationsContainerPointer& operationsContainer){
    Device * ownerDevice = NULL;
    PhyRoute * route = ModelUtils::findUsedRoute(ownerDevice, *subnetsContainer, contract->source32, contract->contractID, contract->destination32);
    if (route == NULL){
        LOG_ERROR("No PhyRoute found between " << Address_toStream(contract->source32) << " and " << Address_toStream(contract->destination32));
        return ResponseStatus::FAIL;
    }

    if(1 == route->alternative) {//created new contract route
        Device* publisher = subnetsContainer->getDevice(contract->source32);
        if(!publisher) {
            return ResponseStatus::FAIL;
        }

        EntityIndex indexRoute = createEntityIndex(publisher->address32, EntityType::Route, route->index);
        IEngineOperationPointer writeRoute(new Operations::WriteAttributeOperation(route, indexRoute));
        operationsContainer->addOperation(writeRoute, publisher);

    }

    ResponseStatus::ResponseStatusEnum resultStatus = linkEngine->allocateAppLinks(contract, route, operationsContainer);

    return resultStatus;
}

void TheoreticEngine::evaluateAppOutboundTraffic(
            Address32 destination32,
            Subnet::PTR& subnet,
            Operations::OperationsContainerPointer&  container){

    PhyRoute * route = ModelUtils::findOutboundRoute(subnet, destination32);
    Device* device = subnet->getDevice(destination32);
    if(!device || device->capabilities.isBackbone()) {
        return;
    }

    if (route == NULL){
        LOG_ERROR("No outbound PhyRoute found for " << Address_toStream(destination32));
        return;
    }

    linkEngine->evaluateAppOutboundLinks(destination32, subnet,  route,  container);
}

void TheoreticEngine::physicalModelUpdate(Operations::OperationsContainerPointer& operationsContainer,
                                            NE::Model::Subnet::PTR subnet,
                                            Uint16 graphID,
                                            DoubleExitEdges &usedEdges,
                                            DoubleExitEdges &notUsedEdges,
                                            bool isInBound,
                                            Uint32 currentTime) {
    std::ostringstream stream;
    stream << ", graphID=" << (int)graphID;
    stream << ", usedEdges={";
    for (DoubleExitEdges::iterator it = usedEdges.begin(); it != usedEdges.end(); ++it) {
        stream << "{" << std::hex << (int)it->first << ", " << it->second << "}, ";
    }
    stream << "}" << std::endl;
    stream << ", notUsedEdges={";
    for (DoubleExitEdges::iterator it = notUsedEdges.begin(); it != notUsedEdges.end(); ++it) {
        stream << "{" << std::hex << (int)it->first << ", " << it->second << "}, ";
    }
    stream << "}" << std::endl;
    LOG_INFO("physicalModelUpdate params=" << stream.str());

    //create management links on inbound
    NE::Model::Operations::OperationsList generatedLinks;
    NE::Model::Operations::OperationsList generatedGraphs;
    for (DoubleExitEdges::iterator it = usedEdges.begin(); it != usedEdges.end(); ++it) {
        updateNeighbors(subnet, operationsContainer, it->first, it->second, isInBound, false);

        if (0 != it->second.prefered) {
            updateManagementLinks(subnet, operationsContainer, it->first, it->second.prefered, isInBound, generatedLinks);
            if (isInBound) {
                subnet->addTheoAttributesChild(it->second.prefered, it->first);
            }
        }

        if (0 != it->second.backup) {
            updateManagementLinks(subnet, operationsContainer, it->first, it->second.backup, isInBound, generatedLinks);
            if (isInBound) {
                subnet->addTheoAttributesChild(it->second.backup, it->first);
            }
        }
        // sync the THEO model with PHY model
        createGraphOperation(subnet, operationsContainer, it->first, it->second, graphID, generatedLinks, generatedGraphs);
    }

    GraphPointer gp = subnet->getGraph(graphID);
    RETURN_ON_NULL_MSG(gp , "graph is null- ID=" << (int) graphID);
    //GraphPointer defaultGraph = subnet->getGraph(DEFAULT_GRAPH_ID);

    const DoubleExitEdges & graphEdges = gp->getGraphEdges();

    for (DoubleExitEdges::iterator it = notUsedEdges.begin(); it != notUsedEdges.end(); ++it) {

        // sync the THEO model with PHY model

            DoubleExitEdges::const_iterator iter = graphEdges.find( it->first );

            DoubleExitDestinations destination(0,0); // default, remove the graph from node
            if( iter != graphEdges.end() ) { // graph is present on node, upgrade it
                destination.prefered = iter->second.prefered;
                destination.backup   = iter->second.backup;
            }
            if( usedEdges.find( it->first ) == usedEdges.end() ) { // if was not sync by usedEdges

                createGraphOperation(subnet, operationsContainer, it->first, destination, graphID, generatedLinks, generatedGraphs);
            }

            if (isInBound) {//remove links only on INBOUND evaluation because On outbound evaluation the links may be used by other out graphs.

                updateNeighbors(subnet, operationsContainer, it->first, it->second, isInBound, true);

                if(it->second.prefered) {
                    subnet->addDirtyEdge(it->first, it->second.prefered);
                }

                if(it->second.backup) {
                    subnet->addDirtyEdge(it->first, it->second.backup);
                }
            }

            else {
                if(it->second.prefered) {
                    EdgePointer edge = subnet->getEdge(it->first, it->second.prefered);
                    EdgePointer revEdge = subnet->getEdge(it->second.prefered, it->first);
                    if(!edge || !revEdge) {
                        subnet->addDirtyEdge(it->first, it->second.prefered);
                    }
                    else {
                        if(!edge->existGraphOnEdge() && !revEdge->existGraphOnEdge() ) {
                            subnet->addDirtyEdge(it->first, it->second.prefered);
                          }
                        else {
                            edge->deleteGraph(graphID);
                        }

                    }
                }
                if(it->second.backup) {
                    EdgePointer edge = subnet->getEdge(it->first, it->second.backup);
                    EdgePointer revEdge = subnet->getEdge(it->second.backup, it->first);
                    if(!edge || !revEdge) {
                        subnet->addDirtyEdge(it->first, it->second.backup);
                    }
                    else {
                        if(!edge->existGraphOnEdge() && !revEdge->existGraphOnEdge() ) {
                            subnet->addDirtyEdge(it->first, it->second.backup);
                        }

                        else {
                            edge->deleteGraph(graphID);
                        }
                }
                }
            }

    }

    addDependencesOnDeleteLink(operationsContainer, generatedGraphs);

}


void TheoreticEngine::updateNeighbors(Subnet::PTR subnet,
                                      Operations::OperationsContainerPointer& operationsContainer,
                                      Address16 device,
                                      DoubleExitDestinations& destinations,
                                      bool isInBound,
                                      bool isDelete) {
	if (!isDelete) {
        if (isInBound) {
            createNeighbors(subnet, operationsContainer, device , destinations.prefered, 2, true);
            createNeighbors(subnet, operationsContainer, destinations.prefered, device, 0, true );
            if (destinations.backup != 0){
                createNeighbors(subnet, operationsContainer, device , destinations.backup, 1 /*1*/, true ); // clockSource = 1 means secondary clock support
                subnet->generateBackupPingOperation( operationsContainer.get(), device);
                createNeighbors(subnet, operationsContainer, destinations.backup, device, 0, true );
            }
        }
	} else {//don't delete on inbound neighbors..only disable clocksource
	    if (isInBound) {
	        if(subnet->existsConfirmedDevice(device) ) {
	            if(subnet->existsConfirmedDevice(destinations.prefered)) {
	                createNeighbors(subnet, operationsContainer,  device, destinations.prefered, 0, true );
	                createNeighbors(subnet, operationsContainer, destinations.prefered, device, 0, true );
	            }
                if (destinations.backup != 0 && subnet->existsConfirmedDevice(destinations.backup)){
                    createNeighbors(subnet, operationsContainer, device , destinations.backup, 0 /*1*/, true );
                    createNeighbors(subnet, operationsContainer, destinations.backup, device, 0, true );
                }
	        }
	    }
    }
}

void TheoreticEngine::createNeighbors(Subnet::PTR subnet,
                                      Operations::OperationsContainerPointer& operationsContainer,
                                      Address16 device,
                                      Address16 neighbor,
                                      Uint8 clockSource,
                                      bool isInBound) {
    Device* source = subnet->getDevice(device);
    RETURN_ON_NULL(source);
    Device* destination = subnet->getDevice(neighbor);
    RETURN_ON_NULL(destination);

    EntityIndex entityIndexNeighbor = createEntityIndex(source->address32, EntityType::Neighbour, neighbor);

    NeighborIndexedAttribute::iterator itNeighbor = source->phyAttributes.neighborsTable.find(entityIndexNeighbor);
    if(itNeighbor == source->phyAttributes.neighborsTable.end() ) {

        PhyNeighbor *phyNeighbor = new PhyNeighbor();
        phyNeighbor->index = neighbor;
        phyNeighbor->address64 = destination->address64;
        phyNeighbor->diagLevel = 0;
        phyNeighbor->extGrCnt = 0;
        phyNeighbor->groupCode = 0;
        phyNeighbor->linkBacklog = 0;
        phyNeighbor->linkBacklogDur = 0;
        phyNeighbor->linkBacklogIndex = 0;
        phyNeighbor->clockSource = clockSource;

        IEngineOperationPointer addNeighbor(new WriteAttributeOperation( phyNeighbor, entityIndexNeighbor));
        operationsContainer->addOperation(addNeighbor, source);
    } else if (isInBound && (itNeighbor->second.getValue() && itNeighbor->second.getValue()->clockSource != clockSource)){//neighbor exists & clocksource must be changed
    	PhyNeighbor *phyNeighbor = new PhyNeighbor(*itNeighbor->second.getValue());//copy it to preserve previous settings
		phyNeighbor->clockSource = clockSource;
		if(clockSource == 0) {
		    phyNeighbor->diagLevel = 0;
		}

		IEngineOperationPointer addNeighbor(new WriteAttributeOperation( phyNeighbor, entityIndexNeighbor));
		operationsContainer->addOperation(addNeighbor, source);
    }
}

void TheoreticEngine::deleteNeighbors(Subnet::PTR subnet,
                                      Operations::OperationsContainerPointer& operationsContainer,
                                      Address16 device,
                                      Address16 neighbor) {
    Device* source = subnet->getDevice(device);
    RETURN_ON_NULL(source);

    EntityIndex eiNeighbor = createEntityIndex(source->address32, EntityType::Neighbour, neighbor);

    NeighborIndexedAttribute::iterator itNeighbor = source->phyAttributes.neighborsTable.find(eiNeighbor);
    if(itNeighbor != source->phyAttributes.neighborsTable.end() ) {

        IEngineOperationPointer deleteNeighbor(new DeleteAttributeOperation(eiNeighbor));
        operationsContainer->addOperation(deleteNeighbor, source);
    }
}

void TheoreticEngine::updateManagementLinks(Subnet::PTR subnet, Operations::OperationsContainerPointer& operationsContainer,
            Address16 src16, Address16 dst16, bool isInBound, NE::Model::Operations::OperationsList& generatedLinks) {
     Device* srcDevice = subnet->getDevice(src16);
     Device* dstDevice = subnet->getDevice(dst16);

     if ( srcDevice && dstDevice ) {
         if ( isInBound ) {
            LOG_DEBUG("add inbound link " << std::hex << src16 << "-->" << dst16);
            linkEngine->addNewInboundEdge(subnet, *operationsContainer, srcDevice, dstDevice, generatedLinks);

            LOG_DEBUG("add outbound link " << std::hex << (int)dst16 << "-->" << (int)src16);
            linkEngine->addNewOutboundEdge(subnet, *operationsContainer, dstDevice, srcDevice,  generatedLinks);

         }
     }
}

void TheoreticEngine::updateManagementLinks(Subnet::PTR subnet, Operations::OperationsContainerPointer& operationsContainer,
               Device * source, Device * destination, Uint16 inboundLnkInterval, Uint16 outboundLnkInterval, NE::Model::Operations::OperationsList& generatedLinks) {
    if ( !source || !destination) {
        return;
    }

    bool existsInboundLinks = false;
    bool existsOutboundLinks = false;

    //check if there already exists inbound management links with new parent
    PhyLink * linkTxInbound = source->getTxMngLink( Address::getAddress16(destination->address32), Tdma::TdmaLinkDir::INBOUND);
    if( linkTxInbound ) { // pair link found -> increase the speed
        LOG_WARN("Already exist INB mng link between device:" << Address_toStream(source->address32)
                    << " -> parent:" << Address_toStream(destination->address32));
        if ( destination->capabilities.isBackbone())  {
            existsInboundLinks = true;
        }
        else {
			PhyLink * linkRxInbound = destination->getPeerRxLink( linkTxInbound );
			if( linkRxInbound  ) {
				if (linkTxInbound->schedule.interval != inboundLnkInterval) {
					linkEngine->updateMngLinksPeriod( source, *linkTxInbound, destination, *linkRxInbound, inboundLnkInterval, operationsContainer, generatedLinks );
				}
				existsInboundLinks = true;
			}
        }
    }


    //check if there already exists outbound management links with new parent
    PhyLink * linkTxOutbound = destination->getTxMngLink( Address::getAddress16(source->address32), Tdma::TdmaLinkDir::OUTBOUND);
    if( linkTxOutbound ) { // pair link found -> increase the speed
        LOG_WARN("Allready exist OUT mng link between parent:" << Address_toStream(destination->address32) << " -> dev:" << Address_toStream(source->address32));
        PhyLink * linkRxOutbound = source->getPeerRxLink( linkTxOutbound );
        if( linkRxOutbound ) {
            if ( linkTxOutbound->schedule.interval != outboundLnkInterval ) {
                linkEngine->updateMngLinksPeriod( destination, *linkTxOutbound, source, *linkRxOutbound, outboundLnkInterval, operationsContainer, generatedLinks );
            }
            existsOutboundLinks = true;
            if ( existsInboundLinks) {
                return;
            }
        }
    }

    //if here => there are nor management links between devices


    ManagementLinksUtils linksUtilsInbound;
    ManagementLinksUtils linksUtilsOutbound;

    if ( !existsInboundLinks && linkEngine->reserveMngChunkInbound(*operationsContainer, subnet, source, destination, false, linksUtilsInbound ) ) {
           linksUtilsInbound.period = inboundLnkInterval;
           linkEngine->addMngInboundLinks(source, destination, *operationsContainer, TdmaLinkTypes::MANAGEMENT, linksUtilsInbound, DEFAULT_MANAGEMENT_SUPERFRAME_ID, generatedLinks, subnet->getSubnetSettings());
    }

    if ( !existsOutboundLinks && linkEngine->reserveMngChunkOutbound(*operationsContainer,subnet, destination, source, false, linksUtilsOutbound.offset, linksUtilsOutbound ) ) {
        std::vector<IEngineOperationPointer> generatedOperations;
        linksUtilsOutbound.period = outboundLnkInterval;
        linkEngine->addMngOutboundLinks(destination, source , *operationsContainer, TdmaLinkTypes::MANAGEMENT, linksUtilsOutbound,  DEFAULT_MANAGEMENT_SUPERFRAME_ID, generatedOperations);
        if (generatedOperations.size() >= 2) {
            generatedLinks.push_back(generatedOperations[0]);
            generatedLinks.push_back(generatedOperations[1]);
        }
    }
}

void TheoreticEngine::updateManagementLinksOnChangeParent(Subnet::PTR subnet, Operations::OperationsContainerPointer& operationsContainer,
               Device * source, Device * destination, Device * oldDestination, NE::Model::Operations::OperationsList& generatedLinks) {

    if ( !source || !destination || !oldDestination) {
        return;
    }
    //check if there already exists inbound management links with new parent
    Uint16 newInboundPeriod ;
    Uint16 newOutboundPeriod;
    const SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    if (source->capabilities.isRouting()) {
          if( destination->capabilities.isBackbone() ) { // router to backbone -> allocate full slot
              newInboundPeriod = 1;
          } else { // router to router -> not full band to protect router battery
              newInboundPeriod = subnetSettings.mng_link_r_in;
          }

          newOutboundPeriod = subnetSettings.mng_link_r_out;
    } else { // non router to backbone or router
          newInboundPeriod= subnetSettings.mng_link_s_in;
          newOutboundPeriod =  subnetSettings.mng_link_s_out;
      }

    newInboundPeriod *= subnetSettings.getSlotsPerSec(); // transform to real offet
    newOutboundPeriod *= subnetSettings.getSlotsPerSec(); // transform to real offet

    PhyLink * linkTxInbound = source->getTxMngLink( Address::getAddress16(oldDestination->address32), Tdma::TdmaLinkDir::INBOUND);
    if( linkTxInbound && !destination->capabilities.isBackbone()  ) { // pair link found -> increase the speed
      newInboundPeriod = linkTxInbound->schedule.interval;
    }

    PhyLink * linkTxOutbound = oldDestination->getNotFullTxMngLink( Address::getAddress16(source->address32), Tdma::TdmaLinkDir::OUTBOUND);
    if( linkTxOutbound ) { // pair link found -> increase the speed
    	newOutboundPeriod = linkTxOutbound->schedule.interval;
    }


    updateManagementLinks( subnet, operationsContainer, source, destination, newInboundPeriod, newOutboundPeriod, generatedLinks) ;

}

void TheoreticEngine::theoreticModelUpdate(NE::Model::Subnet::PTR subnet, GraphPointer& graph, DoubleExitEdges &usedEdges, DoubleExitEdges &notUsedEdges ) {
//    GraphPointer graph = subnet->getGraph(graphID);
    for(DoubleExitEdges::iterator it = notUsedEdges.begin(); it != notUsedEdges.end(); ++it) {
		  subnet->deleteGraphFromEdge(it->first, it->second.prefered, graph->getGraphId());
		  subnet->deleteGraphFromEdge(it->first, it->second.backup, graph->getGraphId());
		  Address16List::iterator itExistsNode ;
		  itExistsNode = std::find( subnet->changedDevicesOnOutboundGraphs.begin(),  subnet->changedDevicesOnOutboundGraphs.end(), it->first);
          if(itExistsNode == subnet->changedDevicesOnOutboundGraphs.end() && graph->getGraphId() != DEFAULT_GRAPH_ID) {
              subnet->changedDevicesOnOutboundGraphs.push_back( it->first);
          }

		  if( it->second.prefered && graph->getGraphId() != DEFAULT_GRAPH_ID) {
		      itExistsNode = std::find( subnet->changedDevicesOnOutboundGraphs.begin(),  subnet->changedDevicesOnOutboundGraphs.end(), it->second.prefered);
		      if(itExistsNode == subnet->changedDevicesOnOutboundGraphs.end()) {
		          subnet->changedDevicesOnOutboundGraphs.push_back( it->second.prefered);
		      }
		  }
		  if(it->second.backup && graph->getGraphId() != DEFAULT_GRAPH_ID) {
	          itExistsNode = std::find( subnet->changedDevicesOnOutboundGraphs.begin(),  subnet->changedDevicesOnOutboundGraphs.end(), it->second.backup);
	              if(itExistsNode == subnet->changedDevicesOnOutboundGraphs.end()) {
	                  subnet->changedDevicesOnOutboundGraphs.push_back( it->second.backup);
	              }
		  }
      }

	for(DoubleExitEdges::iterator it = usedEdges.begin(); it != usedEdges.end(); ++it) {
		subnet->addGraphToEdge(it->first, it->second.prefered, graph->getGraphId());
		subnet->addGraphToEdge(it->first, it->second.backup, graph->getGraphId());
        Address16List::iterator itExistsNode ;
        itExistsNode = std::find( subnet->changedDevicesOnOutboundGraphs.begin(),  subnet->changedDevicesOnOutboundGraphs.end(), it->first);
        if(itExistsNode == subnet->changedDevicesOnOutboundGraphs.end() && graph->getGraphId() != DEFAULT_GRAPH_ID) {
            subnet->changedDevicesOnOutboundGraphs.push_back( it->first);
        }
        if( it->second.prefered && graph->getGraphId() != DEFAULT_GRAPH_ID) {
            itExistsNode = std::find( subnet->changedDevicesOnOutboundGraphs.begin(),  subnet->changedDevicesOnOutboundGraphs.end(), it->second.prefered);
                if(itExistsNode == subnet->changedDevicesOnOutboundGraphs.end()) {
                    subnet->changedDevicesOnOutboundGraphs.push_back( it->second.prefered);
                }
          }
          if(it->second.backup && graph->getGraphId() != DEFAULT_GRAPH_ID) {
              itExistsNode = std::find( subnet->changedDevicesOnOutboundGraphs.begin(),  subnet->changedDevicesOnOutboundGraphs.end(), it->second.backup);
                    if(itExistsNode == subnet->changedDevicesOnOutboundGraphs.end()) {
                        subnet->changedDevicesOnOutboundGraphs.push_back( it->second.backup);
                    }
          }
	}

}

void TheoreticEngine::populateDependencies( const DoubleExitDestinations &dependencies,
                                             NE::Model::Operations::OperationsContainerPointer& operationsContainter,
                                             NE::Model::Operations::OperationDependency &operationDependency,
                                             NE::Model::Operations::EntityDependency &entityDependency,
                                             Address16 deviceAddress16) {

    if ((dependencies.prefered != 0) && (dependencies.prefered != deviceAddress16)) {
        for (OperationsList::iterator it = operationsContainter->getUnsentOperations().begin(); it != operationsContainter->getUnsentOperations().end(); ++it) {
            Address16 owner16 = Address::getAddress16((*it)->getOwner());
            if ((owner16 == dependencies.prefered) && (EntityType::Graph == getEntityType((*it)->getEntityIndex()))) {
                operationDependency.push_back(*it);
                entityDependency.push_back((*it)->getEntityIndex());
            }

        }
    }

    if ((dependencies.backup != 0) && (dependencies.backup != deviceAddress16)) {
        for (OperationsList::iterator it = operationsContainter->getUnsentOperations().begin(); it != operationsContainter->getUnsentOperations().end(); ++it) {
            Address16 owner16 = Address::getAddress16((*it)->getOwner());
            if ((owner16 == dependencies.backup) && (EntityType::Graph == getEntityType((*it)->getEntityIndex()))) {
                operationDependency.push_back(*it);
                entityDependency.push_back((*it)->getEntityIndex());
            }
        }
    }

}

void TheoreticEngine::addDependencesOnDeleteLink( NE::Model::Operations::OperationsContainerPointer& operationsContainter,
                                             NE::Model::Operations::OperationDependency &operationDependency) {
    for (OperationsList::iterator it = operationsContainter->getUnsentOperations().begin(); it != operationsContainter->getUnsentOperations().end(); ++it) {
        if((*it)->getType() == EngineOperationType::DELETE_ATTRIBUTE) {
            (*it)->addOperationDependencies(operationDependency);
        }
    }
}

void TheoreticEngine::updateGraphOnJoin(Operations::OperationsContainerPointer& operationsContainer,
            NE::Model::Subnet::PTR subnet, Uint16 graphID , Address16 deviceAddress16, Address16 parentAddress16) {

    GraphPointer graph = subnet->getGraph(graphID);
    RETURN_ON_NULL(graph);

    DoubleExitEdges  usedEdges = graph->getGraphEdges();

    #warning Find a way to obtain the real list of dependent links
    NE::Model::Operations::OperationsList dependentLinks;

    for (DoubleExitEdges::iterator it = usedEdges.begin(); it != usedEdges.end(); it++) {
        LOG_DEBUG("create graph operation for usedDevice=" << std::hex << it->first);
        if(it->second.prefered) {
            NE::Model::Operations::OperationsList generatedGraphs;
            if(it->second.prefered) {
                subnet->addGraphToEdge(  it->first,  it->second.prefered, graphID);
            }
            if(it->second.backup) {
                subnet->addGraphToEdge(  it->first,  it->second.backup, graphID);
            }
            createGraphOperation(subnet, operationsContainer, it->first, it->second, graphID, dependentLinks,generatedGraphs );
        }
    }

    for (OperationsList::const_iterator it = operationsContainer->getUnsentOperations().begin(); it != operationsContainer->getUnsentOperations().end(); ++it) {
        DoubleExitEdges::const_iterator itEdge = usedEdges.find(Address::getAddress16((*it)->getOwner()));
        if ( itEdge != usedEdges.end()) { //
            OperationDependency operationDependency;
            EntityDependency entityDependency;

            populateDependencies( itEdge->second, operationsContainer, operationDependency, entityDependency, deviceAddress16);
            (*it)->addOperationDependencies(operationDependency);
            (*it)->addEntityDependencies(entityDependency);
        }
    }
}

void TheoreticEngine::getPathForOutboundGraph(Subnet::PTR & subnet, GraphPointer & graph, std::vector<Device*> & outboundPath) {

    DoubleExitEdges usedEdges = graph->getGraphEdges();

    if (!usedEdges.empty()) {
        Device *device = subnet->getDevice(usedEdges.begin()->first);
        RETURN_ON_NULL_MSG(device, "device not found! device=" << (int) usedEdges.begin()->first);

        outboundPath.push_back(device);
    }

    for (DoubleExitEdges::iterator it = usedEdges.begin(); it != usedEdges.end(); ++it) {
        Device *device = subnet->getDevice(it->second.prefered);
        RETURN_ON_NULL_MSG(device, "device not found! device=" << (int) it->second.prefered);

        outboundPath.push_back(device);
    }
}

void TheoreticEngine::getPathForInboundGraph(Subnet::PTR & subnet, Device * device, /*GraphPointer & graph,*/ std::vector<Device*> & inboundPath) {


    Address16 prefered = 0;
    Address16 secondary = 0;

    Device *preferedDevice = device;

    inboundPath.push_back(preferedDevice);

    while (preferedDevice && !preferedDevice->capabilities.isBackbone()) {

        device->getClockSourceNeighbors(prefered, secondary);
        preferedDevice = subnet->getDevice(prefered);
        if (!preferedDevice) {
            LOG_ERROR("Device not found! device=" << (int)prefered);
            continue;
        }
        inboundPath.push_back(preferedDevice);
    }

    // add backbone
    if (preferedDevice && preferedDevice->capabilities.isBackbone()) {
        inboundPath.push_back(preferedDevice);
    }
}

bool TheoreticEngine::allocateUdoLinksForOutboundGraph(Operations::OperationsContainerPointer& container,
            NE::Model::Subnet::PTR subnet, Uint16 graphID, EntityIndex eiContract) {

    GraphPointer graph = subnet->getGraph(graphID);
    if (!graph) {
        LOG_ERROR("Graph not found! graph=" << (int)graphID);
        return false;
    }

    Device * backbone = subnet->getBackbone();
    if (!backbone) {
        LOG_ERROR("Backbone NULL!");
        return false;
    }

    DoubleExitEdges usedEdges = graph->getGraphEdges();

    ManagementLinksUtils linksUtils;

    for (DoubleExitEdges::iterator itEdge = usedEdges.find(Address::getAddress16(backbone->address32)); itEdge != usedEdges.end(); ) {
        Device * parent = subnet->getDevice(itEdge->first);
        if (parent == NULL){
            LOG_ERROR("Parent null on udo band: " << Address_toStream(itEdge->first));
            itEdge = usedEdges.find(itEdge->second.prefered);
            continue;//alocate at least for other devices
        }
        Device * device = subnet->getDevice(itEdge->second.prefered);
        if (device == NULL){
            LOG_ERROR("Prefered null on udo band: " << Address_toStream(itEdge->second.prefered));
            itEdge = usedEdges.find(itEdge->second.prefered);
            continue;//alocate at least for other devices
        }

        PhyLink * linkTxParent = parent->getFirstTxMngLink(Address::getAddress16(device->address32), TdmaLinkDir::OUTBOUND, Tdma::TdmaLinkTypes::UDO_FIRMWARE);
        if (linkTxParent) {//if allready exist a link do not reserve more, it have enough
            //needed because reserve is called from EvaluateGraph also when nothing changed on edges, and cause to reserve up to max chunks limit.
            LOG_WARN("Allready exist OUT mng link between parent:" << Address_toStream(parent->address32) << " -> dev:" << Address_toStream(device->address32));

        } else if (linkEngine->reserveMngChunkOutbound(*container, subnet, parent, device, true, linksUtils.offset, linksUtils)) { //reserve inbound mng chunk
            std::vector<IEngineOperationPointer> generatedOperations;
            linkEngine->addMngOutboundLinks(parent, device, *container,
                        TdmaLinkTypes::UDO_FIRMWARE, linksUtils, DEFAULT_MANAGEMENT_SUPERFRAME_ID, generatedOperations);

#warning Modify access to generatedOperations with brackets: is based on order of operations in vector.
            //add links in mappings from device's theo attributes
            parent->theoAttributes.link2UdoContract[generatedOperations[1]->getEntityIndex()] = eiContract;
            device->theoAttributes.link2UdoContract[generatedOperations[0]->getEntityIndex()] = eiContract;

            subnet->getManager()->theoAttributes.udoContract2Links[eiContract].push_back(generatedOperations[1]->getEntityIndex());
            subnet->getManager()->theoAttributes.udoContract2Links[eiContract].push_back(generatedOperations[0]->getEntityIndex());
        } else {
            return false;
        }

        itEdge = usedEdges.find(itEdge->second.prefered);
    }
    return true;
}

bool TheoreticEngine::allocateUdoLinksForInboundGraph(Operations::OperationsContainerPointer& container,
            NE::Model::Subnet::PTR subnet, Device * device, EntityIndex eiContract /* , Uint16 graphID, Address16 deviceAddress16*/) {

    GraphPointer graph = subnet->getGraph(DEFAULT_GRAPH_ID);
    if (!graph) {
        LOG_ERROR("Graph not found! graph=" << (int)DEFAULT_GRAPH_ID);
        return false;
    }

    const NE::Common::SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    DoubleExitEdges usedEdges = graph->getGraphEdges();

    ManagementLinksUtils linksUtils;


//    for (std::size_t i = 0; (inboundPath.size() > 0) && (i < inboundPath.size() - 1); ++i) {
    for (DoubleExitEdges::iterator itEdge = usedEdges.find(Address::getAddress16(device->address32)); itEdge != usedEdges.end(); ) {
        Device * device = subnet->getDevice(itEdge->first);
        if (device == NULL) {
            LOG_ERROR("Device null on udo band: " << Address_toStream(itEdge->first));
            itEdge = usedEdges.find(itEdge->second.prefered);
            continue;//alocate at least for other devices
        }
        Device * parent = subnet->getDevice(itEdge->second.prefered);
        if (parent == NULL) {
            LOG_ERROR("Prefered null on udo band: " << Address_toStream(itEdge->second.prefered));
            itEdge = usedEdges.find(itEdge->second.prefered);
            continue;//alocate at least for other devices
        }


        PhyLink * linkTxDevice = device->getFirstTxMngLink(Address::getAddress16(parent->address32), TdmaLinkDir::INBOUND, Tdma::TdmaLinkTypes::UDO_FIRMWARE);
        if (linkTxDevice) {//if allready exist a link do not reserve more, it have enough
            //needed because reserve is called from EvaluateGraph also when nothing changed on edges, and cause to reserve up to max chunks limit.
            LOG_WARN("Allready exist INB mng link between parent:" << Address_toStream(device->address32) << " -> dev:" << Address_toStream(parent->address32));

        } else if (linkEngine->reserveMngChunkInbound(*container, subnet, device, parent, true, linksUtils)) { //reserve inbound mng chunk
            OperationsList generatedLinks;

            NE::Model::Operations::IEngineOperationPointer inboundUdolink =
                linkEngine->addMngInboundLinks(device, parent, *container,
                            TdmaLinkTypes::UDO_FIRMWARE, linksUtils, DEFAULT_MANAGEMENT_SUPERFRAME_ID, generatedLinks, subnetSettings);
            LOG_DEBUG("GeneratedLinks: " << generatedLinks.size());
            //add links in mappings from device's theo attributes
            if (parent->capabilities.isBackbone()){
                device->theoAttributes.link2UdoContract[(*generatedLinks.begin())->getEntityIndex()] = eiContract;
                subnet->getManager()->theoAttributes.udoContract2Links[eiContract].push_back((*generatedLinks.begin())->getEntityIndex());
            } else {
                parent->theoAttributes.link2UdoContract[(*generatedLinks.begin())->getEntityIndex()] = eiContract;
                device->theoAttributes.link2UdoContract[(*generatedLinks.rbegin())->getEntityIndex()] = eiContract;

                subnet->getManager()->theoAttributes.udoContract2Links[eiContract].push_back((*generatedLinks.rbegin())->getEntityIndex());
                subnet->getManager()->theoAttributes.udoContract2Links[eiContract].push_back((*generatedLinks.begin())->getEntityIndex());
            }
        } else {
            return false;
        }
        itEdge = usedEdges.find(itEdge->second.prefered);
    }
    return true;
}

void TheoreticEngine::periodicConfigureNeighborDiscovery(Uint32 currentTime, Subnet::PTR& subnet){
    //This task is called periodicaly so:
    // - in one call only one device will have neighbor discovery configured
    // - in one call if device does not exist will be removed and no other device will be checked

    for (Address32Set::iterator it = subnet->noNeighborDiscoveryActivatedDevices.begin(); it != subnet->noNeighborDiscoveryActivatedDevices.end(); ++it){
        Device * device = subnet->getDevice(*it);
        if(device == NULL){
            subnet->noNeighborDiscoveryActivatedDevices.erase(it);
            break;
        }

        if ( (currentTime - subnet->getSubnetSettings().delayConfigureNeighborDiscovery) >= device->joinConfirmTime){
            char reason[128];
                sprintf(reason, "START Neighbor discovery on device %x", *it);
                Operations::OperationsContainerPointer operationsContainer(
                            new Operations::OperationsContainer(reason));

            linkEngine->allocateNeighborDiscoveryLinks(device, operationsContainer, subnet);
            operationsProcessor->addOperationsContainer(operationsContainer);
            subnet->noNeighborDiscoveryActivatedDevices.erase(it);
            break;
        }

    }
}


void TheoreticEngine::createGraphOperation(Subnet::PTR subnet,
                                           Operations::OperationsContainerPointer& operationsContainer,
                                           Address16 src, DoubleExitDestinations &destination,
                                           Uint16 graphID, NE::Model::Operations::OperationsList& dependOnLinksList
                                           ,  NE::Model::Operations::OperationsList& generatedGraphOperations) {
    Device * ownerDevice = subnet->getDevice(src);
    if (ownerDevice == NULL) {
        LOG_WARN("createGraphOperation for inexistent device " << std::hex << src << ", subnet " << (int)subnet->getSubnetId());
        return;
    }
    EntityIndex entityIndexGraph  = createEntityIndex(ownerDevice->address32, EntityType::Graph, graphID);



#warning "TO DO: check if the PHY model already is on sync (maybe on PHY model check) to reduce the traffic"
    if( destination.prefered && (ownerDevice->statusForReports ==  StatusForReports::JOINED_AND_CONFIGURED) ) { // update/add operation
        PhyGraph* graph = new PhyGraph();
        graph->index = graphID;

        if(destination.prefered) {
            graph->neighbors.push_back(destination.prefered);
        }

        if(destination.backup) {
            graph->neighbors.push_back(destination.backup);
        }


        graph->neighborCount = graph->neighbors.size();
        graph->preferredBranch = 1;
        graph->maxLifetime = 0;
        graph->queue = 15;

        IEngineOperationPointer updateGraphOperation(new Operations::WriteAttributeOperation(graph, entityIndexGraph));
        const OperationDependency::iterator& lastDependecy = updateGraphOperation->getOperationDependency().end();
        updateGraphOperation->getOperationDependency().insert(lastDependecy, dependOnLinksList.begin(), dependOnLinksList.end());
        operationsContainer->addOperation(updateGraphOperation, ownerDevice);
        generatedGraphOperations.push_back(updateGraphOperation);
    }
    else { // delete graph operation
        if(ownerDevice->statusForReports == StatusForReports::JOINED_AND_CONFIGURED) {
            IEngineOperationPointer deleteGraphOperation(new Operations::DeleteAttributeOperation(entityIndexGraph));
            // !!! rejoin Dev; add outbound graph X to graphsToBeEvaluated; confirm device; evaluate graph X

            if (ownerDevice->phyAttributes.graphsTable.find(entityIndexGraph) != ownerDevice->phyAttributes.graphsTable.end()) {
                operationsContainer->addOperation(deleteGraphOperation, ownerDevice);
            }
        }
    }
}

void TheoreticEngine::deleteGraph(Subnet::PTR subnet, Uint16 graphId) {
    Device* backbone = subnet->getBackbone();
    if(!backbone) {
        return;
    }

    char reason[64];
    sprintf(reason, "remove  Graph: %d", graphId);
    LOG_INFO(reason);
    OperationsContainerPointer operationsContainer(new OperationsContainer(reason));
    EntityIndex index = createEntityIndex(backbone->address32, EntityType::Graph, graphId);
    DeleteAttributeOperationPointer deleteGraph(new DeleteAttributeOperation(index));
    operationsContainer->addOperation(deleteGraph, backbone);

    GraphPointer graph = subnet->getGraph(graphId);
    if(graph) {
        DoubleExitEdges graphEdges = graph->getGraphEdges();
        for(DoubleExitEdges::iterator it = graphEdges.begin(); it != graphEdges.end(); ++it) {
            Device* device = subnet->getDevice(it->first);
            if((device) && (device->address32 != backbone->address32)) {
                index = createEntityIndex(device->address32, EntityType::Graph, graphId);
                deleteGraph.reset(new DeleteAttributeOperation(index));
                operationsContainer->addOperation(deleteGraph, device);
            }

            if (it->second.prefered) {
            	subnet->deleteGraphFromEdge(it->first, it->second.prefered, graphId);
            }
            if (it->second.backup) {
            	subnet->deleteGraphFromEdge(it->first, it->second.backup, graphId);
            }
        }
    }
    subnet->deleteGraph(graphId);
    operationsProcessor->addOperationsContainer(operationsContainer);


}

void TheoreticEngine::removeDeviceRelyingOnOutboundGraph(Device *backbone, Uint16 graphId){

    RouteIndexedAttribute::iterator itRoute = backbone->phyAttributes.routesTable.begin();
    for (; itRoute !=  backbone->phyAttributes.routesTable.end(); ++itRoute) {
        PhyRoute* phyRoute = (PhyRoute*) itRoute->second.getValue();
        if(!phyRoute) {
            continue;
        }
        if (phyRoute->alternative == 2 ) {
            if (phyRoute->route.size() == 2 && isRouteGraphElement(phyRoute->route[0])) {//mark the graph for verification
                if (graphId == getRouteElement(phyRoute->route[0])) {
                    Address32 addressToRemove = NE::Common::Address::createAddress32(backbone->capabilities.dllSubnetId, phyRoute->selector);
                    printf("Remove device %x for removed graph %d.", addressToRemove, graphId);
                    char reason[64];
                    sprintf(reason, "Remove device %x for removed graph %d.", addressToRemove, graphId);
                    Subnet::PTR subnet = subnetsContainer->getSubnet(backbone->address32);

                    OperationsContainerPointer container(new OperationsContainer(reason));

                    devicesRemover->removeDeviceOnError(addressToRemove, container, RemoveDeviceReason::parentLeft);
                    operationsProcessor->addOperationsContainer(container);
                }
            }
        }
    }
}


void TheoreticEngine::deleteRouteFromBackbone(Subnet::PTR subnet, Uint16 graphId, Address16 destination) {
    Device* backboneDevice = subnet->getBackbone();
    RETURN_ON_NULL_MSG(backboneDevice, "backbone is NULL");

    char reason[128];
    sprintf(reason, "periodic Graphs Evaluation: %d..graph is empty after evaluation", (int)graphId);
    OperationsContainerPointer operationsContainer(new OperationsContainer(reason));

    RouteIndexedAttribute::iterator itRoute = backboneDevice->phyAttributes.routesTable.begin();
    for (; itRoute != backboneDevice->phyAttributes.routesTable.end(); ++itRoute) {
        PhyRoute* phyRoute = (PhyRoute*) itRoute->second.getValue();
        if(!phyRoute) {
            continue;
        }

        if (phyRoute->alternative == 2 && phyRoute->selector == destination) {
            if (phyRoute->route.size() == 1 && isRouteGraphElement(phyRoute->route[0]) && (getRouteElement(phyRoute->route[0]) == graphId)) {//mark the graph for verification
                DeleteAttributeOperationPointer deleteRoute(new DeleteAttributeOperation(itRoute->first));
                operationsContainer->addOperation(deleteRoute, backboneDevice);
                operationsProcessor->addOperationsContainer(operationsContainer);

                deleteGraph(subnet, graphId);
                break;
            }
        }
    }
}

bool TheoreticEngine::periodicDirtyLinksRemoval(Subnet::PTR subnet, Uint32 currentTime) {
    if (subnet->dirtyLinks.empty()){
        return false;
    }

    OperationsContainerPointer container;//(new OperationsContainer(reason));
    int numberOfDeletesGenerated = 0;

    EntityIndexList::iterator itLink = subnet->dirtyLinks.begin();
    for (; itLink != subnet->dirtyLinks.end(); ++itLink){
        EntityIndex indexLink = *itLink;
        Device * ownerOfLink = subnet->getDevice(getDeviceAddress(indexLink));

        if(ownerOfLink == NULL){
            LOG_INFO("Erase dirtyLink " << std::hex << *itLink);
            itLink = subnet->dirtyLinks.erase(itLink);
            continue;
        }
        bool isRouter = ownerOfLink->capabilities.isBackbone() || ownerOfLink->capabilities.isRouting();
        Uint16 communication = isRouter ? subnet->getSubnetSettings().mng_r_out_band : subnet->getSubnetSettings().mng_s_out_band ;
        if (ownerOfLink->wasCommunicationInLastInterval(communication, currentTime)){
            continue;
        }

        bool isPendingStatus = false;
        PhyLink * link = ownerOfLink->getPhyLink(indexLink, isPendingStatus);
        if (link == NULL){
            LOG_INFO("Erase dirtyLink " << std::hex << *itLink);
            itLink = subnet->dirtyLinks.erase(itLink);
            continue;
        }

        if (isPendingStatus){//if operation is on pending on this link.. delay the deletion..maybe will not be necessary next time
            continue;
        }

        if (container == NULL){//creation when needed
            container.reset(new OperationsContainer("Remove dirty links"));
        }

        numberOfDeletesGenerated++;

        //remove the current link
        DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(indexLink));
        container->addOperation(deleteLink, ownerOfLink);

        //unreserve and remove only the receive link on the peer device
        subnet->unreserveLink(ownerOfLink, link, container.get());

        if (link->role == Tdma::TdmaLinkTypes::APPLICATION){
            ownerOfLink->theoAttributes.linkToContract.erase(indexLink);
        }

        LOG_INFO("Erase dirtyLink " << std::hex << *itLink);
        itLink = subnet->dirtyLinks.erase(itLink);

        if (numberOfDeletesGenerated >= 10){
            break;
        }
    }

    if (container != NULL ){
        operationsProcessor->addOperationsContainer(container);
        return true;
    }

    return false;
}

void TheoreticEngine::periodicDirtyContractsEvaluation(Subnet::PTR subnet){

	if (subnet->contractsToBeEvaluated.empty()){
		return;
	}

	if (!subnet->isEnabledInboundGraphEvaluation()) {
	    //if evaluation of default graph is in progress skip reevalution of dirty contracts. In order to base evaluation on unstable data
	    return;
	}

	EntityIndexList::iterator itContractIndex = subnet->contractsToBeEvaluated.begin();
	for (; itContractIndex != subnet->contractsToBeEvaluated.end();){
		Device * ownerOfContract = subnet->getDevice(getDeviceAddress(*itContractIndex));

		if (ownerOfContract == NULL ) {
		    LOG_WARN("Dirty contract owner not found. entity:" << std::hex << *itContractIndex);
		    itContractIndex = subnet->contractsToBeEvaluated.erase(itContractIndex);
		    continue;
		}

        ContractToLinksMap::iterator itContractToLink = ownerOfContract->theoAttributes.contractLinks.find(*itContractIndex);
        if(itContractToLink != ownerOfContract->theoAttributes.contractLinks.end()){
            subnet->moveToDirtyLinks(itContractToLink->second);
        }

        bool isContractPending = false;
        //create a new links allocation for the contract
        PhyContract * contract = ownerOfContract->getPhyContract(*itContractIndex, isContractPending);
        if (contract == NULL){
            LOG_WARN("Dirty contract not found: " << std::hex << *itContractIndex);
            itContractIndex = subnet->contractsToBeEvaluated.erase(itContractIndex);
            continue;
        }

        if (isContractPending) {
            LOG_WARN("Dirty contract pending skipped: " << std::hex << *itContractIndex);
            itContractIndex++;
            continue;
        }

		// reevaluate local loop contract only if oubound graph for device is already evaluated
		Device * contractDestinationDevice = subnet->getDevice(contract->destination32);
		if (!contractDestinationDevice) {
			LOG_WARN("Dirty contract -  contract destination not found. entity: " << std::hex << *itContractIndex);
			itContractIndex = subnet->contractsToBeEvaluated.erase(itContractIndex);
			continue;
		}

		// local loop contract
		if (contractDestinationDevice->capabilities.isDevice()) {
			Device * backbone = subnet->getBackbone();
			if (!backbone) {
				LOG_WARN("Dirty udo contract -  backbone not found. entity: " << std::hex << *itContractIndex);
				itContractIndex = subnet->udoContractsToBeEvaluated.erase(itContractIndex);
				continue;
			}


			Uint16 outBoundGraph = backbone->getOutBoundGraph(Address::getAddress16(contract->destination32));
			GraphsList::iterator outBoundGraphIt = std::find(subnet->graphsToBeEvaluated.begin(), subnet->graphsToBeEvaluated.end(), outBoundGraph);
			if (outBoundGraphIt != subnet->graphsToBeEvaluated.end()) {
				LOG_INFO("Skip evaluate contract " << std::hex << *itContractIndex << ". Outbound graph " << outBoundGraph << " must be evaluated before evaluate contract.");
				++itContractIndex;
				continue;
			}
		}

        char reason[128];
        sprintf(reason, "Evaluation of dirty contract %x of device %x", getIndex(*itContractIndex), getDeviceAddress(*itContractIndex));
        ChainForceReevalContractOnFailPointer chainReevalContract(new ChainForceReevalContractOnFail(subnet, *itContractIndex));
        HandlerResponse handlerChainReevalContract = boost::bind(&ChainForceReevalContractOnFail::process, chainReevalContract, _1, _2, _3);
        OperationsContainerPointer container(new OperationsContainer((Address32)ADDRESS16_MANAGER, 0, handlerChainReevalContract, reason));

//#warning tre decomentat   re-alocare pentru un contract
        ResponseStatus::ResponseStatusEnum response = allocateApplicationTraffic(contract, container);
        if ( ResponseStatus::SUCCESS != response ) {
            LOG_WARN("Alloc for app returned: " << response);
            container->setAsFail( subnetsContainer );
        }
        else {
            operationsProcessor->addOperationsContainer(container);
        }

		itContractIndex = subnet->contractsToBeEvaluated.erase(itContractIndex);
		return;//do evaluation only for one contract a time
	}
}

void TheoreticEngine::periodicUdoContractsEvaluation(Subnet::PTR subnet) {

	if (subnet->udoContractsToBeEvaluated.empty()) {
        return;
    }

    for (EntityIndexList::iterator itContractIndex = subnet->udoContractsToBeEvaluated.begin();
            itContractIndex != subnet->udoContractsToBeEvaluated.end();) {

        LOG_INFO("periodicUdoContractsEvaluation eiContract=" << std::hex << *itContractIndex);

        Device * ownerOfContract = subnet->getDevice(getDeviceAddress(*itContractIndex));
        if (!ownerOfContract) {
            LOG_WARN("Dirty udo contract owner not found. entity: " << std::hex << *itContractIndex);
            itContractIndex = subnet->udoContractsToBeEvaluated.erase(itContractIndex);
            continue;
        }

        bool isContractPending = false;
        //create a new links allocation for the contract
        PhyContract * contract = ownerOfContract->getPhyContract(*itContractIndex, isContractPending);
        if (!contract) {
            LOG_WARN("Dirty udo contract not found: " << std::hex << *itContractIndex);
            itContractIndex = subnet->udoContractsToBeEvaluated.erase(itContractIndex);
            continue;
        }

		// reevaluate udo contract only if oubound graph for device is already evaluated
		Device * backbone = subnet->getBackbone();
        if (!backbone) {
            LOG_WARN("Dirty udo contract -  backbone not found. entity: " << std::hex << *itContractIndex);
            itContractIndex = subnet->udoContractsToBeEvaluated.erase(itContractIndex);
            continue;
        }

        Uint16 outBoundGraph = backbone->getOutBoundGraph(Address::getAddress16(contract->destination32));
        GraphsList::iterator outBoundGraphIt = std::find(subnet->graphsToBeEvaluated.begin(), subnet->graphsToBeEvaluated.end(), outBoundGraph);
        if (outBoundGraphIt != subnet->graphsToBeEvaluated.end()) {
            LOG_INFO("Skip evaluate udo contract " << std::hex << *itContractIndex << ". Outbound graph " << outBoundGraph << " must be evaluated before evaluate udo contract.");
            ++itContractIndex;
        	continue;
        }


        EntityIndex eiContract = *itContractIndex;
        subnet->udoContractsToBeEvaluated.erase(itContractIndex);

        updateLinksForUdoContract(subnet, eiContract, contract->destination32, false);

        return; //one contract at a time
    }
}

void TheoreticEngine::periodicClientServerContractsEvaluation(Subnet::PTR subnet) {

	if (subnet->changedDevicesOnOutboundGraphs.empty()) {
        return;
    }

    for (Address16List::iterator itChangedDevice = subnet->changedDevicesOnOutboundGraphs.begin();
		itChangedDevice != subnet->changedDevicesOnOutboundGraphs.end();) {

		Device * device = subnet->getDevice(*itChangedDevice);

		if (!device) {
			subnet->changedDevicesOnOutboundGraphs.erase(itChangedDevice);
			return;
		}

		// reevaluate ClientServer contract only if oubound graph for device is already evaluated
		Device * backbone = subnet->getBackbone();
		if (!backbone) {
			subnet->changedDevicesOnOutboundGraphs.erase(itChangedDevice);
			return;
		}

		Uint16 outBoundGraph = backbone->getOutBoundGraph(Address::getAddress16(device->address32));
		GraphsList::iterator outBoundGraphIt = std::find(subnet->graphsToBeEvaluated.begin(), subnet->graphsToBeEvaluated.end(), outBoundGraph);
		if (outBoundGraphIt != subnet->graphsToBeEvaluated.end()) {
			LOG_INFO("Skip evaluate ClientServer contract for device " << Address_toStream(device->address32) << ". Outbound graph must be evaluated before evaluate ClientServer contract.");
			++itChangedDevice;
			continue;
		}

		subnet->changedDevicesOnOutboundGraphs.erase(itChangedDevice);

		char reason[128];
		sprintf(reason, "Dirty C/S contract eval, device=%x", device->address32);
		LOG_INFO(reason);
		OperationsContainerPointer operationsContainer(new OperationsContainer(reason));
		evaluateAppOutboundTraffic(device->address32, subnet, operationsContainer);
		operationsProcessor->addOperationsContainer(operationsContainer);

		return;
    }
}

void TheoreticEngine::updateLinksForUdoContract(Subnet::PTR & subnet, EntityIndex & eiContract, Address32 contractDestination, bool isTerminateContract) {

    ContractToLinksMap::iterator itContract2Links = subnet->getManager()->theoAttributes.udoContract2Links.find(eiContract);
    if (itContract2Links == subnet->getManager()->theoAttributes.udoContract2Links.end()) {
        LOG_WARN("Dirty udo contract not in udoContract2Links: " << std::hex << eiContract);
        return;
    }

    char reason[128];
    sprintf(reason, "Delete links for udo contract %x; device %x", getIndex(eiContract), getDeviceAddress(eiContract));
    HandlerResponseList handlersList;
    if (!isTerminateContract){
        ChainCreateUdoLinksPointer chainCreateUdoLinks(new ChainCreateUdoLinks(subnet->getSubnetId(), eiContract, contractDestination, this));
        handlersList.push_back( boost::bind(&ChainCreateUdoLinks::process, chainCreateUdoLinks, _1, _2, _3) );
    }
    OperationsContainerPointer container(new OperationsContainer(0, 0, handlersList, reason));

    // delete all links for current evaluating contract
    for (LinksList::iterator itLinks = itContract2Links->second.begin(); itLinks != itContract2Links->second.end(); ++itLinks) {
        Device *linkOwner = subnet->getDevice(getDeviceAddress(*itLinks));
        if (!linkOwner || linkOwner->status != DeviceStatus::JOIN_CONFIRMED) {
            LOG_INFO("Skip deletion of link " << *itLinks << "; device not joined.");
            continue;
        }

        LinkIndexedAttribute::iterator itPhyLink = linkOwner->phyAttributes.linksTable.find(*itLinks);
        if (itPhyLink != linkOwner->phyAttributes.linksTable.end()){
            PhyLink * link = itPhyLink->second.getValue();
            if (link){
                subnet->unreserveLink(linkOwner, link);
            }

            DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(*itLinks));
            container->addOperation(deleteLink, linkOwner);
        }

        //remove mappings from device's theo attributes
        linkOwner->theoAttributes.link2UdoContract.erase(*itLinks);
    }

    //remove links from SM mapping of dirty udo contract
    itContract2Links->second.clear();

    operationsProcessor->addOperationsContainer(container);
}

void TheoreticEngine::findBetterParentForDevices(Subnet::PTR subnet, Uint32 currentTime) {

    LOG_DEBUG("findBetterParentForDevices");

    const Address16Set& activeDevices = subnet->getActiveDevices();

    Device* backbone = subnet->getBackbone();
    if(!backbone) {
        return;
    }

    if(!subnet->isEnabledInboundGraphEvaluation()) {
        return;
    }

    char reason[128];
    sprintf(reason, "findBetterParentForDevice");
    OperationsContainerPointer operationsContainer(new OperationsContainer(reason));

    Device * device = NULL;
    Device * parent = NULL;
    for(Address16Set::iterator itDev = activeDevices.begin(); itDev != activeDevices.end(); ++itDev) {
        device = subnet->getDevice(*itDev);
        if(!device) {
            continue;
        }
        if(device->statusForReports != StatusForReports::JOINED_AND_CONFIGURED) {
            continue;
        }

        if(!backbone->deviceHasOutboundGraph(device->address32)) {
            continue;
        }
        Int8 deviceLevel = subnet->getDeviceLevel(device->address32);
        parent = subnet->getDevice(device->parent32);
        if(!parent) {
            continue;
        }

        if (device->wasCommunicationInLastInterval(subnet->getSubnetSettings().deviceCommandsSilenceInterval, currentTime) ) {
            continue;
        }

        if (parent &&  parent->wasCommunicationInLastInterval(subnet->getSubnetSettings().deviceCommandsSilenceInterval, currentTime) ){
               continue;
        }


        if (deviceLevel >= 3 /*|| changeParentByLoad*/) {
            //try to detect a new parent for device
            if(detectNewParentForDevice(subnet, device, device, operationsContainer, false)) {
//                subnet->defaultGraphCanBeReevaluated = false;

                LOG_DEBUG("foundBetterParent for device:" << Address_toStream(device->address32) << ", oldParent= " <<
                            Address_toStream(parent->address32) <<  ", newParent= " << Address_toStream(device->parent32) );
                if (!operationsContainer->isContainerEmpty()) {
                    ChainWaitForConfirmOnEvalGraphPointer chainConfirmGraphOperations(new ChainWaitForConfirmOnEvalGraph(subnet, DEFAULT_GRAPH_ID));
                    operationsContainer->addHandlerResponse(boost::bind(&ChainWaitForConfirmOnEvalGraph::process, chainConfirmGraphOperations, _1, _2, _3));
                    LOG_DEBUG("GRAPH EVAL disable in findBetterParentForDevices");
                    subnet->disableInboundGraphEvaluation();
                }
                break;
            }
        }
    }

    if (device && parent && !operationsContainer->isContainerEmpty()) {
        char reason[128];
        sprintf(reason, "findBetterParentForDevice %x, oldParent=%x, newParent=%x", device->address32, parent->address32, device->parent32);
        LOG_INFO(reason);
        operationsContainer->reasonOfOperations = reason;

        operationsProcessor->addOperationsContainer(operationsContainer);
    }
}


void TheoreticEngine::periodicEvaluateDirtyEdges(Subnet::PTR subnet) {
    if(!subnet->isEnabledInboundGraphEvaluation() || !subnet->isEnabledOutboundGraphEvaluation()) {
        return;

    }

    if (subnet->dirtyEdges.empty()) {
        return;
    }

    char reason[64];
    sprintf(reason, "periodicEvaluateDirtyEdges");

    OperationsContainerPointer operationsContainer(new OperationsContainer(reason));

    Uint32List processedEdges;//keep a list with already processed edges in order to not process the same edge twice

    for (Uint32List::iterator itEdge = subnet->dirtyEdges.begin(); itEdge != subnet->dirtyEdges.end(); ) {
        Address16 edgeSrc = *itEdge >> 16;
        Address16 edgeDst = (*itEdge & 0x0000FFFF);
        EdgePointer edge = subnet->getEdge(edgeSrc, edgeDst);
        EdgePointer reverseEdge = subnet->getEdge(edgeDst, edgeSrc);
        bool mustDelete = (!edge || !reverseEdge);
        if(!mustDelete) {
            mustDelete = (!edge->existGraphOnEdge() && !reverseEdge->existGraphOnEdge());
        }

        Uint32List::iterator alreadyProcessed = std::find(processedEdges.begin(),processedEdges.end(),  *itEdge);
        if(alreadyProcessed != processedEdges.end()) {
            mustDelete = false;
        }
        if(mustDelete ) {
            std::ostringstream stream;
            stream << std::hex << edgeSrc << "-->" << std::hex << edgeDst ;
            LOG_INFO("DeleteEdge " << stream.str());
            Uint32 revEdges = ((edgeDst << 16) + edgeSrc);
            processedEdges.push_back(*itEdge);
            processedEdges.push_back(revEdges);
            deleteNeighbors(subnet, operationsContainer, edgeSrc, edgeDst);
            deleteNeighbors(subnet, operationsContainer, edgeDst, edgeSrc);
            NE::Model::Operations::OperationsList generatedLinks;
            subnet->unreserveLinkForDevice(edgeSrc, edgeDst, operationsContainer.get(), generatedLinks);
            subnet->unreserveLinkForDevice(edgeDst, edgeSrc, operationsContainer.get(), generatedLinks);
            subnet->dirtyEdges.erase(itEdge++);

        }
        else {
            if(edge && edge->isGraphOnEdge(DEFAULT_GRAPH_ID)) {
                std::ostringstream stream;
                stream << std::hex << edgeSrc << "-->" << std::hex << edgeDst ;
                LOG_INFO("Edge " << stream.str() << " is removed from dirtyEdges as long as it is reused in graph 1");
                subnet->dirtyEdges.erase(itEdge++);
            }
            else {
                ++itEdge;
            }
        }


    }

    if (!operationsContainer->isContainerEmpty()){
        ChainWaitForConfirmOnEvalGraphPointer chainConfirmGraphOperations(new ChainWaitForConfirmOnEvalGraph(subnet, DEFAULT_GRAPH_ID));
        operationsContainer->addHandlerResponse(boost::bind(&ChainWaitForConfirmOnEvalGraph::process, chainConfirmGraphOperations, _1, _2, _3));
        LOG_DEBUG("GRAPH EVAL disable in periodicEvaluateDirtyEdges");
        subnet->disableInboundGraphEvaluation();
        operationsProcessor->addOperationsContainer(operationsContainer);

    }
}

void TheoreticEngine::deleteLinks(Subnet::PTR subnet, EntityIndexList& linksToBeDeleted, Operations::OperationsContainerPointer& container) {
    for(EntityIndexList::iterator it = linksToBeDeleted.begin(); it != linksToBeDeleted.end(); ++it) {
        // destroy reservation
        Device* device = subnet->getDevice(getDeviceAddress(*it));
        if(!device) {
            continue;
        }

        LinkIndexedAttribute::iterator itLink = device->phyAttributes.linksTable.find(*it);
        if (itLink != device->phyAttributes.linksTable.end()) {
            PhyLink * link = itLink->second.getValue();
            if(link) {
                subnet->unreserveLink(device, link);
                DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(*it));
                container->addOperation(deleteLink, device);
            }
        }
    }
}

void TheoreticEngine::deleteLinks(Subnet::PTR &subnet, Device* neighbor, Device* device, Operations::OperationsContainerPointer container, bool isRemoveDevice) {

    // generate remove LINK operations
    if(!neighbor || !device) {
        return;
    }

    Address16 oldPArent16 = Address::getAddress16(device->address32);
    Address16 device16 = Address::getAddress16(neighbor->address32);;

    LinkIndexedAttribute::iterator itLink = neighbor->phyAttributes.linksTable.begin();
    for (; itLink != neighbor->phyAttributes.linksTable.end(); ++itLink) {
        PhyLink* link = (PhyLink*) itLink->second.getValue();
        if(!link) {
            continue;
        }

        if ( (link->type & NE::Model::Tdma::LinkType::TRANSMIT) && link->neighborType == Tdma::NeighbourTypes::NEIGHBOUR_BY_ADDRESS && oldPArent16 == link->neighbor &&
                    link->role != NE::Model::Tdma::TdmaLinkTypes::APPLICATION) {

            if (isRemoveDevice){
                // destroy reservation
                subnet->unreserveLink(neighbor, link);
            } else {
                // destroy reservation
                subnet->unreserveLink(neighbor, link, container.get());
            }

            DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(itLink->first));
            container->addOperation(deleteLink, neighbor);
        }
    }


    LinkIndexedAttribute::iterator itLnkParent = device->phyAttributes.linksTable.begin();
    for (; itLnkParent != device->phyAttributes.linksTable.end(); ++itLnkParent) {
        PhyLink* link = (PhyLink*) itLnkParent->second.getValue();
        if(!link) {
            continue;
        }

        if ( (link->type & NE::Model::Tdma::LinkType::TRANSMIT) && link->neighborType == Tdma::NeighbourTypes::NEIGHBOUR_BY_ADDRESS && device16 == link->neighbor) {


            if (isRemoveDevice){
                // destroy reservation
                subnet->unreserveLink(device, link, container.get());
            } else {
                // destroy reservation
                subnet->unreserveLink(device, link, container.get());
            }

            if(!isRemoveDevice) {
                DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(itLnkParent->first));
                container->addOperation(deleteLink, device);
            }
        }
    }

}

void TheoreticEngine::onChangeParentsReevaluateGraphs(Subnet::PTR &subnet, Device* device , bool isRemove) {
    //add to evaluation all graphs that pass through removedDevice
    if(!device) {
        return;
    }

    Device* backbone = subnet->getBackbone();

    GraphPointer graph1 = subnet->getGraph(DEFAULT_GRAPH_ID);
    if(graph1 && isRemove) {
        graph1->removedDevices.insert(Address::getAddress16(device->address32));
    }

    GraphIndexedAttribute::iterator itGraph = device->phyAttributes.graphsTable.begin();
    for (; itGraph != device->phyAttributes.graphsTable.end(); ++itGraph) {
        //         ... check backbone's routes to see if there is one that uses the device's graph
        if(getIndex(itGraph->first) != DEFAULT_GRAPH_ID) {
            subnet->addGraphToBeEvaluated(getIndex(itGraph->first));
        }
    }


    if(backbone && !isRemove) {
        Uint16 outboundGraph = backbone->getOutBoundGraph(Address::getAddress16(device->address32));
        if(outboundGraph) {
            subnet->addGraphToBeEvaluated(outboundGraph);
        }
    }

    if(isRemove) {
            subnet->addGraphToBeEvaluated(DEFAULT_GRAPH_ID);

    }


}

void TheoreticEngine::removeDeviceFromOutBoundGraphs(Subnet::PTR &  subnet, const Device* backbone ,const Device* removingDevice,
            OperationsList & generatedOperations, Uint16 targetGraphId) {
    if(!removingDevice) {
        return;
    }

   if(!backbone) {
       return;
   }

    GraphPointer graph = subnet->getGraph(DEFAULT_GRAPH_ID);
    if(!graph) {
        return;
    }

    Device* parent = subnet->getDevice(removingDevice->parent32);
    Address16 backup = graph->getBackupFor(Address::getAddress16(removingDevice->address32));
    Device* backupDevice = subnet->getDevice(backup);
    Uint16 outboundGraphForDevice = backbone->getOutBoundGraph(Address::getAddress16(removingDevice->address32));

    if(parent) {
        EntityIndex graphIx = createEntityIndex( parent->address32, EntityType::Graph, targetGraphId);
        GraphIndexedAttribute::iterator itGraph = targetGraphId ? parent->phyAttributes.graphsTable.find(graphIx) : parent->phyAttributes.graphsTable.begin();
        for (; itGraph != parent->phyAttributes.graphsTable.end() && (!targetGraphId || itGraph->first == graphIx); ++itGraph) {
            //         ... check backbone's routes to see if there is one that uses the device's graph
            if(getIndex(itGraph->first) == DEFAULT_GRAPH_ID ) {
                continue;
            }
            if( getIndex(itGraph->first) == outboundGraphForDevice) {
                continue;
            }
            if ( itGraph->second.getValue() == NULL) {
                continue;
            }
            PhyGraph * graph = new PhyGraph(*itGraph->second.getValue());
            std::vector<Uint16>::iterator existsNeighbor = std::find(graph->neighbors.begin(), graph->neighbors.end(), Address::getAddress16(removingDevice->address32));
            if(existsNeighbor !=  graph->neighbors.end() &&  graph->neighbors.size() > 1) {
                graph->neighbors.erase(existsNeighbor);
                IEngineOperationPointer updateGraphOperation(new Operations::WriteAttributeOperation(graph, itGraph->first));
                generatedOperations.push_back(updateGraphOperation);
            }
            else {
            	delete graph;
            }
        }
    }

    if (backupDevice) {
        EntityIndex graphIx = createEntityIndex( backupDevice->address32, EntityType::Graph, targetGraphId);
        GraphIndexedAttribute::iterator itGraph = targetGraphId ? backupDevice->phyAttributes.graphsTable.find(graphIx) : backupDevice->phyAttributes.graphsTable.begin();
        for (; itGraph != backupDevice->phyAttributes.graphsTable.end() && (!targetGraphId || itGraph->first == graphIx); ++itGraph) {
            //         ... check backbone's routes to see if there is one that uses the device's graph
            if(getIndex(itGraph->first) == DEFAULT_GRAPH_ID ) {
                continue;
            }
            if( getIndex(itGraph->first) == outboundGraphForDevice) {
                continue;
            }

            if ( itGraph->second.getValue() == NULL) {
                continue;
            }
            PhyGraph * graph = new PhyGraph(*itGraph->second.getValue());
            std::vector<Uint16>::iterator existsNeighbor = std::find(graph->neighbors.begin(), graph->neighbors.end(), Address::getAddress16(removingDevice->address32));
            if(existsNeighbor !=  graph->neighbors.end() &&  graph->neighbors.size() > 1) {
                graph->neighbors.erase(existsNeighbor);
                IEngineOperationPointer updateGraphOperation(new Operations::WriteAttributeOperation(graph, itGraph->first));
                generatedOperations.push_back(updateGraphOperation);

            }

            else {
            	delete graph;
            }

        }
    }
}





}

}
