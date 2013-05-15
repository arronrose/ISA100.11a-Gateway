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
 * ASL_Service_PrimitiveTypes.cpp
 *
 *  Created on: Oct 16, 2008
 *      Author: catalin.pop
 */
#include "ASL/Services/ASL_Service_PrimitiveTypes.h"

using namespace Isa100::Common;

namespace Isa100 {
namespace ASL {
namespace Services {

PrimitiveRequest::PrimitiveRequest(const Uint16 contractID_, const Address32 destination_,
            const Isa100::Common::ServicePriority::ServicePriority priority_, const bool discardEligible_,
            const Isa100::Common::TSAP::TSAP_Enum serverTSAP_ID_, const ObjectID::ObjectIDEnum serverObject_,
            const Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID_, const ObjectID::ObjectIDEnum clientObject_,
            const Isa100::ASL::PDU::ClientServerPDUPointer apduRequest_, bool forceUnencrypted_) :
    contractID(contractID_), destination(destination_), priority(priority_), discardEligible(discardEligible_), serverTSAP_ID(
                serverTSAP_ID_), serverObject(serverObject_), clientTSAP_ID(clientTSAP_ID_), clientObject(clientObject_),
                apduRequest(apduRequest_), forceUnencrypted(forceUnencrypted_) {
    createString();
}

PrimitiveRequest::PrimitiveRequest(const Uint16 contractID_, const Address32 destination_,
            const ServicePriority::ServicePriority priority_, const bool discardEligible_,
            const Isa100::ASL::PDU::ClientServerPDUPointer apduRequest_) :
    contractID(contractID_), destination(destination_), priority(priority_), discardEligible(discardEligible_), serverTSAP_ID(
                TSAP::TSAP_DMAP), serverObject(apduRequest_->destinationObjectID), clientTSAP_ID(TSAP::TSAP_DMAP), clientObject(
                apduRequest_->sourceObjectID), apduRequest(apduRequest_), forceUnencrypted(false) {
    createString();
}

void PrimitiveRequest::createString() {
    std::ostringstream stream;
    stream << "Request {";
    stream << "Contr: " << (int) contractID;
    stream << ", Pri: " << (int) priority;
    stream << ", DE: " << (int) discardEligible;
    std::string objectString;
    Isa100::AL::ObjectID::toString(serverObject, serverTSAP_ID, objectString);
    stream << ", SObj: " << objectString;
    Isa100::AL::ObjectID::toString(clientObject, clientTSAP_ID, objectString);
    stream << ", CObj: " << objectString;
    stream << ", Req: " << apduRequest->toString();
    stream << ", ForceUnenc: " << (forceUnencrypted ? "YES" : "NO");
    stream << "}";
    primitiveRequestString = stream.str();
}

const std::string& PrimitiveRequest::toString() {

    return primitiveRequestString;
}

PrimitiveResponse::PrimitiveResponse(const Uint16 contractID_, const Address32 destination_,
            const ServicePriority::ServicePriority priority_, const bool discardEligible_,
            const Uint8 forwardCongestionNotificationEcho_, const TSAP::TSAP_Enum serverTSAP_ID_,
            const ObjectID::ObjectIDEnum serverObject_, const TSAP::TSAP_Enum clientTSAP_ID_,
            const ObjectID::ObjectIDEnum clientObject_, const Isa100::ASL::PDU::ClientServerPDUPointer apduResponse_,
            bool forceUnencrypted_) :
    contractID(contractID_), destination(destination_), priority(priority_), discardEligible(discardEligible_),
                forwardCongestionNotificationEcho(forwardCongestionNotificationEcho_), serverTSAP_ID(serverTSAP_ID_),
                serverObject(apduResponse_->sourceObjectID), clientTSAP_ID(clientTSAP_ID_), clientObject(clientObject_),
                apduResponse(apduResponse_), forceUnencrypted(forceUnencrypted_) {
}

PrimitiveResponse::PrimitiveResponse(const Uint16 contractID_, const Address32 destination_,
            const ServicePriority::ServicePriority priority_, const bool discardEligible_,
            const Uint8 forwardCongestionNotificationEcho_, const Isa100::ASL::PDU::ClientServerPDUPointer apduResponse_) :
    contractID(contractID_), destination(destination_), priority(priority_), discardEligible(discardEligible_),
                forwardCongestionNotificationEcho(forwardCongestionNotificationEcho_), serverTSAP_ID(TSAP::TSAP_DMAP),
                serverObject(apduResponse_->destinationObjectID), clientTSAP_ID(TSAP::TSAP_DMAP), clientObject(
                            apduResponse_->sourceObjectID), apduResponse(apduResponse_), forceUnencrypted(false) {
}

std::string PrimitiveResponse::toString() {
    std::ostringstream stream;
    stream << "Response {";
    stream << "Contr: " << (int) contractID;
    stream << ", Pri: " << (int) priority;
    stream << ", DE: " << (int) discardEligible;
    stream << ", FCNE: " << (int) forwardCongestionNotificationEcho;
    //SObj=clientObject & CObj=serverObject because the PrimitiveResponse has them reversed from creation,
    //so they need to be reversed again here
    std::string objectString;
    Isa100::AL::ObjectID::toString(serverObject, serverTSAP_ID, objectString);
    stream << ", SObj: " << objectString;
    Isa100::AL::ObjectID::toString(clientObject, clientTSAP_ID, objectString);
    stream << ", CObj: " << objectString;
    stream << ", Resp: " << apduResponse->toString();
    stream << ", ForceUnenc: " << (forceUnencrypted ? "YES" : "NO");
    stream << "}";
    return stream.str();
}

PrimitiveIndication::PrimitiveIndication(int elapsedMsec_, TransmissionDetailedTime detailedTxTime_,
            TransmissionTime endToEndTransmissionTime_, Uint8 forwardCongestionNotification_,
            Address128 clientNetworkAddress_, Isa100::ASL::PDU::ClientServerPDUPointer apduRequest_) :
                elapsedMsec(elapsedMsec_), detailedTxTime(detailedTxTime_),
                endToEndTransmissionTime(endToEndTransmissionTime_), forwardCongestionNotification(forwardCongestionNotification_),
                serverTSAP_ID(TSAP::TSAP_DMAP), serverObject(apduRequest_->sourceObjectID), clientTSAP_ID(TSAP::TSAP_DMAP),
                clientObject(apduRequest_->destinationObjectID), apduRequest(apduRequest_) {
    createString();
}

PrimitiveIndication::PrimitiveIndication(int elapsedMsec_, TransmissionDetailedTime detailedTxTime_,
            TransmissionTime endToEndTransmissionTime_, Uint8 forwardCongestionNotification_,
            TSAP::TSAP_Enum serverTSAP_ID_, ObjectID::ObjectIDEnum serverObject_, Address128 clientNetworkAddress_,
            TSAP::TSAP_Enum clientTSAP_ID_, ObjectID::ObjectIDEnum clientObject_,
            Isa100::ASL::PDU::ClientServerPDUPointer apduRequest_) :
                elapsedMsec(elapsedMsec_), detailedTxTime(detailedTxTime_),
                endToEndTransmissionTime(endToEndTransmissionTime_), forwardCongestionNotification(forwardCongestionNotification_),
                serverTSAP_ID(serverTSAP_ID_), serverObject(serverObject_), clientNetworkAddress(clientNetworkAddress_),
                clientTSAP_ID(clientTSAP_ID_), clientObject(clientObject_), apduRequest(apduRequest_) {
    createString();
}

void PrimitiveIndication::createString() {
    std::ostringstream stream;
    std::string objectString;
//    std::string clientObjectId;
//    std::string timeString;
//    Type::toString(endToEndTransmissionTime, timeString);
    stream << "Indic { ";
    stream << "TrTime: " << (long long) endToEndTransmissionTime;
    stream << ", Tx_sec: " << detailedTxTime.tv_sec;
    stream << ", Tx_usec: " << detailedTxTime.tv_usec;
    stream << ", FCN: " << (int) forwardCongestionNotification;
    Isa100::AL::ObjectID::toString(serverObject, serverTSAP_ID, objectString);
    stream << ", SObj: " << objectString;
    stream << ", CAddress:" << clientNetworkAddress.toString();
    Isa100::AL::ObjectID::toString(clientObject, clientTSAP_ID, objectString);
    stream << ", CObj: " << objectString;
    stream << ", APDU: " << apduRequest->toString();
    stream << "}";
    primitiveIndicationString = stream.str();
}

const std::string& PrimitiveIndication::toString() {
    return primitiveIndicationString;

}

PrimitiveConfirmation::PrimitiveConfirmation(int elapsedMsec_, TransmissionDetailedTime detailedTxTime_,
            TransmissionTime endToEndTransmissionTime_, Uint8 forwardCongestionNotification_,
            Uint8 forwardCongestionNotificationEcho_, Address128 serverNetworkAddress_,
            Isa100::ASL::PDU::ClientServerPDUPointer apduResponse_) :
                elapsedMsec(elapsedMsec_), detailedTxTime(detailedTxTime_),
                endToEndTransmissionTime(endToEndTransmissionTime_), forwardCongestionNotification(forwardCongestionNotification_),
                forwardCongestionNotificationEcho(forwardCongestionNotificationEcho_),
                serverNetworkAddress(serverNetworkAddress_), serverTSAP_ID(TSAP::TSAP_DMAP), serverObject(
                            apduResponse_->sourceObjectID), clientTSAP_ID(TSAP::TSAP_DMAP), clientObject(
                            apduResponse_->destinationObjectID), apduResponse(apduResponse_) {
    createString();
}

PrimitiveConfirmation::PrimitiveConfirmation(int elapsedMsec_, TransmissionDetailedTime detailedTxTime_,
            TransmissionTime endToEndTransmissionTime_, Uint8 forwardCongestionNotification_,
            Uint8 forwardCongestionNotificationEcho_, Address128 serverNetworkAddress_,
            Isa100::Common::TSAP::TSAP_Enum serverTSAP_ID_, ObjectID::ObjectIDEnum serverObject_, TSAP::TSAP_Enum clientTSAP_ID_,
            ObjectID::ObjectIDEnum clientObject_, Isa100::ASL::PDU::ClientServerPDUPointer apduResponse_) :
                elapsedMsec(elapsedMsec_), detailedTxTime(detailedTxTime_),
                endToEndTransmissionTime(endToEndTransmissionTime_), forwardCongestionNotification(forwardCongestionNotification_),
                forwardCongestionNotificationEcho(forwardCongestionNotificationEcho_),
                serverNetworkAddress(serverNetworkAddress_), serverTSAP_ID(serverTSAP_ID_), serverObject(serverObject_),
                clientTSAP_ID(clientTSAP_ID_), clientObject(clientObject_), apduResponse(apduResponse_) {
    createString();
}

void PrimitiveConfirmation::createString() {
    std::ostringstream stream;
    std::string objectString;
    stream << "Conf { ";
    stream << "TrTime: " << (long long) endToEndTransmissionTime;
    stream << ", Tx_sec: " << detailedTxTime.tv_sec;
    stream << ", Tx_usec: " << detailedTxTime.tv_usec;
    stream << ", FCN: " << (int) forwardCongestionNotification;
    stream << ", FCNE: " << (int) forwardCongestionNotificationEcho;
    stream << ", SAddress: " << serverNetworkAddress.toString();
    Isa100::AL::ObjectID::toString(serverObject, serverTSAP_ID, objectString);
    stream << ", SObj: " << objectString;
    Isa100::AL::ObjectID::toString(clientObject, clientTSAP_ID, objectString);
    stream << ", CObj: " << objectString;
    stream << ", Resp: " << apduResponse->toString();
    stream << "}";
    confirmationString = stream.str();
}

const std::string& PrimitiveConfirmation::toString() {
    return confirmationString;

}

}
}
}

