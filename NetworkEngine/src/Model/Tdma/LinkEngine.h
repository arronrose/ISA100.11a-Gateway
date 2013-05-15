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
 * LinkEngine.h
 *
 *  Created on: Mar 19, 2009
 *      Author: Catalin Pop, ion.ticus, flori.parauan, eduard.budulea,beniamin.tecar
 */

#ifndef LINKENGINE_H_
#define LINKENGINE_H_

#include <vector>
#include "Common/NETypes.h"
#include "Model/IDevicesProvider.h"
#include "Model/Operations/OperationsContainer.h"
#include "Common/logging.h"
#include "Model/Routing/RouteTypes.h"
#include "Common/ISettingsProvider.h"
#include "Model/ModelUtils.h"
#include "ManagementLinksUtils.h"
#include "AppAllocation.h"
#include "Model/Operations/WriteAttributeOperation.h"

namespace NE {

namespace Model {

class SubnetsContainer;
namespace Operations {
class OperationsProcessor;
class OperationsContainer;
}

namespace Tdma {

class LinkEngine {
    LOG_DEF("I.M.T.LinkEngine");

    private:
        SubnetsContainer * subnetsContainer;
        Operations::OperationsProcessor * operationsProcessor;

    public:
        LinkEngine(SubnetsContainer * subnetsContainer_, Operations::OperationsProcessor * operationsProcessor_);
        virtual ~LinkEngine();

// mng links methods

    public:
        bool setMngFirstLinks(  Device * device,
                                Device * parent,
                                Operations::OperationsContainer& container);

        void activateRouterRole(Device * router,
                                Subnet::PTR& subnet,
                                NE::Model::Operations::OperationsContainerPointer& container);

        bool changeAdvertiseLinkPeriod(Device * router,
                                Uint16 desiredPeriod,
                                Subnet::PTR& subnet,
                                NE::Model::Operations::OperationsContainerPointer& container);
        bool updateAdvPeriod(   Device * router,
                                Subnet::PTR& subnet,
                                NE::Model::Operations::OperationsContainerPointer& container);


        void redirectChildParent(Subnet::PTR subnet, Device* child, Device * parent, NE::Model::Operations::OperationsContainerPointer& container);

        void addNewInboundEdge( Subnet::PTR subnet,
                                Operations::OperationsContainer& container,
                                Device* device,
							    Device* parent,
                                NE::Model::Operations::OperationsList& generatedLinks);

        void addNewOutboundEdge( Subnet::PTR subnet,
                                Operations::OperationsContainer& container,
                                Device* parent,
							    Device* device,
                                NE::Model::Operations::OperationsList& generatedLinks);

        /**
         * Destroy all acceleration links between device and parent1 and between device and parent2.
         * @param device
         * @param parent1
         * @param parent2
         * @param container
         * @param subnet
         * @param currentTime
         */
        void destroyAccMngLink( Device* device, Device* parent1, Device * parent2, Operations::OperationsContainer& container, Subnet::PTR& subnet , Uint32 currentTime);
        /**
         * Destroy all acceleration OUT links between device and parent1 and between device and parent2.
         * @param device
         * @param parent1
         * @param parent2
         * @param container
         * @param subnet
         * @return false if accelerated link was found and was not deleted (was a pending modification on it), true otherwise.
         */
        bool destroyOUTAccMngLink( Device* device, Device* parent1, Device * parent2, Operations::OperationsContainer& container, Subnet::PTR& subnet );

        /**
         * Destroy the accelerated IN links if exist.
         * @param device
         * @param parent
         * @param container
         * @param subnet
         * @return false if accelerated link was found and was not deleted (was a pending modification on it), true otherwise.
         */
        bool destroyINAccMngLink( Device* device, Device* parent, Operations::OperationsContainer& container, Subnet::PTR& subnet );

        void revertJoinLinksToDefault();

        bool reserveMngChunkInbound(Operations::OperationsContainer & operationsContainter,
                    Subnet::PTR& subnet,
                    Device * device,
                    Device * parent,
                    bool reserveForUdo,
                    ManagementLinksUtils & linkUtils );

        bool reserveMngChunkOutbound(Operations::OperationsContainer & operationsContainter,Subnet::PTR & subnet,
                    Device * parent,
                    Device * device,
                    bool reserveForUdo,
                    Uint16 inboundOffset,
                    ManagementLinksUtils & linkUtils);

        void changeNeighborDiscovery(
                    Operations::OperationsContainer & operationsContainter,
                    Subnet::PTR & subnet,
                    Device * device,
                    bool makeFast);

    private:

        NE::Model::Tdma::JoinTimeout::JoinTimeoutEnum calculateJoinTimeout(Device * router, Subnet::PTR& subnet);

        NE::Model::Operations::IEngineOperationPointer
            createMngLink( Device* device,
                            Operations::OperationsContainer& container,
                            Uint16 superframeId,
                            LinkType::LinkTypesEnum linkType,
                            TdmaLinkTypes::TdmaLinkTypesEnum tdmaLinkType,
                            TdmaLinkDir::TdmaLinkDirEnum linkDirection,
                            Uint16 schOffset,
                            Uint16 schedInterval,
                            Uint8 chOffset,
                            Address16 address);

        void createAccelerationOUT(Device* parent, Device* device, Operations::OperationsContainer& container,
                    ManagementLinksUtils &linksUtils, EntityIndex &indexSuperframe, const NE::Common::SubnetSettings& subnetSettings);
        void createAccelerationIN(Device* parent, Device* device, Operations::OperationsContainer& container,
                    ManagementLinksUtils &linksUtils, EntityIndex &indexSuperframe, const NE::Common::SubnetSettings& subnetSettings);

        NE::Model::Operations::IEngineOperationPointer
            createAccMngLink( Device * device,
                            Operations::OperationsContainer & container,
                            EntityIndex &indexSuperframe,
                            LinkType::LinkTypesEnum  linkType,
                            TdmaLinkDir::TdmaLinkDirEnum linkDirection,
                            Uint8 setNo,
                            Uint8 chOffset,
                            Address16 address,
                            const SubnetSettings& subnetSettings);

        void createAndReserveChunkOut(Subnet::PTR & subnet,
									Device * parent,
									Device * device,
									ManagementLinksUtils & linkUtils,
									bool reservedForRouter);


        bool ckMngReservedOutbound( Subnet::PTR subnet, Device * parent, Device * device,ManagementLinksUtils &linkUtils, Uint8& chunksForRouter, Uint8& chunksForIO,  bool reserveForUdo = false);
        bool ckReservedMngInbound(Subnet::PTR subnet, Device * device, Device * parent,ManagementLinksUtils &linkUtils, bool reserveForUdo = false);

        ResponseStatus::ResponseStatusEnum  updateMngInboundLinks(Subnet::PTR subnet,
                                                                   Device * srcDevice,
                                                                   Address16 parent16,
                                                                   Operations::OperationsContainerPointer& container,
                                                                   float neededInbound);

        ResponseStatus::ResponseStatusEnum  updateMngOutboundLinks(Subnet::PTR subnet,
                                                                   Device * srcDevice,
                                                                   Address16 parent16,
                                                                   Operations::OperationsContainerPointer& container,
                                                                   float neededOutbound);

        void updateMngLinks(Subnet::PTR subnet,  Operations::OperationsContainer& container, NE::Common::SubnetSettings& subnetSettings, Device * device, Uint16 graphId = 1);


// app links methods
    public:

        void  updateMngLinksPeriod( Device * srcDevice,
                                      PhyLink & linkTx,
                                      Device * dstDevice,
                                      PhyLink & linkRx,
                                      Uint16 period,
                                      Operations::OperationsContainerPointer & container,
                                      NE::Model::Operations::OperationsList & generatedLinks);

        Operations::IEngineOperationPointer
            addMngInboundLinks(Device* device, Device* parent, Operations::OperationsContainer& container, TdmaLinkTypes::TdmaLinkTypesEnum tdmaLinkType,
                    ManagementLinksUtils &linksUtils, Uint16 superframeId, NE::Model::Operations::OperationsList& createdLinks, const SubnetSettings& subnetSettings);

        void addMngOutboundLinks(Device* parent, Device* device, Operations::OperationsContainer& container,
                         TdmaLinkTypes::TdmaLinkTypesEnum tdmaLinkType, ManagementLinksUtils &linksUtils, Uint16 superframeId,
                         std::vector<Operations::IEngineOperationPointer> & generatedOperations);

        ResponseStatus::ResponseStatusEnum allocateAppLinks(PhyContract * contract, PhyRoute * route,
                    Operations::OperationsContainerPointer& container);

        void updateMngAndAppRedundantInboundLinks(
                                                    Subnet::PTR  subnet,
                                                    Uint16 graphId,
                                                    Operations::OperationsContainerPointer& container,
                                                    Uint32 currentTime);


        PhyChannelHopping * initialiseAdvertiseChannelHopping(NE::Common::SubnetSettings& subnetSettings);

        void addMngChHoppingAndSuperframe(  Device* device,
                                            Operations::OperationsContainer& container,
                                            NE::Common::SubnetSettings& subnetSettings,
                                            EntityIndex& indexSuperframe,
                                            EntityIndex &indexChannelHopping);

        ResponseStatus::ResponseStatusEnum evaluateAppOutboundLinks(
                                                                Address32 destination32,
                                                                Subnet::PTR& subnet,
                                                                PhyRoute * route,
                                                                Operations::OperationsContainerPointer&  container,
                                                                Uint16 startSlot = 0 );

    private:
        ResponseStatus::ResponseStatusEnum allocateAppInboundDirectLinks(Subnet::PTR  subnet,
                                                                        PhyContract * contract,
                                                                        Device* realContractSource,
                                                                        Device* srcDevice,
                                                                        Device* dstDevice,
                                                                        PhyRoute * route,
                                                                        Operations::OperationsContainerPointer& container,
                                                                        Uint16 &graphID,  Uint16 &nextSlot );

        ResponseStatus::ResponseStatusEnum createApplicationDirectLinks(Subnet::PTR subnet,
                                                                        PhyContract * contract,
                                                                        Device* realContractSource,
                                                                        Device* sourceDevice,
                                                                        Operations::OperationsContainerPointer& container,
                                                                        DoubleExitEdges &preferedEdges,
                                                                        Uint16 currSlot,
                                                                        Uint16 maxSlotDelay,
                                                                        Uint16 periodInSlots,  Uint16 &nextSlot ) ;


        Uint32 retryReserveFreeSlot(Subnet::PTR & subnet, Device * srcDevice,
                    Device * edgeDestination, Uint16 currSlot, Uint16 maxSlotDelay, Uint16 periodInSlots, Operations::OperationsContainerPointer& container);



        ResponseStatus::ResponseStatusEnum  updateAppInboundLinks(Subnet::PTR& subnet,
                                                           Device * srcDevice,
                                                           Address16 dstDevice16,
                                                           Operations::OperationsContainerPointer& container,
                                                           float neededInbound);

        ResponseStatus::ResponseStatusEnum computeAppOutboundTraffic(Subnet::PTR& subnet,
                    Operations::OperationsContainerPointer& container, Address16 deviceAddress16, GraphPointer & graph, Uint16& nextStartSlot);

        ResponseStatus::ResponseStatusEnum  increaseAppRedundantLink( Subnet::PTR subnet,
                                                                      Operations::OperationsContainerPointer& container,
                                                                      Device * source,
                                                                      Device * destination,
                                                                      Uint16 neededPeriod, TdmaLinkDir::TdmaLinkDirEnum linkDir, Uint16& nextStartSlot);

        void deleteAppRedundantLink(Subnet::PTR subnet,
                                                    Operations::OperationsContainerPointer& container,
                                                    Device * source,
                                                    Device * destination,
                                                    TdmaLinkDir::TdmaLinkDirEnum linkDir);

        void  decreaseAppRedundantLink( Subnet::PTR& subnet,
                                          Operations::OperationsContainerPointer& container,
                                          Device * source,
                                          Device * destination,
                                          Uint16 extraPeriod, TdmaLinkDir::TdmaLinkDirEnum linkDir);
// neighbor discovery methods
    public:
        void allocateNeighborDiscoveryLinks(Device* device, Operations::OperationsContainerPointer& container, Subnet::PTR subnet) ;


        void    allocateAppDirectLinksForContractsAfterParentChage(Subnet::PTR  subnet,
                                            PhyContract * contract,
                                            Device* realContractSource,
                                            Device* srcDevice,
                                            Device* dstDevice,
                                            Uint16 startSlot,
                                            Operations::OperationsContainerPointer& container,
                                            DoubleExitEdges& preferedEdges);


    private:
        void createNeighborDiscoveryChannelHopping(Device* device, Operations::OperationsContainer& container,
                    EntityIndex &indexChannelHopping, NE::Common::SubnetSettings& subnetSettings);

        void createNeighborDiscoverySuperframe(Device * device,Operations::OperationsContainer& container,EntityIndex &indexSuperframe,
                    NE::Common::SubnetSettings& subnetSettings, bool makeDiscoveryFast );


        NE::Model::Operations::IEngineOperationPointer createNeighborDiscoveryLink(Device* device,
                    EntityIndex &indexSuperframe, Uint16 scheduleOffset);

    private:
         static void createAppLinksPair(
                                        Device * source,
                                        Device * destination,
                                        Uint16 slot,
                                        Uint8 freq,
                                        Uint16 period,
                                        TdmaLinkDir::TdmaLinkDirEnum linkDirection,
                                        TdmaLinkTypes::TdmaLinkTypesEnum linkRole,
                                        Operations::OperationsContainerPointer& container,
                                        PhyContract * contract = NULL,
                                        Device * realContractSource = NULL
                                        );

};

}

}

}

#endif /* LINKENGINE_H_ */
