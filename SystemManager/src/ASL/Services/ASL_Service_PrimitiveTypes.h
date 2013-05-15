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

#ifndef ASL_SERVICE_PRIMITIVETYPES_H_
#define ASL_SERVICE_PRIMITIVETYPES_H_

#include "Common/NETypes.h"
#include "Common/NEAddress.h"
#include "Common/smTypes.h"
#include "ASL/PDU/ClientServerPDU.h"
#include "AL/ObjectsIDs.h"
#include <boost/shared_ptr.hpp>

using namespace Isa100::AL;

namespace Isa100 {
namespace ASL {
namespace Services {

/**
 * @author Beniamin Tecar
 * @version 1.0
 *
 * Updated on: Oct 9, 2008 ; sorin.bidian
 * Updated on: Oct 16,2008 ; catalin.pop
 * -renamed primitives
 * -created cpp file
 * -created 2 constructors
 */
struct PrimitiveRequest {
        //const NE::Model::ContractPointer serviceContract;
        const Uint16 contractID;
        Address32 destination; //useful for loop back determination and logging
        const Isa100::Common::ServicePriority::ServicePriority priority;
        const bool discardEligible;
        const Isa100::Common::TSAP::TSAP_Enum serverTSAP_ID;
        const ObjectID::ObjectIDEnum serverObject;
        const Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID;
        const ObjectID::ObjectIDEnum clientObject;
        const Isa100::ASL::PDU::ClientServerPDUPointer apduRequest;
        const bool forceUnencrypted;
        std::string primitiveRequestString;

        /*
         * Constructor for assigning values for all members of the structure.
         */
        PrimitiveRequest(const Uint16 contractID, const Address32 destination,
                    const Isa100::Common::ServicePriority::ServicePriority priority_, const bool discardEligible_,
                    const Isa100::Common::TSAP::TSAP_Enum serverTSAP_ID_, const ObjectID::ObjectIDEnum serverObject_,
                    const Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID_, const ObjectID::ObjectIDEnum clientObject_,
                    const Isa100::ASL::PDU::ClientServerPDUPointer apduRequest_, bool forceUnencrypted_ = false);

        /*
         * Constructor that default populates TSAP members for DMAP.
         * The clientObjectID and serverObjectID are taken from apdu.
         */
        PrimitiveRequest(const Uint16 contractID, const Address32 destination,
                    const Isa100::Common::ServicePriority::ServicePriority priority_, const bool discardEligible_,
                    const Isa100::ASL::PDU::ClientServerPDUPointer apduRequest_);

        void createString();
        /*
         * Write in a string the values of members.
         * @return std::string
         */
        const std::string& toString();
};
typedef boost::shared_ptr<PrimitiveRequest> PrimitiveRequestPointer;

struct PrimitiveResponse {
        //NE::Model::ContractPointer serviceContract;
        Uint16 contractID;
        Address32 destination; //useful for loop back determination and logging
        Isa100::Common::ServicePriority::ServicePriority priority;
        bool discardEligible;
        Uint8 forwardCongestionNotificationEcho;
        Isa100::Common::TSAP::TSAP_Enum serverTSAP_ID;
        ObjectID::ObjectIDEnum serverObject;
        Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID;
        ObjectID::ObjectIDEnum clientObject;
        Isa100::ASL::PDU::ClientServerPDUPointer apduResponse;
        bool forceUnencrypted;

        PrimitiveResponse(const Uint16 contractID, const Address32 destination,
                    const Isa100::Common::ServicePriority::ServicePriority priority_, const bool discardEligible_,
                    const Uint8 forwardCongestionNotificationEcho_, const Isa100::Common::TSAP::TSAP_Enum serverTSAP_ID_,
                    const ObjectID::ObjectIDEnum serverObject_, const Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID_,
                    const ObjectID::ObjectIDEnum clientObject_, const Isa100::ASL::PDU::ClientServerPDUPointer apduResponse_,
                    bool forceUnencrypted_ = false);

        /*
         * Constructor that default populates TSAP members for DMAP.
         * Objects ids are taken from apdu.
         */
        PrimitiveResponse(const Uint16 contractID, const Address32 destination,
                    const Isa100::Common::ServicePriority::ServicePriority priority_, const bool discardEligible_,
                    const Uint8 forwardCongestionNotificationEcho_, const Isa100::ASL::PDU::ClientServerPDUPointer apduResponse_);

        std::string toString();
};
typedef boost::shared_ptr<PrimitiveResponse> PrimitiveResponsePointer;

struct PrimitiveIndication {
        int elapsedMsec;
        const TransmissionDetailedTime detailedTxTime;
        const TransmissionTime endToEndTransmissionTime;
        const Uint8 forwardCongestionNotification;
        const Isa100::Common::TSAP::TSAP_Enum serverTSAP_ID;
        const ObjectID::ObjectIDEnum serverObject;
        const NE::Common::Address128 clientNetworkAddress;
        const Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID;
        const ObjectID::ObjectIDEnum clientObject;
        const Isa100::ASL::PDU::ClientServerPDUPointer apduRequest;
        std::string primitiveIndicationString;

        /*
         * Constructor for primitive used for DMAP corespondent. Will assign port, and tsap for DMAP.
         * Objects id are taken from apdu.
         */
        PrimitiveIndication(const int elapsedMsec, const TransmissionDetailedTime detailedTxTime,
                    const TransmissionTime endToEndTransmissionTime_, const Uint8 forwardCongestionNotification_,
                    const NE::Common::Address128 clientNetworkAddress_,
                    const Isa100::ASL::PDU::ClientServerPDUPointer apduRequest_);

        PrimitiveIndication(const int elapsedMsec, const TransmissionDetailedTime detailedTxTime,
                    const TransmissionTime endToEndTransmissionTime_, const Uint8 forwardCongestionNotification_,
                    const Isa100::Common::TSAP::TSAP_Enum serverTSAP_ID_, const ObjectID::ObjectIDEnum serverObject_,
                    const NE::Common::Address128 clientNetworkAddress_, const Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID_,
                    const ObjectID::ObjectIDEnum clientObject_, const Isa100::ASL::PDU::ClientServerPDUPointer apduRequest_);

        void createString();
        const std::string& toString();

};
typedef boost::shared_ptr<PrimitiveIndication> PrimitiveIndicationPointer;

struct PrimitiveConfirmation {
        int elapsedMsec;
        const TransmissionDetailedTime detailedTxTime;
        const TransmissionTime endToEndTransmissionTime;
        const Uint8 forwardCongestionNotification;
        const Uint8 forwardCongestionNotificationEcho;
        const NE::Common::Address128 serverNetworkAddress;
        const Isa100::Common::TSAP::TSAP_Enum serverTSAP_ID;
        const ObjectID::ObjectIDEnum serverObject;
        const Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID;
        const ObjectID::ObjectIDEnum clientObject;
        const Isa100::ASL::PDU::ClientServerPDUPointer apduResponse;
        std::string confirmationString;

        /*
         * Constructor for primitive used for DMAP corespondent. Will assign port, and tsap for DMAP.
         * Objects id are taken from apdu.
         */
        PrimitiveConfirmation(const int elapsedMsec, const TransmissionDetailedTime detailedTxTime,
                    const TransmissionTime endToEndTransmissionTime_, const Uint8 forwardCongestionNotification_,
                    const Uint8 forwardCongestionNotificationEcho_, const NE::Common::Address128 serverNetworkAddress_,
                    const Isa100::ASL::PDU::ClientServerPDUPointer apduResponse_);

        PrimitiveConfirmation(const int elapsedMsec, const TransmissionDetailedTime detailedTxTime,
                    const TransmissionTime endToEndTransmissionTime_, const Uint8 forwardCongestionNotification_,
                    const Uint8 forwardCongestionNotificationEcho_, const NE::Common::Address128 serverNetworkAddress_,
                    const Isa100::Common::TSAP::TSAP_Enum serverTSAP_ID_, const ObjectID::ObjectIDEnum serverObject_,
                    const Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID_, const ObjectID::ObjectIDEnum clientObject_,
                    const Isa100::ASL::PDU::ClientServerPDUPointer apduResponse_);

        void createString();
        const std::string& toString();
};
typedef boost::shared_ptr<PrimitiveConfirmation> PrimitiveConfirmationPointer;

}
}
}

#endif /*ASL_SERVICE_PRIMITIVETYPES_H_*/
