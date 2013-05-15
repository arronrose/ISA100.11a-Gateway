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
 * ClientServerPDU.h
 *
 *  Created on: Oct 13, 2008
 *      Author: catalin.pop
 */

#ifndef CLIENTSERVERPDU_H_
#define CLIENTSERVERPDU_H_
#include "Common/NETypes.h"
#include "Common/smTypes.h"
#include "AL/ObjectsIDs.h"

namespace Isa100 {
namespace ASL {
namespace PDU {

class ClientServerPDU {
public:
	Isa100::Common::PrimitiveType::PrimitiveTypeEnum primitiveType;
	Isa100::Common::ServiceType::ServiceType serviceInfo;
	Isa100::AL::ObjectID::ObjectIDEnum sourceObjectID;
	Isa100::AL::ObjectID::ObjectIDEnum destinationObjectID;
	AppHandle appHandle;
	BytesPointer payload;

	ClientServerPDU(Isa100::Common::PrimitiveType::PrimitiveTypeEnum primitiveType_,
	    Isa100::Common::ServiceType::ServiceType serviceInfo_,
	    Isa100::AL::ObjectID::ObjectIDEnum sourceObjectID_,
	    Isa100::AL::ObjectID::ObjectIDEnum destinationObjectID_,
	    AppHandle appHandle_,
	    BytesPointer payload_);

	/*
     * Constructor without payload field. The payload field will be empty.
     */
	ClientServerPDU(Isa100::Common::PrimitiveType::PrimitiveTypeEnum primitiveType_,
	    Isa100::Common::ServiceType::ServiceType serviceInfo_,
	    Isa100::AL::ObjectID::ObjectIDEnum sourceObjectID_,
	    Isa100::AL::ObjectID::ObjectIDEnum destinationObjectID_,
	    AppHandle appHandle_);

	virtual ~ClientServerPDU();

	std::string toString();
};

typedef boost::shared_ptr<ClientServerPDU> ClientServerPDUPointer;

}

}

}

#endif /* CLIENTSERVERPDU_H_ */
