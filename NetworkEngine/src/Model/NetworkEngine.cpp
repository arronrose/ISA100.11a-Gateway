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
 * Main.cpp
 *
 *  Created on: May 14, 2008
 *      Author: catalin.pop, flori.parauan, eduard.budulea,beniamin.tecar, ioan.pocol, george.petrehus, sorin.bidian
 */

#include <boost/bind.hpp>
#include "NetworkEngine.h"
#include "Common/NETypes.h"
#include "SMState/SMStateLog.h"
#include "Model/ContractsHelper.h"
#include "Model/modelDefault.h"
#include "Model/AddressAllocator.h"
#include "Model/Operations/ReadAttributeOperation.h"
#include "Model/Operations/WriteAttributeOperation.h"
#include "Model/Operations/DeleteAttributeOperation.h"
#include "Model/Operations/AlertOperation.h"
#include "Model/ModelUtils.h"
#include "Model/ModelPrinter.h"
#include "Model/DeviceCapabilityHandler.h"
#include "Model/ChainManagementLinks.h"
#include "Model/ChainAddRouteToBeActivated.h"
#include "Model/ChainAddDeviceToRoleActivation.h"
#include "Model/ChainWaitForConfirmOnEvalGraph.h"

using namespace NE::Model::Operations;
using namespace NE::Model::Routing;
using namespace NE::Model::Reports;

namespace NE {
namespace Model {

string NetworkEngine::REGISTRATION_DEVICE = "RegDev";

#define FAIL_ON_NULL(pointer, address32, requestID, msg){\
    if (pointer == NULL){\
        LOG_ERROR(msg);\
        handlerResponse(address32, requestID, ResponseStatus::FAIL);\
        return;\
    }\
}

NetworkEngine::NetworkEngine(SettingsLogic * settingsLogic_) :
        startTime(time(NULL)), //
        operationsProcessor(subnetsContainer), //
        theoreticEngine(&subnetsContainer, &operationsProcessor, this, this), //
        reportsEngine(&subnetsContainer, settingsLogic_, startTime), //
        settingsLogic(settingsLogic_) {

    operationsProcessor.registerRemoveDeviceOnErrorCallback(this);
    setSettingsLogic(settingsLogic_); //should be renamed to initialize and use already existing member settingLogic.
}

NetworkEngine::~NetworkEngine() {
    LOG_DEBUG("NetworkEngine destroyed");
    //    startTime = time(NULL);
}

Uint32 NetworkEngine::getStartTime() {
    return startTime;
}

SettingsLogic& NetworkEngine::getSettingsLogic() {
    return *settingsLogic;
}

void NetworkEngine::updateAdvPeriod() {
    for (SubnetsMap::iterator it = subnetsContainer.getSubnetsList().begin(); it != subnetsContainer.getSubnetsList().end(); ++it) {
        it->second->setUpdateAdvPeriod(true);
    }
}

void createManager(const Address64& managerAddress64, const SettingsLogic& settingsLogic, SubnetsContainer& subnetsContainer) {

    Capabilities capabilities;
    capabilities.euidAddress = managerAddress64;
    capabilities.deviceType = DeviceType::MANAGER;
    capabilities.tagName = settingsLogic.managerTag.substr(0,16);
    subnetsContainer.manager = new Device(capabilities);

    subnetsContainer.manager->phyAttributes.modelID.currentValue = new PhyString(settingsLogic.managerModel.substr(0,16));
    subnetsContainer.manager->phyAttributes.serialNumber.currentValue = new  PhyString(settingsLogic.managerSerialNo.substr(0,16));
    subnetsContainer.manager->phyAttributes.vendorID.currentValue = new PhyString("NIVIS");
    subnetsContainer.manager->phyAttributes.softwareRevisionInformation.currentValue = new PhyString(NE::Common::SettingsLogic::managerVersion);
    subnetsContainer.manager->phyAttributes.packagesStatistics.currentValue = new PhyBytes();
    subnetsContainer.manager->phyAttributes.joinReason.currentValue = new PhyBytes();
    subnetsContainer.manager->phyAttributes.powerSupplyStatus.currentValue = new PhyUint8(0);

    subnetsContainer.manager->status = DeviceStatus::JOIN_CONFIRMED;
    subnetsContainer.manager->statusForReports = StatusForReports::JOINED_AND_CONFIGURED;
    subnetsContainer.manager->address32 = ADDRESS16_MANAGER;
    //    subnetsContainer.manager->address16 = ADDRESS16_MANAGER;
    subnetsContainer.manager->address128 = settingsLogic.managerAddress128;
    subnetsContainer.manager->joinConfirmTime = time(NULL);
    subnetsContainer.manager->startTime = subnetsContainer.manager->joinConfirmTime;

    // init manager metadata
    PhyMetaData defaultManagerMetadata;
    defaultManagerMetadata.used = 0x0000;
    defaultManagerMetadata.total = 0xFFFF;
    subnetsContainer.manager->phyAttributes.linkMetadata.currentValue = new PhyMetaData(defaultManagerMetadata);
    subnetsContainer.manager->phyAttributes.neighborMetadata.currentValue = new PhyMetaData(defaultManagerMetadata);
    subnetsContainer.manager->phyAttributes.graphMetadata.currentValue = new PhyMetaData(defaultManagerMetadata);

    // add contract Manager2Manager
    ContractRequest contractRequest;
    contractRequest.sourceAddress = subnetsContainer.manager->address32;
    contractRequest.destinationAddress = subnetsContainer.manager->address32;
    contractRequest.communicationServiceType = CommunicationServiceType::NonPeriodic;
    contractRequest.committedBurst = 1;
    contractRequest.excessBurst = 1;

    PhyContract * contract = ContractsHelper::createContract(contractRequest, subnetsContainer.manager, subnetsContainer.manager,
                settingsLogic);

    EntityIndex eIndex = createEntityIndex(subnetsContainer.manager->address32, EntityType::Contract, contract->contractID);
    subnetsContainer.manager->phyAttributes.createContract(eIndex, contract);
}

void NetworkEngine::setSettingsLogic(NE::Common::SettingsLogic* _settingsLogic) {
    try {
        settingsLogic = _settingsLogic;

        createManager(settingsLogic->managerAddress64, *_settingsLogic, subnetsContainer);
        Device * manager = subnetsContainer.manager;

        Subnet::PTR subnet;
        // create subnet topologies for all existing subnets
        int noElements = settingsLogic->subnetIds.size();
        for (int i = 0; i < noElements; i++) {
            LOG_DEBUG("Add manager to subnet " << (int) settingsLogic->subnetIds[i]);
            Uint16 subnetId = settingsLogic->subnetIds[i];
            subnet.reset(new Subnet(subnetId, manager, _settingsLogic));//must be used with reset. If created localy, Subnet instance will get destroyed.. don't know why.
            //must be used with reset. If created locally, Subnet instance will get destroyed.. don't know why.
            subnet->registerDeleteDeviceCallback(&operationsProcessor);
            subnetsContainer.addSubnet(subnet);
        }

    } catch (NEException& e) {
        LOG_ERROR("setSettingsLogic:" << e.what());
        throw e;
    }
}

NE::Model::PhyContract * NetworkEngine::findManagerContractToDevice(Address32 deviceAddress32, Uint16 tsapSrc, Uint16 tsapDest) {
    return ContractsHelper::findContractSource2Destination(subnetsContainer.manager, deviceAddress32, tsapSrc, tsapDest);
}

DeviceStatus::DeviceStatus NetworkEngine::getDeviceStatus(Address32 address32) {
    Device * device = getDevice(address32);
    if (device == NULL) {
        return DeviceStatus::NOT_JOINED;
    }
    return device->status;
}

Address32 NetworkEngine::createAddress32(Uint16 subnetId, Uint16 address16) {
    //return (subnetId << 16) + address16; //commented not to use multiple implementations
    Address32 address32 = NE::Common::Address::createAddress32(subnetId, address16);
    return address32;
}

Uint8 NetworkEngine::getDiagLevel(Address32 deviceAddress, Address32 neighborAddress) {
    Subnet::PTR subnet = getSubnet(deviceAddress);
    if (subnet == NULL){
        LOG_ERROR("Subnet NULL for " << Address_toStream(deviceAddress));
        return 0;
    }
    return subnet->getDiagLevel(deviceAddress, neighborAddress);
}

void NetworkEngine::addManagerNeighbor(const Address64& neighborAddress64, const Address128& neighborAddress128) {
    managerNeighborsMapping.insert(AddressMapping64_128::value_type(neighborAddress64, neighborAddress128));
}

void NetworkEngine::initialiseDefaultAttributes(NE::Model::Device * device, NE::Model::Device * parentDevice, Subnet::PTR& subnet) {
    assert(device && "Device is null");
    assert(parentDevice && "Parent device is null");
    if (parentDevice->phyAttributes.advInfo.getValue() == NULL) {
        LOG_ERROR("AdvInfo on parent is not configured. Parent:" << Address_toStream(parentDevice->address32) << ", dev=" << Address_toStream(device->address32));
        return;
    }
    const NE::Common::SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    /************
     * From table 122 in standard.
     */
    PhyNeighbor * neighbor = new PhyNeighbor();
    neighbor->index = Address::getAddress16(parentDevice->address32);
    neighbor->address64 = parentDevice->address64;
    neighbor->clockSource = 2;//prefered
    neighbor->diagLevel = 1;//colect link diag
    neighbor->extGrCnt = 0;
    neighbor->groupCode = 0;
    neighbor->linkBacklog = 0;
    neighbor->linkBacklogDur = 0;
    neighbor->linkBacklogIndex = 0;
    EntityIndex indexNeighbor = createEntityIndex(device->address32, EntityType::Neighbour, neighbor->index);
    device->phyAttributes.createNeighbor(indexNeighbor, neighbor);

    /************
     * From table 123 in standard.
     */
    PhyGraph * graph = new PhyGraph();
    graph->index = DEFAULT_GRAPH_ID;
    graph->preferredBranch = 0;
    graph->neighborCount = 1;
    Address16 address16 = Address::getAddress16(parentDevice->address32);
    graph->neighbors.push_back(address16);
    graph->maxLifetime = 0;
    graph->queue = 0;
    EntityIndex indexGraph = createEntityIndex(device->address32, EntityType::Graph, graph->index);
    device->phyAttributes.createGraph(indexGraph, graph);

    theoreticEngine.createJoinEdge(device, parentDevice, subnet);

    /************
     * From table 124 in standard.
     */
    PhyRoute * route = new PhyRoute();
    route->index = DEFAULT_ROUTE_ID;
    // route->size = 1;
    route->alternative = 3;//configured as default route
    route->route.push_back(0xA001);//configured on default Graph with id 1 (must be prefixed with 0xA)
    EntityIndex indexRoute = createEntityIndex(device->address32, EntityType::Route, route->index);
    device->phyAttributes.createRoute(indexRoute, route);

    /************
     * From table 117 and 118 in standard.
     */
    PhySuperframe * superframe = new PhySuperframe();
    superframe->index = DEFAULT_SUPERFRAME_ID;
    superframe->tsDur = subnetSettings.getTimeslotLength();
    superframe->chIndex = Tdma::ChannelHopping::HOPPING_1;
    superframe->chBirth = 0; // that is not really true but is not used on internal model
    superframe->sfPeriod = subnetSettings.getSlotsPer30Sec();
    superframe->sfBirth = subnetSettings.superframeBirth;
    superframe->chRate = SUPERFRAME_ADVERTISE_CHANNEL_RATE;
    superframe->chMapOv = 0;
    superframe->chMap = DEFAULT_CHANNEL_MAP;
    superframe->sfType = 0;
    superframe->priority = 0;
    superframe->idleUsed = 0;
    superframe->idleTimer = 0;
    superframe->rndSlots = 0;
    EntityIndex indexSuperframe = createEntityIndex(device->address32, EntityType::Superframe, superframe->index);
    device->phyAttributes.createSuperframe(indexSuperframe, superframe);

    /************
     * From table 121, DauxJoinTx in standard.
     */
    PhyLink * linkTx = new PhyLink();
    linkTx->index = DEFAULT_LINK_TX_ID;
    linkTx->superframeIndex = superframe->index;
    linkTx->type = Tdma::LinkType::TRANSMIT_BACKOFF_ADAPTIVE;
    linkTx->template1 = Tdma::TemplatesTypes::TEMPLATE_TRANSMIT;
    linkTx->template2 = 0;
    linkTx->role = Tdma::TdmaLinkTypes::DEFAULT;
    linkTx->direction = Tdma::TdmaLinkDir::OUTBOUND;
    linkTx->neighborType = Tdma::NeighbourTypes::NEIGHBOUR_BY_ADDRESS;
    linkTx->graphType = Tdma::GraphTypes::NONE;
    linkTx->schedType = parentDevice->phyAttributes.advInfo.getValue()->txScheduleType;
    linkTx->chType = Tdma::ChannelOffsetType::NONE;
    linkTx->priorityType = Tdma::PriorityType::USE_SUPERFRAME_PRIORITY;
    linkTx->neighbor = Address::getAddress16(parentDevice->address32);
    linkTx->graphID = 0;
    linkTx->schedule = parentDevice->phyAttributes.advInfo.getValue()->joinTx;
    linkTx->chOffset = 0;
    linkTx->priority = 0;
    EntityIndex indexLinkTx = createEntityIndex(device->address32, EntityType::Link, linkTx->index);
    device->phyAttributes.createLink(indexLinkTx, linkTx);

    /************
     * From table 121, DauxJoinRx in standard.
     */
    PhyLink * linkRx = new PhyLink();
    linkRx->index = DEFAULT_LINK_RX_ID;
    linkRx->direction = Tdma::TdmaLinkDir::OUTBOUND;
    linkRx->superframeIndex = superframe->index;
    linkRx->type = Tdma::LinkType::RECEIVE_ADAPTIVE;
    linkRx->template1 = Tdma::TemplatesTypes::TEMPLATE_RECEIVE;
    linkRx->template2 = 0;
    linkRx->role = Tdma::TdmaLinkTypes::DEFAULT;
    linkRx->neighborType = Tdma::NeighbourTypes::NONE;
    linkRx->graphType = Tdma::GraphTypes::NONE;
    linkRx->schedType = parentDevice->phyAttributes.advInfo.getValue()->rxScheduleType;
    linkRx->chType = Tdma::ChannelOffsetType::NONE;
    linkRx->priorityType = Tdma::PriorityType::USE_SUPERFRAME_PRIORITY;
    linkRx->neighbor = 0;
    linkRx->graphID = 0;
    linkRx->schedule = parentDevice->phyAttributes.advInfo.getValue()->joinRx;
    linkRx->chOffset = 0;
    linkRx->priority = 0;
    EntityIndex indexLinkRx = createEntityIndex(device->address32, EntityType::Link, linkRx->index);
    device->phyAttributes.createLink(indexLinkRx, linkRx);

    /************
     * From table 121, DauxAdvRx in standard.
     */
    PhyLink * linkAdvRx = new PhyLink();
    linkAdvRx->index = DEFAULT_LINK_ADV_RX_ID;
    linkAdvRx->direction = Tdma::TdmaLinkDir::OUTBOUND;
    linkAdvRx->superframeIndex = superframe->index;
    linkAdvRx->type = Tdma::LinkType::RECEIVE_ADAPTIVE;
    linkAdvRx->template1 = Tdma::TemplatesTypes::TEMPLATE_RECEIVE_SCANING;
    linkAdvRx->template2 = 0;
    linkAdvRx->role = Tdma::TdmaLinkTypes::DEFAULT;
    linkAdvRx->neighborType = Tdma::NeighbourTypes::NONE;
    linkAdvRx->graphType = Tdma::GraphTypes::NONE;
    linkAdvRx->schedType = parentDevice->phyAttributes.advInfo.getValue()->advScheduleType;
    linkAdvRx->chType = Tdma::ChannelOffsetType::NONE;
    linkAdvRx->priorityType = Tdma::PriorityType::USE_SUPERFRAME_PRIORITY;
    linkAdvRx->neighbor = 0;
    linkAdvRx->graphID = 0;
    linkAdvRx->schedule = parentDevice->phyAttributes.advInfo.getValue()->advRx;
    linkAdvRx->chOffset = 0;
    linkAdvRx->priority = 0;
    EntityIndex indexLinkAdvRx = createEntityIndex(device->address32, EntityType::Link, linkAdvRx->index);
    device->phyAttributes.createLink(indexLinkAdvRx, linkAdvRx);

}

bool NetworkEngine::removePreviousDevice(Subnet::PTR & subnet, const Address32 oldAddress, const Address64 & deviceAddress64,
            Device * device, const DeviceType::DeviceTypeEnum provisionedType, const Uint16 provisionedSubnetID, OperationsContainerPointer & containerRemDev) {

    Device * prevInstance = subnet->getDevice(oldAddress);
    if (prevInstance) {

        if (!(prevInstance->address64 == deviceAddress64)){
            // Address32 already taken
            subnet->addressMapping.erase(deviceAddress64);
            return false;
        }

        device->phyAttributes.movePersistentData(prevInstance->phyAttributes);

    }

    //TODO implement flow remove device + notif ModelTheoretic

    GraphPointer graph1 = subnet->getGraph(DEFAULT_GRAPH_ID);
    if (graph1 && prevInstance){
        graph1->rejoinedDevices.insert(Address::getAddress16(prevInstance->address32));
    }
    else {
        if ((!prevInstance) && (graph1->removedDevices.find(Address::getAddress16(oldAddress)) != graph1->removedDevices.end())) {
            graph1->rejoinedDevices.insert(Address::getAddress16(oldAddress));
        }
    }

    removeDeviceOnError(oldAddress, containerRemDev, RemoveDeviceReason::rejoin);

    if (provisionedType == DeviceType::BACKBONE) {//the subnet was recreated; the new instance is obtained
        subnet = getSubnet(provisionedSubnetID);
        //clear all address mapping for all devices in current subnet
        for (AddressMapping64_32::iterator it = subnet->addressMapping.begin(); it != subnet->addressMapping.end();) {
            if (NE::Common::Address::extractSubnet(it->second) == subnet->getSubnetId()) {
                subnet->addressMapping.erase(it++);
            } else {
                ++it;
            }
        }
    }

    subnet->addressMapping.erase(deviceAddress64);

    return true;
}

/********************************************************************************************************************************
 *
 * Physical Model interface
 *
 ********************************************************************************************************************************/

void NetworkEngine::requestJoinSecurity(const Address64 & deviceAddress64, const Address32 parentAddress32, int requestID,
            DeviceType::DeviceTypeEnum provisionedType, Uint16 provisionedSubnetID, const PhySessionKey& joinKey,
            HandlerResponse handlerResponse) {

    LOG_INFO("JOIN - security " << deviceAddress64.toString() << ", parent=" << Address_toStream(parentAddress32) << ", reqID=" //
                << requestID << ", provType=" << NE::Model::DeviceType::toString(provisionedType) << ", provSubnet="
                << provisionedSubnetID);

    // check maximum number of devices allowed in network (including manager and gateway)
    if (settingsLogic->networkMaxNodes > 0) {
        const Uint16 numberOfDevices = subnetsContainer.getNumberOfDevicesOnNetwork(); //counts SM and GW too

        int newDevice = 1;
        if(subnetsContainer.existsDevice(deviceAddress64)) {
            newDevice = 0;
        }

        if (numberOfDevices + newDevice > settingsLogic->networkMaxNodes) {
            LOG_ERROR("Maximum number of devices in network is reached: " << (int) numberOfDevices);
            handlerResponse(parentAddress32, requestID, ResponseStatus::FAIL);
            return;
        }
    }

    char message[128];
    sprintf(message, "REASON : SECURITY JOIN parent:%x dev:%s rol:%s", parentAddress32, deviceAddress64.toString().c_str(),
                NE::Model::DeviceType::toString(provisionedType).c_str());
    SMState::SMStateLog::logOperationReason(message);

    if (provisionedType == DeviceType::GATEWAY) {
        requestJoinSecurityGateway(deviceAddress64, parentAddress32, requestID, joinKey, handlerResponse);
        return;
    }

    Subnet::PTR subnet;
    if (parentAddress32 == subnetsContainer.getManagerAddress32()) {
        subnet = getSubnet(provisionedSubnetID);
        if (subnet == NULL) {
            LOG_ERROR("Could not find subnet with ID=" << (int) provisionedSubnetID
                        << ". The subnet provisioned in device may not be the same with the subnet configured "
                            "in the provisioning file. Please check.");
            handlerResponse(parentAddress32, requestID, ResponseStatus::FAIL);
            return;
        }
    } else {//if is a device (not bbr or gw)

        subnet = getSubnet(parentAddress32);
        if (subnet == NULL) {
            LOG_ERROR("No subnet found for parent:" << std::hex << parentAddress32 << ", provisionedSubnet="
                        << (int) provisionedSubnetID);
            handlerResponse(parentAddress32, requestID, ResponseStatus::FAIL);
            return;
        }
    }

    Device * parentDevice = subnet->getDevice(parentAddress32);
    if (parentDevice == NULL || (parentDevice != NULL && !parentDevice->isJoinConfirmed())) {
        LOG_ERROR("Illegal join for device " << deviceAddress64.toString() << " to parent " << Address::toString(parentAddress32)
                    << ". Parent not joined yet.");
        handlerResponse(parentAddress32, requestID, ResponseStatus::FAIL);
        return;
    }

    subnet->deleteTheoAttributesChild(Address::getAddress16(parentDevice->address32), deviceAddress64);

    if (!parentDevice->capabilities.isManager() //
                && !subnet->checkMaxNrOfChildren(Address::getAddress16(parentDevice->address32), deviceAddress64)
                ) {

        //LOG_WARN("Reached max number of children for parent " << Address_toStream(parentAddress32)
        //            << ". Join refused for " << deviceAddress64.toString() );
        handlerResponse(parentAddress32, requestID, ResponseStatus::REFUSED_INSUFICIENT_RESOURCES);
        return;
    }

    // check parentDevice metadata
    if(!parentDevice->capabilities.isManager() //
                && subnet->deviceReachedTheMaxLinksNo(Address::getAddress16(parentAddress32), deviceAddress64)) {

        handlerResponse(parentAddress32, requestID, ResponseStatus::REFUSED_INSUFICIENT_RESOURCES);
        return;
    }

    Capabilities capabilities;
    capabilities.euidAddress = deviceAddress64;
    capabilities.deviceType = provisionedType;
    capabilities.dllSubnetId = provisionedSubnetID;

    Device * device = new Device(capabilities);
    device->statusForReports = StatusForReports::SEC_JOIN_REQUEST_RECEIVED;
    device->parent32 = parentAddress32;
    device->status = DeviceStatus::JOIN_REQUEST_RECEIVED;

    Address32 oldAddress = subnet->getAddress32(deviceAddress64);
    if (oldAddress == parentAddress32) {
        LOG_ERROR("Inconsistent address found. Join of " << deviceAddress64.toString() << "[" << std::hex << oldAddress
                    << "] through " << parentAddress32);
        handlerResponse(parentAddress32, requestID, ResponseStatus::FAIL);
        return;
    }

	HandlerResponseList handlerResponses;
	handlerResponses.push_back(handlerResponse);
	HandlerResponse handlerJoinSecurityResponse = boost::bind(&NetworkEngine::handlerJoinSecurityResponse, this, _1, _2, _3);
	handlerResponses.push_back(handlerJoinSecurityResponse);

    char reason[128];
    sprintf(reason, "SECURITY JOIN - remove device %s %x", deviceAddress64.toString().c_str(), oldAddress);
    OperationsContainerPointer containerRemDev(new OperationsContainer(device->address32, requestID, handlerResponses, reason));
    LOG_WARN(reason);

    if (oldAddress) {
        if( !removePreviousDevice(subnet, oldAddress, deviceAddress64, device, provisionedType, provisionedSubnetID, containerRemDev) ) {
            oldAddress = 0;
        }
    }

    device->address32 = subnet->getNextAddress32(deviceAddress64, oldAddress);
    containerRemDev->requesterAddress32 = device->address32;


    //   device->address16 = Address::getAddress16(device->address32);

    if (provisionedType == DeviceType::BACKBONE) {
        AddressMapping64_128::iterator mappingNeighbor = managerNeighborsMapping.find(deviceAddress64);
        assert(mappingNeighbor != managerNeighborsMapping.end() && "a mapping with Backbone must exist prior to join.");//see addManagerNeighbor()
        device->address128 = mappingNeighbor->second;
    } else {
        device->address128 = AddressAllocator::getNextAddress128(deviceAddress64, device->address32, settingsLogic->ipv6AddrPrefix);
    }


    LOG_DEBUG("New device instance: addr64=" << deviceAddress64.toString() << " addr32=" << std::hex << device->address32
                << " addr16=" << Address::getAddress16(device->address32) << " addr128=" << device->address128.toString());

    //set join key on manager (joinKey is SM->Dev)
    PhySessionKey *managerKey = new PhySessionKey(joinKey);
    managerKey->index = subnetsContainer.manager->getNextKeysTableIndex();
    managerKey->destination128 = device->address128;
    EntityIndex sessionKeyEntityIndex = createEntityIndex(subnetsContainer.manager->address32, EntityType::SessionKey,
                managerKey->index);
    subnetsContainer.manager->phyAttributes.createSessionKey(sessionKeyEntityIndex, managerKey);

    //set join key on device; reverse source with destination (key Dev->SM)
    PhySessionKey *deviceKey = new PhySessionKey(joinKey);
    deviceKey->destination64 = subnetsContainer.manager->address64; //SM
    deviceKey->destination128 = subnetsContainer.manager->address128; //SM
    deviceKey->source64 = device->address64; //Dev
    deviceKey->source128 = device->address128; //Dev
    deviceKey->destinationTSAP = joinKey.sourceTSAP;
    deviceKey->sourceTSAP = joinKey.destinationTSAP;
    EntityIndex deviceKeyEntityIndex = createEntityIndex(device->address32, EntityType::SessionKey, deviceKey->index);
    device->phyAttributes.createSessionKey(deviceKeyEntityIndex, deviceKey);

    if (provisionedType == DeviceType::IN_OUT
                || provisionedType == DeviceType::ROUTER
                || provisionedType == DeviceType::NOT_SET
                || provisionedType == DeviceType::ROUTER_IO) {
        initialiseDefaultAttributes(device, parentDevice, subnet);
    }

    if (provisionedType == DeviceType::BACKBONE
                || provisionedType == DeviceType::IN_OUT
                || provisionedType == DeviceType::NOT_SET
                || provisionedType == DeviceType::ROUTER
                || provisionedType == DeviceType::ROUTER_IO) {
        PhyChannelHopping* channelHopping_1 = new PhyChannelHopping();
        channelHopping_1->index = Tdma::ChannelHopping::HOPPING_1;
        channelHopping_1->length = 16;
        Uint8 channel_list[] = { 8, 1, 9, 13, 5, 12, 7, 14, 3, 10, 0, 4, 11, 6, 2, 15 };

        if(!subnet->getSubnetSettings().reduced_hopping.empty()) {
        	for (Uint8 i = 0; i < subnet->getSubnetSettings().reduced_hopping.size(); ++i) {
        		channelHopping_1->seq.push_back(subnet->getSubnetSettings().reduced_hopping[i]);
        	}
        }
        else {
        	channelHopping_1->seq.assign(channel_list, channel_list + 16);
        }


        EntityIndex indexChannelHopping = createEntityIndex(device->address32, EntityType::ChannelHopping,
                    channelHopping_1->index);
        device->phyAttributes.createChannelHopping(indexChannelHopping, channelHopping_1);

    }

    subnet->addressMapping.insert(AddressMapping64_32::value_type(device->address64, device->address32));
    subnet->addDevice(device);
    subnet->addTheoAttributesChild(Address::getAddress16(parentDevice->address32), Address::getAddress16(device->address32));

    // !!! must be here(after add device to subnet and before add network route on manager), because on NE::handlerJoinSecurityResponse (handler for containerRemDev), must exist device in subnet
    operationsProcessor.addOperationsContainer(containerRemDev);

    if (provisionedType == DeviceType::BACKBONE) {
        LOG_DEBUG("create contracts SM->BBR");
        ContractsHelper::createContractsSmToNeighbor(subnetsContainer.manager, device, operationsProcessor, *settingsLogic);
        LOG_DEBUG("create network route SM->BBR");
        //create network route SM->BBR
        PhyNetworkRoute * networkRoute_SM_BBR = new PhyNetworkRoute();
        networkRoute_SM_BBR->networkRouteID = subnetsContainer.manager->getNextNetworkRouteID();
        networkRoute_SM_BBR->destination = device->address128;
        networkRoute_SM_BBR->nextHop = device->address128;
        networkRoute_SM_BBR->nwkHopLimit = 64;
        networkRoute_SM_BBR->outgoingInterface = 1;

        EntityIndex entityIndex_SM_BBR = createEntityIndex(subnetsContainer.manager->address32, EntityType::NetworkRoute,
                    networkRoute_SM_BBR->networkRouteID);
        IEngineOperationPointer operation_SM_BBR(new WriteAttributeOperation(networkRoute_SM_BBR, entityIndex_SM_BBR));
        operationsProcessor.addManagerOperation(operation_SM_BBR);

    }

}

void NetworkEngine::requestJoinSecurityGateway(const Address64& deviceAddress64, const Address32 parentAddress32, int requestID,
            const PhySessionKey& joinKey, HandlerResponse handlerResponse) {

    Device * gwDevice = subnetsContainer.getGateway();
    if (!gwDevice) {
        Capabilities capabilities;
        capabilities.euidAddress = deviceAddress64;
        capabilities.deviceType = DeviceType::GATEWAY;
        gwDevice = new Device(capabilities);
        gwDevice->address32 = ADDRESS16_GATEWAY;
        gwDevice->parent32 = parentAddress32;
        AddressMapping64_128::iterator mappingNeighbor = managerNeighborsMapping.find(deviceAddress64);
        assert(mappingNeighbor != managerNeighborsMapping.end() && "a mapping with Gateway must exist prior to join.");//see addManagerNeighbor()
        gwDevice->address128 = mappingNeighbor->second;

        subnetsContainer.addGateway(gwDevice);
    }
    gwDevice->status = DeviceStatus::JOIN_REQUEST_RECEIVED;
    gwDevice->statusForReports = StatusForReports::SEC_JOIN_REQUEST_RECEIVED;
    Device* managerDevice = subnetsContainer.manager;

    // 1. remove old GW->Manager keys - in order to create new SessionKey; rejoin GW
    LOG_DEBUG("gw keys=" << gwDevice->phyAttributes.sessionKeysTable);
    SessionKeyIndexedAttribute::iterator itD2MPhySessionKey = gwDevice->phyAttributes.sessionKeysTable.end();
    for (itD2MPhySessionKey = gwDevice->phyAttributes.sessionKeysTable.begin(); itD2MPhySessionKey
                != gwDevice->phyAttributes.sessionKeysTable.end();) {
        PhySessionKey* currD2MPhySessionKey = (PhySessionKey*) itD2MPhySessionKey->second.getValue();
        if (currD2MPhySessionKey && currD2MPhySessionKey->source64 == gwDevice->address64 //
                    && currD2MPhySessionKey->destination64 == managerDevice->address64 ) {

            LOG_DEBUG("delete old GW->M session key from model(rejoin GW)" << *currD2MPhySessionKey);
            delete currD2MPhySessionKey;
            gwDevice->phyAttributes.sessionKeysTable.erase(itD2MPhySessionKey++);
        } else {
            ++itD2MPhySessionKey;
        }
    }

    OperationsContainerPointer containerJoinGW(new OperationsContainer("SECURITY JOIN GW"));
    //    std::list<EntityIndex> keysToBeDeleted;
    // 2. remove old Manager->Device keys - in order to create new SessionKey
    LOG_DEBUG("manager keys=" << managerDevice->phyAttributes.sessionKeysTable);
    SessionKeyIndexedAttribute::iterator itM2DPhySessionKey = managerDevice->phyAttributes.sessionKeysTable.end();
    for (itM2DPhySessionKey = managerDevice->phyAttributes.sessionKeysTable.begin(); itM2DPhySessionKey
                != managerDevice->phyAttributes.sessionKeysTable.end(); ++itM2DPhySessionKey) {
        PhySessionKey* currM2DPhySessionKey = (PhySessionKey*) itM2DPhySessionKey->second.getValue();
        if (currM2DPhySessionKey && currM2DPhySessionKey->source64 == managerDevice->address64 && currM2DPhySessionKey->destination64 == deviceAddress64) {
            //            keysToBeDeleted.push_back(itM2DPhySessionKey->first);
            DeleteAttributeOperationPointer deleteKey(new DeleteAttributeOperation(itM2DPhySessionKey->first));
            containerJoinGW->addOperation(deleteKey, managerDevice);
        }
    }

    operationsProcessor.addOperationsContainer(containerJoinGW);
    LOG_DEBUG("after clean - gw keys=" << gwDevice->phyAttributes.sessionKeysTable);
    LOG_DEBUG("after clean - manager keys=" << managerDevice->phyAttributes.sessionKeysTable);

    // 3. set join key on manager (joinKey is SM->Dev)
    // stack entry for managerKey is set by PSMO on handlerResponse callback! Sorin.Bidian@nivis.com break the template flow!
    PhySessionKey *managerKey = new PhySessionKey(joinKey);
    managerKey->index = subnetsContainer.manager->getNextKeysTableIndex();
    managerKey->destination128 = gwDevice->address128;
    LOG_DEBUG("add manager key=" << *managerKey);
    EntityIndex sessionKeyEntityIndex = createEntityIndex(subnetsContainer.manager->address32, EntityType::SessionKey,
                managerKey->index);
    subnetsContainer.manager->phyAttributes.createSessionKey(sessionKeyEntityIndex, managerKey);

    //set join key on GW; reverse source with destination
    PhySessionKey *gatewayKey = new PhySessionKey(joinKey);
    gatewayKey->destination64 = subnetsContainer.manager->address64; //SM
    gatewayKey->destination128 = subnetsContainer.manager->address128; //SM
    gatewayKey->source64 = gwDevice->address64; //GW
    gatewayKey->source128 = gwDevice->address128; //GW
    gatewayKey->destinationTSAP = joinKey.sourceTSAP;
    gatewayKey->sourceTSAP = joinKey.destinationTSAP;
    LOG_DEBUG("add gw key=" << *gatewayKey);
    EntityIndex gwKeyEntityIndex = createEntityIndex(gwDevice->address32, EntityType::SessionKey, gatewayKey->index);
    gwDevice->phyAttributes.createSessionKey(gwKeyEntityIndex, gatewayKey);

    LOG_DEBUG("create contracts SM->GW");
    ContractsHelper::createContractsSmToNeighbor(subnetsContainer.manager, gwDevice, operationsProcessor, *settingsLogic);

    // 4. skip create (M->GW)network route if already exists
    NetworkRouteIndexedAttribute::iterator itPhyNetworkRoute = managerDevice->phyAttributes.networkRoutesTable.end();
    for (itPhyNetworkRoute = managerDevice->phyAttributes.networkRoutesTable.begin(); itPhyNetworkRoute
                != managerDevice->phyAttributes.networkRoutesTable.end(); ++itPhyNetworkRoute) {
        if (itPhyNetworkRoute->second.getValue() && itPhyNetworkRoute->second.getValue()->destination == gwDevice->address128
                    && itPhyNetworkRoute->second.getValue()->nextHop == gwDevice->address128) {
            break;
        }
    }

    if (itPhyNetworkRoute == managerDevice->phyAttributes.networkRoutesTable.end()) {

        LOG_INFO("create network route SM->GW");
        PhyNetworkRoute * networkRoute_SM_GW = new PhyNetworkRoute();
        networkRoute_SM_GW->networkRouteID = subnetsContainer.manager->getNextNetworkRouteID();
        networkRoute_SM_GW->destination = gwDevice->address128;
        networkRoute_SM_GW->nextHop = gwDevice->address128;
        networkRoute_SM_GW->nwkHopLimit = 64;
        networkRoute_SM_GW->outgoingInterface = 1;

        EntityIndex entityIndex_SM_GW = createEntityIndex(subnetsContainer.manager->address32, EntityType::NetworkRoute,
                    networkRoute_SM_GW->networkRouteID);
        IEngineOperationPointer operation_SM_GW(new WriteAttributeOperation(networkRoute_SM_GW, entityIndex_SM_GW));
        operationsProcessor.addManagerOperation(operation_SM_GW);
    } else {
        LOG_INFO("network route SM->GW already exists.");
    }

    handlerResponse(gwDevice->address32, requestID, ResponseStatus::SUCCESS);

    gwDevice->statusForReports = StatusForReports::SEC_JOIN_RESPONSE_SENT;
}

void createJoinRoute(Device * joiningDevice, Device * parentDevice, Device * subnetBackbone,
            OperationsContainerPointer operationsContainer) {
    //find the management route on parent to be used on joiningDevice by extension with one hop (graph parent+hop)
    if (!joiningDevice->capabilities.isDevice()) {
        return;
    }

    /****** Create neighbor on proxy with new device in group 1 *********/
    PhyNeighbor * neighbor = new PhyNeighbor();
    neighbor->index = Address::getAddress16(joiningDevice->address32);
    neighbor->address64 = joiningDevice->address64;
    neighbor->clockSource = 0;
    neighbor->diagLevel = 0;
    neighbor->extGrCnt = 0;
    neighbor->groupCode = 1;
    neighbor->linkBacklog = 0;
    neighbor->linkBacklogDur = 0;
    neighbor->linkBacklogIndex = 0;
    EntityIndex entityIndexNeighbor = createEntityIndex(parentDevice->address32, EntityType::Neighbour, neighbor->index);
    IEngineOperationPointer addNeighbor(new WriteAttributeOperation(neighbor, entityIndexNeighbor));
    operationsContainer->addOperation(addNeighbor, parentDevice);


    PhyRoute * att_Route = new PhyRoute();
    att_Route->index = subnetBackbone->getNextRouteID();
    att_Route->alternative = 2;
    att_Route->selector = Address::getAddress16(joiningDevice->address32);
    att_Route->evaluationTime = 0;

    EntityIndex entityIndexAddRoute;

    if (parentDevice->capabilities.isBackbone()) {
        att_Route->route.push_back(Address::getAddress16(joiningDevice->address32));
    } else {
        Uint16 graphID = subnetBackbone->getOutBoundGraph(Address::getAddress16(parentDevice->address32));
        att_Route->route.push_back(ROUTE_GRAPH_MASK | graphID);
        att_Route->route.push_back((Address::getAddress16(joiningDevice->address32)));
    }
    entityIndexAddRoute = createEntityIndex(subnetBackbone->address32, EntityType::Route, att_Route->index);
    IEngineOperationPointer operationATT(new Operations::WriteAttributeOperation(att_Route, entityIndexAddRoute));
    operationsContainer->addOperation(operationATT, subnetBackbone);
}

void NetworkEngine::requestJoinSystemManager(const Address64& deviceAddress64, const Address32 parentAddress32, int requestID,
            const Capabilities& capabilities, std::string& softwareRevision, const PhyDeviceCapability& deviceCapability,
            HandlerResponse handlerResponse) {
    LOG_DEBUG("JOIN - system " << deviceAddress64.toString() << ", parent=" << Address_toStream(parentAddress32) << ", reqID=" << requestID << ", rol=" << NE::Model::DeviceType::toString(
                capabilities.deviceType));

    Device * device = getDevice(getAddress32(deviceAddress64));
    if (device == NULL) {
        LOG_ERROR("Device has not performed SecurityJoin:" << std::hex << deviceAddress64.toString() << ", parent="
                    << parentAddress32);
        handlerResponse(parentAddress32, requestID, ResponseStatus::FAIL);
        return;
    }

    if (device->parent32 != parentAddress32) {
        LOG_ERROR("Device sent JoinSecurity and JoinSystemManager through different devices. JoinSecurity through "
        		<< Address_toStream(device->parent32) << ", joinSystemManager through " << Address_toStream(parentAddress32));
        handlerResponse(parentAddress32, requestID, ResponseStatus::REQUEST_DISCARDED);
        return;
    }

    if (device->status != DeviceStatus::JOIN_REQUEST_RECEIVED) {
        LOG_ERROR("Device is in incompatible state:" << std::hex << deviceAddress64.toString() << ", parent=" << parentAddress32
                    << " state=" << device->status << " must be " << DeviceStatus::JOIN_REQUEST_RECEIVED);
        handlerResponse(parentAddress32, requestID, ResponseStatus::REQUEST_DISCARDED);
        return;
    }

    device->statusForReports = StatusForReports::SM_JOIN_RECEIVED;

    DeviceType::DeviceTypeEnum deviceType = (DeviceType::DeviceTypeEnum)device->capabilities.deviceType;
    bool deviceTypeChange = false;

    if(!device->capabilities.isBackbone() && !device->capabilities.isManager() &&
                !device->capabilities.isGateway()) {
        deviceTypeChange = true;

        switch(device->capabilities.deviceType) {

            case DeviceType::IN_OUT :{
                if(capabilities.deviceType == DeviceType::ROUTER) {
                    deviceType = DeviceType::ROUTER;
                } else {
                    deviceType = DeviceType::IN_OUT;
                }
                break;
            }
            case  DeviceType::ROUTER: {
                if(capabilities.deviceType == DeviceType::IN_OUT) {
                    deviceType = DeviceType::IN_OUT;
                } else {
                    deviceType = DeviceType::ROUTER;
                }
                break;
            }
            case  DeviceType::ROUTER_IO:{
                if(capabilities.deviceType == DeviceType::IN_OUT) {
                    deviceType = DeviceType::IN_OUT;
                } else {
                    if(capabilities.deviceType == DeviceType::ROUTER) {
                        deviceType = DeviceType::ROUTER;
                    } else {
                        deviceType = DeviceType::ROUTER_IO;
                    }
                }
                break;
            }

            default:
            case DeviceType::NOT_SET :{
                deviceTypeChange = false;
                break;
            }
        }
    }

    device->capabilities = capabilities; //standard System_Manager_Join params
    device->phyAttributes.setDeviceCapability(deviceCapability); //dlmo.18

    if(deviceTypeChange) {
        device->capabilities.deviceType = deviceType;
        device->roleChanged = true;
    }

    device->status = DeviceStatus::NET_JOIN_RECEIVED;

    size_t found = softwareRevision.find('+');
    if (found != std::string::npos){
        //CUSTOM NIVIS: if the revision contains '+' it means that persistent data was cleared and it must be reconfigured (ARMO)
        //this can happen upon a "reset to factory defaults"
        device->phyAttributes.clearPersistentData();

        //remove '+' from string
        string::iterator itRev = softwareRevision.begin() + found;
        softwareRevision.erase(itRev);
    }

    device->phyAttributes.setSoftwareRevisionInformation(softwareRevision);

    if (device->capabilities.isDevice()) {
        Subnet::PTR subnet = subnetsContainer.getSubnet(parentAddress32);
        const SubnetSettings& subnetSettings = subnet->getSubnetSettings();

        if (subnet == NULL) {
            LOG_ERROR("Subnet does not exists for parent:" << Address_toStream(parentAddress32));
            handlerResponse(parentAddress32, requestID, ResponseStatus::FAIL);
            device->statusForReports = StatusForReports::SM_JOIN_RESPONSE_SENT;
            return;
        }

        Device * parentDevice = subnet->getDevice(parentAddress32);
        if (parentDevice == NULL || (parentDevice != NULL && !parentDevice->isJoinConfirmed())) {
            LOG_ERROR("Illegal join for device " << deviceAddress64.toString() << " to parent " << Address::toString(
                        parentAddress32) << ". Parent not joined yet.");
            handlerResponse(parentAddress32, requestID, ResponseStatus::FAIL);
            device->statusForReports = StatusForReports::SM_JOIN_RESPONSE_SENT;
            return;
        }

        //create address translation for backbone with joining device

        Device * brDevice = subnet->getBackbone();
        if (brDevice == NULL) {
            LOG_ERROR("Backbone NULL for subnet " << std::hex << subnet->getSubnetId());
            handlerResponse(parentAddress32, requestID, ResponseStatus::FAIL);
            device->statusForReports = StatusForReports::SM_JOIN_RESPONSE_SENT;
            return;
        }

        Uint8 currentRoutingChilds = 0;
        Uint8 currentNonRoutingChilds = 0;
        subnet->getCountDirectChildsAndRouters(parentDevice, currentRoutingChilds, currentNonRoutingChilds);
        Uint8 routerChildsLimit = (parentDevice->capabilities.isBackbone() ? subnetSettings.nrRoutersPerBBR
                    : subnetSettings.nrRoutersPerRouter);
        Uint8 nonRouterChildsLimit = (parentDevice->capabilities.isBackbone() ? subnetSettings.nrNonRoutersPerBBR
                    : subnetSettings.nrNonRoutersPerRouter);

        //the device itself is not skipped when calculating the currentRoutingChilds
        //this generates that maximum suported routers on parent is with -1 less than maximum configured
        //thats why the sign >= is used
        //if removed will not allocate the links on resereveManagement().. to corect it whould be needed to take in account also the current device (in link engine)
        //... too complicated for a small benefit
        // the 1 position left will be used on redundancy
        if (device->capabilities.isRouting() && (currentRoutingChilds >= routerChildsLimit)) {
            LOG_WARN("Max nr of routers childs per device " << Address_toStream(parentAddress32) << " reached. Join refused for " << deviceAddress64.toString() << "("
                        << (int)currentRoutingChilds << ">=" << (int)routerChildsLimit << ")");
            handlerResponse(parentAddress32, requestID, ResponseStatus::REFUSED_INSUFICIENT_RESOURCES);
            device->statusForReports = StatusForReports::SM_JOIN_RESPONSE_SENT;
            return;
        }

        if (!device->capabilities.isRouting() && (currentNonRoutingChilds >= nonRouterChildsLimit)) {
            LOG_WARN("Max nr of IO childs per device " << Address_toStream(parentAddress32) << " reached. Join refused for " << deviceAddress64.toString() << "("
                        << (int)currentNonRoutingChilds << ">=" << (int)nonRouterChildsLimit << ")");
            handlerResponse(parentAddress32, requestID, ResponseStatus::REFUSED_INSUFICIENT_RESOURCES);
            device->statusForReports = StatusForReports::SM_JOIN_RESPONSE_SENT;
            return;
        }

        if (subnetSettings.freezeLevelOneRouters && device->capabilities.isRouting() && !parentDevice->capabilities.isBackbone()) {
            //Refuse join on level2 when freezeLevelOneRouters is active
            LOG_WARN("Join on level2 refused (freezeL1=true)" << deviceAddress64.toString() << ", parent: " << Address_toStream(parentAddress32) );
            handlerResponse(parentAddress32, requestID, ResponseStatus::REFUSED_INSUFICIENT_RESOURCES);
            device->statusForReports = StatusForReports::SM_JOIN_RESPONSE_SENT;
            return;
        }

        HandlerResponseList handlerResponses;
        handlerResponses.push_back(handlerResponse);
        HandlerResponse handlerSmJoinResponse = boost::bind(&NetworkEngine::handlerSmJoinResponse, this, _1, _2, _3);
        handlerResponses.push_back(handlerSmJoinResponse);

        char reason[128];
        sprintf(reason, "SYSTEM MANAGER JOIN, device=%x %s", device->address32, device->address64.toString().c_str());
        OperationsContainerPointer operationsContainer(
                    new OperationsContainer(device->address32, requestID, handlerResponses, reason));

        PhyAddressTranslation * addressTranslation = new PhyAddressTranslation();
        addressTranslation->addressTranslationID = brDevice->getNextAddressTranslationID();
        addressTranslation->longAddress = device->address128;
        addressTranslation->shortAddress = Address::getAddress16(device->address32);

        EntityIndex entityIndexATT = createEntityIndex(brDevice->address32, EntityType::AddressTranslation,
                    addressTranslation->addressTranslationID);
        IEngineOperationPointer attOperation(new WriteAttributeOperation(addressTranslation, entityIndexATT));
        operationsContainer->addOperation(attOperation, brDevice);


        createJoinRoute(device, parentDevice, brDevice, operationsContainer);

        //after this call, no modification of the operationsContainer should be made
        operationsProcessor.addOperationsContainer(operationsContainer);

    } else {
        handlerResponse(device->address32, requestID, ResponseStatus::SUCCESS);
        device->statusForReports = StatusForReports::SM_JOIN_RESPONSE_SENT;
    }
}

void NetworkEngine::handlerJoinSecurityResponse(Address32 deviceAddress32, int requestID, ResponseStatus::ResponseStatusEnum status) {

    Device * device = getDevice(deviceAddress32);
    if (device) {
        device->statusForReports = StatusForReports::SEC_JOIN_RESPONSE_SENT;
    } else {
	    LOG_ERROR("Device not created yet: " << Address_toStream(deviceAddress32));
    }
}

void NetworkEngine::handlerSmJoinResponse(Address32 deviceAddress32, int requestID, ResponseStatus::ResponseStatusEnum status) {

    Device * device = getDevice(deviceAddress32);
    if (device) {
        device->statusForReports = StatusForReports::SM_JOIN_RESPONSE_SENT;
    }
}

void NetworkEngine::handlerSmContractJoinResponse(Address32 deviceAddress32, int requestID,
            ResponseStatus::ResponseStatusEnum status) {

    Device * device = getDevice(deviceAddress32);
    if (device) {
        device->statusForReports = StatusForReports::SM_CONTRACT_JOIN_RESPONSE_SENT;
    }
}

void NetworkEngine::handlerSecConfirmResponse(Address32 deviceAddress32, int requestID, ResponseStatus::ResponseStatusEnum status) {
    if (status != ResponseStatus::SUCCESS) {
        LOG_WARN("Handler sec confirm response call with fail status: " << status << ". Device status not changed.");
        return;
    }
    Device * device = getDevice(deviceAddress32);
    if (device) {
        if (device->statusForReports != StatusForReports::SEC_CONFIRM_RECEIVED) {
            LOG_WARN("Invalid status on security confirm. Actual:" << StatusForReports::toString(device->statusForReports) << ", Needed: SEC_CONFIRM_RECEIVED");
            return;
        }

        device->statusForReports = StatusForReports::SEC_CONFIRM_RESPONSE_SENT;
        device->joinConfirmTime = time(NULL);
    }
}

void NetworkEngine::handlerJoinedConfigured(Address32 deviceAddress32, int requestID, ResponseStatus::ResponseStatusEnum status) {

    if (status != ResponseStatus::SUCCESS) {
        LOG_WARN("JoinedConfigured called with status " << status);
        return;
    }

    Device * device = getDevice(deviceAddress32);
    if (!device) {
        LOG_ERROR("handlerJoinedConfigured - device not found: " << Address_toStream(deviceAddress32));
        return;
    }

    device->statusForReports = StatusForReports::JOINED_AND_CONFIGURED;

    Subnet::PTR subnet = getSubnet(deviceAddress32); //there will not be a subnet for SM and GW
    if (!subnet) {
        LOG_INFO("handlerJoinedConfigured - there's no subnet for device " << Address_toStream(deviceAddress32));
        return;
    }

    subnet->addDeviceToPostJoinTask(deviceAddress32);

    //count this full join
    reportsEngine.incrementGlobalJoinCounter();
    subnet->incrementFullJoinCount(device->address64);
    device->fullJoinsCount = subnet->getDeviceFullJoinCount(device->address64);

    //alert device joined
    Uint32 currentTAI = ClockSource::getTAI(*settingsLogic);
    Uint8 powerSupply = (Uint8)(device->phyAttributes.powerSupplyStatus.getValue() ?
                                device->phyAttributes.powerSupplyStatus.getValue()->value : 0);
    VisibleString vendorID = device->phyAttributes.vendorID.getValue() ? device->phyAttributes.vendorID.getValue()->value : "?";
    VisibleString modelID = device->phyAttributes.modelID.getValue() ? device->phyAttributes.modelID.getValue()->value : "?";
    VisibleString softwareRev = device->phyAttributes.softwareRevisionInformation.getValue() ?
                                device->phyAttributes.softwareRevisionInformation.getValue()->value : "?";
    VisibleString serialNumber = device->phyAttributes.serialNumber.getValue() ? device->phyAttributes.serialNumber.getValue()->value : "?";

    AlertJoin *alertJoin = new AlertJoin(device->address128, device->capabilities.deviceType, device->address64,
                                            powerSupply, (Uint8) device->statusForReports, vendorID, modelID, softwareRev,
                                            device->capabilities.tagName, serialNumber);
    AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoin, alertJoin, currentTAI));
    operationsProcessor.sendAlert(alertOperation);
}

/** The entities that must be created:
 * BBR:
 * - route(SM->device)- combination of Graph of parent and deviceAddress,
 * - more bandwidth Links if needed
 * - ATT table with new device
 * GW:
 * - NetworkRoute (GW-device)
 * - ATT table (device)
 * ProxyRouter: neighbor(deviceAddress) in group 1
 * New device: NetworkRoute- this is not sent directly in the field because its content is placed in the response and the device
 *  populates by default a NetworkRoute with this information.
 */
void NetworkEngine::requestJoinContract(const Address64& deviceAddress64, const Address32 parentAddress32, int requestID,
            HandlerResponse handlerResponse) {
    LOG_DEBUG("JOIN - contract " << deviceAddress64.toString() << ", parent=" << Address_toStream(parentAddress32) << ", reqID=" << requestID);
    Address32 deviceAddress32 = getAddress32(deviceAddress64);
    Device * device = getDevice(deviceAddress32);
    if (device == NULL) {
        LOG_ERROR("Device has not performed SecurityJoin:" << std::hex << deviceAddress64.toString() << ", parent="
                    << parentAddress32);
        handlerResponse(parentAddress32, requestID, ResponseStatus::FAIL);
        return;
    }
    if (device->status != DeviceStatus::NET_JOIN_RECEIVED) {
        LOG_ERROR("Device is in incompatible state:" << std::hex << deviceAddress64.toString() << ", parent=" << parentAddress32
                    << " state=" << device->status << " must be " << DeviceStatus::NET_JOIN_RECEIVED);
        handlerResponse(parentAddress32, requestID, ResponseStatus::REQUEST_DISCARDED);
        return;
    }

    device->status = DeviceStatus::CONTRACT_JOIN_RECEIVED;
    device->statusForReports = StatusForReports::SM_CONTRACT_JOIN_RECEIVED;

    if (device->capabilities.isGateway()) {
        requestJoinContractGateway(deviceAddress64, parentAddress32, requestID, handlerResponse);
        return;
    }

    HandlerResponseList handlerResponses;
    handlerResponses.push_back(handlerResponse);
    HandlerResponse handlerSmContractJoinResponse = boost::bind(&NetworkEngine::handlerSmContractJoinResponse, this, _1, _2, _3);
    handlerResponses.push_back(handlerSmContractJoinResponse);

    char reason[128];
    sprintf(reason, "JOIN CONTRACT REQUEST, device=%x %s", device->address32, device->address64.toString().c_str());
    OperationsContainerPointer operationsContainer(
                new OperationsContainer(device->address32, requestID, handlerResponses, reason));
    Subnet::PTR subnet = subnetsContainer.getSubnet(deviceAddress32);
    assert(subnet && "Subnet is NULL");
    assert(subnet->getBackbone() && "Backbone NULL in subnet");

    if (device->capabilities.isDevice()) {
        /**** Create Network Route SM -> Device ******/
        PhyNetworkRoute * networkRoute_SM_Dev = new PhyNetworkRoute();
        networkRoute_SM_Dev->networkRouteID = subnetsContainer.manager->getNextNetworkRouteID();
        networkRoute_SM_Dev->destination = device->address128;
        networkRoute_SM_Dev->nextHop = subnet->getBackbone()->address128;
        networkRoute_SM_Dev->nwkHopLimit = 64;
        networkRoute_SM_Dev->outgoingInterface = 1;
        EntityIndex entityIndex_SM_Dev = createEntityIndex(subnetsContainer.manager->address32, EntityType::NetworkRoute,
                    networkRoute_SM_Dev->networkRouteID);
        IEngineOperationPointer operation_SM_Dev(new WriteAttributeOperation(networkRoute_SM_Dev, entityIndex_SM_Dev));
        operationsProcessor.addManagerOperation(operation_SM_Dev);

        /***** Create contract SM->device. contracts for backbone are created on security join *********/
        ContractRequest newM2DContractRequest;
        newM2DContractRequest.contractRequestId = 0xFFFF; //join contracts are all with contractReqID = 0xFFFF
        newM2DContractRequest.sourceAddress = subnetsContainer.getManagerAddress32();
        newM2DContractRequest.sourceSAP = ContractTDSAP::TDSAP_SMAP - 0xF0B0;//port of SMAP
        newM2DContractRequest.destinationAddress = device->address32;
        newM2DContractRequest.destinationSAP = ContractTDSAP::TDSAP_DMAP - 0xF0B0;
        newM2DContractRequest.communicationServiceType = CommunicationServiceType::NonPeriodic;
        if (device->capabilities.isRouting()) {
            newM2DContractRequest.committedBurst = (-subnet->getSubnetSettings().mng_r_out_band);
            newM2DContractRequest.excessBurst = (-subnet->getSubnetSettings().mng_r_out_band);
        } else {
            newM2DContractRequest.committedBurst = (-subnet->getSubnetSettings().mng_s_out_band);
            newM2DContractRequest.excessBurst = (-subnet->getSubnetSettings().mng_s_out_band);
        }
        newM2DContractRequest.isManagement = true;

        // create the dll contract
        PhyContract * dllM2DPhyContract = ContractsHelper::createContract(newM2DContractRequest, subnetsContainer.manager,
                    device, *settingsLogic);
        EntityIndex dllM2DEntityIndex = createEntityIndex(newM2DContractRequest.sourceAddress, EntityType::Contract,
                    dllM2DPhyContract->contractID);
        IEngineOperationPointer dllM2DContractOperation(new WriteAttributeOperation(dllM2DPhyContract, dllM2DEntityIndex));
        operationsContainer->addOperation(dllM2DContractOperation, subnetsContainer.manager);

        // create the associated network contract
        PhyNetworkContract * netM2DPhyContract = ContractsHelper::createNetworkContract(dllM2DPhyContract,
                    subnetsContainer.manager->address128, device->address128);
        EntityIndex netM2DEntityIndex = createEntityIndex(newM2DContractRequest.sourceAddress, EntityType::NetworkContract,
                    netM2DPhyContract->contractID);
        //subnetsContainer.manager->phyAttributes.createNetworkContract(netM2DEntityIndex, netM2DPhyContract);
        IEngineOperationPointer netM2DContractOperation(new WriteAttributeOperation(netM2DPhyContract, netM2DEntityIndex));
        operationsContainer->addOperation(netM2DContractOperation, subnetsContainer.manager);
    }

    // create contract Device->SM. This is sent in the join contract response
    // set directly in the entity, without generating an operation
    ContractRequest requestDevice_SM;
    requestDevice_SM.contractRequestId = 0xFFFF; //join contracts are all with contractReqID = 0xFFFF
    requestDevice_SM.sourceAddress = device->address32;
    requestDevice_SM.sourceSAP = ContractTDSAP::TDSAP_DMAP - 0xF0B0;
    requestDevice_SM.destinationAddress = subnetsContainer.getManagerAddress32();
    requestDevice_SM.destinationSAP = ContractTDSAP::TDSAP_SMAP - 0xF0B0;//port of SMAP
    requestDevice_SM.communicationServiceType = CommunicationServiceType::NonPeriodic;
    if (device->capabilities.isBackbone()){
        requestDevice_SM.committedBurst = (-subnet->getSubnetSettings().mng_r_in_band);
    } else if (device->capabilities.isRouting()) {
        requestDevice_SM.committedBurst = (-subnet->getSubnetSettings().mng_r_in_band);
    } else {
        requestDevice_SM.committedBurst = (-subnet->getSubnetSettings().mng_s_in_band);
    }
    requestDevice_SM.excessBurst = requestDevice_SM.committedBurst;
    requestDevice_SM.isManagement = true;

    PhyContract * dllD2MContract = ContractsHelper::createContract(requestDevice_SM, device, subnetsContainer.manager,
                *settingsLogic);
    EntityIndex dllD2MEntityIndex = createEntityIndex(device->address32, EntityType::Contract, dllD2MContract->contractID);
    device->phyAttributes.createContract(dllD2MEntityIndex, dllD2MContract);

    //create the associated network contract
    PhyNetworkContract * netD2MContract = ContractsHelper::createNetworkContract(dllD2MContract, device->address128,
                subnetsContainer.manager->address128);
    EntityIndex netD2MEntityIndex = createEntityIndex(device->address32, EntityType::NetworkContract, netD2MContract->contractID);
    device->phyAttributes.createNetworkContract(netD2MEntityIndex, netD2MContract);

    LOG_DEBUG("create network route " << (int) device->address32 << "->SM");

    // create network route Device->SM. This is sent in the join contract response
    // set directly in the entity, without generating an operation
    PhyNetworkRoute * networkRoute_Dev_SM = new PhyNetworkRoute();
    networkRoute_Dev_SM->networkRouteID = device->getNextNetworkRouteID();
    networkRoute_Dev_SM->destination = subnetsContainer.manager->address128;
    networkRoute_Dev_SM->nextHop = subnetsContainer.manager->address128;
    networkRoute_Dev_SM->nwkHopLimit = 64;
    networkRoute_Dev_SM->outgoingInterface = 1;
    EntityIndex routeEntityIndex = createEntityIndex(device->address32, EntityType::NetworkRoute,
                networkRoute_Dev_SM->networkRouteID);
    device->phyAttributes.createNetworkRoute(routeEntityIndex, networkRoute_Dev_SM);

    //after this call, no modification of the operationsContainer should be made
    operationsProcessor.addOperationsContainer(operationsContainer);
}

void NetworkEngine::requestJoinContractGateway(const Address64& deviceAddress64, const Address32 parentAddress32, int requestID,
            HandlerResponse handlerResponse) {

    Address32 deviceAddress32 = getAddress32(deviceAddress64);
    Device * gwDevice = getDevice(deviceAddress32);
    FAIL_ON_NULL(gwDevice, parentAddress32, requestID, "Gateway is NULL");

    // create contract Device->SM. This is sent in the join contract response
    // set directly in the entity, without generating an operation
    ContractRequest contractReqD2M;
    contractReqD2M.sourceAddress = gwDevice->address32;
    contractReqD2M.sourceSAP = ContractTDSAP::TDSAP_DMAP - 0xF0B0;
    contractReqD2M.destinationAddress = subnetsContainer.getManagerAddress32();
    contractReqD2M.destinationSAP = ContractTDSAP::TDSAP_SMAP - 0xF0B0;//port of SMAP
    contractReqD2M.communicationServiceType = CommunicationServiceType::NonPeriodic;
    contractReqD2M.committedBurst = settingsLogic->neighbor2ManagerCommittedBurst;
    contractReqD2M.excessBurst = contractReqD2M.committedBurst;
    contractReqD2M.isManagement = true;

    PhyContract * existingD2MPhyContract = ContractsHelper::findContractSource2Destination(gwDevice,
                subnetsContainer.getManagerAddress32(), contractReqD2M.sourceSAP, contractReqD2M.destinationSAP);
    if (!existingD2MPhyContract) {
        PhyContract * newD2MPhyContract = ContractsHelper::createContract(contractReqD2M, gwDevice, subnetsContainer.manager,
                    *settingsLogic);
        EntityIndex newD2MEntityIndex = createEntityIndex(gwDevice->address32, EntityType::Contract,
                    newD2MPhyContract->contractID);
        gwDevice->phyAttributes.createContract(newD2MEntityIndex, newD2MPhyContract);

        //create the associated network contract
        PhyNetworkContract * newD2MPhyNetContract = ContractsHelper::createNetworkContract(newD2MPhyContract,
                    gwDevice->address128, subnetsContainer.manager->address128);
        EntityIndex eIndexNetworkContract_Device_SM = createEntityIndex(gwDevice->address32, EntityType::NetworkContract,
                    newD2MPhyNetContract->contractID);
        gwDevice->phyAttributes.createNetworkContract(eIndexNetworkContract_Device_SM, newD2MPhyNetContract);

        LOG_DEBUG("create network contract " << Address::toString(gwDevice->address32) << "->SM");
    } else {
        LOG_DEBUG("network contract already exist " << Address::toString(gwDevice->address32) << "->SM");
    }

    NetworkRouteIndexedAttribute::iterator itPhyNetworkRoute = gwDevice->phyAttributes.networkRoutesTable.end();
    for (itPhyNetworkRoute = gwDevice->phyAttributes.networkRoutesTable.begin(); itPhyNetworkRoute
                != gwDevice->phyAttributes.networkRoutesTable.end(); ++itPhyNetworkRoute) {
        if (itPhyNetworkRoute->second.getValue() && itPhyNetworkRoute->second.getValue()->destination == subnetsContainer.manager->address128
                    && itPhyNetworkRoute->second.getValue()->nextHop == subnetsContainer.manager->address128) {
            break;
        }
    }

    if (itPhyNetworkRoute == gwDevice->phyAttributes.networkRoutesTable.end()) {
        // create network route Device->SM. This is sent in the join contract response
        // set directly in the entity, without generating an operation
        PhyNetworkRoute * newD2MPhyNetRoute = new PhyNetworkRoute();
        newD2MPhyNetRoute->networkRouteID = gwDevice->getNextNetworkRouteID();
        newD2MPhyNetRoute->destination = subnetsContainer.manager->address128;
        newD2MPhyNetRoute->nextHop = subnetsContainer.manager->address128;
        newD2MPhyNetRoute->nwkHopLimit = 64;
        newD2MPhyNetRoute->outgoingInterface = 1;
        EntityIndex newD2MEntityIndex = createEntityIndex(gwDevice->address32, EntityType::NetworkRoute,
                    newD2MPhyNetRoute->networkRouteID);
        gwDevice->phyAttributes.createNetworkRoute(newD2MEntityIndex, newD2MPhyNetRoute);

        LOG_DEBUG("create network route " << Address::toString(gwDevice->address32) << "->SM");
    } else {
        LOG_DEBUG("network route already exist " << Address::toString(gwDevice->address32) << "->SM");
    }

    //    //after this call, no modification of the operationsContainer should be made
    handlerResponse(gwDevice->address32, requestID, ResponseStatus::SUCCESS);
    gwDevice->statusForReports = StatusForReports::SM_CONTRACT_JOIN_RESPONSE_SENT;
}

void NetworkEngine::configureNetworkRoute_GW_BBR(SubnetsContainer& subnetsContainer,
            OperationsContainerPointer& operationsContainer, Device * backbone) {

    Device * gwDevice = subnetsContainer.getGateway();

    // add NetworkRoute on BR only if the BR doesn't contain already a net route.
    NetworkRouteIndexedAttribute::iterator itNetRoute = backbone->phyAttributes.networkRoutesTable.end();
    for (itNetRoute = backbone->phyAttributes.networkRoutesTable.begin(); itNetRoute
                != backbone->phyAttributes.networkRoutesTable.end(); ++itNetRoute) {
        PhyNetworkRoute* phyNetworkRoute = itNetRoute->second.getValue();
        if (phyNetworkRoute != NULL && phyNetworkRoute->destination == gwDevice->address128 && phyNetworkRoute->nextHop
                    == gwDevice->address128 && phyNetworkRoute->nwkHopLimit == 64 && phyNetworkRoute->outgoingInterface == 1) {
            break;
        }
    }

    if (itNetRoute == backbone->phyAttributes.networkRoutesTable.end()) {
        PhyNetworkRoute * networkRoute_BBR_GW = new PhyNetworkRoute();
        networkRoute_BBR_GW->networkRouteID = backbone->getNextNetworkRouteID();
        networkRoute_BBR_GW->destination = gwDevice->address128;
        networkRoute_BBR_GW->nextHop = gwDevice->address128;
        networkRoute_BBR_GW->nwkHopLimit = 64;
        networkRoute_BBR_GW->outgoingInterface = 1;

        EntityIndex entityIndex_BBR_GW = createEntityIndex(backbone->address32, EntityType::NetworkRoute,
                    networkRoute_BBR_GW->networkRouteID);
        IEngineOperationPointer operation_BBR_GW(new WriteAttributeOperation(networkRoute_BBR_GW, entityIndex_BBR_GW));
        operationsContainer->addOperation(operation_BBR_GW, backbone);
    }

    // delete old network route on GW (from model) in order to add new network route
    itNetRoute = gwDevice->phyAttributes.networkRoutesTable.begin();
    for (; itNetRoute != gwDevice->phyAttributes.networkRoutesTable.end(); ++itNetRoute) {
        PhyNetworkRoute* phyNetworkRoute = itNetRoute->second.getValue();
        if (phyNetworkRoute && phyNetworkRoute->destination == backbone->address128 && phyNetworkRoute->nextHop == backbone->address128
                    && phyNetworkRoute->nwkHopLimit == 64 && phyNetworkRoute->outgoingInterface == 1) {

            LOG_INFO("delete network route from model; netRoute=" << *phyNetworkRoute);
            delete phyNetworkRoute;
            gwDevice->phyAttributes.networkRoutesTable.erase(itNetRoute);
            break;
        }
    }

    PhyNetworkRoute * networkRoute_GW_BBR = new PhyNetworkRoute();
    networkRoute_GW_BBR->networkRouteID = subnetsContainer.getGateway()->getNextNetworkRouteID();
    networkRoute_GW_BBR->destination = backbone->address128;
    networkRoute_GW_BBR->nextHop = backbone->address128;
    networkRoute_GW_BBR->nwkHopLimit = 64;
    networkRoute_GW_BBR->outgoingInterface = 1;

    EntityIndex entityIndex_GW_BBR = createEntityIndex(subnetsContainer.getGateway()->address32, EntityType::NetworkRoute,
                networkRoute_GW_BBR->networkRouteID);
    IEngineOperationPointer operation_GW_BBR(new WriteAttributeOperation(networkRoute_GW_BBR, entityIndex_GW_BBR));
    operationsContainer->addOperation(operation_GW_BBR, subnetsContainer.getGateway());
}

void NetworkEngine::configureAddresTranslationTable_GW_BBR(SubnetsContainer& subnetsContainer,
            OperationsContainer& operationsContainer, Device * backbone) {

    Device * gwDevice = subnetsContainer.getGateway();

    // add ATT on BR only if the BR doesn't contain already an ATT.
    AddressTranslationIndexedAttribute::iterator itATT = backbone->phyAttributes.addressTranslationTable.begin();
    for (; itATT != backbone->phyAttributes.addressTranslationTable.end(); ++itATT) {
        PhyAddressTranslation* phyAddressTranslation = itATT->second.getValue();
        if (phyAddressTranslation && phyAddressTranslation->longAddress == gwDevice->address128 && phyAddressTranslation->shortAddress
                    == Address::getAddress16(gwDevice->address32)) {
            break;
        }
    }

    if (itATT == backbone->phyAttributes.addressTranslationTable.end()) {
        PhyAddressTranslation * att_BBR_GW = new PhyAddressTranslation();
        att_BBR_GW->addressTranslationID = backbone->getNextAddressTranslationID();
        att_BBR_GW->longAddress = gwDevice->address128;
        att_BBR_GW->shortAddress = Address::getAddress16(gwDevice->address32);

        EntityIndex entityIndex_BBR_GW = createEntityIndex(backbone->address32, EntityType::AddressTranslation,
                    att_BBR_GW->addressTranslationID);
        IEngineOperationPointer operation_BBR_GW(new WriteAttributeOperation(att_BBR_GW, entityIndex_BBR_GW));
        operationsContainer.addOperation(operation_BBR_GW, backbone);
    }

//By Cata: GW does not need ATT table. In cases of multiple BBR could be 2 BBR with same short address and GW produce sfc error on add att.

}

//configure ATT BR->SM (at backbone join)
void configureATTwithManager(SubnetsContainer& subnetsContainer, OperationsContainer& operationsContainer, Device * backbone) {
    PhyAddressTranslation * addressTranslation = new PhyAddressTranslation();
    addressTranslation->addressTranslationID = backbone->getNextAddressTranslationID();
    addressTranslation->longAddress = subnetsContainer.manager->address128;
    addressTranslation->shortAddress = Address::getAddress16(subnetsContainer.manager->address32);

    EntityIndex entityIndexATT = createEntityIndex(backbone->address32, EntityType::AddressTranslation,
                addressTranslation->addressTranslationID);
    IEngineOperationPointer attOperation(new WriteAttributeOperation(addressTranslation, entityIndexATT));
    operationsContainer.addOperation(attOperation, backbone);
}

void NetworkEngine::handlerConfirmationReadMetadata(Address32 deviceAddress32, int requestID, ResponseStatus::ResponseStatusEnum status){
    Device * device = getDevice(deviceAddress32);
    RETURN_ON_NULL_MSG(device, "Device is NULL " << Address_toStream(deviceAddress32));

    device->hasMetadataUpdated = true;

    Subnet::PTR subnet = getSubnet(deviceAddress32);
    if (subnet){
        subnet->addNotPublishingDevice(deviceAddress32);
        subnet->addNotConfiguredForAlertsDevice(deviceAddress32);
        subnet->addNoNeighborDiscoveryConfiguratedDevice(deviceAddress32);
    }
}

void NetworkEngine::readMetadata(Address32 deviceAddress32) {

    Device * device = getDevice(deviceAddress32);
    RETURN_ON_NULL_MSG(device, "Device is NULL " << Address_toStream(deviceAddress32));

    HandlerResponse handlerResponse = boost::bind(&NetworkEngine::handlerConfirmationReadMetadata, this, _1, _2, _3);

    char reason[64];
    sprintf(reason, "READ metadata, device=%x", deviceAddress32);
    OperationsContainerPointer confirmContainer(
                new OperationsContainer(deviceAddress32, 0, handlerResponse, reason));

    // 2. confirm device - read Metadata, Power_Supply, Vendor, start Publish
    if (!device->capabilities.isGateway()) { // for GW - no read Metadata, Power_Supply, Vendor, start Publish
        if (!device->hasMetadataUpdated){
        // 2.1 read Metadata
            {
                // 2.1.1 read DMO_Attribute - Metadata_Contracts_Table
                EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::ContractsTable_MetaData, 0);
                IEngineOperationPointer operation(new ReadAttributeOperation(entityIndex));
                confirmContainer->addOperation(operation, device);
            }

            {
                // 2.1.2  DLMO_Attributes - Neighbor Metadata
                EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::Neighbor_MetaData, 0);
                IEngineOperationPointer operation(new ReadAttributeOperation(entityIndex));
                confirmContainer->addOperation(operation, device);
            }

            {
                // 2.1.3  DLMO_Attributes - Superframe Metadata
                EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::Superframe_MetaData, 0);
                IEngineOperationPointer operation(new ReadAttributeOperation(entityIndex));
                confirmContainer->addOperation(operation, device);
            }

            {
                // 2.1.4  DLMO_Attributes - Graph Metadata
                EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::Graph_MetaData, 0);
                IEngineOperationPointer operation(new ReadAttributeOperation(entityIndex));
                confirmContainer->addOperation(operation, device);
            }

            {
                // 2.1.5  DLMO_Attributes - Link Metadata
                EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::Link_MetaData, 0);
                IEngineOperationPointer operation(new ReadAttributeOperation(entityIndex));
                confirmContainer->addOperation(operation, device);
            }

            {
                // 2.1.6  DLMO_Attributes - Route Metadata
                EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::Route_MetaData, 0);
                IEngineOperationPointer operation(new ReadAttributeOperation(entityIndex));
                confirmContainer->addOperation(operation, device);
            }

            {
                // 2.1.7  DLMO_Attributes - Diag Metadata
                EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::Diag_MetaData, 0);
                IEngineOperationPointer operation(new ReadAttributeOperation(entityIndex));
                confirmContainer->addOperation(operation, device);
            }
        }
    }

    //after this call, no modification of the confirmContainer should be made
    operationsProcessor.addOperationsContainer(confirmContainer);

}

void NetworkEngine::handlerVendorAttributes(Address32 deviceAddress32, int requestID, ResponseStatus::ResponseStatusEnum status) {

    if (status != ResponseStatus::SUCCESS) {
        LOG_WARN("Read Vendor Attributes not started. Called with status " << status);
        return;
    }

    Device * device = getDevice(deviceAddress32);
    RETURN_ON_NULL_MSG(device, "Device is NULL " << Address_toStream(deviceAddress32));

    HandlerResponseList listHandlers;

    listHandlers.push_back(boost::bind(&NetworkEngine::handlerJoinedConfigured, this, _1, _2, _3));

    char reason[64];
    sprintf(reason, "READ Vendor Attributes, device=%x", deviceAddress32);
    OperationsContainerPointer confirmContainer(
                new OperationsContainer(deviceAddress32, requestID, listHandlers, reason));

    readVendorAttributes(device, confirmContainer);

    if (!confirmContainer->isContainerEmpty()) {
        //after this call, no modification of the confirmContainer should be made
        operationsProcessor.addOperationsContainer(confirmContainer);
    } else {
        handlerJoinedConfigured(deviceAddress32, requestID, ResponseStatus::SUCCESS);
    }

}

void NetworkEngine::readVendorAttributes(Device * device, OperationsContainerPointer& confirmContainer) {

	LOG_INFO("readVendorAttributes for device " << Address_toStream(device->address32));

    // 2.3 Vendor Attributes
    if (!device->phyAttributes.vendorID.getValue()) {
        // 2.3.1.  DMO_Attributes - Vendor_ID
        EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::Vendor_ID, 0);
        IEngineOperationPointer operation(new ReadAttributeOperation(entityIndex));
        confirmContainer->addOperation(operation, device);
    }

    if (!device->phyAttributes.modelID.getValue()) {
        // 2.3.2  DMO_Attributes - Model_ID
        EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::Model_ID, 0);
        IEngineOperationPointer operation(new ReadAttributeOperation(entityIndex));
        confirmContainer->addOperation(operation, device);
    }

    if (!device->phyAttributes.serialNumber.getValue()) {
        // 2.3.4  DMO_Attributes - Serial_Number
        EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::SerialNumber, 0);
        IEngineOperationPointer operation(new ReadAttributeOperation(entityIndex));
        confirmContainer->addOperation(operation, device);
    }

    // 3. Write QueuePriority
    if (device->capabilities.isRouting()) {
		Subnet::PTR subnet = getSubnet(device->address32);
		if (subnet) {

			NE::Common::SubnetSettings & subnetSettings = subnet->getSubnetSettings();

			if (subnetSettings.queuePriorities.size() > 0) {

				EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::QueuePriority, 0);
				PhyQueuePriority * attributeValue = new PhyQueuePriority();

				for (std::vector<Uint16>::iterator it = subnetSettings.queuePriorities.begin(); it != subnetSettings.queuePriorities.end(); ++it) {
					Uint8 priority = ((*it) >> 8);
					Uint8 qMax = ((*it) & 0x00FF);
					attributeValue->value.push_back(QueuePriorityEntry(priority, qMax));
				}

				Operations::IEngineOperationPointer operation(new Operations::WriteAttributeOperation(attributeValue, entityIndex));
				confirmContainer->addOperation(operation, device);
			}
		} else {
			LOG_ERROR("No subnet for device " << Address_toStream(device->address32));
		}
    }
}


/**
 * Perform confirmation of the devices.
 * For GW:
 * - if BBR is joined send NetworkRoute operation to both GW and BBR.
 * For BBR:
 * - if GW is joined send NetworkRoute operation to both GW and BBR.
 */
void NetworkEngine::requestConfirmJoin(const Address32 deviceAddress32, int requestID, HandlerResponse handlerResponse) {
    LOG_DEBUG("JOIN - confirm " << Address_toStream(deviceAddress32) << ", reqID=" << requestID);
    Device * device = getDevice(deviceAddress32);
    FAIL_ON_NULL(device, deviceAddress32, requestID, "Device is NULL in confirm " << Address_toStream(deviceAddress32));
    if (device->status != DeviceStatus::CONTRACT_JOIN_RECEIVED) {
        LOG_ERROR("Device is in incompatible state:" << std::hex << device->address64.toString() << ", parent="
                    << device->parent32 << " state=" << device->status << " must be " << DeviceStatus::CONTRACT_JOIN_RECEIVED);
        handlerResponse(deviceAddress32, requestID, ResponseStatus::REQUEST_DISCARDED);
        return;
    }

    device->status = DeviceStatus::JOIN_CONFIRMED;
    device->statusForReports = StatusForReports::SEC_CONFIRM_RECEIVED;

    if (device->capabilities.isGateway()) {
        requestConfirmJoinGW(deviceAddress32, requestID, handlerResponse);
        return;
    }
    Subnet::PTR subnet = subnetsContainer.getSubnet(deviceAddress32);
    FAIL_ON_NULL(subnet, deviceAddress32, requestID, "Subnet is NULL for device " << Address_toStream(deviceAddress32));

    Device * parentDevice = subnetsContainer.getDevice(device->parent32);
    FAIL_ON_NULL(parentDevice, deviceAddress32, requestID, "Parent " << Address_toStream(device->parent32) << " is NULL in confirm of " << Address_toStream(deviceAddress32));

    // 1. joinConfirmRequest
    //      1.1 readDeviceCapability
    //          1.1.1 joinConfirmResponse
    //          1.1.2 readMetadata_VendorAttributes
    //              1.1.2.1 startPublish

    HandlerResponseList handlerResponses;
    handlerResponses.push_back(handlerResponse);
    handlerResponses.push_back(boost::bind(&NetworkEngine::handlerSecConfirmResponse, this, _1, _2, _3));
    handlerResponses.push_back(boost::bind(&NetworkEngine::handlerVendorAttributes, this, _1, _2, _3));

    if (!device->capabilities.isGateway()) {
        if (device->capabilities.isBackbone()) {

            readMetadata(device->address32);

            ChainAddDeviceToRoleActivationPointer chainAddToRoleActivation(new ChainAddDeviceToRoleActivation(subnet));
            handlerResponses.push_back(boost::bind(&ChainAddDeviceToRoleActivation::process, chainAddToRoleActivation, _1, _2, _3));
        } else { // regular device

            ChainAddRouteToBeActivatedPointer chainAddRouteToBeActivated(new ChainAddRouteToBeActivated(subnet));
            HandlerResponse handlerAddRouteToBeActivated = boost::bind(&ChainAddRouteToBeActivated::process,
                        chainAddRouteToBeActivated, _1, _2, _3);

            handlerResponses.push_back(handlerAddRouteToBeActivated);
        }
    }

    HandlerResponseList listHandlerResponsesChainManagementLinks;
    ChainManagementLinksPointer
                chainManagementLinks(
                            new ChainManagementLinks(subnet, theoreticEngine, operationsProcessor, device->address32, parentDevice->address32, handlerResponses));
    listHandlerResponsesChainManagementLinks.push_back(boost::bind(&ChainManagementLinks::process, chainManagementLinks, _1, _2, _3));

    //new: DeviceCapability contained in join request
    DeviceCapabilityHandlerPointer
                deviceCapabilityHandler(
                            new DeviceCapabilityHandler(subnet, theoreticEngine, operationsProcessor, device->address32, parentDevice->address32, listHandlerResponsesChainManagementLinks));
    HandlerResponse handlerReadDeviceCapability = boost::bind(&DeviceCapabilityHandler::process, deviceCapabilityHandler, _1, _2, _3);


    // 1. confirm device - send Links ...
    char reason[128];
    sprintf(reason, "JOIN CONFIRM REQUEST, device=%x %s", deviceAddress32, device->address64.toString().c_str());
    OperationsContainerPointer
                operationsContainer(new OperationsContainer(deviceAddress32, requestID, handlerReadDeviceCapability, reason));

    if (device->capabilities.isBackbone()) {
        if (subnetsContainer.getGateway() != NULL && subnetsContainer.getGateway()->isJoinConfirmed()) {
            configureNetworkRoute_GW_BBR(subnetsContainer, operationsContainer, device);
            configureAddresTranslationTable_GW_BBR(subnetsContainer, *operationsContainer, device);
        }

        theoreticEngine.createGraphG1(subnet, Address::getAddress16(device->address32));

        configureATTwithManager(subnetsContainer, *operationsContainer, device);

        //create default network route outgoing to DLL network
        PhyNetworkRoute * networkRoute_BBR_default = new PhyNetworkRoute();
        networkRoute_BBR_default->networkRouteID = device->getNextNetworkRouteID();
        networkRoute_BBR_default->destination = Address128();
        networkRoute_BBR_default->nextHop = Address128();
        networkRoute_BBR_default->nwkHopLimit = 64;
        networkRoute_BBR_default->outgoingInterface = 0;

        EntityIndex entityIndex_BBR_default = createEntityIndex(device->address32, EntityType::NetworkRoute,
                    networkRoute_BBR_default->networkRouteID);
        IEngineOperationPointer operation_SM_BBR(new WriteAttributeOperation(networkRoute_BBR_default, entityIndex_BBR_default));
        operationsContainer->addOperation(operation_SM_BBR, device);
    }

    //after this call, no modification of the operationsContainer should be made
    operationsProcessor.addOperationsContainer(operationsContainer);
}

void NetworkEngine::requestConfirmJoinGW(const Address32 deviceAddress32, int requestID, HandlerResponse handlerResponse) {

    HandlerResponseList handlerResponses;
    handlerResponses.push_back(handlerResponse);
    HandlerResponse handlerSecConfirmResponse = boost::bind(&NetworkEngine::handlerSecConfirmResponse, this, _1, _2, _3);
    handlerResponses.push_back(handlerSecConfirmResponse);
    HandlerResponse handlerVendorAttributes = boost::bind(&NetworkEngine::handlerVendorAttributes, this, _1, _2, _3);
    handlerResponses.push_back(handlerVendorAttributes);


    Device * device = getDevice(deviceAddress32);
    FAIL_ON_NULL(device, deviceAddress32, requestID, "Gateway is NULL");

    char reason[128];
    sprintf(reason, "JOIN CONFIRM REQUEST, device=%x %s", deviceAddress32, device->address64.toString().c_str());
    OperationsContainerPointer operationsContainer(new OperationsContainer(deviceAddress32, requestID, handlerResponses, reason));

    //configure network routes and ATTs with joined backbones
    SubnetsMap& subnets = subnetsContainer.getSubnetsList();
    for (SubnetsMap::iterator itSubnets = subnets.begin(); itSubnets != subnets.end(); ++itSubnets) {
        if ((itSubnets->second->getBackbone() != NULL) && (itSubnets->second->getBackbone()->isJoinConfirmed())) {
            configureNetworkRoute_GW_BBR(subnetsContainer, operationsContainer, itSubnets->second->getBackbone());
            configureAddresTranslationTable_GW_BBR(subnetsContainer, *operationsContainer, itSubnets->second->getBackbone());
        }
    }

    // RecoverGWSettings

    // 1. recover keys - at rejoin
    if (device->phyAttributes.sessionKeysTable.size() > 1) { //at rejoin there should be more than one key
        for (SessionKeyIndexedAttribute::iterator it = device->phyAttributes.sessionKeysTable.begin(); it
                    != device->phyAttributes.sessionKeysTable.end(); ++it) {

            if (it->second.getValue() && it->second.getValue()->destination64 == subnetsContainer.manager->address64) {
                LOG_INFO("do not recover GW=>M sessionKey : " << it->second);
                continue;
            }
            if (it->second.isPending) {
                LOG_INFO("do not recover sessionKey(is pending) : " << it->second);
                continue;
            }
            LOG_INFO("recover sessionKey : " << it->second);

            PhySessionKey * phySessionKey = new PhySessionKey();
            *phySessionKey = *it->second.getValue();

            if(phySessionKey) {
                EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::SessionKey, phySessionKey->index);

                IEngineOperationPointer operation(new WriteAttributeOperation(phySessionKey, entityIndex));
                operationsContainer->addOperation(operation, device);
            }
        }
    }

    // 2. recover network contracts
    for (NetworkContractIndexedAttribute::iterator it = device->phyAttributes.networkContractsTable.begin(); it
                != device->phyAttributes.networkContractsTable.end(); ++it) {

        if (it->second.isPending) {
            LOG_INFO("do not recover networkContract(is pending) : " << it->second);
            continue;
        }

        PhyNetworkContract * phyNetworkContract = new PhyNetworkContract();

        if(it->second.getValue()) {
        *phyNetworkContract = *it->second.getValue();
        EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::NetworkContract,
                    phyNetworkContract->contractID);

        IEngineOperationPointer operation(new WriteAttributeOperation(phyNetworkContract, entityIndex));
        operationsContainer->addOperation(operation, device);
        }
    }

    // 3. recover network routes
    for (NetworkRouteIndexedAttribute::iterator it = device->phyAttributes.networkRoutesTable.begin(); it
                != device->phyAttributes.networkRoutesTable.end(); ++it) {

        if (it->second.isPending) {
            LOG_INFO("do not recover networkRoutes(is pending) : " << it->second);
            continue;
        }

        PhyNetworkRoute * phyNetworkRoute = new PhyNetworkRoute();
        *phyNetworkRoute = *it->second.getValue();

        EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::NetworkRoute, phyNetworkRoute->networkRouteID);

        IEngineOperationPointer operation(new WriteAttributeOperation(phyNetworkRoute, entityIndex));
        operationsContainer->addOperation(operation, device);
    }

    device->hasRoleActivated = RoleActivationStatus::ACTIVE;//gw role is activated implicitly.

    //after this call, no modification of the confirmContainer should be made
    operationsProcessor.addOperationsContainer(operationsContainer);

}

void NetworkEngine::requestSecurityNewSession(const Address32 deviceAddress32, int requestID, const PhySessionKey& keyFrom,
            const PhySessionKey& keyTo, HandlerResponse handlerResponse) {

    Device * deviceFrom = getDevice(getAddress32(keyFrom.source64));
    Device * deviceTo = getDevice(getAddress32(keyFrom.destination64));

    if (!deviceFrom) {
        LOG_ERROR("The device " << keyFrom.source64.toString() << " not found.");
        handlerResponse(0, requestID, ResponseStatus::FAIL);
        return;
    }
    if (!deviceTo) {
        LOG_ERROR("The device " << keyFrom.destination64.toString() << " not found.");
        handlerResponse(0, requestID, ResponseStatus::FAIL);
        return;
    }

    char reason[128];
    sprintf(reason, "SecurityNewSession, device=%x", deviceAddress32);
    OperationsContainerPointer operationsContainer(new OperationsContainer(deviceAddress32, requestID, handlerResponse, reason));

    PhySessionKey * phySessionKeyTo = new PhySessionKey(keyTo);//MUST BE CREATED A COPY of the reference parameter
    EntityIndex entityIndexTo = createEntityIndex(deviceTo->address32, EntityType::SessionKey, phySessionKeyTo->index);
    IEngineOperationPointer sessionKeyTo(new WriteAttributeOperation(phySessionKeyTo, entityIndexTo));
    operationsContainer->addOperation(sessionKeyTo, deviceTo);

    PhySessionKey * phySessionKeyFrom = new PhySessionKey(keyFrom);//MUST BE CREATED A COPY of the reference parameter
    EntityIndex entityIndexFrom = createEntityIndex(deviceFrom->address32, EntityType::SessionKey, phySessionKeyFrom->index);
    IEngineOperationPointer sessionKeyFrom(new WriteAttributeOperation(phySessionKeyFrom, entityIndexFrom));
    sessionKeyFrom->addOperationDependency(sessionKeyTo);
    operationsContainer->addOperation(sessionKeyFrom, deviceFrom);

    //after this call, no modification of the operationsContainer should be made
    operationsProcessor.addOperationsContainer(operationsContainer);
}

#define FAIL_RESPONSE(msg, sourceAddress, requestID, status)\
    LOG_ERROR(msg);\
    handlerResponse(sourceAddress, requestID, status);\
    return;

void NetworkEngine::requestCreateContract(const ContractRequest& contractReq, int requestID, HandlerResponse handlerResponse) {
    LOG_INFO("Create contract: " << contractReq);
#warning must be decomented when all devices sent requests non-negotiable
    //negotiable contracts - not supported
    //    if (contractReq.contractNegotiability == ContractNegotiability::NegotiableAndRevocable || contractReq.contractNegotiability
    //                == ContractNegotiability::NegotiableButNotRevocable) {
    //        FAIL_RESPONSE("Negotiable contracts are not supported. " << contractReq, contractReq.sourceAddress,
    //                    requestID, ResponseStatus::FAIL);
    //    }

    if (contractReq.sourceSAP >= 0xF0B0 || contractReq.destinationSAP >= 0xF0B0) {
        FAIL_RESPONSE("TSAP (source or dest) is out of range [0, 0xF0B0) in request " << contractReq, contractReq.sourceAddress,
                    requestID, ResponseStatus::FAIL);
    }

    Device * sourceDevice = getDevice(contractReq.sourceAddress);
    FAIL_ON_NULL(sourceDevice, contractReq.sourceAddress, requestID, "Source of contract already deleted: " << contractReq.sourceAddress);

    Device * destDevice = getDevice(contractReq.destinationAddress);
    FAIL_ON_NULL(destDevice, contractReq.sourceAddress, requestID, "Destination of contract does not exists: " << contractReq.destinationAddress);

    //refuse PS contracts with GW for devices that don't have backup
    if (destDevice->capabilities.isGateway()
                && contractReq.communicationServiceType == CommunicationServiceType::Periodic
                && (settingsLogic->psContractsRefusalTimeSpan != 0 || settingsLogic->psContractsRefusalDeviceTimeSpan != 0) ) {

        Subnet::PTR subnet = getSubnet(sourceDevice->address32);
        FAIL_ON_NULL(subnet, sourceDevice->address32, requestID,
                        "Subnet not found for " << Address_toStream(sourceDevice->address32));

        GraphPointer graph = subnet->getGraph(DEFAULT_GRAPH_ID);
        FAIL_ON_NULL(graph, sourceDevice->address32, requestID, "Could not find default graph.");

        Device *backbone = subnet->getBackbone();

        Uint32 currentTime = NE::Common::ClockSource::getCurrentTime();
        Uint32 elapsedInterval = currentTime - backbone->joinConfirmTime;
        Uint32 deviceElapsedInterval = currentTime - sourceDevice->joinConfirmTime;

        if (!graph->deviceHasDoubleExit(Address::getAddress16(sourceDevice->address32))
                    && (elapsedInterval < settingsLogic->psContractsRefusalTimeSpan || deviceElapsedInterval < settingsLogic->psContractsRefusalDeviceTimeSpan) ) {

            LOG_INFO("Device " << Address_toStream(sourceDevice->address32) << " has no backup."
                        << " Periodic contract with GW refused. BR Elapsed interval=" << std::dec << elapsedInterval
                        << " or Device Elapsed interval=" << std::dec << deviceElapsedInterval);

            //must send as requstID the contractRequestID for SCO to add it to the final response
            handlerResponse(sourceDevice->address32, contractReq.contractRequestId, ResponseStatus::REQUEST_DISCARDED);
            return;
        }
    }

    //check if the contract already exists; already created from a previous request
    PhyContract * contract = ContractsHelper::findContractSource2Destination(sourceDevice, contractReq.destinationAddress,
                contractReq.sourceSAP, contractReq.destinationSAP, contractReq.contractRequestId,
                contractReq.communicationServiceType);
    if (contract != NULL) {
        LOG_INFO("Contract already exists. Src=" << std::hex << sourceDevice->address32 << " dest=" << contractReq.destinationAddress << " TSAP: "
                    << std::dec << (int) contractReq.sourceSAP << "->" << (int) contractReq.destinationSAP << " contractRequestID="
                    << contractReq.contractRequestId);
        handlerResponse(sourceDevice->address32, requestID, ResponseStatus::SUCCESS);
        return;
    }

    char reason[255];
    sprintf(reason, "NEW CONTRACT request, src=%x, dest=%x, srcSAP=%d, destSAP=%d, contractRequestID=%d, communicationServiceType=%s",
                sourceDevice->address32, contractReq.destinationAddress, contractReq.sourceSAP, contractReq.destinationSAP, contractReq.contractRequestId, CommunicationServiceType::toString(contractReq.communicationServiceType).c_str());
    OperationsContainerPointer operationsContainer(new OperationsContainer(sourceDevice->address32, requestID, handlerResponse, reason));

    //special case:
    //when request is for contract GW->SM and gwTASP=2 (maybe greater), a reverse contract is needed on SM (SM->GW)
    if (sourceDevice->capabilities.isGateway() && (contractReq.destinationAddress == getManagerAddress32()) //
                && (contractReq.sourceSAP >= 2)) {

        PhyContract * contract_Manager_GW = ContractsHelper::findContractSource2Destination(subnetsContainer.manager,
                    sourceDevice->address32, contractReq.destinationSAP, contractReq.sourceSAP); //SM->GW, usually tsap: 1->2
        if (!contract_Manager_GW) { //create a new contract on SM if none was found
            ContractRequest smRequest;
            smRequest.contractRequestId = 0xFFFE; //0xFFFF is used for join contract
            smRequest.sourceAddress = getManagerAddress32();
            smRequest.sourceSAP = contractReq.destinationSAP;
            smRequest.destinationAddress = sourceDevice->address32;
            smRequest.destinationSAP = contractReq.sourceSAP;
            smRequest.communicationServiceType = contractReq.communicationServiceType;
            smRequest.committedBurst = settingsLogic->manager2NeighborCommittedBurst;
            smRequest.excessBurst = settingsLogic->manager2NeighborCommittedBurst;
            smRequest.isManagement = false;

            contract_Manager_GW = ContractsHelper::createContract(smRequest, subnetsContainer.manager, sourceDevice, *settingsLogic);
            EntityIndex entityIndex = createEntityIndex(subnetsContainer.manager->address32, EntityType::Contract,
                        contract_Manager_GW->contractID);
            IEngineOperationPointer contractOperation(new WriteAttributeOperation(contract_Manager_GW, entityIndex));
            operationsContainer->addOperation(contractOperation, subnetsContainer.manager);

            PhyNetworkContract * networkContract = ContractsHelper::createNetworkContract(contract_Manager_GW,
                        subnetsContainer.manager->address128, sourceDevice->address128);
            EntityIndex eIndexNetworkContract = createEntityIndex(subnetsContainer.manager->address32,
                        EntityType::NetworkContract, networkContract->contractID);
            // subnetsContainer.manager->phyAttributes.createNetworkContract(netM2DEntityIndex, netM2DPhyContract);
            IEngineOperationPointer networkContractOperation(new WriteAttributeOperation(networkContract, eIndexNetworkContract));
            operationsContainer->addOperation(networkContractOperation, subnetsContainer.manager);
        }
    } //special case end

    //create the requested contract
    contract = ContractsHelper::createContract(contractReq, sourceDevice, destDevice, *settingsLogic);


    if (contract->communicationServiceType == CommunicationServiceType::Periodic && settingsLogic->networkMaxLatencyPercent != 0) {
        Uint16 superframeLengthAPP = 0;
        if (!sourceDevice->capabilities.isGateway() && !sourceDevice->capabilities.isManager()){
            Subnet::PTR subnet = getSubnet(sourceDevice->address32);
            if (subnet){
                superframeLengthAPP = subnet->getSubnetSettings().getSuperframeLengthAPP();
            }
        } else if (!destDevice->capabilities.isGateway() && !destDevice->capabilities.isManager()){
            Subnet::PTR subnet = getSubnet(destDevice->address32);
            if (subnet){
                superframeLengthAPP = subnet->getSubnetSettings().getSuperframeLengthAPP();
            }
        }
        if (superframeLengthAPP == 0){
            LOG_ERROR("INCORECT Superframe length for APP (0). Default of 3000 will be used");
            superframeLengthAPP = 3000;
        }
        Uint16 startSlot;
        Uint16 maxSlotDelay;
        Uint16 periodInSlots;
        ModelUtils::getAppAvailableSlotsAndPeriod(contract, superframeLengthAPP, startSlot, maxSlotDelay, periodInSlots);
        Uint16 networkSlotDelay = periodInSlots * settingsLogic->networkMaxLatencyPercent / 100;
        contract->assignedDeadline = std::min(contract->assignedDeadline, networkSlotDelay);
    }

    EntityIndex entityIndexContract = createEntityIndex(sourceDevice->address32, EntityType::Contract, contract->contractID);
    sourceDevice->phyAttributes.createContract(entityIndexContract, contract);

    //the allocation should be done before any other creation of entities.
    //in case an error occurs the entities must be deallocated.
    if (contract->destination32 != ADDRESS16_MANAGER) {//for contract TO manager do not bandwidth traffic. It is already allocated on join.
        Device * src = getDevice(contract->source32);
        Device * dst = getDevice(contract->destination32);
        //if is not a contract request from BBR->GW or from GW->BBR
        if (!(((contract->source32 == ADDRESS16_GATEWAY) && (dst->capabilities.isBackbone())) ||
        ((src->capabilities.isBackbone()) && (contract->destination32 == ADDRESS16_GATEWAY)))) {
            ResponseStatus::ResponseStatusEnum response = theoreticEngine.allocateApplicationTraffic(contract, operationsContainer);
            if (response != ResponseStatus::SUCCESS) {
                LOG_WARN("Alloc for app returned: " << response);
                operationsContainer->setAsFail( &subnetsContainer );

                handlerResponse(contractReq.sourceAddress, requestID, response);
                sourceDevice->phyAttributes.contractsTable.erase(entityIndexContract);
                delete contract;
                return;
            }
        }
    }

    //create the associated network contract
    PhyNetworkContract * networkContract = ContractsHelper::createNetworkContract(contract, sourceDevice->address128,
                destDevice->address128);
    EntityIndex eIndexNetworkContract = createEntityIndex(sourceDevice->address32, EntityType::NetworkContract,
                networkContract->contractID);
    IEngineOperationPointer networkContractOperation(new WriteAttributeOperation(networkContract, eIndexNetworkContract));
    operationsContainer->addOperation(networkContractOperation, sourceDevice);

    //for GW as source do not add ATT. It does not needed it and produce sfc err when multiple BBR that have same short address
    if (!sourceDevice->capabilities.isGateway()){
        // check if already exist an ATT(from join)
        AddressTranslationIndexedAttribute::iterator itSrcATT = sourceDevice->phyAttributes.addressTranslationTable.end();
        for (itSrcATT = sourceDevice->phyAttributes.addressTranslationTable.begin(); itSrcATT
                    != sourceDevice->phyAttributes.addressTranslationTable.end(); ++itSrcATT) {
            AddressTranslationAttribute attAtribute = itSrcATT->second;
            if (attAtribute.getValue() != NULL && attAtribute.getValue()->longAddress == destDevice->address128
                        && attAtribute.getValue()->shortAddress == destDevice->address32) {
                break;
            }
        }

        if (itSrcATT == sourceDevice->phyAttributes.addressTranslationTable.end()) {
            //create addr translation
            PhyAddressTranslation * addressTranslation = new PhyAddressTranslation();
            addressTranslation->addressTranslationID = sourceDevice->getNextAddressTranslationID();
            addressTranslation->longAddress = destDevice->address128;
            addressTranslation->shortAddress = Address::getAddress16(destDevice->address32);

            EntityIndex entityIndexATT = createEntityIndex(sourceDevice->address32, EntityType::AddressTranslation,
                        addressTranslation->addressTranslationID);
            IEngineOperationPointer attOperation(new WriteAttributeOperation(addressTranslation, entityIndexATT));
            operationsContainer->addOperation(attOperation, sourceDevice);
        }
    }

    if (!destDevice->capabilities.isManager() && !destDevice->capabilities.isGateway()) {
        // check if already exist an ATT
        AddressTranslationIndexedAttribute::iterator itDestATT = destDevice->phyAttributes.addressTranslationTable.end();
        for (itDestATT = destDevice->phyAttributes.addressTranslationTable.begin(); itDestATT
                    != destDevice->phyAttributes.addressTranslationTable.end(); ++itDestATT) {
            AddressTranslationAttribute attAtribute = itDestATT->second;
            if (attAtribute.getValue() != NULL && attAtribute.getValue()->longAddress == sourceDevice->address128
                        && attAtribute.getValue()->shortAddress == sourceDevice->address32) {
                break;
            }
        }

        if (itDestATT == destDevice->phyAttributes.addressTranslationTable.end()) {
            //create addr translation
            PhyAddressTranslation * addressTranslation = new PhyAddressTranslation();
            addressTranslation->addressTranslationID = destDevice->getNextAddressTranslationID();
            addressTranslation->longAddress = sourceDevice->address128;
            addressTranslation->shortAddress = Address::getAddress16(sourceDevice->address32);

            EntityIndex entityIndexATT = createEntityIndex(destDevice->address32, EntityType::AddressTranslation,
                        addressTranslation->addressTranslationID);
            IEngineOperationPointer attOperation(new WriteAttributeOperation(addressTranslation, entityIndexATT));
            operationsContainer->addOperation(attOperation, destDevice);
        }
    }

    if (sourceDevice->capabilities.isGateway() && (contract->destination32 != ADDRESS16_MANAGER)){//create Net Routes only between GW and devices
        Subnet::PTR subnet = subnetsContainer.getSubnet(contract->destination32);

        NetworkRouteIndexedAttribute::iterator itPhyNetworkRoute = sourceDevice->phyAttributes.networkRoutesTable.begin();
        for (; itPhyNetworkRoute != sourceDevice->phyAttributes.networkRoutesTable.end(); ++itPhyNetworkRoute) {
            PhyNetworkRoute * networkRoute = itPhyNetworkRoute->second.getValue();
            if (networkRoute && networkRoute->destination == destDevice->address128
                && networkRoute->nextHop == subnet->getBackbone()->address128) {
                break;
            }
        }

        if (itPhyNetworkRoute == sourceDevice->phyAttributes.networkRoutesTable.end()) {
            //create Network Route GW --> device
            PhyNetworkRoute * networkRoute = new PhyNetworkRoute();
            networkRoute->networkRouteID = sourceDevice->getNextNetworkRouteID();
            networkRoute->destination = destDevice->address128;

            FAIL_ON_NULL(subnet, contractReq.sourceAddress, requestID, "Subnet not found for " << contract->destination32);
            networkRoute->nextHop = subnet->getBackbone()->address128;
            networkRoute->nwkHopLimit = 64;
            networkRoute->outgoingInterface = 1;

            EntityIndex entityIndexNetRoute = createEntityIndex(sourceDevice->address32, EntityType::NetworkRoute, networkRoute->networkRouteID);
            IEngineOperationPointer operation(new WriteAttributeOperation(networkRoute, entityIndexNetRoute));
            operationsContainer->addOperation(operation, sourceDevice);
        }
    }

    destDevice->publishers.insert(contractReq.sourceAddress);
    LOG_INFO("Device " << Address_toStream(contractReq.destinationAddress) << " : add publisher " << Address_toStream(contractReq.sourceAddress));

    //after this call, no modification of the operationsContainer should be made
    operationsProcessor.addOperationsContainer(operationsContainer);
}

void NetworkEngine::contractRenewal(const ContractRequest& contractReq, int requestID, HandlerResponse handlerResponse) {
    LOG_INFO("contractRenewal: " << contractReq);

    Device * sourceDevice = getDevice(contractReq.sourceAddress);
    FAIL_ON_NULL(sourceDevice, contractReq.sourceAddress, requestID,
                    "Source of contract already deleted: " << contractReq.sourceAddress);

    PhyContract * contract = ContractsHelper::findContract(sourceDevice, contractReq.contractId);
    if (contract == NULL) {
        LOG_INFO("Contract does not exist. Src=" << std::hex << sourceDevice->address32
                    << " dest=" << contractReq.destinationAddress << " TSAP: "
                    << std::dec << (int) contractReq.sourceSAP << "->" << (int) contractReq.destinationSAP
                    << " contractRequestID=" << contractReq.contractRequestId);
        handlerResponse(sourceDevice->address32, requestID, ResponseStatus::FAIL);
        return;
    }

    contract->requestID = contractReq.contractRequestId;
    Uint32 currentTAI = NE::Common::ClockSource::getTAI(*settingsLogic);
    contract->contract_Activation_Time = currentTAI;

    if (contractReq.contractLife == MAX_32BITS_VALUE) { //non-expiring
        contract->assigned_Contract_Life = MAX_32BITS_VALUE; // = 0;
    } else if (contractReq.contractLife > (MAX_32BITS_VALUE - currentTAI)) {
        contract->assigned_Contract_Life = MAX_32BITS_VALUE - currentTAI;
    } else {
        contract->assigned_Contract_Life = contractReq.contractLife;
    }

    handlerResponse(sourceDevice->address32, requestID, ResponseStatus::SUCCESS);
}

void NetworkEngine::modifyContract(const ContractRequest& contractReq, int requestID, HandlerResponse handlerResponse) {
    LOG_INFO("modifyContract: " << contractReq);

    Device * sourceDevice = getDevice(contractReq.sourceAddress);
    FAIL_ON_NULL(sourceDevice, contractReq.sourceAddress, requestID,
                    "Source of contract already deleted: " << contractReq.sourceAddress);

    Subnet::PTR subnet;
    if (sourceDevice->capabilities.isGateway() || sourceDevice->capabilities.isManager()){
        subnet = subnetsContainer.getSubnet(contractReq.destinationAddress);
    } else {
        subnet = subnetsContainer.getSubnet(contractReq.sourceAddress);
    }
    RETURN_ON_NULL_MSG(subnet, "modifyContract - could not find subnet for " << Address::toString(contractReq.sourceAddress));

    PhyContract * contract = ContractsHelper::findContract(sourceDevice, contractReq.contractId);
    if (contract == NULL) {
        LOG_INFO("Contract does not exist. Src=" << std::hex << sourceDevice->address32 << " dest=" << contractReq.destinationAddress << " TSAP: "
                    << std::dec << (int) contractReq.sourceSAP << "->" << (int) contractReq.destinationSAP << " contractRequestID="
                    << contractReq.contractRequestId);
        handlerResponse(sourceDevice->address32, requestID, ResponseStatus::FAIL);
        return;
    }

    //detect and discard requests that change contracts TSAPs or destination address
    if (contract->sourceSAP != contractReq.sourceSAP
                || contract->destinationSAP != contractReq.destinationSAP
                || contract->destination32 != contractReq.destinationAddress) {

        LOG_ERROR("Contract modification does not accept change of TSAPs or destination address.");
        handlerResponse(sourceDevice->address32, requestID, ResponseStatus::FAIL);
        return;
    }

    contract->requestID = contractReq.contractRequestId;
    Uint32 currentTAI = NE::Common::ClockSource::getTAI(*settingsLogic);
    contract->contract_Activation_Time = currentTAI;

    if (contractReq.contractLife == MAX_32BITS_VALUE) { //non-expiring
        contract->assigned_Contract_Life = MAX_32BITS_VALUE; // = 0;
    } else if (contractReq.contractLife > (MAX_32BITS_VALUE - currentTAI)) {
        contract->assigned_Contract_Life = MAX_32BITS_VALUE - currentTAI;
    } else {
        contract->assigned_Contract_Life = contractReq.contractLife;
    }

    contract->assigned_Contract_Priority = contractReq.contractPriority;

    contract->assignedCommittedBurst = contractReq.committedBurst;
    contract->assignedExcessBurst = contractReq.excessBurst;

    contract->assignedPeriod = contractReq.requestedPeriod;
    contract->assignedPhase = contractReq.requestedPhase;
    contract->assignedDeadline = contractReq.requestedDeadline;

    ContractToLinksMap::iterator it = sourceDevice->theoAttributes.contractLinks.begin();
    for (; it != sourceDevice->theoAttributes.contractLinks.end(); ++it){
        if (getIndex(it->first) == contract->contractID){
            subnet->moveToDirtyLinks(it->second);
            subnet->addContractToBeEvaluated(it->first);
        }
    }

    handlerResponse(sourceDevice->address32, requestID, ResponseStatus::SUCCESS);
}


PhyContract * NetworkEngine::createContractForFirmwareUpdate(const Address64& firmwareUpdateDeviceAddress64) {

    PhyContract * manager2DevicePhyContract = NULL;

    // Udo-Contract
    if (!settingsLogic->firmwareContractsEnabled) {
        return manager2DevicePhyContract;
    }

    Device * manager = subnetsContainer.manager;
    Device * firmwareUpdateDevice = getDevice(subnetsContainer.getAddress32(firmwareUpdateDeviceAddress64));
    if(!firmwareUpdateDevice) {
        LOG_ERROR("Device not found " << firmwareUpdateDeviceAddress64.toString());
        return manager2DevicePhyContract;
    }

    EntityIndex netM2DEntityIndex;
    {
        // 1. create contract Manager->Device
        ContractIndexedAttribute::iterator itContract = manager->phyAttributes.contractsTable.begin();
        for(; itContract != manager->phyAttributes.contractsTable.end(); ++itContract) {
            PhyContract * contract = itContract->second.getValue();
            if (contract && contract->usedForFirmwareUpdate
                        /*&& contract->sourceSAP == tsapSrc && contract->destinationSAP == tsapDest*/
                        && contract->destination32 == firmwareUpdateDevice->address32) {
                manager2DevicePhyContract = contract;
                break;
            }
        }
        if (itContract == manager->phyAttributes.contractsTable.end()) {
            // create contract Manager->firmwareUpdateDevice

            ContractRequest m2dContractRequest;
            m2dContractRequest.contractRequestId = 0xFFFF; //join contracts are all with contractReqID = 0xFFFF
            m2dContractRequest.sourceAddress = subnetsContainer.getManagerAddress32();
            m2dContractRequest.sourceSAP = ContractTDSAP::TDSAP_SMAP - 0xF0B0;//port of SMAP
            m2dContractRequest.destinationAddress = firmwareUpdateDevice->address32;
            m2dContractRequest.destinationSAP = ContractTDSAP::TDSAP_DMAP - 0xF0B0;
            m2dContractRequest.communicationServiceType = CommunicationServiceType::NonPeriodic;
            //        newM2DContractRequest.committedBurst = settingsLogic->committedBurst;
            Subnet::PTR subnet = subnetsContainer.getSubnet(firmwareUpdateDevice->address32);

            m2dContractRequest.committedBurst = -8; //settingsLogic->committedBurst; // 1 APDU's / sec
            m2dContractRequest.excessBurst = 1; // settingsLogic->excessBurst;
            m2dContractRequest.isManagement = true;

            // create the dll contract
            PhyContract * dllM2DPhyContract = ContractsHelper::createContract(m2dContractRequest, subnetsContainer.manager,
                        firmwareUpdateDevice, *settingsLogic);
            EntityIndex dllM2DEntityIndex = createEntityIndex(m2dContractRequest.sourceAddress, EntityType::Contract,
                      dllM2DPhyContract->contractID);
            manager->phyAttributes.createContract(dllM2DEntityIndex, dllM2DPhyContract);
            dllM2DPhyContract->usedForFirmwareUpdate = true;
            manager2DevicePhyContract = dllM2DPhyContract;

            // create the associated network contract
            PhyNetworkContract * netM2DPhyContract = ContractsHelper::createNetworkContract(dllM2DPhyContract,
                      subnetsContainer.manager->address128, firmwareUpdateDevice->address128);
            netM2DEntityIndex = createEntityIndex(m2dContractRequest.sourceAddress, EntityType::NetworkContract,
                      netM2DPhyContract->contractID);
            IEngineOperationPointer netM2DContractOperation(new WriteAttributeOperation(netM2DPhyContract, netM2DEntityIndex));
            operationsProcessor.addManagerOperation(netM2DContractOperation);

            manager->theoAttributes.udoContract2Links[dllM2DEntityIndex] = LinksList();
            theoreticEngine.addUdoContractToBeEvaluated(subnet, dllM2DEntityIndex);
            theoreticEngine.updateUdoContract(subnet, dllM2DEntityIndex, -8);
        }
    }

    return manager2DevicePhyContract;
}

void NetworkEngine::deleteContractForFirmwareUpdate(const Address64& firmwareUpdateDeviceAddress64) {

    // Udo-Contract
    if (!settingsLogic->firmwareContractsEnabled) {
        return;
    }


    Device *manager = getDevice(getManagerAddress32());
    Device *firmwareUpdateDevice = getDevice(subnetsContainer.getAddress32(firmwareUpdateDeviceAddress64));
    RETURN_ON_NULL_MSG(firmwareUpdateDevice, "device not found " << firmwareUpdateDeviceAddress64.toString());

    {
        PhyContract * contract = NULL;
        ContractIndexedAttribute::iterator itContract = manager->phyAttributes.contractsTable.begin();
        for(; itContract != manager->phyAttributes.contractsTable.end(); ++itContract) {
            contract = itContract->second.getValue();
            if (contract && contract->destination32 == firmwareUpdateDevice->address32
                        && contract->usedForFirmwareUpdate) {
                break;
            }
        }

        if (itContract != manager->phyAttributes.contractsTable.end()) {

            EntityIndex eiContract = itContract->first;

            // remove from udoContract2Links
            ContractToLinksMap::iterator itContract2Links = manager->theoAttributes.udoContract2Links.find(eiContract);
            if (itContract2Links == manager->theoAttributes.udoContract2Links.end()) {
                LOG_WARN("Dirty udo contract not in udoContract2Links: " << std::hex << eiContract);
            } else {
                manager->theoAttributes.udoContract2Links.erase(itContract2Links);
            }

            Subnet::PTR subnet = subnetsContainer.getSubnet(firmwareUpdateDevice->address32);
            // delete Links For UdoContract
            theoreticEngine.updateLinksForUdoContract(subnet, eiContract, contract->destination32, true);

            // delete the dll contract
            IEngineOperationPointer dllM2DContractOperation(new DeleteAttributeOperation(itContract->first));
            operationsProcessor.addManagerOperation(dllM2DContractOperation);

            // delete the associated network contract
            EntityIndex eiNetContract = createEntityIndex(getDeviceAddress(itContract->first), EntityType::NetworkContract,
                      getIndex(itContract->first));
            IEngineOperationPointer netM2DContractOperation(new DeleteAttributeOperation(eiNetContract));
            operationsProcessor.addManagerOperation(netM2DContractOperation);
        }
    }

}

void NetworkEngine::addCandidate(Device *existingDevice, const PhyCandidate& candidate) {

    if (existingDevice->capabilities.isGateway()) {
        return;
    }
    Subnet::PTR subnet = subnetsContainer.getSubnet(existingDevice->capabilities.dllSubnetId);
    RETURN_ON_NULL_MSG(subnet, "Could not find the subnet with ID " << (int) existingDevice->capabilities.dllSubnetId);

	// Star Topology
    if (!subnet->getSubnetSettings().enableMultiPath
                || subnet->getSubnetSettings().enableStarTopology
                || subnet->getSubnetSettings().nrRoutersPerBBR <= 1) {
        return;
    }

    Address32 candidateAddress = Address::createAddress32(existingDevice->capabilities.dllSubnetId, candidate.neighbor);

    if (existingDevice->address32 == candidateAddress) {
        LOG_WARN("addCandidate existingDevice is the same with candidate!");
        return;
    }

    Device *visibleNeighbor = getDevice(candidateAddress);
    RETURN_ON_NULL_MSG(visibleNeighbor, "Device " << Address_toStream(candidateAddress) << " not found. Candidate not added.");

    Int8 candidateLevel = subnet->getDeviceLevel(candidateAddress);
    if (subnet->getSubnetSettings().maxNumberOfLayers > 0 && candidateLevel >= subnet->getSubnetSettings().maxNumberOfLayers) {
        LOG_INFO("addCandidate fail : candidate level(" << (int)candidateLevel << ") >= SubnetSettings::maxNumberOfLayers("
                    << (int)subnet->getSubnetSettings().maxNumberOfLayers << ")");
        return;
    }

    LOG_INFO("Adding candidate: " << Address_toStream(candidateAddress) << " for: " << Address_toStream(existingDevice->address32));

    PhyCandidate *phyCandidate = new PhyCandidate(candidate);

    EntityIndex entityIndex = NE::Model::createEntityIndex(existingDevice->address32, EntityType::Candidate,
                phyCandidate->neighbor);
    existingDevice->phyAttributes.createCandidate(entityIndex, phyCandidate);

    bool candidateEvaluated = existingDevice->theoAttributes.isCandidateOnList(Address::getAddress16(visibleNeighbor->address32));

    PhyCandidate * existingPhyCandidate = new PhyCandidate();
    existingPhyCandidate->neighbor = NE::Common::Address::getAddress16(existingDevice->address32);
    existingPhyCandidate->radio = phyCandidate->radio;
    entityIndex = NE::Model::createEntityIndex(visibleNeighbor->address32, EntityType::Candidate, existingPhyCandidate->neighbor);
    visibleNeighbor->phyAttributes.createCandidate(entityIndex, existingPhyCandidate);

    bool existingDeviceEvaluated = visibleNeighbor->theoAttributes.isCandidateOnList(Address::getAddress16(existingDevice->address32));

    EdgePointer deviceNeighborEdge = subnet->getEdge(existingDevice->address32, visibleNeighbor->address32);
    if (deviceNeighborEdge == NULL) {
        Routing::EdgePointer edge(new Routing::Edge(existingDevice->address32, visibleNeighbor->address32));
        edge->setEdgeStatus(Status::CANDIDATE);
        edge->setRSQI(phyCandidate->radio);
        subnet->addEdge(edge);
        candidateEvaluated = false;
    } else {
        LOG_INFO("update RSQI (" <<(int)phyCandidate->radio << ") for edge : " << *deviceNeighborEdge);
        deviceNeighborEdge->setRSQI(phyCandidate->radio);
        if(deviceNeighborEdge->calculateRadioQualityLevel(phyCandidate->radio) != deviceNeighborEdge->getRadioQualityLevel()) {
            if(candidateEvaluated) {
                existingDevice->theoAttributes.deleteCandidate(Address::getAddress16(visibleNeighbor->address32));
                candidateEvaluated = false;
            }
        }
    }

    EdgePointer neighborDeviceEdge = subnet->getEdge(visibleNeighbor->address32, existingDevice->address32);
    if (neighborDeviceEdge == NULL) {//add also the reversed edge
        Routing::EdgePointer edge(new Routing::Edge(visibleNeighbor->address32, existingDevice->address32));
        edge->setEdgeStatus(Status::CANDIDATE);
        edge->setRSQI(phyCandidate->radio);
        subnet->addEdge(edge);
        existingDeviceEvaluated = false;
    } else {
        LOG_INFO("update RSQI (" <<(int)phyCandidate->radio << ") for edge : " << *neighborDeviceEdge);
        neighborDeviceEdge->setRSQI(phyCandidate->radio);
        if(neighborDeviceEdge->calculateRadioQualityLevel(phyCandidate->radio) != neighborDeviceEdge->getRadioQualityLevel()) {
            if(existingDeviceEvaluated) {
                visibleNeighbor->theoAttributes.deleteCandidate(Address::getAddress16(existingDevice->address32));
                existingDeviceEvaluated = false;
            }
        }
    }

    if(!candidateEvaluated && (visibleNeighbor->capabilities.isRouting() || visibleNeighbor->capabilities.isBackbone()) &&
                (visibleNeighbor->address32 != existingDevice->parent32)) {
        if (existingDevice->theoAttributes.candidateIsBadRate(Address::getAddress16(visibleNeighbor->address32))) {
            existingDevice->theoAttributes.deleteCandidate(Address::getAddress16(visibleNeighbor->address32));
            candidateEvaluated = true;
        } else {
            subnet->addDeviceCandidate(Address::getAddress16(existingDevice->address32),
                        Address::getAddress16(visibleNeighbor->address32));
        }
    }

    if(!existingDeviceEvaluated &&  (existingDevice->capabilities.isRouting()|| existingDevice->capabilities.isBackbone()) &&
                (existingDevice->address32 != visibleNeighbor->parent32)) {
        if (visibleNeighbor->theoAttributes.candidateIsBadRate(Address::getAddress16(existingDevice->address32))) {
            visibleNeighbor->theoAttributes.deleteCandidate(Address::getAddress16(existingDevice->address32));
            existingDeviceEvaluated = true;

        } else {
            subnet->addDeviceCandidate(Address::getAddress16(visibleNeighbor->address32),
                Address::getAddress16(existingDevice->address32));
        }
    }

    GraphPointer gp = subnet->getGraph(DEFAULT_GRAPH_ID);
    RETURN_ON_NULL_MSG(gp, "Default graph is NULL.");
    bool existingHasDoubleExit = existingDevice->capabilities.isBackbone()? true : gp->deviceHasDoubleExit(Address::getAddress16(existingDevice->address32), Address::getAddress16(existingDevice->parent32));
    bool candidateHaseDoubleExit = visibleNeighbor->capabilities.isBackbone() ? true : gp->deviceHasDoubleExit(Address::getAddress16(candidateAddress), Address::getAddress16(visibleNeighbor->parent32));

    Int8 deviceLevel = subnet->getDeviceLevel(existingDevice->address32);
    // Int8 candidateLevel = subnet->getDeviceLevel(visibleNeighbor->address32);

    if ((!existingHasDoubleExit && (candidateLevel <= deviceLevel) && !candidateEvaluated ) ||
                (!candidateHaseDoubleExit &&  (candidateLevel >= deviceLevel) && !existingDeviceEvaluated)) {

        subnet->addGraphToBeEvaluated(DEFAULT_GRAPH_ID);//add default inbound graph to be evaluated
        return;
    }



}

void NetworkEngine::addDiagnostics(Device *device, Device *neighborDevice, Int8 rsl, Uint8 rslQual, Uint16 sent,
            Uint16 received, Uint16 failed, Uint16 nack) {
    //diagnostics reported by device related to its communication with neighborDevice

    LOG_INFO("addDiagnostics: reported by " << Address_toStream(device->address32) //
                << ", neighbor=" << Address_toStream(neighborDevice->address32) << ", RSSI=" << (int) rsl //
                << ", RSQI=" << (int) rslQual << ", sent=" << (int) sent << ", received=" << (int) received //
                << ", failed=" << (int) failed << ", nack=" << (int) nack );

    Uint16 neighborReceived = sent + nack;
    Uint16 neighborSent = received;

    device->updateStatistics(sent + nack, received, failed, 0); //TODO - failed reception?
    neighborDevice->updateStatistics(neighborSent, neighborReceived, 0, 0); //TODO - failed reception + failed transmission

    Subnet::PTR subnet = subnetsContainer.getSubnet(device->address32);
    RETURN_ON_NULL_MSG(subnet, "subnet not found for address " << Address_toStream(device->address32));

    GraphPointer graph = subnet->getGraph(DEFAULT_GRAPH_ID);
    if ( !graph ) {
        return;
    }

    if ( device->theoAttributes.candidateIsBadRate(Address::getAddress16(neighborDevice->address32))) {
        return;
    }

    Uint16 nrPublishForFailRate = subnet->getSubnetSettings().noOfPublishForFailRate;
    Uint16 nrLastSentPackages = subnet->getSubnetSettings().numberOfSentPackagesForFailRate;
    Uint16 badTxRateThreshold = subnet->getSubnetSettings().badTransferRateThreshlod;

    Uint16 nrPublishForFailRateShort = subnet->getSubnetSettings().noOfPublishForFailRateShortPeriod;
    Uint16 nrLastSentPackagesShort = subnet->getSubnetSettings().numberOfSentPackagesForFailRateShort;
    Uint16 badTxRateThresholdShort = subnet->getSubnetSettings().badTransferRateThreshlodShort;

    bool isPublishForBackup  = false;

    if ( graph->getBackupFor(Address::getAddress16(device->address32)) == Address::getAddress16( neighborDevice->address32) ) {
        nrPublishForFailRate = subnet->getSubnetSettings().noOfPublishForFailRateOnBackup;
        nrPublishForFailRateShort = subnet->getSubnetSettings().noOfPublishForFailRateOnBackupShortPeriod;
        nrLastSentPackages = subnet->getSubnetSettings().numberOfSentPackagesForFailRateOnBackup;
        nrLastSentPackagesShort = subnet->getSubnetSettings().numberOfSentPackagesForFailRateOnBackupShort;
        badTxRateThreshold = subnet->getSubnetSettings().badTransferRateThreshlodOnBackup;
        badTxRateThresholdShort = subnet->getSubnetSettings().badTransferRateThreshlodOnBackupShort;
        isPublishForBackup = true;
    }

    if (nrPublishForFailRate < nrPublishForFailRateShort) {
        Uint16 pivot = nrPublishForFailRate;
        nrPublishForFailRate = nrPublishForFailRateShort;
        nrPublishForFailRateShort = pivot;
    }

    //update statistics on outbound edges; will be used for NeighborHealthReport
    EdgePointer outboundEdgeToNeighbor = subnet->getEdge(device->address32, neighborDevice->address32);
    if (outboundEdgeToNeighbor) {
        outboundEdgeToNeighbor->addDiagnostics(rsl, rslQual, sent + nack, received, failed, nrPublishForFailRate, nrPublishForFailRateShort);
    }

    EdgePointer outboundEdgeToDevice = subnet->getEdge(neighborDevice->address32, device->address32);
    if (outboundEdgeToDevice) {
        outboundEdgeToDevice->addDiagnostics(rsl, rslQual, neighborSent, neighborReceived, 0, nrPublishForFailRate, nrPublishForFailRateShort); //failed?
    }

	// Star Topology
    if (!subnet->getSubnetSettings().enableMultiPath
                || subnet->getSubnetSettings().enableStarTopology
                || subnet->getSubnetSettings().nrRoutersPerBBR <= 1) {
        return;
    }


    if ((subnet->getDeviceLevel(device->address32) == 1) && device->capabilities.isRouting()
                && subnet->getSubnetSettings().freezeLevelOneRouters ) {//don't redirect level one devices
        return;
    }

    //update statistics on outbound edges; will be used for NeighborHealthReport
    if (outboundEdgeToNeighbor) {
        Device* backbone = subnet->getBackbone();
        if ( !backbone ) {
            return;
        }
        Address16 const addr = Address::getAddress16(device->address32);
        Address16 const bkp = graph->getBackupFor(addr);
        if ( backbone->deviceHasOutboundGraph(device->address32)
                    && subnet->deviceHasHighFailRateWithNeighbor(device, neighborDevice)
                    && outboundEdgeToNeighbor->isWorst(subnet->getEdge(addr, bkp))
                    && subnet->isEnabledInboundGraphEvaluation()) {

            if (device->parent32 == neighborDevice->address32) {
                char reason[128];
                sprintf(reason, "redirectDeviceToNewParent, device=%x, oldParent=%x",  device->address32, neighborDevice->address32);

                OperationsContainerPointer operationsContainer(new OperationsContainer(reason));
                bool detectedNewParent = theoreticEngine.detectNewParentForDevice(subnet, device, device, operationsContainer, true);
                if (!detectedNewParent) {
                    theoreticEngine.retryToFindNewParent(subnet, device, neighborDevice, operationsContainer);
                }
                if (!operationsContainer->isContainerEmpty()) {
                    sprintf(reason, "redirectDeviceToNewParent %x, oldParent=%x, newParent=%x", device->address32, neighborDevice->address32, device->parent32);
                    LOG_INFO(reason);
                	operationsContainer->reasonOfOperations = reason;

                    LOG_DEBUG("GRAPH EVAL disable in addDiagnostics");
                    subnet->disableInboundGraphEvaluation();
                    ChainWaitForConfirmOnEvalGraphPointer chainConfirmGraphOperations(new ChainWaitForConfirmOnEvalGraph(subnet, DEFAULT_GRAPH_ID));
                    operationsContainer->addHandlerResponse(boost::bind(&ChainWaitForConfirmOnEvalGraph::process, chainConfirmGraphOperations, _1, _2, _3));
                    operationsProcessor.addOperationsContainer(operationsContainer);
                }
            } else if ( isPublishForBackup ) {
                subnet->addBadRateCandidate(device, neighborDevice);
                theoreticEngine.onChangeParentsReevaluateGraphs(subnet, device, false);
                subnet->graphsToBeEvaluated.push_front(DEFAULT_GRAPH_ID);

                if (outboundEdgeToNeighbor) {
                    outboundEdgeToNeighbor->resetEdgeDiagnosticsOnChangeParent();
                }
                if (outboundEdgeToDevice) {
                    outboundEdgeToDevice->resetEdgeDiagnosticsOnChangeParent();
                }
            }
        }
    }
}

void NetworkEngine::addDiagnostics(Device *device, const PhyChannelDiag& chDiag) {
    std::vector<Uint8> channelsCCABackoffs;
    for (int i = 0; i <= 15; ++i) { //16 channels
        if (chDiag.channelTransmissionList[i].ccaBackoff < 0) { //5 or fewer attempted transmissions
            Uint8 positiveVal = -1 - chDiag.channelTransmissionList[i].ccaBackoff;
            channelsCCABackoffs.push_back(positiveVal);
        } else if (chDiag.channelTransmissionList[i].ccaBackoff > 0) {
            channelsCCABackoffs.push_back(chDiag.channelTransmissionList[i].ccaBackoff - 1);
        } else {
            channelsCCABackoffs.push_back(0);
        }
    }

    device->updateStatistics(channelsCCABackoffs);
}

void NetworkEngine::removeExpiredKeys() {
    Uint32 currentTAI = ClockSource::getTAI(*settingsLogic);

	OperationsContainerPointer operationsContainer(new OperationsContainer("delete expired keys"));

    SubnetsMap& subnetsMap = subnetsContainer.getSubnetsList();
    for (SubnetsMap::iterator itSubnets = subnetsMap.begin(); itSubnets != subnetsMap.end(); ++itSubnets) {
        Address16Set activeDevices = itSubnets->second->getActiveDevices();

        //check sm keys -sm not in activeDevices
        activeDevices.insert(ADDRESS16_MANAGER);

        for (Address16Set::const_iterator itDevices = activeDevices.begin(); itDevices != activeDevices.end(); ++itDevices) {

            Device *device = itSubnets->second->getDevice(*itDevices);
            if (!device) {
                continue;
            }

            //session keys
            for (SessionKeyIndexedAttribute::iterator itKeys = device->phyAttributes.sessionKeysTable.begin(); itKeys
                        != device->phyAttributes.sessionKeysTable.end(); ) {

                //search for keys with expired hardLifeTime
                if (itKeys->second.currentValue
                            && (itKeys->second.currentValue->sessionKeyPolicy.KeyHardLifetime > 0) //keys with hardLifeTime=0 are non-expiring
                            && (itKeys->second.currentValue->hardLifeTime < currentTAI)) {
                    LOG_INFO("Found expired key on [" << Address_toStream(device->address32) << "] "
                                << device->address64.toString() << ", keyID=" << (int) itKeys->second.currentValue->keyID
                                << ", dest=" << itKeys->second.currentValue->destination64.toString()
                                << ", hardLifeTime=" << (long long)itKeys->second.currentValue->hardLifeTime
                                << ", currentTAI=" << (long long) currentTAI);

                    //remove key from model
                    delete itKeys->second.currentValue;
                    itKeys->second.currentValue = NULL;
                    delete itKeys->second.previousValue;
                    itKeys->second.previousValue = NULL;
                    device->phyAttributes.sessionKeysTable.erase(itKeys++);

                    //enable printing of attributes tables
                    if(device->capabilities.isGateway() || device->capabilities.isManager()) {
                        device->hasChanged = true;
                    }
                    else {
                        itSubnets->second->changedDevices.push_back(Address::getAddress16(device->address32));
                        device->hasChanged = true;
                    }


                } else {
                    ++itKeys;
                }
            }

            //master keys
            for (SpecialKeyIndexedAttribute::iterator itKeys = device->phyAttributes.masterKeysTable.begin(); itKeys
                        != device->phyAttributes.masterKeysTable.end(); ) {
                //search for keys with expired hardLifeTime
                if (itKeys->second.currentValue
                            && (itKeys->second.currentValue->policy.KeyHardLifetime > 0) //keys with hardLifeTime=0 are non-expiring
                            && (itKeys->second.currentValue->hardLifeTime < currentTAI)) {
                    LOG_INFO("Found expired master key on [" << Address_toStream(device->address32) << "] "
                                << device->address64.toString() << " keyID=" << (int) itKeys->second.currentValue->keyID);

                    //remove key from model
                    delete itKeys->second.currentValue;
                    itKeys->second.currentValue = NULL;
                    delete itKeys->second.previousValue;
                    itKeys->second.previousValue = NULL;
                    device->phyAttributes.masterKeysTable.erase(itKeys++);

                    //enable printing of attributes tables
                    if(device->capabilities.isGateway() || device->capabilities.isManager()) {
                        device->hasChanged = true;
                    }
                    else {
                        itSubnets->second->changedDevices.push_back(Address::getAddress16(device->address32));
                        device->hasChanged = true;
                    }


                } else {
                    ++itKeys;
                }
            }

            //subnet keys
            for (SpecialKeyIndexedAttribute::iterator itKeys = device->phyAttributes.subnetKeysTable.begin(); itKeys
                        != device->phyAttributes.subnetKeysTable.end(); ) {
                //search for keys with expired hardLifeTime
                if (itKeys->second.currentValue
                            && (itKeys->second.currentValue->policy.KeyHardLifetime > 0) //keys with hardLifeTime=0 are non-expiring
                            && (itKeys->second.currentValue->hardLifeTime < currentTAI)) {
                    LOG_INFO("Found expired subnet key on [" << Address_toStream(device->address32) << "] "
                                << device->address64.toString() << " keyID=" << (int) itKeys->second.currentValue->keyID);

                    //remove key from model
                    delete itKeys->second.currentValue;
                    itKeys->second.currentValue = NULL;
                    delete itKeys->second.previousValue;
                    itKeys->second.previousValue = NULL;
                    device->phyAttributes.subnetKeysTable.erase(itKeys++);

                    //enable printing of attributes tables
                    if(device->capabilities.isGateway() || device->capabilities.isManager()) {
                        device->hasChanged = true;
                    }
                    else {
                        itSubnets->second->changedDevices.push_back(Address::getAddress16(device->address32));
                        device->hasChanged = true;
                    }

                } else {
                    ++itKeys;
                }
            }
        }
    }


    //after this call, no modification of the operationsContainer should be made
    operationsProcessor.addOperationsContainer(operationsContainer);

}

void NetworkEngine::removeSessionKeys(Device *src, Device *dst, int sourceTSAP, int destinationTSAP,
            Operations::OperationsContainerPointer& operationsContainer) {

    if(!dst) {
        return;
    }

    LOG_INFO("Delete session keys between " << src->address64.toString() << " and " << dst->address64.toString()
                << ", tsaps " << sourceTSAP << "->" << destinationTSAP);

    //OBS:
    //1.on a device there can be two contracts with the same destination and tsaps; for example: one periodic and one non-periodic
    //2.session keys on peers go together; if we delete the key on one device we have to delete the peer key on the peer device


    //do not delete keys if multiple contracts are sharing it
    if (src->hasMultipleContractsWithPeerOnSameTSAP(dst->address32, sourceTSAP, destinationTSAP)) {
        LOG_INFO("No key deleted - multiple contracts on source are sharing the session.");
        return;
    }

    //do not delete keys if there is a contract on destination using this session
    PhyContract * contractOnDest =
        ContractsHelper::findContractSource2Destination(dst, src->address32, destinationTSAP, sourceTSAP);
    if (contractOnDest) {
        LOG_INFO("No key deleted - at least one contract on destination is using the session.");
        return;
    }

    //search and delete key on source
    for (SessionKeyIndexedAttribute::iterator itKeys = src->phyAttributes.sessionKeysTable.begin(); itKeys
                != src->phyAttributes.sessionKeysTable.end(); ++itKeys) {
        if (itKeys->second.currentValue && (itKeys->second.currentValue->destination64 == dst->address64) &&
                    (itKeys->second.currentValue->sourceTSAP == sourceTSAP) &&
                    (itKeys->second.currentValue->destinationTSAP == destinationTSAP)){
            LOG_INFO("found key to remove on [" << Address_toStream(src->address32) << "] "
                        << src->address64.toString() << " keyID=" << (int) itKeys->second.currentValue->keyID
                        << " dest=" << itKeys->second.currentValue->destination64.toString());
            DeleteAttributeOperationPointer deleteKey(new DeleteAttributeOperation(itKeys->first));
            operationsContainer->addOperation(deleteKey, src);

        }
    }

    //search and delete the peer key
    for (SessionKeyIndexedAttribute::iterator itKeys = dst->phyAttributes.sessionKeysTable.begin(); itKeys
                != dst->phyAttributes.sessionKeysTable.end(); ++itKeys) {
        if (itKeys->second.currentValue && (itKeys->second.currentValue->destination64 == src->address64) &&
                    (itKeys->second.currentValue->sourceTSAP == destinationTSAP) &&
                    (itKeys->second.currentValue->destinationTSAP == sourceTSAP)){
            LOG_INFO("found key to remove on [" << Address_toStream(dst->address32) << "] "
                        << dst->address64.toString() << " keyID=" << (int) itKeys->second.currentValue->keyID
                        << " dest=" << itKeys->second.currentValue->destination64.toString());
            DeleteAttributeOperationPointer deleteKey(new DeleteAttributeOperation(itKeys->first));
            operationsContainer->addOperation(deleteKey, dst);

        }
    }
}

void NetworkEngine::periodicTerminateExpiredContracts() {
    Uint32 currentTAI = ClockSource::getTAI(*settingsLogic);

    SubnetsMap& subnetsMap = subnetsContainer.getSubnetsList();
    for (SubnetsMap::iterator itSubnets = subnetsMap.begin(); itSubnets != subnetsMap.end(); ++itSubnets) {
        const Address16Set& activeDevices = itSubnets->second->getActiveDevices();
        for (Address16Set::const_iterator itDevices = activeDevices.begin(); itDevices != activeDevices.end(); ++itDevices) {
            if (*itDevices == ADDRESS16_MANAGER) {
                continue;
            }

            Device *device = itSubnets->second->getDevice(*itDevices);
            if (!device) {
                continue;
            }

            bool deviceTerminateContracts = false;

            for (ContractIndexedAttribute::iterator itContract = device->phyAttributes.contractsTable.begin(); itContract
                        != device->phyAttributes.contractsTable.end(); ++itContract) {

                PhyContract * phyContract = itContract->second.getValue();
                if (!phyContract) {
                    continue;
                }

                //if (phyContract->assigned_Contract_Life == 0) { //non-expiring contract
                if (phyContract->assigned_Contract_Life == MAX_32BITS_VALUE
                            || phyContract->assigned_Contract_Life == 0) { //non-expiring contract
                    continue;
                }

                if (currentTAI >= (phyContract->contract_Activation_Time + phyContract->assigned_Contract_Life)) {
                    LOG_INFO("Expired contract detected: TAI=" << (int) currentTAI << " Contract: " << *phyContract);
                    deviceTerminateContracts = true;
                    terminateContract(phyContract->source32, phyContract->contractID);
                }
            }

            if (deviceTerminateContracts) {
                return;
            }
        }
    }
}

void NetworkEngine::terminateContract(Address32 src, ContractId contractId) {
    Device *source = subnetsContainer.getDevice(src);

    if(!source) {
        LOG_ERROR("Device not found" << Address_toStream(src));
        return;
    }

    PhyContract *contract =  ContractsHelper::findContract( source, contractId) ;
    if(!contract) {
        LOG_ERROR("Contract with ID:" << (int)contractId << " not found on device: "<< Address_toStream(src));
        return;
    }

    char reason[128];
    sprintf(reason, "TERMINATE CONTRACT request, device=%x, contractID=%d", source->address32, contractId);
    LOG_INFO(reason);
    OperationsContainerPointer operationsContainer(new OperationsContainer(reason));

    Device *destination = subnetsContainer.getDevice(contract->destination32);

    //remove session keys

    removeSessionKeys(source, destination, contract->sourceSAP, contract->destinationSAP, operationsContainer);

    //remove network contract
    EntityIndex eIndexNetworkContract = createEntityIndex(source->address32, EntityType::NetworkContract, contractId);
    DeleteAttributeOperationPointer deleteNetworkContract(new DeleteAttributeOperation(eIndexNetworkContract));
    operationsContainer->addOperation(deleteNetworkContract, source);

    //remove ATT_table if this is the last contract with peer
    if(source->hasOnlyOneContractWithPeer(contract->destination32, contractId)) {
        removeATT(contract->destination32, source, operationsContainer);
    }

    //if is a local loop contract..remove route for contract and ATT on subscriber
    if((contract->source32 != ADDRESS16_GATEWAY) && ( contract->destination32 != ADDRESS16_GATEWAY) &&
    (contract->source32 != ADDRESS16_MANAGER) && (contract->destination32 != ADDRESS16_MANAGER)) {
        removeLocalLoopRouteForContract(source, contractId, operationsContainer);
        if(destination && destination->hasOnlyOneContractWithPeer(contract->source32, contractId)) {
            removeATT(source->address32, destination, operationsContainer);
        }
    }


    //delete contract
    EntityIndex entityIndexContract = createEntityIndex(source->address32, EntityType::Contract,  contractId);
    DeleteAttributeOperationPointer deleteContract(new DeleteAttributeOperation(entityIndexContract));
    operationsContainer->addOperation(deleteContract, source);

    // for Non Periodic contracts GW=>Device, application links are not allocated on contract.
    if((contract->source32 != ADDRESS16_GATEWAY)
                && (contract->source32 != ADDRESS16_MANAGER)) {
        Subnet::PTR subnet = subnetsContainer.getSubnet(contract->source32);
        if(subnet) {
            subnet->addContractToBeEvaluated(entityIndexContract);
        }
    }

    Device* dstDevice = subnetsContainer.getDevice(contract->destination32);
    if (dstDevice) {
        EntityIndex key = createEntityIndex(contract->source32, EntityType::Contract, contract->contractID);
        dstDevice->theoAttributes.removeServedContract(key);
        LOG_INFO("Device " << Address_toStream(dstDevice->address32) << ", remove served contract: " << std::hex << key);
    }

    if (contract->source32 == ADDRESS16_GATEWAY
                && contract->communicationServiceType == CommunicationServiceType::NonPeriodic
                && contract->destination32 != ADDRESS16_MANAGER) {//for C/S contract GW->device recalculate the APP outbount traffic
        Subnet::PTR subnet = subnetsContainer.getSubnet(contract->destination32);
        if (subnet == NULL){
            LOG_ERROR("NO subnet for " << Address_toStream(contract->destination32));
            return;
        }

        theoreticEngine.evaluateAppOutboundTraffic(contract->destination32, subnet, operationsContainer);
    }

    operationsProcessor.addOperationsContainer(operationsContainer);
}

SubnetsMap& NetworkEngine::getSubnetsList() {
    return subnetsContainer.getSubnetsList();
}

Subnet::PTR NetworkEngine::getSubnet(Uint16 subnetId) {
    return subnetsContainer.getSubnet(subnetId);
}

Subnet::PTR NetworkEngine::getSubnet(Address32 address32) {
    return subnetsContainer.getSubnet(address32);
}

void NetworkEngine::periodicEvaluation(NE::Common::EvaluationSignal evaluationSignal, Uint32 currentTime) {

    operationsProcessor.cleanEmptyContainers(currentTime, settingsLogic->containerExpirationInterval);

    for (SubnetsContainer::iterator itSubnet = subnetsContainer.begin(); itSubnet != subnetsContainer.end(); ++itSubnet) {
        postJoinTask(itSubnet->second, currentTime);
        if (NE::Common::isHRCOReconfigureSignal(evaluationSignal)) {
            itSubnet->second->periodicStartPublish(currentTime, operationsProcessor);
            itSubnet->second->periodicConfigureAlerts(currentTime, operationsProcessor);
        }
        if(NE::Common::isIgnoredDevicesSignal(evaluationSignal)) {
            itSubnet->second->evaluateIgnoredDevices(currentTime, operationsProcessor);
        }
        theoreticEngine.periodicEvaluation(itSubnet->second, evaluationSignal, currentTime);
        if(NE::Common::isBlackListCheckSignal(evaluationSignal)) {
            itSubnet->second->checkExpiringBlacklists(currentTime, operationsProcessor);
        }
    }
}

void NetworkEngine::postJoinTask(Subnet::PTR& subnet, Uint32 currentTime){
    for (Address32Set::iterator it = subnet->postJoinTaskDevices.begin(); it != subnet->postJoinTaskDevices.end(); ++it){
        Device * device = getDevice(*it);
        if(device == NULL){
            subnet->postJoinTaskDevices.erase(it);
            break;
        }

        if ( (currentTime - subnet->getSubnetSettings().delayPostJoinTasks) >= device->joinConfirmTime){
            if (!device->capabilities.isBackbone()){
                readMetadata(*it);
            }

            subnet->postJoinTaskDevices.erase(it);
            break;
        }

    }
}

void NetworkEngine::verifyInactiveDevices(Uint32 period) {
    OperationsContainerPointer operationsContainer(new OperationsContainer("verify inactive devices"));

    SubnetsMap& subnetsMap = subnetsContainer.getSubnetsList();
    Uint32 currentTAI = ClockSource::getTAI(*settingsLogic);
    Uint32 currentTime = NE::Common::ClockSource::getCurrentTime();
    for (SubnetsMap::iterator itSubnets = subnetsMap.begin(); itSubnets != subnetsMap.end(); ++itSubnets) {
        const Address16Set& activeDevices = itSubnets->second->getActiveDevices();
        for (Address16Set::const_iterator itDevices = activeDevices.begin(); itDevices != activeDevices.end(); ++itDevices) {
            if (*itDevices == ADDRESS16_MANAGER) {
                continue;
            }
            Device *device = itSubnets->second->getDevice(*itDevices);
            if (!device) {
                continue;
            }

            if (device->statusForReports != StatusForReports::JOINED_AND_CONFIGURED) {
                //remove device if 5 minutes have elapsed since join start time and device is not yet JOINED_AND_CONFIGURED
                if ((currentTime - device->startTime) > settingsLogic->joinMaxDuration ) {
                    LOG_INFO("Join timeout! Elapsed time from join start: " << currentTime - device->startTime
                                << ". Removing device (status=" << (int) device->statusForReports << ")..");
                    removeDeviceOnError(device->address32, operationsContainer, RemoveDeviceReason::joinTimeout);
                    break; //should break because removeDeviceOnError erase from activeDevices and we are in a loop in activeDevices
                }
                continue;
            }

            Uint32 interval = currentTAI - device->lastPacketTAI;
            if ((device->lastPacketTAI != 0) && (interval >= period)) {
                LOG_WARN("Found inactive device: [" << Address_toStream(device->address32) << "] "
                                << device->address64.toString() //
                                << std::dec << " (period=" << interval << ")");
                EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::PowerSupply, 0);
                //EntityIndex entityIndex = createEntityIndex(device->address32, EntityType::PackagesStatistics, 0);
                Operations::IEngineOperationPointer operation(new Operations::ReadAttributeOperation(entityIndex));
                operationsContainer->addOperation(operation, device);
                itSubnets->second->addNotPublishingDevice(device->address32);
            }
        }
    }


    //after this call, no modification of the operationsContainer should be made
    operationsProcessor.addOperationsContainer(operationsContainer);

}

void NetworkEngine::removeSubscriber(const Device* subscriber, Operations::OperationsContainerPointer & container) {

    if (subscriber) {
        LOG_INFO("removeSubscriber - subscriber=" << Address_toStream(subscriber->address32));

        Subnet::PTR subnet = subnetsContainer.getSubnet(subscriber->address32);

        Uint16 outboundGraphID = subnet->getBackbone()->getOutBoundGraph(getAddress16(subscriber->address32));

        for(Address32Set::iterator itDeviceAddress32 = subscriber->publishers.begin(); itDeviceAddress32 != subscriber->publishers.end(); ++itDeviceAddress32) {

            Device * publisher = subnet->getDevice(*itDeviceAddress32);
            if (!publisher) {
                continue;
            }

            removeLocalLoopRoute(subscriber, publisher, outboundGraphID, container);
            removeATT(subscriber->address32, publisher, container);
            removeNetworkContracts(subscriber, publisher, container);
            removeContracts(subscriber, publisher, container);
            removeKeys(subscriber, publisher, container);
        }
    }
}

void NetworkEngine::removeLocalLoopRoute(const Device* removingDevice, Device* targetDevice, Uint16 outboundGraphID,
            Operations::OperationsContainerPointer & container) {
    if (targetDevice) {
        RouteIndexedAttribute::iterator itRoute = targetDevice->phyAttributes.routesTable.begin();
        for (; itRoute != targetDevice->phyAttributes.routesTable.end(); ++itRoute) {
            PhyRoute* route = (PhyRoute*) itRoute->second.getValue();
            if (route && route->alternative == 1 && route->route.size() == 2 && route->route[1] == (ROUTE_GRAPH_MASK | outboundGraphID)) {
                DeleteAttributeOperationPointer deleteRoute(new DeleteAttributeOperation(itRoute->first));
                container->addOperation(deleteRoute, targetDevice);
            }
        }
    }
}

void NetworkEngine::removeLocalLoopRouteForContract(Device* publisher, Uint16 contractID,
            Operations::OperationsContainerPointer & container) {
    if (publisher) {
        RouteIndexedAttribute::iterator itRoute = publisher->phyAttributes.routesTable.begin();
        for (; itRoute != publisher->phyAttributes.routesTable.end(); ++itRoute) {
            PhyRoute* route = (PhyRoute*) itRoute->second.getValue();
            if (route &&  route->alternative == 1 && route->selector == contractID) {
                DeleteAttributeOperationPointer deleteRoute(new DeleteAttributeOperation(itRoute->first));
                container->addOperation(deleteRoute, publisher);
            }
        }
    }
}

void NetworkEngine::removeATT(const Address32 removingDeviceAddress32, Device* targetDevice,
            Operations::OperationsContainerPointer & container) {

    if (targetDevice) {
        for (AddressTranslationIndexedAttribute::iterator it = targetDevice->phyAttributes.addressTranslationTable.begin(); it
                    != targetDevice->phyAttributes.addressTranslationTable.end(); ++it) {
            PhyAddressTranslation * att = it->second.getValue();

            if (att && att->shortAddress == Address::getAddress16(removingDeviceAddress32)) {
                DeleteAttributeOperationPointer deleteATT(new DeleteAttributeOperation(it->first));
                container->addOperation(deleteATT, targetDevice);
                break;
            }
        }
    }
}

void NetworkEngine::removeNetworkRoute(const Device* removingDevice, Device* targetDevice,
            Operations::OperationsContainerPointer & container) {
    if (targetDevice) {
        NetworkRouteIndexedAttribute::iterator itNetRoute = targetDevice->phyAttributes.networkRoutesTable.begin();
        for (; itNetRoute != targetDevice->phyAttributes.networkRoutesTable.end(); ++itNetRoute) {
            PhyNetworkRoute* netRoute = (PhyNetworkRoute*) itNetRoute->second.getValue();

            if (netRoute && netRoute->destination == removingDevice->address128) {
                DeleteAttributeOperationPointer deleteNetworkRoute(new DeleteAttributeOperation(itNetRoute->first));
                container->addOperation(deleteNetworkRoute, targetDevice);
            }
        }
    }
}

void NetworkEngine::removeNetworkRoute(Uint16 removingSubnet, Device* targetDevice,
            Operations::OperationsContainerPointer & container) {
    if (targetDevice) {
        NetworkRouteIndexedAttribute::iterator itNetRoute = targetDevice->phyAttributes.networkRoutesTable.begin();
        for (; itNetRoute != targetDevice->phyAttributes.networkRoutesTable.end(); ++itNetRoute) {
            PhyNetworkRoute* netRoute = (PhyNetworkRoute*) itNetRoute->second.getValue();
            if(netRoute) {
                Address32 destination32 = getAddress32(netRoute->destination);
                if (getSubnetId(destination32) == removingSubnet) {
                    DeleteAttributeOperationPointer deleteNetworkRoute(new DeleteAttributeOperation(itNetRoute->first));
                    container->addOperation(deleteNetworkRoute, targetDevice);
                }
            }
        }
    }
}

void NetworkEngine::removeContracts(const Device* removingDevice, Device* targetDevice,
            Operations::OperationsContainerPointer & container) {
    if (targetDevice) {
        // remove DMO contracts
        ContractIndexedAttribute::iterator itContracts = targetDevice->phyAttributes.contractsTable.begin();
        for (; itContracts != targetDevice->phyAttributes.contractsTable.end(); ++itContracts) {
            PhyContract* contract = (PhyContract*) itContracts->second.getValue();

            if (contract && (contract->destination32 == removingDevice->address32)) {
                DeleteAttributeOperationPointer deleteContract(new DeleteAttributeOperation(itContracts->first));
                container->addOperation(deleteContract, targetDevice);
            }
        }
    }
}

void NetworkEngine::removeContracts(Uint16 removingSubnet, Device* targetDevice,
            Operations::OperationsContainerPointer & container) {
    if (targetDevice) {
        // remove DMO contracts
        ContractIndexedAttribute::iterator itContracts = targetDevice->phyAttributes.contractsTable.begin();
        for (; itContracts != targetDevice->phyAttributes.contractsTable.end(); ++itContracts) {
            PhyContract* contract = (PhyContract*) itContracts->second.getValue();

            if (contract
                        && !subnetsContainer.isGatewayAddress(contract->destination32)
                        && getSubnetId(contract->destination32) == removingSubnet) {
                DeleteAttributeOperationPointer deleteContract(new DeleteAttributeOperation(itContracts->first));
                container->addOperation(deleteContract, targetDevice);
            }
        }
    }
}

void NetworkEngine::removeNetworkContracts(const Device* removingDevice, Device* targetDevice,
            Operations::OperationsContainerPointer & container) {
    if (targetDevice) {
        // remove NL contracts
        NetworkContractIndexedAttribute::iterator itNetContracts = targetDevice->phyAttributes.networkContractsTable.begin();
        for (; itNetContracts != targetDevice->phyAttributes.networkContractsTable.end(); ++itNetContracts) {
            PhyNetworkContract* networkContract = (PhyNetworkContract*) itNetContracts->second.getValue();

            if (networkContract && networkContract->destinationAddress == removingDevice->address128) {
                DeleteAttributeOperationPointer deleteContract(new DeleteAttributeOperation(itNetContracts->first));
                container->addOperation(deleteContract, targetDevice);
            }
        }
    }
}

void NetworkEngine::removeNetworkContracts(Uint16 removingSubnet, Device* targetDevice,
            Operations::OperationsContainerPointer & container) {
    if (targetDevice) {
        // remove NL contracts
        NetworkContractIndexedAttribute::iterator itNetContracts = targetDevice->phyAttributes.networkContractsTable.begin();
        for (; itNetContracts != targetDevice->phyAttributes.networkContractsTable.end(); ++itNetContracts) {
            PhyNetworkContract* networkContract = (PhyNetworkContract*) itNetContracts->second.getValue();

            if(!networkContract) {
                continue;
            }

            Address32 destination32 = getAddress32(networkContract->destinationAddress);

            if (subnetsContainer.isGatewayAddress(destination32)) {
                continue;
            }

            if (getSubnetId(destination32) == removingSubnet) {
                DeleteAttributeOperationPointer deleteContract(new DeleteAttributeOperation(itNetContracts->first));
                container->addOperation(deleteContract, targetDevice);
            }
        }
    }
}

void NetworkEngine::removeKeys(const Device* removingDevice, Device* targetDevice,
            Operations::OperationsContainerPointer & container) {
    if (targetDevice) {
        // remove keys
        SessionKeyIndexedAttribute::iterator itKeys = targetDevice->phyAttributes.sessionKeysTable.begin();
        for (; itKeys != targetDevice->phyAttributes.sessionKeysTable.end(); ++itKeys) {
            PhySessionKey* key = (PhySessionKey*) itKeys->second.getValue();
            if(!key) {
                continue;
            }

            if (key->destination128 == removingDevice->address128) {
                DeleteAttributeOperationPointer deleteKey(new DeleteAttributeOperation(itKeys->first));
                container->addOperation(deleteKey, targetDevice);
            }
        }
    }
}

void NetworkEngine::removeKeys(Uint16 removingSubnet, Device* targetDevice, Operations::OperationsContainerPointer & container) {
    if (targetDevice) {
        // remove keys
        SessionKeyIndexedAttribute::iterator itKeys = targetDevice->phyAttributes.sessionKeysTable.begin();
        for (; itKeys != targetDevice->phyAttributes.sessionKeysTable.end(); ++itKeys) {
            PhySessionKey* key = (PhySessionKey*) itKeys->second.getValue();
            if(!key) {
                continue;
            }

            Address32 destination32 = getAddress32(key->destination128);
            if (getSubnetId(destination32) == removingSubnet) {
                DeleteAttributeOperationPointer deleteKey(new DeleteAttributeOperation(itKeys->first));
                container->addOperation(deleteKey, targetDevice);
            }
        }
    }
}

void NetworkEngine::removeDeviceOnError(Address32 removingDeviceAddress32, Operations::OperationsContainerPointer container,
            int reason) {

    if (subnetsContainer.isGatewayAddress(removingDeviceAddress32)) {
        return;
    }

    Device* removingDevice = subnetsContainer.getDevice(removingDeviceAddress32);
    RETURN_ON_NULL_MSG(removingDevice, "Remove device : " << Address::toString(removingDeviceAddress32) << " - not exist!");

    Subnet::PTR subnet = subnetsContainer.getSubnet(removingDeviceAddress32);
    RETURN_ON_NULL_MSG(subnet, "Remove device : subnet for " << Address::toString(removingDeviceAddress32) << " - not exist!");

    Uint32 currentTime = time(NULL);
    int diff = (currentTime - removingDevice->startTime);
    int diffS = (diff % 60);
    int diffM = (diff / 60) % 60;
    int diffH = (diff / 3600);
    LOG_WARN("Remove device : " << Address::toString(removingDeviceAddress32)
                                << " " << DeviceType::toString(removingDevice->capabilities.deviceType)
                                << " " << StatusForReports::toString(removingDevice->statusForReports)
                                << " parent: " << Address::toString(removingDevice->parent32)
//                                << " neighb: " << (int)((removingDevice->phyAttributes.neighborMetadata.getValue() != NULL) ? removingDevice->phyAttributes.neighborMetadata.getValue()->used : 0)
                                << " neighb: " << (int)removingDevice->phyAttributes.neighborsTable.size()
                                << " chlds: " << (int)subnet->getInboundChildrenNo( removingDevice )
                                << " Activity: " << diffH << ":" << diffM << ":" << diffS);

    //send remove alert
    Uint32 currentTAI = ClockSource::getTAI(*settingsLogic);
    if (reason == RemoveDeviceReason::joinTimeout) {
        //LOG_INFO("[SORIN] alert join timeout");
        AlertJoinFailed *alertJoinFailed = new AlertJoinFailed(removingDevice->address64,
                                                                (Uint8) removingDevice->statusForReports,
                                                                (Uint8) JoinFailureReason::timeout);
        AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoinFailed, alertJoinFailed, currentTAI));
        operationsProcessor.sendAlert(alertOperation);
    } else {
        if (removingDevice->statusForReports != StatusForReports::JOINED_AND_CONFIGURED) {
            //LOG_INFO("[SORIN] alert join failed (on removeDeviceOnError)");
            AlertJoinFailed *alertJoinFailed = new AlertJoinFailed(removingDevice->address64,
                                                                    (Uint8) removingDevice->statusForReports,
                                                                    (Uint8) reason);
            AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoinFailed, alertJoinFailed, currentTAI));
            operationsProcessor.sendAlert(alertOperation);
        } else {
            //LOG_INFO("[SORIN] alert device leave");
            AlertLeave *alertLeave = new AlertLeave(removingDevice->address64, (Uint8) reason);
            AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceLeave, alertLeave, currentTAI));
            operationsProcessor.sendAlert(alertOperation);
        }
    }

    Device* managerDevice = subnet->getManager();
    Device* gatewayDevice = subnet->getGateway();

    //remove device from the list of reading rejoin reason
    subnet->listDeviceToReadJoinReason.remove(Address::getAddress16(removingDeviceAddress32));

    if (!removingDevice->capabilities.isBackbone()) // standard device
    {
        Device* backboneDevice = subnet->getBackbone();
        if (!backboneDevice) {
            LOG_ERROR("Backbone device ain't here! subnet:" << subnet->getSubnetId());
            return;
        }

        OperationsList generatedOperations;

        theoreticEngine.removeDeviceFromOutBoundGraphs(subnet, backboneDevice, removingDevice, generatedOperations);

        subnet->deleteTheoAttributesChild(removingDevice->parent32, removingDevice->address64);

        Uint16 removingAddress16 = getAddress16(removingDeviceAddress32);

        // mark udo contracts that passing through removing device to be evaluated
        for(LinkToContractMap::iterator it = removingDevice->theoAttributes.link2UdoContract.begin();
            it != removingDevice->theoAttributes.link2UdoContract.end(); ++it) {

            theoreticEngine.addUdoContractToBeEvaluated(subnet, it->second);
            theoreticEngine.updateUdoContract( subnet, it->second, -8);
        }


        removeSubscriber(removingDevice, container);

        removeNetworkRoute(removingDevice, managerDevice, container);
        removeNetworkRoute(removingDevice, gatewayDevice, container);
        removeNetworkRoute(removingDevice, backboneDevice, container);

        removeContracts(removingDevice, managerDevice, container);
        removeNetworkContracts(removingDevice, managerDevice, container);
        removeContracts(removingDevice, gatewayDevice, container);
        removeNetworkContracts(removingDevice, gatewayDevice, container);

        removeKeys(removingDevice, managerDevice, container);
        removeKeys(removingDevice, gatewayDevice, container);

        // to do : remove for local loops

        // clean DL layer

        // steps through removed device's neighbors
        LOG_DEBUG("device->phyAttributes.neighborsTable.size() : " << (int) removingDevice->phyAttributes.neighborsTable.size());
        NeighborIndexedAttribute::iterator itNeighbor = removingDevice->phyAttributes.neighborsTable.begin();
        for (; itNeighbor != removingDevice->phyAttributes.neighborsTable.end(); ++itNeighbor) {
            PhyNeighbor* neighbor = (PhyNeighbor*) itNeighbor->second.getValue();
            if(!neighbor) {
                continue;
            }

            Address32 neighborAddress32 = subnet->getAddress32(neighbor->address64);
            LOG_DEBUG("neighborAddress32 : " << Address::toString(neighborAddress32));

            Device* neighborDevice = subnetsContainer.getDevice(neighborAddress32);
            if (!neighborDevice) {
                LOG_DEBUG("neighborDevice for address " << Address::toString(neighborAddress32) << " not found!");
                continue;
            }

            // generate remove NEIGHBOR operations
            NeighborIndexedAttribute::iterator itRemDevNeighbor = neighborDevice->phyAttributes.neighborsTable.begin();
            for (; itRemDevNeighbor != neighborDevice->phyAttributes.neighborsTable.end(); ++itRemDevNeighbor) {
                PhyNeighbor* neighborD = (PhyNeighbor*) itRemDevNeighbor->second.getValue();
                if(!neighborD) {
                    continue;
                }

                if (subnet->getAddress32(neighborD->address64) == removingDeviceAddress32) {
                    DeleteAttributeOperationPointer deleteNeighbor(new DeleteAttributeOperation(itRemDevNeighbor->first));
                    container->addOperation(deleteNeighbor, neighborDevice);
                }
            }

            // generate remove LINK operations
            theoreticEngine.deleteLinks(subnet, neighborDevice, removingDevice, container, true);

        }


        // steps through removed device's candidates
        LOG_DEBUG("device->phyAttributes.candidatesTable.size() : " << (int) removingDevice->phyAttributes.candidatesTable.size());
        CandidateIndexedAttribute::iterator itCandidate = removingDevice->phyAttributes.candidatesTable.begin();
        for (; itCandidate != removingDevice->phyAttributes.candidatesTable.end(); ++itCandidate) {
            PhyCandidate* candidate = (PhyCandidate*) itCandidate->second.getValue();
            if(!candidate) {
                continue;
            }

            Address32 candidateAddress32 = Address::createAddress32(subnet->getSubnetId(), candidate->neighbor);
            LOG_DEBUG("candidateAddress32 : " << Address::toString(candidateAddress32));

            Device* candidateDevice = subnetsContainer.getDevice(candidateAddress32);
            if (!candidateDevice) {
            	LOG_INFO("candidateDevice for address " << Address::toString(candidateAddress32) << " not found!");
                continue;
            }

            // remove from candidate's candidates table
            EntityIndex entityIndex = NE::Model::createEntityIndex(candidateDevice->address32, EntityType::Candidate,
                            Address::getAddress16(removingDeviceAddress32));
            candidateDevice->phyAttributes.removeCandidate(entityIndex);
            candidateDevice->theoAttributes.deleteCandidate(Address::getAddress16(removingDeviceAddress32));

        }

        // deallocate slots
        LinkIndexedAttribute::iterator itLink;
        for (itLink = removingDevice->phyAttributes.linksTable.begin(); itLink != removingDevice->phyAttributes.linksTable.end(); ++itLink) {
            subnet->unreserveLink(removingDevice, itLink->second.getValue(), container.get());
        }

        if (removingDevice->capabilities.isRouting()) {
            theoreticEngine.redirectChildParent(subnet, removingDevice);
        }

        // clean DL routes from BBR
        RouteIndexedAttribute::iterator itRoute = backboneDevice->phyAttributes.routesTable.begin();
        for (; itRoute != backboneDevice->phyAttributes.routesTable.end(); ++itRoute) {
            PhyRoute* phyRoute = (PhyRoute*) itRoute->second.getValue();
            if(!phyRoute) {
                continue;
            }

            if (phyRoute->alternative == 2 && phyRoute->selector == removingAddress16) {
                if (phyRoute->route.size() == 1 && isRouteGraphElement(phyRoute->route[0])) {//mark the graph for verification
                    subnet->addGraphToBeRemoved(getRouteElement(phyRoute->route[0]));
                }
                DeleteAttributeOperationPointer deleteRoute(new DeleteAttributeOperation(itRoute->first));
                container->addOperation(deleteRoute, backboneDevice);
            }
        }

        //Delete ATT on BBR
        for (AddressTranslationIndexedAttribute::iterator it = backboneDevice->phyAttributes.addressTranslationTable.begin(); it
                    != backboneDevice->phyAttributes.addressTranslationTable.end(); ++it) {
            PhyAddressTranslation * att = it->second.getValue();
            if(!att) {
                continue;
            }

            if (att->shortAddress == Address::getAddress16(removingDevice->address32)) {
                DeleteAttributeOperationPointer deleteATT(new DeleteAttributeOperation(it->first));
                container->addOperation(deleteATT, backboneDevice);
                break;
            }
        }

        //add to evaluation all graphs that pass through removedDevice
        theoreticEngine.onChangeParentsReevaluateGraphs(subnet, removingDevice, true);

        Device* parent = subnet->getDevice(removingDevice->parent32);
        if (parent) {
            parent->decrementChildId();
            parent->setDirtyInbound();
        }

        subnet->markServedContractsDirty(removingDevice);
        //remove the links(of removing device) from dirty links because
        //..after a fast rejoin the links are created with same ID and the dirty link task could delete a wrong link
        subnet->removeFromDirtyLinks(removingDevice);

        if (removingDevice->capabilities.isRouting()){//remove router from listOfAdvertisingRouters
            subnet->listOfAdvertisingRouters.remove(Address::getAddress16(removingDevice->address32));
        }

        theoreticEngine.removeDevice(removingDeviceAddress32, subnet);
        subnet->destroyDevice(removingDevice->address128, removingDevice->address64, removingDeviceAddress32);

        if(!generatedOperations.empty()) {
            for (OperationsList::iterator it = container->getUnsentOperations().begin(); it != container->getUnsentOperations().end(); ++it) {
            	if ( Address::getAddress16((*it)->getOwner()) != ADDRESS16_GATEWAY
            			&& Address::getAddress16((*it)->getOwner()) != ADDRESS16_MANAGER ) {
            		(*it)->addOperationDependencies(generatedOperations);
            	}
            }

            for(OperationsList::iterator itOper = generatedOperations.begin(); itOper != generatedOperations.end(); ++itOper) {
                Device* owner = subnet->getDevice((*itOper)->getOwner());
                if(owner) {
                    container->addOperation(*itOper, owner);
                }
                else {
                    LOG_INFO("OWNER is null for operation=" << **itOper);
                }
            }

        }

    } else // remove backbone device -> destroy whole network
    {
        for(Address32 i = 0; i < MAX_NUMBER_OF_DEVICES; ++i) {
            Device *device = subnet->getDevice(i);
            if (!device || (device && (device->capabilities.isManager() || device->capabilities.isBackbone() || device->capabilities.isGateway()))) {
                continue;
            }

            //send remove alert
            if (device->statusForReports != StatusForReports::JOINED_AND_CONFIGURED) {
                //LOG_INFO("[SORIN] alert join failed (on remove bbr)");
                AlertJoinFailed *alertJoinFailed = new AlertJoinFailed(device->address64,
                                                                        (Uint8) device->statusForReports,
                                                                        (Uint8) JoinFailureReason::parentLeft); //RemoveDeviceReason::parentLeft
                AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceJoinFailed, alertJoinFailed, currentTAI));
                operationsProcessor.sendAlert(alertOperation);
            } else {
                //LOG_INFO("[SORIN] alert device leave (on remove bbr)");
                AlertLeave *alertLeave = new AlertLeave(device->address64, (Uint8) RemoveDeviceReason::parentLeft);
                AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::DeviceLeave, alertLeave, currentTAI));
                operationsProcessor.sendAlert(alertOperation);
            }

            //remove device from the list of reading rejoin reason
            subnet->listDeviceToReadJoinReason.remove(Address::getAddress16(device->address32));

            removeNetworkRoute(device, managerDevice, container);
            removeNetworkRoute(device, gatewayDevice, container);
            //removeNetworkRoute(device, backboneDevice, container);
            removeNetworkRoute(device, removingDevice, container); //removingDevice is backboneDevice

            removeContracts(device, managerDevice, container);
            removeNetworkContracts(device, managerDevice, container);
            removeContracts(device, gatewayDevice, container);
            removeNetworkContracts(device, gatewayDevice, container);

            removeKeys(device, managerDevice, container);
            removeKeys(device, gatewayDevice, container);
        }


        // delete removing device settings from Manager/GW
        Uint16 removingSubnet = getSubnetId(removingDeviceAddress32);

        removeNetworkRoute(removingSubnet, managerDevice, container);
        removeNetworkRoute(removingSubnet, gatewayDevice, container);

        removeContracts(removingSubnet, managerDevice, container);
        removeNetworkContracts(removingSubnet, managerDevice, container);
        removeContracts(removingSubnet, gatewayDevice, container);
        removeNetworkContracts(removingSubnet, gatewayDevice, container);

        removeKeys(removingSubnet, managerDevice, container);
        removeKeys(removingSubnet, gatewayDevice, container);

        // to do : remove for local loops

        theoreticEngine.removeDevice(removingDeviceAddress32, subnet);
        subnet->destroyDevice(removingDevice->address128, removingDevice->address64, removingDeviceAddress32);
        Subnet::PTR nextSubnet(new Subnet(removingSubnet, subnet->getManager(), settingsLogic));
        nextSubnet->copySubnetShort(subnet);
        subnetsContainer.removeSubnet(removingSubnet);
        subnetsContainer.addSubnet(nextSubnet);
    }

    LOG_DEBUG("Operations generated on remove device " << Address::toString(removingDeviceAddress32) << *container);
}

void NetworkEngine::processChannelBlackListing(const Model::Tdma::ChannelDiagnostics& channelDiags, const Address32& source ) {
    Subnet::PTR subnet = subnetsContainer.getSubnet(source);
    if(subnet) {
        subnet->processChannelDiag(channelDiags, source, operationsProcessor);

    }
}


}
}

