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
 * ReportsEngine.cpp
 *
 *  Created on: Oct 2, 2009
 *      Author: flori.parauan
 */

#include "ReportsEngine.h"

#define BROADCAST_ADDRESS_32 0xFFFFFFFF
#define BROADCAST_ADDRESS_128 "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF"

using namespace NE::Common;

namespace NE {
namespace Model {
namespace Reports {

Uint8 CHANNEL_LIST[16] = { 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 };

ReportsEngine::ReportsEngine(NE::Model::SubnetsContainer * subnetsContainer_, NE::Common::SettingsLogic * settingsLogic_,
            Uint32 startTime_) :
    subnetsContainer(subnetsContainer_), //
    settingsLogic(settingsLogic_), //
    startTime(startTime_), //
    globalJoinCounter(0), DPDUsSent(0), DPDUsLost(0) {

}

void ReportsEngine::getDeviceReport(Address16 deviceAddress, Subnet::PTR subnet, DeviceListReport& deviceListReport) {

    Device *device = subnet->getDevice(deviceAddress);
    RETURN_ON_NULL(device);

    LOG_DEBUG("[REPORT] getDeviceReport for " << Address::toString(device->address32) << " status=" << (int) device->status
                << " isEligible=" << (int) device->isEligibleForReport());

    DeviceInfo devInfo(device->address32, device->capabilities.deviceType,
            device->phyAttributes.powerSupplyStatus.getValue() ? device->phyAttributes.powerSupplyStatus.getValue()->value : 0,
            device->statusForReports,
            device->phyAttributes.vendorID.getValue() ? device->phyAttributes.vendorID.getValue()->value : "?",
            device->phyAttributes.modelID.getValue() ? device->phyAttributes.modelID.getValue()->value : "?",
            device->phyAttributes.softwareRevisionInformation.getValue() ? device->phyAttributes.softwareRevisionInformation.getValue()->value : "?",
            device->capabilities.tagName,
            device->phyAttributes.serialNumber.getValue() ? device->phyAttributes.serialNumber.getValue()->value : "?");
    deviceListReport.push_back(devInfo);
}

void ReportsEngine::getDeviceReport(Device *device, NetworkOrderStream& stream, Uint16 & nrOfDevices) {

    RETURN_ON_NULL(device);

    LOG_DEBUG("[REPORT] getDeviceReport for " << Address::toString(device->address32) << " status=" << (int) device->status
                << " isEligible=" << (int) device->isEligibleForReport());

    device->address128.marshall(stream);

    stream.write(device->capabilities.deviceType);

    device->address64.marshall(stream);

    stream.write((Uint8)(device->phyAttributes.powerSupplyStatus.getValue() ? device->phyAttributes.powerSupplyStatus.getValue()->value : 0));

    stream.write((Uint8) device->statusForReports);

    VisibleString vendorID = device->phyAttributes.vendorID.getValue() ? device->phyAttributes.vendorID.getValue()->value : "?";
    Bytes manufacturer(vendorID.begin(), vendorID.end());
    stream.write((Uint8) manufacturer.size());
    stream.write(manufacturer);

    VisibleString modelID = device->phyAttributes.modelID.getValue() ? device->phyAttributes.modelID.getValue()->value : "?";
    Bytes model(modelID.begin(), modelID.end());
    stream.write((Uint8) model.size());
    stream.write(model);

    VisibleString softwareRevisionInformation = device->phyAttributes.softwareRevisionInformation.getValue() ? device->phyAttributes.softwareRevisionInformation.getValue()->value : "?";
    Bytes revision(softwareRevisionInformation.begin(), softwareRevisionInformation.end());
    stream.write((Uint8) revision.size());
    stream.write(revision);

    Bytes deviceTag(device->capabilities.tagName.begin(), device->capabilities.tagName.end());
    stream.write((Uint8) deviceTag.size());
    stream.write(deviceTag);

    VisibleString serialNumber = device->phyAttributes.serialNumber.getValue() ? device->phyAttributes.serialNumber.getValue()->value : "?";
    Bytes serialNo(serialNumber.begin(), serialNumber.end());
    stream.write((Uint8) serialNo.size());
    stream.write(serialNo);

    ++nrOfDevices;
}

void ReportsEngine::createDeviceListReport(DeviceListReport& deviceListReport) {
    //get info for manager
    Address32 managerAddress32 = subnetsContainer->getManagerAddress32();
    Device* manager = subnetsContainer->getDevice(managerAddress32);
    if (!manager) {
        std::ostringstream errStream;
        errStream << "Manager not found.";
        LOG_ERROR(errStream.str());
        throw NEException(errStream.str());
    }
    Uint8 powerSupplyStatus = 0;
    if (manager->phyAttributes.powerSupplyStatus.getValue()) {
        powerSupplyStatus = manager->phyAttributes.powerSupplyStatus.getValue()->value;
    }
    DeviceInfo devInfo(manager->address32, manager->capabilities.deviceType,
            manager->phyAttributes.powerSupplyStatus.getValue() ? manager->phyAttributes.powerSupplyStatus.getValue()->value : 0,
    		manager->statusForReports,
    		manager->phyAttributes.vendorID.getValue() ? manager->phyAttributes.vendorID.getValue()->value : "?",
    		manager->phyAttributes.modelID.getValue() ? manager->phyAttributes.modelID.getValue()->value : "?",
    		manager->phyAttributes.softwareRevisionInformation.getValue() ? manager->phyAttributes.softwareRevisionInformation.getValue()->value : "?",
			manager->capabilities.tagName,
			manager->phyAttributes.serialNumber.getValue() ? manager->phyAttributes.serialNumber.getValue()->value : "?");
    deviceListReport.push_back(devInfo);

    //get info for gateway
    Device* gateway = subnetsContainer->getGateway();
    if (gateway && gateway->isJoinConfirmed()) {
        DeviceInfo devInfo(gateway->address32, gateway->capabilities.deviceType,
					gateway->phyAttributes.powerSupplyStatus.getValue() ? gateway->phyAttributes.powerSupplyStatus.getValue()->value : 0,
					gateway->statusForReports,
					gateway->phyAttributes.vendorID.getValue() ? gateway->phyAttributes.vendorID.getValue()->value : "?",
					gateway->phyAttributes.modelID.getValue() ? gateway->phyAttributes.modelID.getValue()->value : "?",
					gateway->phyAttributes.softwareRevisionInformation.getValue() ? gateway->phyAttributes.softwareRevisionInformation.getValue()->value : "?",
					gateway->capabilities.tagName,
					gateway->phyAttributes.serialNumber.getValue() ? gateway->phyAttributes.serialNumber.getValue()->value : "?");
        deviceListReport.push_back(devInfo);
    }

    //get info for the rest of devices
    SubnetsMap &subnetsMap = subnetsContainer->getSubnetsList();

    //getDeviceReport(gatewayAddress16, subnetsContainer->getSubnet(managerSubnet), deviceListReport);

    for (SubnetsMap::iterator it = subnetsMap.begin(); it != subnetsMap.end(); ++it) {
        LOG_DEBUG("traverse subnet " << (int) it->first);
        const Address16Set& activeDevices = it->second->getActiveDevices();
        for (Address16Set::const_iterator itDevices = activeDevices.begin(); itDevices != activeDevices.end(); ++itDevices) {

            if (!(*itDevices == it->second->getAddress16(subnetsContainer->manager->address32)) && !(*itDevices
                        == it->second->getAddress16(subnetsContainer->getGateway()->address32))) {

                getDeviceReport(*itDevices, it->second, deviceListReport);
            }
        }
    }
}

void ReportsEngine::createDeviceListReport(NetworkOrderStream& stream) {

    // 1. nrOfDevices
    long savedNrOfDevicesPos = stream.ostream.tellp();
    Uint16 nrOfDevices = 0;
    stream.write(nrOfDevices);

    //get info for manager
    Address32 managerAddress32 = subnetsContainer->getManagerAddress32();
    Device* managerDevice = subnetsContainer->getDevice(managerAddress32);
    if (!managerDevice) {
        std::ostringstream errStream;
        errStream << "Manager not found.";
        LOG_ERROR(errStream.str());
        throw NEException(errStream.str());
    }

    getDeviceReport(subnetsContainer->manager, stream, nrOfDevices);

    //get info for gateway
    Device* gatewayDevice = subnetsContainer->getGateway();
    if (gatewayDevice && gatewayDevice->isJoinConfirmed()) {
        getDeviceReport(subnetsContainer->getGateway(), stream, nrOfDevices);
    }

    //get info for the rest of devices
    SubnetsMap &subnetsMap = subnetsContainer->getSubnetsList();
    for (SubnetsMap::iterator itSubnetPair = subnetsMap.begin(); itSubnetPair != subnetsMap.end(); ++itSubnetPair) {
        LOG_DEBUG("traverse subnet " << (int) itSubnetPair->first);
        const Address16Set& activeDevices = itSubnetPair->second->getActiveDevices();
        for (Address16Set::const_iterator itAddress16 = activeDevices.begin(); itAddress16 != activeDevices.end(); ++itAddress16) {

            Device* device = itSubnetPair->second->getDevice(*itAddress16);
            if (device && (*itAddress16 != ADDRESS16_MANAGER) && (*itAddress16 != ADDRESS16_GATEWAY)) {

                getDeviceReport(device, stream, nrOfDevices);
            }
        }
    }

    if (nrOfDevices > 0) {
        long savedCrtPos = stream.ostream.tellp();

        // update nrOfDevices
        stream.ostream.seekp(savedNrOfDevicesPos);
        stream.write(nrOfDevices);

        // seek to the saved current position
        stream.ostream.seekp(savedCrtPos);
    }
}

void ReportsEngine::getTopologyReportEntry(Subnet::PTR & subnet, Device& device, TopologyReport& topologyReport) {
    LOG_DEBUG("[REPORT] getTopologyReportEntry for " << device.address64.toString());

    Topology::Device topologyDevice;
    topologyDevice.networkAddress = device.address32;

    //add neighbors
    for (NeighborIndexedAttribute::iterator itNeighbors = device.phyAttributes.neighborsTable.begin(); itNeighbors
                != device.phyAttributes.neighborsTable.end(); ++itNeighbors) {

        PhyNeighbor* neighbor = itNeighbors->second.getValue();
        if (neighbor == NULL) {
            continue;
        }

        Address32 neighborAddress32 = subnet->getAddress32(neighbor->address64);
        Device *neighborDevice = subnetsContainer->getDevice(neighborAddress32);
        //TODO check if isEligibleForReport condition is needed
        if (!neighborDevice || !neighborDevice->isEligibleForReport()) {
            continue;
        }

        Topology::Device::Neighbor deviceNeighbor;
        deviceNeighbor.neighborAddress = neighborAddress32;
        deviceNeighbor.clockSource = neighbor->clockSource;

        LOG_DEBUG("[REPORT]      add neighbor: " << neighbor->address64.toString() << ", addr32:"
                    << NE::Common::Address::toString(neighborAddress32));

        topologyDevice.neighborList.push_back(deviceNeighbor);
    }

    //add graphs
    for (GraphIndexedAttribute::iterator itGraphs = device.phyAttributes.graphsTable.begin(); itGraphs
                != device.phyAttributes.graphsTable.end(); ++itGraphs) {

        PhyGraph* graph = itGraphs->second.getValue();
        if (graph == NULL) {
            continue;
        }

        Topology::Device::Graph graphEntry;
        graphEntry.graphIdentifier = graph->index;
        LOG_DEBUG("[REPORT]      add graph " << (int) graph->index);
        for (std::vector<Uint16>::iterator itNodes = graph->neighbors.begin(); itNodes != graph->neighbors.end(); ++itNodes) {
            Address32 neighborAddress32 = NE::Common::Address::createAddress32(device.capabilities.dllSubnetId, *itNodes);
            Device *neighborDevice = subnetsContainer->getDevice(neighborAddress32);
            //TODO check if isEligibleForReport condition is needed
            if (!neighborDevice || !neighborDevice->isEligibleForReport()) {
                continue;
            }
            graphEntry.graphMemberList.push_back(neighborAddress32);
            LOG_DEBUG("[REPORT]          graph member: " << NE::Common::Address::toString(neighborAddress32));
        }

        topologyDevice.graphList.push_back(graphEntry);
    }

    topologyReport.deviceList.push_back(topologyDevice);

    //add backbone info if this device is backbone
    if (device.capabilities.isBackbone()) {
        Topology::Backbone topologyBackbone;
        topologyBackbone.backboneAddress = device.address32;
        topologyBackbone.subnetID = device.capabilities.dllSubnetId;

        topologyReport.backboneList.push_back(topologyBackbone);
    }
}

void ReportsEngine::getTopologyReportEntry(Subnet::PTR & subnet, Device& device, NetworkOrderStream & stream, NetworkOrderStream & backboneStream, Uint16 & nrOfDevices, Uint16 & nrOfBackbones) {
    LOG_DEBUG("[REPORT] getTopologyReportEntry for " << device.address64.toString());

    device.address128.marshall(stream);

    Uint16 nrOfNeighbors = 0;
    Uint16 nrOfGraphs = 0;

    long savedNrOfNeighborsPos = stream.ostream.tellp();
    stream.write(nrOfNeighbors);

    long savedNrOfGraphsPos = stream.ostream.tellp();
    stream.write(nrOfGraphs);

    // 1. add neighbors
    for (NeighborIndexedAttribute::iterator itNeighbors = device.phyAttributes.neighborsTable.begin(); itNeighbors
                != device.phyAttributes.neighborsTable.end(); ++itNeighbors) {

        PhyNeighbor* neighbor = itNeighbors->second.getValue();
        if (neighbor == NULL) {
            continue;
        }

        Address32 neighborAddress32 = subnet->getAddress32(neighbor->address64);
        Device *neighborDevice = subnetsContainer->getDevice(neighborAddress32);

        if (!neighborDevice || !neighborDevice->isEligibleForReport()) {
            continue;
        }

        neighborDevice->address128.marshall(stream);
        stream.write(neighbor->clockSource);

        LOG_DEBUG("[REPORT]      add neighbor: " << neighbor->address64.toString() << ", addr32:"
                    << Address::toString(neighborAddress32));

        ++nrOfNeighbors;
    }

    if (nrOfNeighbors > 0) {
        long savedCrtPos = stream.ostream.tellp();

        // update nrOfNeighbors
        stream.ostream.seekp(savedNrOfNeighborsPos);
        stream.write(nrOfNeighbors);

        // seek to the saved current position
        stream.ostream.seekp(savedCrtPos);
    }

    // 2. add graphs
    for (GraphIndexedAttribute::iterator itGraphs = device.phyAttributes.graphsTable.begin(); itGraphs
                != device.phyAttributes.graphsTable.end(); ++itGraphs) {

        PhyGraph* graph = itGraphs->second.getValue();
        if (graph == NULL) {
            continue;
        }

        stream.write(graph->index);
        LOG_DEBUG("[REPORT]      add graph " << (int) graph->index);

        long savedNrOfGraphMembersPos = stream.ostream.tellp();
        Uint16 nrOfGraphMembers = 0;
        stream.write(nrOfGraphMembers);

        for (std::vector<Uint16>::iterator itNodes = graph->neighbors.begin(); itNodes != graph->neighbors.end(); ++itNodes) {
            Address32 neighborAddress32 = NE::Common::Address::createAddress32(device.capabilities.dllSubnetId, *itNodes);
            Device *neighborDevice = subnetsContainer->getDevice(neighborAddress32);

            if (!neighborDevice || !neighborDevice->isEligibleForReport()) {
                continue;
            }
            neighborDevice->address128.marshall(stream);
            ++nrOfGraphMembers;
            LOG_DEBUG("[REPORT]          graph member: " << NE::Common::Address::toString(neighborAddress32));
        }

        if (nrOfGraphMembers > 0) {
            long savedCrtPos = stream.ostream.tellp();

             // update nrOfGraphMembers
             stream.ostream.seekp(savedNrOfGraphMembersPos);
             stream.write(nrOfGraphMembers);

             // seek to the saved current position
             stream.ostream.seekp(savedCrtPos);
        }
        ++nrOfGraphs;
    }

    if (nrOfGraphs > 0) {
        long savedCrtPos = stream.ostream.tellp();

        // update nrOfGraphs
        stream.ostream.seekp(savedNrOfGraphsPos);
        stream.write(nrOfGraphs);

        // seek to the saved current position
        stream.ostream.seekp(savedCrtPos);
    }

    ++nrOfDevices;

    // 3. add backbone info if this device is backbone
    if (device.capabilities.isBackbone()) {

        device.address128.marshall(backboneStream);
        backboneStream.write(device.capabilities.dllSubnetId);

        ++nrOfBackbones;
    }
}

//manager is handled separately because its neighborsTable is not populated
//the following neighbors will be added: GW, BBRs
void ReportsEngine::getTopologyEntryForManager(TopologyReport& topologyReport) {
    Topology::Device topologyDevice;
    topologyDevice.networkAddress = subnetsContainer->manager->address32;

    Topology::Device::Neighbor deviceNeighbor;
    deviceNeighbor.clockSource = 0;

    if (subnetsContainer->getGateway() != NULL) {
        //add GW
        deviceNeighbor.neighborAddress = subnetsContainer->getGateway()->address32;
        topologyDevice.neighborList.push_back(deviceNeighbor);
    }

    //add backbones
    SubnetsMap &subnetsMap = subnetsContainer->getSubnetsList();
    for (SubnetsMap::iterator it = subnetsMap.begin(); it != subnetsMap.end(); ++it) {
        Device *backbone = it->second->getBackbone();
        if (backbone != NULL) {
            deviceNeighbor.neighborAddress = backbone->address32;
            topologyDevice.neighborList.push_back(deviceNeighbor);
        }
    }

    topologyReport.deviceList.push_back(topologyDevice);
}

void ReportsEngine::getTopologyEntryForManager(NetworkOrderStream& stream, Uint16 & nrOfDevices) {

    subnetsContainer->manager->address128.marshall(stream);

    long savedNrOfNeighborsPos = stream.ostream.tellp();
    Uint16 nrOfNeighbors = 0;
    stream.write(nrOfNeighbors);

    Uint16 nrOfGraphs = 0;
    stream.write(nrOfGraphs);

    Uint8 clockSource = 0;
    Device * gwDevice = subnetsContainer->getGateway();
    if (gwDevice != NULL) {
        gwDevice->address128.marshall(stream);
        stream.write(clockSource);

        ++nrOfNeighbors;
    }

    SubnetsMap &subnetsMap = subnetsContainer->getSubnetsList();
    for (SubnetsMap::iterator it = subnetsMap.begin(); it != subnetsMap.end(); ++it) {
        Device *backbone = it->second->getBackbone();
        if (backbone != NULL) {
            backbone->address128.marshall(stream);
            stream.write(clockSource);

            ++nrOfNeighbors;
        }
    }

    long savedCrtPos = stream.ostream.tellp();

    // update nrOfNeighbors
    stream.ostream.seekp(savedNrOfNeighborsPos);
    stream.write(nrOfNeighbors);

    // seek to the saved current position
    stream.ostream.seekp(savedCrtPos);

    nrOfDevices++;
}

void ReportsEngine::createTopologyReport(TopologyReport& topologyReport) {
    getTopologyEntryForManager(topologyReport);
    Subnet::PTR gwSubnet = subnetsContainer->getSubnet(subnetsContainer->getGateway()->address32);
    getTopologyReportEntry(gwSubnet, *subnetsContainer->getGateway(), topologyReport);

    Address16 managerAddress16 = Address::getAddress16(subnetsContainer->manager->address32);
    Address16 gatewayAddress16 = Address::getAddress16(subnetsContainer->getGateway()->address32);

    SubnetsMap &subnetsMap = subnetsContainer->getSubnetsList();
    for (SubnetsMap::iterator it = subnetsMap.begin(); it != subnetsMap.end(); ++it) {

        const Address16Set& activeDevices = it->second->getActiveDevices();
        for (Address16Set::const_iterator itDevices = activeDevices.begin(); itDevices != activeDevices.end(); ++itDevices) {

            if (!(*itDevices == managerAddress16) && !(*itDevices == gatewayAddress16)) {

                Device *device = it->second->getDevice(*itDevices);
                if (!device || !device->isEligibleForReport()) {
                    continue;
                }
                getTopologyReportEntry(it->second, *device, topologyReport);
            }
        }
    }
}

void ReportsEngine::createTopologyReport(NetworkOrderStream& stream) {

    long savedNrOfDevicesPos = stream.ostream.tellp();
    Uint16 nrOfDevices = 0;
    stream.write(nrOfDevices);

    long savedNrOfBackbonesPos = stream.ostream.tellp();
    Uint16 nrOfBackbones = 0;
    stream.write(nrOfBackbones);

    getTopologyEntryForManager(stream, nrOfDevices);

    NetworkOrderStream backboneStream;
    Subnet::PTR gwSubnet = subnetsContainer->getSubnet(subnetsContainer->getGateway()->address32);
    getTopologyReportEntry(gwSubnet, *subnetsContainer->getGateway(), stream, backboneStream, nrOfDevices, nrOfBackbones);

    Address16 managerAddress16 = Address::getAddress16(subnetsContainer->manager->address32);
    Address16 gatewayAddress16 = Address::getAddress16(subnetsContainer->getGateway()->address32);

    SubnetsMap &subnetsMap = subnetsContainer->getSubnetsList();
    for (SubnetsMap::iterator it = subnetsMap.begin(); it != subnetsMap.end(); ++it) {

        const Address16Set& activeDevices = it->second->getActiveDevices();
        for (Address16Set::const_iterator itDevices = activeDevices.begin(); itDevices != activeDevices.end(); ++itDevices) {

            if (!(*itDevices == managerAddress16) && !(*itDevices == gatewayAddress16)) {

                Device *device = it->second->getDevice(*itDevices);
                if (!device || !device->isEligibleForReport()) {
                    continue;
                }
                getTopologyReportEntry(it->second, *device, stream, backboneStream, nrOfDevices, nrOfBackbones);
            }
        }
    }

    if (backboneStream.ostream.str().size() > 0) {
        stream.write(backboneStream.ostream.str());
    }

    if (nrOfDevices > 0) {
        long savedCrtPos = stream.ostream.tellp();

        // update nrOfDevices
        stream.ostream.seekp(savedNrOfDevicesPos);
        stream.write(nrOfDevices);

        // seek to the saved current position
        stream.ostream.seekp(savedCrtPos);
    }

    if (nrOfBackbones > 0) {
        long savedCrtPos = stream.ostream.tellp();

        // update nrOfBackbones
        stream.ostream.seekp(savedNrOfBackbonesPos);
        stream.write(nrOfBackbones);

        // seek to the saved current position
        stream.ostream.seekp(savedCrtPos);
    }
}


void ReportsEngine::getDeviceSchedule(Address16 deviceAddress, Subnet::PTR subnet, ScheduleReport& scheduleReport) {

    Device* device = subnet->getDevice(deviceAddress);
    RETURN_ON_NULL(device);

    LOG_INFO("[REPORT] Schedule for " << Address::toString(device->address32) << " status=" << (int) device->status
                << " isEligible=" << (int) device->isEligibleForReport());

    if (!device->isEligibleForReport()) {
        LOG_INFO("Device " << Address::toString(device->address32) << " not EligibleForReport");
        return;
    }

    const SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    SuperframeListT sfList;
    for (std::map<EntityIndex, SuperframeAttribute>::iterator it = device->phyAttributes.superframesTable.begin(); it
                != device->phyAttributes.superframesTable.end(); ++it) {

        PhySuperframe *superframe = it->second.getValue();
        if (!superframe) {
            continue;
        }

        LinkListT linkList;

        for (std::map<EntityIndex, LinkAttribute>::iterator itLinks = device->phyAttributes.linksTable.begin(); itLinks
                    != device->phyAttributes.linksTable.end(); ++itLinks) {

            PhyLink *link = itLinks->second.getValue();
            if (!link) {
                continue;
            }

            if (link->superframeIndex == superframe->index) {
                Address32 peerAddress = 0xFFFFFFFF; //set a device address only for transmit links with neighborType=1
                Uint8 direction = 0;
                //if transmission

                if (link->type & Tdma::LinkType::TRANSMIT) {
                    direction = 1;
                    if (link->neighborType == 1) {
                        Device *neighborLink = subnet->getDevice(link->neighbor);
                        if (!neighborLink) {
                            LOG_ERROR("neighbor device not found for link " << (int) link->index << " (neigh="
                                        << (int) link->neighbor << " decimal)");
                            continue;
                        }
                        peerAddress = neighborLink->address32;
                    }
                }
                // TODO calculate channel number of the link??
                // TODO determinate  the link type (periodic/aperiodic)
                Uint8 channelNumber = link->chOffset;

                Uint8 gsLinkType = 3; //periodic management
                if (link->role == Tdma::TdmaLinkTypes::APPLICATION) {
                    gsLinkType = 2; //periodic data
                }
                Link newLink(peerAddress, link->schedule.offset, link->schedule.interval, subnetSettings.getTimeslotLength(), channelNumber, direction, gsLinkType);
                linkList.push_back(newLink);

                LOG_DEBUG("[REPORT]      added link...sf=" << (int) superframe->index << " link=" << (int) link->index
                            << " link_sf=" << (int) link->superframeIndex);
            }
        }

        Int32 startTime = superframe->sfBirth; //start time of the superframe; offset relative to the beginning of TAI time
        //-1; //set to -1 to indicate that the superframe has no known synchronization

        Superframe sf(superframe->index, superframe->sfPeriod, startTime, linkList);
        sfList.push_back(sf);
    }

    DeviceSchedule deviceschedule(device->address32, sfList);
    scheduleReport.deviceScheduleList.push_back(deviceschedule);
    LOG_INFO("ScheduleReport deviceScheduleList add device " << Address::toString(device->address32));
}

void ReportsEngine::getDeviceSchedule(Subnet::PTR subnet, Device* device, NetworkOrderStream & stream, Uint16 & nrOfSchedules) {

    RETURN_ON_NULL(device);

    LOG_DEBUG("[REPORT] Schedule for " << Address::toString(device->address32) << " status=" << (int) device->status
                << " isEligible=" << (int) device->isEligibleForReport());

    if (!device->isEligibleForReport()) {
        LOG_DEBUG("Device " << Address::toString(device->address32) << " not EligibleForReport");
        return;
    }

    ++nrOfSchedules;

    // 1. address128
    device->address128.marshall(stream);

    // 2. nrOfSuperframes
    long savedNrOfSuperframesPos = stream.ostream.tellp();
    Uint16 nrOfSuperframes = 0;
    stream.write(nrOfSuperframes);

    for (std::map<EntityIndex, SuperframeAttribute>::iterator it = device->phyAttributes.superframesTable.begin(); it
                != device->phyAttributes.superframesTable.end(); ++it) {

        PhySuperframe *superframe = it->second.getValue();
        if (!superframe) {
            continue;
        }

        // 3. superframe info
        stream.write(superframe->index);
        stream.write(superframe->sfPeriod);

        Int32 startTime = 0; //start time of the superframe; offset relative to the beginning of TAI time
        //-1; //set to -1 to indicate that the superframe has no known synchronization
        stream.write(startTime);

        // 4. nrOfLinks
        long savedNrOfLinksPos = stream.ostream.tellp();
        Uint16 nrOfLinks = 0;
        stream.write(nrOfLinks);

        for (std::map<EntityIndex, LinkAttribute>::iterator itLinks = device->phyAttributes.linksTable.begin(); itLinks
                    != device->phyAttributes.linksTable.end(); ++itLinks) {

            PhyLink *link = itLinks->second.getValue();
            if (!link) {
                continue;
            }
            if (link->superframeIndex != superframe->index) {
                continue;
            }

            //Address32 peerAddress = BROADCAST_ADDRESS_32; //set a device address only for transmit links with neighborType=1
            Address128 peerAddress128;
            peerAddress128.loadString(BROADCAST_ADDRESS_128);
            Uint8 direction = 0;
            //if transmission

            if (link->type & Tdma::LinkType::TRANSMIT) {
                direction = 1;
                if (subnet && link->neighborType == 1) {
                    Device *neighborLink = subnet->getDevice(link->neighbor);
                    if (!neighborLink) {
                        LOG_ERROR("neighbor device not found for link " << (int) link->index << " (neigh="
                                    << (int) link->neighbor << " decimal)");
                        continue;
                    }
                    //peerAddress = neighborLink->address32;
                    peerAddress128 = neighborLink->address128;
                }
            }
            // TODO calculate channel number of the link??
            // TODO determinate  the link type (periodic/aperiodic)
            Uint8 channelNumber = link->chOffset;

            Uint8 gsLinkType = 3; //periodic management
            if (link->role == Tdma::TdmaLinkTypes::APPLICATION) {
                gsLinkType = 2; //periodic data
            }

            // 5. Link
            peerAddress128.marshall(stream);
            stream.write(link->schedule.offset);
            stream.write(link->schedule.interval);
            stream.write((Uint16)superframe->tsDur);
            stream.write(channelNumber);
            stream.write(direction);
            stream.write(gsLinkType);

            ++nrOfLinks;

            LOG_DEBUG("[REPORT]      added link...sf=" << (int) superframe->index << " link=" << (int) link->index
                        << " link_sf=" << (int) link->superframeIndex);
        }

        if (nrOfLinks > 0) {
            long savedCrtPos = stream.ostream.tellp();

            // 6. update nrOfLinks
            stream.ostream.seekp(savedNrOfLinksPos);
            stream.write(nrOfLinks);

            // seek to the saved current position
            stream.ostream.seekp(savedCrtPos);
        }

        ++nrOfSuperframes;
    }

    if (nrOfSuperframes > 0) {
        long savedCrtPos = stream.ostream.tellp();

        // 7. update nrOfSuperframes
        stream.ostream.seekp(savedNrOfSuperframesPos);
        stream.write(nrOfSuperframes);

        // seek to the saved current position
        stream.ostream.seekp(savedCrtPos);
    }

    LOG_INFO("[REPORT] added schedule for " << Address::toString(device->address32));
}

void ReportsEngine::createScheduleReport(ScheduleReport& scheduleReport) {
    bool enableChannel26 = false;
    //TODO update enableChannnel (read from config) if it is to be used

    //get schedule for manager
    Address32 managerAddress32 = subnetsContainer->getManagerAddress32();
    Device* manager = subnetsContainer->getDevice(managerAddress32);
    if (!manager) {
        std::ostringstream errStream;
        errStream << "Manager not found.";
        LOG_ERROR(errStream.str());
        throw NEException(errStream.str());
    }
    SuperframeListT sfList;
    DeviceSchedule deviceschedule(manager->address32, sfList); //manager has no superframes
    scheduleReport.deviceScheduleList.push_back(deviceschedule);
    LOG_INFO("ScheduleReport deviceScheduleList add manager " << Address::toString(manager->address32));

    //get schedule for gateway
    Device* gateway = subnetsContainer->getGateway();
    if (gateway && gateway->isEligibleForReport()) {
        SuperframeListT sfList;
        DeviceSchedule deviceschedule(gateway->address32, sfList); //gw has no superframes
        scheduleReport.deviceScheduleList.push_back(deviceschedule);
        LOG_INFO("ScheduleReport deviceScheduleList add gw " << Address::toString(gateway->address32));
    } else {
        LOG_INFO("gateway not EligibleForReport");
    }

    //get schedule for the rest of devices
    SubnetsMap &subnetsMap = subnetsContainer->getSubnetsList();
    std::vector<Uint8> blacklistedChannels;

    for (SubnetsMap::iterator it = subnetsMap.begin(); it != subnetsMap.end(); ++it) {

        const Address16Set& activeDevices = it->second->getActiveDevices();
        for (Address16Set::const_iterator itDevices = activeDevices.begin(); itDevices != activeDevices.end(); ++itDevices) {

            if (!(*itDevices == it->second->getAddress16(subnetsContainer->manager->address32)) && !(*itDevices
                        == it->second->getAddress16(subnetsContainer->getGateway()->address32))) {

                getDeviceSchedule(*itDevices, it->second, scheduleReport);
                std::vector<Uint8> blackListChannels;
                it->second->ChannelBlacklist().getBlacklistedChannels(blackListChannels);
                for(std::vector<Uint8>::iterator itBlack = blackListChannels.begin(); itBlack !=  blackListChannels.end(); ++itBlack) {
                    std::vector<Uint8>::iterator itExists = std::find(blacklistedChannels.begin(), blacklistedChannels.end(), *itBlack);
                    if (itExists == blacklistedChannels.end()) {
                        blacklistedChannels.push_back(*itBlack);
                    }
                }
            }
        }
    }

    //add channels
    ChannelListT channelList;
    for (int i = 0; i <= 15; ++i) {
        if ((i < 15) || (i == 15 && enableChannel26)) { //treat ch26 separately
            //determine the channel status
            Uint8 status = 1;
            std::vector<Uint8>::iterator it = std::find(blacklistedChannels.begin(), blacklistedChannels.end(), i);
            if (it != blacklistedChannels.end()) {
                status = 0;
            }
            Channel ch(CHANNEL_LIST[i], status);
            channelList.push_back(ch);
        } else {
            Channel ch(CHANNEL_LIST[i], 0);
            channelList.push_back(ch);
        }
    }
    scheduleReport.channelList = channelList;
}

void ReportsEngine::createScheduleReport(NetworkOrderStream & stream) {

    // 1. nrOfChannels
    long savedNrOfChannelsPos = stream.ostream.tellp();
    Uint8 nrOfChannels = 0;
    stream.write(nrOfChannels);

    // 2. nrOfSchedules
    long savedNrOfSchedulesPos = stream.ostream.tellp();
    Uint16 nrOfSchedules = 0;
    stream.write(nrOfSchedules);

    //get schedule for the rest of devices
    SubnetsMap &subnetsMap = subnetsContainer->getSubnetsList();
    std::vector<Uint8> blacklistedChannels;
    std::vector<Uint8> configChannels;

    for (SubnetsMap::iterator it = subnetsMap.begin(); it != subnetsMap.end(); ++it) {

        SubnetSettings & subnetSettings = it->second->getSubnetSettings();
        for(std::vector<Uint8>::iterator itSubnetCh = subnetSettings.channel_list.begin();
                itSubnetCh !=  subnetSettings.channel_list.end(); ++itSubnetCh) {
            std::vector<Uint8>::iterator itExists = std::find(configChannels.begin(), configChannels.end(), *itSubnetCh);
            if (itExists == configChannels.end()) {
                configChannels.push_back(*itSubnetCh);
            }
        }

        std::vector<Uint8> blackListChannels;
        it->second->ChannelBlacklist().getBlacklistedChannels(blackListChannels);
        for(std::vector<Uint8>::iterator itBlack = blackListChannels.begin(); itBlack !=  blackListChannels.end(); ++itBlack) {
            std::vector<Uint8>::iterator itExists = std::find(blacklistedChannels.begin(), blacklistedChannels.end(), *itBlack);
            if (itExists == blacklistedChannels.end()) {
                blacklistedChannels.push_back(*itBlack);
            }
        }
    }

    bool enableChannel26 = false;
    //TODO update enableChannnel (read from config) if it is to be used

    // 3. channels
    for (int i = 0; i <= 15; ++i) {
        Uint8 channelStatus = 0;
        Uint8 channelNumber = CHANNEL_LIST[i];
        if ((i < 15) || (i == 15 && enableChannel26)) { //treat ch26 separately
            //determine the channel status
            channelStatus = 1;

            std::vector<Uint8>::iterator it = std::find(blacklistedChannels.begin(), blacklistedChannels.end(), i);
            if (it != blacklistedChannels.end()) {
                channelStatus = 0;
            } else {
                it = std::find(configChannels.begin(), configChannels.end(), i);
                if (it == configChannels.end()) {
                    channelStatus = 0;
                }
            }
        } else {
            channelStatus = 0;
        }
        stream.write(channelNumber);
        stream.write(channelStatus);
        ++nrOfChannels;
    }

    // 4. update nrOfChannels
    if (nrOfChannels > 0) {
        long savedCrtPos = stream.ostream.tellp();

        // update nrOfChannels
        stream.ostream.seekp(savedNrOfChannelsPos);
        stream.write(nrOfChannels);

        // seek to the saved current position
        stream.ostream.seekp(savedCrtPos);
    }

    //get schedule for manager
    getDeviceSchedule(Subnet::PTR(), subnetsContainer->manager, stream, nrOfSchedules);

    //get schedule for gateway
    getDeviceSchedule(Subnet::PTR(), subnetsContainer->getGateway(), stream, nrOfSchedules);

    for (SubnetsMap::iterator it = subnetsMap.begin(); it != subnetsMap.end(); ++it) {
        const Address16Set& activeDevices = it->second->getActiveDevices();
        for (Address16Set::const_iterator itAddress16 = activeDevices.begin(); itAddress16 != activeDevices.end(); ++itAddress16) {
            Device* device = it->second->getDevice(*itAddress16);
            if ((*itAddress16 != ADDRESS16_MANAGER) && (*itAddress16 != ADDRESS16_GATEWAY)) {

                // 5. DeviceSchedule
                getDeviceSchedule(it->second, device, stream, nrOfSchedules);
            }
        }
    }

    // 6. update nrOfSchedules
    if (nrOfSchedules > 0) {
        long savedCrtPos = stream.ostream.tellp();

        // update nrOfSchedules
        stream.ostream.seekp(savedNrOfSchedulesPos);
        stream.write(nrOfSchedules);

        // seek to the saved current position
        stream.ostream.seekp(savedCrtPos);
    }
}

void ReportsEngine::getDeviceHealth(Address16 deviceAddress, Subnet::PTR subnet, DeviceHealthReport& deviceHealthReport) {
    Device* device = subnet->getDevice(deviceAddress);
    RETURN_ON_NULL(device);

    if (device->isEligibleForReport()) {
        LOG_DEBUG("[REPORT] getDeviceHealth for " << Address::toString(device->address32));

        DeviceHealth deviceHealth(device->address32, device->deviceStatistics.DPDUsTransmitted,
                    device->deviceStatistics.DPDUsReceived, device->deviceStatistics.DPDUsFailedTransmission,
                    device->deviceStatistics.DPDUsFailedReception);

        deviceHealthReport.push_back(deviceHealth);
    }
}

void ReportsEngine::getDeviceHealth(Device * device, NetworkOrderStream& stream, Uint16 & nrOfHealth) {
    //RETURN_ON_NULL(device);

    if (device->isEligibleForReport()) {
        LOG_DEBUG("[REPORT] getDeviceHealth for " << Address::toString(device->address32));

        device->address128.marshall(stream);

        stream.write(device->deviceStatistics.DPDUsTransmitted);
        stream.write(device->deviceStatistics.DPDUsReceived);
        stream.write(device->deviceStatistics.DPDUsFailedTransmission);
        stream.write(device->deviceStatistics.DPDUsFailedReception);

        if (device->deviceStatistics.DPDUsTransmitted == MAX_32BITS_VALUE
                    || device->deviceStatistics.DPDUsFailedTransmission == MAX_32BITS_VALUE) {

            device->deviceStatistics.DPDUsTransmitted = 0;
            device->deviceStatistics.DPDUsFailedTransmission = 0;
        }

        if (device->deviceStatistics.DPDUsReceived == MAX_32BITS_VALUE
                    || device->deviceStatistics.DPDUsFailedReception == MAX_32BITS_VALUE) {

            device->deviceStatistics.DPDUsReceived = 0;
            device->deviceStatistics.DPDUsFailedReception = 0;
        }

        ++nrOfHealth;
    }
}

void ReportsEngine::createDeviceHealthReport(DeviceHealthReport& deviceHealthReport) {

    //get info for the rest of devices
    SubnetsMap &subnetsMap = subnetsContainer->getSubnetsList();

    for (SubnetsMap::iterator it = subnetsMap.begin(); it != subnetsMap.end(); ++it) {
        const Address16Set& activeDevices = it->second->getActiveDevices();
        for (Address16Set::const_iterator itDevices = activeDevices.begin(); itDevices != activeDevices.end(); ++itDevices) {

            if (!(*itDevices == it->second->getAddress16(subnetsContainer->manager->address32)) && !(*itDevices
                        == it->second->getAddress16(subnetsContainer->getGateway()->address32))) {

                getDeviceHealth(*itDevices, it->second, deviceHealthReport);
            }
        }
    }
}

void ReportsEngine::createDeviceHealthReport(NetworkOrderStream & stream) {

    long savedNrOfHealthPos = stream.ostream.tellp();
    Uint16 nrOfHealth = 0;
    stream.write(nrOfHealth);

    //get info for the rest of devices
    SubnetsMap &subnetsMap = subnetsContainer->getSubnetsList();

    for (SubnetsMap::iterator it = subnetsMap.begin(); it != subnetsMap.end(); ++it) {
        const Address16Set& activeDevices = it->second->getActiveDevices();
        for (Address16Set::const_iterator itAddress16 = activeDevices.begin(); itAddress16 != activeDevices.end(); ++itAddress16) {
            Device * device = it->second->getDevice(*itAddress16);

            if (device && (*itAddress16 != ADDRESS16_MANAGER) && (*itAddress16 != ADDRESS16_GATEWAY)) {

                getDeviceHealth(device, stream, nrOfHealth);
            }
        }
    }

    if (nrOfHealth > 0) {
        long savedCrtPos = stream.ostream.tellp();

        // update nrOfHealth
        stream.ostream.seekp(savedNrOfHealthPos);
        stream.write(nrOfHealth);

        // seek to the saved current position
        stream.ostream.seekp(savedCrtPos);
    }
}

void ReportsEngine::createNeighborHealthReport(NeighborHealthReport& neighborHealthReport, const Address32& neighborAddress) {
    Subnet::PTR subnet = subnetsContainer->getSubnet(neighborAddress);
    RETURN_ON_NULL_MSG(subnet, "subnet not found for address " << Address_toStream(neighborAddress));

    std::list<EdgePointer> edges;
    Address16 deviceAddress16 = subnet->getAddress16(neighborAddress);
    subnet->getOutBoundEdges(deviceAddress16, edges);

    for (std::list<EdgePointer>::iterator it = edges.begin(); it != edges.end(); ++it) {
        Uint8 linkstatus = 0; //inactive

		//        RETURN_ON_NULL(*it);
        if (*it == NULL){
            continue;
        }
        if ((*it)->getEdgeStatus() == Status::ACTIVE) {
            linkstatus = 1;
        }

        Address32 address = Address::createAddress32(subnet->getSubnetId(), (*it)->getDestination());
        if (address == neighborAddress) {
            address = Address::createAddress32(subnet->getSubnetId(), (*it)->getSource());
        }

        Int16 signalStrength = (Int16) (*it)->getRSSI() - 64; //signalStrength is reported as dBm, not as 0-100 range specified by standard

        Uint32 transmitted = (*it)->getSent();
        if (transmitted == 0xFFFFFFFF) { //max value
            (*it)->resetSent();
            (*it)->resetFailedSent();
        }

        Uint32 failedTransmit = (*it)->getFailedSent();
        if (failedTransmit == 0xFFFFFFFF) { //max value
            (*it)->resetSent();
            (*it)->resetFailedSent();
        }

        Uint32 received = (*it)->getReceived();
        if (received == 0xFFFFFFFF) { //max value
            (*it)->resetReceived();
            //(*it)->resetFailedReceived();
        }

        // TODO failed receive
        NeighborHealth neighborHealth(address, linkstatus, transmitted, received,
                    failedTransmit, (Uint32) 0, signalStrength, (*it)->getRSQI());

        neighborHealthReport.push_back(neighborHealth);

        LOG_DEBUG("[REPORT] added neighborHealth entry with " << Address::toString(address) << " signalStrength="
                    << signalStrength);
    }
}

bool ReportsEngine::createNeighborHealthReport(const Address32& neighborAddress, NetworkOrderStream& stream) {
    Subnet::PTR subnet = subnetsContainer->getSubnet(neighborAddress);
    if (!subnet) {
        LOG_ERROR("subnet not found for address " << Address_toStream(neighborAddress));
        return false;
    }

    NE::Model::Device * reportDevice = subnet->getDevice(neighborAddress);
    if (!reportDevice || !reportDevice->isEligibleForReport()) {
        return false;
    }

    std::string infoString; //for LOG_INFO details of report

    long savedNrOfNeighborHealthPos = stream.ostream.tellp();
    Uint16 nrOfNeighborHealth = 0;
    stream.write(nrOfNeighborHealth);

    std::list<EdgePointer> edges;
    Address16 deviceAddress16 = subnet->getAddress16(neighborAddress);
    subnet->getOutBoundEdges(deviceAddress16, edges);

    for (std::list<EdgePointer>::iterator it = edges.begin(); it != edges.end(); ++it) {
        Uint8 linkStatus = 0; //inactive

        if (*it == NULL){
            continue;
        }
        if ((*it)->getEdgeStatus() == Status::ACTIVE) {
            linkStatus = 1;
        }

        Address32 address = Address::createAddress32(subnet->getSubnetId(), (*it)->getDestination());
        if (address == neighborAddress) {
            address = Address::createAddress32(subnet->getSubnetId(), (*it)->getSource());
        }

        Int16 signalStrength = (Int16) (*it)->getRSSI() - 64; //signalStrength is reported as dBm, not as 0-100 range specified by standard

        Uint32 transmitted = (*it)->getSent();
        Uint32 failedTransmit = (*it)->getFailedSent();

        if (transmitted == MAX_32BITS_VALUE
                    || failedTransmit == MAX_32BITS_VALUE) { //max value
            (*it)->resetSent();
            (*it)->resetFailedSent();
        }

        Uint32 received = (*it)->getReceived();
        if (received == MAX_32BITS_VALUE) { //max value
            (*it)->resetReceived();
            //(*it)->resetFailedReceived();
        }

        // TODO failed receive

        NE::Model::Device * device = subnet->getDevice(address);
        if (!device) {
            continue;
        }

        device->address128.marshall(stream);

        stream.write(linkStatus);
        stream.write(transmitted);
        stream.write(received);
        stream.write(failedTransmit);
        stream.write((Uint32) 0);
        stream.write(signalStrength);
        stream.write((Uint8)(*it)->getRSQI());

        ++nrOfNeighborHealth;

        LOG_DEBUG("[REPORT] added neighborHealth entry with " << Address::toString(address) << " signalStrength="
                    << signalStrength);

        infoString += Address::toString(address) + " ";
    }

    if (nrOfNeighborHealth > 0) {
        long savedCrtPos = stream.ostream.tellp();

        // update nrOfNeighborHealth
        stream.ostream.seekp(savedNrOfNeighborHealthPos);
        stream.write(nrOfNeighborHealth);

        // seek to the saved current position
        stream.ostream.seekp(savedCrtPos);
    }

    LOG_INFO("added neighbors: " << infoString);

    return true;
}

void ReportsEngine::createNetworkHealthReport(NetworkHealthReport& networkHealthReport) {
    SubnetsMap &subnetsMap = subnetsContainer->getSubnetsList();

    Uint32 networkID = settingsLogic->networkID;
    Uint8 networkType = 0; // TODO the networkType is set to 0 for the moment
    Uint16 deviceCount = 0; //OBSERVATION: manager and gateway are not counted; TODO add them if needed
    Uint32 startDateCurrent = startTime; //TAINetworkTimeValue.currentTAI
    Uint16 startDateFractional = 0; //TAINetworkTimeValue.fractionalTAI
    Uint32 currentDateCurrent = NE::Common::ClockSource::getCurrentTime(); //TAINetworkTimeValue.currentTAI
    Uint16 currentDateFractional = 0; //TAINetworkTimeValue.fractionalTAI
//    Uint32 DPDUsSent = 0; // GS_DPDUs_Sent
//    Uint32 DPDUsLost = 0; // GS_DPDUs_Lost
    Uint8 GPDULatency = 0; // GS_GPDU_Latency
    Uint8 GPDUPathReliability = 0; // GS_GPDU_Path_Reliability
    Uint8 GPDUDataReliability = 0; // GS_GPDU_Data_Reliability
    Uint32 joinCountNetwork = globalJoinCounter; // GS_Join_Count
    //Uint16 totalDataReliability = 0;

    for (SubnetsMap::iterator itSubnets = subnetsMap.begin(); itSubnets != subnetsMap.end(); ++itSubnets) {

        deviceCount += itSubnets->second->getNumbetOfDevicesFromSubnet();

        const Address16Set& activeDevices = itSubnets->second->getActiveDevices();

        for (Address16Set::const_iterator it = activeDevices.begin(); it != activeDevices.end(); ++it) {
            Device* device = itSubnets->second->getDevice(*it);
            if (device == NULL){
                continue;
            }
            if (!device->isEligibleForReport() || device->capabilities.isManager() || device->capabilities.isGateway()) {
                --deviceCount;
                continue;
            }

            LOG_DEBUG("[REPORT] getNetworkHealth for " << Address::toString(device->address32));

            Uint32 currentTime = NE::Common::ClockSource::getCurrentTime();
            if (((MAX_32BITS_VALUE - DPDUsSent) <= device->getTransmissionsSinceLastReport())
                        || ((MAX_32BITS_VALUE - DPDUsLost) <= device->getFailedTransmissionsSinceLastReport())) {
                DPDUsSent = 0;
                DPDUsLost = 0;
            }
            DPDUsSent += device->getTransmissionsSinceLastReport();
            DPDUsLost += device->getFailedTransmissionsSinceLastReport();
            device->resetStatisticsSinceLastReport();


            Uint32 joinCount = device->fullJoinsCount;

            //TODO fractional time set to 0 for the moment
            //NOTE GPDU data must be completed by gateway
            DeviceNetworkHealth deviceNetworkHealth(device->address32, device->startTime, 0, currentTime, 0, //
                        device->deviceStatistics.DPDUsTransmitted, device->deviceStatistics.DPDUsFailedTransmission, //
                        0, 0, //GPDULatency, GPDUPathReliability
                        0, //dataReliability, //GPDUDataReliability
                        joinCount);
            networkHealthReport.deviceHealthList.push_back(deviceNetworkHealth);

        }

    }


    NetworkHealth networkHealth(networkID, networkType, deviceCount, startDateCurrent, startDateFractional, currentDateCurrent,
                currentDateFractional, DPDUsSent, DPDUsLost, GPDULatency, GPDUPathReliability, GPDUDataReliability,
                joinCountNetwork);
    networkHealthReport.networkHealth = networkHealth;
}

void ReportsEngine::createNetworkHealthReport(NetworkOrderStream & stream) {
    SubnetsMap &subnetsMap = subnetsContainer->getSubnetsList();

    // 1. nrOfHealths
    long savedNrOfHealthsPos = stream.ostream.tellp();
    Uint16 nrOfHealths = 0;
    stream.write(nrOfHealths);

    Uint32 networkID = settingsLogic->networkID;
    Uint8 networkType = 0; // TODO the networkType is set to 0 for the moment
    Uint16 deviceCount = 0; //OBSERVATION: manager and gateway are not counted; TODO add them if needed
    Uint32 startDateCurrent = startTime; //TAINetworkTimeValue.currentTAI
    Uint16 startDateFractional = 0; //TAINetworkTimeValue.fractionalTAI
    Uint32 currentDateCurrent = NE::Common::ClockSource::getCurrentTime(); //TAINetworkTimeValue.currentTAI
    Uint16 currentDateFractional = 0; //TAINetworkTimeValue.fractionalTAI
    Uint8 GPDULatency = 0; // GS_GPDU_Latency
    Uint8 GPDUPathReliability = 0; // GS_GPDU_Path_Reliability
    Uint8 GPDUDataReliability = 0; // GS_GPDU_Data_Reliability
    Uint32 joinCountNetwork = globalJoinCounter; // GS_Join_Count
    // Uint16 totalDataReliability = 0;

    // networkHealth
    stream.write(networkID);
    stream.write(networkType);
    stream.write(deviceCount);
    stream.write(startDateCurrent);
    stream.write(startDateFractional);
    stream.write(currentDateCurrent);
    stream.write(currentDateFractional);
    stream.write(DPDUsSent);
    stream.write(DPDUsLost);
    stream.write(GPDULatency);
    stream.write(GPDUPathReliability);
    stream.write(GPDUDataReliability);
    stream.write(joinCountNetwork);

    Uint32 currentTime = NE::Common::ClockSource::getCurrentTime();

    for (SubnetsMap::iterator itSubnets = subnetsMap.begin(); itSubnets != subnetsMap.end(); ++itSubnets) {


        deviceCount += itSubnets->second->getNumbetOfDevicesFromSubnet();

        const Address16Set& activeDevices = itSubnets->second->getActiveDevices();

        for (Address16Set::const_iterator it = activeDevices.begin(); it != activeDevices.end(); ++it) {
            Device* device = itSubnets->second->getDevice(*it);
            if (device == NULL){
                continue;
            }
            if (!device->isEligibleForReport() || device->capabilities.isManager() || device->capabilities.isGateway()) {
                --deviceCount;
                continue;
            }

            LOG_DEBUG("[REPORT] getNetworkHealth for " << Address::toString(device->address32));

            if (((MAX_32BITS_VALUE - DPDUsSent) <= device->getTransmissionsSinceLastReport())
                        || ((MAX_32BITS_VALUE - DPDUsLost) <= device->getFailedTransmissionsSinceLastReport())) {
                DPDUsSent = 0;
                DPDUsLost = 0;
            }
            DPDUsSent += device->getTransmissionsSinceLastReport();
            DPDUsLost += device->getFailedTransmissionsSinceLastReport();
            device->resetStatisticsSinceLastReport();

            // 2. DeviceNetworkHealth
            device->address128.marshall(stream);
            stream.write(device->startTime);
            stream.write((Uint16)0);
            stream.write(currentTime);
            stream.write((Uint16)0);
            stream.write(device->deviceStatistics.DPDUsTransmitted);
            stream.write(device->deviceStatistics.DPDUsFailedTransmission);
            stream.write((Uint8)0);
            stream.write((Uint8)0);
            stream.write((Uint8)0); //dataReliability
            //stream.write(dataReliability);
            stream.write((Uint32)device->fullJoinsCount);

            if (device->deviceStatistics.DPDUsTransmitted == MAX_32BITS_VALUE
                        || device->deviceStatistics.DPDUsFailedTransmission == MAX_32BITS_VALUE) {

                device->deviceStatistics.DPDUsTransmitted = 0;
                device->deviceStatistics.DPDUsFailedTransmission = 0;
            }

            ++nrOfHealths;
        }
    }

    // 3. update nrOfHealths
    {
        long savedCrtPos = stream.ostream.tellp();

        // update nrOfHealths
        stream.ostream.seekp(savedNrOfHealthsPos);
        stream.write(nrOfHealths);

        // seek to the saved current position
        stream.ostream.seekp(savedCrtPos);
    }

    // 4. update networkHealth
    {
        long savedCrtPos = stream.ostream.tellp();

        stream.ostream.seekp(savedNrOfHealthsPos + sizeof(nrOfHealths));

        // update networkHealth
        stream.write(networkID);
        stream.write(networkType);
        stream.write(deviceCount);
        stream.write(startDateCurrent);
        stream.write(startDateFractional);
        stream.write(currentDateCurrent);
        stream.write(currentDateFractional);
        stream.write(DPDUsSent);
        stream.write(DPDUsLost);
        stream.write(GPDULatency);
        stream.write(GPDUPathReliability);
        stream.write(GPDUDataReliability);
        stream.write(joinCountNetwork);

        // seek to the saved current position
        stream.ostream.seekp(savedCrtPos);
    }

}

void ReportsEngine::createNetworkResourceReport(NetworkResourceReport& networkResourceReport) {

}


void ReportsEngine::createNetworkResourceReport(NetworkOrderStream & stream) {
    SubnetsMap &subnetsMap = subnetsContainer->getSubnetsList();

    long savedSubnetsCountPos = stream.ostream.tellp();
    Uint8 subnetsCount = 0;
    stream.write(subnetsCount);

    //LOG_INFO("[SORIN] slotLength=" << std::hex << slotLength << " (dec)" << std::dec << slotLength);

    for (SubnetsMap::iterator itSubnets = subnetsMap.begin(); itSubnets != subnetsMap.end(); ++itSubnets) {
        Device* backbone = itSubnets->second->getBackbone();
        if (!backbone) {
            continue;
        }

        //26fb (dec)9979
        Uint32 slotLength = ((unsigned long long) itSubnets->second->getSubnetSettings().getTimeslotLength() * 1000000) / (1 << 20); // reported in micro seconds --> * 2^-20 * 10^6


        float appSlots = 0.0;
        float manSlots = 0.0;

        itSubnets->second->getNumberOfSlotsPerSecondOnBBR(manSlots, appSlots);

        //LOG_INFO("[SORIN] appSlots=" << appSlots << " manSlots=" << manSlots);

        Uint32 appSlotsFraction = (Uint32) ((appSlots - (Uint32) appSlots) * 100);
        Uint32 manSlotsFraction = (Uint32) ((manSlots - (Uint32) manSlots) * 100);

        // SubnetResource
        backbone->address128.marshall(stream);
        stream.write(itSubnets->second->getSubnetId());

        stream.write(slotLength);

        //Occupied slots = GS_Slots_Linktype_0_X + GS_Slots_Linktype_0_Y / 100 + ... + GS_Slots_Linktype_3_X + GS_Slots_Linktype_3_Y / 100
        //X representing integer part of the value, Y representing fractional part of the value * 100
        stream.write((Uint32) (appSlots + manSlots));

        stream.write((Uint32)0); //AperiodicData_X = 0; // GS_Slots_Linktype_0_X
        stream.write((Uint32)0); //AperiodicData_Y = 0; // GS_Slots_Linktype_0_Y
        stream.write((Uint32)0); //AperiodicMgmt_X = 0; // GS_Slots_Linktype_1_X
        stream.write((Uint32)0); //AperiodicMgmt_Y = 0; // GS_Slots_Linktype_1_Y
        stream.write((Uint32) appSlots); //PeriodicData_X // GS_Slots_Linktype_2_X
        stream.write(appSlotsFraction); //PeriodicData_Y // GS_Slots_Linktype_2_Y
        stream.write((Uint32) manSlots); //PeriodicMgmt_X // GS_Slots_Linktype_3_X
        stream.write(manSlotsFraction); //PeriodicMgmt_Y // GS_Slots_Linktype_3_Y

        ++subnetsCount;
    }

    // update subnetsCount
    {
        long savedCrtPos = stream.ostream.tellp();

        // update nrOfNetworkResources
        stream.ostream.seekp(savedSubnetsCountPos);
        stream.write(subnetsCount);

        // seek to the saved current position
        stream.ostream.seekp(savedCrtPos);
    }
}


}
}
}
