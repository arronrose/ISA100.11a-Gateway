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
#ifndef ENTITIESHELPER_H_
#define ENTITIESHELPER_H_

#include "Model/model.h"
#include "Security/SecurityKeyAndPolicies.h"
#include "Security/SecurityDeleteKeyReq.h"
#include "Misc/Marshall/NetworkOrderStream.h"

namespace Isa100 {
namespace Model {

void marshallEntity(const NE::Model::PhyUint8& entity, NE::Misc::Marshall::NetworkOrderStream& stream);
void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyUint8& entity);

void marshallEntity(const NE::Model::PhyUint16& entity, NE::Misc::Marshall::NetworkOrderStream& stream);
void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyUint16& entity);

void unmarshallEntity(Bytes & value, NE::Model::PhyString & entity);
void unmarshallEntity(Bytes & value, NE::Model::PhyBytes & entity);

void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyMetaData& entity);

void marshallEntity(const NE::Model::PhyNeighbor& entity, NE::Misc::Marshall::NetworkOrderStream& stream);
void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyNeighbor& entity);

void marshallEntity(const NE::Model::PhyRoute& entity, NE::Misc::Marshall::NetworkOrderStream& stream);
void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyRoute& entity);

void marshallEntity(const NE::Model::PhyNetworkContract& entity, NE::Misc::Marshall::NetworkOrderStream& stream);
void marshallEntity(const NE::Model::PhyAddressTranslation& entity, NE::Misc::Marshall::NetworkOrderStream& stream);

void marshallEntity(const NE::Model::PhyNetworkRoute& entity, NE::Misc::Marshall::NetworkOrderStream& stream);

void marshallEntity(const NE::Model::PhyLink& entity, NE::Misc::Marshall::NetworkOrderStream& stream);
void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyLink& entity);

void marshallEntity(const NE::Model::PhyAdvJoinInfo& entity, NE::Misc::Marshall::NetworkOrderStream& stream);

void marshallEntity(const NE::Model::PhySuperframe& entity, NE::Misc::Marshall::NetworkOrderStream& stream);
void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhySuperframe& entity);

void marshallEntity(const NE::Model::PhyGraph& entity, NE::Misc::Marshall::NetworkOrderStream& stream);
void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyGraph& entity);

void marshallEntity(const NE::Model::PhyCommunicationAssociationEndpoint& entity, NE::Misc::Marshall::NetworkOrderStream& stream);

void marshallEntity(const NE::Model::PhyAlertCommunicationEndpoint& entity, NE::Misc::Marshall::NetworkOrderStream& stream);

void unmarshallEntity(NE::Misc::Marshall::InputStream& stream, NE::Model::PhyEnergyDesign& entity);

void unmarshallEntity(NE::Misc::Marshall::InputStream & stream, NE::Model::PhyDeviceCapability & entity);

void marshallEntity(const NE::Model::PhyQueuePriority & entity, NE::Misc::Marshall::NetworkOrderStream & stream);

void marshallEntity(const NE::Model::Policy& entity, NE::Misc::Marshall::NetworkOrderStream& stream);

void marshallEntity(const Isa100::Security::SecurityKeyAndPolicies& entity, NE::Misc::Marshall::NetworkOrderStream& stream);

void marshallEntity(const Isa100::Security::SecurityDeleteKeyReq& entity, NE::Misc::Marshall::NetworkOrderStream& stream);

//    void marshallEntity(const NE::Model::PhyObjectAttributeIndexAndSize& entity, NE::Misc::Marshall::NetworkOrderStream& stream);

}
}

#endif /* ENTITIESHELPER_H_ */
