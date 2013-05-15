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
 * model.h
 *
 *  Created on: Sep 4, 2009
 *      Author: Catalin Pop
 */

#ifndef MODEL_H_
#define MODEL_H_

#include "Common/NEAddress.h"
#include "Common/logging.h"
#include "Model/Tdma/TdmaTypes.h"
#include "Model/ContractTypes.h"
#include <Model/SecurityKey.h>
#include <Model/Policy.h>
#include <map>
#include <list>
#include <boost/noncopyable.hpp>
#include "modelDefault.h"

namespace NE {
namespace Model {

#define ROUTE_GRAPH_MASK 0xA000
#define isRouteGraphElement(value) ((value & ROUTE_GRAPH_MASK) == ROUTE_GRAPH_MASK)
#define getRouteElement(value) (isRouteGraphElement(value)? (value & 0x0FFF) : value)

typedef Uint16 OperationID;
typedef std::list<OperationID> OperationIDList;

template<class T>
struct Attribute {
        T * currentValue;
        T * previousValue;
        bool isPending;
        OperationIDList waitingOperations;//Used as a FIFO for operations waiting to be applied to this entity.

        Attribute() :
            currentValue(NULL), previousValue(NULL), isPending(false) {
        }

        Attribute(T * currentValue_, bool isPending_ = false) :
            currentValue(currentValue_), previousValue(NULL), isPending(isPending_) {
        }

        bool isOnPending() const {
            return isPending;
        }

        T * getValue() const {
            return currentValue;
        }
        void addWaitingOperation(OperationID opId){ waitingOperations.push_back(opId); }
        bool isFirstOperation(OperationID opId){ return waitingOperations.front() == opId; }
        void removeOperation(OperationID opId){
            if (waitingOperations.empty()){
                return;
            }
            waitingOperations.remove(opId);
        }

};

namespace EntityType {
enum EntityTypeEnum {
    Link = 1, // indexed attribute
    Superframe = 2, // indexed attribute
    Neighbour = 3, // indexed attribute
    Candidate = 4, // indexed attribute
    Route = 5, // indexed attribute
    NetworkRoute = 6, // indexed attribute
    Graph = 7, // indexed attribute
    Contract = 8, // indexed attribute
    NetworkContract = 9,
    AdvJoinInfo = 10,
    AdvSuperframe = 11,
    SessionKey = 12, // indexed attribute
    PowerSupply = 13,
    AddressTranslation = 14, // indexed attribute
    ClientServerRetryTimeout = 15,
    ClientServerRetryMaxTimeoutInterval = 16,
    ClientServerRetryCount = 17,
    Channel = 18,
    BlackListChannels = 19,
    ChannelHopping = 20,

    // Vendor Entities
    Vendor_ID = 21,
    Model_ID = 22,
    Software_Revision_Information = 23,

    // Metadata Entities
    ContractsTable_MetaData = 24,
    Neighbor_MetaData = 25,
    Superframe_MetaData = 26,
    Graph_MetaData = 27,
    Link_MetaData = 28,
    Route_MetaData = 29,
    Diag_MetaData = 30,

    ChannelDiag = 31,
    NeighborDiag = 32,

    // publish
    HRCO_CommEndpoint = 33,
    HRCO_Publish = 34,

    DeviceCapability = 35,

    // alerts
    ARMO_CommEndpoint = 36,

    SerialNumber = 37,
    MasterKey = 38,
    SubnetKey = 39,
    DLMO_MaxLifetime = 40,

    AssignedRole = 41,

    DLMO_ClockExpire = 42,
    DLMO_ClockStale = 43,

    DLMO_IdleChannels = 44,

    PackagesStatistics = 45,
    JoinReason = 46,
    DLMO_DiscoveryAlert = 47,
    PingInterval = 48,

    QueuePriority = 49
};

inline std::string toString(NE::Model::EntityType::EntityTypeEnum value) {
    switch (value) {
        case NE::Model::EntityType::Link:
            return "Link";
        case NE::Model::EntityType::Superframe:
            return "Superframe";
        case NE::Model::EntityType::Neighbour:
            return "Neighbor";
        case NE::Model::EntityType::Candidate:
            return "Candidate";
        case NE::Model::EntityType::Route:
            return "Route";
        case NE::Model::EntityType::NetworkRoute:
            return "NetworkRoute";
        case NE::Model::EntityType::Graph:
            return "Graph";
        case NE::Model::EntityType::Contract:
            return "Contract";
        case NE::Model::EntityType::NetworkContract:
            return "NetworkContract";
        case NE::Model::EntityType::AdvJoinInfo:
            return "AdvJoinInfo";
        case NE::Model::EntityType::AdvSuperframe:
            return "AdvSuperframe";
        case NE::Model::EntityType::SessionKey:
            return "SessionKey";
        case NE::Model::EntityType::PowerSupply:
            return "PowerSupply";
        case NE::Model::EntityType::AddressTranslation:
            return "AddressTranslation";
        case NE::Model::EntityType::ClientServerRetryTimeout:
            return "ClientServerRetryTimeout";
        case NE::Model::EntityType::ClientServerRetryMaxTimeoutInterval:
            return "ClientServerRetryMaxTimeoutInterval";
        case NE::Model::EntityType::ClientServerRetryCount:
            return "ClientServerRetryCount";
        case NE::Model::EntityType::Channel:
            return "Channel";
        case NE::Model::EntityType::BlackListChannels:
            return "BlackListChannels";
        case NE::Model::EntityType::ChannelHopping:
            return "ChannelHopping";
        case NE::Model::EntityType::Vendor_ID:
            return "Vendor_ID";
        case NE::Model::EntityType::Model_ID:
            return "Model_ID";
        case NE::Model::EntityType::Software_Revision_Information:
            return "Software_Revision_Information";
        case NE::Model::EntityType::ContractsTable_MetaData:
            return "ContractsTable_MetaData";
        case NE::Model::EntityType::Neighbor_MetaData:
            return "Neighbor_MetaData";
        case NE::Model::EntityType::Superframe_MetaData:
            return "Superframe_MetaData";
        case NE::Model::EntityType::Graph_MetaData:
            return "Graph_MetaData";
        case NE::Model::EntityType::Link_MetaData:
            return "Link_MetaData";
        case NE::Model::EntityType::Route_MetaData:
            return "Route_MetaData";
        case NE::Model::EntityType::Diag_MetaData:
            return "Diag_MetaData";
        case NE::Model::EntityType::HRCO_CommEndpoint:
            return "HRCO_CommEndpoint";
        case NE::Model::EntityType::HRCO_Publish:
            return "HRCO_Publish";
        case NE::Model::EntityType::DeviceCapability:
            return "DeviceCapability";
        case NE::Model::EntityType::QueuePriority:
        	return "QueuePriority";
        case NE::Model::EntityType::ARMO_CommEndpoint:
            return "ARMO_CommEndpoint";
        case NE::Model::EntityType::SerialNumber:
            return "SerialNumber";
        case NE::Model::EntityType::DLMO_MaxLifetime:
            return "DLMO_MaxLifetime";
        case NE::Model::EntityType::MasterKey:
            return "MasterKey";
        case NE::Model::EntityType::SubnetKey:
            return "SubnetKey";
        case NE::Model::EntityType::AssignedRole:
            return "AssignedRole";
        case NE::Model::EntityType::DLMO_ClockExpire:
            return "DLMO_ClockExpire";
        case NE::Model::EntityType::DLMO_ClockStale:
            return "DLMO_ClockStale";
        case NE::Model::EntityType::DLMO_IdleChannels:
            return "DLMO_IdleChannels";
        case NE::Model::EntityType::PingInterval:
            return "DMO_PingInterval";
        case NE::Model::EntityType::DLMO_DiscoveryAlert:
            return "DLMO_DiscoveryAlert";
        default:
            return "?";
    }
}
}

typedef Uint64 EntityIndex;
typedef std::list<EntityIndex> EntityIndexList;

inline EntityIndex createEntityIndex(Address32 deviceAddress, EntityType::EntityTypeEnum entityType, Uint16 index) {
    EntityIndex entityIndex = deviceAddress;
    entityIndex <<= 16;
    entityIndex |= entityType;
    entityIndex <<= 16;
    entityIndex |= index;
    return entityIndex;
}

inline Uint32 getDeviceAddress(const EntityIndex& entityIndex) {
    return (entityIndex >> 32);
}

inline void setDeviceAddress(EntityIndex& entityIndex, Uint32 deviceAddress) {
    EntityIndex newEntityIndex = deviceAddress;
    newEntityIndex <<= 32;

    // remove old address
    entityIndex <<= 32;
    entityIndex >>= 32;

    entityIndex |= newEntityIndex;
}

inline EntityType::EntityTypeEnum getEntityType(const EntityIndex& entityIndex) {
    Uint16 index = entityIndex >> 16;
    return (EntityType::EntityTypeEnum) index;
}

inline bool isEntityIndexOfType(const EntityIndex& entityIndex, const EntityType::EntityTypeEnum type) {
    const EntityType::EntityTypeEnum currentType = getEntityType(entityIndex);
    return currentType == type;
}

inline Uint16 getIndex(const EntityIndex& entityIndex) {
    return (Uint16) entityIndex;//Index is at first 16 bits of entityIndex
}

struct Schedule {
        Uint16 offset; //ExtDLUint
        Uint16 interval; //ExtDLUint
};

/*************************************************
 *  Start definition of attributes types
 ************************************************/

struct IPhy {
    virtual ~IPhy(){}
};

struct PhyUint8 : public IPhy {
        PhyUint8():value(0){}
        PhyUint8(Uint8 value_):value(value_){}
        Uint8 value;
};
typedef Attribute<PhyUint8> AttributeUint8;

struct PhyUint16 : public IPhy {
        PhyUint16():value(0){}
        Uint16 value;
};
typedef Attribute<PhyUint16> AttributeUint16;

struct PhyString : public IPhy {
	PhyString(){}
	PhyString(std::string value_):value(value_){}
	std::string value;
};
typedef Attribute<PhyString> AttributeString;

struct PhyBytes : public IPhy {
    PhyBytes() {}
    PhyBytes(Bytes & value_) : value(value_){}
    Bytes value;
};
typedef Attribute<PhyBytes> AttributeBytes;

struct PhyMetaData : public IPhy {
        PhyMetaData():used(0), total(0){}
        PhyMetaData(Uint16 used_, Uint16 total_):used(used_), total(total_){}
        Uint16 used;
        Uint16 total;
};
typedef Attribute<PhyMetaData> AttributeMetaData;

struct PhyAdvJoinInfo : public IPhy {
        Uint8 joinBackoff;
        Uint8 joinTimeout;
        NE::Model::Tdma::ScheduleType::ScheduleTypeEnum txScheduleType;
        NE::Model::Tdma::ScheduleType::ScheduleTypeEnum rxScheduleType;
        bool sendAdvRx;
        NE::Model::Tdma::ScheduleType::ScheduleTypeEnum advScheduleType;
        Schedule joinTx;
        Schedule joinRx;
        Schedule advRx;
};
typedef Attribute<PhyAdvJoinInfo> AdvJoinInfoAttribute;

struct PhyNeighbor : public IPhy {
        PhyNeighbor() :
            index(0), groupCode(0), clockSource(0), extGrCnt(0), diagLevel(0), linkBacklog(0), linkBacklogIndex(0), linkBacklogDur(0) {
        }
        /**
         * Unique identifier.
         */
        Uint16 index; //ExtDLUint

        /**
         * Neighbor's 64 bit address.
         */
        NE::Common::Address64 address64;

        /**
         * May associate a group code with a set of neighbors; used by dlmo11a.Link.
         */
        Uint8 groupCode;

        /**
         * Indicates whether neighbor is a clock source.
         * 0 = no
         * 1 = secondary
         * 2 = preferred
         */
        Uint8 clockSource;

        /**
         * Count of graphs virtually extended for this neighbor.
         */
        Uint8 extGrCnt;

        /**
         * Selection of neighbor diagnostics to collect.
         * Bit0 = 1   Collect link diagnostics
         * Bit1 = 1   Collect clock diagnostics
         * Bit2 = 1   Collect per-channel informaitonType
         */
        Uint8 diagLevel;

        /**
         * Indicates that link information is provided to facilitate clearing message queue backlog to the neighbor.
         * 1 = Extra link information is provided
         * 0 = No extra link information
         */
        Uint8 linkBacklog;

        struct ExtendGraph {
                /**
                 * Index of dlmo11a.Graph attribute.
                 */
                Uint16 graph_ID;

                /**
                 * Indicates whether the neighbor shall be the last hop.
                 */
                Uint8 lastHop;

                /**
                 * Indicates whether to treat the neighbor as the preferred branch.
                 */
                Uint8 preferredBranch;
        };

        std::vector<ExtendGraph> extendGraph;

        /**
         * Activate this link Link to clear queue backlog.
         */
        Uint16 linkBacklogIndex; //ExtDLUint

        /**
         * Duration of link activation.
         */
        Uint8 linkBacklogDur;
};
typedef Attribute<PhyNeighbor> NeighborAttribute;
typedef std::map<EntityIndex, NeighborAttribute> NeighborIndexedAttribute;

struct PhyRoute : public IPhy {
        PhyRoute() :
            index(0), evaluationTime(0), alternative(0), forwardLimit(15), selector(0), sourceAddress(0) {
        }
        /**
         * Unique identifier.
         */
        Uint16 index; //ExtDLUint

        /**
         * evaluation Time=0 when the route it's created.
         */
        Uint8 evaluationTime;

        /**
         * 0 - route based on contract and srcAddress. used on BBR for configuring a route for a contract of SM and other route for a contract of GW.
         * 1 - select this route based on the contract ID from selector field
         * 2 - select this route based on Address16 from selector
         * 3 - default route (selector is null)
         */
        Uint8 alternative;

        /**
         * Initialization value for the forwarding limit in DPDUs that use this route.
         */
        Uint8 forwardLimit;

        /**
         * Series of routing destinations; if entry starts with 0, represents a unicast address;
         * if entry starts with 1010(0xA), represents a graph.
         */
        std::vector<Uint16> route;

        /**
         *
         */
        Uint16 selector;

        /**
         * Custom: The source of the route; sent only if alternative = 0.
         */
        Address32 sourceAddress;
};
typedef Attribute<PhyRoute> RouteAttribute;
typedef std::map<EntityIndex, RouteAttribute> RouteIndexedAttribute;

struct PhyGraph : public IPhy {
        PhyGraph() :
            index(0),
            preferredBranch(1),
            neighborCount(0),
            queue(5),//should be verified if is reasonable default value
            maxLifetime(0) //0--the value from dlmo.MaxLifetime will be used by devices
        {
        }
        /**
         * Unique identifier. Must be maximum on 12 bits because is referred by Route.
         */
        Uint16 index; //ExtDLUint

        /**
         * Indicates whether to treat the first listed neighbor as the preferred branch.
         */
        Uint8 preferredBranch;

        /**
         *
         */
        Uint8 neighborCount;

        /**
         * Allocates buffers in the message queue for messages that are being forwarded using this graph.
         */
        Uint8 queue;

        /**
         * If this element is non-zero, the value of dlmo11a.MaxLifetime shall be overridden
         * for all messages being forwarded following this graph.
         */
        Uint16 maxLifetime; //ExtDLUint

        /**
         * Row IDs into dlmo11a.Neighbor; typically two or three neighbors in list for next-hop diversity.
         */
        std::vector<Uint16> neighbors; //ExtDLUint
};
typedef Attribute<PhyGraph> GraphAttribute;
typedef std::map<EntityIndex, GraphAttribute> GraphIndexedAttribute;

struct PhyLink : public IPhy {
        PhyLink():
                    index(0), //ExtDLUint
                    superframeIndex(0), //ExtDLUint
                    type(0),
                    direction(0),
                    template1(0), //ExtDLUint
                    template2(0), //ExtDLUint
                    role(Tdma::TdmaLinkTypes::DEFAULT),
                    neighborType(0),
                    graphType(0),
                    schedType(NE::Model::Tdma::ScheduleType::OFFSET),
                    chType(0),
                    priorityType(0),
                    neighbor(0), //ExtDLUint
                    graphID(0), //ExtDLUint
                    chOffset(0),
                    priority(0)

        {
        }
        /**
         * Unique identifier.
         */
        Uint16 index; //ExtDLUint

        /**
         * Reference to dlmo11a. Superframe entry.
         */
        Uint16 superframeIndex; //ExtDLUint

        /**
         * The type of the link. See LinkTypesEnum(TdmaTypes.h) and in standard table 172.
         */
        Uint8 type;

        /**
         * enum TdmaLinkDirEnum { INBOUND  = 0,OUTBOUND = 1 }
         */
        Uint8 direction;

        /**
         * dlmo11a.TsTemplate reference to primary template.
         */
        Uint16 template1; //ExtDLUint

        /**
         * dlmo11a.TsTemplate reference to secondary template.
         * Use Template2 as the receive template, if there is no message in the queue for the primary template.
         */
        Uint16 template2; //ExtDLUint

        /**
         * The role of the link: DEFAULT/JOIN/MANAGEMENT/APPLICATION
         */

        Tdma::TdmaLinkTypes::TdmaLinkTypesEnum role;

        /**
         * Possible values NeighbourTypesEnum {  NONE = 0, NEIGHBOUR_BY_ADDRESS = 1, NEIGHBOUR_BY_GROUP = 2 }
         */
        Uint8 neighborType;

        /**
         * Possible values: enum GraphTypesEnum { NONE = 0, SPECIFIC_GRAPH = 1, SPECIFIC_GRAPH_WITH_PRIORITY = 2 }
         */
        Uint8 graphType;

        NE::Model::Tdma::ScheduleType::ScheduleTypeEnum schedType;

        /**
         * Possible values: enum ChannelOffsetTypeEnum { NONE = 0, USE_CHANNEL_OFFSET = 1}
         */
        Uint8 chType;

        /**
         * Possible values: enum PriorityTypeEnum { USE_SUPERFRAME_PRIORITY = 0, USE_LINK_PRIORITY = 1};
         */
        Uint8 priorityType;

        /**
         * Identify neighbor or group.
         */
        Uint16 neighbor; //ExtDLUint

        /**
         * 12-bit identity of graph with exclusive or prioritized access to link.
         */
        Uint16 graphID; //ExtDLUint

        /**
         * Link schedule.
         */
        Schedule schedule;

        /**
         * Select channel based on offset from superframe's hop pattern.
         */
        Uint8 chOffset;

        /**
         * Link priority.
         */
        Uint8 priority;
};
typedef Attribute<PhyLink> LinkAttribute;
typedef std::map<EntityIndex, LinkAttribute> LinkIndexedAttribute;

namespace SuperframeType {
enum SuperframeType {
    Baseline = 0, Hop_On_Link = 1, Randomize_Slow_Hop = 2, Randomize_Sf_Period = 3
};
}

struct PhySuperframe : public IPhy {
        PhySuperframe():
                    index(0), //ExtDLUint
                    tsDur(0),
                    chIndex(0), //ExtDLUint
                    chBirth(0),
                    sfType(0),
                    priority(0),
                    chMapOv(1),
                    idleUsed(0),
                    sfPeriod(0),
                    sfBirth(0),
                    chRate(1),
                    chMap(DEFAULT_CHANNEL_MAP),
                    idleTimer(0),
                    rndSlots(0)
        {
        }
        /**
         * Unique identifier.
         */
        Uint16 index; //ExtDLUint

        /**
         * Duration of time slots within superframe;
         * time slots are realigned with TAI time reference every 250 msec.
         */
        Uint16 tsDur;

        /**
         * Selects hopping pattern from dlmo11a.Ch.
         */
        Uint16 chIndex; //ExtDLUint

        /**
         * Absolute slot number where channel hopping pattern nominally started.
         */
        Uint8 chBirth;

        /**
         * Type of superframe.
         * enum SuperframeType { Baseline = 0, Hop_On_Link = 1, Randomize_Slow_Hop = 2, Randomize_Sf_Period = 3};
         */
        Uint8 sfType;

        /**
         * Priority to select among multiple available links.
         */
        Uint8 priority;

        /**
         * Indicates whether to override ChMap default.
         * 1=Override ChMap default
         * 0=use ChMap default of 0x7fff
         */
        Uint8 chMapOv;

        /**
         * 1=IdleTimer transmitted and used
         * 0=IdleTime not transmitted and defaults to -1
         */
        Uint8 idleUsed;

        /**
         * Base number of timeslots in each superframe cycle.
         */
        Uint16 sfPeriod;

        /**
         * Absolute slot number where the first superframe cycle nominally started.
         */
        Uint16 sfBirth;

        /**
         * Indicates the number of timeslots per hop.
         * 1 = slotted hopping
         * >1 = slow hopping
         */
        Uint16 chRate;

        /**
         * Channel map used to eliminate certain channels from the hopping pattern for spectrum management.
         */
        Uint16 chMap;

        /**
         * Idle/wake up timer for superframe.
         */
        Int32 idleTimer;

        /**
         * Indicates extent of randomization, in number of slots.
         */
        Uint8 rndSlots;
};
typedef Attribute<PhySuperframe> SuperframeAttribute;
typedef std::map<EntityIndex, SuperframeAttribute> SuperframeIndexedAttribute;

/*
 * Network Route table row element.
 */
struct PhyNetworkRoute : public IPhy {
        PhyNetworkRoute() :
            networkRouteID(0),nwkHopLimit(255), outgoingInterface(0) {
        }

        Uint16 networkRouteID;
        NE::Common::Address128 destination;
        NE::Common::Address128 nextHop;
        Uint8 nwkHopLimit;
        /**
         * 0 - DLL interface: this route goes to RF
         * 1 - Backbone network interface: this route goes in backbone network (IP network)
         */
        Uint8 outgoingInterface;
};
typedef Attribute<PhyNetworkRoute> NetworkRouteAttribute;
typedef std::map<EntityIndex, NetworkRouteAttribute> NetworkRouteIndexedAttribute;

/**
 * Address Translation Table row element.
 */
struct PhyAddressTranslation : public IPhy {
        PhyAddressTranslation() :
            addressTranslationID(0),
            shortAddress(0) {
        }

        Uint16 addressTranslationID;
        NE::Common::Address128 longAddress;
        Address16 shortAddress;
};
typedef Attribute<PhyAddressTranslation> AddressTranslationAttribute;
typedef std::map<EntityIndex, AddressTranslationAttribute> AddressTranslationIndexedAttribute;

struct PhyContract : public IPhy {
        PhyContract():
            contractID(0),
            requestID(0),
            sourceSAP(0),
            source32(0),
            destinationSAP(0),
            destination32(0),
            responseCode(NE::Model::ContractResponseCode::SuccessWithImmediateEffect),
            communicationServiceType(NE::Model::CommunicationServiceType::NonPeriodic),
            contract_Activation_Time(0),
            assigned_Contract_Life(0),
            assigned_Contract_Priority(NE::Model::ContractPriority::BestEffortQueued),
            assigned_Max_NSDU_Size(0),
            assigned_Reliability_And_PublishAutoRetransmit(0),
            assignedPeriod(0),
            assignedPhase(0),
            assignedDeadline(0),
            assignedCommittedBurst(0),
            assignedExcessBurst(0),
            assigned_Max_Send_Window_Size(0),
            isManagement(0),
            usedForFirmwareUpdate(0)
        {
        }
        /**
         * Value 0 reserved to mean no contract.
         */
        Uint16 contractID;

        /**
         * A numerical value, uniquely assigned by the device requesting a contract from
         * the system manager, to identify the request being made.
         */
        //Uint8 requestID;
        Uint16 requestID;

        Uint16 sourceSAP;

        /**
         * The 32bit address of the source of this contract.
         */
        Address32 source32;

        Uint16 destinationSAP;

        /**
         * The 32bit address of the destination of this contract.
         */
        Address32 destination32;

        /**
         * The 128bit address of the destination of this contract.(used by ContractData)
         */
        NE::Common::Address128 destination128;

        /**
         * Indicates the system manager's response for the request; Enumeration:
         0  success with immediate effect
         1  success with delayed effect
         2  success with immediate effect but negotiated down
         3  success with delayed effect but negotiated down
         4  failure with no further guidance
         5  failure with retry guidance
         6  failure with retry and negotiation guidance
         * Uint8
         */
        ContractResponseCode::ContractResponseCode responseCode;

        /**
         * Type of service supported by this contract; (Uint8)
         */
        CommunicationServiceType::CommunicationServiceType communicationServiceType;

        /**
         * Start time for the source to start using the assigned contract.
         */
        Uint32 contract_Activation_Time;

        /**
         * Determines how long the system manager will keep the contract before it is terminated; units in seconds.
         */
        Uint32 assigned_Contract_Life;

        /**
         * Establishes a base priority for all messages sent using the contract; (Uint8)
         */
        ContractPriority::ContractPriority assigned_Contract_Priority;

        /**
         * Indicates the maximum NSDU supported in octets which can be converted by the source into max APDU size
         * supported by taking into account the TL, security, AL header and TMIC sizes;
         * valid value set: 70 - 1280.
         */
        Uint16 assigned_Max_NSDU_Size;

        /**
         * Bit 0 indicates if retransmission of old publish data is necessary if buffer is not overwritten with
         * new publish data, bit 0 is only applicable for periodic communication and is 0 for non-periodic communication;
         * bits 1 to 7 indicate the supported reliability for delivering the transmitted TSDUs to the destination
         * Bit 0 - 0 retransmit (default), 1 do not retransmit
         * Bits 1 to 7  Enumeration
         0 = low
         1 = medium
         2 = high
         */
        Uint8 assigned_Reliability_And_PublishAutoRetransmit;

        /**
         * Used for periodic communication; to identify the assigned publishing period in the contract;
         * a positive number is a multiple of 1 second and a negative number is a fraction of 1 second.
         */
        Int16 assignedPeriod;

        /**
         * Used for periodic communication; to identify the assigned phase of publications in the contract;
         * valid value set: 0 to 99.
         */
        Uint8 assignedPhase;

        /**
         * Used for periodic communication; to identify the maximum end-to-end transport delay supported
         * by the assigned contract; units in 10 milliseconds.
         */
        Uint16 assignedDeadline;

        /**
         * Used for non-periodic communication to identify the long term rate that is supported for client-server
         * or source-sink messages; positive values indicate APDUs per second, negative values indicate seconds per APDU.
         */
        Int16 assignedCommittedBurst;

        /**
         * Used for non-periodic communication to identify the short term rate that is supported for client-server
         * or source-sink messages; positive values indicate APDUs per second, negative values indicate seconds per APDU.
         */
        Int16 assignedExcessBurst;

        /**
         * Used for non-periodic communication; to identify the maximum number of client requests that may be
         * simultaneously awaiting a response.
         */
        Uint8 assigned_Max_Send_Window_Size;

        /**
         * Identifies if the current contract is management contract.
         */
        bool isManagement;

        /*
         * Used for firmware update.
         */
        bool usedForFirmwareUpdate;
};
typedef Attribute<PhyContract> ContractAttribute;
typedef std::map<EntityIndex, ContractAttribute> ContractIndexedAttribute;

/**
 * PhyNetworkContract
 */
struct PhyNetworkContract : public IPhy {
        PhyNetworkContract():
            contractID(0),
            contract_Priority(ContractPriority::BestEffortQueued),
            include_Contract_Flag(0),
            assigned_Max_NSDU_Size(0),
            assigned_Max_Send_Window_Size(0),
            assignedCommittedBurst(0),
            assignedExcessBurst(0)
            {
        }

        Uint16 contractID;

        /**
         * This element is the same as in ContractRequest.
         */
        Address128 sourceAddress;

        /**
         * This element is the same as in ContractRequest.
         */
        Address128 destinationAddress;

        /**
         *
         */
        ContractPriority::ContractPriority contract_Priority;

        /**
         *
         */
        bool include_Contract_Flag;

        /**
         * This element is the same as in ContractResponse.
         */
        Uint16 assigned_Max_NSDU_Size;

        /**
         * This element is the same as in ContractResponse.
         */
        Uint8 assigned_Max_Send_Window_Size;

        /**
         * This element is the same as in ContractResponse.
         */
        Int16 assignedCommittedBurst;

        /**
         * This element is the same as in ContractResponse.
         */
        Int16 assignedExcessBurst;
};
typedef Attribute<PhyNetworkContract> NetworkContractAttribute;
typedef std::map<EntityIndex, NetworkContractAttribute> NetworkContractIndexedAttribute;

struct PhyChannelHopping : public IPhy {
        PhyChannelHopping() :
            index(0), length(0) {
        }
        /*
         * Unique identifier.
         */
        Uint16 index; //ExtDLUint

        /*
         * 0 to 15 indicating length of hopping sequence.
         */
        Uint8 length;

        /*
         * Sequence of channels.
         */
        std::vector<Uint8> seq;
};
typedef Attribute<PhyChannelHopping> ChannelHoppingAttribute;
typedef std::map<EntityIndex, ChannelHoppingAttribute> ChannelHoppingIndexedAttribute;

struct PhyBlacklistChannels : public IPhy {
        std::vector<Uint8> seq;
};
typedef Attribute<PhyBlacklistChannels> BlacklistChannelsAttribute;
typedef std::map<EntityIndex, BlacklistChannelsAttribute> BlacklistChannelsIndexedAttribute;

struct PhyChannelDiag : public IPhy {
        /**
         * Number of attempted unicast transmissions for all channels.
         */
        Uint16 count;

        struct ChannelTransmission {
                /**
                 * Percentage of time transmissions on channel that did not receive an ACK.
                 */
                Int8 noAck;

                /**
                 * Percentage of time transmissions on channel aborted due to CCA.
                 */
                Int8 ccaBackoff;
        };

        std::vector<ChannelTransmission> channelTransmissionList;
};
typedef Attribute<PhyChannelDiag> ChannelDiagAttribute;
typedef std::map<EntityIndex, ChannelDiagAttribute> ChannelDiagIndexedAttribute;

struct PhyCandidate : public IPhy {
        PhyCandidate():neighbor(0),radio(0) {
        }
        /**
         * The 16-bit address of candidate neighbor in the DL subnet.
         */
        Uint16 neighbor; //ExtDLUint
        /**
         * RSQI. The quality of the radio signal from neighbor.
         */
        Uint8 radio;
};
typedef Attribute<PhyCandidate> CandidateAttribute;
typedef std::map<EntityIndex, CandidateAttribute> CandidateIndexedAttribute;

/**
 * Used for master key and DL subnet key.
 */
struct PhySpecialKey : public IPhy {
        PhySpecialKey() :
            keyID(0), softLifeTime(0), hardLifeTime(0), markedAsExpiring(false)  {
        }
        Uint8 keyID;
        SecurityKey key;
        Policy policy;
        Uint32 softLifeTime;
        Uint32 hardLifeTime;
        bool markedAsExpiring;
};
typedef Attribute<PhySpecialKey> SpecialKeyAttribute;
typedef std::map<EntityIndex, SpecialKeyAttribute> SpecialKeyIndexedAttribute;

struct PhySessionKey : public IPhy {
        PhySessionKey() :
            index(0),keyID(0), destinationTSAP(0),sourceTSAP(0), softLifeTime(0), hardLifeTime(0), markedAsExpiring(false)  {
        }
        Uint16 index;
        Uint16 keyID; //actually it is Uint8
        NE::Common::Address64 destination64;
        NE::Common::Address128 destination128;
        NE::Common::Address64 source64; //TODO: analyze if source can be removed; source is the device that holds the key
        NE::Common::Address128 source128; //TODO: analyze if source can be removed; source is the device that holds the key
        int destinationTSAP;
        int sourceTSAP;
        SecurityKey key;
        Policy sessionKeyPolicy;
        Uint32 softLifeTime;
        Uint32 hardLifeTime;
        bool markedAsExpiring;
};
typedef Attribute<PhySessionKey> SessionKeyAttribute;
typedef std::map<EntityIndex, SessionKeyAttribute> SessionKeyIndexedAttribute;

/**
 * Structure that describes the type of the 2nd attribute -CommunicationEndpoint- in Concentrator object.
 */
struct PhyCommunicationAssociationEndpoint : public IPhy {
        PhyCommunicationAssociationEndpoint():
            remotePort(0),
            remoteObjectID(0),
            staleDataLimit(0),
            publicationPeriod(0),
            idealPublicationPhase(0),
            publishAutoRetransmit(0),
            configurationStatus(0){
        }
        /**
         * Network address of remote endpoint.
         */
        NE::Common::Address128 remoteAddress;

        /**
         * Transport layer port at remote endpoint.
         */
        Uint16 remotePort;

        /**
         * Object ID at remote endpoint.
         */
        Uint16 remoteObjectID;

        /**
         * Stale data limit (in units of seconds).
         */
        Uint8 staleDataLimit;

        /**
         * Data publication period.
         */
        Int16 publicationPeriod;

        /**
         * Ideal publication phase  (in units of tens of milliseconds).
         */
        Uint8 idealPublicationPhase;

        /**
         * Valid value set:
         *   0: Transmit only if application content changed since last publication
         *   1 : Transmit at every periodic opportunity (regardless of whether application
         *       content changed since last transmission or not)
         */
        bool publishAutoRetransmit;

        /**
         * Valid value set:
         *   0 : not configured (connection endpoint not valid)
         *   1: configured (connection endpoint valid)
         */
        Uint8 configurationStatus;

};
typedef Attribute<PhyCommunicationAssociationEndpoint> CommunicationAssociationEndpointAttribute;

struct PhyEntityIndexList : public IPhy {
	EntityIndexList value;
};
typedef Attribute<PhyEntityIndexList> EntityIndexListAttribute;

struct PhyAlertCommunicationEndpoint : public IPhy {
        /**
         * Network address of remote endpoint.
         */
        NE::Common::Address128 remoteAddress;

        /**
         * Transport layer port at remote endpoint.
         */
        Uint16 remotePort;

        /**
         * Object ID at remote endpoint.
         */
        Uint16 remoteObjectID;
        PhyAlertCommunicationEndpoint():remotePort(0), remoteObjectID(0) {}
};
typedef Attribute<PhyAlertCommunicationEndpoint> AlertCommunicationEndpointAttribute;

struct PhyEnergyDesign {
        Int16 energyLife; // (DL energy life by design; positive for months, negative for days)
        Uint16 listenRate; // (DL's energy capacity to operate its receiver, in seconds per hour)
        Uint16 transmitRate; // (DL's energy capacity to transmit DPDUs, in DPDUs per minute
        Uint16 advRate; // (DL's energy capacity to transmit advertisements, in DPDUs per minute)

        PhyEnergyDesign() :
        	energyLife(0), listenRate(0), transmitRate(0), advRate(0) {
        }
};

struct PhyDeviceCapability : public IPhy {
        Uint16 queueCapacity; // (capacity of the queue that is available for forwarding operations) // Uint16 ~ ExtDLUint
        Uint8 clockAccuracy; // (nominal clock accuracy of this device, in ppm)
        Uint16 channelMap; // (map of radio channels supported by the device
        Uint8 dlRoles; // (DL roles supported by the device)
        PhyEnergyDesign energyDesign; // (copy of attribute dlmo11a.EnergyDesign) - OctetString
        Int16 energyLeft; // (copy of attribute dlmo11a.EnergyLeft)
        Uint16 ack_Turnaround; // (Time to turn around an ACK/NACK)
        Uint16 neighborDiagCapacity; // (memory capacity for dlmo11a.NeighborDiag)
        Int8 radioTransmitPower; // (copy of attribute RadioTransmitPower)
        Uint8 options; // (optional features supported by DL)

        PhyDeviceCapability() :
        	queueCapacity(0), clockAccuracy(0), channelMap(0), dlRoles(0),
        	energyLeft(0), ack_Turnaround(0), neighborDiagCapacity(0), radioTransmitPower(0), options(0) {
        }
};
typedef Attribute<PhyDeviceCapability> DeviceCapabilityAttribute;

// QueuePriority
struct QueuePriorityEntry {
	Uint8 priority;
	Uint8 qMax;

	QueuePriorityEntry()
		: priority(0), qMax(0) {
	}
	QueuePriorityEntry(Uint8 priority_, Uint8 qMax_)
		: priority(priority_), qMax(qMax_) {
	}
};

typedef std::vector<QueuePriorityEntry> QueuePriorityEntries;

struct PhyQueuePriority : public IPhy {
	QueuePriorityEntries value;
};
typedef Attribute<PhyQueuePriority> QueuePriorityAttribute;


struct PhyAlert_NeighborDiscovery {
        std::list<PhyCandidate> candidates;
};

struct PhyAlert_ChannelDiag {
        PhyChannelDiag channelDiag;
};

struct PhyAlert_NeighborDiag {
        Uint16 index;

        //summary - depends on diagLevel
        Int8 rssi;
        Uint8 rsqi;
        Uint16 rxDPDU;
        Uint16 txSuccessful;
        Uint16 txFailed;
        Uint16 txCCA_Backoff;
        Uint16 txNACK;
        Int16 clockSigma;

        //clock detail - depends on diagLevel
        Int16 clockBias;
        Uint16 clockCount;
        Uint16 clockTimeout;
        Uint16 clockOutliers;
};

struct PhyAlert_MalformedAPDU {
        Address32 srcAddr;
        Uint16 treshold;
        Uint32 currentTAI;
        Uint16 fractionalTAI;
};

struct PhyAlert_PowerStatus {
        //0 - line powered
        //1 - battery powered, greater than 75% remaining capacity
        //2 - battery powered, between 25% and 75% remaining capacity
        //3 - battery powered, less than 25% remaining capacity
        Uint8 powerSupplyStatus;
};

struct AlertsTable {
        std::list<PhyAlert_NeighborDiscovery> neighborDiscoveryAlerts;
        std::list<PhyAlert_ChannelDiag> channelDiagAlerts;
        std::list<PhyAlert_NeighborDiag> neighborDiagAlerts;
        std::list<PhyAlert_MalformedAPDU> malformedAPDUAlerts;
        std::list<PhyAlert_PowerStatus> powerStatusAlerts;

        void addNeighborDiscoveryAlert(const PhyAlert_NeighborDiscovery& alert) {
            neighborDiscoveryAlerts.push_back(alert);
        }
        void addChannelDiagAlert(const PhyAlert_ChannelDiag& alert) {
            channelDiagAlerts.push_back(alert);
        }
        void addNeighborDiagAlert(const PhyAlert_NeighborDiag& alert) {
            neighborDiagAlerts.push_back(alert);
        }
        void addMalformedAPDUAlert(const PhyAlert_MalformedAPDU& alert) {
            malformedAPDUAlerts.push_back(alert);
        }
        void addPowerStatusAlert(const PhyAlert_PowerStatus& alert) {
            powerStatusAlerts.push_back(alert);
        }
};


struct PhyAttributes: public boost::noncopyable {
        //Simple attributes

        /** DLMO.2[id=2] Join information to be placed in advertisement.*/
        AdvJoinInfoAttribute advInfo;

        /** DLMO.AdvSuperframe[id=3] Superframe reference for advertisement. */
        AttributeUint16 advSuperframe;

        /** DMO attribute 10. Status information of power supply of device.
         * Has 4 values: 0= line powered, 1= power > 75%, 2= power between 25% and 75%, 3= power < 25% */
        AttributeUint8 powerSupplyStatus;


        AttributeUint16 assignedRole;

        /**
         * DMO attribute 9. Serial number of device.
         */
        AttributeString serialNumber;

        /**
         * Vendor Attributes
         */
        AttributeString vendorID;
        AttributeString modelID;
        AttributeString softwareRevisionInformation;

        /**
         * The packages statistics.
         */
        AttributeBytes packagesStatistics;

        /**
         * Join reason.
         */
        AttributeBytes joinReason;

        /**
         * Interval (in seconds) device must ping its backup.
         */
        AttributeUint16 pingInterval;

        /**
         * Metadata Attributes
         */
        AttributeMetaData contractsTableMetadata;
        AttributeMetaData neighborMetadata;
        AttributeMetaData superframeMetadata;
        AttributeMetaData graphMetadata;
        AttributeMetaData linkMetadata;
        AttributeMetaData routeMetadata;
        AttributeMetaData diagMetadata;

        CommunicationAssociationEndpointAttribute hrcoCommEndpoint;
        EntityIndexListAttribute hrcoEntityIndexListAttribute;

        AlertCommunicationEndpointAttribute armoCommEndpoint;

        DeviceCapabilityAttribute deviceCapability;
        QueuePriorityAttribute queuePriority;

        AttributeUint16 retryTimeout;
        AttributeUint16 max_Retry_Timeout_Interval;
        AttributeUint8 retryCount;
        AttributeUint16 dlmoMaxLifeTime;

        AttributeUint16 dlmoIdleChannels;
        AttributeUint8 dlmoDiscoveryAlert;

        //Indexed Attributes
        BlacklistChannelsIndexedAttribute blacklistChannelsTable;
        ChannelHoppingIndexedAttribute channelHoppingTable;
        LinkIndexedAttribute linksTable;
        SuperframeIndexedAttribute superframesTable;
        NeighborIndexedAttribute neighborsTable;
        CandidateIndexedAttribute candidatesTable;
        RouteIndexedAttribute routesTable;
        NetworkRouteIndexedAttribute networkRoutesTable;
        GraphIndexedAttribute graphsTable;
        ContractIndexedAttribute contractsTable;
        NetworkContractIndexedAttribute networkContractsTable;
        SessionKeyIndexedAttribute sessionKeysTable;
        SpecialKeyIndexedAttribute masterKeysTable;
        SpecialKeyIndexedAttribute subnetKeysTable;
        AddressTranslationIndexedAttribute addressTranslationTable;

        //Alerts lists
        // AlertsTable alertsTable;

        PhyAttributes();

        ~PhyAttributes();

        void createBlacklistChannel(EntityIndex index, PhyBlacklistChannels * blacklistChannels, bool onPending = false);

        void createChannelHopping(EntityIndex index, PhyChannelHopping * channelHopping, bool onPending = false);

        void createLink(EntityIndex index, PhyLink * link, bool onPending = false);

        void createSuperframe(EntityIndex index, PhySuperframe * superframe, bool onPending = false);

        void createNeighbor(EntityIndex index, PhyNeighbor * neighbor, bool onPending = false);

        void createCandidate(EntityIndex index, PhyCandidate * candidate, bool onPending = false);

        void removeCandidate(EntityIndex candidateIndex);

        void createRoute(EntityIndex index, PhyRoute * route, bool onPending = false);

        void createNetworkRoute(EntityIndex index, PhyNetworkRoute * networkRoute, bool onPending = false);

        void createGraph(EntityIndex index, PhyGraph * graph, bool onPending = false);

        void createContract(EntityIndex index, PhyContract * contract, bool onPending = false);

        void createNetworkContract(EntityIndex index, PhyNetworkContract * networkContract, bool onPending = false);

        void createSessionKey(EntityIndex index, PhySessionKey * sessionKey, bool onPending = false);

        void createMasterKey(EntityIndex index, PhySpecialKey * specialKey, bool onPending = false);

        void createSubnetKey(EntityIndex index, PhySpecialKey * specialKey, bool onPending = false);

        void createAddressTranslation(EntityIndex index, PhyAddressTranslation * addressTranslation, bool onPending = false);

        void setSoftwareRevisionInformation(const std::string& softwareRevision);

        std::string getSoftwareRevisionInformation();

        void setDeviceCapability(const PhyDeviceCapability& deviceCapability);

        /**
         * THis function clears all persistent data from a join to another.
         */
        void clearPersistentData();

        /**
         * Move the persistent data from sourceDevice to current instance. The persistent attributes in sourceDevice will become NULL.
         * @param sourceDevice
         */
        void movePersistentData(PhyAttributes& sourcePhyAttributes);
};

} // namespace model
} // namespace NE


#endif /* MODEL_H_ */
