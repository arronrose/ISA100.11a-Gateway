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
 * ClientServerPDU.cpp
 *
 *  Created on: Oct 13, 2008
 *      Author: catalin.pop
 */

#include "ClientServerPDU.h"
#include "Misc/Convert/Convert.h"

namespace Isa100 {
namespace ASL {
namespace PDU {

ClientServerPDU::ClientServerPDU(Isa100::Common::PrimitiveType::PrimitiveTypeEnum primitiveType_,
		Isa100::Common::ServiceType::ServiceType serviceInfo_,
		Isa100::AL::ObjectID::ObjectIDEnum sourceObjectID_,
		Isa100::AL::ObjectID::ObjectIDEnum destinationObjectID_,
		AppHandle appHandle_,
		BytesPointer payload_) :
	primitiveType(primitiveType_),
    serviceInfo(serviceInfo_), sourceObjectID(sourceObjectID_), destinationObjectID(destinationObjectID_), appHandle(appHandle_),
    payload(payload_) {
}

ClientServerPDU::ClientServerPDU(Isa100::Common::PrimitiveType::PrimitiveTypeEnum primitiveType_,
        Isa100::Common::ServiceType::ServiceType serviceInfo_,
        Isa100::AL::ObjectID::ObjectIDEnum sourceObjectID_,
        Isa100::AL::ObjectID::ObjectIDEnum destinationObjectID_,
        AppHandle appHandle_) :
    primitiveType(primitiveType_),
    serviceInfo(serviceInfo_), sourceObjectID(sourceObjectID_), destinationObjectID(destinationObjectID_), appHandle(appHandle_),
    payload(new Bytes()) {

}

ClientServerPDU::~ClientServerPDU() {
}

std::string ClientServerPDU::toString(){
		std::ostringstream stream;
		stream << "CSPDU {"
		<< "PrType: " << Isa100::Common::PrimitiveType::toString(primitiveType)
		<< ", Srv: " << Isa100::Common::ServiceType::toString(serviceInfo)
		<< ", Obj: " << (int)sourceObjectID
		<< "->" << (int)destinationObjectID
		<< ", reqID: " << (int)appHandle;
		if (payload) {
			stream << ", sz: " << payload->size();
			stream << ", payload: " << NE::Misc::Convert::bytes2string(*payload);
		} else {
			stream << ", sz: 0";
		}

		return stream.str();
	}

}
}
}
