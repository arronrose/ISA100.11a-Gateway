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

#include "Stats/Cmds.h"
#include "AL/ObjectsIDs.h"
#include "Misc/Convert/Convert.h"
#include <Common/NETypes.h>
#include "ASL/PDU/ExecuteRequestPDU.h"
#include "ASL/PDUUtils.h"
#include <Common/SmSettingsLogic.h>
#include "Model/EngineProvider.h"
#include <iomanip>

using namespace Isa100::ASL;
using namespace Isa100::ASL::PDU;
using namespace Isa100::Common;
using namespace Isa100::Common::ServiceType;
using namespace Isa100::AL;
using namespace Isa100::ASL::Services;
using namespace Isa100::Common::Objects;
using namespace Isa100::Model;
using namespace NE::Misc::Convert;

namespace Isa100 {
namespace Stats {

Cmds::Cmds() {
}

Cmds::~Cmds() {
}

void Cmds::getObjectName(ObjectID::ObjectIDEnum objectID, Uint8 tsap, std::string &objectName) {
    ObjectID::toString(objectID, (TSAP::TSAP_Enum) tsap, objectName);
}

void Cmds::logInfo(Isa100::ASL::Services::PrimitiveIndicationPointer primitiveIndication, std::string msg) {
    try {
        Isa100::ASL::PDU::ClientServerPDUPointer apdu = primitiveIndication->apduRequest;
        std::ostringstream stream;
        stream << "IN  " << std::setw(2) << Isa100::Common::ServiceType::toString(apdu->serviceInfo);
        stream << ":" << Isa100::Common::PrimitiveType::toString(apdu->primitiveType);
        stream << " ID=" << std::setw(3) << (int) apdu->appHandle;
        stream << " " << (int) primitiveIndication->clientTSAP_ID;
        stream << ">" << (int) primitiveIndication->serverTSAP_ID << "    ";
        logAddress(stream, primitiveIndication->clientNetworkAddress);
        stream << ".";
        std::string objectNameString;
        getObjectName(primitiveIndication->clientObject, primitiveIndication->clientTSAP_ID, objectNameString);
        stream << objectNameString;
        getObjectName(primitiveIndication->serverObject, primitiveIndication->serverTSAP_ID, objectNameString);
        stream << ">" << objectNameString;

        if (apdu->serviceInfo == Isa100::Common::ServiceType::execute) {
            if (apdu->primitiveType == Isa100::Common::PrimitiveType::request) {
                ExecuteRequestPDUPointer execRequest = Isa100::ASL::PDUUtils::extractExecuteRequest(apdu);
                stream << "." << (int) execRequest->methodID << "(" << bytes2string(*execRequest->parameters,
                            SmSettingsLogic::instance().cmdsMaxBytesLogged) << ")";
            }
        } else if (apdu->serviceInfo == Isa100::Common::ServiceType::write) {
            if (apdu->primitiveType == Isa100::Common::PrimitiveType::request) {
                WriteRequestPDUPointer writeRequest = Isa100::ASL::PDUUtils::extractWriteRequest(apdu);
                stream << "." << writeRequest->targetAttribute.toIndentString() << "(" << bytes2string(*writeRequest->value,
                            SmSettingsLogic::instance().cmdsMaxBytesLogged) << ")";
            }
        } else if (apdu->serviceInfo == Isa100::Common::ServiceType::read) {
            if (apdu->primitiveType == Isa100::Common::PrimitiveType::request) {
                ReadRequestPDUPointer readRequest = Isa100::ASL::PDUUtils::extractReadRequest(apdu);
                stream << "." << readRequest->targetAttribute.toIndentString() << "()";
            }
        }

        stream << " t=" << primitiveIndication->elapsedMsec / 1000 << "." << primitiveIndication->elapsedMsec % 1000 << " ";

        if (primitiveIndication->forwardCongestionNotification > 0) {
            stream << " FCN=" << (int) primitiveIndication->forwardCongestionNotification << " ";
        }

        stream << msg;
        LOG_INFO(stream.str());

    } catch (NEException& ex) {
        LOG_ERROR(ex.what());
    } catch (std::exception& ex) {
        LOG_ERROR("log PrimitiveIndicationPointer: unknown exception!" << ex.what());
    }
}

void Cmds::logInfo(Isa100::ASL::Services::PrimitiveConfirmationPointer primitiveConfirmation, std::string msg) {
    try {
        Isa100::ASL::PDU::ClientServerPDUPointer apdu = primitiveConfirmation->apduResponse;
        std::ostringstream stream;
        stream << "IN  " << std::setw(2) << Isa100::Common::ServiceType::toString(apdu->serviceInfo);
        stream << ":" << Isa100::Common::PrimitiveType::toString(apdu->primitiveType);
        stream << " ID=" << std::setw(3) << (int) apdu->appHandle;
        stream << " " << (int) primitiveConfirmation->serverTSAP_ID;
        stream << ">" << (int) primitiveConfirmation->clientTSAP_ID << "    ";
        logAddress(stream, primitiveConfirmation->serverNetworkAddress);
        stream << ".";
        //format: addr128.clientObj<ServerObj(payload)
        std::string objectNameString;
        getObjectName(primitiveConfirmation->clientObject, primitiveConfirmation->clientTSAP_ID, objectNameString);
        stream << objectNameString;
        getObjectName(primitiveConfirmation->serverObject, primitiveConfirmation->serverTSAP_ID, objectNameString);
        stream << "<" << objectNameString;

        if (apdu->serviceInfo == Isa100::Common::ServiceType::execute) {
            if (apdu->primitiveType == Isa100::Common::PrimitiveType::response) {
                ExecuteResponsePDUPointer execResponse = Isa100::ASL::PDUUtils::extractExecuteResponse(apdu);
                if (execResponse->feedbackCode != SFC::success) {
                    stream << "(SFC err=";
                    logFeedbackCode(stream, execResponse->feedbackCode);
                    stream << "; " << bytes2string(*execResponse->parameters, SmSettingsLogic::instance().cmdsMaxBytesLogged)
                                << ")";
                } else {
                    stream << "(SFC=";
                    logFeedbackCode(stream, execResponse->feedbackCode);
                    stream << "; " << bytes2string(*execResponse->parameters, SmSettingsLogic::instance().cmdsMaxBytesLogged)
                                << ")";
                }
            }
        } else if (apdu->serviceInfo == Isa100::Common::ServiceType::write) {
            if (apdu->primitiveType == Isa100::Common::PrimitiveType::response) {
                WriteResponsePDUPointer writeResponse = Isa100::ASL::PDUUtils::extractWriteResponse(apdu);
                if (writeResponse->feedbackCode != SFC::success) {
                    stream << "(SFC err=";
                    logFeedbackCode(stream, writeResponse->feedbackCode);
                    stream << ")";
                } else {
                    stream << "(SFC=";
                    logFeedbackCode(stream, writeResponse->feedbackCode);
                    stream << ")";
                }
            }
        } else if (apdu->serviceInfo == Isa100::Common::ServiceType::read) {
            if (apdu->primitiveType == Isa100::Common::PrimitiveType::response) {
                ReadResponsePDUPointer readResponse = Isa100::ASL::PDUUtils::extractReadResponse(apdu);
                if (readResponse->feedbackCode != SFC::success) {
                    stream << "(SFC err=";
                    logFeedbackCode(stream, readResponse->feedbackCode);
                    stream << "; " << bytes2string(*readResponse->value, SmSettingsLogic::instance().cmdsMaxBytesLogged) << ")";
                } else {
                    stream << "(SFC=";
                    logFeedbackCode(stream, readResponse->feedbackCode);
                    stream << "; " << bytes2string(*readResponse->value, SmSettingsLogic::instance().cmdsMaxBytesLogged) << ")";
                }
            }
        }

        stream << " t=" << primitiveConfirmation->elapsedMsec / 1000 << "." << primitiveConfirmation->elapsedMsec % 1000 << " ";

        if (primitiveConfirmation->forwardCongestionNotification > 0) {
            stream << " FCN=" << (int) primitiveConfirmation->forwardCongestionNotification << " ";
        }

        stream << msg;
        LOG_INFO(stream.str());

    } catch (NEException& ex) {
        LOG_ERROR(ex.what());
    } catch (std::exception& ex) {
        LOG_ERROR("log PrimitiveConfirmationPointer: unknown exception!" << ex.what());
    }
}

void Cmds::logInfo(Isa100::ASL::Services::PrimitiveRequestPointer primitiveRequest) {
    try {
        Isa100::ASL::PDU::ClientServerPDUPointer apdu = primitiveRequest->apduRequest;
        std::ostringstream stream;
        stream << "OUT " << std::setw(2) << Isa100::Common::ServiceType::toString(apdu->serviceInfo);
        stream << ":" << Isa100::Common::PrimitiveType::toString(apdu->primitiveType);
        stream << " ID=" << std::setw(3) << (int) apdu->appHandle;
        stream << " " << (int) primitiveRequest->clientTSAP_ID;
        stream << ">" << (int) primitiveRequest->serverTSAP_ID;
        stream << " C" << primitiveRequest->contractID << " ";
        logAddress(stream, primitiveRequest->destination);
        std::string objectNameString;
        getObjectName(primitiveRequest->clientObject, primitiveRequest->clientTSAP_ID, objectNameString);
        stream << "." << objectNameString;
        getObjectName(primitiveRequest->serverObject, primitiveRequest->serverTSAP_ID, objectNameString);
        stream << ">" << objectNameString;

        if (apdu->serviceInfo == Isa100::Common::ServiceType::execute) {
            if (apdu->primitiveType == Isa100::Common::PrimitiveType::request) {
                ExecuteRequestPDUPointer execRequest = Isa100::ASL::PDUUtils::extractExecuteRequest(apdu);
                stream << "." << (int) execRequest->methodID << "(" << bytes2string(*execRequest->parameters,
                            SmSettingsLogic::instance().cmdsMaxBytesLogged) << ")";
            }
        } else if (apdu->serviceInfo == Isa100::Common::ServiceType::write) {
            if (apdu->primitiveType == Isa100::Common::PrimitiveType::request) {
                WriteRequestPDUPointer writeRequest = Isa100::ASL::PDUUtils::extractWriteRequest(apdu);
                stream << "." << writeRequest->targetAttribute.toIndentString() << "(" << bytes2string(*writeRequest->value,
                            SmSettingsLogic::instance().cmdsMaxBytesLogged) << ")";
            }
        } else if (apdu->serviceInfo == Isa100::Common::ServiceType::read) {
            if (apdu->primitiveType == Isa100::Common::PrimitiveType::request) {
                ReadRequestPDUPointer readRequest = Isa100::ASL::PDUUtils::extractReadRequest(apdu);
                stream << "." << readRequest->targetAttribute.toIndentString() << "()";
            }
        }

        LOG_INFO(stream.str());

    } catch (NEException& ex) {
        LOG_ERROR(ex.what());
    } catch (std::exception& ex) {
        LOG_ERROR("log PrimitiveRequestPointer: unknown exception!" << ex.what());
    }
}

void Cmds::logInfo(Isa100::ASL::Services::PrimitiveResponsePointer primitiveResponse) {
    try {
        Isa100::ASL::PDU::ClientServerPDUPointer apdu = primitiveResponse->apduResponse;
        std::ostringstream stream;
        stream << "OUT " << std::setw(2) << Isa100::Common::ServiceType::toString(apdu->serviceInfo);
        stream << ":" << Isa100::Common::PrimitiveType::toString(apdu->primitiveType);
        stream << " ID=" << std::setw(3) << (int) apdu->appHandle;
        stream << " " << (int) primitiveResponse->serverTSAP_ID;
        stream << ">" << (int) primitiveResponse->clientTSAP_ID ;
        stream << " C" << primitiveResponse->contractID << " ";
        logAddress(stream, primitiveResponse->destination);
        std::string objectNameString;
        getObjectName(primitiveResponse->serverObject, primitiveResponse->serverTSAP_ID, objectNameString);
        stream << "." << objectNameString;
        getObjectName(primitiveResponse->clientObject, primitiveResponse->clientTSAP_ID, objectNameString);
        stream << ">" << objectNameString;
        if (apdu->serviceInfo == Isa100::Common::ServiceType::execute) {
            if (apdu->primitiveType == Isa100::Common::PrimitiveType::response) {
                ExecuteResponsePDUPointer execResponse = Isa100::ASL::PDUUtils::extractExecuteResponse(apdu);
                if (execResponse->feedbackCode != SFC::success) {
                    stream << "(SFC err=";
                    logFeedbackCode(stream, execResponse->feedbackCode);
                    stream << "; " << bytes2string(*execResponse->parameters, SmSettingsLogic::instance().cmdsMaxBytesLogged)
                                << ")";
                } else {
                    stream << "(SFC=";
                    logFeedbackCode(stream, execResponse->feedbackCode);
                    stream << "; " << bytes2string(*execResponse->parameters, SmSettingsLogic::instance().cmdsMaxBytesLogged)
                                << ")";
                }
            }
        } else if (apdu->serviceInfo == Isa100::Common::ServiceType::write) {
            if (apdu->primitiveType == Isa100::Common::PrimitiveType::response) {
                WriteResponsePDUPointer writeResponse = Isa100::ASL::PDUUtils::extractWriteResponse(apdu);
                if (writeResponse->feedbackCode != SFC::success) {
                    stream << "(SFC err=";
                    logFeedbackCode(stream, writeResponse->feedbackCode);
                    stream << ")";
                } else {
                    stream << "(SFC=";
                    logFeedbackCode(stream, writeResponse->feedbackCode);
                    stream << ")";
                }
            }
        } else if (apdu->serviceInfo == Isa100::Common::ServiceType::read) {
            if (apdu->primitiveType == Isa100::Common::PrimitiveType::response) {
                ReadResponsePDUPointer readResponse = Isa100::ASL::PDUUtils::extractReadResponse(apdu);
                if (readResponse->feedbackCode != SFC::success) {
                    stream << "(SFC err=";
                    logFeedbackCode(stream, readResponse->feedbackCode);
                    stream << "; " << bytes2string(*readResponse->value, SmSettingsLogic::instance().cmdsMaxBytesLogged) << ")";
                } else {
                    stream << "(SFC=";
                    logFeedbackCode(stream, readResponse->feedbackCode);
                    stream << "; " << bytes2string(*readResponse->value, SmSettingsLogic::instance().cmdsMaxBytesLogged) << ")";
                }
            }
        }
        LOG_INFO(stream.str());

    } catch (NEException& ex) {
        LOG_ERROR(ex.what());
    } catch (std::exception& ex) {
        LOG_ERROR("log PrimitiveResponsePointer: unknown exception!" << ex.what());
    }
}

void Cmds::logInfo(Isa100::ASL::Services::ASL_AlertReport_IndicationPointer alertIndication, std::string msg) {
    try {
        Isa100::ASL::PDU::ClientServerPDUPointer apdu = alertIndication->alertReport;
        std::ostringstream stream;
        stream << "IN  " << std::setw(2) << Isa100::Common::ServiceType::toString(apdu->serviceInfo) << ":";
        stream << Isa100::Common::PrimitiveType::toString(apdu->primitiveType);
        stream << " ID=" << std::setw(3) << (int) apdu->appHandle;
        stream << " " << (int) alertIndication->armoTSAP;
        stream << ">" << (int) alertIndication->sinkTSAP;
        stream << "    ";
        logAddress(stream, alertIndication->sourceAddress);
        stream << ".";

        std::string objectNameString;
        getObjectName(alertIndication->sinkObject, alertIndication->sinkTSAP, objectNameString);
        stream << objectNameString;

        AlertReportPDUPointer alertReport = Isa100::ASL::PDUUtils::extractAlertReport(apdu);
        getObjectName(alertReport->detectingObjectID,
                    (TSAP::TSAP_Enum) (alertReport->detectingObjectTransportLayerPort - 0xF0B0), objectNameString);
        stream << "(detObj=" << objectNameString;
        stream << ", cls=" << (int) alertReport->alertClass;
        stream << ", categ=" << (int) alertReport->alertCategory;
        stream << ", type=" << (int) alertReport->alertType;
        stream << "; " << bytes2string(*alertReport->alertValue, SmSettingsLogic::instance().cmdsMaxBytesLogged) << ")";

        //LOG_DEBUG("extractAlertReport:" << alertReport->toString());

        stream << " t=" << alertIndication->elapsedMsec / 1000 << "." << alertIndication->elapsedMsec % 1000 << " ";
        stream << msg;
        LOG_INFO(stream.str());

    } catch (NEException& ex) {
        LOG_ERROR(ex.what());
    } catch (std::exception& ex) {
        LOG_ERROR("log ASL_AlertReport_IndicationPointer: unknown exception!" << ex.what());
    }
}

void Cmds::logInfo(Isa100::ASL::Services::ASL_AlertReport_RequestPointer alertRequest, std::string msg) {
    try {
        Isa100::ASL::PDU::ClientServerPDUPointer apdu = alertRequest->alertReport;
        std::ostringstream stream;
        stream << "OUT  " << std::setw(2) << Isa100::Common::ServiceType::toString(apdu->serviceInfo) << ":";
        stream << Isa100::Common::PrimitiveType::toString(apdu->primitiveType);
        stream << " ID=" << std::setw(3) << (int) apdu->appHandle;
        stream << " " << (int) alertRequest->armoTSAP;
        stream << ">" << (int) alertRequest->sinkTSAP;
        stream << " C" << std::setw(2) << alertRequest->contractID << " ";
        logAddress(stream, alertRequest->destination);
        stream << " ";

        std::string objectNameString;
        //        getObjectName(alertRequest->armoObject, alertRequest->armoTSAP, objectNameString);
        //        stream << "." << objectNameString;
        //        getObjectName(alertRequest->sinkObject, alertRequest->sinkTSAP, objectNameString);
        //        stream << ">" << objectNameString;

        AlertReportPDUPointer alertReport = Isa100::ASL::PDUUtils::extractAlertReport(apdu);

        TSAP::TSAP_Enum detectingObjectTSAP = (TSAP::TSAP_Enum) (alertReport->detectingObjectTransportLayerPort - 0xF0B0);
        getObjectName((ObjectID::ObjectIDEnum) alertReport->detectingObjectID, detectingObjectTSAP, objectNameString);
        stream << "(detectingObj=" << objectNameString << ", type=" << (int) alertReport->alertType;
        stream << "; " << bytes2string(*alertReport->alertValue, SmSettingsLogic::instance().cmdsMaxBytesLogged) << ")";

        stream << msg;
        LOG_INFO(stream.str());

    } catch (NEException& ex) {
        LOG_ERROR(ex.what());
    } catch (std::exception& ex) {
        LOG_ERROR("log ASL_AlertReport_RequestPointer: unknown exception!" << ex.what());
    }
}

void Cmds::logInfo(Isa100::ASL::Services::ASL_AlertAcknowledge_IndicationPointer alertAcknowledgeIndication) {
    try {
        Isa100::ASL::PDU::ClientServerPDUPointer apdu = alertAcknowledgeIndication->alertAcknowledge;
        std::ostringstream stream;
        stream << "IN " << std::setw(2) << Isa100::Common::ServiceType::toString(apdu->serviceInfo) << ":";
        stream << Isa100::Common::PrimitiveType::toString(apdu->primitiveType);
        stream << " ID=" << std::setw(3) << (int) apdu->appHandle;
        stream << " " << (int) alertAcknowledgeIndication->destinationTSAP;
        stream << ">" << (int) alertAcknowledgeIndication->sourceTSAP;
        stream << " ";
        logAddress(stream, alertAcknowledgeIndication->sourceNetworkAddress);
        //        std::string objectNameString;
        //        getObjectName(alertAcknowledgeIndication->destinationObject, alertAcknowledgeIndication->destinationTSAP, objectNameString);
        //        stream << "." << objectNameString << "()";

        LOG_INFO(stream.str());

    } catch (NEException& ex) {
        LOG_ERROR(ex.what());
    } catch (std::exception& ex) {
        LOG_ERROR("log ASL_AlertAcknowledge_Indication: unknown exception!" << ex.what());
    }
}

void Cmds::logInfo(Isa100::ASL::Services::ASL_AlertAcknowledge_RequestPointer alertAcknowledgeRequest) {
    try {
        Isa100::ASL::PDU::ClientServerPDUPointer apdu = alertAcknowledgeRequest->alertAcknowledge;
        std::ostringstream stream;
        stream << "OUT " << std::setw(2) << Isa100::Common::ServiceType::toString(apdu->serviceInfo) << ":";
        stream << Isa100::Common::PrimitiveType::toString(apdu->primitiveType);
        stream << " ID=" << std::setw(3) << (int) apdu->appHandle;
        stream << " " << (int) alertAcknowledgeRequest->sourceTSAP;
        stream << ">" << (int) alertAcknowledgeRequest->destinationTSAP;
        stream << " C" << std::setw(2) << alertAcknowledgeRequest->contractID << " ";
        logAddress(stream, alertAcknowledgeRequest->destination);
        std::string objectNameString;
        getObjectName(alertAcknowledgeRequest->destinationObject, alertAcknowledgeRequest->destinationTSAP, objectNameString);
        stream << "." << objectNameString << "()";

        LOG_INFO(stream.str());

    } catch (NEException& ex) {
        LOG_ERROR(ex.what());
    } catch (std::exception& ex) {
        LOG_ERROR("log ASL_AlertReport_RequestPointer: unknown exception!" << ex.what());
    }
}

void Cmds::logInfo(Isa100::ASL::Services::ASL_Publish_IndicationPointer publishIndication, std::string msg) {
    try {
        Isa100::ASL::PDU::ClientServerPDUPointer apdu = publishIndication->apduPublish;
        std::ostringstream stream;
        stream << "IN  " << std::setw(2) << Isa100::Common::ServiceType::toString(apdu->serviceInfo) << ":";
        stream << Isa100::Common::PrimitiveType::toString(apdu->primitiveType);
        stream << " fn=" << std::setw(3) << (int) apdu->appHandle;
        stream << " " << (int) publishIndication->publisherTSAP;
        stream << ">" << (int) publishIndication->subscriberTSAP;
        stream << "    ";
        logAddress(stream, publishIndication->publisherAddress);
        stream << ".";
        std::string objectNameString;
        getObjectName(publishIndication->subscribingObject, publishIndication->subscriberTSAP, objectNameString);
        stream << objectNameString;

        PublishPDUPointer publishedData = Isa100::ASL::PDUUtils::extractPublishedData(apdu);
        stream << "(" << bytes2string(*publishedData->data, SmSettingsLogic::instance().cmdsMaxBytesLogged) << ")";

        stream << " t=" << publishIndication->elapsedMsec / 1000 << "." << publishIndication->elapsedMsec % 1000 << " ";
        stream << msg;
        LOG_INFO(stream.str());

    } catch (NEException& ex) {
        LOG_ERROR(ex.what());
    } catch (std::exception& ex) {
        LOG_ERROR("log ASL_Publish_IndicationPointer: unknown exception!" << ex.what());
    }
}

void Cmds::logFeedbackCode(std::ostringstream& stream, Isa100::Common::Objects::SFC::SFCEnum feedbackCode) {
    if (feedbackCode == Isa100::Common::Objects::SFC::success) {
        stream << (int) feedbackCode;
        return;
    }

    stream << (int) feedbackCode << "[" << SFC::getSFCDescription(feedbackCode) << "]";
}

void Cmds::logAddress(std::ostringstream& stream, Address32 address32) {
    NE::Model::Device * device = Isa100::Model::EngineProvider::getEngine()->getDevice(address32);

    if (!device) {
        stream  << "???[" << std::hex << address32 << std::dec << "]";
    }

    stream << device->address64.toString();
    stream  << "[" << std::hex << address32 << std::dec << "]";
}

void Cmds::logAddress(std::ostringstream& stream, const NE::Common::Address128& address128) {
    Address32 address32 = Isa100::Model::EngineProvider::getEngine()->getAddress32(address128);

    NE::Model::Device * device = Isa100::Model::EngineProvider::getEngine()->getDevice(address32);

    if (!device) {
        stream << address128.toString();
    } else {
        stream << device->address64.toString();
    }
    stream  << "[" << std::hex << address32 << std::dec << "]";

//    return stream;
}

//std::string Cmds::logAddress(const NE::Model::Address128& address128) {
//    try {
//        if (SmAddressTable::instance().existsAddress128(address128)) {
//            return SmAddressTable::instance().getAddress64(address128).toString();
//        } else {
//            return address128.toString();
//        }
//    } catch (std::exception& ex) {
//        return address128.toString();
//    }
//}

}
}
