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
 * ReportsStructures.h
 *
 *  Created on: Jun 16, 2009
 *      Author: Sorin.Bidian
 */

#ifndef REPORTSSTRUCTURES_H_
#define REPORTSSTRUCTURES_H_

#include "Common/NEAddress.h"

namespace NE {
namespace Model {
namespace Reports {

struct DeviceInfo {
    Address32 deviceAddress;
    Uint16 deviceType;
    Uint8 powerSupplyStatus;
    Uint8 joinStatus;
    VisibleString manufacturer;
    VisibleString model;
    VisibleString revision;
    std::string deviceTag;
    VisibleString serialNo;

    DeviceInfo(const Address32 &_deviceAddress, const Uint16 &_deviceType, Uint8 powerSupplyStatus_, Uint8 joinStatus_,
                const VisibleString &_manufacturer, const VisibleString &_model, const VisibleString &_revision,
                const std::string& deviceTag_, const VisibleString &serialNo_) :
        deviceAddress(_deviceAddress), deviceType(_deviceType), powerSupplyStatus(powerSupplyStatus_), joinStatus(joinStatus_), //
                    manufacturer(_manufacturer), model(_model), revision(_revision), deviceTag(deviceTag_), serialNo(serialNo_) {

    }
};
typedef std::list<DeviceInfo> DeviceListReport;

struct Topology {
    struct Device {
        struct Neighbor {
                Address32 neighborAddress; //Address128
                Uint8 clockSource; //0:no; 1:secondary; 2:preferred
        };

        struct Graph {
                Uint16 graphIdentifier;
                //Uint16  numberOfMembers;
                std::list<Address32> graphMemberList;
        };

        Address32 networkAddress; //Address128
        std::list<Neighbor> neighborList;
        std::list<Graph> graphList;
    };

    struct Backbone {
            Address32 backboneAddress; //Address128
            Uint16 subnetID;
    };
    std::list<Device> deviceList;
    std::list<Backbone> backboneList;
};
//typedef std::list<Topology> TopologyReport;
typedef Topology TopologyReport;

struct Channel {
    Uint8 channelNumber; //GS_channel_number
    Uint8 channelStatus; //GS_channel_status : 0 to indicate a disabled channel or 1 to indicate an enabled channel.

    Channel() :
        channelNumber(0), channelStatus(0) {
    }

    Channel(const Uint8& _channelNumber, const Uint8& _channelStatus) :
        channelNumber(_channelNumber), channelStatus(_channelStatus) {
    }

    Channel(const Channel& _channel) {
        channelNumber = _channel.channelNumber;
        channelStatus = _channel.channelStatus;
    }
};

typedef std::list<Channel> ChannelListT;

struct Link {
    Address32 deviceAddress; //GS_network_address
    Uint16 slotIndex; //GS_slot_index
    Uint16 linkPeriod;
    Uint16 slotLength; //GS_slot_length
    Uint8 channelNumber; //GS_channel_number
    Uint8 direction; //GS_Direction
    Uint8 linkType; //GS_Link_Type

    Link() :
        deviceAddress(0x00000000), slotIndex(0), linkPeriod(0), slotLength(0), channelNumber(0), direction(0), linkType(0) {
    }

    Link(const Address32 _deviceAddress, const Uint16 _slotIndex, Uint16 _linkPeriod, const Uint16 _slotLength, const Uint8 _channelNumber,
                const Uint8 _direction, const Uint8 _linkType) :
        deviceAddress(_deviceAddress), slotIndex(_slotIndex), linkPeriod(_linkPeriod), slotLength(_slotLength), channelNumber(_channelNumber),
                    direction(_direction), linkType(_linkType) {

    }
};

typedef std::list<Link> LinkListT;

struct Superframe {
    Uint16 superframeID; //GS_superframe_ID
    Uint16 numberOfTimeSlots; // GS_Num_Time_Slots
    Int32 startTime; //GS_start_time;
    LinkListT linkList; //linkList[ numberOfTimeSlots ]

    Superframe(Uint16 _superframeID, Uint16 _numberOfTimeSlots, const Int32 &_startTime, const LinkListT &_linkList) {
        superframeID = _superframeID;
        numberOfTimeSlots = _numberOfTimeSlots;
        startTime = _startTime;
        linkList = _linkList;
    }
};

typedef std::list<Superframe> SuperframeListT;

struct DeviceSchedule {
    Address32 deviceAddress;
    //Uint16 nrOfSuperframes;
    SuperframeListT superframeList; //superframeList[ nrOfSuperframes ]

    DeviceSchedule(const Address32 &_deviceAddress, const SuperframeListT &_superframeList) {

        deviceAddress = _deviceAddress;
        superframeList = _superframeList;
    }
};

//will contain entries for every device
typedef std::list<DeviceSchedule> DeviceScheduleList;

struct ScheduleReport {
    ChannelListT channelList;
    DeviceScheduleList deviceScheduleList;

    ScheduleReport() { }

    ScheduleReport(const ChannelListT &_channelList, const DeviceScheduleList &_deviceScheduleList) {
        channelList = _channelList;
        deviceScheduleList = _deviceScheduleList;
    }
};

struct DeviceHealth {
    Address32 deviceAddress;
    Uint32 DPDUsTransmitted;
    Uint32 DPDUsReceived;
    Uint32 DPDUsFailedTransmission;
    Uint32 DPDUsFailedReception;

    DeviceHealth() :
        deviceAddress(0x00000000), DPDUsTransmitted(0), DPDUsReceived(0), DPDUsFailedTransmission(0), DPDUsFailedReception(0) {
    }

    DeviceHealth(const Address32 &_deviceAddress, const Uint32 &_DPDUsTransmitted, const Uint32 &_DPDUsReceived,
                const Uint32 &_DPDUsFailedTransmission, const Uint32 &_DPDUsFailedReception) :
        deviceAddress(_deviceAddress), DPDUsTransmitted(_DPDUsTransmitted), DPDUsReceived(_DPDUsReceived),
                    DPDUsFailedTransmission(_DPDUsFailedTransmission), DPDUsFailedReception(_DPDUsFailedReception) {

    }
};

//will contain entries for every device
typedef std::list<DeviceHealth> DeviceHealthReport;

struct NeighborHealth {
    Address32 networkAddress; // GS_Network_Address
    Uint8 linkStatus; // GS_Link_Status -- indicates whether the neighbor is un/available for communication (0/1)
    Uint32 DPDUsTransmitted; // GS_DPDUs_Transmitted
    Uint32 DPDUsReceived; // GS_DPDUs_Received
    Uint32 DPDUsFailedTransmission; // GS_DPDUs_Failed_Transmission
    Uint32 DPDUsFailedReception; // GS_DPDUs_Failed_Reception
    Int16 signalStrength; // GS_Signal_Strength
    Uint8 signalQuality; // GS_Signal_Quality

    NeighborHealth() :
        networkAddress(0x00000000), linkStatus(0), DPDUsTransmitted(0), DPDUsReceived(0), DPDUsFailedTransmission(0),
                    DPDUsFailedReception(0), signalStrength(0), signalQuality(0) {
    }

    NeighborHealth(const Address32 _networkAddress, const Uint8 _linkStatus, const Uint32 _DPDUsTransmitted,
                const Uint32 _DPDUsReceived, const Uint32 _DPDUsFailedTransmission, const Uint32 _DPDUsFailedReception,
                const Int16 _signalStrength, const Uint8 _signalQuality) :
        networkAddress(_networkAddress), linkStatus(_linkStatus), DPDUsTransmitted(_DPDUsTransmitted), DPDUsReceived(
                    _DPDUsReceived), DPDUsFailedTransmission(_DPDUsFailedTransmission), DPDUsFailedReception(
                    _DPDUsFailedReception), signalStrength(_signalStrength), signalQuality(_signalQuality) {
    }
};

typedef std::list<NeighborHealth> NeighborHealthReport;

struct NetworkHealth {
    Uint32 networkID;
    Uint8 networkType;
    Uint16 deviceCount;
    Uint32 startDateCurrent; //TAINetworkTimeValue.currentTAI
    Uint16 startDateFractional; //TAINetworkTimeValue.fractionalTAI
    Uint32 currentDateCurrent; //TAINetworkTimeValue.currentTAI
    Uint16 currentDateFractional; //TAINetworkTimeValue.fractionalTAI
    Uint32 DPDUsSent;
    Uint32 DPDUsLost;
    Uint8 GPDULatency;
    Uint8 GPDUPathReliability;
    Uint8 GPDUDataReliability;
    Uint32 joinCount;

    NetworkHealth() :
        networkID(0), networkType(0), deviceCount(0), startDateCurrent(0), startDateFractional(0), currentDateCurrent(0),
                    currentDateFractional(0), DPDUsSent(0), DPDUsLost(0), GPDULatency(0), GPDUPathReliability(0),
                    GPDUDataReliability(0), joinCount(0) {
    }

    NetworkHealth(const Uint32 &_networkID, const Uint8 &_networkType, const Uint16 &_deviceCount,
                const Uint32 &_startDateCurrent, const Uint16 &_startDateFractional, const Uint32 &_currentDateCurrent,
                const Uint16 &_currentDateFractional, const Uint32 &_DPDUsSent, const Uint32 &_DPDUsLost,
                const Uint8 &_GPDULatency, const Uint8 &_GPDUPathReliability, const Uint8 &_GPDUDataReliability,
                const Uint32 &_joinCount) :
        networkID(_networkID), networkType(_networkType), deviceCount(_deviceCount), startDateCurrent(_startDateCurrent),
                    startDateFractional(_startDateFractional), currentDateCurrent(_currentDateCurrent),
                    currentDateFractional(_currentDateFractional), DPDUsSent(_DPDUsSent), DPDUsLost(_DPDUsLost), GPDULatency(
                                _GPDULatency), GPDUPathReliability(_GPDUPathReliability), GPDUDataReliability(
                                _GPDUDataReliability), joinCount(_joinCount) {

    }
};

struct DeviceNetworkHealth {
    Address32 deviceAddress; // GS_Network_Address
    Uint32 startDateCurrent; //TAINetworkTimeValue.currentTAI
    Uint16 startDateFractional; //TAINetworkTimeValue.fractionalTAI
    Uint32 currentDateCurrent; //TAINetworkTimeValue.currentTAI
    Uint16 currentDateFractional; //TAINetworkTimeValue.fractionalTAI
    Uint32 DPDUsSent; // GS_DPDUs_Sent
    Uint32 DPDUsLost; // GS_DPDUs_Lost
    Uint8 GPDULatency; // GS_GPDU_Latency
    Uint8 GPDUPathReliability; // GS_GPDU_Path_Reliability
    Uint8 GPDUDataReliability; // GS_GPDU_Data_Reliability
    Uint32 joinCount; // GS_Join_Count

    DeviceNetworkHealth() :
        deviceAddress(0x00000000), startDateCurrent(0), startDateFractional(0), currentDateCurrent(0), currentDateFractional(
                    0), DPDUsSent(0), DPDUsLost(0), GPDULatency(0), GPDUPathReliability(0), GPDUDataReliability(0),
                    joinCount(0) {
    }

    DeviceNetworkHealth(const Address32 _deviceAddress, const Uint32 _startDateCurrent, const Uint16 _startDateFractional,
                const Uint32 _currentDateCurrent, const Uint16 _currentDateFractional, const Uint32 _DPDUsSent,
                const Uint32 _DPDUsLost, const Uint8 _GPDULatency, const Uint8 _GPDUPathReliability,
                const Uint8 _GPDUDataReliability, const Uint32 _joinCount) :
        deviceAddress(_deviceAddress), startDateCurrent(_startDateCurrent), startDateFractional(_startDateFractional),
                    currentDateCurrent(_currentDateCurrent), currentDateFractional(_currentDateFractional), DPDUsSent(
                                _DPDUsSent), DPDUsLost(_DPDUsLost), GPDULatency(_GPDULatency), GPDUPathReliability(
                                _GPDUPathReliability), GPDUDataReliability(_GPDUDataReliability), joinCount(_joinCount) {

    }
};

typedef std::list<DeviceNetworkHealth> DeviceNetworkHealthList;

struct NetworkHealthReport {
        NetworkHealth networkHealth;
        //Uint32 numberOfDevices;
        DeviceNetworkHealthList deviceHealthList; //deviceHealthList [ numberOfDevices ]
};

struct NetworkResource { // all members are sent network order
    Address32 BBRAddress; // GS_Network_Address
    Uint16 SubnetID; // GS_Subnet_ID associated with BBR
    Uint32 SlotLength; // GS_Slot_Length - Length of timeslot, will be reported in �Seconds.
    Uint32 SlotsOccupied; // GS_Number_Slots_Occupied
    Uint32 AperiodicData_X; // GS_Slots_Linktype_0_X
    Uint32 AperiodicData_Y; // GS_Slots_Linktype_0_Y
    Uint32 AperiodicMgmt_X; // GS_Slots_Linktype_1_X
    Uint32 AperiodicMgmt_Y; // GS_Slots_Linktype_1_Y
    Uint32 PeriodicData_X; // GS_Slots_Linktype_2_X
    Uint32 PeriodicData_Y; // GS_Slots_Linktype_2_Y
    Uint32 PeriodicMgmt_X; // GS_Slots_Linktype_3_X
    Uint32 PeriodicMgmt_Y; // GS_Slots_Linktype_3_Y


    NetworkResource() :
        BBRAddress(0x00000000), SubnetID(0), SlotLength(0), SlotsOccupied(0), AperiodicData_X(0), AperiodicData_Y(0),
                    AperiodicMgmt_X(0), AperiodicMgmt_Y(0), PeriodicData_X(0), PeriodicData_Y(0), PeriodicMgmt_X(0),
                    PeriodicMgmt_Y(0) {
    }

    NetworkResource(const Address32 _BBRAddress, const Uint16 _SubnetID, const Uint32 _SlotLength,
                const Uint32 _SlotsOccupied, const Uint32 _AperiodicData_X, const Uint32 _AperiodicData_Y,
                const Uint32 _AperiodicMgmt_X, const Uint32 _AperiodicMgmt_Y, const Uint32 _PeriodicData_X,
                const Uint32 _PeriodicData_Y, const Uint32 _PeriodicMgmt_X, const Uint32 _PeriodicMgmt_Y) :
        BBRAddress(_BBRAddress), SubnetID(_SubnetID), SlotLength(_SlotLength), SlotsOccupied(_SlotsOccupied),
                    AperiodicData_X(_AperiodicData_X), AperiodicData_Y(_AperiodicData_Y), AperiodicMgmt_X(_AperiodicMgmt_X),
                    AperiodicMgmt_Y(_AperiodicMgmt_Y), PeriodicData_X(_PeriodicData_X), PeriodicData_Y(_PeriodicData_Y),
                    PeriodicMgmt_X(_PeriodicMgmt_X), PeriodicMgmt_Y(_PeriodicMgmt_Y) {
    }
};

typedef std::list<NetworkResource> NetworkResourceReport;

}
}
}

#endif /* REPORTSSTRUCTURES_H_ */
