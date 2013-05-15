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
 * Superframe.h
 *
 *  Created on: Mar 24, 2009
 *      Author: Catalin Pop
 */

#ifndef TDMATYPES_H_
#define TDMATYPES_H_

#include <set>
#include <map>
#include "Common/NETypes.h"


#define SUPERFRAME_APPLICATION_LENGTH 3000


//#define SLOTS_PER_250ms 25
//#define SLOTS_PER_SECOND (4*SLOTS_PER_250ms)

#define GROUP_NEIGHBOR_JOIN 1

namespace NE {

namespace Model {

namespace Tdma {

#define AppSlotsLength 3000
#define INVALID_FREE_SLOT 0xFFFFFFFF

//#define FreqCount 14

/**
 * Uint16 - keeps the frequencies occupied by that slot.
 * Even slots (0, 2, 4, … , 2998) are reserved for APP usage.
 */
typedef unsigned short AppSlots[AppSlotsLength];

inline std::string AppSlots_toString(AppSlots & appSlots) {
    std::ostringstream stream;
    for(Uint32 currSlot = 0; currSlot < AppSlotsLength; currSlot += 2) {
        stream << std::hex << appSlots[currSlot] << ", ";
    }
    return stream.str();
}

/**
 * Uint12 keeps the slot; Uint4 keeps the frequency.
 */
typedef Uint32 SlotFreq;



static const Uint8 Hopping_Sequence_1[] = { 8, 1, 9, 13, 5, 12, 7, 14, 3, 10, 0, 4, 11, 6, 2, 15 };

typedef Uint16 LinkId;

namespace JoinTimeout {
/*
 * Enumeration used for joinTimeout in seconds. The value is used as power of 2. TIMEOUT_32 = 5 means 2^5=32 seconds.
 */
enum JoinTimeoutEnum {
    TIMEOUT_00 = 0,
    TIMEOUT_02 = 1,
    TIMEOUT_04 = 2,
    TIMEOUT_08 = 3,
    TIMEOUT_16 = 4,
    TIMEOUT_32 = 5,
    TIMEOUT_64 = 6,
    TIMEOUT_128 = 7,
    TIMEOUT_256 = 8,
    TIMEOUT_512 = 9,
    TIMEOUT_1024 = 10,
    TIMEOUT_2048 = 11,
    TIMEOUT_4096 = 12,
    TIMEOUT_8192 = 13,
    TIMEOUT_16384 = 14,
    TIMEOUT_32768 = 15
};

} // namespace JoinTimeout

namespace EntityStatus {
/** This enum should hold the status of modification of the entity */
enum EntityStatusEnum {
    NEW = 1, MODIFIED = 2, DELETED = 3, MARKED_FOR_REMOVE = 4
};
inline std::string toString(NE::Model::Tdma::EntityStatus::EntityStatusEnum type) {
    switch (type) {
        case NE::Model::Tdma::EntityStatus::NEW:
            return "N";
        case NE::Model::Tdma::EntityStatus::MODIFIED:
            return "M";
        case NE::Model::Tdma::EntityStatus::DELETED:
            return "D";
        case NE::Model::Tdma::EntityStatus::MARKED_FOR_REMOVE:
            return "R";
        default:
            return "??";
    }
}
}

namespace ConfirmStatus {
/** This enum should hold the status of the confirmation(from devices) of an entity.*/
enum ConfirmStatusEnum {
    GENERATED = 1, SENT = 2, CONFIRMED = 3, TIMEOUT = 4, ERROR = 5, IGNORED = 6
};
inline std::string toString(NE::Model::Tdma::ConfirmStatus::ConfirmStatusEnum type) {
    switch (type) {
        case NE::Model::Tdma::ConfirmStatus::GENERATED:
            return "G";
        case NE::Model::Tdma::ConfirmStatus::SENT:
            return "S";
        case NE::Model::Tdma::ConfirmStatus::CONFIRMED:
            return "C";
        case NE::Model::Tdma::ConfirmStatus::TIMEOUT:
            return "T";
        case NE::Model::Tdma::ConfirmStatus::ERROR:
            return "E";
        default:
            return "??";
    }
}
}

namespace TdmaLinkDir {
enum TdmaLinkDirEnum {
    INBOUND  = 0,
    OUTBOUND = 1
};
}

namespace TdmaLinkTypes {

enum TdmaLinkTypesEnum {
    DEFAULT = 1,
    JOIN = 2,
    APPLICATION = 3,
    MANAGEMENT = 4,
    NEIGHBOR_DISCOVERY = 5,
    MANAGEMENT_ACC = 6,
    APPLICATION_BACKUP = 7,
    UDO_FIRMWARE = 8
};

inline bool isAppLink( NE::Model::Tdma::TdmaLinkTypes::TdmaLinkTypesEnum linkRole ) {
	return (linkRole == APPLICATION || linkRole == APPLICATION_BACKUP);
}

inline std::string toString(NE::Model::Tdma::TdmaLinkTypes::TdmaLinkTypesEnum type) {
    switch (type) {
        case NE::Model::Tdma::TdmaLinkTypes::DEFAULT:
            return "D";
        case NE::Model::Tdma::TdmaLinkTypes::JOIN:
            return "J";
        case NE::Model::Tdma::TdmaLinkTypes::APPLICATION:
            return "A";
        case NE::Model::Tdma::TdmaLinkTypes::MANAGEMENT:
            return "M";
        case NE::Model::Tdma::TdmaLinkTypes::NEIGHBOR_DISCOVERY:
            return "N";
        case NE::Model::Tdma::TdmaLinkTypes::MANAGEMENT_ACC:
            return "!";
        case NE::Model::Tdma::TdmaLinkTypes::APPLICATION_BACKUP:
            return "a";
        case NE::Model::Tdma::TdmaLinkTypes::UDO_FIRMWARE:
            return "U";
        default:
            return "??";
    }
}

} // namespace LinkTypes


namespace ScheduleType {

enum ScheduleTypeEnum {
    OFFSET = 0, OFFSET_AND_INTERVAL = 1, RANGE = 2, BITMAP = 3
};
inline ScheduleTypeEnum fromUint8(Uint8 value) {
    switch (value) {
        case OFFSET:
            return OFFSET;
        case OFFSET_AND_INTERVAL:
            return OFFSET_AND_INTERVAL;
        case RANGE:
            return RANGE;
        case BITMAP:
            return BITMAP;
        default:
            return OFFSET;
    }
}

inline
std::string toString(ScheduleType::ScheduleTypeEnum scheduleType) {
    switch (scheduleType) {
        case OFFSET:
            return "Offset";
        case OFFSET_AND_INTERVAL:
            return "Off&Int";
        case RANGE:
            return "Range";
        case BITMAP:
            return "Bitmap";
        default:
            return "Offset";
    }
    return "Unknown";
}

} // namespace ScheduleType


namespace ChannelHopping {
//Represents the index of default Channels Hopping entries from draft.
enum ChannelHopping {
    HOPPING_1 = 1,//channels: 8,1,9,13,5,12,7,14,3,10,0,4,11,6,2,15
    HOPPING_2 = 2,//channels: HOPPING_1 in reverse
    HOPPING_3 = 3,//channels: 4,9,14
    HOPPING_4 = 4,//channels: 14,9,4
    HOPPING_5 = 5,//channels: 4,7,11,8,5,10,6,3,12,9
    HOPPING_6 = 6,//channels: HOPPING_5 in reverse
    HOPPING_7 = 7,
    HOPPING_8 = 8,
    HOPPING_9 = 9
//channels: 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15

};
}

namespace SuperframePriority {

enum SuperframePriorityEnum {
    PRIOR_0 = 0, //LOWEST
    PRIOR_1 = 1,
    PRIOR_2 = 2,
    PRIOR_3 = 3,
    PRIOR_4 = 4,
    PRIOR_5 = 5,
    PRIOR_6 = 6,
    PRIOR_7 = 7,
    PRIOR_8 = 8,
    PRIOR_9 = 9,
    PRIOR_10 = 10,
    PRIOR_11 = 11,
    PRIOR_12 = 12,
    PRIOR_13 = 13,
    PRIOR_14 = 14,
    PRIOR_15 = 15
//HEIGHES
};

} // namespace SuperframePriority

namespace LinkType {
/**
 * Enumeration with possible LinkType values.
 * Supported combinations include T, TB, TR, TRB, R, D.
 * See dlmo11a.Link[].Type structure.
 */
enum LinkTypesEnum {
    TRANSMIT = 128, //0x80 (T)
    TRANSMIT_BACKOFF = 160, //0xA0 (TB)
    TRANSMIT_BACKOFF_ADAPTIVE = 161, //0xA1 (TB)
    TRANSMIT_RECEIVE = 192, //0xC0 (TR)
    TRANSMIT_RECEIVE_ADAPTIVE = 193, //0xC1 (TR)
    TRANSMIT_RECEIVE_BACKOFF = 224, //0xE0 (TRB)
    RECEIVE = 64, //0x40 (R)
    RECEIVE_ADAPTIVE = 65, //0x41 (RA)
    RECEIVE_IDLE_ADAPTIVE = 81, //0x51 (RIA)
    DELEGATED = 16, //0x10 (D)
    NONE = 0,
    TRANSMIT_BURST_ADVERTISEMENT = 136, //0x88
    ADVERTISEMENT = 132, //0x84 Transmit + Advertise
    BURST_ADVERTISEMENT = 8,
    SOLICITATION = 12, //0x0C
    JOIN_RESPONSE = 130, //0xA2 Transmit + Join Response
    JOIN_RESPONSE_BACKOFF = 162, //0xA2 Transmit + Join Response + BACKOFF
    ADAPTIVE_ALLOWED = 1
};
inline
std::string toString(NE::Model::Tdma::LinkType::LinkTypesEnum value){
    switch(value){
        case TRANSMIT : return "T";
        case TRANSMIT_BACKOFF : return "TB";
        case TRANSMIT_BACKOFF_ADAPTIVE : return "TBA";
        case TRANSMIT_RECEIVE : return "TR";
        case TRANSMIT_RECEIVE_ADAPTIVE : return "TRA";
        case TRANSMIT_RECEIVE_BACKOFF : return "TRB";
        case RECEIVE : return "R";
        case RECEIVE_ADAPTIVE : return "RA";
        case RECEIVE_IDLE_ADAPTIVE : return "RIA";
        case DELEGATED : return "D";
        case NONE : return "-";
        case TRANSMIT_BURST_ADVERTISEMENT : return "TBADV";
        case ADVERTISEMENT : return "ADV";
        case BURST_ADVERTISEMENT : return "BADV";
        case SOLICITATION : return "S";
        case JOIN_RESPONSE : return "JR";
        case JOIN_RESPONSE_BACKOFF : return "JRB";
        case ADAPTIVE_ALLOWED : return "AA";
        default : return "?";
    }
}
}  // namespace PhyLinkType

namespace TemplatesTypes {

enum TemplatesTypesEnum {
    TEMPLATE_RECEIVE = 0x01, TEMPLATE_TRANSMIT = 0x02, TEMPLATE_RECEIVE_SCANING = 0x03,
};
inline TemplatesTypesEnum fromLinkType(LinkType::LinkTypesEnum value) {
    switch (value) {
        case LinkType::TRANSMIT : return TemplatesTypes::TEMPLATE_TRANSMIT;
        case LinkType::TRANSMIT_BACKOFF : return TemplatesTypes::TEMPLATE_TRANSMIT;
        case LinkType::TRANSMIT_BACKOFF_ADAPTIVE : return TemplatesTypes::TEMPLATE_TRANSMIT;
        case LinkType::TRANSMIT_RECEIVE : return TemplatesTypes::TEMPLATE_TRANSMIT;
        case LinkType::TRANSMIT_RECEIVE_ADAPTIVE : return TemplatesTypes::TEMPLATE_TRANSMIT;
        case LinkType::TRANSMIT_RECEIVE_BACKOFF : return TemplatesTypes::TEMPLATE_TRANSMIT;
        case LinkType::RECEIVE : return TemplatesTypes::TEMPLATE_RECEIVE;
        case LinkType::RECEIVE_ADAPTIVE : return TemplatesTypes::TEMPLATE_RECEIVE;
        case LinkType::RECEIVE_IDLE_ADAPTIVE : return TemplatesTypes::TEMPLATE_RECEIVE;
        case LinkType::DELEGATED : return TemplatesTypes::TEMPLATE_TRANSMIT;
        case LinkType::NONE : return TemplatesTypes::TEMPLATE_TRANSMIT;
        case LinkType::TRANSMIT_BURST_ADVERTISEMENT : return TemplatesTypes::TEMPLATE_TRANSMIT;
        case LinkType::ADVERTISEMENT : return TemplatesTypes::TEMPLATE_TRANSMIT;
        case LinkType::BURST_ADVERTISEMENT : return TemplatesTypes::TEMPLATE_TRANSMIT;
        case LinkType::SOLICITATION : return TemplatesTypes::TEMPLATE_TRANSMIT;
        case LinkType::JOIN_RESPONSE : return TemplatesTypes::TEMPLATE_TRANSMIT;
        case LinkType::ADAPTIVE_ALLOWED : return TemplatesTypes::TEMPLATE_TRANSMIT;
        default:
            return TemplatesTypes::TEMPLATE_RECEIVE;
    }
}

} // namespace TemplatesTypes

namespace NeighbourTypes {

enum NeighbourTypesEnum {
    NONE = 0, NEIGHBOUR_BY_ADDRESS = 1, NEIGHBOUR_BY_GROUP = 2
};
inline NeighbourTypesEnum fromInt(int value) {
    switch (value) {
        case NEIGHBOUR_BY_ADDRESS:
            return NEIGHBOUR_BY_ADDRESS;
        case NEIGHBOUR_BY_GROUP:
            return NEIGHBOUR_BY_GROUP;
        default:
            return NONE;
    }
}
} // namespace NeighborTypes

namespace GraphTypes {

enum GraphTypesEnum {
    NONE = 0, SPECIFIC_GRAPH = 1, SPECIFIC_GRAPH_WITH_PRIORITY = 2
};
inline GraphTypesEnum fromInt(int value) {
    switch (value) {
        case NONE:
            return NONE;
        case SPECIFIC_GRAPH:
            return SPECIFIC_GRAPH;
        case SPECIFIC_GRAPH_WITH_PRIORITY:
            return SPECIFIC_GRAPH_WITH_PRIORITY;
        default:
            return NONE;
//            throw NE::Common::NEException("Illegal graph type option in Link:" + (int) value);
    }
}

} // namespace GraphTypes


namespace ChannelOffsetType {

enum ChannelOffsetTypeEnum {
    NONE = 0, USE_CHANNEL_OFFSET = 1
};
inline ChannelOffsetTypeEnum fromInt(int value) {
    switch (value) {
        case USE_CHANNEL_OFFSET:
            return USE_CHANNEL_OFFSET;
        default:
            return NONE;
    }
}
} // namespace ChannelOffsetType

namespace PriorityType {

enum PriorityTypeEnum {
    USE_SUPERFRAME_PRIORITY = 0, USE_LINK_PRIORITY = 1
};
inline PriorityTypeEnum fromUint8(Uint8 value) {
    switch (value) {
        case USE_LINK_PRIORITY:
            return USE_LINK_PRIORITY;
        default:
            return USE_SUPERFRAME_PRIORITY;
    }
}

} // namespace PriorityType


typedef std::set<Uint16> AllocatedRoutes;
typedef std::set<Uint16> RoutesIdSet;
typedef std::set<Uint16> ContractsIdSet;

} // namespace Tdma

} // namespace Model

} // namespace Isa100

#endif /* TDMATYPES_H_ */
