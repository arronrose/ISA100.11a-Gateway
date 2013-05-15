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
#ifndef MODELPRINTER_H_
#define MODELPRINTER_H_

#include "Model/model.h"

namespace NE {
namespace Model {

#define ATTR_CURRENT " CUR "
#define ATTR_PREVIOUS " PRV "
#define ATTR_CUR_PEN_SPACE "     "

#define PENDING_READ " RP "
#define PENDING_DELETE " DP "
#define PENDING_READ_DEL_SPACE "Pend"
#define ENTITY_TYPE_NOT_USED -1

/**
 * float = BEFORE.AFTER
 * precision = digits<BEFORE> + digits<AFTER>
 * if the std::fixed is used: precision = digits<AFTER>
 */
#define FLOAT_FORMAT(width, precision, value) std::setw(width) << std::fixed << std::setprecision(precision) << value
#define TYPE_FORMAT(width, value) std::setw(width) << value

struct LogDeviceDetail{
    LogDeviceDetail():logSimpleAttributes(false), logIndexedAttributes(false), logTheoreticAttributes(false), entityType(-1) {}
    bool logSimpleAttributes;
    bool logIndexedAttributes;
    bool logTheoreticAttributes;
    int entityType; // EntityType::EntityTypeEnum
};

struct LevelPrinterPhyAttributes{
    NE::Model::PhyAttributes & phyAttributes;
    LogDeviceDetail& level;
    LevelPrinterPhyAttributes(NE::Model::PhyAttributes & phyAttributes_, LogDeviceDetail& level_): phyAttributes(phyAttributes_), level(level_){}
};

template<class T>
std::ostream& operator<<(std::ostream& stream, const Attribute<T>& entity) {

    if (entity.isPending == true) {
        stream << "isPending ";
    }
//    stream << std::endl;

    if (entity.currentValue != NULL) {
        stream << " CURENT {" << *entity.currentValue << "}";
    }

    if (entity.previousValue != NULL) {
        stream << " PREVIOUS { " << *entity.previousValue << "} ";
    }

    return stream;
}

/**
 * Used for : LinkIndexedAttribute, SuperframeIndexedAttribute, NeighborIndexedAttribute, CandidateIndexedAttribute, RouteIndexedAttribute,
 * NetworkRouteIndexedAttribute, GraphIndexedAttribute, ContractIndexedAttribute, SessionKeyIndexedAttribute, AddressTranslationIndexedAttribute
 * @param stream
 * @param entity
 * @return
 */
template<class T>
std::ostream& operator<<(std::ostream& stream, const std::map<EntityIndex, T>& table) {
    typename std::map<EntityIndex, T>::const_iterator it;
    for (it = table.begin(); it != table.end(); ++it) {
        stream << "[" << std::hex << it->first << " , " << it->second << "]" << std::endl;
    }

    return stream;
}
std::ostream& operator<<(std::ostream& stream, const OperationIDList& operationsIDList);

std::ostream& operator<<(std::ostream& stream, const PhyUint8& entity);

std::ostream& operator<<(std::ostream& stream, const PhyUint16& entity);

std::ostream& operator<<(std::ostream & stream, const PhyString & entity);

std::ostream& operator<<(std::ostream & stream, const PhyBytes & entity);

std::ostream& operator<<(std::ostream& stream, const Schedule& entity);

std::ostream& operator<<(std::ostream& stream, const PhyAdvJoinInfo& entity);

std::ostream& operator<<(std::ostream& stream, const PhyMetaData& entity);

std::ostream& operator<<(std::ostream& stream, const PhyNeighbor::ExtendGraph& entity);

std::ostream& operator<<(std::ostream& stream, const PhyNeighbor& entity);

std::ostream& operator<<(std::ostream& stream, const PhyRoute& entity);

std::ostream& operator<<(std::ostream& stream, const PhyGraph& entity);

std::ostream& operator<<(std::ostream& stream, const PhyLink& entity);

std::ostream& operator<<(std::ostream& stream, const PhySuperframe& entity);

std::ostream& operator<<(std::ostream& stream, const PhyNetworkRoute& entity);

std::ostream& operator<<(std::ostream& stream, const PhyAddressTranslation& entity);

std::ostream& operator<<(std::ostream& stream, const PhyContract& entity);

std::ostream& operator<<(std::ostream& stream, const PhyNetworkContract& entity);

std::ostream& operator<<(std::ostream& stream, const PhyChannelHopping& entity);

std::ostream& operator<<(std::ostream& stream, const PhyBlacklistChannels& entity);

std::ostream& operator<<(std::ostream& stream, const PhyChannelDiag::ChannelTransmission& entity);

std::ostream& operator<<(std::ostream& stream, const PhyChannelDiag& entity);

std::ostream& operator<<(std::ostream& stream, const PhyCandidate& entity);

std::ostream& operator<<(std::ostream& stream, const PhySessionKey& entity);

std::ostream& operator<<(std::ostream& stream, const PhySpecialKey& entity);

std::ostream& operator<<(std::ostream& stream, const PhyCommunicationAssociationEndpoint& entity);

std::ostream& operator<<(std::ostream& stream, const PhyAlertCommunicationEndpoint& entity);

std::ostream& operator<<(std::ostream& stream, const PhyEntityIndexList& entity);

std::ostream& operator<<(std::ostream& stream, const PhyEnergyDesign& entity);

std::ostream& operator<<(std::ostream& stream, const PhyDeviceCapability & entity);

std::ostream& operator<<(std::ostream& stream, const QueuePriorityEntry & entity);

std::ostream& operator<<(std::ostream& stream, const PhyQueuePriority & entity);

std::ostream& operator<<(std::ostream& stream, const BlacklistChannelsIndexedAttribute& table);

std::ostream& operator<<(std::ostream& stream, const ChannelHoppingIndexedAttribute& table);

std::ostream& operator<<(std::ostream& stream, const LinkIndexedAttribute& table);

std::ostream& operator<<(std::ostream& stream, const SuperframeIndexedAttribute& table);

std::ostream& operator<<(std::ostream& stream, const NeighborIndexedAttribute& table);

std::ostream& operator<<(std::ostream& stream, const CandidateIndexedAttribute& table);

std::ostream& operator<<(std::ostream& stream, const RouteIndexedAttribute& table);

std::ostream& operator<<(std::ostream& stream, const NetworkRouteIndexedAttribute& table);

std::ostream& operator<<(std::ostream& stream, const GraphIndexedAttribute& table);

std::ostream& operator<<(std::ostream& stream, const ContractIndexedAttribute& table);

std::ostream& operator<<(std::ostream& stream, const NetworkContractIndexedAttribute& table);

std::ostream& operator<<(std::ostream& stream, const SessionKeyIndexedAttribute& table);

std::ostream& operator<<(std::ostream& stream, const SpecialKeyIndexedAttribute& table);

std::ostream& operator<<(std::ostream& stream, const AddressTranslationIndexedAttribute& table);

std::ostream& operator<<(std::ostream& stream, const AlertsTable& alertsTable);

std::ostream& operator<<(std::ostream& stream, const PhyAttributes& entity);

std::ostream& operator<<(std::ostream& stream, const LevelPrinterPhyAttributes& printer);

}
}

#endif /* MODELPRINTER_H_ */
