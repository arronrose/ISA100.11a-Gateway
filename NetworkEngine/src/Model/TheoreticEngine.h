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
 * TheoreticEngine.h
 *
 *  Created on: Sep 18, 2009
 *      Author: Catalin Pop, beniamin.tecar
 */

#ifndef THEORETICENGINE_H_
#define THEORETICENGINE_H_
#include "Model/Operations/OperationsContainer.h"
#include "Common/EvaluationSignal.h"
#include "Common/ISettingsProvider.h"
#include "Model/Subnet.h"
#include "Model/ModelPrinter.h"
#include "Model/IDeviceRemover.h"

namespace NE {
namespace Model {

class SubnetsContainer;
class RouteEngine;

namespace Operations {
    class OperationsProcessor;
}

namespace Tdma {
    class LinkEngine;
}

class TheoreticEngine {
        LOG_DEF("I.M.TheoreticEngine");

    private:

        SubnetsContainer* subnetsContainer;
        NE::Model::Operations::OperationsProcessor* operationsProcessor;
        NE::Common::ISettingsProvider * settingsProvider;

        NE::Model::RouteEngine * routeEngine;
        NE::Model::Tdma::LinkEngine * linkEngine;
        NE::Model::IDeviceRemover * devicesRemover;


    public:

        TheoreticEngine(SubnetsContainer * subnetsContainer_, Operations::OperationsProcessor * operationsProcessor_,
                    NE::Model::IDeviceRemover * devicesRemover_,
                    NE::Common::ISettingsProvider * settingsProvider_);

        virtual ~TheoreticEngine();

        SubnetsContainer * getSubnetsContainer() {
            return subnetsContainer;
        }

        NE::Model::Operations::OperationsProcessor* getOperationsProcessor() {
            return operationsProcessor;
        }

        /**
         * This method creates the JoinEdge (edge between new device and it's proxy router)
         * and generates the operations required by the join.
         */
        void createJoinEdge(Device * joiningDevice, Device * parentDevice, Subnet::PTR subnet);

        /**
         * This method allocates first management links between a joining  device and its parent
         * @param joiningDevice
         * @param parentDevice
         * @param operationsContainer
         * @return
         */
        bool allocateManagementJoinLinks(Device * joiningDevice, Device * parentDevice, Operations::OperationsContainerPointer& operationsContainer);

        void periodicConfigureNeighborDiscovery(Uint32 currentTime, Subnet::PTR& subnet);

        ResponseStatus::ResponseStatusEnum allocateApplicationTraffic(PhyContract * contract, Operations::OperationsContainerPointer& operationsContainer);

        void evaluateAppOutboundTraffic(
                    Address32 destination32,
                    Subnet::PTR& subnet,
                    Operations::OperationsContainerPointer&  container);

        /**
         * Perform the periodic evaluation of the network: activation of roles, evaluation
         * of the routes and graphs, garbage collection.
         */
        void periodicEvaluation(Subnet::PTR subnet, NE::Common::EvaluationSignal evaluationSignal, Uint32 currentTime);

        void readJoinReason(Subnet::PTR subnet, Uint32 currentTime);

        void periodicRemoveAcceleratedLinks(Subnet::PTR& subnet, Uint32 currentTime);

        void periodicRoutersAdvertiseCheck(Subnet::PTR& subnet);

        void periodicFastDiscoveryCheck(Subnet::PTR& subnet, Uint32 currentTime);

        void updateAdvertisePeriod(Subnet::PTR& subnet);

        /**
         * Creates the G1 graph after backbone's join.
         */
        void createGraphG1(Subnet::PTR subnet, Address16 backboneAddress);

        /**
         * When a device is removed, add its outbound graph to garbage collection in order to be removed.
         */
        void addGraphToGarbageCollection(Uint16 subnetId, Uint16 graphId);

        /**
         * When a device is removed, add a route to be reevaluated.
         */
        void addRouteToBeEvaluated(Uint16 subnetId, EntityIndex routeEntityIndex);

        /**
         * Remove device and update the theoretic's engine model's containers.
         */
        void removeDevice(Address32 deviceToRemove, Subnet::PTR& subnet);

        void redirectChildParent(Subnet::PTR& subnet, Device* removingDevice) ;

        bool detectNewParentForDevice(Subnet::PTR & subnet, Device * child,   Device * avoidClockSourceDevice,
                    Operations::OperationsContainerPointer & operationsContainer, bool isUrgentToChangeParent);

        void redirectDeviceToNewParent(Subnet::PTR & subnet, Device * device, Device * oldParent, Device * newParent,
                    Operations::OperationsContainerPointer & operationsContainer);

        void retryToFindNewParent(Subnet::PTR & subnet, Device* device,  Device* oldParent, Operations::OperationsContainerPointer & operationsContainer);

        void getPathForOutboundGraph(Subnet::PTR & subnet, GraphPointer & graph, std::vector<Device*> & outboundPath);

        void getPathForInboundGraph(Subnet::PTR & subnet, Device * device, /*GraphPointer & graph,*/ std::vector<Device*> & inboundPath);

        bool allocateUdoLinksForOutboundGraph(Operations::OperationsContainerPointer& operationsContainer,
                    NE::Model::Subnet::PTR subnet, Uint16 graphID, EntityIndex eiContract);

        bool allocateUdoLinksForInboundGraph(Operations::OperationsContainerPointer& operationsContainer,
                    NE::Model::Subnet::PTR subnet, Device * device, EntityIndex eiContract /*, Uint16 graphID, Address16 deviceAddress16*/);

        void addUdoContractToBeEvaluated(Subnet::PTR subnet, EntityIndex & eiContract);

        void updateUdoContract(Subnet::PTR subnet, EntityIndex & eiContract, Int16 newCommittedBurst);

        void onChangeParentForDevice(Subnet::PTR & subnet, Device * changingDevice);

    private:

        /**
         * Picks next device to be activated and perform an activation of it's role. The activation of the role means
         * configuring all settings that device needs to accomplish the role of Routing device (add advertise and join links).
         */
        void periodicRoleActivation(Subnet::PTR subnet, Uint32 currentTime);

        /**
         * Periodic out bound graphs routes  Evaluation.
         */
        void periodicRoutesEvaluation(Subnet::PTR subnet);

        /**
         * Periodic inbound graphs.
         */
        void periodicGraphsEvaluation(Subnet::PTR& subnet, Uint32 currentTime);

        void evaluateGraph(Subnet::PTR subnet, Uint16 graphId, Uint32 currentTime, Operations::OperationsContainerPointer & operationsContainer);

        void periodicGraphsRedundancyEvaluation(Subnet::PTR subnet, Uint32 currentTime);


        void physicalModelUpdate(Operations::OperationsContainerPointer& operationsContainer,
                                 NE::Model::Subnet::PTR subnet,
                                 Uint16 graphID,
                                 DoubleExitEdges &usedEdges,
                                 DoubleExitEdges &notUsedEdges,
                                 bool isInBound, Uint32 currentTime);


        void deleteEdgeFromGraph(Subnet::PTR subnet, Operations::OperationsContainerPointer& operationsContainer, Address16 src, Address16 dst,
                    Uint16 graphID );

        void updateNeighbors(Subnet::PTR subnet, Operations::OperationsContainerPointer& operationsContainer,
                    Address16 device, DoubleExitDestinations& destinations, bool isInBound, bool isDelete);

        void createNeighbors(Subnet::PTR subnet, Operations::OperationsContainerPointer& operationsContainer,
                    Address16 device, Address16 neighbor, Uint8 clockSource, bool isInBound);

        void deleteNeighbors(Subnet::PTR subnet, Operations::OperationsContainerPointer& operationsContainer,
                    Address16 device, Address16 neighbor);

        void updateManagementLinks(Subnet::PTR subnet, Operations::OperationsContainerPointer& operationsContainer,
                    Address16 source, Address16 destination, bool isInbound, NE::Model::Operations::OperationsList& generatedLinks);

        void updateManagementLinks(Subnet::PTR subnet, Operations::OperationsContainerPointer& operationsContainer,
                    Device * source, Device * destination, Uint16 inboundLnkInterval, Uint16 outboundLnkInterval, NE::Model::Operations::OperationsList& generatedLinks);

        void updateManagementLinksOnChangeParent(Subnet::PTR subnet, Operations::OperationsContainerPointer& operationsContainer,
                    Device * source, Device * destination, Device * oldDestination, NE::Model::Operations::OperationsList& generatedLinks);

        void theoreticModelUpdate(NE::Model::Subnet::PTR subnet, GraphPointer& graph, DoubleExitEdges &usedEdges, DoubleExitEdges &notUsedEdges );


         /**
          *
          */

         void populateDependencies(const DoubleExitDestinations &dependencies,
                     NE::Model::Operations::OperationsContainerPointer& operationsContainter,
                     NE::Model::Operations::OperationDependency &operationDependency,
                     NE::Model::Operations::EntityDependency &entityDependency,
                     Address16 deviceAddress16);

         void addDependencesOnDeleteLink( NE::Model::Operations::OperationsContainerPointer& operationsContainter,
                                                      NE::Model::Operations::OperationDependency &operationDependency);

         void updateGraphOnJoin(Operations::OperationsContainerPointer& operationsContainer,
                     NE::Model::Subnet::PTR subnet, Uint16 graphID, Address16 deviceAddress16, Address16 parentAddress16 );


         void createGraphOperationDoubleExit(Subnet::PTR subnet, Operations::OperationsContainerPointer& operationsContainer, Address16 device,
                     DoubleExitDestinations &dependencies, Uint16 graphID );

         void createGraphOperation(Subnet::PTR subnet, Operations::OperationsContainerPointer& operationsContainer, Address16 device,DoubleExitDestinations &destination,
                     Uint16 graphID, NE::Model::Operations::OperationsList& dependOnLinksList,  NE::Model::Operations::OperationsList& generatedGraphOperations );

         void deleteGraph(Subnet::PTR subnet, Uint16 graphId);

         /**
          * Check all hybrid (graph+address) routes on BBR and remove the devices that are the destination of that route.
          * This are lost devices for outbound traffic, and this device did not completed join.
          * @param graphId
          */
         void removeDeviceRelyingOnOutboundGraph(Device *backbone, Uint16 graphId);

         void deleteRouteFromBackbone(Subnet::PTR subnet, Uint16 graphId, Address16 destination);

         void periodicDirtyContractsEvaluation(Subnet::PTR subnet);

         void periodicUdoContractsEvaluation(Subnet::PTR subnet);

         void periodicClientServerContractsEvaluation(Subnet::PTR subnet);

         /**
          * iterates through subnet's active devices ..selects those ones with inferior level( >=3 ) and tries to find a better parent(superior level)
          * @param subnet
          */
         void findBetterParentForDevices(Subnet::PTR subnet, Uint32 currentTime);


         /**
          * iterates through subnet's dirtyEdges...if the edge is no more used in any graph, deletes the links and neighbors from the edge
          * @param subnet
          * @param currentTime
          */

         void periodicEvaluateDirtyEdges(Subnet::PTR subnet);


         /**
          * Remove dirty links: app links used by contracts of removed devices or terminated contracts.
          *
          * @param subnet
          * @return true - if at least 1 link was removed, false if no link was removed.
          */
         bool periodicDirtyLinksRemoval(Subnet::PTR subnet, Uint32 currentTime);

         void createNewLinksForContractsOnParentChange(Subnet::PTR subnet, Device* dvc, Device* oldParent, Device* newParent);

         void deleteOldLinksForContract(Subnet::PTR subnet,EntityIndex contractIndex, Address16 oldParent, EntityIndexList& linksToBeDeleted);

         void deleteLinks(Subnet::PTR subnet,EntityIndexList& linksToBeDeleted, Operations::OperationsContainerPointer& container);

    public:

         void deleteLinks(Subnet::PTR &subnet, Device* neighbor, Device* device, Operations::OperationsContainerPointer container, bool isRemoveDevice);

         void onChangeParentsReevaluateGraphs(Subnet::PTR &subnet, Device* device ,  bool isRemove);

         void updateLinksForUdoContract(Subnet::PTR & subnet, EntityIndex & eiContract, Address32 contractDestination, bool isTerminateContract);

         void removeDeviceFromOutBoundGraphs(Subnet::PTR & subnet, const Device* backbone ,const Device* removingDevice, Operations::OperationsList & generatedOperations, Uint16 targetGraphId = 0);

};

}

}

#endif /* THEORETICENGINE_H_ */
