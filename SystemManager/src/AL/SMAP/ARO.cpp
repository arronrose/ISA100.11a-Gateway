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
 * @author sorin.bidian, beniamin.tecar, catalin.pop
 */
#include "ARO.h"

#include "AL/Types/AlertTypes.h"
#include "ASL/PDUUtils.h"
#include "Common/AttributesIDs.h"
#include "Common/SmSettingsLogic.h"
#include "Common/MethodsIDs.h"
#include "Common/HandleFactory.h"
#include "Misc/Convert/Convert.h"
#include "Model/EngineProvider.h"
#include "Model/DllStructures.h"
#include "Stats/Isa100SMStateLog.h"

using namespace Isa100::ASL;
using namespace Isa100::ASL::Services;
using namespace Isa100::AL::Types;
using namespace Isa100::Common::Objects;
using namespace Isa100::Model;
using namespace NE::Model;

namespace Isa100 {
namespace AL {
namespace SMAP {

ARO::ARO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine_) :
    Isa100Object(tsap, engine_) {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: created object:" << objectIdString);
}

ARO::~ARO() {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: destroyed object:" << objectIdString);
}

void ARO::logAlert(Device* device, ASL::PDU::AlertReportPDUPointer alertPDU) {

    std::ostringstream stream;

    stream << std::endl;
    stream << std::setw(3) << "ALR";
    stream << std::setw(2) << "ID";
    stream << std::setw(20) << "Alert from";
    stream << std::setw(7) << "DetObj";
    stream << std::setw(6) << "Cls";
    stream << std::setw(9) << "Categ";
    stream << std::setw(16) << "Type";
    stream << std::setw(5) << "Dir";
    stream << " Data";
    stream << std::endl;

    stream << std::setw(3) << "ALR";
    stream << std::setw(2) << (int) alertPDU->alertID;

    // address64
    stream << std::setw(20) << device->address64.toString();

    // detObj
    std::string objectId;
    //    const detectingObjectID = alertPDU->detectingObjectID;
    ObjectID::toString((Isa100::AL::ObjectID::ObjectIDEnum) alertPDU->detectingObjectID,
                (TSAP::TSAP_Enum) (alertPDU->detectingObjectTransportLayerPort - 0xF0B0), objectId);
    stream << std::setw(7) << objectId;

    //stream << ", cls=" << (int) alertPDU->alertClass;
    stream << std::setw(6);
    if (AlertClass::event == alertPDU->alertClass) {
        stream << "EV";
    } else if (AlertClass::alarm == alertPDU->alertClass) {
        stream << "AL";
    } else {
        //stream << (int) alertPDU->alertClass;
        stream << "N/A";
    }

    //stream << ", categ=" << (int) alertPDU->alertCategory;
    stream << std::setw(9);
    if (AlertCategory::deviceDiagnostic == alertPDU->alertCategory) {
        stream << "devDiag";
    } else if (AlertCategory::communicationsDiagnostic == alertPDU->alertCategory) {
        stream << "commDiag";
    } else if (AlertCategory::security == alertPDU->alertCategory) {
        stream << "security";
    } else if (AlertCategory::process == alertPDU->alertCategory) {
        stream << "process";
    } else {
        //stream << (int) alertPDU->alertCategory;
        stream << "N/A";
    }

    //stream << ", type=" << (int) alertPDU->alertType;
    stream << std::setw(16);
    if (alertPDU->detectingObjectID == ObjectID::ID_ARMO) {
        if (alertPDU->alertType == ARMOAlerts::Alarm_Recovery_Start) {
            stream << "AlrRcvrStrt";
        } else if (alertPDU->alertType == ARMOAlerts::Alarm_Recovery_End) {
            stream << "AlrRcvrEnd";
        } else {
            stream << "N/A";
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_ASLMO) {
        if (alertPDU->alertType == ASLMOAlerts::MalformedAPDUCommunicationAlert) {
            stream << "MlfrmdComm";
        } else {
            stream << "N/A";
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_DLMO) {
        if (alertPDU->alertType == DLMOAlerts::DL_Connectivity) {
            stream << "DlConn";
        } else if (alertPDU->alertType == DLMOAlerts::NeighborDiscovery) {
            stream << "NghbrDscvry";
        } else {
            stream << "N/A";
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_NLMO) {
        if (alertPDU->alertType == NLMOAlerts::NLDroppedPDU) {
            stream << "NLDropped";
        } else {
            stream << "N/A";
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_TLMO) {
        if (alertPDU->alertType == TLMOAlerts::IllegalUseOfPort) {
            stream << "IllglUsePort";
        }
        if (alertPDU->alertType == TLMOAlerts::TPDUonUnregisteredPort) {
            stream << "TpduUnregPrt";
        }
        if (alertPDU->alertType == TLMOAlerts::TPDUoutOfSecurityPolicies) {
            stream << "TpduOutSecPol";
        } else {
            stream << "N/A";
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_DMO) {
        if (alertPDU->alertType == DMOAlerts::Device_Power_Status_Check) {
            stream << "DevPwrStatus";
        } else if (alertPDU->alertType == DMOAlerts::Device_Restart) {
            stream << "DevRestart";
        } else {
            stream << "N/A";
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_DSMO) {
        if (alertPDU->alertType == DSMOAlerts::Security_MPDU_Fail_Rate_Exceeded) {
            stream << "SecMpduFail";
        } else if (alertPDU->alertType == DSMOAlerts::Security_TPDU_Fail_Rate_Exceeded) {
            stream << "SecTpduFail";
        } else if (alertPDU->alertType == DSMOAlerts::Security_Key_Update_Fail_Rate_Exceeded) {
            stream << "SecKeyUpdtFail";
        } else {
            stream << "N/A";
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_DPSO) {
        if (alertPDU->alertType == DPSOAlerts::Not_On_Whitelist_Alert) {
            stream << "DevPwrStatus";
        } else if (alertPDU->alertType == DPSOAlerts::Inadequate_Join_Capability_Alert) {
            stream << "InadeqJoinCpblty";
        } else {
            stream << "N/A";
        }
    } else {
        stream << "N/A";
    }

    //stream << ", direction=" << (int) alertPDU->alarmDirection;
    stream << std::setw(5);
    if (alertPDU->alarmDirection == 0) {
        stream << "out";
    } else {
        stream << "in";
    }

    //stream << ", data=" << NE::Misc::Convert::bytes2string(*alertPDU->alertValue);
    stream << " ";

    NetworkOrderStream alertDataStream(alertPDU->alertValue);

    if (alertPDU->detectingObjectID == ObjectID::ID_ASLMO) {
        if (alertPDU->alertType == ASLMOAlerts::MalformedAPDUCommunicationAlert) {
            Address128 srcAddr;
            Uint16 treshold;
            TAINetworkTimeValue interval;
            try {
                srcAddr.unmarshall(alertDataStream);
                alertDataStream.read(treshold);
                interval.unmarshall(alertDataStream);
            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR("MalformedAPDUCommunicationAlert: " << ex.what());
            }
            stream << "srcAddr=" << srcAddr.toString() << ", tresh=" << (int) treshold << ", interval="
                        << (int) interval.currentTAI << "," << (int) interval.fractionalTAI << "s";
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_DLMO) {
        if (alertPDU->alertType == DLMOAlerts::DL_Connectivity) {
            Uint8 attributeNumber;
            alertDataStream.read(attributeNumber);
            if (attributeNumber == Isa100::Common::DLMO_Attributes::ChannelDiag) {
                ChannelDiag channelDiag;
                try {
                    channelDiag.unmarshall(alertDataStream);
                } catch (NE::Misc::Marshall::StreamException& ex) {
                    LOG_ERROR("DL_Connectivity alert: " << ex.what());
                }
                stream << "trans: ";
                stream << channelDiag.count;
                stream << " ch(noAck|ccaBackoff): ";
                for (int i = 0; i <= 15; ++i) {
                    stream << "c" << i + 11; // channels 11 to 26
                    stream << "(" << (int) channelDiag.channelTransmissionList[i].noAck;
                    stream << "|" << (int) channelDiag.channelTransmissionList[i].ccaBackoff << ") ";
                }

            } else if (attributeNumber == Isa100::Common::DLMO_Attributes::NeighborDiag) {
                Uint16 index = Isa100::Common::Utils::unmarshallExtDLUint(alertDataStream);
                Address32 neighborAddress32 = engine->createAddress32(device->capabilities.dllSubnetId, index);
                Uint8 diagLevel = engine->getDiagLevel(device->address32, neighborAddress32);
                NeighborDiag neighborDiag(index);
                try {
                    neighborDiag.unmarshall(alertDataStream, diagLevel);
                } catch (NE::Misc::Marshall::StreamException& ex) {
                    LOG_ERROR("DL_Connectivity alert: " << ex.what());
                }
                stream << "addr(txSccs|txFld|txCCA_B|txNACK): ";
                stream << std::hex << (int) index;
                stream << "(" << neighborDiag.summary.txSuccessful << "|" << neighborDiag.summary.txFailed << "|"
                            << neighborDiag.summary.txCCA_Backoff << "|" << neighborDiag.summary.txNACK << ") ";
            }
        } else if (alertPDU->alertType == DLMOAlerts::NeighborDiscovery) {
            Candidates candidates;
            try {
                candidates.unmarshall(alertDataStream);
            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR("NeighborDiscovery alert: " << ex.what());
            }
            stream << "addr(rsqi): ";
            for (int i = 0; i < candidates.candidatesCount; ++i) {
                stream << std::hex << (int) candidates.neighborRadioList[i].neighbor;
                stream << "(" << (int) candidates.neighborRadioList[i].radio << ") ";
            }
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_NLMO) {
        if (alertPDU->alertType == NLMOAlerts::NLDroppedPDU) {
            Uint8 size;
            Uint8 diagnosticCode;
            Bytes value;
            try {
                alertDataStream.read(size);
                alertDataStream.read(diagnosticCode);
                alertDataStream.read(value, size - 2); // without size and diagnosticCode
            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR("NLDroppedPDUAlert: " << ex.what());
            }
            if (diagnosticCode == 1) {
                stream << "destination unreachable; ";
            } else if (diagnosticCode == 2) {
                stream << "fragmentation error; ";
            } else if (diagnosticCode == 3) {
                stream << "reassembly timeout; ";
            } else if (diagnosticCode == 4) {
                stream << "hop limit reached; ";
            } else if (diagnosticCode == 5) {
                stream << "header errors; ";
            } else if (diagnosticCode == 6) {
                stream << "next hop unreachable; ";
            } else if (diagnosticCode == 7) {
                stream << "out of memory; ";
            } else if (diagnosticCode == 8) {
                stream << "NPDU too long; ";
            } else {
                stream << "code=" << (int) diagnosticCode << "; ";
            }
            stream << "NPDU header=" << NE::Misc::Convert::bytes2string(value);
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_TLMO) {
        if (alertPDU->alertType == TLMOAlerts::IllegalUseOfPort) {
            Uint16 portNumber;
            alertDataStream.read(portNumber);
            stream << "port=" << (int) portNumber;
        } else {
            stream << NE::Misc::Convert::bytes2string(*alertPDU->alertValue); // size + TPDU
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_DMO) {
        if (alertPDU->alertType == DMOAlerts::Device_Power_Status_Check) {
            Uint8 powerSupplyStatus;
            alertDataStream.read(powerSupplyStatus);
            if (powerSupplyStatus == 0) {
                stream << "line powered";
            } else if (powerSupplyStatus == 1) {
                stream << "battery power greater than 75%";
            } else if (powerSupplyStatus == 2) {
                stream << "battery power between 25% and 75%";
            } else if (powerSupplyStatus == 3) {
                stream << "battery power less than 75%";
            }
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_DSMO) {
        Uint16 numberOfFailures;
        alertDataStream.read(numberOfFailures);
        stream << "nbOfFailures=" << numberOfFailures;
    } else if (alertPDU->detectingObjectID == ObjectID::ID_DPSO) {
        if (alertPDU->alertType == DPSOAlerts::Not_On_Whitelist_Alert) {
            Address64 device;
            try {
                device.unmarshall(alertDataStream);
            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR("Not_On_Whitelist: " << ex.what());
            }
            stream << "dev=" << device.toString();
        } else if (alertPDU->alertType == DPSOAlerts::Inadequate_Join_Capability_Alert) {
            Uint8 diagnosticCode;
            Address64 device;
            try {
                alertDataStream.read(diagnosticCode);
                device.unmarshall(alertDataStream);
            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR("Inadequate_Join_Capability_Alert: " << ex.what());
            }
            if (diagnosticCode == 1) {
                stream << "bad join key; ";
            } else if (diagnosticCode == 2) {
                stream << "expired join key; ";
            } else if (diagnosticCode == 3) {
                stream << "authentication failed; ";
            }
            stream << "dev= " << device.toString();
        }
    } else {
        stream << NE::Misc::Convert::bytes2string(*alertPDU->alertValue);
    }

//    Isa100SMState::Isa100SMStateLog::logAlerts(stream);
    LOG_INFO(stream.str());
}

void ARO::indicate(ASL_AlertReport_IndicationPointer alertReport) {
    LOG_INFO("Alert received:" << alertReport->toString());

    Address32 deviceAddress = engine->getAddress32(alertReport->sourceAddress);
    if (deviceAddress == 0){
        LOG_ERROR("Address32 not found for " << alertReport->sourceAddress.toString());
        return;
    }
    NE::Model::Device * device = engine->getDevice(deviceAddress);
    if (!device) {
        LOG_ERROR("Device " << Address::toString(deviceAddress) << " not found.");
        return;
    }

    sendAcknowledge(device, alertReport); //notify that alert has been received

    ASL::PDU::AlertReportPDUPointer alertPDU = PDUUtils::extractAlertReport(alertReport->alertReport);

    //handle alerts forwarded by GW
    if (device->capabilities.isGateway()) {
        ASL::PDU::CascadedAlertReportPDUPointer cascadedAlertPDU =
                    PDUUtils::extractCascadingAlertReport(alertPDU->alertValue);

        Address32 originalAddress32 = engine->getAddress32(cascadedAlertPDU->originalDeviceAddress);
        device = engine->getDevice(originalAddress32);
        if (!device) {
            std::ostringstream errStream;
            errStream << "Device " << cascadedAlertPDU->originalDeviceAddress.toString() << " not found.";
            LOG_ERROR(errStream.str());
            throw NEException(errStream.str());
        }

        alertPDU.reset(new AlertReportPDU(cascadedAlertPDU->alertReportPDU));
    }

    logAlert(device, alertPDU);

    if (alertPDU->alertCategory == AlertCategory::deviceDiagnostic) {
        processDeviceAlert(alertPDU, device);
    } else if (alertPDU->alertCategory == AlertCategory::communicationsDiagnostic) {
        processCommunicationAlert(alertPDU, device);
    } else if (alertPDU->alertCategory == AlertCategory::security) {
        processSecurityAlert(alertPDU, device);
    } else if (alertPDU->alertCategory == AlertCategory::process) {
        processProcessAlert(alertPDU, device);
    }
}

void ARO::processDeviceAlert(PDU::AlertReportPDUPointer alertPDU, Device* reporterDevice) {
    LOG_DEBUG("DeviceDiagnostic alert reported by " << reporterDevice->address64.toString());
}

void ARO::processCommunicationAlert(PDU::AlertReportPDUPointer alertPDU, Device* reporterDevice) {
    NetworkOrderStream alertDataStream(alertPDU->alertValue);

    if (alertPDU->detectingObjectID == ObjectID::ID_DLMO) {
        //handle NeighborDiscovery alert
        if (alertPDU->alertType == DLMOAlerts::NeighborDiscovery) {
            Candidates candidates;
            try {
                candidates.unmarshall(alertDataStream);
            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR("NeighborDiscovery alert: " << ex.what());
                return;
            }
            LOG_DEBUG("receiving Candidate - DLMO alert: " << candidates.toString());

            //PhyAlert_NeighborDiscovery phyAlert_NeighborDiscovery;

            for (int i = 0; i < candidates.candidatesCount; ++i) {
                LOG_INFO("Candidate: " << std::hex << (int) candidates.neighborRadioList[i].neighbor << "from: "
                            << Address_toStream(reporterDevice->address32));

                PhyCandidate phyCandidate;
                phyCandidate.neighbor = candidates.neighborRadioList[i].neighbor;
                phyCandidate.radio = candidates.neighborRadioList[i].radio;
                //phyAlert_NeighborDiscovery.candidates.push_back(phyCandidate);
                engine->addCandidate(reporterDevice, phyCandidate);
            }
            //reporterDevice->phyAttributes.alertsTable.addNeighborDiscoveryAlert(phyAlert_NeighborDiscovery);

            //hanlde DL_Connectivity alert
        } else if (alertPDU->alertType == DLMOAlerts::DL_Connectivity) {
            Uint8 attributeNumber;
            alertDataStream.read(attributeNumber);
            if (attributeNumber == Isa100::Common::DLMO_Attributes::ChannelDiag) {
                ChannelDiag channelDiag;
                try {
                    channelDiag.unmarshall(alertDataStream);
                } catch (NE::Misc::Marshall::StreamException& ex) {
                    LOG_ERROR("DL_Connectivity alert: " << ex.what());
                    return;
                }
                LOG_DEBUG("DLMO DL_Connectivity alert - " << channelDiag);

                PhyChannelDiag phyChannelDiag;
                phyChannelDiag.count = channelDiag.count;
                for (int i = 0; i < (int) channelDiag.channelTransmissionList.size(); i++) {
                    PhyChannelDiag::ChannelTransmission channelTransmission;
                    channelTransmission.noAck = channelDiag.channelTransmissionList[i].noAck;
                    channelTransmission.ccaBackoff = channelDiag.channelTransmissionList[i].ccaBackoff;
                    phyChannelDiag.channelTransmissionList.push_back(channelTransmission);
                }
//                PhyAlert_ChannelDiag phyAlert_ChannelDiag;
//                phyAlert_ChannelDiag.channelDiag = phyChannelDiag;
//                reporterDevice->phyAttributes.alertsTable.addChannelDiagAlert(phyAlert_ChannelDiag);

                engine->addDiagnostics(reporterDevice, phyChannelDiag);


            } else if (attributeNumber == Isa100::Common::DLMO_Attributes::NeighborDiag) {
                Uint16 index = Isa100::Common::Utils::unmarshallExtDLUint(alertDataStream);
                Address32 neighborAddress32 = engine->createAddress32(reporterDevice->capabilities.dllSubnetId, index);
                Device *neighborDevice = engine->getDevice(neighborAddress32);
                if (neighborDevice == NULL) {
                    LOG_WARN("Neighbor no longer exists: " << Address_toStream(neighborAddress32));
                    return;
                }
                Uint8 diagLevel = engine->getDiagLevel(reporterDevice->address32, neighborAddress32);
                NeighborDiag neighborDiag(index);
                try {
                    neighborDiag.unmarshall(alertDataStream, diagLevel);
                } catch (NE::Misc::Marshall::StreamException& ex) {
                    LOG_ERROR("DL_Connectivity alert: " << ex.what());
                    return;
                }
//                engine->addDiagnostics(reporterDevice, neighborDevice, neighborDiag.summary.rssi, neighborDiag.summary.rsqi,
//                            neighborDiag.summary.txSuccessful, neighborDiag.summary.rxDPDU, neighborDiag.summary.txFailed,
//                            neighborDiag.summary.txNACK);
                LOG_DEBUG("DLMO DL_Connectivity alert - " << neighborDiag);
            }
        } else {
            LOG_DEBUG("DLMO unsupported alert - ");
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_ARMO) {
        if (alertPDU->alertType == ARMOAlerts::Alarm_Recovery_Start) {
            LOG_DEBUG("ARMO Alarm_Recovery_Start alert - ");
        } else if (alertPDU->alertType == ARMOAlerts::Alarm_Recovery_End) {
            LOG_DEBUG("ARMO Alarm_Recovery_End alert - ");
        } else {
            LOG_DEBUG("ARMO unsupported alert - ");
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_ASLMO) {
        if (alertPDU->alertType == ASLMOAlerts::MalformedAPDUCommunicationAlert) {
            Address128 srcAddr;
            Uint16 treshold;
            TAINetworkTimeValue interval;
            try {
                srcAddr.unmarshall(alertDataStream);
                alertDataStream.read(treshold);
                interval.unmarshall(alertDataStream);
            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR("MalformedAPDUCommunicationAlert: " << ex.what());
            }
            LOG_DEBUG("MalformedAPDU alert - srcAddress=" << srcAddr.toString());
        } else {
            LOG_DEBUG("ASLMO unsupported alert - ");
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_NLMO) {
        if (alertPDU->alertType == NLMOAlerts::NLDroppedPDU) {
            Uint8 size;
            Bytes value;
            try {
                alertDataStream.read(size);
                alertDataStream.read(value, --size); // size included in length
            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR("NLDroppedPDUAlert: " << ex.what());
            }
            LOG_DEBUG("NLMO NLDroppedPDU alert - ");
        } else {
            LOG_DEBUG("NLMO unsupported alert - ");
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_TLMO) {
        if (alertPDU->alertType == TLMOAlerts::IllegalUseOfPort) {
            Uint16 portNumber;
            alertDataStream.read(portNumber);
            LOG_DEBUG("TLMO IllegalUseOfPort alert - port: " << std::hex << portNumber);
        } else if (alertPDU->alertType == TLMOAlerts::TPDUonUnregisteredPort) {
            //First 40 octets of the TPDU - can be less
            Bytes value;
            try {
                alertDataStream.read(value, (Uint8) alertPDU->alertValueSize); //maximum is 40 bytes so alertValueSize is always 1 byte
            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR("TPDUonUnregisteredPortAlert: " << ex.what());
            }
            LOG_DEBUG("TLMO TPDUonUnregisteredPort alert - " << bytes2string(value));
        } else if (alertPDU->alertType == TLMOAlerts::TPDUoutOfSecurityPolicies) {
            //First 40 octets of the TPDU - can be less
            Bytes value;
            try {
                alertDataStream.read(value, (Uint8) alertPDU->alertValueSize); //maximum is 40 bytes so alertValueSize is always 1 byte
            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR("TPDUoutOfSecurityPoliciesAlert: " << ex.what());
            }
            LOG_DEBUG("TLMO TPDUoutOfSecurityPolicies alert - " << bytes2string(value));
        } else {
            LOG_DEBUG("TLMO unsupported alert - ");
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_DMO) {
        if (alertPDU->alertType == DMOAlerts::Device_Power_Status_Check) {
            Uint8 powerSupplyStatus;
            alertDataStream.read(powerSupplyStatus);
            LOG_DEBUG("DMO Device_Power_Status_Check alert - status: " << (int) powerSupplyStatus);
        } else if (alertPDU->alertType == DMOAlerts::Device_Restart) {
            LOG_DEBUG("DMO Device_Restart alert - ");
        } else {
            LOG_DEBUG("DMO unsupported alert - ");
        }
    }
}

void ARO::processSecurityAlert(PDU::AlertReportPDUPointer alertPDU, Device* reporterDevice) {
    NetworkOrderStream alertDataStream(alertPDU->alertValue);

    if (alertPDU->detectingObjectID == ObjectID::ID_ARMO) {
        if (alertPDU->alertType == ARMOAlerts::Alarm_Recovery_Start) {
            LOG_DEBUG("ARMO Alarm_Recovery_Start alert - ");
        } else if (alertPDU->alertType == ARMOAlerts::Alarm_Recovery_End) {
            LOG_DEBUG("ARMO Alarm_Recovery_End alert - ");
        } else {
            LOG_DEBUG("ARMO unsupported alert - ");
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_DSMO) {
        if (alertPDU->alertType == DSMOAlerts::Security_MPDU_Fail_Rate_Exceeded) {
            Uint16 numberOfFailures;
            alertDataStream.read(numberOfFailures);
            LOG_DEBUG("DSMO Security_MPDU_Fail_Rate_Exceeded alert - numberOfFailures: " << (int) numberOfFailures);
        } else if (alertPDU->alertType == DSMOAlerts::Security_TPDU_Fail_Rate_Exceeded) {
            Uint16 numberOfFailures;
            alertDataStream.read(numberOfFailures);
            LOG_DEBUG("DSMO Security_TPDU_Fail_Rate_Exceeded alert - numberOfFailures: " << (int) numberOfFailures);
        } else if (alertPDU->alertType == DSMOAlerts::Security_Key_Update_Fail_Rate_Exceeded) {
            Uint16 numberOfFailures;
            alertDataStream.read(numberOfFailures);
            LOG_DEBUG("DSMO Security_Key_Update_Fail_Rate_Exceeded alert - numberOfFailures: " << (int) numberOfFailures);
        } else {
            LOG_DEBUG("DSMO unsupported alert - ");
        }
    } else if (alertPDU->detectingObjectID == ObjectID::ID_DPSO) {
        if (alertPDU->alertType == DPSOAlerts::Not_On_Whitelist_Alert) {
            Address64 device;
            try {
                device.unmarshall(alertDataStream);
            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR("Not_On_Whitelist: " << ex.what());
            }
            LOG_DEBUG("DPSO Not_On_Whitelist alert - device: " << device.toString());
        } else if (alertPDU->alertType == DPSOAlerts::Inadequate_Join_Capability_Alert) {
            Uint8 diagnosticCode;
            Address64 device;
            try {
                alertDataStream.read(diagnosticCode);
                device.unmarshall(alertDataStream);
            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR("Inadequate_Join_Capability_Alert: " << ex.what());
            }
            LOG_DEBUG("DPSO Inadequate_Join_Capability alert - " << device.toString() << ", diagnostic: " << diagnosticCode);
        } else {
            LOG_DEBUG("DPSO unsupported alert - ");
        }
    }
}

void ARO::processProcessAlert(PDU::AlertReportPDUPointer alertPDU, Device* reporterDevice) {
    LOG_DEBUG("Process alert reported by " << reporterDevice->address64.toString());
}

void ARO::sendAcknowledge(Device* device, ASL_AlertReport_IndicationPointer alertReport) {

    Isa100::ASL::PDU::ClientServerPDUPointer apdu(new Isa100::ASL::PDU::ClientServerPDU( //
                Isa100::Common::PrimitiveType::request, //
                Isa100::Common::ServiceType::alertAcknowledge, //
                ObjectID::ID_ARO, //
                //ObjectID::ID_ARMO, //
                alertReport->alertReport->sourceObjectID, alertReport->alertReport->appHandle));

    NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(device->address32, TSAP::TSAP_SMAP,
                TSAP::TSAP_DMAP);
    if (!contract_SM_Dev) {
        std::ostringstream errStream;
        errStream << "Contract SMAP-DMAP between manager and " << Address::toString(device->address32) << " not found.";
        LOG_ERROR(errStream.str());
        throw NEException(errStream.str());
    }

    ASL_AlertAcknowledge_RequestPointer primitiveRequest(new ASL_AlertAcknowledge_Request( //
                contract_SM_Dev->contractID, //
                contract_SM_Dev->destination32, //
                ServicePriority::high, //TODO: establish priority
                false, //discardEligible
                TSAP::TSAP_SMAP, ObjectID::ID_ARO, //
                TSAP::TSAP_DMAP, //ObjectID::ID_ARMO, //
                alertReport->alertReport->sourceObjectID, //
                apdu));

    LOG_DEBUG("Send ack to:" << alertReport->sourceAddress.toString() << "; alertID="
                << (int) alertReport->alertReport->appHandle);

    messageDispatcher->Request(primitiveRequest);
}

}
}
}
