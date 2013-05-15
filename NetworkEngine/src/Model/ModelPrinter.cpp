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
 * @author radu.pop, catalin.pop, beniamin.tecar
 */
#include "ModelPrinter.h"
#include "Model/model.h"
#include <iomanip>
#include "Misc/Convert/Convert.h"
#include "Model/Tdma/TdmaTypes.h"

namespace NE {
namespace Model {

std::ostream& operator<<(std::ostream& stream, const PhyUint8& entity) {
    stream << std::hex << (int) entity.value;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyUint16& entity) {
    stream << std::hex << (int) entity.value;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyString& entity) {
    stream << entity.value;
    return stream;
}

std::ostream& operator<<(std::ostream & stream, const PhyBytes & entity) {
    stream << bytes2string(entity.value);
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const Schedule& entity) {
    stream << "[O=" << std::dec << (int) entity.offset << ", I=" << (int) entity.interval << "]";
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyAdvJoinInfo& entity) {
    stream << "joinBackoff=" << std::hex << (int) entity.joinBackoff << ", joinTimeout=" << (int) entity.joinTimeout
                << ", txScheduleType=" << Tdma::ScheduleType::toString(entity.txScheduleType) << ", rxScheduleType="
                << Tdma::ScheduleType::toString(entity.rxScheduleType) << ", sendAdvRx=" << entity.sendAdvRx
                << ", advScheduleType=" << Tdma::ScheduleType::toString(entity.advScheduleType) << ", joinTx=" << entity.joinTx
                << ", joinRx=" << entity.joinRx << ", advRx=" << entity.advRx;

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyMetaData& entity) {
    stream << "used=" << std::hex << (int) entity.used;
    stream << ", total=" << std::hex << (int) entity.total;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyNeighbor::ExtendGraph& entity) {
    stream << "graph_ID=" << std::hex << (int) entity.graph_ID << ", lastHop=" << (int) entity.lastHop << ", preferredBranch="
                << (int) entity.lastHop;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyNeighbor& entity) {
    stream << "index=" << std::hex << (int) entity.index << ", address64=" << entity.address64.toString() << ", groupCode="
                << (int) entity.groupCode << ", clockSource=" << (int) entity.clockSource << ", extGrCnt="
                << (int) entity.extGrCnt << ", diagLevel=" << (int) entity.diagLevel << ", linkBacklog="
                << (int) entity.linkBacklog;

    stream << ", extendGraph={";
    for (std::vector<PhyNeighbor::ExtendGraph>::const_iterator it = entity.extendGraph.begin(); it != entity.extendGraph.end(); ++it) {
        stream << "{" << *it << "}, ";
    }
    stream << "}";

    stream << ", linkBacklogIndex=" << (int) entity.linkBacklogIndex << ", linkBacklogDur=" << (int) entity.linkBacklogDur;

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyRoute& entity) {
    stream << "index=" << std::hex << (int) entity.index << ", size=" << (int) entity.route.size() << ", evaluationTime="
                << (int) entity.evaluationTime << ", alternative="
                << (int) entity.alternative << ", forwardLimit=" << (int) entity.forwardLimit;

    stream << ", route={";
    for (std::vector<Uint16>::const_iterator it = entity.route.begin(); it != entity.route.end(); ++it) {
        stream << *it << ", ";
    }
    stream << "}";

    stream << ", selector=" << (int) entity.selector;
    stream << ", srcAddr=" << Address_toStream(entity.sourceAddress);

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyGraph& entity) {
    stream << "index=" << std::hex << (int) entity.index << ", preferredBranch=" << (int) entity.preferredBranch
                << ", neighborCount=" << (int) entity.neighborCount << ", queue=" << (int) entity.queue << ", maxLifetime="
                << (int) entity.maxLifetime;

    stream << ", neighbors={";
    for (std::vector<Uint16>::const_iterator it = entity.neighbors.begin(); it != entity.neighbors.end(); ++it) {
        stream << Address_toStream(*it) << ", ";
    }
    stream << "}";

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyLink& entity) {
    stream << "index=" << std::hex << (int) entity.index << ", sfIndex=" << (int) entity.superframeIndex << ", type="
                << std::setw(5) << NE::Model::Tdma::LinkType::toString((NE::Model::Tdma::LinkType::LinkTypesEnum) entity.type)
                << ", T1=" << (int) entity.template1 << ", T2=" << (int) entity.template2 << ", Role="
                << NE::Model::Tdma::TdmaLinkTypes::toString((NE::Model::Tdma::TdmaLinkTypes::TdmaLinkTypesEnum) entity.role)
                << ", nType=" << (int) entity.neighborType << ", neighbor=" << (int) entity.neighbor << ", sType="
                << Tdma::ScheduleType::toString(entity.schedType) << ", schedule=" << entity.schedule << ", chType="
                << (int) entity.chType << ", ch=" << (int) entity.chOffset << ", gType=" << (int) entity.graphType
                << ", graphID=" << (int) entity.graphID << ", pType=" << (int) entity.priorityType << ", priority="
                << (int) entity.priority << ", dir=" << (entity.direction == NE::Model::Tdma::TdmaLinkDir::INBOUND ? "IN" : "OUT");

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhySuperframe& entity) {
    stream << "index=" << std::hex << (int) entity.index << ", tsDur=" << (int) entity.tsDur << ", chIndex="
                << (int) entity.chIndex << ", chBirth=" << (int) entity.chBirth << ", sfType=" << (int) entity.sfType
                << ", priority=" << (int) entity.priority << ", chMapOv=" << (int) entity.chMapOv << ", idleUsed="
                << (int) entity.idleUsed << ", sfPeriod=" << (int) entity.sfPeriod << ", sfBirth=" << (int) entity.sfBirth
                << ", chRate=" << (int) entity.chRate << ", chMap=" << (int) entity.chMap << ", idleTimer="
                << (int) entity.idleTimer << ", rndSlots=" << (int) entity.rndSlots;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyNetworkRoute& entity) {
    stream << "index=" << std::hex << (int) entity.networkRouteID << ", destination=" << entity.destination.toString()
                << ", nextHop=" << entity.nextHop.toString() << ", nwkHopLimit=" << (int) entity.nwkHopLimit
                << ", outgoingInterface=" << (int) entity.outgoingInterface;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyAddressTranslation& entity) {
    stream << "index=" << std::hex << (int) entity.addressTranslationID << ", longAddress=" << entity.longAddress.toString()
                << ", shortAddress=" << Address_toStream(entity.shortAddress);
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyContract& entity) {
    stream << "contractID=" << std::hex << (int) entity.contractID << ", reqID=" << (int) entity.requestID << ", srcSAP="
                << (int) entity.sourceSAP << ", src32=" << Address::toString(entity.source32) << ", destSAP="
                << (int) entity.destinationSAP << ", dest32=" << Address::toString(entity.destination32)
                << ", responseCode=" << (int) entity.responseCode << ", communicationServiceType="
                << CommunicationServiceType::toString(entity.communicationServiceType) << ", contractActivationTime="
                << (int) entity.contract_Activation_Time << ", Life=" << (int) entity.assigned_Contract_Life
                << std::dec
                << ", Priority=" << (int) entity.assigned_Contract_Priority << ", Max_NSDU_Size="
                << (int) entity.assigned_Max_NSDU_Size << ", Reliability_And_PublishAutoRetransmit="
                << (int) entity.assigned_Reliability_And_PublishAutoRetransmit << ", Period=" << (int) entity.assignedPeriod
                << ", a_Phase=" << (int) entity.assignedPhase << ", a_Deadline=" << (int) entity.assignedDeadline
                << ", a_CommittedBurst=" << (int) entity.assignedCommittedBurst << ", a_ExcessBurst="
                << (int) entity.assignedExcessBurst << ", a_Max_Send_Window_Size=" << (int) entity.assigned_Max_Send_Window_Size
                << ", isManagement=" << entity.isManagement;

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyNetworkContract& entity) {
    stream << "contractID=" << std::hex << (int) entity.contractID << ", sourceAddress=" << entity.sourceAddress.toString()
                << ", destinationAddress=" << entity.destinationAddress.toString() << ", contract_Priority="
                << (int) entity.contract_Priority << ", include_Contract_Flag=" << (int) entity.include_Contract_Flag;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyChannelHopping& entity) {
    stream << "index=" << std::hex << (int) entity.index << ", length=" << (int) entity.length;

    stream << ", seq={";
    for (std::vector<Uint8>::const_iterator it = entity.seq.begin(); it != entity.seq.end(); ++it) {
        stream << (int) (*it) << ", ";
    }
    stream << "}";

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyBlacklistChannels& entity) {
    stream << ", seq={";
    for (std::vector<Uint8>::const_iterator it = entity.seq.begin(); it != entity.seq.end(); ++it) {
        stream << (int) (*it) << ", ";
    }
    stream << "}";

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyChannelDiag::ChannelTransmission& entity) {
    stream << "[noAck=" << (int) entity.noAck << "ccaBackoff=" << (int) entity.ccaBackoff << "]";
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyChannelDiag& entity) {
    stream << "count=" << (int) entity.count;

    stream << ", channelTransmissionList={";
    for (std::vector<PhyChannelDiag::ChannelTransmission>::const_iterator it = entity.channelTransmissionList.begin(); it
                != entity.channelTransmissionList.end(); ++it) {
        stream << "{" << *it << "}, ";
    }
    stream << "}";

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyCandidate& entity) {
    stream << "neighbor=" << Address_toStream(entity.neighbor) << ", RSQI=" << (int) entity.radio;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhySessionKey& entity) {
    stream << "keyID=" << (int) entity.keyID << ", index=" << (int) entity.index << ", destination=" << entity.destination64.toString() << ", source="
                << entity.source64.toString() << ", destinationTSAP=" << (int) entity.destinationTSAP << ", sourceTSAP="
                << (int) entity.sourceTSAP << ", key=" << entity.key.toString() << ", sessionKeyPolicy="
                << entity.sessionKeyPolicy.toString() << ", softLifeTime=" << (int) entity.softLifeTime << ", hardLifeTime="
                << (int) entity.hardLifeTime << ", markedAsExpiring=" << entity.markedAsExpiring;

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhySpecialKey& entity) {
    stream << "keyID=" << (int) entity.keyID << ", key=" << entity.key.toString()
                << ", softLifeTime=" << (int) entity.softLifeTime << ", hardLifeTime=" << (int) entity.hardLifeTime
                << ", policy=" << entity.policy.toString() << ", markedAsExpiring=" << entity.markedAsExpiring;

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyCommunicationAssociationEndpoint& entity) {
    stream << "remoteAddress=" << entity.remoteAddress.toString() << ", remotePort=" << (int) entity.remotePort
                << ", remoteObjectID=" << (int) entity.remoteObjectID << ", staleDataLimit=" << (int) entity.staleDataLimit
                << ", publicationPeriod=" << (int) entity.publicationPeriod << ", idealPublicationPhase="
                << (int) entity.idealPublicationPhase << ", publishAutoRetransmit=" << (int) entity.publishAutoRetransmit
                << ", configurationStatus=" << (int) entity.configurationStatus;

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyAlertCommunicationEndpoint& entity) {
    stream << "remoteAddress=" << entity.remoteAddress.toString() << ", remotePort=" << (int) entity.remotePort
                << ", remoteObjectID=" << (int) entity.remoteObjectID;

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyEntityIndexList& entity) {
    stream << "value={";
    for (std::list<EntityIndex>::const_iterator it = entity.value.begin(); it != entity.value.end(); ++it) {
        stream << "{" << std::hex << *it << "}, ";
    }
    stream << "}";

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyEnergyDesign& entity) {
    stream << "energyLife=" << (int) entity.energyLife << ", listenRate=" << (int) entity.listenRate << ", transmitRate="
                << (int) entity.transmitRate << ", advRate=" << (int) entity.advRate;

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyDeviceCapability& entity) {
    stream << "queueCapacity=" << (int) entity.queueCapacity << ", clockAccuracy=" << (int) entity.clockAccuracy
                << ", channelMap=" << (int) entity.channelMap << ", dlRoles=" << (int) entity.dlRoles << ", energyDesign={ "
                << entity.energyDesign << " }" << ", energyLeft=" << (int) entity.energyLeft << ", ack_Turnaround="
                << (int) entity.ack_Turnaround << ", neighborDiagCapacity=" << (int) entity.neighborDiagCapacity
                << ", radioTransmitPower=" << (int) entity.radioTransmitPower << ", options=" << (int) entity.options;

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const QueuePriorityEntry & entity) {

	stream << (int)entity.priority << ", " << (int)entity.qMax;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyQueuePriority & entity) {

	stream << "count=" << (int)entity.value.size();
	for (QueuePriorityEntries::const_iterator it = entity.value.begin(); it != entity.value.end(); ++it) {
		stream << "{" << *it << "}, ";
	}
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const AlertsTable& entity) {
    stream << "neighborDiscoveryAlerts size=" << (int)entity.neighborDiscoveryAlerts.size()
        << ", channelDiagAlerts size=" << (int)entity.channelDiagAlerts.size()
        << ", neighborDiagAlerts size=" << (int)entity.neighborDiagAlerts.size()
        << ", malformedAPDUAlerts size=" << (int)entity.malformedAPDUAlerts.size()
        << ", powerStatusAlerts size=" << (int)entity.powerStatusAlerts.size();
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhyAttributes& entity) {
    stream << std::endl;
    stream << "advInfo {" << entity.advInfo << "}" << std::endl;
    stream << "advSuperframe {" << entity.advSuperframe << "}" << std::endl;
    stream << "powerSupplyStatus {" << entity.powerSupplyStatus << "}" << std::endl;
    stream << "vendorID {" << entity.vendorID << "}" << std::endl;
    stream << "modelID {" << entity.modelID << "}" << std::endl;
    stream << "softwareRevisionInformation {" << entity.softwareRevisionInformation << "}" << std::endl;

    stream << "packagesStatistics {" << entity.packagesStatistics << "}" << std::endl;

    stream << "joinReason {" << entity.joinReason << "}" << std::endl;
    stream << "contractsTableMetadata {" << entity.contractsTableMetadata << "}" << std::endl;
    stream << "neighborMetadata {" << entity.neighborMetadata << "}" << std::endl;
    stream << "superframeMetadata {" << entity.superframeMetadata << "}" << std::endl;
    stream << "graphMetadata {" << entity.graphMetadata << "}" << std::endl;
    stream << "linkMetadata {" << entity.linkMetadata << "}" << std::endl;
    stream << "routeMetadata {" << entity.routeMetadata << "}" << std::endl;
    stream << "diagMetadata {" << entity.diagMetadata << "}" << std::endl;

    stream << "HrcoCommEndpoint {" << entity.hrcoCommEndpoint << "}" << std::endl;
    stream << "HrcoObjectAttributes {" << entity.hrcoEntityIndexListAttribute << "}" << std::endl;

    stream << "deviceCapability {" << entity.deviceCapability << "}" << std::endl;

    stream << "ArmoCommEndpoint {" << entity.armoCommEndpoint << "}" << std::endl;

    stream << "retryTimeout {" << entity.retryTimeout << "}" << std::endl;
    stream << "max_Retry_Timeout_Interval {" << entity.max_Retry_Timeout_Interval << "}" << std::endl;
    stream << "retryCount {" << entity.retryCount << "}" << std::endl;

    stream << "dlmoMaxLifeTime {" << entity.dlmoMaxLifeTime << "}" << std::endl;

    stream << "dlmoIdleChannels {" << entity.dlmoIdleChannels << "}" << std::endl;
    stream << "dlmoDiscoveryAlert {" << entity.dlmoDiscoveryAlert << "}" << std::endl;

    stream << "BlacklistChannelsIndexedAttribute {" << entity.blacklistChannelsTable << "}" << std::endl;
    stream << "ChannelHoppingIndexedAttribute {" << entity.channelHoppingTable << "}" << std::endl;
    stream << "LinksTable {" << entity.linksTable << "}" << std::endl;
    stream << "SuperframesTable {" << entity.superframesTable << "}" << std::endl;
    stream << "NeighborsTable {" << entity.neighborsTable << "}" << std::endl;
    stream << "CandidatesTable {" << entity.candidatesTable << "}" << std::endl;
    stream << "RoutesTable {" << entity.routesTable << "}" << std::endl;
    stream << "NetworkRoutesTable {" << entity.networkRoutesTable << "}" << std::endl;
    stream << "GraphsTable {" << entity.graphsTable << "}" << std::endl;
    stream << "ContractsTable {" << entity.contractsTable << "}" << std::endl;
    stream << "NetworkContractsTable {" << entity.networkContractsTable << "}" << std::endl;
    stream << "SessionKeysTable {" << entity.sessionKeysTable << "}" << std::endl;
    stream << "MasterKeysTable {" << entity.masterKeysTable << "}" << std::endl;
    stream << "SubnetKeysTable {" << entity.subnetKeysTable << "}" << std::endl;
    stream << "AddressTranslationTable {" << entity.addressTranslationTable << "}" << std::endl;
    //stream << "AlertsTable {" << entity.alertsTable << "}" << std::endl;

    return stream;
}

void logEnityLevel(std::ostream& stream, const LevelPrinterPhyAttributes& printer) {
    PhyAttributes& entity = printer.phyAttributes;
    stream << std::endl;
    switch(printer.level.entityType) {
        case EntityType::Link:
            stream << "LinksTable {" << entity.linksTable << "}" << std::endl;
            break;
        case EntityType::Superframe:
            stream << "SuperframesTable {" << entity.superframesTable << "}" << std::endl;
            break;
        case EntityType::Neighbour:
            stream << "NeighborsTable {" << entity.neighborsTable << "}" << std::endl;
            break;
        case EntityType::Candidate:
            stream << "CandidatesTable {" << entity.candidatesTable << "}" << std::endl;
            break;
        case EntityType::Route:
            stream << "RoutesTable {" << entity.routesTable << "}" << std::endl;
            break;
        case EntityType::NetworkRoute:
            stream << "NetworkRoutesTable {" << entity.networkRoutesTable << "}" << std::endl;
            break;
        case EntityType::Graph:
            stream << "GraphsTable {" << entity.graphsTable << "}" << std::endl;
            break;
        case EntityType::Contract:
            stream << "ContractsTable {" << entity.contractsTable << "}" << std::endl;
            break;
        case EntityType::NetworkContract:
            stream << "NetworkContractsTable {" << entity.networkContractsTable << "}" << std::endl;
            break;
        case EntityType::AdvJoinInfo:
            stream << "advInfo {" << entity.advInfo << "}" << std::endl;
            break;
        case EntityType::AdvSuperframe:
            stream << "advSuperframe {" << entity.advSuperframe << "}" << std::endl;
            break;
        case EntityType::SessionKey:
            stream << "SessionKeysTable {" << entity.sessionKeysTable << "}" << std::endl;
            break;
        case EntityType::MasterKey:
            stream << "MasterKeysTable {" << entity.masterKeysTable << "}" << std::endl;
            break;
        case EntityType::SubnetKey:
            stream << "SubnetKeysTable {" << entity.subnetKeysTable << "}" << std::endl;
            break;
        case EntityType::PowerSupply:
            stream << "powerSupplyStatus {" << entity.powerSupplyStatus << "}" << std::endl;
            break;
        case EntityType::AssignedRole:
            stream << "assignedRole {" << entity.assignedRole << "}" << std::endl;
            break;
        case EntityType::AddressTranslation:
            stream << "AddressTranslationTable {" << entity.addressTranslationTable << "}" << std::endl;
            break;
        case EntityType::ClientServerRetryTimeout:
            stream << "retryTimeout {" << entity.retryTimeout << "}" << std::endl;
            break;
        case EntityType::ClientServerRetryMaxTimeoutInterval:
            stream << "max_Retry_Timeout_Interval {" << entity.max_Retry_Timeout_Interval << "}" << std::endl;
            break;
        case EntityType::ClientServerRetryCount:
            stream << "retryCount {" << entity.retryCount << "}" << std::endl;
            break;
        case EntityType::Channel:
            break;
        case EntityType::BlackListChannels:
            break;
        case EntityType::ChannelHopping:
            break;
        case EntityType::Vendor_ID:
            stream << "vendorID {" << entity.vendorID << "}" << std::endl;
            break;
        case EntityType::Model_ID:
            stream << "modelID {" << entity.modelID << "}" << std::endl;
            break;
        case EntityType::Software_Revision_Information:
            stream << "softwareRevisionInformation {" << entity.softwareRevisionInformation << "}" << std::endl;
            break;
        case EntityType::PackagesStatistics:
            stream << "packagesStatistics {" << entity.packagesStatistics << "}" << std::endl;
            break;
        case EntityType::JoinReason:
            stream << "joinReason {" << entity.joinReason << "}" << std::endl;
            break;
        case EntityType::ContractsTable_MetaData:
            stream << "contractsTableMetadata {" << entity.contractsTableMetadata << "}" << std::endl;
            break;
        case EntityType::Neighbor_MetaData:
            stream << "neighborMetadata {" << entity.neighborMetadata << "}" << std::endl;
            break;
        case EntityType::Superframe_MetaData:
            stream << "superframeMetadata {" << entity.superframeMetadata << "}" << std::endl;
            break;
        case EntityType::Graph_MetaData:
            stream << "graphMetadata {" << entity.graphMetadata << "}" << std::endl;
            break;
        case EntityType::Link_MetaData:
            stream << "linkMetadata {" << entity.linkMetadata << "}" << std::endl;
            break;
        case EntityType::Route_MetaData:
            stream << "routeMetadata {" << entity.routeMetadata << "}" << std::endl;
            break;
        case EntityType::Diag_MetaData:
            stream << "diagMetadata {" << entity.diagMetadata << "}" << std::endl;
            break;
        case EntityType::ChannelDiag:
            break;
        case EntityType::NeighborDiag:
            break;
        case EntityType::HRCO_CommEndpoint:
            stream << "HrcoCommEndpoint {" << entity.hrcoCommEndpoint << "}" << std::endl;
            break;
        case EntityType::HRCO_Publish:
            stream << "HrcoObjectAttributes {" << entity.hrcoEntityIndexListAttribute << "}" << std::endl;
            break;
        case EntityType::DeviceCapability:
            stream << "deviceCapability {" << entity.deviceCapability << "}" << std::endl;
            break;
        case EntityType::ARMO_CommEndpoint:
            stream << "ArmoCommEndpoint {" << entity.armoCommEndpoint << "}" << std::endl;
            break;
        case EntityType::DLMO_MaxLifetime:
            stream << "DLMO_MaxLifetime {" << entity.dlmoMaxLifeTime << "}" << std::endl;
            break;
        case EntityType::DLMO_IdleChannels:
            stream << "DLMO_IdleChannels {" << entity.dlmoIdleChannels << "}" << std::endl;
            break;
        case EntityType::PingInterval:
            stream << "pingInterval {" << entity.pingInterval << "}" << std::endl;
            break;
        case EntityType::DLMO_DiscoveryAlert:
            stream << "DLMO_DiscoveryAlert {" << entity.dlmoDiscoveryAlert << "}" << std::endl;
            break;
        case EntityType::SerialNumber:
            break;
        default:
            stream << "Unknown entity type!" << std::endl;
            break;
    }
}

std::ostream& operator<<(std::ostream& stream, const LevelPrinterPhyAttributes& printer) {

    if (printer.level.entityType != ENTITY_TYPE_NOT_USED) {
        logEnityLevel(stream, printer);
        return stream;
    }

    PhyAttributes& entity = printer.phyAttributes;
    stream << std::endl;
    if (printer.level.logSimpleAttributes) {
        stream << "advInfo {" << entity.advInfo << "}" << std::endl;
        stream << "advSuperframe {" << entity.advSuperframe << "}" << std::endl;
        stream << "powerSupplyStatus {" << entity.powerSupplyStatus << "}" << std::endl;
        stream << "vendorID {" << entity.vendorID << "}" << std::endl;
        stream << "modelID {" << entity.modelID << "}" << std::endl;
        stream << "softwareRevisionInformation {" << entity.softwareRevisionInformation << "}" << std::endl;

        stream << "packagesStatistics {" << entity.packagesStatistics << "}" << std::endl;

        stream << "joinReason {" << entity.joinReason << "}" << std::endl;

        stream << "contractsTableMetadata {" << entity.contractsTableMetadata << "}" << std::endl;
        stream << "neighborMetadata {" << entity.neighborMetadata << "}" << std::endl;
        stream << "superframeMetadata {" << entity.superframeMetadata << "}" << std::endl;
        stream << "graphMetadata {" << entity.graphMetadata << "}" << std::endl;
        stream << "linkMetadata {" << entity.linkMetadata << "}" << std::endl;
        stream << "routeMetadata {" << entity.routeMetadata << "}" << std::endl;
        stream << "diagMetadata {" << entity.diagMetadata << "}" << std::endl;

        stream << "HrcoCommEndpoint {" << entity.hrcoCommEndpoint << "}" << std::endl;
        stream << "HrcoObjectAttributes {" << entity.hrcoEntityIndexListAttribute << "}" << std::endl;

        stream << "deviceCapability {" << entity.deviceCapability << "}" << std::endl;

        stream << "ArmoCommEndpoint {" << entity.armoCommEndpoint << "}" << std::endl;

        stream << "retryTimeout {" << entity.retryTimeout << "}" << std::endl;
        stream << "max_Retry_Timeout_Interval {" << entity.max_Retry_Timeout_Interval << "}" << std::endl;
        stream << "retryCount {" << entity.retryCount << "}" << std::endl;
        stream << "dlmoMaxLifeTime {" << entity.dlmoMaxLifeTime << "}" << std::endl;
        stream << "dlmoIdleChannels {" << entity.dlmoIdleChannels << "}" << std::endl;
        stream << "dmoPingInterval {" << entity.pingInterval << "}" << std::endl;
        stream << "dlmoDiscoveryAlert {" << entity.dlmoDiscoveryAlert << "}" << std::endl;
    }
    if (printer.level.logIndexedAttributes) {
        stream << "BlacklistChannelsIndexedAttribute {" << entity.blacklistChannelsTable << "}" << std::endl;
        stream << "ChannelHoppingIndexedAttribute {" << entity.channelHoppingTable << "}" << std::endl;
        stream << "LinksTable {" << entity.linksTable << "}" << std::endl;
        stream << "SuperframesTable {" << entity.superframesTable << "}" << std::endl;
        stream << "NeighborsTable {" << entity.neighborsTable << "}" << std::endl;
        stream << "CandidatesTable {" << entity.candidatesTable << "}" << std::endl;
        stream << "RoutesTable {" << entity.routesTable << "}" << std::endl;
        stream << "NetworkRoutesTable {" << entity.networkRoutesTable << "}" << std::endl;
        stream << "GraphsTable {" << entity.graphsTable << "}" << std::endl;
        stream << "ContractsTable {" << entity.contractsTable << "}" << std::endl;
        stream << "NetworkContractsTable {" << entity.networkContractsTable << "}" << std::endl;
        stream << "SessionKeysTable {" << entity.sessionKeysTable << "}" << std::endl;
        stream << "MasterKeysTable {" << entity.masterKeysTable << "}" << std::endl;
        stream << "SubnetKeysTable {" << entity.subnetKeysTable << "}" << std::endl;
        stream << "AddressTranslationTable {" << entity.addressTranslationTable << "}" << std::endl;
        //stream << "AlertsTable {" << entity.alertsTable << "}" << std::endl;
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const OperationIDList& operationsIDList){
    if (!operationsIDList.empty()){
        stream << " opID:" << std::hex;
        for (OperationIDList::const_iterator it = operationsIDList.begin(); it != operationsIDList.end(); ++it){
            stream << (int)*it << ",";
        }
        stream << std::dec;
    }
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const BlacklistChannelsIndexedAttribute& table) {
    stream << std::endl;

    int idx = 0;
    std::string prefix = " AT BC ";
    std::string cols[] = { prefix, ATTR_CUR_PEN_SPACE, PENDING_READ_DEL_SPACE, "      entityIndex", " sequence" };

    for (uint i = 0; i < sizeof(cols) / sizeof(std::string); i++) {
        stream << cols[i];
    }
    stream << std::endl;

    BlacklistChannelsIndexedAttribute::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        if (!it->second.currentValue && !it->second.previousValue) {
            idx = 0;
            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << "NULL values for both currentValue and previousValue";
            stream << std::endl;
        }

        if (it->second.currentValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << it->first;
            std::vector<Uint8>::iterator itSeq = it->second.currentValue->seq.begin();
            for (; itSeq != it->second.currentValue->seq.end(); ++itSeq) {
                stream << " " << std::setw(2) << (int)(*itSeq);
            }
            stream << it->second.waitingOperations;

            stream << std::endl;
        }

        if (it->second.previousValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_PREVIOUS;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << it->first;
            std::vector<Uint8>::iterator itSeq = it->second.previousValue->seq.begin();
            for (; itSeq != it->second.previousValue->seq.end(); ++itSeq) {
                stream << " " << std::setw(2) << (int)(*itSeq);
            }
            stream << it->second.waitingOperations;

            stream << std::endl;
        }
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const ChannelHoppingIndexedAttribute& table) {
    stream << std::endl;

    int idx = 0;
    std::string prefix = " AT CH ";
    std::string cols[] = { prefix, ATTR_CUR_PEN_SPACE, PENDING_READ_DEL_SPACE, "      entityIndex", "      index", " len",
            " sequence" };

    for (uint i = 0; i < sizeof(cols) / sizeof(std::string); i++) {
        stream << cols[i];
    }
    stream << std::endl;

    PhyChannelHopping * currentValue = NULL;
    PhyChannelHopping * previousValue = NULL;
    ChannelHoppingIndexedAttribute::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        currentValue = it->second.currentValue;
        previousValue = it->second.previousValue;
        if (!currentValue && !previousValue) {
            idx = 0;
            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << "NULL values for both currentValue and previousValue";
            stream << std::endl;
        }

        if (currentValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << std::hex << it->first;
            stream << std::setw(cols[idx++].size()) << std::dec << currentValue->index;
            stream << std::setw(cols[idx++].size()) << (int) currentValue->length;
            std::vector<Uint8>::iterator itSeq = currentValue->seq.begin();
            for (; itSeq != currentValue->seq.end(); ++itSeq) {
                stream << " " << (int) (*itSeq);
            }
            stream << it->second.waitingOperations;

            stream << std::endl;
        }

        if (previousValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_PREVIOUS;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << it->first;
            stream << std::setw(cols[idx++].size()) << previousValue->index;
            stream << std::setw(cols[idx++].size()) << (int) previousValue->length;
            std::vector<Uint8>::iterator itSeq = previousValue->seq.begin();
            for (; itSeq != previousValue->seq.end(); ++itSeq) {
                stream << " " << (int) (*itSeq);
            }
            stream << it->second.waitingOperations;

            stream << std::endl;
        }
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const LinkIndexedAttribute& table) {

    std::string prefix = " AT LNK ";

    stream << std::endl;
    stream << std::setw(prefix.size() + 1) << prefix;
    stream << std::setw(5) << " "; //sizeof(" CUR ")
    stream << std::setw(4 + 1) << PENDING_READ_DEL_SPACE;
    stream << std::setw(16 + 1) << "entityIndex"; //16 = number of characters on <long long> + 1 = blank space
    stream << std::setw(5 + 1) << "index"; //sizeof("index") + 1 = blank space
    stream << std::setw(4 + 1) << "sfID"; //sizeof("sfID") + 1 = blank space
    stream << std::setw(3 + 1) << "rol"; //sizeof("rol") + 1 = blank space
    stream << std::setw(5 + 1) << "type"; //sizeof("TBADV") + 1 = blank space
    stream << std::setw(7 + 1) << "schedT"; //sizeof("Off&Int") + 1 = blank space
    stream << std::setw(9 + 2) << "off/inter"; // 1 blank space + 4 digits(off) + 1(/) + 5(inter)
    stream << std::setw(2 + 1) << "T1"; //sizeof("T1") + 1 = blank space
    stream << std::setw(2 + 1) << "T2"; //sizeof("T2") + 1 = blank space
    stream << std::setw(4 + 1) << "neiT"; //sizeof("neiT") + 1 = blank space
    stream << std::setw(6 + 1) << "neighb"; //sizeof("neighb") + 1 = blank space
    stream << std::setw(6 + 1) << "graphT"; //sizeof("graphT") + 1 = blank space
    stream << std::setw(5 + 1) << "graph"; //sizeof("graph") + 1 = blank space
    stream << std::setw(3 + 1) << "chT"; //sizeof("chT") + 1 = blank space
    stream << std::setw(5 + 1) << "chOff"; //sizeof("chOff") + 1 = blank space
    stream << std::setw(5 + 1) << "prioT"; //sizeof("prioT") + 1 = blank space
    stream << std::setw(4 + 1) << "prio"; //sizeof("prioT") + 1 = blank space
    stream << std::setw(4 + 1) << "dir";

    stream << std::endl;
    float nrRx = 0, nrRxIN = 0, nrRxOUT = 0;
    float nrTx = 0, nrTxIN = 0, nrTxOUT = 0;
    float number = 0;
    int countOfLinks = 0;
    LinkIndexedAttribute::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        if (!it->second.getValue()) {
            stream << std::setw(prefix.size() + 1) << prefix;
            stream << std::setw(5) << ATTR_CURRENT;
            stream << "NULL values for both currentValue and previousValue";
            stream << std::endl;
        }

        if (it->second.getValue()) {
            stream << std::setw(prefix.size() + 1) << prefix;
            stream << std::setw(5) << ATTR_CURRENT;
            stream << std::setw(4 + 1) << (it->second.isPending == true ? "P" : "");
            stream << std::hex << std::setw(16 + 1) << it->first; //entityIndex
            stream << std::dec << std::setw(5 + 1) << it->second.getValue()->index;//index
            stream << std::setw(4 + 1) << (int) it->second.getValue()->superframeIndex; //sfID
            stream << std::setw(3 + 1) << Tdma::TdmaLinkTypes::toString(
                        (Tdma::TdmaLinkTypes::TdmaLinkTypesEnum) it->second.getValue()->role); //rol
            stream << std::setw(5 + 1) << Tdma::LinkType::toString((Tdma::LinkType::LinkTypesEnum) it->second.getValue()->type); //type
            stream << std::setw(7 + 1) << NE::Model::Tdma::ScheduleType::toString(it->second.getValue()->schedType); //schedT
            stream << std::setw(4 + 1) << std::dec << (int) it->second.getValue()->schedule.offset; //off
            stream << std::setw(6) << (int) it->second.getValue()->schedule.interval;
            stream << std::hex; //inter
            stream << std::setw(2 + 1) << (int) it->second.getValue()->template1; //T1
            stream << std::setw(2 + 1) << (int) it->second.getValue()->template2; //T2
            stream << std::setw(4 + 1) << (int) it->second.getValue()->neighborType; //neiT
            stream << std::hex << std::setw(6 + 1) << (int) it->second.getValue()->neighbor; //neighb
            stream << std::dec << std::setw(6 + 1) << (int) it->second.getValue()->graphType; //graphT
            stream << std::setw(5 + 1) << (int) it->second.getValue()->graphID; //graph
            stream << std::setw(3 + 1) << (int) it->second.getValue()->chType; //chT
            stream << std::setw(5 + 1) << (int) it->second.getValue()->chOffset; //chOff
            stream << std::setw(5 + 1) << (int) it->second.getValue()->priorityType; //prioT
            stream << std::setw(4 + 1) << (int) it->second.getValue()->priority; //prio
            stream << std::setw(4 + 1) << (it->second.getValue()->direction == NE::Model::Tdma::TdmaLinkDir::INBOUND ? "IN" : "OUT");//direction

            stream << it->second.waitingOperations;
            stream << std::endl;

            countOfLinks++;
            //if interval is 0 consider 1 repetition in 30s
            number = it->second.getValue()->schedule.interval?(3000.0 / it->second.getValue()->schedule.interval) : 1;
            if ((it->second.getValue()->type & Tdma::LinkType::TRANSMIT) != 0){
                nrTx += number;
                if (it->second.getValue()->direction == NE::Model::Tdma::TdmaLinkDir::INBOUND){
                    nrTxIN += number;
                } else {
                    nrTxOUT += number;
                }
            } else {
                nrRx += number;
                if (it->second.getValue()->direction == NE::Model::Tdma::TdmaLinkDir::INBOUND){
                    nrRxIN += number;
                } else {
                    nrRxOUT += number;
                }
            }
        }

        if (it->second.previousValue) {
            stream << std::setw(prefix.size() + 1) << prefix;
            stream << std::setw(5) << ATTR_PREVIOUS;
            stream << std::setw(4 + 1) << (it->second.isPending == true ? "P" : "");
            stream << std::hex << std::setw(16 + 1) << it->first; //entityIndex
            stream << std::dec << std::setw(5 + 1) << it->second.previousValue->index;//index
            stream << std::setw(4 + 1) << (int) it->second.previousValue->superframeIndex; //sfID
            stream << std::setw(3 + 1) << Tdma::TdmaLinkTypes::toString(
                        (Tdma::TdmaLinkTypes::TdmaLinkTypesEnum) it->second.previousValue->role); //rol
            stream << std::setw(5 + 1) << Tdma::LinkType::toString((Tdma::LinkType::LinkTypesEnum) it->second.previousValue->type); //type
            stream << std::setw(7 + 1) << NE::Model::Tdma::ScheduleType::toString(it->second.previousValue->schedType); //schedT
            stream << std::setw(4 + 1) << std::dec << (int) it->second.previousValue->schedule.offset; //off
            stream << std::setw(6) << (int) it->second.previousValue->schedule.interval;
            stream << std::hex; //inter
            stream << std::setw(2 + 1) << (int) it->second.previousValue->template1; //T1
            stream << std::setw(2 + 1) << (int) it->second.previousValue->template2; //T2
            stream << std::setw(4 + 1) << (int) it->second.previousValue->neighborType; //neiT
            stream << std::hex << std::setw(6 + 1) << (int) it->second.previousValue->neighbor; //neighb
            stream << std::dec << std::setw(6 + 1) << (int) it->second.previousValue->graphType; //graphT
            stream << std::setw(5 + 1) << (int) it->second.previousValue->graphID; //graph
            stream << std::setw(3 + 1) << (int) it->second.previousValue->chType; //chT
            stream << std::setw(5 + 1) << (int) it->second.previousValue->chOffset; //chOff
            stream << std::setw(5 + 1) << (int) it->second.previousValue->priorityType; //prioT
            stream << std::setw(4 + 1) << (int) it->second.previousValue->priority; //prio
            stream << std::setw(4 + 1) << (it->second.previousValue->direction == NE::Model::Tdma::TdmaLinkDir::INBOUND ? "IN" : "OUT");//direction

            stream << it->second.waitingOperations;
            stream << std::endl;
        }
    }

    stream << std::setw(prefix.size() + 1) << prefix;
    stream << std::setw(5) << ATTR_CURRENT;
    stream << "Nr: " << countOfLinks << " TOTAL(30s) Tx=" << nrTx << " Rx="<< nrRx << " TxIN=" << nrTxIN << " TxOUT=" << nrTxOUT << " RxIN=" << nrRxIN << " RxOUT=" << nrRxOUT;


    return stream;
}

std::ostream& operator<<(std::ostream& stream, const SuperframeIndexedAttribute& table) {
    stream << std::endl;

    int idx = 0;
    std::string prefix = " AT SF ";
    std::string cols[] = { prefix, ATTR_CUR_PEN_SPACE, PENDING_READ_DEL_SPACE, "      entityIndex", "      index", "      tsDur",
            " chIndex", " chBirth", " sfType", " prirty", " chMapOv", " idlUsd", " sfPeriod", " sfBirth", " chRate", " chMap",
            "     idleTimer", " rndSlts" };

    for (uint i = 0; i < sizeof(cols) / sizeof(std::string); i++) {
        stream << cols[i];
    }
    stream << std::endl;

    SuperframeIndexedAttribute::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        if (!it->second.currentValue && !it->second.previousValue) {
            idx = 0;
            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << "NULL values for both currentValue and previousValue";
            stream << std::endl;
        }

        if (it->second.currentValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << std::hex << it->first;
            stream << std::setw(cols[idx++].size()) << std::dec << it->second.currentValue->index;
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->tsDur;
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->chIndex;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->chBirth;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->sfType;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->priority;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->chMapOv;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->idleUsed;
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->sfPeriod;
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->sfBirth;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->chRate;
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->chMap;
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->idleTimer;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->rndSlots;

            stream << it->second.waitingOperations;
            stream << std::endl;
        }

        if (it->second.previousValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_PREVIOUS;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << it->first;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->index;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->tsDur;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->chIndex;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->chBirth;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->sfType;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->priority;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->chMapOv;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->idleUsed;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->sfPeriod;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->sfBirth;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->chRate;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->chMap;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->idleTimer;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->rndSlots;

            stream << it->second.waitingOperations;
            stream << std::endl;
        }
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const NeighborIndexedAttribute& table) {
    stream << std::endl;

    int idx = 0;
    std::string prefix = " AT NEI ";
    std::string cols[] = { prefix, ATTR_CUR_PEN_SPACE, PENDING_READ_DEL_SPACE, "      entityIndex", "      index",
            "            address64", " grpCod", " clckSrc", " extGrCnt", " diagLvl", " lnkBcklog", " lnkBcklogIdx",
            " lnkBvklogDur", " extendGraph (grId, lastHop, prefBranch)" };

    for (uint i = 0; i < sizeof(cols) / sizeof(std::string); i++) {
        stream << cols[i];
    }
    stream << std::endl;

    NeighborIndexedAttribute::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        if (!it->second.currentValue && !it->second.previousValue) {
            idx = 0;
            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << "NULL values for both currentValue and previousValue";
            stream << std::endl;
        }
        if (it->second.currentValue) {
            idx = 0;

            stream << std::setw(cols[0].size()) << cols[0];
            stream << std::setw(cols[1].size()) << ATTR_CURRENT;
            stream << std::setw(cols[2].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[3].size()) << std::hex <<  it->first;
            stream << std::setw(cols[4].size()) << std::hex << it->second.currentValue->index;
            stream << std::setw(cols[5].size()) << it->second.currentValue->address64.toString();
            stream << std::setw(cols[6].size()) << (int) it->second.currentValue->groupCode;
            stream << std::setw(cols[7].size()) << (int) it->second.currentValue->clockSource;
            stream << std::setw(cols[8].size()) << (int) it->second.currentValue->extGrCnt;
            stream << std::setw(cols[9].size()) << (int) it->second.currentValue->diagLevel;
            stream << std::setw(cols[10].size()) << (int) it->second.currentValue->linkBacklog;
            stream << std::setw(cols[11].size()) << (int) it->second.currentValue->linkBacklogIndex;
            stream << std::setw(cols[12].size()) << (int) it->second.currentValue->linkBacklogDur;

//            int colIdx = ++idx;
            std::vector<PhyNeighbor::ExtendGraph>::iterator itGraph = it->second.currentValue->extendGraph.begin();
            for (; itGraph != it->second.currentValue->extendGraph.end(); ++itGraph) {
                stream << " (";
                stream << std::setw(cols[13].size()) << itGraph->graph_ID;
                stream << ", " << std::setw(cols[13].size()) << (int) itGraph->lastHop;
                stream << ", " << std::setw(cols[13].size()) << (int) itGraph->preferredBranch;
                stream << ")";
            }

            stream << it->second.waitingOperations;
            stream << std::endl;
        }

        if (it->second.previousValue) {
            idx = 0;

            stream << std::setw(cols[0].size()) << cols[0];
            stream << std::setw(cols[1].size()) << ATTR_PREVIOUS;
            stream << std::setw(cols[2].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[3].size()) << it->first;
            stream << std::setw(cols[4].size()) << std::hex << it->second.previousValue->index;
            stream << std::setw(cols[5].size()) << it->second.previousValue->address64.toString();
            stream << std::setw(cols[6].size()) << (int) it->second.previousValue->groupCode;
            stream << std::setw(cols[7].size()) << (int) it->second.previousValue->clockSource;
            stream << std::setw(cols[8].size()) << (int) it->second.previousValue->extGrCnt;
            stream << std::setw(cols[9].size()) << (int) it->second.previousValue->diagLevel;
            stream << std::setw(cols[10].size()) << (int) it->second.previousValue->linkBacklog;
            stream << std::setw(cols[11].size()) << (int) it->second.previousValue->linkBacklogIndex;
            stream << std::setw(cols[12].size()) << (int) it->second.previousValue->linkBacklogDur;

//            int colIdx = ++idx;
            std::vector<PhyNeighbor::ExtendGraph>::iterator itGraph = it->second.previousValue->extendGraph.begin();
            for (; itGraph != it->second.previousValue->extendGraph.end(); ++itGraph) {
                stream << " (";
                stream << std::setw(cols[13].size()) << itGraph->graph_ID;
                stream << ", " << std::setw(cols[13].size()) << (int) itGraph->lastHop;
                stream << ", " << std::setw(cols[13].size()) << (int) itGraph->preferredBranch;
                stream << ")";
            }

            stream << it->second.waitingOperations;
            stream << std::endl;
        }
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const CandidateIndexedAttribute& table) {
    stream << std::endl;

    int idx = 0;
    std::string prefix = " AT CAN ";
    std::string cols[] = { prefix, ATTR_CUR_PEN_SPACE, PENDING_READ_DEL_SPACE, "      entityIndex", "    neighbor", " rsqi" };

    for (uint i = 0; i < sizeof(cols) / sizeof(std::string); i++) {
        stream << cols[i];
    }
    stream << std::endl;

    CandidateIndexedAttribute::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        if (!it->second.currentValue && !it->second.previousValue) {
            idx = 0;
            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << "NULL values for both currentValue and previousValue";
            stream << std::endl;
        }

        if (it->second.currentValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << it->first;
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->neighbor;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->radio;

            stream << it->second.waitingOperations;
            stream << std::endl;
        }

        if (it->second.previousValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_PREVIOUS;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << it->first;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->neighbor;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->radio;

            stream << it->second.waitingOperations;
            stream << std::endl;
        }
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const RouteIndexedAttribute& table) {
    stream << std::endl;

    int idx = 0;
    std::string prefix = " AT ROU ";
    std::string cols[] = { prefix, ATTR_CUR_PEN_SPACE, PENDING_READ_DEL_SPACE, "      entityIndex", "      index", " size",
            " evalTime", " altrntve", " frwrdLmt", " selector", " srcAddr", " route" };

    for (uint i = 0; i < sizeof(cols) / sizeof(std::string); i++) {
        stream << cols[i];
    }
    stream << std::endl;

    RouteIndexedAttribute::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        if (!it->second.currentValue && !it->second.previousValue) {
            idx = 0;
            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << "NULL values for both currentValue and previousValue";
            stream << std::endl;
        }

        if (it->second.currentValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << it->first;
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->index;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->route.size();
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->evaluationTime;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->alternative;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->forwardLimit;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->selector;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->sourceAddress;
            std::vector<Uint16>::iterator itRoute = it->second.currentValue->route.begin();
            for (; itRoute != it->second.currentValue->route.end(); ++itRoute) {
                stream << " " << Address::toString(*itRoute);
            }

            stream << it->second.waitingOperations;
            stream << std::endl;
        }

        if (it->second.previousValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_PREVIOUS;
            stream << std::setw(cols[idx++].size()) << it->first;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->index;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->route.size();
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->evaluationTime;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->alternative;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->forwardLimit;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->selector;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->sourceAddress;
            std::vector<Uint16>::iterator itRoute = it->second.previousValue->route.begin();
            for (; itRoute != it->second.previousValue->route.end(); ++itRoute) {
                stream << " " << Address::toString(*itRoute);
            }

            stream << it->second.waitingOperations;
            stream << std::endl;
        }
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const NetworkRouteIndexedAttribute& table) {
    stream << std::endl;

    int idx = 0;
    std::string prefix = " AT NR ";
    std::string cols[] = { prefix, ATTR_CUR_PEN_SPACE, PENDING_READ_DEL_SPACE, "      entityIndex", " netRouteID",
            "                              destination", "                                  nextHop", " nwkHopLimit", " outInt" };

    for (uint i = 0; i < sizeof(cols) / sizeof(std::string); i++) {
        stream << cols[i];
    }
    stream << std::endl;

    NetworkRouteIndexedAttribute::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        if (!it->second.currentValue && !it->second.previousValue) {
            idx = 0;
            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << "NULL values for both currentValue and previousValue";
            stream << std::endl;
        }

        if (it->second.currentValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << std::hex << it->first;
            stream << std::setw(cols[idx++].size()) << std::dec << it->second.currentValue->networkRouteID;
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->destination.toString();
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->nextHop.toString();
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->nwkHopLimit;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->outgoingInterface;

            stream << it->second.waitingOperations;
            stream << std::endl;
        }

        if (it->second.previousValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_PREVIOUS;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << it->first;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->networkRouteID;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->destination.toString();
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->nextHop.toString();
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->nwkHopLimit;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->outgoingInterface;

            stream << it->second.waitingOperations;
            stream << std::endl;
        }
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const GraphIndexedAttribute& table) {
    stream << std::endl;

    int idx = 0;
    std::string prefix = " AT GRA ";
    std::string cols[] = { prefix, ATTR_CUR_PEN_SPACE, PENDING_READ_DEL_SPACE, "      entityIndex", "      index", " prfdBrnch",
            " nghbrCnt", " queue", " maxLifetime", " neighbors" };

    for (uint i = 0; i < sizeof(cols) / sizeof(std::string); i++) {
        stream << cols[i];
    }
    stream << std::endl;

    GraphIndexedAttribute::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        if (!it->second.currentValue && !it->second.previousValue) {
            idx = 0;
            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << "NULL values for both currentValue and previousValue";
            stream << std::endl;
        }

        if (it->second.currentValue) {
            idx = 0;

            stream << std::hex << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::hex << std::setw(cols[idx++].size()) << it->first;
            stream << std::dec << std::setw(cols[idx++].size()) << it->second.currentValue->index;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->preferredBranch;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->neighborCount;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->queue;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->maxLifetime;
            std::vector<Uint16>::iterator itNei = it->second.currentValue->neighbors.begin();
            for (; itNei != it->second.currentValue->neighbors.end(); ++itNei) {
                stream << std::hex << " " << (int) (*itNei);
            }

            stream << it->second.waitingOperations;
            stream << std::endl;
        }

        if (it->second.previousValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_PREVIOUS;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::hex << std::setw(cols[idx++].size()) << it->first;
            stream << std::dec << std::setw(cols[idx++].size()) << it->second.previousValue->index;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->preferredBranch;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->neighborCount;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->queue;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->maxLifetime;
            std::vector<Uint16>::iterator itNei = it->second.previousValue->neighbors.begin();
            for (; itNei != it->second.previousValue->neighbors.end(); ++itNei) {
                stream << std::hex << " " << (int) (*itNei);
            }

            stream << it->second.waitingOperations;
            stream << std::endl;
        }
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const ContractIndexedAttribute& table) {
    stream << std::endl;

    int idx = 0;
    std::string prefix = " AT CON ";
    std::string cols[] = { prefix, ATTR_CUR_PEN_SPACE, PENDING_READ_DEL_SPACE, "      entityIndex", " contrID", " reqtID",
            " srcSAP", "  src32", " dstSAP", " dest32", " resCode", "   Type", " ActivTim", "       Life", " Prior", " MxNsduSz",
            " Reliab", " Period", " Phase", " Dedln", " ComBurst", " ExsBurst", " MxSendWndSz", " isMngmnt" };

    for (uint i = 0; i < sizeof(cols) / sizeof(std::string); i++) {
        stream << cols[i];
    }
    stream << std::endl;

    ContractIndexedAttribute::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        if (!it->second.currentValue && !it->second.previousValue) {
            idx = 0;
            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << "NULL values for both currentValue and previousValue";
            stream << std::endl;
        }

        if (it->second.currentValue) {
            idx = 0;
            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << std::hex << it->first;
            stream << std::setw(cols[idx++].size()) << std:: dec << it->second.currentValue->contractID;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->requestID;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->sourceSAP;
            stream << std::setw(cols[idx++].size()) << Address::toString(it->second.currentValue->source32);
            stream << std::setw(cols[idx++].size()) << std::dec << (int) it->second.currentValue->destinationSAP;
            stream << std::setw(cols[idx++].size()) << Address::toString(it->second.currentValue->destination32);
            stream << std::setw(cols[idx++].size()) << std::dec <<  (int) it->second.currentValue->responseCode;
            stream << std::setw(cols[idx++].size()) << CommunicationServiceType::toString(
                        it->second.currentValue->communicationServiceType);
            stream << std::setw(cols[idx++].size()) << std::hex << it->second.currentValue->contract_Activation_Time;
            stream << std::setw(cols[idx++].size()) << std::hex << it->second.currentValue->assigned_Contract_Life;
            stream << std::dec;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->assigned_Contract_Priority;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->assigned_Max_NSDU_Size;
            stream << std::setw(cols[idx++].size())
                        << (int) it->second.currentValue->assigned_Reliability_And_PublishAutoRetransmit;
            stream << std::setw(cols[idx++].size()) << std::dec << (int) it->second.currentValue->assignedPeriod;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->assignedPhase;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->assignedDeadline;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->assignedCommittedBurst;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->assignedExcessBurst;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->assigned_Max_Send_Window_Size;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->isManagement;

            stream << it->second.waitingOperations;
            stream << std::endl;
        }

        if (it->second.previousValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_PREVIOUS;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << it->first;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->contractID;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->requestID;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->sourceSAP;
            stream << std::setw(cols[idx++].size()) << Address::toString(it->second.previousValue->source32);
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->destinationSAP;
            stream << std::setw(cols[idx++].size()) << Address::toString(it->second.previousValue->destination32);
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->responseCode;
            stream << std::setw(cols[idx++].size()) << CommunicationServiceType::toString(
                        it->second.previousValue->communicationServiceType);
            stream << std::setw(cols[idx++].size()) << std::hex << it->second.previousValue->contract_Activation_Time;
            stream << std::setw(cols[idx++].size()) << std::hex << it->second.previousValue->assigned_Contract_Life;
            stream << std::dec;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->assigned_Contract_Priority;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->assigned_Max_NSDU_Size;
            stream << std::setw(cols[idx++].size())
                        << (int) it->second.previousValue->assigned_Reliability_And_PublishAutoRetransmit;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->assignedPeriod;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->assignedPhase;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->assignedDeadline;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->assignedCommittedBurst;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->assignedExcessBurst;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->assigned_Max_Send_Window_Size;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->isManagement;

            stream << it->second.waitingOperations;
            stream << std::endl;
        }
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const NetworkContractIndexedAttribute& table) {
    stream << std::endl;

    int idx = 0;
    std::string prefix = " AT NC ";
    std::string cols[] = { prefix, ATTR_CUR_PEN_SPACE, PENDING_READ_DEL_SPACE, "      entityIndex", " contractID",
            "                           sourceAddress", "                      destinationAddress", " contract_Priority",
            " include_Contract_Flag" };

    for (uint i = 0; i < sizeof(cols) / sizeof(std::string); i++) {
        stream << cols[i];
    }
    stream << std::endl;

    NetworkContractIndexedAttribute::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        if (!it->second.currentValue && !it->second.previousValue) {
            idx = 0;
            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << "NULL values for both currentValue and previousValue";
            stream << std::endl;
        }

        if (it->second.currentValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << std::hex << it->first;
            stream << std::setw(cols[idx++].size()) << std::dec << it->second.currentValue->contractID;
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->sourceAddress.toString();
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->destinationAddress.toString();
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->contract_Priority;
            stream << std::setw(cols[idx++].size()) << (it->second.currentValue->include_Contract_Flag ? "true" : "false");

            stream << it->second.waitingOperations;
            stream << std::endl;
        }

        if (it->second.previousValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_PREVIOUS;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << it->first;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->contractID;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->sourceAddress.toString();
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->destinationAddress.toString();
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->contract_Priority;
            stream << std::setw(cols[idx++].size()) << (it->second.previousValue->include_Contract_Flag ? "true" : "false");

            stream << it->second.waitingOperations;
            stream << std::endl;
        }
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const SessionKeyIndexedAttribute& table) {
    stream << std::endl;

    int idx = 0;
    std::string prefix = " AT SK ";
    std::string cols[] = { prefix, ATTR_CUR_PEN_SPACE, PENDING_READ_DEL_SPACE, "      entityIndex", "       keyID",
            "         destination", "              source", " dstTSAP", " srcTSAP", "                                             key", " softLifeTime", " hardLifeTime",
            " sessionKeyPolicy" };

    for (uint i = 0; i < sizeof(cols) / sizeof(std::string); i++) {
        stream << cols[i];
    }
    stream << std::endl;

    SessionKeyIndexedAttribute::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        if (!it->second.currentValue && !it->second.previousValue) {
            idx = 0;
            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << "NULL values for both currentValue and previousValue";
            stream << std::endl;
        }

        if (it->second.currentValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << std::hex << it->first;
            stream << std::setw(cols[idx++].size()) << std::dec << it->second.currentValue->keyID;
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->destination64.toString();
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->source64.toString();
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->destinationTSAP;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->sourceTSAP;
            stream << std::setw(cols[idx++].size()) << NE::Misc::Convert::array2string(it->second.currentValue->key.value, SecurityKey::LENGTH);
            stream << std::hex;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->softLifeTime;
            stream << std::setw(cols[idx++].size()) << (int) it->second.currentValue->hardLifeTime;
            stream << " " << it->second.currentValue->sessionKeyPolicy.toString();

            stream << it->second.waitingOperations;
            stream << std::endl;
        }

        if (it->second.previousValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_PREVIOUS;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << it->first;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->keyID;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->destination64.toString();
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->source64.toString();
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->destinationTSAP;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->sourceTSAP;
            stream << std::setw(cols[idx++].size()) << NE::Misc::Convert::array2string(it->second.previousValue->key.value, SecurityKey::LENGTH);
            stream << std::hex;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->softLifeTime;
            stream << std::setw(cols[idx++].size()) << (int) it->second.previousValue->hardLifeTime;
            stream << " " << it->second.previousValue->sessionKeyPolicy.toString();

            stream << it->second.waitingOperations;
            stream << std::endl;
        }
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const SpecialKeyIndexedAttribute& table) {
    std::string prefix = " AT SpK ";

    stream << std::endl;
    stream << std::setw(prefix.size() + 1) << prefix;
    stream << std::setw(5) << " "; //sizeof(" CUR ")
    stream << std::setw(4 + 1) << PENDING_READ_DEL_SPACE;
    stream << std::setw(16 + 1) << "entityIndex"; //16 = number of characters on <long long> + 1 = blank space
    stream << std::setw(5 + 1) << "keyID"; //sizeof("keyID") + 1 = blank space
    stream << std::setw(48) << "key";
    stream << std::setw(12 + 1) << "softLifeTime"; //sizeof("softLifeTime") + 1 = blank space
    stream << std::setw(12 + 1) << "hardLifeTime"; //sizeof("hardLifeTime") + 1 = blank space
    stream << std::setw(8 + 1) << "expiring"; //sizeof("expiring") + 1 = blank space
    stream << std::setw(8) << "policy";

    stream << std::endl;

    SpecialKeyIndexedAttribute::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        if (!it->second.getValue()) {
            stream << std::setw(prefix.size() + 1) << prefix;
            stream << std::setw(5) << ATTR_CURRENT;
            stream << "NULL values for both currentValue and previousValue";
            stream << std::endl;
        }

        if (it->second.getValue()) {
            stream << std::setw(prefix.size() + 1) << prefix;
            stream << std::setw(5) << ATTR_CURRENT;
            stream << std::setw(4 + 1) << (it->second.isPending == true ? "P" : "");
            stream << std::hex << std::setw(16 + 1) << std::hex << it->first; //entityIndex
            stream << std::dec << std::setw(5 + 1) << std::dec << (int) it->second.getValue()->keyID; //keyID
            stream << std::setw(48) << NE::Misc::Convert::array2string(it->second.currentValue->key.value, SecurityKey::LENGTH); //key
            stream << std::setw(12 + 1) << (int) it->second.getValue()->softLifeTime; //softLifeTime
            stream << std::setw(12 + 1) << (int) it->second.getValue()->hardLifeTime; //hardLifeTime
            stream << std::setw(8 + 1) << (int) it->second.getValue()->markedAsExpiring; //expiring
            stream << " " << it->second.getValue()->policy.toString();

            stream << it->second.waitingOperations;
            stream << std::endl;
        }

        if (it->second.previousValue) {
            stream << std::setw(prefix.size() + 1) << prefix;
            stream << std::setw(5) << ATTR_PREVIOUS;
            stream << std::setw(4 + 1) << (it->second.isPending == true ? "P" : "");
            stream << std::hex << std::setw(16 + 1) << std::hex << it->first; //entityIndex
            stream << std::dec << std::setw(5 + 1) << std::dec << (int) it->second.previousValue->keyID; //keyID
            stream << std::setw(48) << NE::Misc::Convert::array2string(it->second.previousValue->key.value, SecurityKey::LENGTH); //key
            stream << std::setw(12 + 1) << (int) it->second.previousValue->softLifeTime; //softLifeTime
            stream << std::setw(12 + 1) << (int) it->second.previousValue->hardLifeTime; //hardLifeTime
            stream << std::setw(8 + 1) << (int) it->second.previousValue->markedAsExpiring; //expiring
            stream << " " << it->second.previousValue->policy.toString();

            stream << it->second.waitingOperations;
            stream << std::endl;
        }
    }

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const AddressTranslationIndexedAttribute& table) {
    stream << std::endl;

    int idx = 0;
    std::string prefix = " AT ATT ";
    std::string cols[] = { prefix, ATTR_CUR_PEN_SPACE, PENDING_READ_DEL_SPACE, "      entityIndex", "   addrssTransID",
            "                             longAddress", " shortAddress" };

    for (uint i = 0; i < sizeof(cols) / sizeof(std::string); i++) {
        stream << cols[i];
    }
    stream << std::endl;

    AddressTranslationIndexedAttribute::const_iterator it = table.begin();
    for (; it != table.end(); ++it) {
        if (!it->second.currentValue && !it->second.previousValue) {
            idx = 0;
            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << "NULL values for both currentValue and previousValue";
            stream << std::endl;
        }

        if (it->second.currentValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_CURRENT;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << it->first;
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->addressTranslationID;
            stream << std::setw(cols[idx++].size()) << it->second.currentValue->longAddress.toString();
            stream << " " << Address::toString(it->second.currentValue->shortAddress);

            stream << it->second.waitingOperations;
            stream << std::endl;
        }

        if (it->second.previousValue) {
            idx = 0;

            stream << std::setw(cols[idx++].size()) << cols[0];
            stream << std::setw(cols[idx++].size()) << ATTR_PREVIOUS;
            stream << std::setw(cols[idx++].size()) << (it->second.isPending == true ? "P" : "");
            stream << std::setw(cols[idx++].size()) << it->first;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->addressTranslationID;
            stream << std::setw(cols[idx++].size()) << it->second.previousValue->longAddress.toString();
            stream << " " << Address::toString(it->second.previousValue->shortAddress);

            stream << it->second.waitingOperations;
            stream << std::endl;
        }
    }

    return stream;
}

}
}
