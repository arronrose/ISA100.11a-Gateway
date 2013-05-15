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
 * STSO.cpp
 *
 *  Created on: Mar 30, 2009
 *      Author: sorin.bidian, beniamin.tecar, radu.pop
 */

#include "STSO.h"
#include "Common/NEException.h"
#include "Common/SmSettingsLogic.h"
#include "Misc/Marshall/NetworkOrderStream.h"
#include "ASL/PDUUtils.h"
#include "ASL/PDU/ReadRequestPDU.h"
#include "Model/EngineProvider.h"
#include "AL/ActiveDevicesTable.h"

using namespace Isa100::ASL;
using namespace Isa100::ASL::Services;
using namespace Isa100::Common;
using namespace Isa100::Common::Objects;
using namespace Isa100::Model;
using namespace NE::Model;
using namespace NE::Misc::Marshall;


namespace Isa100 {
namespace AL {
namespace SMAP {

STSO::STSO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine_)
        : Isa100Object(tsap, engine_),
    currentUTCAdjustment(0), nextUTCAdjustmentTime(0), nextUTCAdjustment(34) {

    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: created object:" << objectIdString);
}

STSO::~STSO() {
    std::string objectIdString;
    getObjectIDString(objectIdString);
	LOG_DEBUG(LOG_OI << "LIFE: destroyed object:" << objectIdString);
}

bool STSO::isJobFinished() {
    return false;
}

void STSO::execute( Uint32 currentTime ) {

    resetLifeTime( currentTime );
}

Isa100::AL::ObjectID::ObjectIDEnum STSO::getObjectID() const {
    return Isa100::AL::ObjectID::ID_STSO;
}

bool STSO::expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    return true;
}

void STSO::indicateRead(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {

	ASL::PDU::ReadRequestPDUPointer readRequest = PDUUtils::extractReadRequest(indication->apduRequest);
	if ((int)readRequest->targetAttribute.attributeID == (int)STSOAttributeID::Current_UTC_Adjustment) {
	    Device* device = engine->getDevice(
	                engine->getAddress32(indication->clientNetworkAddress));
	    if (device) {

			NetworkOrderStream stream;
			stream.write(SmSettingsLogic::instance().currentUTCAdjustment);
			sendReadResponseToRequester(indication, SFC::success, BytesPointer(new Bytes(stream.ostream.str())), jobFinished);
		} else {
			sendReadResponseToRequester(indication, SFC::objectAccessDenied, BytesPointer(new Bytes()), jobFinished);
		}
	} else if ((int)readRequest->targetAttribute.attributeID == (int)STSOAttributeID::Next_UTC_Adjustment_Time) {
		NetworkOrderStream stream;
		stream.write(SmSettingsLogic::instance().nextUTCAdjustmentTime);
		sendReadResponseToRequester(indication, SFC::success, BytesPointer(new Bytes(stream.ostream.str())), jobFinished);
	} else if ((int)readRequest->targetAttribute.attributeID == (int)STSOAttributeID::Next_UTC_Adjustment) {
		NetworkOrderStream stream;
		stream.write(SmSettingsLogic::instance().nextUTCAdjustment);
		sendReadResponseToRequester(indication, SFC::success, BytesPointer(new Bytes(stream.ostream.str())), jobFinished);
	} else {
		THROW_EX(NE::Common::NEException, "Unknown attribute id: " << (int)readRequest->targetAttribute.attributeID);
		sendReadResponseToRequester(indication, SFC::invalidArgument, BytesPointer(new Bytes()), jobFinished);
	}
}

}
}
}
