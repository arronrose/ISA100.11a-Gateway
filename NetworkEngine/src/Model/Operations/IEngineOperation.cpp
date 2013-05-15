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
 * IEngineOperation.cpp
 *
 *  Created on: Oct 27, 2009
 *      Author: Catalin Pop
 */

#include "IEngineOperation.h"
#include "../ModelPrinter.h"
#include "Model/model.h"
#include <iomanip>
#include "Common/logging.h"

namespace NE {
namespace Model {
namespace Operations {

OperationID IEngineOperation::lastOperationID = 0;

IEngineOperation::IEngineOperation() :
            containerId(0),
    physicalEntity(NULL),
    taiCutOver(0),
    state(OperationState::GENERATED),
    errorCode(OperationErrorCode::SUCCESS),
    opID(lastOperationID++)
{
}

IEngineOperation::~IEngineOperation() {
    if (physicalEntity) {
        //the phyEntity was not applied to model and no one handled the phyEntity correctly.
        //possible this could lead to a memory leak and the phyEntity gets destroyed here.
        // this case should be avoided by other means or otherwise documented here.
        LOG_ERROR("PhyEntity not NULL on destructor of operation on entIndex " << std::hex << getEntityIndex() << " type " << getType() << " opID:" << opID);

//By cata:
//       comented the delete of entity. Avoiding the crash when the entity is destroyed here and the device already has this entity.
//Bad:  in case no device has this entity a memory leak is produced
        EntityType::EntityTypeEnum entityType = getEntityType(getEntityIndex());
        switch (entityType) {
            case EntityType::Link:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyLink *)physicalEntity));
                delete (PhyLink *) physicalEntity;
                return;
            case EntityType::Superframe:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhySuperframe *)physicalEntity));
                delete (PhySuperframe *) physicalEntity;
                return;
            case EntityType::Neighbour:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyNeighbor *)physicalEntity));
                delete (PhyNeighbor *) physicalEntity;
                return;
            case EntityType::Candidate:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyCandidate *)physicalEntity));
                delete (PhyCandidate *) physicalEntity;
                return;
            case EntityType::Route:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyRoute *)physicalEntity));
                delete (PhyRoute *) physicalEntity;
                return;
            case EntityType::NetworkRoute:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyNetworkRoute *)physicalEntity));
                delete (PhyNetworkRoute *) physicalEntity;
                return;
            case EntityType::Graph:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyGraph *)physicalEntity));
                delete (PhyGraph *) physicalEntity;
                return;
            case EntityType::Contract:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyContract *)physicalEntity));
                delete (PhyContract *) physicalEntity;
                return;
            case EntityType::NetworkContract:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyNetworkContract *)physicalEntity));
                delete (PhyNetworkContract *) physicalEntity;
                return;
            case EntityType::AdvJoinInfo:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyAdvJoinInfo *)physicalEntity));
                delete (PhyAdvJoinInfo *) physicalEntity;
                return;
            case EntityType::AdvSuperframe:
            case EntityType::ClientServerRetryTimeout:
            case EntityType::ClientServerRetryMaxTimeoutInterval:
            case EntityType::DLMO_MaxLifetime:
            case EntityType::DLMO_IdleChannels:
            case EntityType::PingInterval:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyUint16 *)physicalEntity));
                delete (PhyUint16 *) physicalEntity;
                return;
            case EntityType::SessionKey:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhySessionKey *)physicalEntity));
                delete (PhySessionKey *) physicalEntity;
                return;
            case EntityType::MasterKey:
            case EntityType::SubnetKey:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhySpecialKey *)physicalEntity));
                delete (PhySpecialKey *) physicalEntity;
                return;
            case EntityType::PowerSupply:
            case EntityType::ClientServerRetryCount:
            case EntityType::AssignedRole:
            case EntityType::DLMO_DiscoveryAlert:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyUint8 *)physicalEntity));
                delete (PhyUint8 *) physicalEntity;
                return;
            case EntityType::AddressTranslation:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyAddressTranslation *)physicalEntity));
                delete (PhyAddressTranslation *) physicalEntity;
                return;
            case EntityType::Channel:
                LOG_DEBUG("PhyEntity not NULL op=Channel - Not implemented yet!");
                // delete (PhyChannel *) physicalEntity;
                return;
            case EntityType::BlackListChannels:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyBlacklistChannels *)physicalEntity));
                delete (PhyBlacklistChannels *) physicalEntity;
                return;
            case EntityType::ChannelHopping:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyChannelHopping *)physicalEntity));
                delete (PhyChannelHopping *) physicalEntity;
                return;
            case EntityType::Vendor_ID:
            case EntityType::Model_ID:
            case EntityType::Software_Revision_Information:
            case EntityType::SerialNumber:
            case EntityType::PackagesStatistics:
            case EntityType::JoinReason:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyString *)physicalEntity));
                delete (PhyString *) physicalEntity;
                return;
            case EntityType::ContractsTable_MetaData:
            case EntityType::Neighbor_MetaData:
            case EntityType::Superframe_MetaData:
            case EntityType::Graph_MetaData:
            case EntityType::Link_MetaData:
            case EntityType::Route_MetaData:
            case EntityType::Diag_MetaData:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyMetaData *)physicalEntity));
                delete (PhyMetaData *) physicalEntity;
                return;
            case EntityType::ChannelDiag:
                LOG_DEBUG("PhyEntity not NULL op=ChannelDiag - Not implemented yet!");
                return;
            case EntityType::NeighborDiag:
                LOG_DEBUG("PhyEntity not NULL op=NeighborDiag - Not implemented yet!");
                return;
            case EntityType::HRCO_CommEndpoint:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyCommunicationAssociationEndpoint *)physicalEntity));
                delete (PhyCommunicationAssociationEndpoint *) physicalEntity;
                return;
            case EntityType::HRCO_Publish:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyEntityIndexList *)physicalEntity));
                delete (PhyEntityIndexList *) physicalEntity;
                return;
            case EntityType::DeviceCapability:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyDeviceCapability *)physicalEntity));
                delete (PhyDeviceCapability *) physicalEntity;
                return;
            case EntityType::QueuePriority:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyQueuePriority *)physicalEntity));
                delete (PhyQueuePriority *) physicalEntity;
                return;
            case EntityType::ARMO_CommEndpoint:
                LOG_DEBUG("PhyEntity not NULL op=" << *((PhyAlertCommunicationEndpoint *)physicalEntity));
                delete (PhyAlertCommunicationEndpoint *) physicalEntity;
                return;

            default:
                LOG_ERROR("Deletion not implemented for entityType " << std::hex << entityType);
                return;
        }
    }

}

std::string IEngineOperation::getName() const {
    switch (getType()) {
        case EngineOperationType::WRITE_ATTRIBUTE:
            return "WA";
        case EngineOperationType::READ_ATTRIBUTE:
            return "RA";
        case EngineOperationType::UPDATE_GRAPH:
            return "UG";
        case EngineOperationType::DELETE_ATTRIBUTE:
            return "DA";
        default:
            throw NEException("Invalid operation type.");
    }
}

void IEngineOperation::physicalToString(std::ostream& stream, EntityType::EntityTypeEnum entityType, void * physical) const {
    if (!physical) {
        return;
    }

    switch (entityType) {
        case EntityType::Link: {
            stream << *((PhyLink*) physical);
            break;
        }
        case EntityType::Superframe: {
            stream << *((PhySuperframe*) physical);
            break;
        }
        case EntityType::Neighbour: {
            stream << *((PhyNeighbor*) physical);
            break;
        }
        case EntityType::Candidate: {
            stream << *((PhyCandidate*) physical);
            break;
        }
        case EntityType::Route: {
            stream << *((PhyRoute*) physical);
            break;
        }
        case EntityType::NetworkRoute: {
            stream << *((PhyNetworkRoute*) physical);
            break;
        }
        case EntityType::Graph: {
            stream << *((PhyGraph*) physical);
            break;
        }
        case EntityType::Contract: {
            stream << *((PhyContract*) physical);
            break;
        }
        case EntityType::NetworkContract: {
            stream << *((PhyNetworkContract*) physical);
            break;
        }
        case EntityType::AdvJoinInfo: {
            stream << *((PhyAdvJoinInfo*) physical);
            break;
        }
        case EntityType::AdvSuperframe:
        case EntityType::ClientServerRetryTimeout:
        case EntityType::ClientServerRetryMaxTimeoutInterval:
        case EntityType::DLMO_MaxLifetime:
        case EntityType::DLMO_IdleChannels:
        case EntityType::PingInterval: {
            stream << *((PhyUint16*) physical);
            break;
        }
        case EntityType::SessionKey: {
            stream << *((PhySessionKey*) physical);
            break;
        }
        case EntityType::MasterKey:
        case EntityType::SubnetKey: {
            stream << *((PhySpecialKey*) physical);
            break;
        }
        case EntityType::PowerSupply:
        case EntityType::ClientServerRetryCount:
        case EntityType::AssignedRole:
        case EntityType::DLMO_DiscoveryAlert: {
            stream << *((PhyUint8*) physical);
            break;
        }
        case EntityType::AddressTranslation: {
            stream << *((PhyAddressTranslation*) physical);
            break;
        }
        case EntityType::Channel: {
            stream << "Not implemented yet!";
            break;
        }
        case EntityType::BlackListChannels: {
            stream << *((PhyBlacklistChannels*) physical);
            break;
        }
        case EntityType::ChannelHopping: {
            stream << *((PhyChannelHopping*) physical);
            break;
        }
        case EntityType::Vendor_ID:
        case EntityType::Model_ID:
        case EntityType::Software_Revision_Information:
        case EntityType::SerialNumber:
        case EntityType::PackagesStatistics:
        case EntityType::JoinReason: {
            stream << *((PhyString*) physical);
            break;
        }
        case EntityType::ContractsTable_MetaData:
        case EntityType::Neighbor_MetaData:
        case EntityType::Superframe_MetaData:
        case EntityType::Graph_MetaData:
        case EntityType::Link_MetaData:
        case EntityType::Route_MetaData:
        case EntityType::Diag_MetaData: {
            stream << *((PhyMetaData*) physical);
            break;
        }
        case EntityType::ChannelDiag: {
            stream << "Not implemented yet!";
            break;
        }
        case EntityType::NeighborDiag: {
            stream << "Not implemented yet!";
            break;
        }
        case EntityType::HRCO_CommEndpoint: {
            stream << *((PhyCommunicationAssociationEndpoint*) physical);
            break;
        }
        case EntityType::HRCO_Publish: {
            stream << *((PhyEntityIndexList*) physical);
            break;
        }
        case EntityType::DeviceCapability: {
            stream << *((PhyDeviceCapability*) physical);
            break;
        }
        case EntityType::QueuePriority: {
            stream << *((PhyQueuePriority*) physical);
            break;
        }
        case EntityType::ARMO_CommEndpoint: {
            stream << *((PhyAlertCommunicationEndpoint*) physical);
            break;
        }

        default: {
            throw NE::Common::NEException("Entity type not treated!");
        }
    }
}

void IEngineOperation::toStringCommonOperationState(std::ostream& stream) const {
    stream << getName() << " {";
    stream << "containerId: " << std::dec << containerId;
    stream << ", " << std::hex << getOwner();
    stream << "-" << std::dec << getEntityType(getEntityIndex());
    stream << "-" << std::hex << getIndex(getEntityIndex());
    stream << ", opID: " << (int)opID;
    stream << ", errCode: " << getErrorCode();
    stream << ", " << NE::Model::EntityType::toString(getEntityType(getEntityIndex()));

    stream << ", opDep: ";
    OperationDependency::const_iterator it = operationDependency.begin();
    for (; it != operationDependency.end(); ++it) {
        stream << std::hex << (*it)->getEntityIndex() << "&";
    }
    stream << ", entDep: ";
    EntityDependency::const_iterator itEntity = entityDependency.begin();
    for (; itEntity != entityDependency.end(); ++itEntity) {
        stream << std::hex << (*itEntity) << "&";
    }

    if (getType() != EngineOperationType::READ_ATTRIBUTE && getType() != EngineOperationType::DELETE_ATTRIBUTE) {
        stream << std::endl;
        stream << "    PHY " << NE::Model::EntityType::toString(getEntityType(getEntityIndex()));
        stream << std::hex << " " << getOwner();
        stream << " {";
        const EntityType::EntityTypeEnum entityType = getEntityType(getEntityIndex());
        physicalToString(stream, entityType, physicalEntity);
    }
    stream << "}";

}

void IEngineOperation::toStringShortState(std::ostream& stream) const{
    stream << getName() << " {";
    stream << "containerId: " << std::dec << containerId;
    stream << ", " << std::hex << getOwner();
    stream << "-" << std::dec << getEntityType(getEntityIndex());
    stream << "-" << std::hex << getIndex(getEntityIndex());
    stream << ", errCode: " << getErrorCode();
    stream << ", " << NE::Model::EntityType::toString(getEntityType(getEntityIndex()));

    stream << "}";
}

bool IEngineOperation::operator==(const IEngineOperation& operation) const {
    if (this->getEntityIndex() == operation.getEntityIndex()
        && this->getType() == operation.getType()
        && this->getPhysicalEntity() == operation.getPhysicalEntity() ) {

        return true;
    }

    return false;
}

std::ostream& operator<<(std::ostream& stream, const IEngineOperation& operation) {
    operation.toString(stream);
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const IEngineOperationShortPrinter& shortPrinter){
    shortPrinter.operation.toStringShortState(stream);
    return stream;
}

}
}
}
