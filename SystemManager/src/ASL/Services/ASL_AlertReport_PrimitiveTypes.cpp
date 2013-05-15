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
 * ASL_AlertReport_PrimitiveTypes.cpp
 *
 *  Created on: Jun 3, 2009
 *      Author: Sorin.Bidian
 */
#include "ASL_AlertReport_PrimitiveTypes.h"

using namespace Isa100::Common;
using namespace Isa100::AL;

namespace Isa100 {
namespace ASL {
namespace Services {

ASL_AlertReport_Indication::ASL_AlertReport_Indication(
            int elapsedMsec_,
            TransmissionTime endToEndTransmissionTime_,
		TSAP::TSAP_Enum armoTSAP_,
		ObjectID::ObjectIDEnum armoObject_,
		Address128 sourceAddress_,
		TSAP::TSAP_Enum sinkTSAP_,
		ObjectID::ObjectIDEnum sinkObject_,
		Isa100::ASL::PDU::ClientServerPDUPointer alertReport_) :
		    elapsedMsec(elapsedMsec_),
			endToEndTransmissionTime(endToEndTransmissionTime_),
			armoTSAP(armoTSAP_),
			armoObject(armoObject_), //ObjectID::ID_ARMO
			sourceAddress(sourceAddress_),
			sinkTSAP(sinkTSAP_),
			sinkObject(sinkObject_),
			alertReport(alertReport_) {

}

std::string ASL_AlertReport_Indication::toString() {
    std::ostringstream stream;
    std::string objectIdString;
    stream << "AlertReport_Indication {";
    stream << "TrTime: " << (int) endToEndTransmissionTime;
    Isa100::AL::ObjectID::toString(armoObject, armoTSAP, objectIdString);
    stream << ", armoObject: " << objectIdString;
    stream << ", sourceAddress: " << sourceAddress.toString();
    Isa100::AL::ObjectID::toString(sinkObject, sinkTSAP, objectIdString);
    stream << ", destObject: " << objectIdString;
    stream << ", APDU: " << alertReport->toString();
    stream << "}";

    return stream.str();
}

std::string ASL_AlertReport_Request::toString() {
    std::ostringstream stream;
    stream << "AlertReport_Request {";
    stream << "Contr: " << (int) contractID;
    stream << ", Pri: " << (int) priority;
    stream << ", DE: " << (int) discardEligible;
    std::string objectIdString;
    Isa100::AL::ObjectID::toString(sinkObject, sinkTSAP, objectIdString);
    stream << ", SObj: " << objectIdString;
    Isa100::AL::ObjectID::toString(armoObject, armoTSAP, objectIdString);
    stream << ", CObj: " << objectIdString;
    stream << ", Alert: " << alertReport->toString();
    stream << "}";
    return stream.str();
}

}
}
}
