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
 * ASL_AlertAcknowledge_PrimitiveTypes.cpp
 *
 *  Created on: Jun 3, 2009
 *      Author: Sorin.Bidian
 */
#include "ASL_AlertAcknowledge_PrimitiveTypes.h"

using namespace Isa100::Common;
using namespace Isa100::AL;

namespace Isa100 {
namespace ASL {
namespace Services {

ASL_AlertAcknowledge_Request::ASL_AlertAcknowledge_Request(Uint16 contractID_, Address32 destination_,
            ServicePriority::ServicePriority priority_, bool discardEligible_, Isa100::Common::TSAP::TSAP_Enum sourceTSAP_,
            Isa100::AL::ObjectID::ObjectIDEnum sourceObject_, Isa100::Common::TSAP::TSAP_Enum destinationTSAP_,
            Isa100::AL::ObjectID::ObjectIDEnum destinationObject_, Isa100::ASL::PDU::ClientServerPDUPointer alertAcknowledge_) :
    contractID(contractID_), destination(destination_), priority(priority_), discardEligible(discardEligible_), sourceTSAP(
                sourceTSAP_), sourceObject(sourceObject_), destinationTSAP(destinationTSAP_), destinationObject(
                destinationObject_), alertAcknowledge(alertAcknowledge_) {

}

std::string ASL_AlertAcknowledge_Request::toString() {
    std::ostringstream stream;
    std::string objectId;
    stream << "AlertAcknowledge_Request {";
    stream << "contract: " << (int) contractID;
    stream << ", priority: " << (int) priority;
    stream << ", DE: " << (int) discardEligible;
    Isa100::AL::ObjectID::toString(sourceObject, sourceTSAP, objectId);
    stream << ", SrcObj: " << objectId;
    Isa100::AL::ObjectID::toString(destinationObject, destinationTSAP, objectId);
    stream << ", DestObj: " << objectId;
    stream << ", APDU: " << alertAcknowledge->toString();
    stream << "}";

    return stream.str();
}

std::string ASL_AlertAcknowledge_Indication::toString() {
    std::ostringstream stream;
    stream << "AlertAcknowledge_Indication {";
    stream << "TrTime: " << endToEndTransmissionTime;
    stream << ", SrcAddress:" << sourceNetworkAddress.toString();
    std::string objectId;
    Isa100::AL::ObjectID::toString(sourceObject, sourceTSAP, objectId);
    stream << ", SrcObj: " << objectId;
    Isa100::AL::ObjectID::toString(destinationObject, destinationTSAP, objectId);
    stream << ", DestObj: " << objectId;
    stream << ", APDU: " << alertAcknowledge->toString();
    stream << "}";

    return stream.str();
}

}
}
}
