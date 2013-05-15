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
 * PDUUtils.cpp
 *
 *  Created on: Oct 16, 2008
 *      Author: catalin.pop
 */

#include "PDUUtils.h"
#include "Misc/Marshall/NetworkOrderStream.h"
#include "Misc/Convert/Convert.h"

using namespace NE::Misc::Marshall;
using namespace Isa100::Common;
using namespace Isa100::Common::Objects;

namespace Isa100 {
namespace ASL {

PDUUtils::PDUUtils() {
}

PDUUtils::~PDUUtils() {
}

ClientServerPDUPointer PDUUtils::appendAlertReport(ClientServerPDUPointer sourceAPDU,
            AlertReportPDUPointer alertReportPDU) {

    if (!isAlertReport(sourceAPDU)) {
        std::ostringstream msg;
        msg << "Service type mismatch in appendAlertReport.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }

    sourceAPDU->payload->push_back(alertReportPDU->alertID);

    sourceAPDU->payload->push_back((Uint8) (alertReportPDU->detectingObjectTransportLayerPort >> 8));
    sourceAPDU->payload->push_back((Uint8) alertReportPDU->detectingObjectTransportLayerPort);

    sourceAPDU->payload->push_back((Uint8) (alertReportPDU->detectingObjectID >> 8));
    sourceAPDU->payload->push_back((Uint8) alertReportPDU->detectingObjectID);

    NetworkOrderStream stream;
    alertReportPDU->detectionTime.marshall(stream);
    Bytes detectionTimeBytes(stream.ostream.str());
    sourceAPDU->payload->append(detectionTimeBytes);

    Uint8 octet = alertReportPDU->alertClass << 7;
    octet |= alertReportPDU->alarmDirection << 6;
    octet |= alertReportPDU->alertCategory << 4;
    octet |= alertReportPDU->alertPriority;

    sourceAPDU->payload->push_back(octet);

    sourceAPDU->payload->push_back(alertReportPDU->alertType);

    if (alertReportPDU->alertValueSize < 0x80) {
        sourceAPDU->payload->push_back((Uint8) (alertReportPDU->alertValueSize & 0x7F));
    } else {
        alertReportPDU->alertValueSize |= 0x8000; //set msb to 1
        sourceAPDU->payload->push_back((Uint8) (alertReportPDU->alertValueSize >> 8));
        sourceAPDU->payload->push_back((Uint8) alertReportPDU->alertValueSize);
    }

    sourceAPDU->payload->append(*(alertReportPDU->alertValue));

    //LOG_INFO("[SORIN] appendAlertReport CSPDU payload=" << NE::Misc::Convert::bytes2string(*sourceAPDU->payload));

    return sourceAPDU;
}

ClientServerPDUPointer PDUUtils::appendExecuteRequest(ClientServerPDUPointer sourceAPDU,
            ExecuteRequestPDUPointer executeRequestPDU) {
    if (!sourceAPDU) {
        std::ostringstream msg;
        msg << "NULL pointer in appendExecuteRequest.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    if (sourceAPDU->primitiveType != PrimitiveType::request) {
        std::ostringstream msg;
        msg << "Primitive type mismatch in appendExecuteRequest.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    if (sourceAPDU->serviceInfo != ServiceType::execute) {
        std::ostringstream msg;
        msg << "Service type mismatch in appendExecuteRequest.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    sourceAPDU->payload->push_back(executeRequestPDU->methodID);
    Uint16 parametersSize;
    if (executeRequestPDU->parameters) {
        parametersSize = (Uint16) executeRequestPDU->parameters->size();
    } else {
        //if parameters = null pointer set size to zero
        parametersSize = 0;
    }
    // append length of request parameters
    if (parametersSize <= 0x7F) { //seven-bit length field
        sourceAPDU->payload->push_back((Uint8) parametersSize);
    } else { //fifteen-bit length
        parametersSize |= 0x8000; //set msb to 1
        sourceAPDU->payload->push_back((Uint8)(parametersSize >> 8));
        sourceAPDU->payload->push_back((Uint8) parametersSize);
    }
    sourceAPDU->payload->append(*(executeRequestPDU->parameters));

    return sourceAPDU;
}

ClientServerPDUPointer PDUUtils::appendExecuteResponse(ClientServerPDUPointer sourceAPDU,
            ExecuteResponsePDUPointer executeResponsePDU) {
    if (!sourceAPDU) {
        std::ostringstream msg;
        msg << "NULL pointer in appendExecuteResponse.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    if (sourceAPDU->primitiveType != PrimitiveType::response) {
        std::ostringstream msg;
        msg << "Primitive type mismatch in appendExecuteResponse.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    if (sourceAPDU->serviceInfo != ServiceType::execute) {
        std::ostringstream msg;
        msg << "Service type mismatch in appendExecuteResponse.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    sourceAPDU->payload->push_back((Uint8) executeResponsePDU->forwardCongestionNotificationEcho);
    sourceAPDU->payload->push_back((Uint8) executeResponsePDU->feedbackCode);
    Uint16 parametersSize;
    if (executeResponsePDU->parameters) {
        parametersSize = (Uint16) executeResponsePDU->parameters->size();
    } else {
        //if parameters = null pointer set size to zero
        parametersSize = 0;
    }
    // append length of request parameters
    if (parametersSize <= 0x7F) { //seven-bit length field
        sourceAPDU->payload->push_back((Uint8) parametersSize);
    } else { //fifteen-bit length
        parametersSize |= 0x8000; //set msb to 1
        sourceAPDU->payload->push_back((Uint8)(parametersSize >> 8));
        sourceAPDU->payload->push_back((Uint8) parametersSize);
    }
    sourceAPDU->payload->append(*(executeResponsePDU->parameters));

    return sourceAPDU;
}

ClientServerPDUPointer PDUUtils::appendWriteRequest(ClientServerPDUPointer sourceAPDU,
            WriteRequestPDUPointer writeRequestPDU) {
    if (!sourceAPDU) {
        std::ostringstream msg;
        msg << "NULL pointer in appendExecuteRequest.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    if (sourceAPDU->primitiveType != PrimitiveType::request) {
        std::ostringstream msg;
        msg << "Primitive type mismatch in appendExecuteRequest.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    if (sourceAPDU->serviceInfo != ServiceType::write) {
        std::ostringstream msg;
        msg << "Service type mismatch in appendExecuteRequest.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    NetworkOrderStream stream;
    writeRequestPDU->targetAttribute.marshall(stream);
    Bytes marshalledAttribute = stream.ostream.str();
    sourceAPDU->payload->append(marshalledAttribute);
    Uint16 parametersSize;
    if (writeRequestPDU->value) {
        parametersSize = (Uint16) writeRequestPDU->value->size();
    } else {
        //if parameters = null pointer set size to zero
        parametersSize = 0;
    }
    // append length of request parameters
    if (parametersSize <= 0x7F) { //seven-bit length field
        sourceAPDU->payload->push_back((Uint8) parametersSize);
    } else { //fifteen-bit length
        parametersSize |= 0x8000; //set msb to 1
        sourceAPDU->payload->push_back((Uint8)(parametersSize >> 8));
        sourceAPDU->payload->push_back((Uint8) parametersSize);
    }
    sourceAPDU->payload->append(*(writeRequestPDU->value));

    return sourceAPDU;
}

ClientServerPDUPointer PDUUtils::appendWriteResponse(ClientServerPDUPointer sourceAPDU,
            WriteResponsePDUPointer writeResponsePDU) {
    if (!sourceAPDU) {
        std::ostringstream msg;
        msg << "NULL pointer in appendExecuteResponse.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    if (sourceAPDU->primitiveType != PrimitiveType::response) {
        std::ostringstream msg;
        msg << "Primitive type mismatch in appendExecuteResponse.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    if (sourceAPDU->serviceInfo != ServiceType::write) {
        std::ostringstream msg;
        msg << "Service type mismatch in appendExecuteResponse.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    sourceAPDU->payload->push_back((Uint8) writeResponsePDU->forwardCongestionNotificationEcho);
    sourceAPDU->payload->push_back((Uint8) writeResponsePDU->feedbackCode);

    return sourceAPDU;
}

ClientServerPDUPointer PDUUtils::appendReadRequest(ClientServerPDUPointer sourceAPDU,
            ReadRequestPDUPointer readRequestPDU) {
    if (!sourceAPDU) {
        std::ostringstream msg;
        msg << "NULL pointer in appendReadRequest.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    if (sourceAPDU->primitiveType != PrimitiveType::request) {
        std::ostringstream msg;
        msg << "Primitive type mismatch in appendReadRequest.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    if (sourceAPDU->serviceInfo != ServiceType::read) {
        std::ostringstream msg;
        msg << "Service type mismatch in appendReadRequest.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    NetworkOrderStream stream;
    readRequestPDU->targetAttribute.marshall(stream);
    Bytes marshalledAttribute = stream.ostream.str();
    sourceAPDU->payload->append(marshalledAttribute);

    return sourceAPDU;
}

ClientServerPDUPointer PDUUtils::appendReadResponse(ClientServerPDUPointer sourceAPDU,
            ReadResponsePDUPointer readResponsePDU) {
    if (!sourceAPDU) {
        std::ostringstream msg;
        msg << "NULL pointer in appendExecuteResponse.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    if (sourceAPDU->primitiveType != PrimitiveType::response) {
        std::ostringstream msg;
        msg << "Primitive type mismatch in appendExecuteResponse.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    if (sourceAPDU->serviceInfo != ServiceType::read) {
        std::ostringstream msg;
        msg << "Service type mismatch in appendExecuteResponse.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
    sourceAPDU->payload->push_back((Uint8) readResponsePDU->forwardCongestionNotificationEcho);
    sourceAPDU->payload->push_back((Uint8) readResponsePDU->feedbackCode);
    Uint16 parametersSize;
    if (readResponsePDU->value) {
        parametersSize = (Uint16) readResponsePDU->value->size();
    } else {
        //if parameters = null pointer set size to zero
        parametersSize = 0;
    }
    // append length of request parameters
    if (parametersSize <= 0x7F) { //seven-bit length field
        sourceAPDU->payload->push_back((Uint8) parametersSize);
    } else { //fifteen-bit length
        parametersSize |= 0x8000; //set msb to 1
        sourceAPDU->payload->push_back((Uint8)(parametersSize >> 8));
        sourceAPDU->payload->push_back((Uint8) parametersSize);
    }
    sourceAPDU->payload->append(*(readResponsePDU->value));

    return sourceAPDU;
}

/*
 * assign(string to use, index of the first character, number of characters to use)
 */

ExecuteRequestPDUPointer PDUUtils::extractExecuteRequest(ClientServerPDUPointer sourceAPDU) {
    if (isExecuteRequest(sourceAPDU)) {
        Uint16 index = 0;
        Uint8 methodID = sourceAPDU->payload->at(index);
        //executeRequestPDU->methodID =
        BytesPointer parameters(new Bytes());
        Uint16 parametersSize = sourceAPDU->payload->at(++index);
        //check if parameters size is fifteen-bit length
        if ((parametersSize & 0x80) != 0) {
            parametersSize = (parametersSize << 8) | sourceAPDU->payload->at(++index);
            parametersSize &= 0x7FFF; //fifteen-bit length
            parameters->assign(*(sourceAPDU->payload), ++index, parametersSize);
        } else { //seven-bit length
            parameters->assign(*(sourceAPDU->payload), ++index, parametersSize);
        }
        ExecuteRequestPDUPointer executeRequestPDU(new ExecuteRequestPDU(methodID, parameters));
        return executeRequestPDU;
    } else {
        std::ostringstream msg;
        msg << "Service type mismatch in extractExecuteRequest.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
}

ExecuteResponsePDUPointer PDUUtils::extractExecuteResponse(ClientServerPDUPointer sourceAPDU) {
    if (isExecuteResponse(sourceAPDU)) {
        Uint16 index = 0;
        bool forwardCongestionNotificationEcho = sourceAPDU->payload->at(index);
        //executeResponsePDU->forwardCongestionNotificationEcho = sourceAPDU->payload->at(index);
        SFC::SFCEnum feedbackCode = (SFC::SFCEnum) sourceAPDU->payload->at(++index);
        //executeResponsePDU->feedbackCode = (SFC::ServiceFeedbackCode)sourceAPDU->payload->at(++index);
        BytesPointer parameters(new Bytes());
        Uint16 parametersSize = sourceAPDU->payload->at(++index);
        //check if parameters size is fifteen-bit length
        if ((parametersSize & 0x80) != 0) {
            parametersSize = (parametersSize << 8) | sourceAPDU->payload->at(++index);
            parametersSize &= 0x7FFF; //fifteen-bit length
            parameters->assign(*(sourceAPDU->payload), ++index, parametersSize);
        } else { //seven-bit length
            parameters->assign(*(sourceAPDU->payload), ++index, parametersSize);
        }
        ExecuteResponsePDUPointer executeResponsePDU(new ExecuteResponsePDU(forwardCongestionNotificationEcho,
                    feedbackCode, parameters));
        return executeResponsePDU;
    } else {
        std::ostringstream msg;
        msg << "Service type mismatch in extractExecuteResponse.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
}

WriteRequestPDUPointer PDUUtils::extractWriteRequest(ClientServerPDUPointer sourceAPDU) {
    if (isWriteRequest(sourceAPDU)) {
        Uint16 index = 0;
        //extract targetAttribute
        ExtensibleAttributeIdentifier targetAttribute;
        Uint16 attributeSize = targetAttribute.getSize(sourceAPDU->payload, index);
        Bytes attribute;
        attribute.assign(*(sourceAPDU->payload), index, attributeSize);
        NetworkOrderStream stream(attribute);
        targetAttribute.unmarshall(stream);
        //writeRequestPDU->targetAttribute.unmarshall(stream);
        index += attributeSize;
        BytesPointer value(new Bytes());
        Uint16 parametersSize = sourceAPDU->payload->at(index);
        //check if parameters size is fifteen-bit length
        if ((parametersSize & 0x80) != 0) {
            parametersSize = (parametersSize << 8) | sourceAPDU->payload->at(++index);
            parametersSize &= 0x7FFF; //fifteen-bit length
            value->assign(*(sourceAPDU->payload), ++index, parametersSize);
        } else { //seven-bit length
            value->assign(*(sourceAPDU->payload), ++index, parametersSize);
        }
        WriteRequestPDUPointer writeRequestPDU(new WriteRequestPDU(targetAttribute, value));
        return writeRequestPDU;
    } else {
        std::ostringstream msg;
        msg << "Service type mismatch in extractWriteRequest.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
}

WriteResponsePDUPointer PDUUtils::extractWriteResponse(ClientServerPDUPointer sourceAPDU) {
    if (isWriteResponse(sourceAPDU)) {
        Uint16 index = 0;
        bool forwardCongestionNotificationEcho = sourceAPDU->payload->at(index);
        //writeResponsePDU->forwardCongestionNotificationEcho = sourceAPDU->payload->at(index);
        SFC::SFCEnum feedbackCode = (SFC::SFCEnum) sourceAPDU->payload->at(++index);
        //writeResponsePDU->feedbackCode = (SFC::ServiceFeedbackCode) sourceAPDU->payload->at(++index);
        WriteResponsePDUPointer writeResponsePDU(new WriteResponsePDU(forwardCongestionNotificationEcho, feedbackCode));
        return writeResponsePDU;
    } else {
        std::ostringstream msg;
        msg << "Service type mismatch in extractWriteResponse.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
}

ReadRequestPDUPointer PDUUtils::extractReadRequest(ClientServerPDUPointer sourceAPDU) {
    if (isReadRequest(sourceAPDU)) {
        //extract targetAttribute
        ExtensibleAttributeIdentifier targetAttribute;
        Uint16 attributeSize = targetAttribute.getSize(sourceAPDU->payload, 0);
        Bytes attribute;
        attribute.assign(*(sourceAPDU->payload), 0, attributeSize);
        NetworkOrderStream stream(attribute);
        targetAttribute.unmarshall(stream);
        //readRequestPDU->targetAttribute.unmarshall(stream);
        ReadRequestPDUPointer readRequestPDU(new ReadRequestPDU(targetAttribute));
        return readRequestPDU;
    } else {
        std::ostringstream msg;
        msg << "Service type mismatch in extractReadRequest.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
}

ReadResponsePDUPointer PDUUtils::extractReadResponse(ClientServerPDUPointer sourceAPDU) {
    if (isReadResponse(sourceAPDU)) {
        Uint16 index = 0;
        bool forwardCongestionNotificationEcho = sourceAPDU->payload->at(index);
        SFC::SFCEnum feedbackCode = (SFC::SFCEnum) sourceAPDU->payload->at(++index);
        //readResponsePDU->forwardCongestionNotificationEcho = sourceAPDU->payload->at(index);
        //readResponsePDU->feedbackCode = (SFC::ServiceFeedbackCode) sourceAPDU->payload->at(++index);
        BytesPointer value(new Bytes());
        Uint16 parametersSize = sourceAPDU->payload->at(++index);
        //check if parameters size is fifteen-bit length
        if ((parametersSize & 0x80) != 0) {
            parametersSize = (parametersSize << 8) | sourceAPDU->payload->at(++index);
            parametersSize &= 0x7FFF; //fifteen-bit length
            value->assign(*(sourceAPDU->payload), ++index, parametersSize);
        } else { //seven-bit length
            value->assign(*(sourceAPDU->payload), ++index, parametersSize);
        }
        ReadResponsePDUPointer readResponsePDU(new ReadResponsePDU(forwardCongestionNotificationEcho, feedbackCode,
                    value));
        return readResponsePDU;
    } else {
        std::ostringstream msg;
        msg << "Service type mismatch in extractReadResponse.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
}

AlertReportPDUPointer PDUUtils::extractAlertReport(ClientServerPDUPointer sourceAPDU) {
    if (isAlertReport(sourceAPDU)) {
        Uint16 index = 0;

        Uint8 alertID = sourceAPDU->payload->at(index);
        Uint16 detectingObjectTransportLayerPort = sourceAPDU->payload->at(++index);
        detectingObjectTransportLayerPort = (detectingObjectTransportLayerPort << 8) | sourceAPDU->payload->at(++index);
        Uint16 detectingObjectID = sourceAPDU->payload->at(++index);
        detectingObjectID = (detectingObjectID << 8) | sourceAPDU->payload->at(++index);

        //extract TAINetworkTimeValue
        TAINetworkTimeValue detectionTime;
        Bytes time;
        time.assign(*(sourceAPDU->payload), ++index, 6);
        NetworkOrderStream stream(time);
        detectionTime.unmarshall(stream);
        index += 6;

        Uint8 octet = sourceAPDU->payload->at(index);
        Uint8 alertClass = octet >> 7;
        Uint8 alarmDirection = (octet & 0x40) >> 6;
        Uint8 alertCategory = (octet & 0x30) >> 4;
        Uint8 alertPriority = octet & 0x0F;

        Uint8 alertType = sourceAPDU->payload->at(++index);

        BytesPointer value(new Bytes());
        Uint16 parametersSize = sourceAPDU->payload->at(++index);
        //check if parameters size is fifteen-bit length
        if ((parametersSize & 0x80) != 0) {
            parametersSize = (parametersSize << 8) | sourceAPDU->payload->at(++index);
            parametersSize &= 0x7FFF; //fifteen-bit length
            value->assign(*(sourceAPDU->payload), ++index, parametersSize);
        } else { //seven-bit length
            value->assign(*(sourceAPDU->payload), ++index, parametersSize);
        }

        AlertReportPDUPointer alertReportPDU(
        		new AlertReportPDU(alertID, //
        				detectingObjectTransportLayerPort, //
        				detectingObjectID, //
        				detectionTime, //
						alertClass, //
						alarmDirection, //
						alertCategory, //
						alertPriority, //
						alertType, //
						value));
        return alertReportPDU;

    } else {
        std::ostringstream msg;
        msg << "Service type mismatch in extractAlertReport.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
}

CascadedAlertReportPDUPointer PDUUtils::extractCascadingAlertReport(BytesPointer pseudoPayload) {
    NetworkOrderStream stream(*pseudoPayload);

    //extract address128
    NE::Common::Address128 address128;
    address128.unmarshall(stream);

    //create a ClientServerPDUPointer with address128 removed from pseudoPayload and call extractAlertReport()
    //after address128 the next 2 bytes contain - primitiveType + serviceInfo + sourceObjectID + destinationObjectID

    BytesPointer newPayload(new Bytes());
    try {
        newPayload->assign(*pseudoPayload, 18, AlertReportPDU::getSize(pseudoPayload, 18));
        LOG_DEBUG("extractCascadingAlertReport - payload for original alert: "
                    << NE::Misc::Convert::bytes2string(*newPayload));

    } catch(std::exception& ex) {
        LOG_ERROR("extractCascadingAlertReport - " << ex.what());
    }

    ClientServerPDUPointer newPDU(new ClientServerPDU(Isa100::Common::PrimitiveType::request, //
                Isa100::Common::ServiceType::alertReport, //
                0, //not needed
                0, //not needed
                0, //not needed
                newPayload));

    AlertReportPDUPointer alertReport = extractAlertReport(newPDU);

    CascadedAlertReportPDUPointer cascadedAlert(new CascadedAlertReportPDU(address128, *alertReport));

    //cascadedAlert->originalDeviceAddress = address128;
    //cascadedAlert->alertReportPDU = *alertReport;

    return cascadedAlert;
}

PublishPDUPointer PDUUtils::extractPublishedData(ClientServerPDUPointer sourceAPDU) {
    if (isPublish(sourceAPDU)) {
        Uint16 index = 0;
        Uint8 freshnessSequenceNumber = sourceAPDU->payload->at(index);
        BytesPointer data(new Bytes());
        Uint16 parametersSize = sourceAPDU->payload->at(++index);
        //check if parameters size is fifteen-bit length
        if ((parametersSize & 0x80) != 0) {
            parametersSize = (parametersSize << 8) | sourceAPDU->payload->at(++index);
            parametersSize &= 0x7FFF; //fifteen-bit length
            data->assign(*(sourceAPDU->payload), ++index, parametersSize);
        } else { //seven-bit length
            data->assign(*(sourceAPDU->payload), ++index, parametersSize);
        }
        //LOG_DEBUG("data=" << NE::Misc::Convert::bytes2string(*data) << ", sourceAPDU->payload="
        //            << NE::Misc::Convert::bytes2string(*sourceAPDU->payload));
        PublishPDUPointer publishPDU(new PublishPDU(freshnessSequenceNumber, data));
        return publishPDU;
    } else {
        std::ostringstream msg;
        msg << "Service type mismatch in extractPublishedData.";
        LOG_ERROR(msg.str());
        throw NE::Common::NEException(msg.str());
    }
}

bool PDUUtils::isExecuteRequest(ClientServerPDUPointer sourceAPDU) {
    return (sourceAPDU->serviceInfo == Isa100::Common::ServiceType::execute) && (sourceAPDU->primitiveType
                == Isa100::Common::PrimitiveType::request);
}

bool PDUUtils::isWriteRequest(ClientServerPDUPointer sourceAPDU) {
    return (sourceAPDU->serviceInfo == Isa100::Common::ServiceType::write) && (sourceAPDU->primitiveType
                == Isa100::Common::PrimitiveType::request);
}

bool PDUUtils::isReadRequest(ClientServerPDUPointer sourceAPDU) {
    return (sourceAPDU->serviceInfo == Isa100::Common::ServiceType::read) && (sourceAPDU->primitiveType
                == Isa100::Common::PrimitiveType::request);
}

bool PDUUtils::isExecuteResponse(ClientServerPDUPointer sourceAPDU) {
    return (sourceAPDU->serviceInfo == Isa100::Common::ServiceType::execute) && (sourceAPDU->primitiveType
                == Isa100::Common::PrimitiveType::response);
}

bool PDUUtils::isWriteResponse(ClientServerPDUPointer sourceAPDU) {
    return (sourceAPDU->serviceInfo == Isa100::Common::ServiceType::write) && (sourceAPDU->primitiveType
                == Isa100::Common::PrimitiveType::response);
}

bool PDUUtils::isReadResponse(ClientServerPDUPointer sourceAPDU) {
    return (sourceAPDU->serviceInfo == Isa100::Common::ServiceType::read) && (sourceAPDU->primitiveType
                == Isa100::Common::PrimitiveType::response);
}

bool PDUUtils::isAlertReport(ClientServerPDUPointer sourceAPDU) {
    return (sourceAPDU->serviceInfo == Isa100::Common::ServiceType::alertReport) && (sourceAPDU->primitiveType
                == Isa100::Common::PrimitiveType::request);
}

bool PDUUtils::isPublish(ClientServerPDUPointer sourceAPDU) {
    return (sourceAPDU->serviceInfo == Isa100::Common::ServiceType::publish) && (sourceAPDU->primitiveType
                == Isa100::Common::PrimitiveType::request);
}

}
}
