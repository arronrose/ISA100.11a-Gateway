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
 * @authors catalin.pop, beniamin.tecar, sorin.bidian, andrei.petrut
 */
#include "Isa100Object.h"
#include "Misc/Marshall/NetworkOrderStream.h"
#include "Common/smTypes.h"
#include "Common/Utils/ContractUtils.h"
#include "AL/DuplicateRequests.h"
#include "Stats/Cmds.h"
#include "ASL/PDU/ExecuteRequestPDU.h"
#include "ASL/PDU/ExecuteResponsePDU.h"
#include "ASL/PDU/ReadResponsePDU.h"
#include "ASL/PDUUtils.h"
#include "Security/SecurityException.h"
#include "Model/EngineProvider.h"


using namespace Isa100::Common;
using namespace NE::Common;
using namespace Isa100::Common::Objects;
using namespace Isa100::ASL;
using namespace Isa100::ASL::PDU;
using namespace Isa100::ASL::Services;

namespace Isa100 {
namespace AL {

Isa100Object::Isa100Object(Isa100::Common::TSAP::TSAP_Enum tsap_, NE::Model::IEngine* engine_) :
    jobFinished(false), creationTime(time(NULL)), tsap(tsap_), engine(engine_) {

    static int innerObjectId = 0;
    ++innerObjectId;
    objectId = innerObjectId;

    // LOG_DEBUG(LOG_OI << "LIFE: created object:" << getObjectIDString());
}

Isa100Object::~Isa100Object() {
    // LOG_DEBUG(LOG_OI << "LIFE: destroyed object:" << getObjectIDString());
}

void Isa100Object::indicateObject(Isa100::ASL::Services::PrimitiveIndicationPointer indication_) {
    //    LOG_DEBUG(getObjectIDString() << ": indicateObject()" << indication_->toString());

    indication = indication_;

    indicateTimestamp = time(NULL); // current time
    indexRetryOperations = 0;
    jobFinished = false;
    try {

        indicate(indication);

    } catch (NE::Model::ResourceException& ex) {
        jobFinished = true;
        LOG_ERROR(LOG_OI << "indicateObject() : " << ex.what() << ", object=" << *this);
        cleanupOnError();
        sendExecuteResponseToRequester(indication, SFC::insufficientDeviceResources, BytesPointer(new Bytes()), true);
    } catch (std::exception& ex) {
        jobFinished = true;
        LOG_ERROR(LOG_OI << "indicateObject() : " << ex.what() << ", object=" << *this);
        cleanupOnError();
        sendExecuteResponseToRequester(indication, SFC::internalError, BytesPointer(new Bytes()), true);
    } catch (...) {
        jobFinished = true;
        LOG_ERROR(LOG_OI << "indicateObject: unknown exception: " << ", object=" << *this);
        cleanupOnError();
        sendExecuteResponseToRequester(indication, SFC::internalError, BytesPointer(new Bytes()), true);
    }
}

void Isa100Object::indicate(Isa100::ASL::Services::ASL_Publish_IndicationPointer indicate) {
    THROW_EX(NE::Common::NEException, "Invalid object for publish indicate.")
    ;
}

void Isa100Object::indicate(Isa100::ASL::Services::ASL_AlertReport_IndicationPointer indicate) {
    THROW_EX(NE::Common::NEException, "Invalid object for alert report indicate.")
    ;
}

void Isa100Object::indicate(Isa100::ASL::Services::ASL_AlertAcknowledge_IndicationPointer indicate) {
    THROW_EX(NE::Common::NEException, "Invalid object for alert acknowledge indicate.")
    ;
}

void Isa100Object::indicate(Isa100::ASL::Services::PrimitiveIndicationPointer indicate) {
    if (PDUUtils::isExecuteRequest(indicate->apduRequest)) {
        indicateExecute(indicate);
    } else if (PDUUtils::isWriteRequest(indicate->apduRequest)) {
        indicateWrite(indicate);
    } else if (PDUUtils::isReadRequest(indicate->apduRequest)) {
        indicateRead(indicate);
    } else {
        std::ostringstream stream;
        std::string objectIdString;
        getObjectIDString(objectIdString);
        stream << objectIdString << ": Indicate unknown! indicate=" << indicate->toString();
        LOG_ERROR(LOG_OI << stream.str());
        throw NEException(stream.str());
    }
}

void Isa100Object::indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    sendExecuteResponseToRequester(indication, SFC::invalidObjectID, BytesPointer(new Bytes()), true);

    std::ostringstream stream;
    std::string objectIdString;
    getObjectIDString(objectIdString);
    stream << objectIdString << ": Not implemented yet! indicate=" << indication->toString();
    LOG_ERROR(LOG_OI << stream.str());
}

void Isa100Object::indicateWrite(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    sendWriteResponseToRequester(indication, SFC::invalidObjectID, true);

    std::ostringstream stream;
    std::string objectIdString;
    getObjectIDString(objectIdString);
    stream << objectIdString << ": Not implemented yet! indicate=" << indication->toString();
    LOG_ERROR(LOG_OI << stream.str());
}

void Isa100Object::indicateRead(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    sendReadResponseToRequester(indication, SFC::invalidObjectID, BytesPointer(new Bytes()), true);

    std::ostringstream stream;
    std::string objectIdString;
    getObjectIDString(objectIdString);
    stream << objectIdString << ": Not implemented yet! indicate=" << indication->toString();
    LOG_ERROR(LOG_OI << stream.str());
}

void Isa100Object::confirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {

    try {

        NE::Model::Device * device = engine->getDevice(engine->getAddress32(confirm->serverNetworkAddress));

        if (!device) {
            std::string objectIdString;
            getObjectIDString(objectIdString);
            LOG_ERROR(LOG_OI << objectIdString << ": Confirm received from removed device! address=" << confirm->serverNetworkAddress.toString());
            return;
        }

        if (PDUUtils::isExecuteResponse(confirm->apduResponse)) {
            confirmExecute(confirm);
        } else if (PDUUtils::isWriteResponse(confirm->apduResponse)) {
            confirmWrite(confirm);
        } else if (PDUUtils::isReadResponse(confirm->apduResponse)) {
            confirmRead(confirm);
        } else {
            std::ostringstream stream;
            std::string objectIdString;
            getObjectIDString(objectIdString);
            stream << objectIdString << ": Confirm unknown! confirm=" << confirm->toString();
            LOG_ERROR(LOG_OI << stream.str());
            throw NEException(stream.str());
        }

    } catch (std::exception& ex) {
        jobFinished = true;
        std::string objectString;
        LOG_ERROR(LOG_OI << "confirm() : " << ex.what() << ", object=" << *this);
        cleanupOnError();
    } catch (...) {
        jobFinished = true;
        LOG_ERROR(LOG_OI << "confirm: unknown exception: " << ", object=" << *this);
        cleanupOnError();
    }
}

void Isa100Object::confirmExecute(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
    std::ostringstream stream;
    std::string objectIdString;
    getObjectIDString(objectIdString);
    stream << objectIdString << ": Not implemented yet! confirm=" << confirm->toString();
    LOG_ERROR(LOG_OI << stream.str());
    throw NEException(stream.str());
}

void Isa100Object::confirmWrite(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
    std::ostringstream stream;
    std::string objectIdString;
    getObjectIDString(objectIdString);
    stream << objectIdString << ": Not implemented yet! confirm=" << confirm->toString();
    LOG_ERROR(LOG_OI << stream.str());
    throw NEException(stream.str());
}

void Isa100Object::confirmRead(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
    std::ostringstream stream;
    std::string objectIdString;
    getObjectIDString(objectIdString);
    stream << objectIdString << ": Not implemented yet! confirm=" << confirm->toString();
    LOG_ERROR(LOG_OI << stream.str());
    throw NEException(stream.str());
}

void Isa100Object::sendTimeoutToRequester() {
    try {
        sendExecuteResponseToRequester(indication, SFC::timeout, BytesPointer(new Bytes()), true);
    } catch (NE::Common::NEException& ex) {
        LOG_ERROR(LOG_OI << ex.what());
    } catch (std::exception& ex) {
        std::string objectIdString;
        getObjectIDString(objectIdString);
        LOG_ERROR(LOG_OI << "sendTimeoutToRequester: unknown exception. Object=" << objectIdString << ex.what());
    }
}

void Isa100Object::forwardToObject(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            Isa100::Common::TSAP::TSAP_Enum destinationTSAP_ID, ObjectID::ObjectIDEnum destinationObject,
            Isa100::Common::TSAP::TSAP_Enum sourceTSAP_ID, ObjectID::ObjectIDEnum sourceObject) {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << objectIdString << ": Forwarding indicate to object=" << destinationObject << " and TSAP="
                << destinationTSAP_ID);

    Isa100::ASL::Services::PrimitiveIndicationPointer forwardedIndication(
                new Isa100::ASL::Services::PrimitiveIndication(indication->elapsedMsec, //
                            indication->detailedTxTime, //
                            indication->endToEndTransmissionTime, //
                            indication->forwardCongestionNotification, //
                            destinationTSAP_ID, //
                            destinationObject, //
                            SmSettingsLogic::instance().managerAddress128, //
                            sourceTSAP_ID, sourceObject, //
                            indication->apduRequest));

    LOG_DEBUG(LOG_OI << objectIdString << ": Indication forwarded is " << forwardedIndication->toString());
    messageDispatcher->LoopbackIndicate(forwardedIndication);
}

void Isa100Object::sendExecuteResponseToRequester(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            SFC::SFCEnum feedbackCode, BytesPointer payload, bool jobFinished_) {

    if (indication == NULL) {
        LOG_ERROR(LOG_OI << "Indication is NULL! object=" << *this);
        jobFinished = jobFinished_;
        return;
    }

        ExecuteResponsePDUPointer responsePDU(
                    new ExecuteResponsePDU(indication->forwardCongestionNotification, feedbackCode, payload));

        ClientServerPDUPointer apdu(new ClientServerPDU( //
                    Isa100::Common::PrimitiveType::response, //
                    Isa100::Common::ServiceType::execute, indication->serverObject, //
                    indication->clientObject, indication->apduRequest->appHandle));

        std::string objectIdString;
        getObjectIDString(objectIdString);

        LOG_DEBUG(LOG_OI << objectIdString << ": sendExecuteResponseToRequester: client="
                    << indication->clientNetworkAddress.toString() << ", reqID=" << (int)indication->apduRequest->appHandle << ", size=" << payload->size());

        ClientServerPDUPointer apduResponse = PDUUtils::appendExecuteResponse(apdu, responsePDU);

        Address32 destAddress32 = engine->getAddress32(indication->clientNetworkAddress);
        if (destAddress32 == 0){
            LOG_WARN("Device no more exists: " << indication->clientNetworkAddress.toString());
            jobFinished = jobFinished_;
            return;
        }

        // get the contract ID; don't need one for loopback
        Uint16 contractID = 0;
        if (!(indication->clientNetworkAddress == SmSettingsLogic::instance().managerAddress128)) {
            NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(
                        destAddress32, indication->serverTSAP_ID, indication->clientTSAP_ID);

            if (!contract_SM_Dev) {
                LOG_ERROR(LOG_OI << "NO contract between manager and " << Address_toStream(destAddress32) << ", tsap "
                        << (int) indication->serverTSAP_ID << "->" << (int) indication->clientTSAP_ID
                        << ", reqID=" << (int)indication->apduRequest->appHandle );
                jobFinished = jobFinished_;
                return;
            }

            contractID = contract_SM_Dev->contractID;
        }

        PrimitiveResponsePointer primitiveResponse(new PrimitiveResponse( //
                    contractID, // send 0 for loopback
                    destAddress32, //
                    ServicePriority::high, //
                    false, //
                    indication->forwardCongestionNotification, //fwdNotifEcho
                    indication->serverTSAP_ID, //serverTSAPID
                    indication->serverObject, //serverObj
                    indication->clientTSAP_ID,//clientTSAPID
                    indication->clientObject,//clientObj
                    apduResponse //apdu
                    ));

        messageDispatcher->Response(primitiveResponse);

    jobFinished = jobFinished_;
}

void Isa100Object::sendReadResponseToRequester(PrimitiveIndicationPointer indication, SFC::SFCEnum feedbackCode,
            BytesPointer payload, bool jobFinished_) {
    std::string objectIdString;
    getObjectIDString(objectIdString);

    ReadResponsePDUPointer responsePDU(new ReadResponsePDU(indication->forwardCongestionNotification, feedbackCode, payload));

    ClientServerPDUPointer apdu(new ClientServerPDU( //
                Isa100::Common::PrimitiveType::response, Isa100::Common::ServiceType::read, //
                indication->serverObject, indication->clientObject, indication->apduRequest->appHandle));

    LOG_DEBUG(LOG_OI << objectIdString << ": sendReadResponseToRequester: client=" << indication->clientNetworkAddress.toString()
                << ", payload size=" << payload->size());

    ClientServerPDUPointer apduResponse = PDUUtils::appendReadResponse(apdu, responsePDU);

    Address32 destAddress32 = engine->getAddress32(indication->clientNetworkAddress);

    // get the contract ID; don't need one for loopback
    Uint16 contractID = 0;
    if (!(indication->clientNetworkAddress == SmSettingsLogic::instance().managerAddress128)) {
        NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(
                    destAddress32, indication->serverTSAP_ID, indication->clientTSAP_ID);

        if (!contract_SM_Dev) {
            std::ostringstream stream;
            LOG_ERROR(LOG_OI << "NO contract between manager and " << Address_toStream(destAddress32) << ", tsap "
                    << (int) indication->serverTSAP_ID << "->" << (int) indication->clientTSAP_ID );
            jobFinished = jobFinished_;
            return;
        }

        contractID = contract_SM_Dev->contractID;
    }

    PrimitiveResponsePointer primitiveResponse(new PrimitiveResponse( //
                contractID, //
                destAddress32, //
                ServicePriority::high, //
                false, //
                indication->forwardCongestionNotification, //fwdNotifEcho
                indication->serverTSAP_ID, //serverTSAPID
                indication->serverObject, //serverObj
                indication->clientTSAP_ID,//clientTSAPID
                indication->clientObject,//clientObj
                apduResponse //apdu
                ));

    messageDispatcher->Response(primitiveResponse);

    jobFinished = jobFinished_;

}

void Isa100Object::sendWriteResponseToRequester(PrimitiveIndicationPointer indication, SFC::SFCEnum feedbackCode,
            bool jobFinished_) {
    std::string objectIdString;
    getObjectIDString(objectIdString);

    WriteResponsePDUPointer responsePDU(new WriteResponsePDU(indication->forwardCongestionNotification, feedbackCode));

    ClientServerPDUPointer apdu(new ClientServerPDU( //
                Isa100::Common::PrimitiveType::response, Isa100::Common::ServiceType::write, //
                indication->serverObject, indication->clientObject, indication->apduRequest->appHandle));

    LOG_DEBUG(LOG_OI << objectIdString << ": sendWriteResponseToRequester: client="
                << indication->clientNetworkAddress.toString());

    ClientServerPDUPointer apduResponse = PDUUtils::appendWriteResponse(apdu, responsePDU);

    Address32 destAddress32 = engine->getAddress32(indication->clientNetworkAddress);

    // get the contract ID; don't need one for loopback
    Uint16 contractID = 0;
    if (!(indication->clientNetworkAddress == SmSettingsLogic::instance().managerAddress128)) {
        NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(
                    destAddress32, indication->serverTSAP_ID, indication->clientTSAP_ID);

        if (!contract_SM_Dev) {
            LOG_ERROR(LOG_OI << "NO contract between manager and " << destAddress32 << ", tsap "
                    << (int) indication->serverTSAP_ID << "->" << (int) indication->clientTSAP_ID );
            jobFinished = jobFinished_;
            return;
        }

        contractID = contract_SM_Dev->contractID;
    }

    PrimitiveResponsePointer primitiveResponse(new PrimitiveResponse( //
                contractID, //
                destAddress32, //
                ServicePriority::high, //
                false, //
                indication->forwardCongestionNotification, //fwdNotifEcho
                indication->serverTSAP_ID, //should be indication->clientTSAP_ID
                indication->serverObject, //should be indication->clientObject
                indication->clientTSAP_ID,//should be indication->serverTSAP_ID
                indication->clientObject,//should be indication->serverObject
                apduResponse //apdu
                ));

    messageDispatcher->Response(primitiveResponse);

    jobFinished = jobFinished_;

}

void Isa100Object::lifeTimeExpired() {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_ERROR(LOG_OI << objectIdString << ": Countdown timer expired for object: " << objectIdString << ", object details="
                << *this);
}

void Isa100Object::getObjectIDString(std::string &objectId) const{
    Isa100::AL::ObjectID::toString(getObjectID(), getObjectTSAPId(), objectId);
}

std::ostream& operator<<(std::ostream& stream, const Isa100Object& object){
    std::string objectIdString;
    object.getObjectIDString(objectIdString);
    stream << "Obj: " << std::setw(5) << objectIdString;
    stream << ", objId=" << object.objectId;
    if (object.indication) {
        stream << ", Client: " << object.indication->clientNetworkAddress.toString();
        stream << ", Last_ReqID: " << (int) object.indication->apduRequest->appHandle;
    } else {
        stream << ", indication=NULL";
    }
    return stream;
}

}
}

