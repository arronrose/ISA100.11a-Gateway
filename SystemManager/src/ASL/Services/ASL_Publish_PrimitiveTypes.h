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

#ifndef ASL_PUBLISH_PRIMITIVETYPES_H_
#define ASL_PUBLISH_PRIMITIVETYPES_H_

#include "Common/smTypes.h"
#include "Common/NETypes.h"
#include "Common/Address128.h"
#include "AL/ObjectsIDs.h"
#include "ASL/PDU/ClientServerPDU.h"

#include <boost/shared_ptr.hpp>

using namespace Isa100::Common;

namespace Isa100 {
namespace ASL {
namespace Services {

/**
 * * Updated: Mar 2009 ; sorin.bidian
 *      - created indication primitive (D2A specification)
 *      - created .cpp file
 */

struct ASL_Publish_Indication {
        int elapsedMsec;
        TransmissionTime endToEndTransmissionTime;
        //Uint8 publishedDataSize; //i'm not using it
        Isa100::Common::TSAP::TSAP_Enum subscriberTSAP;
        Isa100::AL::ObjectID::ObjectIDEnum subscribingObject;
        NE::Common::Address128 publisherAddress;
        //TransportPorts::TransportPortsEnum publisherTransportLayerPort;
        Isa100::Common::TSAP::TSAP_Enum publisherTSAP;
        Isa100::AL::ObjectID::ObjectIDEnum publishingObject;
        Isa100::ASL::PDU::ClientServerPDUPointer apduPublish;

        ASL_Publish_Indication(const int elapsedMsec, TransmissionTime endToEndTransmissionTime, /*Uint8 publishedDataSize,*/
                    Isa100::Common::TSAP::TSAP_Enum subscriberTSAP,
                    Isa100::AL::ObjectID::ObjectIDEnum subscribingObject, NE::Common::Address128 publisherAddress,
                    Isa100::Common::TSAP::TSAP_Enum publisherTSAP, Isa100::AL::ObjectID::ObjectIDEnum publishingObject,
                    Isa100::ASL::PDU::ClientServerPDUPointer apduPublish);

       void toString( std::string &value);
};

typedef boost::shared_ptr<ASL_Publish_Indication> ASL_Publish_IndicationPointer;

}
}
}

#endif /*ASL_PUBLISH_PRIMITIVETYPES_H_*/
