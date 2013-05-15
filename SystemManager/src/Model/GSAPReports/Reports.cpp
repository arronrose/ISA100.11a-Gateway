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
 * Reports.cpp
 *
 *  Created on: Jun 11, 2009
 *      Author: flori.parauan, sorin.bidian
 */
#include "Reports.h"

#define BROADCAST_ADDRESS_32 0xFFFFFFFF
#define BROADCAST_ADDRESS_128 "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF"


LOG_DEF("I.M.GSAPReports.Reports");

using namespace NE::Model::Reports;

namespace Isa100 {
namespace Model {
namespace GSAPReports {

void marshall(NE::Misc::Marshall::OutputStream& stream, NE::Model::Reports::DeviceListReport& deviceListReport) {
    stream.write((Uint16) deviceListReport.size());

    for (NE::Model::Reports::DeviceListReport::iterator itDevices = deviceListReport.begin(); itDevices != deviceListReport.end(); ++itDevices) {

        NE::Model::Device * device = Model::EngineProvider::getEngine()->getDevice(itDevices->deviceAddress);
        if (!device) { //also checked for null when report was generated; normally should not be null here
            std::ostringstream errStream;
            errStream << "marshall deviceListReport - Device " << Address::toString(itDevices->deviceAddress) << " not found.";
            throw NEException(errStream.str());
        }

        //128 address
        device->address128.marshall(stream);
        stream.write(itDevices->deviceType);
        //64 address
        device->address64.marshall(stream);
        stream.write(itDevices->powerSupplyStatus);

        stream.write((Uint8) itDevices->joinStatus);

        Bytes manufacturer(itDevices->manufacturer.begin(), itDevices->manufacturer.end());
        Bytes model(itDevices->model.begin(), itDevices->model.end());
        Bytes revision(itDevices->revision.begin(), itDevices->revision.end());
        Bytes deviceTag(itDevices->deviceTag.begin(), itDevices->deviceTag.end());
        Bytes serialNo(itDevices->serialNo.begin(), itDevices->serialNo.end());
        stream.write((Uint8) manufacturer.size());
        stream.write(manufacturer);
        stream.write((Uint8) model.size());
        stream.write(model);
        stream.write((Uint8) revision.size());
        stream.write(revision);
        stream.write((Uint8) deviceTag.size());
        stream.write(deviceTag);
        stream.write((Uint8) serialNo.size());
        stream.write(serialNo);
    }
}

void marshall(NE::Misc::Marshall::OutputStream& stream, NE::Model::Reports::TopologyReport& topologyReport) {

    //numberOfDevices
    stream.write((Uint16) topologyReport.deviceList.size());
    //numberOfBackbones
    stream.write((Uint16) topologyReport.backboneList.size());

    //devices
    for (std::list<Topology::Device>::iterator itDevices = topologyReport.deviceList.begin(); itDevices
                != topologyReport.deviceList.end(); ++itDevices) {

        NE::Model::Device * device = Model::EngineProvider::getEngine()->getDevice(itDevices->networkAddress);
        if (device == NULL) { //also checked for null when report was generated; normally should not be null here
            std::string errStream = "Device is NULL on topologyReport marshall. " + Address::toString(itDevices->networkAddress);
            throw NE::Common::NEException(errStream);
        }

        device->address128.marshall(stream);
        stream.write((Uint16) itDevices->neighborList.size());
        stream.write((Uint16) itDevices->graphList.size());

        for (std::list<Topology::Device::Neighbor>::iterator itNeighbors = itDevices->neighborList.begin(); itNeighbors
                    != itDevices->neighborList.end(); ++itNeighbors) {
            NE::Model::Device * neighbor = Model::EngineProvider::getEngine()->getDevice(itNeighbors->neighborAddress);
            if (neighbor == NULL) { //also checked for null when report was generated; normally should not be null here
                std::string errStream = "Neighbor is NULL on topologyReport marshall. " + Address::toString(
                            itNeighbors->neighborAddress);
                throw NE::Common::NEException(errStream);
            }

            neighbor->address128.marshall(stream);
            stream.write(itNeighbors->clockSource);
        }

        for (std::list<Topology::Device::Graph>::iterator itGraphs = itDevices->graphList.begin(); itGraphs
                    != itDevices->graphList.end(); ++itGraphs) {

            stream.write(itGraphs->graphIdentifier);
            stream.write((Uint16) itGraphs->graphMemberList.size());

            for (std::list<Address32>::iterator itMembers = itGraphs->graphMemberList.begin(); itMembers
                        != itGraphs->graphMemberList.end(); ++itMembers) {

                NE::Model::Device * member = Model::EngineProvider::getEngine()->getDevice(*itMembers);
                if (member == NULL) { //also checked for null when report was generated; normally should not be null here
                    std::string errStream = "GraphNeighbor is NULL on topologyReport marshall. " + Address::toString(*itMembers);
                    throw NE::Common::NEException(errStream);
                }

                member->address128.marshall(stream);
            }

        }
    }

    //backbones
    for (std::list<Topology::Backbone>::iterator itBackbones = topologyReport.backboneList.begin(); itBackbones
                != topologyReport.backboneList.end(); ++itBackbones) {

        NE::Model::Device * backbone = Model::EngineProvider::getEngine()->getDevice(itBackbones->backboneAddress);
        if (backbone == NULL) { //also checked for null when report was generated; normally should not be null here
            std::string errStream = "Backbone is NULL on topologyReport marshall. " + Address::toString(itBackbones->backboneAddress);
            throw NE::Common::NEException(errStream);
        }

        backbone->address128.marshall(stream);
        stream.write(itBackbones->subnetID);
    }
}

void marshall(NE::Misc::Marshall::OutputStream& stream, NE::Model::Reports::ScheduleReport& scheduleReport) {

    stream.write((Uint8) scheduleReport.channelList.size());
    stream.write((Uint16) scheduleReport.deviceScheduleList.size());
    LOG_INFO("ScheduleReport deviceScheduleList size=" << (int)scheduleReport.deviceScheduleList.size());

    for (NE::Model::Reports::ChannelListT::iterator itChannels = scheduleReport.channelList.begin(); itChannels
                != scheduleReport.channelList.end(); ++itChannels) {
        //channels
        stream.write(itChannels->channelNumber);
        stream.write(itChannels->channelStatus);
    }

    for (NE::Model::Reports::DeviceScheduleList::iterator itDevices = scheduleReport.deviceScheduleList.begin(); itDevices
                != scheduleReport.deviceScheduleList.end(); ++itDevices) {

        NE::Model::Device * device = Model::EngineProvider::getEngine()->getDevice(itDevices->deviceAddress);
        if (!device) {
            std::ostringstream errStream;
            errStream << "marshall ScheduleReport - Device " << Address::toString(itDevices->deviceAddress) << " not found.";
            throw NEException(errStream.str());
        }
        //address
        //NE::Common::Address128 networkAddress = device->address128;
        device->address128.marshall(stream);

        //superframes
        stream.write((Uint16) itDevices->superframeList.size());
        LOG_INFO("ScheduleReport device=" << Address::toString(device->address32) << ", superframeList size=" << (int)itDevices->superframeList.size());
        for (std::list<NE::Model::Reports::Superframe>::iterator itSf = itDevices->superframeList.begin(); itSf
                    != itDevices->superframeList.end(); ++itSf) {
            stream.write(itSf->superframeID);
            stream.write(itSf->numberOfTimeSlots);
            stream.write(itSf->startTime);
            stream.write((Uint16) itSf->linkList.size());

            //links
            for (std::list<NE::Model::Reports::Link>::iterator itLinks = itSf->linkList.begin(); itLinks != itSf->linkList.end(); ++itLinks) {
                //logical network address of a communication partner for the slot
                NE::Common::Address128 networkAddress;
                if (itLinks->deviceAddress == BROADCAST_ADDRESS_32) {
                    networkAddress.loadString(BROADCAST_ADDRESS_128);
                } else {
                    device = Model::EngineProvider::getEngine()->getDevice(itLinks->deviceAddress);
                    if (device == NULL) {
                        throw NE::Common::NEException("Device is NULL on scheduleReport.");
                    }
                    networkAddress = device->address128;
                }
                networkAddress.marshall(stream);
                stream.write(itLinks->slotIndex);
                stream.write(itLinks->linkPeriod);
                stream.write(itLinks->slotLength);
                stream.write(itLinks->channelNumber);
                stream.write(itLinks->direction);
                stream.write(itLinks->linkType);
            }
        }
    }
}

void marshall(NE::Misc::Marshall::OutputStream& stream, NE::Model::Reports::DeviceHealthReport& deviceHealthReport) {

    stream.write((Uint16) deviceHealthReport.size());

    for (NE::Model::Reports::DeviceHealthReport::iterator itDevices = deviceHealthReport.begin(); itDevices
                != deviceHealthReport.end(); ++itDevices) {

        NE::Model::Device * device = Model::EngineProvider::getEngine()->getDevice(itDevices->deviceAddress);
        if (!device) {
            std::ostringstream errStream;
            errStream << "marshall DeviceHealthReport - Device " << Address::toString(itDevices->deviceAddress) << " not found.";
            throw NEException(errStream.str());
        }

        device->address128.marshall(stream);

        stream.write(itDevices->DPDUsTransmitted);
        stream.write(itDevices->DPDUsReceived);
        stream.write(itDevices->DPDUsFailedTransmission);
        stream.write(itDevices->DPDUsFailedReception);
    }
}

void marshall(NE::Misc::Marshall::OutputStream& stream, NE::Model::Reports::NeighborHealthReport& neighborHealthReport) {

    stream.write((Uint16) neighborHealthReport.size());
    for (NE::Model::Reports::NeighborHealthReport::iterator it = neighborHealthReport.begin(); it != neighborHealthReport.end(); ++it) {

        NE::Model::Device * device = Model::EngineProvider::getEngine()->getDevice(it->networkAddress);
        if (!device) {
            std::ostringstream errStream;
            errStream << "marshall NeighborHealthReport - Device " << Address::toString(it->networkAddress) << " not found.";
            throw NEException(errStream.str());
        }

        device->address128.marshall(stream);

        stream.write(it->linkStatus);
        stream.write(it->DPDUsTransmitted);
        stream.write(it->DPDUsReceived);
        stream.write(it->DPDUsFailedTransmission);
        stream.write(it->DPDUsFailedReception);
        stream.write(it->signalStrength);
        stream.write(it->signalQuality);

    }

}

void marshall(NE::Misc::Marshall::OutputStream& stream, NE::Model::Reports::NetworkHealthReport& networkHealthReport) {
    stream.write((Uint16) networkHealthReport.deviceHealthList.size());

    //networkHealth
    stream.write(networkHealthReport.networkHealth.networkID);
    stream.write(networkHealthReport.networkHealth.networkType);
    stream.write(networkHealthReport.networkHealth.deviceCount);
    stream.write(networkHealthReport.networkHealth.startDateCurrent);
    stream.write(networkHealthReport.networkHealth.startDateFractional);
    stream.write(networkHealthReport.networkHealth.currentDateCurrent);
    stream.write(networkHealthReport.networkHealth.currentDateFractional);
    stream.write(networkHealthReport.networkHealth.DPDUsSent);
    stream.write(networkHealthReport.networkHealth.DPDUsLost);
    stream.write(networkHealthReport.networkHealth.GPDULatency);
    stream.write(networkHealthReport.networkHealth.GPDUPathReliability);
    stream.write(networkHealthReport.networkHealth.GPDUDataReliability);
    stream.write(networkHealthReport.networkHealth.joinCount);

    //NetworkHealthReports
    for (NE::Model::Reports::DeviceNetworkHealthList::iterator itDevices = networkHealthReport.deviceHealthList.begin(); itDevices
                != networkHealthReport.deviceHealthList.end(); ++itDevices) {

        NE::Model::Device * device = Model::EngineProvider::getEngine()->getDevice(itDevices->deviceAddress);
        if (!device) {
            std::ostringstream errStream;
            errStream << "marshall NetworkHealthReport - Device " << Address::toString(itDevices->deviceAddress) << " not found.";
            throw NEException(errStream.str());
        }
        device->address128.marshall(stream);

        stream.write(itDevices->startDateCurrent);
        stream.write(itDevices->startDateFractional);
        stream.write(itDevices->currentDateCurrent);
        stream.write(itDevices->currentDateFractional);
        stream.write(itDevices->DPDUsSent);
        stream.write(itDevices->DPDUsLost);
        stream.write(itDevices->GPDULatency);
        stream.write(itDevices->GPDUPathReliability);
        stream.write(itDevices->GPDUDataReliability);
        stream.write(itDevices->joinCount);
    }
}

void marshall(NE::Misc::Marshall::OutputStream& stream, NE::Model::Reports::NetworkResourceReport& networkResourceReport) {

    stream.write((Uint8) networkResourceReport.size());
    for (NE::Model::Reports::NetworkResourceReport::iterator itReports = networkResourceReport.begin(); itReports
                != networkResourceReport.end(); ++itReports) {

        NE::Model::Device * device = Model::EngineProvider::getEngine()->getDevice(itReports->BBRAddress);
        if (!device) {
            std::ostringstream errStream;
            errStream << "marshall NetworkResourceReport - Device " << Address::toString(itReports->BBRAddress) << " not found.";
            throw NEException(errStream.str());
        }
        device->address128.marshall(stream);

        stream.write(itReports->SubnetID);
        stream.write(itReports->SlotLength);
        stream.write(itReports->SlotsOccupied);
        stream.write(itReports->AperiodicData_X);
        stream.write(itReports->AperiodicData_Y);
        stream.write(itReports->AperiodicMgmt_X);
        stream.write(itReports->AperiodicMgmt_Y);
        stream.write(itReports->PeriodicData_X);
        stream.write(itReports->PeriodicData_Y);
        stream.write(itReports->PeriodicMgmt_X);
        stream.write(itReports->PeriodicMgmt_Y);
    }
}

}
}
}
