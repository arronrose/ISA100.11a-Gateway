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
 * ASL_Publish_PrimitiveTypes.cpp
 *
 *  Created on: Mar 17, 2009
 *      Author: sorin.bidian
 */

#include "ASL/Services/ASL_Publish_PrimitiveTypes.h"

using namespace Isa100::Common;
//using namespace Isa100::Common::Objects;
using namespace Isa100::ASL::PDU;
using namespace Isa100::AL;
using namespace NE::Common;

namespace Isa100 {
namespace ASL {
namespace Services {

ASL_Publish_Indication::ASL_Publish_Indication(int elapsedMsec_, TransmissionTime endToEndTransmissionTime_, /*Uint8 publishedDataSize_,*/
            TSAP::TSAP_Enum subscriberTSAP_, ObjectID::ObjectIDEnum subscribingObject_, Address128 publisherAddress_,
            Isa100::Common::TSAP::TSAP_Enum publisherTSAP_, ObjectID::ObjectIDEnum publishingObject_,
            ClientServerPDUPointer apduPublish_) :
                elapsedMsec(elapsedMsec_),
                endToEndTransmissionTime(endToEndTransmissionTime_), /*publishedDataSize(publishedDataSize_),*/
                subscriberTSAP(subscriberTSAP_), subscribingObject(subscribingObject_), publisherAddress(publisherAddress_),
                publisherTSAP(publisherTSAP_), publishingObject(publishingObject_), apduPublish(apduPublish_) {

}

void ASL_Publish_Indication::toString( std::string &value) {
    std::ostringstream stream;
    stream << "ASL_Publish_Indication { ";
    stream << "TrTime: " << (int) endToEndTransmissionTime;
    //stream << ", subscriberTSAP: " << (int)subscriberTSAP;
    std::string objectIdString;
    Isa100::AL::ObjectID::toString(subscribingObject, subscriberTSAP, objectIdString);
    stream << ", subscribingObject: " << objectIdString;
    stream << ", publisherAddress: " << publisherAddress.toString();
    //stream << ", publisherTSAP: " << (int)publisherTSAP;
    Isa100::AL::ObjectID::toString(publishingObject, publisherTSAP, objectIdString);
    stream << ", publishingObject: " <<  objectIdString;
    stream << ", APDU: " << apduPublish->toString();
    stream << "}";

    value = stream.str();
}

}
}
}

