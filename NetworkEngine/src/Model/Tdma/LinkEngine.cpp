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
 * LinkEngine.cpp
 *
 *  Created on: Mar 19, 2009
 *      Author: Catalin Pop, ion.ticus, flori.parauan, eduard.budulea,beniamin.tecar
 */

#include "LinkEngine.h"
#include <Model/NetworkEngine.h>
#include <set>
#include "Model/SubnetsContainer.h"
#include "Model/Operations/OperationsProcessor.h"
#include "Model/modelDefault.h"
#include "Model/Operations/WriteAttributeOperation.h"
#include "Model/Operations/DeleteAttributeOperation.h"
#include "Model/ModelUtils.h"
#include "Model/ContractsHelper.h"
#include "Model/ModelPrinter.h"
#include "Model/Routing/GraphPrinter.h"
#include "_baseAlgorithms.h"


namespace NE {

namespace Model {

namespace Tdma {


LinkEngine::LinkEngine(SubnetsContainer * subnetsContainer_, OperationsProcessor * operationsProcessor_)
: subnetsContainer(subnetsContainer_), operationsProcessor(operationsProcessor_) {
}

LinkEngine::~LinkEngine() {
}

PhyLink * createLink(Uint16 index,
                     Uint16 superframeIndex,
                     LinkType::LinkTypesEnum linkType,
                     TdmaLinkTypes::TdmaLinkTypesEnum linkRole,
                     TdmaLinkDir::TdmaLinkDirEnum     linkDirection,
                     Uint16 schOffset,
                     Uint16 schedInterval,
                     Uint8  chOffset ){
    PhyLink * linkEmpty = new PhyLink();
    linkEmpty->index = index;
    linkEmpty->superframeIndex = superframeIndex; //ExtDLUint
    linkEmpty->type = linkType;
    linkEmpty->role = linkRole; // DEFAULT/MANAGEMENT/APP/JOIN
    linkEmpty->direction = linkDirection; // INBOUND/OUTBOUND
    linkEmpty->template1 = TemplatesTypes::fromLinkType(linkType); //ExtDLUint
    linkEmpty->template2 = 0; //ExtDLUint
    linkEmpty->neighborType = NeighbourTypes::NONE;
    linkEmpty->graphType = GraphTypes::NONE;

    linkEmpty->schedule.offset = schOffset;
    linkEmpty->schedule.interval = schedInterval;
    linkEmpty->schedType = (schedInterval ? ScheduleType::OFFSET_AND_INTERVAL : ScheduleType::OFFSET);


    linkEmpty->chOffset = chOffset;
    linkEmpty->chType = ( chOffset ? ChannelOffsetType::USE_CHANNEL_OFFSET : ChannelOffsetType::NONE);

    linkEmpty->priorityType = PriorityType::USE_LINK_PRIORITY;
    linkEmpty->neighbor = 0; //ExtDLUint
    linkEmpty->graphID = 0; //ExtDLUint
    linkEmpty->priority = PRIORITY_LINK_GENERAL;

    return linkEmpty;
}



PhySuperframe * initialiseAdvertiseSuperframe(Device * device, Uint8 channelBirth, Subnet::PTR& subnet){
    const NE::Common::SubnetSettings& subnetSettings = subnet->getSubnetSettings();
	PhySuperframe * superframe = new PhySuperframe();
	superframe->index = DEFAULT_ADVERTISE_SUPERFRAME_ID;
	superframe->tsDur = subnetSettings.getTimeslotLength();
	superframe->chIndex = Tdma::ChannelHopping::HOPPING_1;
	superframe->chBirth = channelBirth;
	superframe->sfPeriod = subnetSettings.getSlotsPer30Sec();
	superframe->sfBirth = subnetSettings.superframeBirth;
	superframe->chRate = SUPERFRAME_ADVERTISE_CHANNEL_RATE;
	superframe->sfType = 0;
	superframe->priority = 0;
	superframe->idleUsed = 0;
	superframe->idleTimer = 0;
	superframe->rndSlots = 0;
	return superframe;
}

void deleteDefaultJoinLinks(NE::Model::Operations::OperationsList& generatedLinksInbound, Device* device, Operations::OperationsContainer& container){
    for (LinkIndexedAttribute::iterator itLink = device->phyAttributes.linksTable.begin(); itLink != device->phyAttributes.linksTable.end(); ++itLink){
        PhyLink * link = itLink->second.getValue();

        if (link && link->role == TdmaLinkTypes::DEFAULT
                    && ( link->index == DEFAULT_LINK_TX_ID || link->index == DEFAULT_LINK_RX_ID) ){
            NE::Model::Operations::IEngineOperationPointer deleteLink(
                                new NE::Model::Operations::DeleteAttributeOperation(itLink->first));
            for (NE::Model::Operations::OperationsList::iterator itDependOperation = generatedLinksInbound.begin(); itDependOperation != generatedLinksInbound.end(); ++itDependOperation ){
                deleteLink->addOperationDependency(*itDependOperation);
            }
            container.addOperation(deleteLink, device);
        }
    }
}

bool LinkEngine::setMngFirstLinks(Device * device,
                                  Device * parent,
                                  OperationsContainer& container) {
    if(device->capabilities.isGateway() || device->capabilities.isManager()){
        return true;
    }

    LOG_DEBUG("Set mng first links: " << Address_toStream(device->address32) << ", parent: " << Address_toStream(parent->address32));

    Subnet::PTR subnet = subnetsContainer->getSubnet(device->address32);
    if (subnet == NULL){
        LOG_ERROR("Subnet NULL for " << Address_toStream(device->address32));
        return false;
    }
    const NE::Common::SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    ManagementLinksUtils linksUtils;

    if (device->capabilities.isBackbone()) {

        device->theoAttributes.childID = 0;

        EntityIndex indexSuperframe;
        EntityIndex indexChannelHopping;

        addMngChHoppingAndSuperframe(device,
                                        container,
                                        subnet->getSubnetSettings(),
                                        indexSuperframe,
                                        indexChannelHopping);

        //create a continous Receive Link with lowest priority
        PhyLink * linkInboundRx = createLink(device->getNextLinkID()
                                            , DEFAULT_MANAGEMENT_SUPERFRAME_ID
                                            , LinkType::RECEIVE
                                            , TdmaLinkTypes::DEFAULT // consider as default in order to don't include on management searches
                                            , TdmaLinkDir::INBOUND
                                            , 0
                                            , 2999
                                            , 0
                                        );

        linkInboundRx->priority = PRIORITY_LINK_BBR_RECEIVE; // lowest
        linkInboundRx->schedType = ScheduleType::RANGE;

        EntityIndex indexLinkReceive = createEntityIndex(device->address32, EntityType::Link, linkInboundRx->index);
        NE::Model::Operations::IEngineOperationPointer writeLinkInboundRx(new NE::Model::Operations::WriteAttributeOperation(linkInboundRx, indexLinkReceive));
        container.addOperation(writeLinkInboundRx, device);
    }
    else
    {
        if(device->capabilities.isRouting()) {
            device->theoAttributes.childID = 0;
        }

        // ----- Change neighbor on parent with new device in group 0
	    PhyNeighbor * neighbor = new PhyNeighbor();
	    neighbor->index = Address::getAddress16(device->address32);
	    neighbor->address64 = device->address64;
	    neighbor->clockSource = 0;
	    neighbor->diagLevel = 0;
	    neighbor->extGrCnt = 0;
	    neighbor->groupCode = 0;
	    neighbor->linkBacklog = 0;
	    neighbor->linkBacklogDur = 0;
	    neighbor->linkBacklogIndex = 0;
	    EntityIndex entityIndexNeighbor = createEntityIndex(parent->address32, EntityType::Neighbour, neighbor->index);
	    IEngineOperationPointer addNeighbor(new WriteAttributeOperation(neighbor, entityIndexNeighbor));

	    Uint16 channelMapMask = DEFAULT_CHANNEL_MAP;
	    subnet->channelsBlacklist.createChannelMap(channelMapMask);

	    if(channelMapMask != DEFAULT_CHANNEL_MAP) {
            EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::DLMO_IdleChannels, 0);
            PhyUint16 * idleChannels = new PhyUint16();
            idleChannels->value = ~channelMapMask;
            Operations::IEngineOperationPointer opWriteIdleChannels(new Operations::WriteAttributeOperation(idleChannels, entityIndex));
    //        opWriteSuperframe->setTaiCutOver(settingsLogic->tai_cutover_offset);
            container.addOperation(opWriteIdleChannels, device);
	    }

	    EntityIndex indexSuperframe;
	    EntityIndex indexChannelHopping;

	    addMngChHoppingAndSuperframe(device,
	                                    container,
	                                    subnet->getSubnetSettings(),
	                                    indexSuperframe,
	                                    indexChannelHopping);

	    PhyLink * linkTxDevice = device->getFirstTxMngLink(Address::getAddress16(parent->address32), TdmaLinkDir::INBOUND, Tdma::TdmaLinkTypes::MANAGEMENT);
	    if (linkTxDevice) {//if allready exist a link do not reserve more, it have enough
            //needed because reserve is called from EvaluateGraph also when nothing changed on edges, and cause to reserve up to max chunks limit.
            LOG_WARN("Allready exist INB mng link between parent:" << Address_toStream(device->address32) << " -> dev:" << Address_toStream(parent->address32));

	    } else if( reserveMngChunkInbound(container, subnet, device, parent, false, linksUtils ) ) { //reserve inbound mng chunk

        	NE::Model::Operations::OperationsList generatedLinks;
            addNeighbor->addOperationDependency(
                            addMngInboundLinks(device, parent, container, TdmaLinkTypes::MANAGEMENT,
                                        linksUtils, DEFAULT_MANAGEMENT_SUPERFRAME_ID, generatedLinks, subnetSettings));
            createAccelerationIN(parent, device, container, linksUtils, indexSuperframe, subnetSettings);

            deleteDefaultJoinLinks(generatedLinks, device, container);
        } else {
            if (neighbor) {
                delete neighbor;
                neighbor = NULL;
            }
            addNeighbor->setPhysicalEntity(NULL);
            return false;
        }

	    PhyLink * linkTxParent = parent->getFirstTxMngLink(Address::getAddress16(device->address32), TdmaLinkDir::OUTBOUND, Tdma::TdmaLinkTypes::MANAGEMENT);
	    if (linkTxParent) {//if allready exist a link do not reserve more, it have enough
	        //needed because reserve is called from EvaluateGraph also when nothing changed on edges, and cause to reserve up to max chunks limit.
	        LOG_WARN("Allready exist OUT mng link between parent:" << Address_toStream(parent->address32) << " -> dev:" << Address_toStream(device->address32));

	    } else if( reserveMngChunkOutbound(container, subnet, parent, device, false, linksUtils.offset, linksUtils) ) { //reserve outbound mng chunk
            std::vector<IEngineOperationPointer> generatedOperations;
            addMngOutboundLinks(parent, device, container, TdmaLinkTypes::MANAGEMENT, linksUtils, DEFAULT_MANAGEMENT_SUPERFRAME_ID, generatedOperations);
            addNeighbor->addOperationDependency(generatedOperations[1]);
//#warning the accelerated links are commented
            createAccelerationOUT(parent, device, container, linksUtils, indexSuperframe, subnetSettings);
        } else {
            if (neighbor) {
                delete neighbor;
                neighbor = NULL;
            }
            addNeighbor->setPhysicalEntity(NULL);
            return false;
        }

	    container.addOperation(addNeighbor, parent);

    }
    return true;

}

PhyChannelHopping * LinkEngine::initialiseAdvertiseChannelHopping(NE::Common::SubnetSettings& subnetSettings) {
    PhyChannelHopping*  channelHopping = new PhyChannelHopping();
     channelHopping->index = Tdma::ChannelHopping::HOPPING_1;
     channelHopping->length = subnetSettings.reduced_hopping.size();

     for (int i = 0; i < channelHopping->length; ++i){
         channelHopping->seq.push_back(subnetSettings.reduced_hopping[i]);
     }

     return channelHopping;

}

void LinkEngine::addMngChHoppingAndSuperframe(
                                        Device* device,
                                        Operations::OperationsContainer& container,
                                        NE::Common::SubnetSettings& subnetSettings,
                                        EntityIndex& indexSuperframe,
                                        EntityIndex &indexChannelHopping) {

    indexChannelHopping = createEntityIndex(device->address32, EntityType::ChannelHopping, Tdma::ChannelHopping::HOPPING_8);
    if(!device->existsChannelHoppingIndex(indexChannelHopping))
    {
        PhyChannelHopping*  channelHopping = new PhyChannelHopping();
        channelHopping->index = Tdma::ChannelHopping::HOPPING_8;
        channelHopping->length = subnetSettings.channel_list.size();

        for (int i = 0; i < channelHopping->length; ++i){
			channelHopping->seq.push_back(subnetSettings.channel_list[i]);
        }


        NE::Model::Operations::IEngineOperationPointer writeChHopping(
                    new NE::Model::Operations::WriteAttributeOperation(channelHopping, indexChannelHopping));
        container.addOperation(writeChHopping, device);
    }

    indexSuperframe = createEntityIndex(device->address32, EntityType::Superframe, DEFAULT_MANAGEMENT_SUPERFRAME_ID);
    if (!device->existsSuperframeIndex(indexSuperframe ))
    {
        PhySuperframe* allocatedSuperframe = new PhySuperframe();
        allocatedSuperframe->index = DEFAULT_MANAGEMENT_SUPERFRAME_ID;
        allocatedSuperframe->tsDur = subnetSettings.getTimeslotLength();
        allocatedSuperframe->chIndex = Tdma::ChannelHopping::HOPPING_8;
        allocatedSuperframe->chBirth = 0;
        allocatedSuperframe->sfPeriod = subnetSettings.getSlotsPer30Sec();
        allocatedSuperframe->sfBirth = subnetSettings.superframeBirth;
        allocatedSuperframe->chRate = SUPERFRAME_ADVERTISE_CHANNEL_RATE;
        allocatedSuperframe->sfType = 0;
        allocatedSuperframe->priority = 0;
        allocatedSuperframe->idleUsed = 0;
        allocatedSuperframe->idleTimer = 0;
        allocatedSuperframe->rndSlots = 0;

        NE::Model::Operations::IEngineOperationPointer writeSuperframe(new NE::Model::Operations::WriteAttributeOperation(allocatedSuperframe, indexSuperframe));
        container.addOperation(writeSuperframe, device);
    }
}


NE::Model::Operations::IEngineOperationPointer LinkEngine::createMngLink( Device* device,
            Operations::OperationsContainer& container,
            Uint16 superframeId,
            LinkType::LinkTypesEnum linkType,
            TdmaLinkTypes::TdmaLinkTypesEnum tdmaLinkType,
            TdmaLinkDir::TdmaLinkDirEnum linkDirection,
            Uint16 schOffset,
            Uint16 schedInterval,
            Uint8 chOffset,
            Address16 address) {

    PhyLink*  managementLink;
    managementLink = createLink(device->getNextLinkID()
                                , superframeId
                                , linkType
                                , tdmaLinkType
                                , linkDirection
                                , schOffset
                                , schedInterval
                                , chOffset);

    if (!device->capabilities.isRouting() && !device->capabilities.isBackbone() ){
        managementLink->priority = PRIORITY_LINK_NON_ROUTING;
    }
    managementLink->neighborType = ( address ? NeighbourTypes::NEIGHBOUR_BY_ADDRESS : NeighbourTypes::NONE  );
    managementLink->neighbor = address;

    EntityIndex indexLinkMng = createEntityIndex(device->address32, EntityType::Link, managementLink->index);
    NE::Model::Operations::IEngineOperationPointer writeLinkMng(new NE::Model::Operations::WriteAttributeOperation(managementLink, indexLinkMng));

    return writeLinkMng;
}

void  LinkEngine::addNewInboundEdge(Subnet::PTR subnet,
                             Operations::OperationsContainer& container,
                             Device* device,
                             Device* parent,
                             NE::Model::Operations::OperationsList& generatedLinks) {

    ManagementLinksUtils linksUtils;

    PhyLink * linkTxDevice = device->getFirstTxMngLink(Address::getAddress16(parent->address32), TdmaLinkDir::INBOUND, Tdma::TdmaLinkTypes::MANAGEMENT);
    if (linkTxDevice) {//if allready exist a link do not reserve more, it have enough
        //needed because reserve is called from EvaluateGraph also when nothing changed on edges, and cause to reserve up to max chunks limit.
        LOG_WARN("Already exist INB mng link between device:" << Address_toStream(device->address32)
                    << " -> parent:" << Address_toStream(parent->address32));

    } else if( reserveMngChunkInbound(container, subnet, device, parent, false, linksUtils ) ) {
        addMngInboundLinks(device, parent, container, TdmaLinkTypes::MANAGEMENT, linksUtils, DEFAULT_MANAGEMENT_SUPERFRAME_ID, generatedLinks, subnet->getSubnetSettings());
    }
}

void  LinkEngine::addNewOutboundEdge(Subnet::PTR subnet,
                             Operations::OperationsContainer& container,
                             Device* parent,
                             Device* device,
                             NE::Model::Operations::OperationsList& generatedLinks) {

    ManagementLinksUtils linksUtils;

    PhyLink * linkTxParent = parent->getFirstTxMngLink(Address::getAddress16(device->address32), TdmaLinkDir::OUTBOUND, Tdma::TdmaLinkTypes::MANAGEMENT);
    if (linkTxParent) {//if allready exist a link do not reserve more, it have enough
        //needed because reserve is called from EvaluateGraph also when nothing changed on edges, and cause to reserve up to max chunks limit.
        LOG_WARN("Allready exist OUT mng link between parent:" << Address_toStream(parent->address32) << " -> dev:" << Address_toStream(device->address32));

    } else if( reserveMngChunkOutbound(container,subnet, parent, device, false, linksUtils.offset, linksUtils ) ) {
        std::vector<IEngineOperationPointer> generatedOperations;
        addMngOutboundLinks(parent, device, container, TdmaLinkTypes::MANAGEMENT, linksUtils,  DEFAULT_MANAGEMENT_SUPERFRAME_ID, generatedOperations);
    }
}

void LinkEngine::createAccelerationOUT(Device* parent, Device* device, Operations::OperationsContainer& container,
            ManagementLinksUtils &linksUtils, EntityIndex &indexSuperframe, const NE::Common::SubnetSettings& subnetSettings) {

    if (subnetSettings.accelerationInterval == 0){
        return;
    }

    if (!device->capabilities.isRouting() || parent->capabilities.isBackbone()){
        return;//for IO devices not permit acceleration on IN.  Devices with BBR as parent have link at 1 s by default.
    }

    // Accelerated RX
    device->setAcceleratedFlag();
    device->setLastTimeAccessed(time(NULL)); // not mandatory but to be safe if RX link is not send before ACC evaulation

    NE::Model::Operations::IEngineOperationPointer rxAcceleratedOperation;
    rxAcceleratedOperation = createAccMngLink(
                device,
                container,
                indexSuperframe,
                LinkType::RECEIVE,
                TdmaLinkDir::OUTBOUND,
                linksUtils.setNo,
                linksUtils.freqNo,
                0,
                subnetSettings);


    LOG_DEBUG("OutBound management boosting receive link on device" << *rxAcceleratedOperation << "entityIndex=" << rxAcceleratedOperation->getEntityIndex());
    container.addOperation(rxAcceleratedOperation, device);

    NE::Model::Operations::IEngineOperationPointer txAcceleratedOperation;
    txAcceleratedOperation = createAccMngLink(
                parent,
                container,
                indexSuperframe,
                LinkType::TRANSMIT,
                TdmaLinkDir::OUTBOUND,
                linksUtils.setNo,
                linksUtils.freqNo,
                Address::getAddress16(device->address32),
                subnetSettings);

    txAcceleratedOperation->addOperationDependency(rxAcceleratedOperation);

    LOG_DEBUG("OutBound management boosting receive link on device" << *txAcceleratedOperation << "entityIndex=" << txAcceleratedOperation->getEntityIndex());
    container.addOperation(txAcceleratedOperation, parent);

    PhyContract * managerDeviceContract = ContractsHelper::findContractSource2Destination(subnetsContainer->manager, device->address32, ContractTDSAP::TDSAP_SMAP - 0xF0B0, ContractTDSAP::TDSAP_DMAP - 0xF0B0);
    if (managerDeviceContract){
        PhyContract * newManagerDeviceContract = new PhyContract(*managerDeviceContract);
        if (device->capabilities.isRouting()){//change contract bandwidth only for routers. IO do not have acceleration on INbound
            newManagerDeviceContract->assignedCommittedBurst = 1;
        } else {
            newManagerDeviceContract->assignedCommittedBurst = -8;
        }
        EntityIndex entityIndex = createEntityIndex(newManagerDeviceContract->source32, EntityType::Contract, newManagerDeviceContract->contractID);
        IEngineOperationPointer contractOperation(new WriteAttributeOperation(newManagerDeviceContract, entityIndex));

        contractOperation->addOperationDependency(txAcceleratedOperation);

        container.addOperation(contractOperation, subnetsContainer->manager);
    }


}
void LinkEngine::createAccelerationIN(Device* parent, Device* device, Operations::OperationsContainer& container,
            ManagementLinksUtils &linksUtils, EntityIndex &indexSuperframe, const NE::Common::SubnetSettings& subnetSettings) {
    if (subnetSettings.accelerationInterval == 0){
        return;
    }

    if (!device->capabilities.isRouting() || parent->capabilities.isBackbone()){
        return;//for IO devices not permit acceleration on IN.  Devices with BBR as parent have link at 1 s by default.
    }

    // Accelerated RX
    device->setAcceleratedFlag();
    device->setLastTimeAccessed(time(NULL)); // not mandatory but to be safe if RX link is not send before ACC evaulation

    NE::Model::Operations::IEngineOperationPointer rxAcceleratedOperation;
    rxAcceleratedOperation = createAccMngLink(
                parent,
                container,
                indexSuperframe,
                LinkType::RECEIVE,
                TdmaLinkDir::INBOUND,
                linksUtils.setNo,
                linksUtils.freqNo,
                0,
                subnetSettings);


    LOG_DEBUG("InBound management boosting receive link on device" << *rxAcceleratedOperation << "entityIndex=" << rxAcceleratedOperation->getEntityIndex());
    container.addOperation(rxAcceleratedOperation, parent);

    NE::Model::Operations::IEngineOperationPointer txAcceleratedOperation;
    txAcceleratedOperation = createAccMngLink(
                device,
                container,
                indexSuperframe,
                LinkType::TRANSMIT,
                TdmaLinkDir::INBOUND,
                linksUtils.setNo,
                linksUtils.freqNo,
                Address::getAddress16(parent->address32),
                subnetSettings);

    txAcceleratedOperation->addOperationDependency(rxAcceleratedOperation);

    LOG_DEBUG("OutBound management boosting receive link on device" << *txAcceleratedOperation << "entityIndex=" << txAcceleratedOperation->getEntityIndex());
    container.addOperation(txAcceleratedOperation, device);


}

NE::Model::Operations::IEngineOperationPointer LinkEngine::createAccMngLink(
            Device* device,Operations::OperationsContainer& container,
            EntityIndex &indexSuperframe,
            LinkType::LinkTypesEnum  linkType,
            TdmaLinkDir::TdmaLinkDirEnum     linkDirection,
            Uint8 setNo,
            Uint8 chOffset,
            Address16 address,
            const SubnetSettings& subnetSettings) {

    PhyLink*  managementLink;
    managementLink = createLink(device->getNextLinkID()
                                , getIndex(indexSuperframe)
                                , linkType
                                , TdmaLinkTypes::MANAGEMENT_ACC
                                , linkDirection
                                , setNo
                                , subnetSettings.getSlotsPerSec()
                                , chOffset);

    managementLink->priority = PRIORITY_LINK_ACCELERATION;
    managementLink->neighborType = ( address ? NeighbourTypes::NEIGHBOUR_BY_ADDRESS : NeighbourTypes::NONE  );
    managementLink->neighbor = address;

    EntityIndex indexLinkMng = createEntityIndex(device->address32, EntityType::Link, managementLink->index);
    NE::Model::Operations::IEngineOperationPointer writeLinkMng(new NE::Model::Operations::WriteAttributeOperation(managementLink, indexLinkMng));



    return writeLinkMng;
}

void LinkEngine::destroyAccMngLink( Device* device, Device* parent1, Device * parent2, Operations::OperationsContainer& container, Subnet::PTR& subnet, Uint32 currentTime ) {

    if( device->getAcceleratedFlag() && !device->capabilities.isBackbone() )
    {
        if( (device->getLastTimeAccessed() + subnet->getSubnetSettings().accelerationInterval) < currentTime ) { // 60 seconds without SM control
            // destroy accelerated links
            bool removedOnOUT = destroyOUTAccMngLink( device, parent1, parent2, container, subnet );
            bool removedOnIN = destroyINAccMngLink( device, parent1, container, subnet );
            if (removedOnOUT && removedOnIN){
                device->unsetAcceleratedFlag();
            }
        }
    }
}


void LinkEngine::revertJoinLinksToDefault(){

}

bool LinkEngine::destroyOUTAccMngLink( Device* device, Device* parent1, Device * parent2, Operations::OperationsContainer& container, Subnet::PTR& subnet ) {
    bool linkRemoved = true;

    NE::Model::Operations::OperationsList listOfTransmitLinks;
    Address16 device16 = Address::getAddress16(device->address32);
    if( parent1 ) { // destroy TX acclereated link on parent1
        LinkIndexedAttribute::iterator itLink;
        for (itLink = parent1->phyAttributes.linksTable.begin(); itLink != parent1->phyAttributes.linksTable.end(); ++itLink) {

            PhyLink * link = itLink->second.getValue();

            if ( link && (link->neighbor == device16)//destroy OUT acceleration
                && (link->direction == TdmaLinkDir::OUTBOUND )
                && (link->type == LinkType::TRANSMIT)
                && (link->role == Tdma::TdmaLinkTypes::MANAGEMENT_ACC) ) {

                if( itLink->second.isPending ) { // don't destroy a pending link
                    linkRemoved = false;
                    continue;
                }
                // destroy link
                LOG_DEBUG("destroy link" << std::hex << itLink->first << "on" << parent1->address32);
                DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(itLink->first));
                container.addOperation(deleteLink, parent1);
                listOfTransmitLinks.push_back(deleteLink);
            }
        }
    }

    if( parent2 ) { // destroy TX acclereated link on parent2
        LinkIndexedAttribute::iterator itLink;
        for (itLink = parent2->phyAttributes.linksTable.begin(); itLink != parent2->phyAttributes.linksTable.end(); ++itLink) {

            PhyLink * linkTx = itLink->second.getValue();
            if ( linkTx &&   (linkTx->neighbor == device16)
                && (linkTx->direction == TdmaLinkDir::OUTBOUND )
                && (linkTx->type == LinkType::TRANSMIT)
                && (linkTx->role == Tdma::TdmaLinkTypes::MANAGEMENT_ACC) ) {

                if( itLink->second.isPending ) { // don't destroy a pending link
                    linkRemoved = false;
                    continue;
                }
                // destroy link
                LOG_DEBUG("destroy link" << std::hex << itLink->first << "on" << parent2->address32);
                DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(itLink->first));
                container.addOperation(deleteLink, parent2);
                listOfTransmitLinks.push_back(deleteLink);
            }
        }
    }

    // destroy RX acclereated link
//    device->setLastTimeAccessed(time(NULL)); // not mandatory but to be safe if RX link is not send before ACC evaulation
//    device->unsetAcceleratedFlag();

    if (linkRemoved){//remove Rx on device only if Tx was removed on parent
        LinkIndexedAttribute::iterator itLink;
        for (itLink = device->phyAttributes.linksTable.begin(); itLink != device->phyAttributes.linksTable.end(); ++itLink) {

            PhyLink * linkRx = itLink->second.getValue();
            if ( linkRx &&  (linkRx->direction == TdmaLinkDir::OUTBOUND )
                && (linkRx->type == LinkType::RECEIVE)
                && (linkRx->role == Tdma::TdmaLinkTypes::MANAGEMENT_ACC) ) {

                // destroy link
                if( !itLink->second.isPending ) { // don't destroy a pending link
                    LOG_DEBUG("destroy link" << std::hex << itLink->first << "on" << device->address32);
                    DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(itLink->first));
                    OperationDependency& dependencyList = deleteLink->getOperationDependency();
                    dependencyList.insert(dependencyList.end(), listOfTransmitLinks.begin(), listOfTransmitLinks.end());//send delete of Receive after delete of transmit
                    container.addOperation(deleteLink, device);
                } else {
                    linkRemoved = false;
                }

            }
        }
    }

    if (linkRemoved){
        //reset manager->device contract bandwidth to normal
        PhyContract * managerDeviceContract = ContractsHelper::findContractSource2Destination(subnetsContainer->manager, device->address32, ContractTDSAP::TDSAP_SMAP - 0xF0B0, ContractTDSAP::TDSAP_DMAP - 0xF0B0);
        if (managerDeviceContract){
            PhyContract * newManagerDeviceContract = new PhyContract(*managerDeviceContract);

            if (device->capabilities.isRouting()) {
                newManagerDeviceContract->assignedCommittedBurst = (-subnet->getSubnetSettings().mng_r_out_band);
            } else {
                newManagerDeviceContract->assignedCommittedBurst = (-subnet->getSubnetSettings().mng_s_out_band);
            }
          //  newManagerDeviceContract->assignedCommittedBurst = 1;
            EntityIndex entityIndex = createEntityIndex(newManagerDeviceContract->source32, EntityType::Contract, newManagerDeviceContract->contractID);
            IEngineOperationPointer contractOperation(new WriteAttributeOperation(newManagerDeviceContract, entityIndex));
            container.addOperation(contractOperation, subnetsContainer->manager);
        }
    }

    return linkRemoved;

}
bool LinkEngine::destroyINAccMngLink( Device* device, Device* parent, Operations::OperationsContainer& container, Subnet::PTR& subnet ) {

    if (parent == NULL){
        return false;
    }

    bool linkRemoved = true;

    NE::Model::Operations::OperationsList listOfTransmitLinks;
    Address16 parent16 = Address::getAddress16(parent->address32);
    int txSlotNumber = -1;

     // destroy TX acclereated link on device
    LinkIndexedAttribute::iterator itLink;
    for (itLink = device->phyAttributes.linksTable.begin(); itLink != device->phyAttributes.linksTable.end(); ++itLink) {

        PhyLink * link = itLink->second.getValue();

        if ( link && (link->neighbor == parent16)//destroy OUT acceleration
            && (link->direction == TdmaLinkDir::INBOUND )
            && (link->type == LinkType::TRANSMIT)
            && (link->role == Tdma::TdmaLinkTypes::MANAGEMENT_ACC) ) {

            if( itLink->second.isPending ) { // don't destroy a pending link
                linkRemoved = false;
                continue;
            }
            // destroy link
            LOG_DEBUG("destroy link" << std::hex << itLink->first << "on" << device->address32);
            DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(itLink->first));
            container.addOperation(deleteLink, device);
            listOfTransmitLinks.push_back(deleteLink);
            txSlotNumber = link->schedule.offset;
        }
    }
    if (txSlotNumber == -1){
        return linkRemoved;
    }

    // destroy RX acclereated link on parent
//    device->setLastTimeAccessed(time(NULL)); // not mandatory but to be safe if RX link is not send before ACC evaulation
//    device->unsetAcceleratedFlag();

    if (linkRemoved){//remove Rx on parent only if Tx was removed on device
        LinkIndexedAttribute::iterator itLinkRx;
        for (itLinkRx = parent->phyAttributes.linksTable.begin(); itLinkRx != parent->phyAttributes.linksTable.end(); ++itLinkRx) {

            PhyLink * linkRx = itLinkRx->second.getValue();
            if ( linkRx &&  (linkRx->direction == TdmaLinkDir::INBOUND )
                && (linkRx->type == LinkType::RECEIVE)
                && (linkRx->role == Tdma::TdmaLinkTypes::MANAGEMENT_ACC)
                && (linkRx->schedule.offset == txSlotNumber) ) {

                // destroy link
                if( !itLinkRx->second.isPending ) { // don't destroy a pending link
                    LOG_DEBUG("destroy link" << std::hex << itLinkRx->first << "on" << parent->address32);
                    DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(itLinkRx->first));
                    OperationDependency& dependencyList = deleteLink->getOperationDependency();
                    dependencyList.insert(dependencyList.end(), listOfTransmitLinks.begin(), listOfTransmitLinks.end());//send delete of Receive after delete of transmit
                    container.addOperation(deleteLink, parent);
                } else {
                    linkRemoved = false;
                }

            }
        }
    }
    return linkRemoved;

}

/**
 * Calculate a possible response time for a join response to arrive in device.
 * The join method used in calculation is "join confirm". For this request Sm must communicate with device 3-4 C/S request after
 * it sends the response. This request/response are sent by router to new device on the join Tx/Rx links.
 * @param router
 * @param subnet
 * @return
 */
NE::Model::Tdma::JoinTimeout::JoinTimeoutEnum LinkEngine::calculateJoinTimeout(Device * router, Subnet::PTR& subnet) {
    const Uint8 max_SM_requests = 4;
    const Int8 routerLevel = subnet->getDeviceLevel(router->address32);

    if (routerLevel < 0) {
        LOG_WARN("calculateJoinTimeout - invalid deviceLevel for " << Address_toStream(router->address32) << ". Timeout= "
                    << (int) JoinTimeout::TIMEOUT_08);
        return JoinTimeout::TIMEOUT_08;
    }

    int possibleResponseTime = max_SM_requests *
                ( subnet->getSubnetSettings().mng_link_r_in * routerLevel + subnet->getSubnetSettings().join_rxtx_period//inbount + Receive period
                + subnet->getSubnetSettings().mng_link_r_out * routerLevel + 1 // outbound + JR period
                );
//    if (possibleResponseTime <= 8){
//        return JoinTimeout::TIMEOUT_08;
//    } else if (possibleResponseTime <= 16){
//        return JoinTimeout::TIMEOUT_16;
//    } else
//        if (possibleResponseTime <= 32){
//        return JoinTimeout::TIMEOUT_32;
//    } else if (possibleResponseTime <= 64){
//        return JoinTimeout::TIMEOUT_64;
//    } else
        if (possibleResponseTime <= 128){
        return JoinTimeout::TIMEOUT_128;
    } else if (possibleResponseTime <= 256){
        return JoinTimeout::TIMEOUT_256;
    } else if (possibleResponseTime <= 512){
        return JoinTimeout::TIMEOUT_512;
    } else if (possibleResponseTime <= 1024){
        return JoinTimeout::TIMEOUT_1024;
    } else if (possibleResponseTime <= 2048){
        return JoinTimeout::TIMEOUT_2048;
    } else if (possibleResponseTime <= 4096){
        return JoinTimeout::TIMEOUT_4096;
    } else if (possibleResponseTime <= 8192){
        return JoinTimeout::TIMEOUT_8192;
    } else { //if (possibleResponseTime <= 16384){
        return JoinTimeout::TIMEOUT_16384;
    }
}

void LinkEngine::activateRouterRole(Device * router, Subnet::PTR& subnet, OperationsContainerPointer& container){

	LOG_DEBUG("Activation of router role : " << Address_toStream(router->address32));

	const SubnetSettings& subnetSettings = subnet->getSubnetSettings();

	AdvAlgorithmData * advData = obtainAdvAlgorithmData(router, subnet);
    if (advData == NULL) {
        LOG_WARN("No advertise slots are available. The router will not send advertise.");
        return;
    }

	Int8 deviceLevel = subnet->getDeviceLevel(router->address32);
    if (subnet->getSubnetSettings().maxNumberOfLayers > 0 && deviceLevel >= subnet->getSubnetSettings().maxNumberOfLayers) {
        LOG_INFO("Activation of router role (low advertise): device level(" << (int)deviceLevel << ") >= SubnetSettings::maxNumberOfLayers("
                    << (int)subnet->getSubnetSettings().maxNumberOfLayers << ")");
        advData->advertisePeriod = subnetSettings.getSlotsPer30Sec();
    }


    //-------------Create default superframe
    PhySuperframe * superframe = initialiseAdvertiseSuperframe(router, advData->superframeChannelBirth, subnet);

    EntityIndex indexSuperframe = createEntityIndex(router->address32, EntityType::Superframe, superframe->index);

    NE::Model::Operations::IEngineOperationPointer writeSuperframe(new NE::Model::Operations::WriteAttributeOperation(superframe, indexSuperframe));

    if (router->capabilities.isBackbone() && !subnet->getSubnetSettings().reduced_hopping.empty()) {
    	PhyChannelHopping * channelHopping = initialiseAdvertiseChannelHopping(subnet->getSubnetSettings());
        EntityIndex indexChannelHopping = createEntityIndex(router->address32, EntityType::ChannelHopping, Tdma::ChannelHopping::HOPPING_1);
        NE::Model::Operations::IEngineOperationPointer writeChHopping(
                    new NE::Model::Operations::WriteAttributeOperation(channelHopping, indexChannelHopping));
        writeSuperframe->addOperationDependency(writeChHopping);
        container->addOperation(writeChHopping, router);

    }
    container->addOperation(writeSuperframe, router);

    // -----------Write the DLMO.advSuperframe attribute
    PhyUint16 * advSuperframe = new PhyUint16();
    advSuperframe->value = superframe->index;
    EntityIndex indexAdvSuperframe = createEntityIndex(router->address32, EntityType::AdvSuperframe, 0);
    NE::Model::Operations::IEngineOperationPointer writeAdvSuperframe(new NE::Model::Operations::WriteAttributeOperation(advSuperframe, indexAdvSuperframe));
    container->addOperation(writeAdvSuperframe, router);


    //create Advertise Link
    PhyLink * linkAdvertise = createLink(router->getNextLinkID()
                                         ,superframe->index
                                         ,LinkType::ADVERTISEMENT
                                         ,TdmaLinkTypes::JOIN
                                         ,TdmaLinkDir::OUTBOUND
                                         ,advData->advertiseStartSlot
                                         ,advData->advertisePeriod
                                         ,advData->channelOffset
                                         );

    linkAdvertise->priority = PRIORITY_LINK_ADVERTISE; // increase the priority that a normal link
    if (advData->advertisePeriod >= subnetSettings.getSlotsPer30Sec()){
        linkAdvertise->schedType = NE::Model::Tdma::ScheduleType::OFFSET;
        linkAdvertise->schedule.interval = 0;
    } else {
        linkAdvertise->schedType = NE::Model::Tdma::ScheduleType::OFFSET_AND_INTERVAL;
        linkAdvertise->schedule.interval = advData->advertisePeriod;
    }

    EntityIndex indexLinkAdvertise = createEntityIndex(router->address32, EntityType::Link, linkAdvertise->index);
    NE::Model::Operations::IEngineOperationPointer writeLinkAdvertise(new NE::Model::Operations::WriteAttributeOperation(linkAdvertise, indexLinkAdvertise));
    container->addOperation(writeLinkAdvertise, router);

    //create Join Response Link
    PhyLink * linkJR = createLink(router->getNextLinkID()
                                  ,superframe->index
                                  ,LinkType::JOIN_RESPONSE
                                  ,TdmaLinkTypes::JOIN
                                  ,TdmaLinkDir::OUTBOUND
                                  ,advData->txStartSlot
                                  ,subnetSettings.getSlotsPerSec() // advData->tx_rx_period // max outbound
                                  ,0
                                  );

    EntityIndex indexLinkJR = createEntityIndex(router->address32, EntityType::Link, linkJR->index);
    NE::Model::Operations::IEngineOperationPointer writeLinkJR(new NE::Model::Operations::WriteAttributeOperation(linkJR, indexLinkJR));
    container->addOperation(writeLinkJR, router);

    //create Transmit Group 1 link - used during join
    PhyLink * linkTransmitGroup = createLink(router->getNextLinkID()
                                            , superframe->index
                                            , LinkType::TRANSMIT
                                            , TdmaLinkTypes::JOIN
                                            , TdmaLinkDir::OUTBOUND
                                            , advData->txStartSlot
                                            , subnetSettings.getSlotsPerSec() // advData->tx_rx_period // max outbound
                                            , 0
                                            );

    linkTransmitGroup->neighborType = NeighbourTypes::NEIGHBOUR_BY_GROUP;
    linkTransmitGroup->neighbor = GROUP_NEIGHBOR_JOIN;//used for group 1
    EntityIndex indexLinkTransmitGroup = createEntityIndex(router->address32, EntityType::Link, linkTransmitGroup->index);
    NE::Model::Operations::IEngineOperationPointer writeLinkTransmitGroup(new NE::Model::Operations::WriteAttributeOperation(linkTransmitGroup, indexLinkTransmitGroup));
    container->addOperation(writeLinkTransmitGroup, router);

    //create Receive Link
    PhyLink * linkReceive = createLink(router->getNextLinkID()
                                        , superframe->index
                                        , LinkType::RECEIVE
                                        , TdmaLinkTypes::JOIN
                                        , TdmaLinkDir::INBOUND
                                        , advData->rxStartSlot
                                        , subnetSettings.getSlotsPerSec() // created at max period. Will be reconfiguret to advData->tx_rx_period on remove acceleration
                                        , 0
                                        );

    EntityIndex indexLinkReceive = createEntityIndex(router->address32, EntityType::Link, linkReceive->index);
    NE::Model::Operations::IEngineOperationPointer writeLinkReceive(new NE::Model::Operations::WriteAttributeOperation(linkReceive, indexLinkReceive));
    container->addOperation(writeLinkReceive, router);

    PhyAdvJoinInfo * advJoinInfo = new PhyAdvJoinInfo();
    advJoinInfo->sendAdvRx = true;
    advJoinInfo->joinBackoff = 1;
//    advJoinInfo->joinTimeout = NE::Model::Tdma::JoinTimeout::TIMEOUT_64;
    advJoinInfo->joinTimeout = calculateJoinTimeout(router, subnet);
    advJoinInfo->advScheduleType = linkAdvertise->schedType;
    advJoinInfo->advRx.offset = linkAdvertise->schedule.offset;
    advJoinInfo->advRx.interval = subnetSettings.join_adv_period * subnetSettings.getSlotsPerSec(); // optimization for power consumption // linkAdvertise->schedule.interval;
    advJoinInfo->rxScheduleType = linkJR->schedType;
    advJoinInfo->joinRx = linkJR->schedule;
    advJoinInfo->txScheduleType = linkReceive->schedType;
    advJoinInfo->joinTx = linkReceive->schedule;
    EntityIndex indexAdvInfo = createEntityIndex(router->address32, EntityType::AdvJoinInfo, 0);
    NE::Model::Operations::IEngineOperationPointer writeAdvInfo(new NE::Model::Operations::WriteAttributeOperation(advJoinInfo, indexAdvInfo));
    writeAdvInfo->addEntityDependency(indexSuperframe);
    container->addOperation(writeAdvInfo, router);

    if (advData->advertisePeriod >= subnetSettings.getSlotsPer30Sec()){
        router->hasRoleActivated = RoleActivationStatus::ACTIVE_SLOW;
    } else {
        router->hasRoleActivated = RoleActivationStatus::ACTIVE;
    }

    delete advData;

    if (!router->capabilities.isBackbone()){//all except backbone are added
        //add router to list of routers to have adve period changed when gets full.
        subnet->listOfAdvertisingRouters.push_back(Address::getAddress16(router->address32));//add router to list of routers to have adve period changed when gets full.
    }
}

bool LinkEngine::changeAdvertiseLinkPeriod(Device * router, Uint16 desiredPeriodInSlots, Subnet::PTR& subnet, OperationsContainerPointer& container){

    const SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    //Find the advertise link
    LinkIndexedAttribute::iterator itLink = router->phyAttributes.linksTable.begin();
    for(; itLink != router->phyAttributes.linksTable.end(); ++itLink) {
        PhyLink * phyLink = itLink->second.getValue();
        if (phyLink
                    && phyLink->superframeIndex == DEFAULT_ADVERTISE_SUPERFRAME_ID
                    && phyLink->type == LinkType::ADVERTISEMENT
                    && phyLink->role == TdmaLinkTypes::JOIN
                    && phyLink->direction == TdmaLinkDir::OUTBOUND) {
            break;
        }
    }
    PhyLink * phyLink = NULL;
    if (itLink == router->phyAttributes.linksTable.end()) {
        LOG_WARN("changeAdvertiseLinkPeriod - No advertise link on router: " << Address_toStream(router->address32));
        return false;
    } else {
        phyLink = itLink->second.getValue();
        // overwrite the existing linkID
        if(phyLink == NULL) {
            LOG_WARN("changeAdvertiseLinkPeriod - No advertise link on router: " << Address_toStream(router->address32));
            return false;
        }
    }
    if (phyLink->schedule.interval == desiredPeriodInSlots
                || (desiredPeriodInSlots >= subnetSettings.getSlotsPer30Sec() && phyLink->schedule.interval == 0)){//period is the same no change needed
        return true;
    }

    LOG_INFO("changeAdvertiseLinkPeriod - " << Address_toStream(router->address32) << " per:" << (int)desiredPeriodInSlots << " link: " << (int) phyLink->index);
    PhyAdvJoinInfo * advJoinInfo = router->phyAttributes.advInfo.getValue();
    if (advJoinInfo == NULL){
        LOG_ERROR("Router with advInfo NULL: " << Address_toStream(router->address32));
        return false;
    }

//----- Update Advertise Link
    PhyLink * linkAdvertise = createLink(phyLink->index, //router->getNextLinkID(),
                                         DEFAULT_ADVERTISE_SUPERFRAME_ID, //superframe->index
                                         LinkType::ADVERTISEMENT,
                                         TdmaLinkTypes::JOIN,
                                         TdmaLinkDir::OUTBOUND,
                                         phyLink->schedule.offset,
                                         desiredPeriodInSlots,
                                         0
                                         );

    linkAdvertise->priority = PRIORITY_LINK_ADVERTISE; // increase the priority that a normal link
    if (desiredPeriodInSlots >= subnetSettings.getSlotsPer30Sec()){
        linkAdvertise->schedType = NE::Model::Tdma::ScheduleType::OFFSET;
        linkAdvertise->schedule.interval = 0;
        router->hasRoleActivated = RoleActivationStatus::ACTIVE_SLOW;
    } else {
        linkAdvertise->schedType = NE::Model::Tdma::ScheduleType::OFFSET_AND_INTERVAL;
        linkAdvertise->schedule.interval = desiredPeriodInSlots;
        router->hasRoleActivated = RoleActivationStatus::ACTIVE;
    }

    EntityIndex indexLinkAdvertise = createEntityIndex(router->address32, EntityType::Link, linkAdvertise->index);
    NE::Model::Operations::IEngineOperationPointer writeLinkAdvertise(new NE::Model::Operations::WriteAttributeOperation(linkAdvertise, indexLinkAdvertise));
//    LOG_INFO("updateAdvPeriod - add advertise link " << *linkAdvertise);
    container->addOperation(writeLinkAdvertise, router);

//------  Update advInfo with changes to advertise period
    advJoinInfo = new PhyAdvJoinInfo(*advJoinInfo);//create a copy required for modify attribute
    advJoinInfo->sendAdvRx = true;
    advJoinInfo->advScheduleType = linkAdvertise->schedType;
    advJoinInfo->advRx.offset = linkAdvertise->schedule.offset;
    advJoinInfo->advRx.interval = subnetSettings.join_adv_period * subnetSettings.getSlotsPerSec(); // optimization for power consumption // linkAdvertise->schedule.interval;

    EntityIndex indexAdvInfo = createEntityIndex(router->address32, EntityType::AdvJoinInfo, 0);
    NE::Model::Operations::IEngineOperationPointer writeAdvInfo(new NE::Model::Operations::WriteAttributeOperation(advJoinInfo, indexAdvInfo));
    //writeAdvInfo->addEntityDependency(indexSuperframe);
    LOG_INFO("changeAdvertiseLinkPeriod - add advJoinInfo " << *advJoinInfo);
    container->addOperation(writeAdvInfo, router);

    return true;
}

bool LinkEngine::updateAdvPeriod(Device * router, Subnet::PTR& subnet, OperationsContainerPointer& container){

    LOG_INFO("updateAdvPeriod - " << Address_toStream(router->address32));
    AdvAlgorithmData * advData = obtainAdvAlgorithmData(router, subnet);
    if (advData == NULL){
        LOG_WARN("No advertise slots are available. The router will not send advertise.");
        return false;
    }

    const SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    LinkIndexedAttribute::iterator itLink = router->phyAttributes.linksTable.begin();
    PhyLink * phyLink = NULL;
    for(; itLink != router->phyAttributes.linksTable.end(); ++itLink) {
        phyLink = itLink->second.getValue();
        if (phyLink
                    && phyLink->superframeIndex == DEFAULT_ADVERTISE_SUPERFRAME_ID
                    && phyLink->type == LinkType::ADVERTISEMENT
                    && phyLink->role == TdmaLinkTypes::JOIN
                    && phyLink->direction == TdmaLinkDir::OUTBOUND) {
            break;
        }
    }
    if (phyLink && (phyLink->schedType == ScheduleType::OFFSET || phyLink->schedule.interval >= subnetSettings.getSlotsPer30Sec())){
        return true;//for slow advertise routers do not update period.
    }

    Uint16 advlinkID;
    if (itLink == router->phyAttributes.linksTable.end()) {
        advlinkID = router->getNextLinkID();
        LOG_WARN("updateAdvPeriod - No advertise link on router: " << Address_toStream(router->address32));
        return false;
    } else {
        // overwrite the existing linkID
        if(itLink->second.getValue() == NULL) {
            advlinkID = router->getNextLinkID();
            LOG_WARN("updateAdvPeriod - No advertise link on router: " << Address_toStream(router->address32));
            return false;
        } else {
            advlinkID = itLink->second.getValue()->index;
//            router->phyAttributes.linksTable.erase(itLink);
            LOG_INFO("updateAdvPeriod - change adv linkID=" << (int)advlinkID);
        }
    }
    //create Advertise Link
    PhyLink * linkAdvertise = createLink(advlinkID, //router->getNextLinkID(),
                                         DEFAULT_ADVERTISE_SUPERFRAME_ID, //superframe->index
                                         LinkType::ADVERTISEMENT,
                                         TdmaLinkTypes::JOIN,
                                         TdmaLinkDir::OUTBOUND,
                                         advData->advertiseStartSlot,
                                         advData->advertisePeriod,
                                         0
                                         );

    linkAdvertise->priority = PRIORITY_LINK_ADVERTISE; // increase the priority that a normal link

    EntityIndex indexLinkAdvertise = createEntityIndex(router->address32, EntityType::Link, linkAdvertise->index);
    NE::Model::Operations::IEngineOperationPointer writeLinkAdvertise(new NE::Model::Operations::WriteAttributeOperation(linkAdvertise, indexLinkAdvertise));
    LOG_INFO("updateAdvPeriod - add advertise link " << *linkAdvertise);
    container->addOperation(writeLinkAdvertise, router);


    Schedule linkJRSchedule;
    linkJRSchedule.offset = advData->txStartSlot;
    linkJRSchedule.interval = subnetSettings.getSlotsPerSec();

    Schedule linkReceiveSchedule;
    linkReceiveSchedule.offset = advData->rxStartSlot;
    linkReceiveSchedule.interval = advData->tx_rx_period;

    PhyAdvJoinInfo * advJoinInfo = new PhyAdvJoinInfo();
    advJoinInfo->sendAdvRx = true;
    advJoinInfo->joinBackoff = 1;
//    advJoinInfo->joinTimeout = NE::Model::Tdma::JoinTimeout::TIMEOUT_64;
    advJoinInfo->joinTimeout = calculateJoinTimeout(router, subnet);
    advJoinInfo->advScheduleType = linkAdvertise->schedType;
    advJoinInfo->advRx.offset = linkAdvertise->schedule.offset;
    advJoinInfo->advRx.interval = subnetSettings.join_adv_period * subnetSettings.getSlotsPerSec(); // optimization for power consumption // linkAdvertise->schedule.interval;

    advJoinInfo->rxScheduleType = (linkJRSchedule.interval ? ScheduleType::OFFSET_AND_INTERVAL : ScheduleType::OFFSET);

    advJoinInfo->joinRx = linkJRSchedule;

    advJoinInfo->txScheduleType = (linkReceiveSchedule.interval ? ScheduleType::OFFSET_AND_INTERVAL : ScheduleType::OFFSET);

    advJoinInfo->joinTx = linkReceiveSchedule;

    EntityIndex indexAdvInfo = createEntityIndex(router->address32, EntityType::AdvJoinInfo, 0);
    NE::Model::Operations::IEngineOperationPointer writeAdvInfo(new NE::Model::Operations::WriteAttributeOperation(advJoinInfo, indexAdvInfo));
    LOG_INFO("updateAdvPeriod - add advJoinInfo " << *advJoinInfo);
    container->addOperation(writeAdvInfo, router);

    delete advData;

    if (advData->advertisePeriod >= subnetSettings.getSlotsPer30Sec()){
        router->hasRoleActivated = RoleActivationStatus::ACTIVE_SLOW;
    } else {
        router->hasRoleActivated = RoleActivationStatus::ACTIVE;
    }


    for(Address32 i = 0; i < MAX_NUMBER_OF_DEVICES; ++i) {
        Device * routerChild = subnet->getDevice(i);
        if (routerChild && routerChild->parent32 == router->address32 &&
                    router->phyAttributes.advInfo.getValue()) {

            PhyLink * linkAdvRx = new PhyLink();
            linkAdvRx->index = DEFAULT_LINK_ADV_RX_ID;
            linkAdvRx->superframeIndex = DEFAULT_SUPERFRAME_ID; //superframe->index;
            linkAdvRx->type = Tdma::LinkType::RECEIVE_ADAPTIVE;
            linkAdvRx->template1 = Tdma::TemplatesTypes::TEMPLATE_RECEIVE_SCANING;
            linkAdvRx->template2 = 0;
            linkAdvRx->role = Tdma::TdmaLinkTypes::DEFAULT;
            linkAdvRx->neighborType = Tdma::NeighbourTypes::NONE;
            linkAdvRx->graphType = Tdma::GraphTypes::NONE;
            linkAdvRx->schedType = router->phyAttributes.advInfo.getValue()->advScheduleType; // parentDevice->phyAttributes.advInfo.getValue()->advScheduleType;
            linkAdvRx->chType = Tdma::ChannelOffsetType::NONE;
            linkAdvRx->priorityType = Tdma::PriorityType::USE_SUPERFRAME_PRIORITY;
            linkAdvRx->neighbor = 0;
            linkAdvRx->graphID = 0;
            linkAdvRx->schedule = router->phyAttributes.advInfo.getValue()->advRx; // parentDevice->phyAttributes.advInfo.getValue()->advRx;
            linkAdvRx->chOffset = 0;
            linkAdvRx->priority = 0;
            EntityIndex indexLinkAdvRx = createEntityIndex(routerChild->address32, EntityType::Link, linkAdvRx->index);
            NE::Model::Operations::IEngineOperationPointer writeAdvRxLink(new NE::Model::Operations::WriteAttributeOperation(linkAdvRx, indexLinkAdvRx));
            LOG_INFO("updateAdvPeriod - add AdvRxLink " << *linkAdvRx);
            container->addOperation(writeAdvRxLink, routerChild);
        }
    }
    return true;
}

void LinkEngine::redirectChildParent(Subnet::PTR subnet, Device* child, Device * parent, NE::Model::Operations::OperationsContainerPointer& container) {
    if (parent->phyAttributes.advInfo.getValue() == NULL) {
        LOG_ERROR("For device " << std::hex << child->address32 << " selected new parent " << parent->address32 << " does not have advInfo configured.");
        return;
    }
    PhyLink * linkAdvRx = new PhyLink();
    linkAdvRx->index = DEFAULT_LINK_ADV_RX_ID;
    linkAdvRx->superframeIndex = DEFAULT_SUPERFRAME_ID; //superframe->index;
    linkAdvRx->type = Tdma::LinkType::RECEIVE_ADAPTIVE;
    linkAdvRx->template1 = Tdma::TemplatesTypes::TEMPLATE_RECEIVE_SCANING;
    linkAdvRx->template2 = 0;
    linkAdvRx->role = Tdma::TdmaLinkTypes::DEFAULT;
    linkAdvRx->neighborType = Tdma::NeighbourTypes::NONE;
    linkAdvRx->graphType = Tdma::GraphTypes::NONE;
    linkAdvRx->schedType = parent->phyAttributes.advInfo.getValue()->advScheduleType; // parentDevice->phyAttributes.advInfo.getValue()->advScheduleType;
    linkAdvRx->chType = Tdma::ChannelOffsetType::NONE;
    linkAdvRx->priorityType = Tdma::PriorityType::USE_SUPERFRAME_PRIORITY;
    linkAdvRx->neighbor = 0;
    linkAdvRx->graphID = 0;
    linkAdvRx->schedule = parent->phyAttributes.advInfo.getValue()->advRx; // parentDevice->phyAttributes.advInfo.getValue()->advRx;
    linkAdvRx->chOffset = 0;
    linkAdvRx->priority = 0;
    EntityIndex indexLinkAdvRx = createEntityIndex(child->address32, EntityType::Link, linkAdvRx->index);
    NE::Model::Operations::IEngineOperationPointer writeAdvRxLink(new NE::Model::Operations::WriteAttributeOperation(linkAdvRx, indexLinkAdvRx));
    LOG_INFO("redirectChildParent - add AdvRxLink " << *linkAdvRx);
    container->addOperation(writeAdvRxLink, child);

}

bool LinkEngine::ckReservedMngInbound(Subnet::PTR subnet, Device * device, Device * parent,ManagementLinksUtils &linkUtils, bool reserveForUdo)
{
    if( !device->capabilities.isRouting() && !reserveForUdo) // non router to router
    {
        BCList::iterator it = parent->theoAttributes.mngChunckInboundNonRouter.begin();
        for (; it != parent->theoAttributes.mngChunckInboundNonRouter.end(); ++it) {
            Uint16 offset = (*it).getFirstAvailableSlot();
            if( offset < linkUtils.period ) // chunk not full
            {
                linkUtils.setNo = (*it).setNo;
                if( !device->isUsedBcSetNumber(linkUtils.setNo) )
                {
                    linkUtils.freqNo = (*it).freqNo;
                    linkUtils.offset = offset;
                    (*it).reserveSlot(offset);
                    return true;
                }
            }
        }
    }

    return false;
}

bool LinkEngine::ckMngReservedOutbound(Subnet::PTR subnet, Device * parent, Device * device,ManagementLinksUtils &linkUtils, Uint8& chunksForRouter, Uint8& chunksForIO,  bool reserveForUdo)
{
	if (!device->capabilities.isBackbone() && device->capabilities.isRouting()) {
		for ( BCList::iterator it = parent->theoAttributes.mngChunckOutbound.begin(); it != parent->theoAttributes.mngChunckOutbound.end(); ++it) {
			if ( it->usedForRouter ) {
			    chunksForRouter++;
			}
		}
		if (chunksForRouter == 0) {
			return false;
		}
	} else if (!device->capabilities.isBackbone() && device->capabilities.isFieldDevice()) {
		for ( BCList::iterator it = parent->theoAttributes.mngChunckOutbound.begin(); it != parent->theoAttributes.mngChunckOutbound.end(); ++it) {
			if ( !it->usedForRouter ) {
			    chunksForIO++;
			}
		}
		if(chunksForIO == 0) {
			return false;
		}
	}

        if (device->capabilities.isNonRouting() && !reserveForUdo) {
            BCList::iterator favorableChunk = parent->theoAttributes.mngChunckOutbound.end();
            Uint16 offset = 0xFFFF;
            //find chunk with minimum offset. most free chunk
            BCList::iterator it;
            for (it = parent->theoAttributes.mngChunckOutbound.begin(); it != parent->theoAttributes.mngChunckOutbound.end(); ++it) {
                if (it->usedForRouter) {
                    continue;
                }

                Uint16 actualOffset = it->getFirstAvailableSlot();
                if ((actualOffset < offset) && (actualOffset < linkUtils.period ) && !device->isUsedBcSetNumber(it->setNo)) {
                    favorableChunk = it;
                    offset = actualOffset;
                    break;
                }
            }
            if (favorableChunk == parent->theoAttributes.mngChunckOutbound.end()){
                return false;
            }

            linkUtils.setNo = favorableChunk->setNo;
            linkUtils.freqNo = favorableChunk->freqNo;
            linkUtils.offset = offset;
            favorableChunk->reserveSlot(offset);
            return true;


        } else {

            BCList::iterator it;
            for (it = parent->theoAttributes.mngChunckOutbound.begin(); it != parent->theoAttributes.mngChunckOutbound.end(); ++it) {
                if ((device->capabilities.isRouting() && !it->usedForRouter) || (device->capabilities.isFieldDevice() && reserveForUdo && it->usedForRouter)){//skip chunk if isRouting an chunk is for IO
                    continue;
                }


                if (((device->capabilities.isRouting()  && it->usedForRouter)
                		|| ( reserveForUdo && device->capabilities.isFieldDevice()  && !it->usedForRouter)) && it->slotsReservationMap != 0) {
                    //use only free chunck for a router. Do not mix 2 routers on same chunck.
                    continue;
                }

                if ((device->capabilities.isRouting() || reserveForUdo)) {
                    linkUtils.setNo = it->setNo;
                    if( !device->isUsedBcSetNumber(linkUtils.setNo) ) {
                        linkUtils.freqNo = it->freqNo;
                        it->reserveSlot(0);
                        return true;
                    }
                }
            }
        }

    return false;
}

bool LinkEngine::reserveMngChunkInbound(Operations::OperationsContainer & operationsContainter,
            Subnet::PTR& subnet, Device * device, Device * parent, bool reserveForUdo, ManagementLinksUtils &linkUtils) {

    linkUtils.offset = 0;

    const SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    if (reserveForUdo) {
        linkUtils.period = 1;
    } else if (device->capabilities.isRouting()) {
        if( parent->capabilities.isBackbone() ) { // router to backbone -> allocate full slot
            linkUtils.period = 1;
        } else { // router to router -> not full band to protect router battery
            linkUtils.period = subnetSettings.mng_link_r_in;
        }
    } else { // non router to backbone or router
        linkUtils.period = subnetSettings.mng_link_s_in;
    }


    if (!ckReservedMngInbound(subnet, device, parent, linkUtils, reserveForUdo)) {
        // allocate new chunk
        if( !subnet->getAvailableMngChunckInbound(operationsContainter, device, parent, linkUtils.setNo, linkUtils.freqNo, subnetSettings.join_reserved_set) ) {
            LOG_ERROR("reserveMngChunkInbound(): Out of resources !!! dev:" << Address_toStream(device->address32) << " parent:" << Address_toStream(parent->address32));
            return false;
        }

        subnet->reserveMngChunck(linkUtils.setNo, linkUtils.freqNo, Address::getAddress16(parent->address32)); // owner is parent

        BandwidthChunck chunk( linkUtils.setNo, linkUtils.freqNo );
        chunk.reserveSlot( 0 ); // first slot for new chunk

        if ( device->capabilities.isRouting() ) {// inbound from a parent
            parent->theoAttributes.mngChunckInboundRouter.push_back( chunk );
        } else {            // inbound from a non router
            parent->theoAttributes.mngChunckInboundNonRouter.push_back( chunk );
        }
    }

    linkUtils.period *= subnetSettings.getSlotsPerSec(); // transform period from seconds to slots

    return true;
}


bool LinkEngine::reserveMngChunkOutbound(Operations::OperationsContainer & operationsContainter, Subnet::PTR& subnet,
                                        Device * parent,
                                        Device * device,
                                        bool reserveForUdo,
                                        Uint16 inboundOffset,
                                        ManagementLinksUtils &linkUtils) {

	LOG_DEBUG("OUT reserve " << Address_toStream(parent->address32) << "->" << Address_toStream(device->address32) << " INoffset:" << inboundOffset);

    if ( !device->capabilities.isBackbone() ){
        parent->incrementChildId();
    }

    const SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    if (reserveForUdo) {
        linkUtils.period = 1;
    } else if( device->capabilities.isRouting() || device->capabilities.isBackbone()) {

        linkUtils.period = subnetSettings.mng_link_r_out;
    } else { // non routing
        linkUtils.period = subnetSettings.mng_link_s_out;
    }



    linkUtils.offset = 0;

    Uint8 chunksForRouter = 0;
    Uint8 chunksForIO = 0;

    if(  !ckMngReservedOutbound(subnet, parent, device, linkUtils, chunksForRouter, chunksForIO, reserveForUdo) ) {

        // allocate new chunk
        if( !subnet->getAvailableMngChunckOutbound(operationsContainter, parent, device, linkUtils.setNo, linkUtils.freqNo, subnetSettings.join_reserved_set ) ) {
            LOG_ERROR("reserveMngChunkOutbound(): Out of resources !!! parent: " << Address_toStream(parent->address32) << " for dev:" << Address_toStream(device->address32));
            return false;
        }

        if (device->capabilities.isBackbone()) {
        	LOG_ERROR("BBR should allready have reserved chunks.");
        	return false;
        } else {
        	createAndReserveChunkOut(subnet, parent, device, linkUtils, device->capabilities.isRouting());
        }
    }
    linkUtils.period *= subnetSettings.getSlotsPerSec(); // transform to real offet

    return true;
}

void LinkEngine::createAndReserveChunkOut(Subnet::PTR& subnet,
										Device * parent,
										Device * device,
										ManagementLinksUtils &linkUtils,
										bool reservedForRouter){
	BandwidthChunck chunk( linkUtils.setNo, linkUtils.freqNo );

	subnet->reserveMngChunck(linkUtils.setNo, linkUtils.freqNo, Address::getAddress16(parent->address32)); // owner is parent

	if ( !device->capabilities.isBackbone() ){
	    chunk.reserveSlot(0);
	}
	chunk.usedForRouter = reservedForRouter;
	parent->theoAttributes.mngChunckOutbound.push_back( chunk );
    LOG_DEBUG("device=" << Address_toStream(parent->address32) << ", chunk=" << chunk
                << ", AFTER ADD chunk to mngChunckOutbound=" << parent->theoAttributes.mngChunckOutbound);

}


ResponseStatus::ResponseStatusEnum  LinkEngine::updateMngInboundLinks(Subnet::PTR subnet,
                                                                   Device * srcDevice,
                                                                   Address16 parent16,
                                                                   Operations::OperationsContainerPointer& container,
                                                                   float neededInbound) {

    Device * parent = subnet->getDevice(parent16);
    if( parent && !parent->capabilities.isBackbone() )
    {
        // check inbound traffic
        // compute the allocated traffic
        float allocatedLink = srcDevice->getAllocatedInboundLink( parent16, Tdma::TdmaLinkTypes::MANAGEMENT );

        if( allocatedLink < neededInbound ) {// under allocation -> need to increase
            if (srcDevice->statusForReports == StatusForReports::JOINED_AND_CONFIGURED && !parent->capabilities.isBackbone()){
                LOG_DEBUG("Increase for a device in joined configure state");
            }
            LOG_DEBUG("Need to increase mng IN between "
            		<< std::hex << Address::getAddress16(srcDevice->address32) << "->" << parent16
            		<< std::dec << " aloc:" << allocatedLink << " need:" << neededInbound);
            // try to increase any not full link
            PhyLink * linkTx = srcDevice->getNotFullTxMngLink( parent16, Tdma::TdmaLinkDir::INBOUND);
            if( linkTx ) { // pair link found -> increase the speed
                PhyLink * linkRx = parent->getPeerRxLink( linkTx );
                if( linkRx ) {
                    Uint16 newInterval = linkTx->schedule.interval;

                    // allignement
                    if( newInterval <= 200 ) {
                        newInterval = 100;
                    } else {
                        if(newInterval <= 400 ) {
                            newInterval = 200;
                        } else {
                            if( newInterval <= 800 ) {
                                newInterval = 400;
                            } else {
                                newInterval = 800;
                            }
                        }
                    }

                    NE::Model::Operations::OperationsList generatedLinks;
                    updateMngLinksPeriod( srcDevice, *linkTx, parent, *linkRx, newInterval, container, generatedLinks );
                    LOG_DEBUG("Increased IN: lnkIndex=" << std::dec << linkTx->index << " newInterval=" << newInterval);
                } else {
                    LOG_DEBUG("Increase ended because IN peer RX link not found" );
                }
            } else {
                LOG_DEBUG("Increase ended because IN peer TX link is full" );
                // need to add new chunk ... not on this release
            }
        } else if(  allocatedLink > ( neededInbound * 2 ) ) {// overallocation -> need to decrease
            if (allocatedLink > (subnet->getSubnetSettings().mng_link_r_in / 2) ) {// make sense to decrease
                // to decrese
                LOG_WARN("DECREASE needed for man IN: " << std::hex << Address::getAddress16(srcDevice->address32) << "->" << parent16
                            << std::dec << " aloc:" << allocatedLink << " need:" << neededInbound << " alloc: " << allocatedLink);

                PhyLink * linkTx = srcDevice->getFirstTxMngLink( Address::getAddress16(parent->address32), Tdma::TdmaLinkDir::INBOUND, Tdma::TdmaLinkTypes::MANAGEMENT);

                if( linkTx ) {
                    PhyLink * linkRx = parent->getPeerRxLink( linkTx );
                    if( linkRx ) {

                        Uint16 newInterval = SUPERFRAME_APPLICATION_LENGTH / neededInbound;

                        // allignement
                        if( newInterval <= 200 ){
                            newInterval = 100;
                        } else {
                            if(newInterval <= 400 ) {
                                newInterval = 200;
                            } else {
                                if( newInterval <= 800 ) {
                                    newInterval = 400;
                                } else {
                                    newInterval = 800;
                                }
                            }
                        }

                        NE::Model::Operations::OperationsList generatedLinks;
                        updateMngLinksPeriod( srcDevice, *linkTx, parent, *linkRx, newInterval, container, generatedLinks );
                        LOG_DEBUG("Decreased IN: lnkIndex=" << linkTx->index << " newInterval=" << newInterval);
                    } else {
                        LOG_ERROR("Increase ended because IN peer RX link not found" );
                    }
                } else  {
                    //it can arrive here also when is no mng link at all (on first eval of G1 shortly after join)
                    LOG_WARN("Increase ended because IN peer TX link is full" );
                    // need to add new chunk ... not on this release
                }

            }

        }

        // check outbound traffic
    }
   return ResponseStatus::SUCCESS;
}


ResponseStatus::ResponseStatusEnum  LinkEngine::updateMngOutboundLinks(Subnet::PTR subnet,
                                                                   Device * srcDevice,
                                                                   Address16 parent16,
                                                                   Operations::OperationsContainerPointer& container,
                                                                   float neededOutbound) {

    NE::Model::Operations::OperationsList generatedLinks;
    Device * parent = subnet->getDevice(parent16);
    if( parent ) {
        // check inbound traffic
        // compute the allocated traffic
        float allocatedLink = parent->getAllocatedOutboundLink( Address::getAddress16(srcDevice->address32), Tdma::TdmaLinkTypes::MANAGEMENT );

        if( allocatedLink < neededOutbound ) {// under allocation -> need to increase

            // increase also the outbound traffic
            LOG_DEBUG("Need to increase mng OUT between " << Address_toStream(parent16) << " -> " << Address_toStream(Address::getAddress16(srcDevice->address32))
                        << " N:" << neededOutbound << ", A:" << allocatedLink );
            PhyLink * linkTx = parent->getNotFullTxMngLink( Address::getAddress16(srcDevice->address32), Tdma::TdmaLinkDir::OUTBOUND);

            if( linkTx ) {
                PhyLink * linkRx = srcDevice->getPeerRxLink( linkTx );
                if( linkRx ) {
                    Uint16 newInterval = linkTx->schedule.interval;

                    // allignement
                    if( newInterval <= 200 ) {
                        newInterval = 100;
                    } else {
                        if(newInterval <= 400 ) {
                            newInterval = 200;
                        } else {
                            if( newInterval <= 800 ) {
                                newInterval = 400;
                            } else {
                                newInterval = 800;
                            }
                        }
                    }

                    updateMngLinksPeriod( parent, *linkTx, srcDevice, *linkRx, newInterval, container, generatedLinks );
                    LOG_DEBUG("Increased OUT: lnkIndex=" << linkTx->index << " newInterval=" << newInterval );
                } else {
                    LOG_ERROR("Increase ended because OUT peer RX link not found" );
                }
            } else  {
                //it can arrive here also when is no mng link at all (on first eval of G1 shortly after join)
                LOG_WARN("Increase ended because OUT peer TX link is full" );
                // need to add new chunk ... not on this release
            }
        } else if(  allocatedLink > ( neededOutbound * 2 ) ) {// overallocation -> need to decrease
            if (allocatedLink > (subnet->getSubnetSettings().mng_link_r_out / 2) ) {// make sense to decrease
                // to decrese
                LOG_WARN("DECREASE needed for man OUT: " << std::hex << Address::getAddress16(srcDevice->address32) << "->" << parent16
                                   << std::dec << " aloc:" << allocatedLink << " need:" << neededOutbound << " alloc: " << allocatedLink);

                PhyLink * linkTx = parent->getFirstTxMngLink( Address::getAddress16(srcDevice->address32), Tdma::TdmaLinkDir::OUTBOUND, Tdma::TdmaLinkTypes::MANAGEMENT);

                if( linkTx ) {
                    PhyLink * linkRx = srcDevice->getPeerRxLink( linkTx );
                    if( linkRx ) {

                        Uint16 newInterval = SUPERFRAME_APPLICATION_LENGTH / neededOutbound;

                        // allignement
                        if( newInterval <= 200 ){
                            newInterval = 100;
                        } else {
                            if(newInterval <= 400 ) {
                                newInterval = 200;
                            } else {
                                if( newInterval <= 800 ) {
                                    newInterval = 400;
                                } else {
                                    newInterval = 800;
                                }
                            }
                        }

                        updateMngLinksPeriod( parent, *linkTx, srcDevice, *linkRx, newInterval, container, generatedLinks );
                        LOG_DEBUG("Increased OUT: lnkIndex=" << linkTx->index << " newInterval=" << newInterval );
                    } else {
                        LOG_ERROR("Increase ended because OUT peer RX link not found" );
                    }
                } else  {
                    //it can arrive here also when is no mng link at all (on first eval of G1 shortly after join)
                    LOG_WARN("Increase ended because OUT peer TX link is full" );
                    // need to add new chunk ... not on this release
                }
            }
        }

        // check outbound traffic
    }
   return ResponseStatus::SUCCESS;
}

void  LinkEngine::createAppLinksPair(
                                     Device * source,
                                     Device * destination,
                                     Uint16 slot,
                                     Uint8 freq,
                                     Uint16 period,
                                     TdmaLinkDir::TdmaLinkDirEnum     linkDirection,
                                     TdmaLinkTypes::TdmaLinkTypesEnum linkRole,
                                     Operations::OperationsContainerPointer& container,
                                     PhyContract * contract,// default == NULL
                                     Device * realContractSource //default == NULL
                                     ){
    bool needToAddRx = ( !destination->capabilities.isBackbone() || (freq != 0) );

    Uint16 indexTransmit = source->getNextLinkID();
    PhyLink * linkTransmit = createLink(indexTransmit
                                        , DEFAULT_MANAGEMENT_SUPERFRAME_ID
                                        , LinkType::TRANSMIT
                                        , linkRole
                                        , linkDirection
                                        , slot
                                        , period
                                        , freq);
    linkTransmit->neighborType = NeighbourTypes::NEIGHBOUR_BY_ADDRESS;
    linkTransmit->neighbor = Address::getAddress16(destination->address32);
    EntityIndex entityIndexTransmit = createEntityIndex(source->address32, EntityType::Link, indexTransmit);
    Operations::IEngineOperationPointer opWriteTransmit(new Operations::WriteAttributeOperation(linkTransmit, entityIndexTransmit));
    if (contract && realContractSource){// are != NULL only for direct path allocation
        //add link<->contract association
        EntityIndex contractIndex = createEntityIndex(realContractSource->address32, EntityType::Contract, contract->contractID);
        realContractSource->theoAttributes.addLinkIndexForContract(contractIndex, entityIndexTransmit);
        source->theoAttributes.setContractIndexForLink(entityIndexTransmit, contractIndex);
    }

    if( needToAddRx )
    {
        Uint16 indexReceive = destination->getNextLinkID();
        PhyLink * linkReceive = createLink(indexReceive
                                        , DEFAULT_MANAGEMENT_SUPERFRAME_ID
                                        , LinkType::RECEIVE
                                        , linkRole
                                        , linkDirection
                                        , slot
                                        , period
                                        , freq
                                        );

        EntityIndex entityIndexReceive = createEntityIndex(destination->address32, EntityType::Link, indexReceive);
        Operations::IEngineOperationPointer opWriteReceive(new Operations::WriteAttributeOperation(linkReceive, entityIndexReceive));
        container->addOperation(opWriteReceive, destination);

        opWriteTransmit->addOperationDependency(opWriteReceive);

        if (contract && realContractSource){// are != NULL only for direct path allocation
            //add link<->contract association
            EntityIndex contractIndex = createEntityIndex(realContractSource->address32, EntityType::Contract, contract->contractID);
            realContractSource->theoAttributes.addLinkIndexForContract(contractIndex, entityIndexReceive);
            destination->theoAttributes.setContractIndexForLink(entityIndexReceive, contractIndex);
        }
    }

    container->addOperation(opWriteTransmit, source);

}

void  LinkEngine::updateMngLinksPeriod( Device * srcDevice,
                                      PhyLink & linkTx,
                                      Device * dstDevice,
                                      PhyLink & linkRx,
                                      Uint16 period,
                                      Operations::OperationsContainerPointer & container,
                                      NE::Model::Operations::OperationsList & generatedLinks) {

    PhyLink * newlinkTx = new PhyLink();
    memcpy( newlinkTx, &linkTx, sizeof(PhyLink) );

    PhyLink * newlinkRx = new PhyLink();
    memcpy( newlinkRx, &linkRx, sizeof(PhyLink) );

    newlinkTx->schedule.interval = period;
    newlinkRx->schedule.interval = period;

    EntityIndex entityIndexTx = createEntityIndex(srcDevice->address32, EntityType::Link, newlinkTx->index );
    Operations::IEngineOperationPointer opWriteTransmit(new Operations::WriteAttributeOperation(newlinkTx, entityIndexTx));


    EntityIndex entityIndexRx = createEntityIndex(dstDevice->address32, EntityType::Link, newlinkRx->index);
    Operations::IEngineOperationPointer opWriteReceive(new Operations::WriteAttributeOperation(newlinkRx, entityIndexRx));
    generatedLinks.push_back(opWriteTransmit);
    generatedLinks.push_back(opWriteReceive);
    container->addOperation(opWriteReceive, dstDevice);

    opWriteTransmit->addOperationDependency(opWriteReceive);

    container->addOperation(opWriteTransmit, srcDevice);

}

ResponseStatus::ResponseStatusEnum LinkEngine::allocateAppLinks(
                                PhyContract * contract,
                                PhyRoute * route,
                                Operations::OperationsContainerPointer& container){
    ResponseStatus::ResponseStatusEnum resultStatus = ResponseStatus::SUCCESS;

    Subnet::PTR subnet;
    Device* srcDevice = NULL;

    if (contract->source32 == ADDRESS16_GATEWAY || contract->source32 == ADDRESS16_MANAGER){//in case of source GW or SM use backbone as source
        subnet = subnetsContainer->getSubnet(contract->destination32);
        if (subnet == NULL){
            LOG_ERROR("No subnet found for " << Address_toStream(contract->destination32));
            return ResponseStatus::FAIL;
        }
        srcDevice = subnet->getBackbone();//use backbone from destination subnet
    } else {
        subnet = subnetsContainer->getSubnet(contract->source32);
        if (subnet == NULL){
            LOG_ERROR("No subnet found for " << Address_toStream(contract->destination32));
            return ResponseStatus::FAIL;
        }
        srcDevice = subnet->getDevice(contract->source32);//use the source of contract
    }
    Device* backbone = subnet->getBackbone();
    if(!backbone) {
       return  ResponseStatus::FAIL;
    }

    Device* dstDevice = subnetsContainer->getDevice(contract->destination32);
    if ( !srcDevice  || !dstDevice){
        LOG_ERROR("Unknown device for contract from " << Address_toStream(contract->source32) << " to " << Address_toStream(contract->destination32));
        return ResponseStatus::FAIL;
    }
    Device* realContractSource = subnet->getDevice(contract->source32);
    if (realContractSource == NULL){
        LOG_ERROR("Contract source is NULL: " << std::hex << contract->source32);
        return ResponseStatus::FAIL;
    }
    const SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    #warning "must be check for local loop between 2 subnets"

    if (contract->source32 == ADDRESS16_GATEWAY && contract->communicationServiceType == CommunicationServiceType::NonPeriodic) {
        Int16 period = -contract->assignedCommittedBurst;
        float traffic = 0.0;
        if( period > 0 )
            traffic =  (float)SUPERFRAME_APPLICATION_LENGTH / (period * subnetSettings.getSlotsPerSec());
        else
            traffic = (float)SUPERFRAME_APPLICATION_LENGTH * (-period) / subnetSettings.getSlotsPerSec();

        EntityIndex key = createEntityIndex(contract->source32, EntityType::Contract, contract->contractID);
        dstDevice->theoAttributes.addServedContract(key, traffic);
        LOG_INFO("Device " << Address_toStream(dstDevice->address32) << ", add/update served contract: " << std::hex << key << ", traffic=" << traffic);
    }

    if ((CommunicationServiceType::NonPeriodic == contract->communicationServiceType) &&
                (!(dstDevice->capabilities.isGateway())) &&
                (!(dstDevice->capabilities.isManager()))) {
        resultStatus = evaluateAppOutboundLinks(contract->destination32, subnet,  route,  container);
    } else {

            Uint16 graphID = 0;

            srcDevice->setDirtyInbound();

            subnet->markAllDevicesUnVisited();
            Uint16 nextSlot = 0;

            resultStatus = allocateAppInboundDirectLinks(subnet,
                                                        contract,
                                                        realContractSource,
                                                        srcDevice,
                                                        dstDevice,
                                                        route,
                                                        container,
                                                        graphID,  nextSlot);
            subnet->markAllDevicesUnVisited();
            if( !dstDevice->capabilities.isGateway() && !dstDevice->capabilities.isManager() && resultStatus == ResponseStatus::SUCCESS ) {

                nextSlot += 2;
                Int16 period = ( contract->communicationServiceType == CommunicationServiceType::Periodic ?
                                        contract->assignedPeriod
                                     : -contract->assignedCommittedBurst );
                float traffic = 0.0;
                if( period > 0 )
                    traffic =  (float)SUPERFRAME_APPLICATION_LENGTH / (period * subnetSettings.getSlotsPerSec());
                else
                    traffic = (float)SUPERFRAME_APPLICATION_LENGTH * (-period) / subnetSettings.getSlotsPerSec();

                EntityIndex key = createEntityIndex(contract->source32, EntityType::Contract, contract->contractID);
                dstDevice->theoAttributes.addServedContract(key, traffic);
                LOG_INFO("Device " << Address_toStream(backbone->address32) << ", add/update served contract: " << std::hex << key << ", traffic=" << traffic);

                resultStatus = evaluateAppOutboundLinks(contract->destination32,
                                                        subnet,
                                                        route,
                                                        container, nextSlot);
            }

    }

    return resultStatus;
}

ResponseStatus::ResponseStatusEnum LinkEngine::allocateAppInboundDirectLinks(
                                                    Subnet::PTR  subnet,
                                                    PhyContract * contract,
                                                    Device* realContractSource,
                                                    Device* srcDevice,
                                                    Device* dstDevice,
                                                    PhyRoute * route,
                                                    Operations::OperationsContainerPointer& container,
                                                    Uint16 &graphID, Uint16 &nextSlot) {

    Int16 period = ( contract->communicationServiceType == CommunicationServiceType::Periodic ?
                            contract->assignedPeriod
                         : -contract->assignedCommittedBurst );
    const NE::Common::SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    float traffic = 0.0;
    if( period > 0 ) {
        traffic =  (float)subnetSettings.getSuperframeLengthAPP() / (period * subnet->getSubnetSettings().getSlotsPerSec());
    } else {
        traffic = (float)subnetSettings.getSuperframeLengthAPP() * (-period) / subnet->getSubnetSettings().getSlotsPerSec();
    }

    if (abs(traffic) == 2.0)  {
        return ResponseStatus::SUCCESS;
    }

    DoubleExitEdges preferedEdges;
    searchPreferredEdges(dstDevice, route, subnet, preferedEdges, graphID);

    Uint16 startSlot;
    Uint16 maxSlotDelay;
    Uint16 periodInSlots;

    ModelUtils::getAppAvailableSlotsAndPeriod(contract, subnetSettings.getSuperframeLengthAPP(), startSlot, maxSlotDelay, periodInSlots);

    if( periodInSlots < 50 ) {
    	int nextStartSlot = startSlot + periodInSlots;
        periodInSlots = 50;
        ResponseStatus::ResponseStatusEnum response = createApplicationDirectLinks(subnet, contract, realContractSource,
        		srcDevice, container, preferedEdges, startSlot, maxSlotDelay, periodInSlots, nextSlot); // allocate twice
        if (response != ResponseStatus::SUCCESS) {
            return response;
        }
        startSlot = nextStartSlot;
    }

    return createApplicationDirectLinks(subnet, contract, realContractSource, srcDevice, container, preferedEdges, startSlot, maxSlotDelay, periodInSlots, nextSlot);
}
ResponseStatus::ResponseStatusEnum LinkEngine::createApplicationDirectLinks(
                                    Subnet::PTR subnet,
                                    PhyContract * contract,
                                    Device* realContractSource,
                                    Device* srcDevice,
                                    Operations::OperationsContainerPointer& container,
                                    DoubleExitEdges &preferedEdges,
                                    Uint16 currSlot,
                                    Uint16 maxSlotDelay,
                                    Uint16 periodInSlots,
                                    Uint16 &nextStartSlot ) {
    if( !srcDevice ) {
        return ResponseStatus::FAIL;
    }

    srcDevice->setVisited();

    Address16 srcDevice16 = Address::getAddress16(srcDevice->address32);

    DoubleExitEdges::const_iterator itPreferedEdge = preferedEdges.find(srcDevice16);
    if(itPreferedEdge == preferedEdges.end()) {
        if(srcDevice->capabilities.isBackbone()) {
            LOG_WARN("No edges found for " << Address_toStream(srcDevice->address32) << " for contract " << *contract);
            return ResponseStatus::SUCCESS;
        }
        else {
            LOG_ERROR("No edges found for " << Address_toStream(srcDevice->address32) << " for contract " << *contract);
            return ResponseStatus::FAIL;
        }
    }
    Device * edgeDestination1 = subnet->getDevice(itPreferedEdge->second.prefered);
    if ( edgeDestination1 == NULL ) {
        LOG_ERROR("Ends of an edge are NULL " << Address_toStream(srcDevice16) << "--" << Address_toStream(itPreferedEdge->second.prefered));
        return ResponseStatus::FAIL;
    }

    Address16 dstDevice16 = Address::getAddress16(edgeDestination1->address32);

    ResponseStatus::ResponseStatusEnum  response = subnet->checkMaxNumberOfLinksOnEdge( srcDevice16, dstDevice16, 1, container.get() );
    if ( response != ResponseStatus::SUCCESS ) {
        return response;
    }

    bool hasStartDelay = false;
    Uint16 currStartSlot =  currSlot;
    if(subnet->getSubnetSettings().appSlotsStartOffset != 0 && subnet->getSubnetSettings().appSlotsStartOffset < periodInSlots) {
        currSlot = currStartSlot + subnet->getSubnetSettings().appSlotsStartOffset;
        if(currSlot < periodInSlots) {
            hasStartDelay = true;
        }
        else {
            currSlot = currStartSlot + 2;
        }

    }
    else {
        currSlot = currStartSlot + 2;
    }


    //the contract paylod is added only on destination(in oerder to avoid double ading of a paylod).
    //The edgeDestination has Receive links and must suport this trafic for the transmits links (if is not the end of communication).
//        edgeDestination->theoAttributes.addServedContract(contractKey, usedSlots);

	Uint32 slotFreq = reserveFreeSlot(subnet->appSlots, currSlot, maxSlotDelay, periodInSlots, *srcDevice, *edgeDestination1, subnet->getSubnetSettings().getSuperframeLengthAPP(), subnet->getSubnetSettings().numberOfFrequencies);


	if( slotFreq == INVALID_FREE_SLOT ) {
	    if(hasStartDelay) {
	        currSlot = currStartSlot + 2;
	        slotFreq = reserveFreeSlot(subnet->appSlots, currSlot, maxSlotDelay, periodInSlots, *srcDevice, *edgeDestination1, subnet->getSubnetSettings().getSuperframeLengthAPP(), subnet->getSubnetSettings().numberOfFrequencies);
	    }
	    else {
	        return ResponseStatus::REFUSED_INSUFICIENT_RESOURCES;
	    }

	}

	if (slotFreq == INVALID_FREE_SLOT ) {
	    return ResponseStatus::REFUSED_INSUFICIENT_RESOURCES;
	}

	currSlot = getSlot(slotFreq);
	nextStartSlot = currSlot;

	createAppLinksPair(
						srcDevice,
						edgeDestination1,
						currSlot,
						getFreq(slotFreq),
						periodInSlots,
						TdmaLinkDir::INBOUND,
						TdmaLinkTypes::APPLICATION,
						container,
						contract,
						realContractSource);

	if (edgeDestination1->capabilities.isBackbone()){
		edgeDestination1->setVisited();
	} else if ( !edgeDestination1->isAlreadyVisited() ) {
		response = createApplicationDirectLinks(subnet, contract, realContractSource, edgeDestination1, container,  preferedEdges,  currSlot, maxSlotDelay, periodInSlots, nextStartSlot);
		if( response != ResponseStatus::SUCCESS ) {
		    return response;
		}
	}

    return ResponseStatus::SUCCESS;

}


Uint32 LinkEngine::retryReserveFreeSlot(Subnet::PTR & subnet, Device * srcDevice, Device * edgeDestination, Uint16 currSlot, Uint16 maxSlotDelay, Uint16 periodInSlots, Operations::OperationsContainerPointer& container) {

    Uint32 slotFreq = INVALID_FREE_SLOT;
    LOG_INFO("reserveFreeSlot : INVALID_FREE_SLOT - try to move slot...");
    const NE::Common::SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    // 1. get overlapped link
    LinkIndexedAttribute::iterator itLink = edgeDestination->phyAttributes.linksTable.end();
    for(itLink = edgeDestination->phyAttributes.linksTable.begin(); itLink != edgeDestination->phyAttributes.linksTable.end(); ++itLink) {
        if( itLink->second.getValue() && isAppLink( itLink->second.getValue()->role ) ) {
            PhyLink * link = itLink->second.getValue();
            if(!link) {
                continue;
            }
            int period2 = edgeDestination->getLinkPeriod(link);
            if (isOverlapping(currSlot, periodInSlots, link->schedule.offset, period2, subnetSettings.getSlotsPer30Sec())) {
                break;
            }
        }
    }

    if (itLink != edgeDestination->phyAttributes.linksTable.end()) {
        LOG_INFO("current slot=" << (int)currSlot << ", found overlapping link : " << std::hex << itLink->first);

        LinkToContractMap::iterator itLink2Contract = edgeDestination->theoAttributes.linkToContract.find(itLink->first);
        if (itLink2Contract != edgeDestination->theoAttributes.linkToContract.end()) {
            EntityIndex contractForLinkEntityIndex = itLink2Contract->second;
            Device * srcContractDevice = subnet->getDevice(getDeviceAddress(contractForLinkEntityIndex));

            if (srcContractDevice) {
                ContractIndexedAttribute::iterator itContract = srcContractDevice->phyAttributes.contractsTable.find(contractForLinkEntityIndex);
                PhyContract * lookingContract = itContract->second.getValue();
                if (lookingContract) {
                    LOG_INFO("lookingContract=" << *lookingContract);

                    // 2. search link from the currSlot;
                    ContractToLinksMap::iterator itContract2Links = srcContractDevice->theoAttributes.contractLinks.find(contractForLinkEntityIndex);
                    if (itContract2Links != srcContractDevice->theoAttributes.contractLinks.end()) {
                        LinksList linksList = itContract2Links->second;

                        Address16 brAddress16 = Address::getAddress16(edgeDestination->address32);
                        PhyLink * lookingLink = NULL;
                        Device * srcLinkDevice = NULL;
                        LinksList::iterator lookingLinkEntityIndex;
                        for(lookingLinkEntityIndex = linksList.begin(); lookingLinkEntityIndex != linksList.end(); ++lookingLinkEntityIndex) {
                            srcLinkDevice = subnet->getDevice(getDeviceAddress(*lookingLinkEntityIndex));
                            if (!srcLinkDevice) {
                                continue;
                            }
                            LinkIndexedAttribute::iterator itLinkDevice = srcLinkDevice->phyAttributes.linksTable.find(*lookingLinkEntityIndex);
                            if (itLinkDevice == srcLinkDevice->phyAttributes.linksTable.end()) {
                                continue;
                            }
                            PhyLink * linkDevice = itLinkDevice->second.getValue();
                            if (!linkDevice) {
                                continue;
                            }

                            if (brAddress16 == linkDevice->neighbor) {
                                lookingLink = linkDevice;
                                break;
                            }
                        }
                        if (lookingLink) {
                            LOG_INFO("lookingLink=" << *lookingLink);

                            // 3. move link to another slot
                            Uint16 startSlot;
                            Uint16 maxSlotDelay;
                            Uint16 periodInSlots;
                            ModelUtils::getAppAvailableSlotsAndPeriod(lookingContract, subnetSettings.getSuperframeLengthAPP(), startSlot, maxSlotDelay, periodInSlots);

                            Uint16 contractStopSlot = lookingContract->assignedPhase + lookingContract->assignedDeadline;
                            for (Uint16 contractCurrSlot = currSlot; contractCurrSlot < contractStopSlot; ++contractCurrSlot) {

                                // bool isFreeSlot(Uint16  slot, Uint16 period, Device& device)
                                if (isFreeSlot(contractCurrSlot, periodInSlots, *edgeDestination, subnetSettings.getSuperframeLengthAPP()) // isFreeSlot on BR
                                 && isFreeSlot(contractCurrSlot, periodInSlots, *srcLinkDevice, subnetSettings.getSuperframeLengthAPP())) { // isFreeSlot on Router(neighbor of BR)

                                    LOG_INFO("move lookingLink to a free slot found; slot=" << (int)contractCurrSlot);

                                    // a free slot was found; can move link on this slot
                                    PhyLink *linkCopy = new PhyLink(*lookingLink);
                                    linkCopy->schedule.offset = contractCurrSlot;
                                    IEngineOperationPointer operationLinkCopy(new WriteAttributeOperation(linkCopy, *lookingLinkEntityIndex));
                                    container->addOperation(operationLinkCopy, srcLinkDevice);

                                    // 4. reserveFreeSlot again
                                    LOG_INFO("reserveFreeSlot again");
                                    slotFreq = reserveFreeSlot(subnet->appSlots, currSlot, maxSlotDelay, periodInSlots, *srcDevice, *edgeDestination, subnetSettings.getSuperframeLengthAPP(), subnet->getSubnetSettings().numberOfFrequencies);

                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return slotFreq;
}

void LinkEngine::updateMngAndAppRedundantInboundLinks(
                                                        Subnet::PTR  subnet,
                                                        Uint16 graphId,
                                                        Operations::OperationsContainerPointer& container,
                                                        Uint32 currentTime) {
    GraphPointer graph = subnet->getGraph(graphId);
    if( graph )
    {
        const DoubleExitEdges & edges = graph->getGraphEdges();
        const SubnetSettings & subnetSettings = subnet->getSubnetSettings();

        for (DoubleExitEdges::const_iterator it = edges.begin(); it != edges.end(); ++it) {
            Device * device = subnet->getDevice(it->first);
            if( device && device->isDirtyInbound() && device->statusForReports == StatusForReports::JOINED_AND_CONFIGURED) {
            	Uint16 const comBandwidth = device->capabilities.isRouting() ? subnetSettings.mng_r_out_band : subnetSettings.mng_s_out_band;
                if (device->wasCommunicationInLastInterval(comBandwidth, currentTime)){
                    continue;
                }
                if( !device->capabilities.isBackbone() ) {

                    if (it->second.prefered == it->second.backup) {
                        LOG_ERROR("Prefered==Backup:" << std::hex << it->second.prefered <<" == " << it->second.backup
                                    << " Graph " << graphId
                                    << " content: " << NE::Model::Routing::GraphPrinter(subnet.get(), graph) );
                        continue;
                    }
                    // inbound redundant traffic
                    // compute needed inbound traffic
                    float neededInbound = subnet->getInboundAppTraffic( device, edges);

                    if ( ResponseStatus::FAIL == updateAppInboundLinks(subnet, device, it->second.prefered, container, neededInbound)) {
                       continue;
                    }

                    if ( ResponseStatus::FAIL == updateAppInboundLinks(subnet, device, it->second.backup , container, neededInbound)) {
                       continue;
                    }

                    if( device->capabilities.isRouting() ) {// increase routing bandwidth management

                        Uint32 children = subnet->getInboundChildrenNo( device );
                        Uint16 routingChildNo = (children >> 16) + 1;
                        Uint16 nonRoutingChildsNo = (children & 0x0000FFFF);

                        Device * parent = subnet->getDevice(device->parent32);
                        if( parent && !parent->capabilities.isBackbone() ) {
                            LOG_DEBUG("Eval backup&retry device level > 1");
                        }

                        // retry traffic will be set forced via Mng traffic
                        neededInbound = (neededInbound * subnetSettings.retry_proc)/100;

                        float neededOutbound = neededInbound; // consider retries on both sense ... but is not realy that

                        neededInbound += (float)nonRoutingChildsNo*SUPERFRAME_APPLICATION_LENGTH/subnetSettings.getSlotsPerSec()/subnetSettings.mng_s_in_band;
                        neededInbound += (float)routingChildNo*SUPERFRAME_APPLICATION_LENGTH/subnetSettings.getSlotsPerSec()/subnetSettings.mng_r_in_band;

                        neededOutbound += (float)nonRoutingChildsNo*SUPERFRAME_APPLICATION_LENGTH/subnetSettings.getSlotsPerSec()/subnetSettings.mng_s_out_band;
                        neededOutbound += (float)routingChildNo*SUPERFRAME_APPLICATION_LENGTH/subnetSettings.getSlotsPerSec()/subnetSettings.mng_r_out_band;

                        if ( ResponseStatus::FAIL == updateMngInboundLinks(subnet, device, it->second.prefered, container, neededInbound)) {
                           continue;
                        }

                        if ( ResponseStatus::FAIL == updateMngOutboundLinks(subnet, device, it->second.prefered, container, neededOutbound)) {
                           continue;
                        }

                        if ( ResponseStatus::FAIL == updateMngInboundLinks(subnet, device, it->second.backup, container, neededInbound)) {
                           continue;
                        }

                        if ( ResponseStatus::FAIL == updateMngOutboundLinks(subnet, device, it->second.backup, container, neededOutbound)) {
                           continue;
                        }
                    }
                }
                // reset dirty flag
                device->unsetDirtyInbound();
                if(!container->isContainerEmpty()){
                    //if operations was generated return: increase/evaluate only one device at a time
                    return;
                }
            }
        }
    }
}


ResponseStatus::ResponseStatusEnum LinkEngine::evaluateAppOutboundLinks(
                        Address32 destination32,
                        Subnet::PTR& subnet,
                        PhyRoute * route,
                        Operations::OperationsContainerPointer&  container, Uint16 startSlot /* = 0 */) {

    Device* destinationDevice = subnetsContainer->getDevice(destination32);
    if (destinationDevice == NULL){
        LOG_ERROR("Contract destination is NULL: " << Address_toStream(destination32));
        return ResponseStatus::FAIL;
    }

     Uint8 routeSize = route->route.size();
     Uint16 graphID = 0;

     if (routeSize == 1){
         //check if graph or device address
         Uint16 firstRouteElement = route->route[0];
         if (isRouteGraphElement(firstRouteElement)){
             graphID = getRouteElement(firstRouteElement);
         } else {
             return ResponseStatus::SUCCESS;
         }
     } else {
         if (1 == route->alternative) {
             Uint16 secondRouteElement = route->route[1];
             if (isRouteGraphElement(secondRouteElement)){
                 graphID = getRouteElement(secondRouteElement);
             } else {
                 return ResponseStatus::SUCCESS;
             }
         }
         else {
             LOG_ERROR("Default route was not evaluated yet ! Route is  hybrid!");
             return ResponseStatus::INAPPROPRIATE_PROCESS_MODE;
         }
     }

     Uint16 nextStartSlot =startSlot;
     if (graphID != 0) {
         GraphPointer graph = subnet->getGraph(graphID);
         if( graph == NULL) {
        	 LOG_ERROR("Graph is null: " << graph);
             return ResponseStatus::FAIL;
         }

         const DoubleExitEdges & graphEdges = graph->getGraphEdges();
         for( DoubleExitEdges::const_iterator it = graphEdges.begin(); it != graphEdges.end(); it++ ) {

             computeAppOutboundTraffic(subnet, container, it->first, graph, nextStartSlot);
         }

         computeAppOutboundTraffic(subnet, container, graph->getDestination(), graph, nextStartSlot);
     } else {
    	 LOG_ERROR("GraphID == 0 on allocation for destination " << Address_toStream(destination32));
     }

     return ResponseStatus::SUCCESS;
}


ResponseStatus::ResponseStatusEnum  LinkEngine::updateAppInboundLinks(Subnet::PTR& subnet,
                                                                       Device *  srcDevice,
                                                                       Address16 dstDevice16,
                                                                       Operations::OperationsContainerPointer& container,
                                                                       float neededInbound) {

    Device * dstDevice = subnet->getDevice( dstDevice16 );

    if( dstDevice == NULL){
        return ResponseStatus::SUCCESS;
    }
    bool hasAppBackupRxLnk = false;
    bool hasAppBackupTxLnk = false;

    //verify if there is a application_backup receive link on the source device/but not transmit

    LinkIndexedAttribute::iterator itLink;
       for (itLink = srcDevice->phyAttributes.linksTable.begin(); itLink != srcDevice->phyAttributes.linksTable.end(); ++itLink) {
           PhyLink * link = itLink->second.getValue();
           if ( link && (link->direction == TdmaLinkDir::INBOUND )
               && (link->role == Tdma::TdmaLinkTypes::APPLICATION_BACKUP) ) {
               if (link->type == LinkType::RECEIVE) {
                   hasAppBackupRxLnk = true;
               }

               else {
                   hasAppBackupTxLnk = true;
               }
           }
       }


       Uint16 startSlot =2;
   if((neededInbound == 0) && (hasAppBackupRxLnk && !hasAppBackupTxLnk)) {
       //if there are backup RXlinks , but no backup TxLinks on srcDevice ..and needed is 0 => create redundant TxLinks
       ResponseStatus::ResponseStatusEnum response = increaseAppRedundantLink(subnet, container, srcDevice, dstDevice, subnet->getSubnetSettings().mng_alloc_band * 100, TdmaLinkDir::INBOUND, startSlot) ;
       if( ResponseStatus::SUCCESS != response ) {
           return response;
       }
   }

   else {
        if (neededInbound == 0) {//when no app contract established yet
            return ResponseStatus::SUCCESS;
        }

        float allocated = srcDevice->getAllocatedInboundLink2Roles( dstDevice16, Tdma::TdmaLinkTypes::APPLICATION,
                                                                                 Tdma::TdmaLinkTypes::APPLICATION_BACKUP );

        float unallocated = neededInbound - allocated;
        LOG_INFO("LINK: APP IN " << std::hex << srcDevice->address32 << "->" << dstDevice->address32
                   << " N: " << std::dec << neededInbound << " A: " << allocated << " U: " << unallocated);

        if (unallocated > 2 ) { // under allocation that is not supported by mng links
           Uint16 period = (SUPERFRAME_APPLICATION_LENGTH / unallocated);
           if( period >= 1000 - 2 ) {
               period = 1000;
           }
           else if( period >= 100 - 2 ) {
               period -= (period%100); // multiple of seconds
           }
           else if( period >= 50 - 2 ) { // half a second
               period = 50;
           }
           else { // sub half a second
               period = 20;
           }

           ResponseStatus::ResponseStatusEnum response = increaseAppRedundantLink(subnet, container, srcDevice,
                       dstDevice, period, TdmaLinkDir::INBOUND, startSlot);
           if( ResponseStatus::SUCCESS != response ) {
               return response;
           }
        }
        else {
            float overallocated = -unallocated;

            if( (overallocated>2) && (allocated > 2)) { // over allocation is considered if more that 2 packets at 30 seconds

                LOG_INFO( "Reduce redundant link: allocated :" << allocated <<", needed: " << neededInbound
                            << " edge: " << std::hex << srcDevice->address32 << "->" << dstDevice->address32);
                Uint16 period = (SUPERFRAME_APPLICATION_LENGTH / overallocated + 2);
                decreaseAppRedundantLink(subnet, container, srcDevice, dstDevice, period, TdmaLinkDir::INBOUND );
            }
        }
   }

    if( !dstDevice->capabilities.isBackbone() )
        dstDevice->setDirtyInbound();

    return ResponseStatus::SUCCESS;
}

ResponseStatus::ResponseStatusEnum  LinkEngine::computeAppOutboundTraffic(
                                                                          Subnet::PTR& subnet,
                                                                          Operations::OperationsContainerPointer& container,
                                                                          Address16 deviceAddress16,
                                                                          GraphPointer & graph, Uint16& nextStartSlot) {
    Device* device = subnet->getDevice(deviceAddress16);

    if (device == NULL) {
        return ResponseStatus::FAIL;
    }

    Address16Set outEdgesTargets;
    graph->getOutBoundEdges(deviceAddress16, outEdgesTargets);

    for (Address16Set::iterator it = outEdgesTargets.begin(); it != outEdgesTargets.end(); ++it) {
       Device* dst = subnet->getDevice(*it);
       if( dst ) {
           // LOG_DEBUG("edgeTarget=" << std::hex << *it);
           float neededOutbound = subnet->getOutboundAppTraffic( dst);

           float allocated =  device->getAllocatedOutboundLink(*it,Tdma::TdmaLinkTypes::APPLICATION)
                            + device->getAllocatedOutboundLink(*it,Tdma::TdmaLinkTypes::APPLICATION_BACKUP);
           float unallocated = neededOutbound - allocated;
           LOG_INFO("LINK: APP OUT " << std::hex << device->address32 << "->" << dst->address32
                       << " N: " << std::dec << neededOutbound << " A: " << allocated << " U: " << unallocated);
           if (unallocated > 2) { // under allocation that is not supported by mng links
               Uint16 period = (SUPERFRAME_APPLICATION_LENGTH / unallocated);
               if( period >= 1000 ) {
                   period = 1000;
               }
               else if( period >= 100 ) {
                   period -= (period%100); // multiple of seconds
               }
               else {
                   period = 50; // half a second
               }
               ResponseStatus::ResponseStatusEnum response = increaseAppRedundantLink(subnet, container, device,
                           dst, period, TdmaLinkDir::OUTBOUND, nextStartSlot);
               if( ResponseStatus::SUCCESS != response ) {
                   return response;
               }
           } else {
               float overallocated = -unallocated;

               if( (overallocated>2) && (allocated > 2)) { // over allocation is considered if more that 2 packets at 30 seconds

                   LOG_INFO( "Reduce redundant link: allocated :" << allocated <<", needed: " << neededOutbound
                               << " edge: " << std::hex << device->address32 << "->" << dst->address32);
                   Uint16 period = (SUPERFRAME_APPLICATION_LENGTH / overallocated);
                   decreaseAppRedundantLink(subnet, container, device, dst, period, TdmaLinkDir::OUTBOUND );
               }
           }
       }
    }

    return ResponseStatus::SUCCESS;
}

ResponseStatus::ResponseStatusEnum LinkEngine::increaseAppRedundantLink(
                                                            Subnet::PTR subnet,
                                                            Operations::OperationsContainerPointer& container,
                                                            Device * source,
                                                            Device * destination,
                                                            Uint16 neededPeriod, TdmaLinkDir::TdmaLinkDirEnum linkDir, Uint16 &nextStartSlot) {
    if ( !source || !destination ) {
        return ResponseStatus::FAIL;
    }

    Address16 srcDevice16 = Address::getAddress16(source->address32);
    Address16 dstDevice16 = Address::getAddress16(destination->address32);

    ResponseStatus::ResponseStatusEnum  response = subnet->checkMaxNumberOfLinksOnEdge( srcDevice16, dstDevice16, 1, container.get() );
    if ( response != ResponseStatus::SUCCESS ) {
        return response;
    }

    const NE::Common::SubnetSettings& subnetSettings = subnet->getSubnetSettings();
    Uint16 maxSlotDelay = subnetSettings.getSlotsPer30Sec();


    bool hasStartDelay = false;
    Uint16 currStartSlot =  nextStartSlot;
    if(subnet->getSubnetSettings().appSlotsStartOffset != 0 && subnet->getSubnetSettings().appSlotsStartOffset < neededPeriod) {
        nextStartSlot = nextStartSlot + subnet->getSubnetSettings().appSlotsStartOffset;
        if(nextStartSlot < neededPeriod) {
            hasStartDelay = true;
        }
        else {
            nextStartSlot = currStartSlot;
        }

    }


    Uint32 slotFreq =  reserveFreeSlot(subnet->appSlots, nextStartSlot, maxSlotDelay,  neededPeriod, *source, *destination, subnetSettings.getSuperframeLengthAPP(), subnet->getSubnetSettings().numberOfFrequencies);

    if( slotFreq == INVALID_FREE_SLOT)
    {
        if(hasStartDelay) {
            nextStartSlot = currStartSlot;
            slotFreq =  reserveFreeSlot(subnet->appSlots, nextStartSlot, maxSlotDelay,  neededPeriod, *source, *destination, subnetSettings.getSuperframeLengthAPP(), subnet->getSubnetSettings().numberOfFrequencies);
        }
        else {
            return ResponseStatus::REFUSED_INSUFICIENT_RESOURCES;
        }
    }

    if( slotFreq == INVALID_FREE_SLOT) {
        return ResponseStatus::REFUSED_INSUFICIENT_RESOURCES;
    }


    nextStartSlot = getSlot(slotFreq) ;
    createAppLinksPair(source,
                        destination,
                        getSlot(slotFreq),
                        getFreq(slotFreq),
                        neededPeriod,
                        linkDir,
                        TdmaLinkTypes::APPLICATION_BACKUP,
                        container);

    return ResponseStatus::SUCCESS;
}

void LinkEngine::deleteAppRedundantLink(Subnet::PTR subnet,
                                            Operations::OperationsContainerPointer& container,
                                            Device * source,
                                            Device * destination,
                                            TdmaLinkDir::TdmaLinkDirEnum linkDir) {
    if(!source || !destination) {
        return;
    }

    LinkIndexedAttribute::iterator itDevLink = source->phyAttributes.linksTable.begin();
    for (; itDevLink != source->phyAttributes.linksTable.end(); ++itDevLink) {
        PhyLink* link = (PhyLink*) itDevLink->second.getValue();
        if(!link) {
            continue;
        }

        if (link->neighborType == Tdma::NeighbourTypes::NEIGHBOUR_BY_ADDRESS
                    && (Address::getAddress16(destination->address32) == link->neighbor)
                    && (link->type == Tdma::LinkType::TRANSMIT)
                    && (link->direction == linkDir)
                    &&(link->role ==  Tdma::TdmaLinkTypes::APPLICATION_BACKUP)) {
            // destroy reservation
            subnet->unreserveLink(source, link,  container.get());
            DeleteAttributeOperationPointer deleteLink(new DeleteAttributeOperation(itDevLink->first));
            container->addOperation(deleteLink, source);
        }

    }
}

void  LinkEngine::decreaseAppRedundantLink( Subnet::PTR& subnet,
                                              Operations::OperationsContainerPointer& container,
                                              Device * source,
                                              Device * destination,
                                              Uint16 extraPeriod, TdmaLinkDir::TdmaLinkDirEnum linkDir) {

    Address16 peerDevice = Address::getAddress16( destination->address32 );

    LinkIndexedAttribute::iterator itLink;
    for (itLink = source->phyAttributes.linksTable.begin(); itLink != source->phyAttributes.linksTable.end(); ++itLink) {

        PhyLink * linkTx = itLink->second.getValue();
        if (  linkTx &&  (linkTx->neighbor == peerDevice)
            && (linkTx->direction == linkDir )
            && (linkTx->type == NE::Model::Tdma::LinkType::TRANSMIT)
            && (linkTx->role == TdmaLinkTypes::APPLICATION_BACKUP) ) {

            if( linkTx->schedule.interval >= extraPeriod ) {
//                extraPeriod -= linkTx->schedule.interval;
                LOG_INFO("LINK: destroy extPer:" << extraPeriod
                            << " Idx:" << std::hex << itLink->first
                            << " Off: " << std::dec << linkTx->schedule.offset
                            << " Int:" << linkTx->schedule.interval
                            << " Dir:" << linkTx->direction);

                // destroy reservation
                subnet->unreserveLink(source, linkTx);

                // remove app links pair
                DeleteAttributeOperationPointer deleteTxLink(new DeleteAttributeOperation(itLink->first));
                container->addOperation(deleteTxLink, source);

                //
                if( !destination->capabilities.isBackbone() ) {
                    LinkIndexedAttribute::iterator itLinkRx;
                    for (itLinkRx = destination->phyAttributes.linksTable.begin(); itLinkRx != destination->phyAttributes.linksTable.end(); ++itLinkRx) {

                        PhyLink * linkRx = itLinkRx->second.getValue();
                        if (    (linkRx->direction == linkTx->direction )
                            && (linkRx->type == NE::Model::Tdma::LinkType::RECEIVE)
                            && (linkRx->role == linkTx->role)
                            && (linkRx->chOffset == linkTx->chOffset)
                            && (linkRx->schedule.interval == linkTx->schedule.interval)
                            && (linkRx->schedule.offset == linkTx->schedule.offset) ) {

                                DeleteAttributeOperationPointer deleteRxLink(new DeleteAttributeOperation(itLinkRx->first));
                                deleteRxLink->addOperationDependency( deleteTxLink );
                                container->addOperation(deleteRxLink, destination);
                        }
                    }
                }
                break;
            }
        }
    }
}


NE::Model::Operations::IEngineOperationPointer
LinkEngine::addMngInboundLinks(Device* device, Device* parent, Operations::OperationsContainer& container, TdmaLinkTypes::TdmaLinkTypesEnum tdmaLinkType,
            ManagementLinksUtils &linksUtils, Uint16 superframeId, NE::Model::Operations::OperationsList& createdLinks, const SubnetSettings& subnetSettings) {

    Uint16 schedOffest = linksUtils.setNo + linksUtils.offset * subnetSettings.getSlotsPerSec();

    //1. reception link on parent
    bool existsLink = parent->capabilities.isBackbone();

    if( !existsLink )
    {
        LinkIndexedAttribute::iterator itLink;
        for (itLink = parent->phyAttributes.linksTable.begin(); itLink != parent->phyAttributes.linksTable.end(); ++itLink) {

            const PhyLink * link = itLink->second.getValue();
            if (  link &&  (link->direction == Tdma::TdmaLinkDir::INBOUND)
                && (link->schedule.offset == schedOffest)
                && ((link->schedule.interval == linksUtils.period) || (link->schedule.interval == 100))
                && (link->chOffset == linksUtils.freqNo )
                && (link->type == LinkType::RECEIVE)
                && (link->role == tdmaLinkType ) ) {

                existsLink = true;
                break;
            }
        }
    }

    NE::Model::Operations::IEngineOperationPointer inboundReceiveOperation;
    if(!existsLink) {
        inboundReceiveOperation = createMngLink(
                    parent,
                    container,
                    superframeId,
                    LinkType::RECEIVE,
                    tdmaLinkType,
                    TdmaLinkDir::INBOUND,
                    schedOffest,
                    linksUtils.period,
                    linksUtils.freqNo,
                    0);

        LOG_DEBUG("Inbound management receive link on parent: " << *inboundReceiveOperation
                    << ", entityIndex=" << std::hex << inboundReceiveOperation->getEntityIndex());
        container.addOperation(inboundReceiveOperation, parent);
        createdLinks.push_back(inboundReceiveOperation);
    }

    //2. transmit link on device
    NE::Model::Operations::IEngineOperationPointer inboundTransmitOperation;
    inboundTransmitOperation = createMngLink(
                device,
                container,
                superframeId,
                LinkType::TRANSMIT,
                tdmaLinkType,
                TdmaLinkDir::INBOUND,
                schedOffest,
                linksUtils.period,
                linksUtils.freqNo,
                Address::getAddress16(parent->address32));

    if(!existsLink) {
        inboundTransmitOperation->addOperationDependency(inboundReceiveOperation);
    }
    LOG_DEBUG("Inbound management transmit link on device: " << *inboundTransmitOperation
                << ", entityIndex=" << std::hex << inboundTransmitOperation->getEntityIndex());
    container.addOperation(inboundTransmitOperation, device);
    createdLinks.push_back(inboundTransmitOperation);

    return inboundTransmitOperation;
}

void LinkEngine::addMngOutboundLinks(Device* parent, Device* device, Operations::OperationsContainer& container,
             TdmaLinkTypes::TdmaLinkTypesEnum tdmaLinkType, ManagementLinksUtils &linksUtils, Uint16 superframeId,
             std::vector<IEngineOperationPointer> & generatedOperations) {


    Uint16 schedOffest = linksUtils.setNo + linksUtils.offset * 100;

    NE::Model::Operations::IEngineOperationPointer rxOperation;
    rxOperation = createMngLink(
                device,
                container,
                superframeId,
                LinkType::RECEIVE,
                tdmaLinkType,
                TdmaLinkDir::OUTBOUND,
                schedOffest,
                linksUtils.period,
                linksUtils.freqNo,
                0);

    LOG_DEBUG("OutBound management receive link on device: " << *rxOperation
                << ", entityIndex=" << std::hex << rxOperation->getEntityIndex());
    container.addOperation(rxOperation, device);
    generatedOperations.push_back(rxOperation);

    //2. transmit link on parent
    NE::Model::Operations::IEngineOperationPointer txOperation;
    txOperation = createMngLink(
                parent,
                container,
                superframeId,
                LinkType::TRANSMIT,
                tdmaLinkType,
                TdmaLinkDir::OUTBOUND,
                schedOffest,
                linksUtils.period,
                linksUtils.freqNo,
                Address::getAddress16(device->address32));

    txOperation->addOperationDependency(rxOperation);
    LOG_DEBUG("OutBound management transmit link on device: " << *txOperation
                << ", entityIndex=" << std::hex << txOperation->getEntityIndex());

     container.addOperation(txOperation, parent);
     generatedOperations.push_back(txOperation);

    // return txOperation;
}

void LinkEngine::allocateNeighborDiscoveryLinks(Device* device, Operations::OperationsContainerPointer& container, Subnet::PTR subnet) {

    RETURN_ON_NULL(device);

    EntityIndex indexSuperframe;
    EntityIndex indexChannelHopping;

    createNeighborDiscoveryChannelHopping(device, *container, indexChannelHopping, subnet->getSubnetSettings());
    createNeighborDiscoverySuperframe(device, *container, indexSuperframe, subnet->getSubnetSettings(), true);
    subnet->addToFastDiscovery(device, time(NULL));

    if(device->capabilities.isBackbone()) {


        //create 3 neighbor descovery links
        //1st Link
        NE::Model::Operations::IEngineOperationPointer writeLinkNeighDscvry;
        writeLinkNeighDscvry = createNeighborDiscoveryLink( device,
                    indexSuperframe, subnet->getSubnetSettings().slotsNeighborDiscovery[1]);
        container->addOperation( writeLinkNeighDscvry, device);

        //2nd Link

        writeLinkNeighDscvry.reset();
        writeLinkNeighDscvry = createNeighborDiscoveryLink( device,
                    indexSuperframe, subnet->getSubnetSettings().slotsNeighborDiscovery[2]);
        container->addOperation( writeLinkNeighDscvry, device);

        //3rd Link

        writeLinkNeighDscvry.reset();
        writeLinkNeighDscvry = createNeighborDiscoveryLink( device,
                    indexSuperframe, subnet->getSubnetSettings().slotsNeighborDiscovery[3]);
        container->addOperation( writeLinkNeighDscvry, device);
    }

    else {
        //create 4 neighbor descovery links
        //1st Link
        NE::Model::Operations::IEngineOperationPointer writeLinkNeighDscvry;
        writeLinkNeighDscvry = createNeighborDiscoveryLink( device,
                    indexSuperframe, subnet->getSubnetSettings().slotsNeighborDiscovery[0]);
        container->addOperation( writeLinkNeighDscvry, device);

        //2nd Link

        writeLinkNeighDscvry.reset();
        writeLinkNeighDscvry = createNeighborDiscoveryLink( device,
                    indexSuperframe, subnet->getSubnetSettings().slotsNeighborDiscovery[1]);
        container->addOperation( writeLinkNeighDscvry, device);

        //3rd Link

        writeLinkNeighDscvry.reset();
        writeLinkNeighDscvry = createNeighborDiscoveryLink( device,
                    indexSuperframe, subnet->getSubnetSettings().slotsNeighborDiscovery[2]);
        container->addOperation( writeLinkNeighDscvry, device);

        //3rd Link

        writeLinkNeighDscvry.reset();
        writeLinkNeighDscvry = createNeighborDiscoveryLink( device,
                    indexSuperframe, subnet->getSubnetSettings().slotsNeighborDiscovery[3]);
        container->addOperation( writeLinkNeighDscvry, device);

    }

}

void LinkEngine::createNeighborDiscoveryChannelHopping(Device* device,
                                                       Operations::OperationsContainer& container,
                                                       EntityIndex &indexChannelHopping,
                                                       NE::Common::SubnetSettings& subnetSettings) {

    indexChannelHopping = createEntityIndex(device->address32, EntityType::ChannelHopping, Tdma::ChannelHopping::HOPPING_9);
    if(!device->existsChannelHoppingIndex(indexChannelHopping)) {

        PhyChannelHopping*  channelHopping =  new PhyChannelHopping();

        channelHopping->index = Tdma::ChannelHopping::HOPPING_9;
        channelHopping->length = subnetSettings.neigh_disc_channel_list.size();

        for (int i = 0; i < channelHopping->length; ++i){
            channelHopping->seq.push_back(subnetSettings.neigh_disc_channel_list[i]);
        }

        NE::Model::Operations::IEngineOperationPointer writeChHopping(
                    new NE::Model::Operations::WriteAttributeOperation(channelHopping, indexChannelHopping));
        container.addOperation(writeChHopping, device);
    }

}

void LinkEngine::createNeighborDiscoverySuperframe(Device * device,Operations::OperationsContainer& container,EntityIndex &indexSuperframe,
            NE::Common::SubnetSettings& subnetSettings, bool makeDiscoveryFast ) {

        indexSuperframe = createEntityIndex(device->address32, EntityType::Superframe, DEFAULT_NEIGH_DISCOVERY_SUPERFRAME_ID);

        SuperframeIndexedAttribute::iterator superframeIt = device->phyAttributes.superframesTable.find(indexSuperframe);

        if ( superframeIt == device->phyAttributes.superframesTable.end() || superframeIt->second.getValue() == NULL) {//if not exists
            PhySuperframe* allocatedSuperframe =  new PhySuperframe();

            allocatedSuperframe->index = DEFAULT_NEIGH_DISCOVERY_SUPERFRAME_ID;
            allocatedSuperframe->tsDur = subnetSettings.getTimeslotLength();
            allocatedSuperframe->chIndex = Tdma::ChannelHopping::HOPPING_9;
            allocatedSuperframe->chBirth = 0;
//            allocatedSuperframe->sfPeriod = SUPERFRAME_NEIGH_DISCOVERY_LENGTH;
            allocatedSuperframe->sfPeriod = makeDiscoveryFast ? subnetSettings.superframeNeighborDiscoveryFastLength : subnetSettings.superframeNeighborDiscoveryLength;
            allocatedSuperframe->sfBirth = subnetSettings.superframeBirth;
            allocatedSuperframe->chRate = SUPERFRAME_ADVERTISE_CHANNEL_RATE;
//            allocatedSuperframe->chMapOv = 0;
//            allocatedSuperframe->chMap = DEFAULT_CHANNEL_MAP;
            allocatedSuperframe->sfType = 0;
            allocatedSuperframe->priority = 0;
            allocatedSuperframe->idleUsed = 0;
            allocatedSuperframe->idleTimer = 0;
            allocatedSuperframe->rndSlots = 0;

            NE::Model::Operations::IEngineOperationPointer writeSuperframe(new NE::Model::Operations::WriteAttributeOperation(allocatedSuperframe, indexSuperframe));
            container.addOperation(writeSuperframe, device);
        } else {
            PhySuperframe* allocatedSuperframe =  new PhySuperframe(*(superframeIt->second.getValue()));
            allocatedSuperframe->sfPeriod = makeDiscoveryFast ? subnetSettings.superframeNeighborDiscoveryFastLength : subnetSettings.superframeNeighborDiscoveryLength;

            NE::Model::Operations::IEngineOperationPointer writeSuperframe(new NE::Model::Operations::WriteAttributeOperation(allocatedSuperframe, indexSuperframe));
            container.addOperation(writeSuperframe, device);
        }
}

NE::Model::Operations::IEngineOperationPointer LinkEngine::createNeighborDiscoveryLink(Device* device,
            EntityIndex &indexSuperframe, Uint16 scheduleOffset) {
    PhyLink * neighborDiscLink = createLink(device->getNextLinkID(),
                                            getIndex(indexSuperframe),
                                            LinkType::RECEIVE,
                                            TdmaLinkTypes::NEIGHBOR_DISCOVERY,
                                            TdmaLinkDir::INBOUND,
                                            scheduleOffset,
                                            0,
                                            0);

    neighborDiscLink->priority = PRIORITY_LINK_NEIGHBOR_DISCOV; // RX

    EntityIndex indexLinkMng = createEntityIndex(device->address32, EntityType::Link, neighborDiscLink->index);
    NE::Model::Operations::IEngineOperationPointer writeLinkNeighDiscovery(new NE::Model::Operations::WriteAttributeOperation(neighborDiscLink, indexLinkMng));

    return writeLinkNeighDiscovery;
}

void LinkEngine::allocateAppDirectLinksForContractsAfterParentChage(Subnet::PTR  subnet,
                                                                    PhyContract * contract,
                                                                    Device* realContractSource,
                                                                    Device* srcDevice,
                                                                    Device* dstDevice,
                                                                    Uint16 startSlot,
                                                                    Operations::OperationsContainerPointer& container,
                                                                    DoubleExitEdges& preferedEdges) {


    Int16 period = ( contract->communicationServiceType == CommunicationServiceType::Periodic ?
                            contract->assignedPeriod
                         : -contract->assignedCommittedBurst );
    const NE::Common::SubnetSettings& subnetSettings = subnet->getSubnetSettings();
    float traffic = 0.0;
    if( period > 0 )
        traffic =  (float)SUPERFRAME_APPLICATION_LENGTH / (period * subnetSettings.getSlotsPerSec());
    else
        traffic = (float)SUPERFRAME_APPLICATION_LENGTH * (-period) / subnetSettings.getSlotsPerSec();

    if (abs(traffic) == 2.0)  {
        return ;
    }

    Uint16 maxSlotDelay;
    Uint16 periodInSlots;
    Uint16 nextSlot = 0;
    ModelUtils::getAppAvailableSlotsAndPeriod(contract, subnetSettings.getSuperframeLengthAPP(), startSlot, maxSlotDelay, periodInSlots);
    ResponseStatus::ResponseStatusEnum response;
    if( periodInSlots < 50 ) {
        int nextStartSlot = startSlot + periodInSlots;
        periodInSlots = 50;
        response = createApplicationDirectLinks(subnet, contract, realContractSource,
                srcDevice, container, preferedEdges, startSlot, maxSlotDelay, periodInSlots, nextSlot); // allocate twice
        if (response != ResponseStatus::SUCCESS){
            return ;
        }
        startSlot = nextStartSlot;
    }


    response = createApplicationDirectLinks(subnet, contract, realContractSource, srcDevice, container, preferedEdges, startSlot, maxSlotDelay, periodInSlots, nextSlot);
}

void LinkEngine::changeNeighborDiscovery(
                    Operations::OperationsContainer & operationsContainter,
                    Subnet::PTR & subnet,
                    Device * device,
                    bool makeFast){

    EntityIndex indexSuperframe;
    createNeighborDiscoverySuperframe(device, operationsContainter, indexSuperframe, subnet->getSubnetSettings(), makeFast);
}


}

}

}


