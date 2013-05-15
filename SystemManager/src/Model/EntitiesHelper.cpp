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
 * @author beniamin.tecar, sorin.bidian
 */
#include "EntitiesHelper.h"
#include "Common/Utils/DllUtils.h"
#include "Model/Routing/RouteTypes.h"
#include "Common/ClockSource.h"
#include "Common/SmSettingsLogic.h"
#include "Security/KeyUtils.h"

namespace Isa100 {
namespace Model {

#define MASK_SF_TYPE 0xC0
#define MASK_PRIORITY 0x3C
#define MASK_CH_MAP_OV 0x2

//LOG_DEF("Isa100.Model.EngineProvider");

void marshallEntity(const NE::Model::PhyUint8& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {
    stream.write(entity.value);
}

void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyUint8& entity) {
    stream.read(entity.value);
}

void marshallEntity(const NE::Model::PhyUint16& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {
    stream.write(entity.value);
}

void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyUint16& entity) {
    stream.read(entity.value);
}

void unmarshallEntity(Bytes & value, NE::Model::PhyString & entity) {
    entity.value.assign(value.begin(), value.end());
}

void unmarshallEntity(Bytes & value, NE::Model::PhyBytes & entity) {
    entity.value.assign(value.begin(), value.end());
}

void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyMetaData& entity) {
    stream.read(entity.used);
    stream.read(entity.total);
}

void marshallEntity(const NE::Model::PhyNeighbor& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {

    Isa100::Common::Utils::marshallExtDLUint(stream, entity.index);

    entity.address64.marshall(stream);

    stream.write(entity.groupCode);

    Uint8 octet;
    octet = (entity.clockSource << 6) & 0xC0; // bits 7 and 6
    octet |= (entity.extGrCnt << 4) & 0x30; // bits 5 and 4
    octet |= (entity.diagLevel << 2) & 0x0C; // bits 3 and 2
    octet |= (entity.linkBacklog << 1) & 0x02; // bit 1
    octet &= 0xFE; // bit 0 reserved => we'll set it on 0

    stream.write(octet);

    for (Uint8 i = 0; i < entity.extGrCnt; ++i) {
        octet = entity.extendGraph[i].graph_ID >> 4;
        stream.write(octet);
        octet = (entity.extendGraph[i].graph_ID & 0x000F) << 4;
        octet |= entity.extendGraph[i].lastHop << 3;
        octet |= entity.extendGraph[i].preferredBranch << 2;
        stream.write(octet);
    }

    if (entity.linkBacklog == 1) {
        Isa100::Common::Utils::marshallExtDLUint(stream, entity.linkBacklogIndex);
        stream.write(entity.linkBacklogDur);
    }
}

void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyNeighbor& entity) {
    entity.index = Isa100::Common::Utils::unmarshallExtDLUint(stream);

    entity.address64.unmarshall(stream);

    stream.read(entity.groupCode);

    Uint8 octet;
    stream.read(octet);
    entity.clockSource = (octet & 0xC0) >> 6; // bits 7 and 6
    entity.extGrCnt = (octet & 0x30) >> 4; // bits 5 and 4
    entity.diagLevel = (octet & 0x0E) >> 2; // bits 3 and 2
    entity.linkBacklog = (octet & 0x02) >> 1; // bit 1
    // bit 0 is reserved => we ignore it

    for (Uint8 i = 0; i < entity.extGrCnt; ++i) {
        NE::Model::PhyNeighbor::ExtendGraph entry;
        stream.read(octet);
        entry.graph_ID = octet;

        stream.read(octet);
        entry.graph_ID = (entry.graph_ID << 4) | ((octet & 0xF0) >> 4);
        entry.lastHop = (octet & 0x08) >> 3;
        entry.preferredBranch = (octet & 0x04) >> 2;
        entity.extendGraph.push_back(entry);
    }

    if (entity.linkBacklog == 1) {
        entity.linkBacklogIndex = Isa100::Common::Utils::unmarshallExtDLUint(stream);
        stream.read(entity.linkBacklogDur);
    }
}

void marshallEntity(const NE::Model::PhyRoute& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {

    Isa100::Common::Utils::marshallExtDLUint(stream, entity.index);

    Uint8 octet;
    octet = (entity.route.size() << 4) & 0xF0; // bits 7, 6, 5 and 4
    octet |= (entity.alternative << 2) & 0xC; // bits 3 and 2
    octet &= 0xFC; // bits 1 and 0 are reserved
    stream.write(octet);

    stream.write(entity.forwardLimit);

    for (Uint8 i = 0; i < entity.route.size(); ++i) {
        stream.write(entity.route[i]);
    }

    // When dlmo11a.Route[].alternative=3, dlmo11a.Route[].Selector is null and not transmitted
    if (entity.alternative != NE::Model::Routing::RouteOption::UseDefaultRoute) {
        stream.write(entity.selector);
    }

    if (entity.alternative == NE::Model::Routing::RouteOption::UseBbrContractID) {
        Isa100::Common::Utils::marshallExtDLUint(stream, entity.sourceAddress);
    }
}

void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyRoute& entity) {

    entity.index = Isa100::Common::Utils::unmarshallExtDLUint(stream);

    Uint8 octet;
    stream.read(octet);
    Uint8 routeSize = (octet & 0xF0) >> 4;
    entity.alternative = (octet & 0x0C) >> 2;

    stream.read(entity.forwardLimit);

    for (Uint8 i = 0; i < routeSize; ++i) {
        Uint16 entry;
        stream.read(entry);
        entity.route.push_back(entry);
    }

    if (entity.alternative < 2) {
        stream.read(entity.selector);
    }

    if (entity.alternative == 0) {
        stream.read(entity.sourceAddress);
    } else {
        entity.sourceAddress = 0;
    }
}

void marshallEntity(const NE::Model::PhyNetworkContract& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {
    stream.write(entity.contractID);
    entity.sourceAddress.marshall(stream);
    entity.destinationAddress.marshall(stream);

    // MSB used to match device expectations
    Uint8 contractInfo = ((Uint8)entity.contract_Priority << 6) | ((Uint8)(entity.include_Contract_Flag << 5));
    stream.write(contractInfo);
}

void marshallEntity(const NE::Model::PhyAddressTranslation& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {
    entity.longAddress.marshall(stream);
    stream.write(entity.shortAddress);
}

void marshallEntity(const NE::Model::PhyNetworkRoute& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {
    entity.destination.marshall(stream);
    entity.nextHop.marshall(stream);
    stream.write(entity.nwkHopLimit);
    stream.write(entity.outgoingInterface);
}

void marshallEntity(const NE::Model::PhyLink& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {

    Uint8 octet;
    Isa100::Common::Utils::marshallExtDLUint(stream, entity.index);
    Isa100::Common::Utils::marshallExtDLUint(stream, entity.superframeIndex);
    stream.write(entity.type);
    Isa100::Common::Utils::marshallExtDLUint(stream, entity.template1);
    //Template 2 is transmitted and meaningful only for TRx links
    if ((entity.type & 0x80) && (entity.type & 0x40)) {
        Isa100::Common::Utils::marshallExtDLUint(stream, entity.template2);
    }
    octet = entity.neighborType << 6;
    octet |= entity.graphType << 4;
    octet |= entity.schedType << 2;
    octet |= entity.chType << 1;
    octet |= entity.priorityType;
    stream.write(octet);
    //If dlmo11a.Link[].NeighborType=0, dlmo11a.Link[].Neighbor is null, and not transmitted
    if (entity.neighborType) {
        Isa100::Common::Utils::marshallExtDLUint(stream, entity.neighbor);
    }
    //If GraphType=0, the GraphID element is null and is not transmitted
    if (entity.graphType) {
        Isa100::Common::Utils::marshallExtDLUint(stream, entity.graphID);
    }

    switch (entity.schedType) {
        case 0: {
            Isa100::Common::Utils::marshallExtDLUint(stream, entity.schedule.offset);
            break;
        }
        case 1:
        case 2: {
            Isa100::Common::Utils::marshallExtDLUint(stream, entity.schedule.offset);
            Isa100::Common::Utils::marshallExtDLUint(stream, entity.schedule.interval);
            break;
        }
        case 3: {
            Uint8 octet;
            octet = entity.schedule.interval; // & 0x00FF;
            stream.write(octet);
            octet = entity.schedule.interval >> 8;
            stream.write(octet);
            octet = entity.schedule.offset;
            stream.write(octet);
            octet = entity.schedule.offset >> 8;
            stream.write(octet);
            break;
        }
    }

    //If dlmo11a.Link[].ChType=0, dlmo11a.Link[].ChOffset is null and not transmitted
    if (entity.chType) {
        stream.write(entity.chOffset);
    }
    //If dlmo11a.Link[].PriorityType=0, dlmo11a.Link[].Priority is null and not transmitted
    if (entity.priorityType) {
        stream.write(entity.priority);
    }
}

void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyLink& entity) {

    Uint8 octet;
    entity.index = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    entity.superframeIndex = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    stream.read(entity.type);
    entity.template1 = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    //Template 2 is transmitted and meaningful only for TRx links
    if ((entity.type & 0x80) && (entity.type & 0x40)) {
        entity.template2 = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    }
    stream.read(octet);
    entity.neighborType = (octet & 0xC0) >> 6;
    entity.graphType = (octet & 0x30) >> 4;
    entity.schedType = NE::Model::Tdma::ScheduleType::fromUint8((octet & 0x0C) >> 2);
    entity.chType = (octet & 0x02) >> 1;
    entity.priorityType = NE::Model::Tdma::PriorityType::fromUint8(octet & 0x01);
    //If dlmo11a.Link[].NeighborType=0, dlmo11a.Link[].Neighbor is null, and not transmitted
    if (entity.neighborType) {
        entity.neighbor = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    }
    //If GraphType=0, the GraphID element is null and is not transmitted
    if (entity.graphType) {
        entity.graphID = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    }

    switch (entity.schedType) {
        case 0: {
            entity.schedule.offset = Isa100::Common::Utils::unmarshallExtDLUint(stream);
            break;
        }
        case 1:
        case 2: {
            entity.schedule.offset = Isa100::Common::Utils::unmarshallExtDLUint(stream);
            entity.schedule.interval = Isa100::Common::Utils::unmarshallExtDLUint(stream);
            break;
        }
        case 3: {
            Uint8 octet1, octet2;
            stream.read(octet1);
            stream.read(octet2);
            entity.schedule.interval = ((Uint16)octet2 << 8) | octet1;
            stream.read(octet1);
            stream.read(octet2);
            entity.schedule.offset = ((Uint16)octet2 << 8) | octet1;
            break;
        }
    }

    //If dlmo11a.Link[].ChType=0, dlmo11a.Link[].ChOffset is null and not transmitted
    if (entity.chType) {
        stream.read(entity.chOffset);
    }
    //If dlmo11a.Link[].PriorityType=0, dlmo11a.Link[].Priority is null and not transmitted
    if (entity.priorityType == NE::Model::Tdma::PriorityType::USE_LINK_PRIORITY) {
        stream.read(entity.priority);
    }
}

void marshallEntity(const NE::Model::PhyAdvJoinInfo& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {

    Uint8 firstOctet = (entity.joinBackoff << 4);
    firstOctet |= entity.joinTimeout;
    stream.write(firstOctet);
    Uint8 fidXmit = ((Uint8)entity.txScheduleType << 6);
    fidXmit |= ((Uint8)entity.rxScheduleType << 4);
    fidXmit |= ((entity.sendAdvRx & 1) << 3);
    fidXmit |= ((Uint8)entity.advScheduleType << 1);
    stream.write(fidXmit);

    Isa100::Common::Utils::marshallExtDLUint(stream, entity.joinTx.offset);
    Isa100::Common::Utils::marshallExtDLUint(stream, entity.joinTx.interval);
    Isa100::Common::Utils::marshallExtDLUint(stream, entity.joinRx.offset);
    Isa100::Common::Utils::marshallExtDLUint(stream, entity.joinRx.interval);
    if (entity.sendAdvRx){
        Isa100::Common::Utils::marshallExtDLUint(stream, entity.advRx.offset);
        Isa100::Common::Utils::marshallExtDLUint(stream, entity.advRx.interval);
    }
}

void marshallEntity(const NE::Model::PhySuperframe& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {

    Isa100::Common::Utils::marshallExtDLUint(stream, entity.index);
    stream.write(entity.tsDur);
    Isa100::Common::Utils::marshallExtDLUint(stream, entity.chIndex);
    stream.write(entity.chBirth);

    Uint8 octet = 0;
    octet = ((entity.sfType & 3) << 6) | ((entity.priority & 0xF) << 2) | ((entity.chMapOv & 1) << 1) | (entity.idleUsed & 1);
    stream.write(octet);


    stream.write(entity.sfPeriod);
    stream.write(entity.sfBirth);
    Isa100::Common::Utils::marshallExtDLUint(stream, entity.chRate);

    if (entity.chMapOv == 1) {
        stream.write(entity.chMap);
    }
    if (entity.idleUsed == 1) {
        stream.write(entity.idleTimer);
    }
    if (entity.sfType == NE::Model::SuperframeType::Randomize_Slow_Hop) {
        stream.write(entity.rndSlots);
    }
}

void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhySuperframe& entity) {

    entity.index = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    stream.read(entity.tsDur);
    entity.chIndex = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    stream.read(entity.chBirth);

    Uint8 octet;
    stream.read(octet);
    entity.sfType = (octet & MASK_SF_TYPE) >> 6;
    entity.priority = (octet & MASK_PRIORITY) >> 2;

    //        stream.read(octet);
    entity.chMapOv = (octet & MASK_CH_MAP_OV) >> 1;
    entity.idleUsed = (octet & 1);

    stream.read(entity.sfPeriod);
    stream.read(entity.sfBirth);
    entity.chRate = Isa100::Common::Utils::unmarshallExtDLUint(stream);

    if (entity.chMapOv == 1) {
        stream.read(entity.chMap);
    }
    if (entity.idleUsed == 1) {
        stream.read(entity.idleTimer);
    }
    if (entity.sfType == NE::Model::SuperframeType::Randomize_Slow_Hop) {
        stream.read(entity.rndSlots);
    }
}

void marshallEntity(const NE::Model::PhyGraph& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {

    Isa100::Common::Utils::marshallExtDLUint(stream, entity.index);

    Uint8 octet;
    octet = (entity.preferredBranch << 7) & 0x80; // bit 7
    octet |= (entity.neighborCount << 4) & 0x70; // bits 6, 5 and 4
    octet |= entity.queue & 0x0F ; // bits 3, 2, 1 and 0
    stream.write(octet);

    Isa100::Common::Utils::marshallExtDLUint(stream, entity.maxLifetime);

    for (Uint8 i = 0; i < entity.neighborCount; ++i) {
        Isa100::Common::Utils::marshallExtDLUint(stream, entity.neighbors[i]);
    }
}

void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyGraph& entity) {

    entity.index = Isa100::Common::Utils::unmarshallExtDLUint(stream);

    Uint8 octet;
    stream.read(octet);
    entity.preferredBranch = (octet & 0x80) >> 7;
    entity.neighborCount = (octet & 0x70) >> 4;
    entity.queue = octet & 0x0F;

    entity.maxLifetime = Isa100::Common::Utils::unmarshallExtDLUint(stream);

    for (Uint8 i = 0; i < entity.neighborCount; ++i) {
        Uint16 entry = Isa100::Common::Utils::unmarshallExtDLUint(stream);
        entity.neighbors.push_back(entry);
    }
}

void marshallEntity(const NE::Model::PhyCommunicationAssociationEndpoint& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {
    entity.remoteAddress.marshall(stream);
    stream.write(entity.remotePort);
    stream.write(entity.remoteObjectID);
    stream.write(entity.staleDataLimit);
    stream.write(entity.publicationPeriod);
    stream.write(entity.idealPublicationPhase);
    stream.write(entity.publishAutoRetransmit);
    stream.write(entity.configurationStatus);
}

void marshallEntity(const NE::Model::PhyAlertCommunicationEndpoint& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {
    entity.remoteAddress.marshall(stream);
    stream.write(entity.remotePort);
    stream.write(entity.remoteObjectID);
}

void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyEnergyDesign& entity) {

    stream.read(entity.energyLife);
    entity.listenRate = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    entity.transmitRate = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    entity.advRate = Isa100::Common::Utils::unmarshallExtDLUint(stream);
}

void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyDeviceCapability& entity) {

    entity.queueCapacity = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    stream.read(entity.clockAccuracy);
    stream.read(entity.channelMap);
    stream.read(entity.dlRoles);
    unmarshallEntity(stream, entity.energyDesign);
    stream.read(entity.energyLeft);
    entity.ack_Turnaround = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    entity.neighborDiagCapacity = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    stream.read(entity.radioTransmitPower);
    stream.read(entity.options);
}

void marshallEntity(const NE::Model::PhyQueuePriority & entity, NE::Misc::Marshall::NetworkOrderStream & stream) {

	stream.write((Uint8)entity.value.size());

	for (NE::Model::QueuePriorityEntries::const_iterator it = entity.value.begin(); it != entity.value.end(); ++it) {
		stream.write(it->priority);
		stream.write(it->qMax);
	}
}

void marshallEntity(const NE::Model::Policy& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {

    Uint8 keyTypeUsageValue = (entity.Granularity & 0x03) | ((entity.KeyUsage & 0x07) << 2) | ((entity.KeyType & 0x07) << 5);
    stream.write(keyTypeUsageValue);

    // NotValidBefore has to be normalized to granularity when setting key on device
    Uint32 notValidBefore = entity.NotValidBefore / Isa100::Security::KeyUtils::GetGranularityMultiplier(entity.Granularity);

    switch (entity.Granularity & 0x03)
    {
    case 0x00:  // second
    {
        stream.write(notValidBefore);
        stream.write(entity.KeyHardLifetime);
        break;
	}
    case 0x01:  // minute
    {
        stream.write(notValidBefore);
        stream.write((Uint8)((entity.KeyHardLifetime & 0xFF0000) >> 16));
        stream.write((Uint8)((entity.KeyHardLifetime & 0xFF00) >> 8));
        stream.write((Uint8)(entity.KeyHardLifetime & 0xFF));
        break;
    }
    case 0x02: // hour
    {
        stream.write((Uint8)((notValidBefore & 0xFF0000) >> 16));
        stream.write((Uint8)((notValidBefore & 0xFF00) >> 8));
        stream.write((Uint8)(notValidBefore & 0xFF));
        stream.write((Uint8)((entity.KeyHardLifetime & 0xFF00) >> 8));
        stream.write((Uint8)(entity.KeyHardLifetime & 0xFF));
        break;
	}
    case 0x03:  // day
    {
        stream.write((Uint8)((notValidBefore & 0xFF00) >> 8));
        stream.write((Uint8)(notValidBefore & 0xFF));
        stream.write((Uint8)((entity.KeyHardLifetime & 0xFF00) >> 8));
        stream.write((Uint8)(entity.KeyHardLifetime & 0xFF));
        break;
	}
    default: break;
    }

    stream.write(entity.PolicyPolicy);
}

void marshallEntity(const Isa100::Security::SecurityKeyAndPolicies& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {

    marshallEntity(entity.keyPolicy, stream);

    //for DL and master keys the following fields are not sent
    if (entity.keyPolicy.KeyUsage == 0x01) {
        stream.write((Uint16)entity.endPortSource);
        entity.EUI64Remote.marshall(stream);
        entity.remoteAddress128.marshall(stream);
        stream.write((Uint16)entity.endPortRemote);
    }

    stream.write(entity.algorithmIdentifier);
    stream.write(entity.securityControl);
    stream.write(entity.keyIdentifier);
    stream.write(entity.timeStamp);
    stream.write(entity.newKeyID);

    entity.keyMaterial.marshall(stream);

    Uint8 sizeMic = sizeof(entity.MIC)/sizeof(Uint8);
    for(std::size_t i = 0; i < sizeMic; ++i) {
        stream.write(entity.MIC[i]);
    }
}

void marshallEntity(const Isa100::Security::SecurityDeleteKeyReq& entity, NE::Misc::Marshall::NetworkOrderStream& stream) {

    stream.write(entity.keyType);
    stream.write(entity.masterKeyID);
    stream.write(entity.keyID);
    stream.write(entity.sourcePort);
    entity.destinationAddress.marshall(stream);
    stream.write(entity.destinationPort);
    stream.write(entity.nonce);

    Uint8 sizeMic = sizeof(entity.mic)/sizeof(Uint8);
    for(std::size_t i = 0; i < sizeMic; ++i) {
        stream.write(entity.mic[i]);
    }
}


}
}
