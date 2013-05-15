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
 * MVO.cpp
 *
 *  Created on: Dec 16, 2008
 *      Author: mulderul(catalin.pop), beniamin.tecar
 */

#include "MVO.h"
#include "ASL/PDUUtils.h"
#include "AL/ObjectsProvider.h"
#include "AL/ProcessesProvider.h"
#include "AL/Process.h"
#include "ASL/Services/ASL_Service_PrimitiveTypes.h"
#include "Common/Address128.h"
#include "Common/MethodsIDs.h"
#include "Common/HandleFactory.h"
#include "Common/SmSettingsLogic.h"
#include "Common/simpleini/SimpleIni.h"
#include "Common/Utils/ContractUtils.h"
#include "Misc/Convert/Convert.h"
#include "Misc/Marshall/NetworkOrderStream.h"
#include "Model/EngineProvider.h"
#include "Model/SmJoinRequest.h"
#include "Model/Routing/RouteUtils.h"
#include "Common/ClockSource.h"
#include "Model/DevicePrinter.h"
#include "SMState/SMStateLog.h"
#include "Model/ChainWaitForConfirmOnEvalGraph.h"

#include <fstream>
#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <iostream>

using namespace Isa100::Common;
using namespace Isa100::Common::Objects;
using namespace Isa100::ASL;
using namespace Isa100::ASL::Services;
using namespace Isa100::AL;

namespace Isa100 {

namespace AL {

namespace SMAP {

using namespace Isa100::Model;

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

bool MVO::flagCommands = false;
bool MVO::flagConfigs = false;

MVO::MVO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine_) :
    Isa100Object(tsap, engine_) {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: created object:" << objectIdString);

    expectResponse = false;

    //TODO must be set based on a file configuration from sm_commands.ini
    loggingDetailLevel.logSimpleAttributes = true;
    loggingDetailLevel.logIndexedAttributes = true;
    loggingDetailLevel.logTheoreticAttributes = true;

    lastUnstableCheck = ClockSource::getCurrentTime();
}

MVO::~MVO() {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: destroyed object:" << objectIdString);
}

void MVO::execute(Uint32 currentTime) {

    if(flagCommands) {
        flagCommands = false;
        readCommandFile();
        return;
    }
    else {
        if (flagConfigs) {
            flagConfigs = false;
            LOG_REINIT(Isa100::Common::SmSettingsLogic::instance().logConfFileName);
            Isa100::Common::SmSettingsLogic::instance().reload();
            //after reload remove from network the devices that are not provisioned anymore
            LOG_INFO("Provisioning file <system_manager.ini> reloaded.");
            removeUnprovisionedDevices();

            Isa100::Model::EngineProvider::getEngine()->updateAdvPeriod();
            return;

        }
    }

    if (SmSettingsLogic::instance().unstableCheck) {
        unstableCheck(currentTime);
    }

    static int stepLogging = 0;
    if (stepLogging < 5) {
        ++stepLogging;
        return;
    } else {
        stepLogging = 0;
    }

    try {

        // logg max one device at each execute
        Device* managerDevice = engine->getDevice(ADDRESS16_MANAGER);
        Uint8 i =0;
        if (managerDevice->hasChanged) {
            //                LOG_DEBUG("Device CHANGED " << *managerDevice);
            SMState::SMStateLog::logDeviceAttributes(managerDevice, loggingDetailLevel, 0);
            managerDevice->hasChanged = false;
            ++i;
        }

        Device* gatewayDevice = engine->getDevice(ADDRESS16_GATEWAY);
        if (gatewayDevice != NULL && gatewayDevice->hasChanged) {
            SMState::SMStateLog::logDeviceAttributes(gatewayDevice, loggingDetailLevel, 0);
            gatewayDevice->hasChanged = false;
            ++i;
        }

        NE::Model::SubnetsMap& subnets = engine->getSubnetsList();
        NE::Model::SubnetsMap::iterator itSubnet = subnets.begin();
        for (; (itSubnet != subnets.end()); ++itSubnet) {
            const Subnet::PTR subnet = itSubnet->second;
            const NE::Common::SubnetSettings& subnetSettings = subnet->getSubnetSettings();
            Address16 address = 0;
            while (!subnet->changedDevices.empty() && (i < 5)){
                address = subnet->changedDevices.front();
                Device* device = itSubnet->second->getDevice(address);
                if (device != NULL && device->hasChanged) {
                    SMState::SMStateLog::logDeviceAttributes(device, loggingDetailLevel, subnetSettings.getSlotsPerSec());
                    device->hasChanged = false;
                    ++i;
                }
                subnet->changedDevices.pop_front();
            }
        }
    } catch (std::exception& ex) {
        LOG_ERROR(LOG_OI << ex.what());
    }
}

void MVO::unstableCheck(Uint32 currentTime) {

    if (currentTime < lastUnstableCheck + 60) {
        return;
    }

    try {
        lastUnstableCheck = currentTime;

        static const std::string pathToUnstableLogFile = "/tmp/sm.unstable";

        bool checkContraints = true;
        std::ostringstream reasonStream;

        NE::Model::SubnetsMap & subnets = engine->getSubnetsList();
        for (NE::Model::SubnetsMap::iterator itSubnet = subnets.begin(); checkContraints && itSubnet != subnets.end(); ++itSubnet) {
            Subnet::PTR subnet = itSubnet->second;

            Uint16 subnetID = subnet->getSubnetId();
            //  if (lastNrOfDevices.find(subnetID) == lastNrOfDevices.end()) {
            //      lastNrOfDevices[subnetID] = 0;
            //  }
            if (lastNrOfConfirmedDevices.find(subnetID) == lastNrOfConfirmedDevices.end()) {
                lastNrOfConfirmedDevices[subnetID] = 0;
            }

            Uint16 nrOfDevices = 0;
            Uint16 nrOfConfirmedDevices = 0;
            Uint32 brStartTime = 0;

            const Address16Set & activeDevices = subnet->getActiveDevices();
            for (Address16Set::const_iterator it = activeDevices.begin(); checkContraints && it != activeDevices.end(); ++it) {

                Device * device = subnet->getDevice(*it);
                if (!device) {
                    continue;
                }

                if (device->capabilities.isManager() || device->capabilities.isGateway() || device->capabilities.isBackbone()) {

                    if (device->capabilities.isBackbone()) {
                        brStartTime = device->startTime;
                    }
                    continue;
                }

                if (device->status == DeviceStatus::JOIN_CONFIRMED) {
                    ++nrOfConfirmedDevices;
                }
                ++nrOfDevices;
            }

            if (nrOfDevices == 0 || brStartTime == 0){
                continue;
            }

            //if setup stable after 40 minutes do not perform stability check anymore
            if (currentTime > brStartTime + 40 * 60
                        && nrOfConfirmedDevices == nrOfDevices) {

                SmSettingsLogic::instance().unstableCheck = false;
                LOG_INFO("Stability check is being disabled. 40 minutes have elapsed since BR join and system is stable.");
            }

            // 1. after 20 minutes from start BR
            if (currentTime > brStartTime + 20 * 60
                        && nrOfConfirmedDevices + 3 < lastNrOfConfirmedDevices[subnetID]) {
                checkContraints = false;
                reasonStream << "after 20 minutes from BR start - count of confirmed devices ("
                    << (int) nrOfConfirmedDevices << ") + 3 < last count of confirmed devices ("
                    << (int) lastNrOfConfirmedDevices[subnetID] << ")";
                lastNrOfConfirmedDevices[subnetID] = nrOfConfirmedDevices;
                LOG_ERROR("System unstable: curentTime:" << currentTime << ", brStart:" << brStartTime
                            << ", devicesCount=" << nrOfDevices << ", confirmed:" << nrOfConfirmedDevices
                            << ", previousConfirmedCount:" << lastNrOfConfirmedDevices[subnetID]);
                break;
            }

            // 2. after 40 minutes from start BR
            if (currentTime > brStartTime + 40 * 60
                        && ((nrOfConfirmedDevices * 100) / nrOfDevices) < 80) {
                checkContraints = false;
                reasonStream << "after 40 minutes from BR start - count of confirmed devices ("
                    << (int) nrOfConfirmedDevices << ") * 100 / count of devices (" << (int)nrOfDevices << ") " << " < 80)";
                lastNrOfConfirmedDevices[subnetID] = nrOfConfirmedDevices;
                LOG_ERROR("System unstable: curentTime:" << currentTime << ", brStart:" << brStartTime
                            << ", devicesCount=" << nrOfDevices << ", confirmed:" << nrOfConfirmedDevices
                            << ", previousConfirmedCount:" << lastNrOfConfirmedDevices[subnetID]);
                break;
            }

            // 3. after 60 minutes from start BR
            if (currentTime > brStartTime + 60 * 60
                        && nrOfConfirmedDevices < nrOfDevices) {
                checkContraints = false;
                reasonStream << "after 60 minutes from BR start - count of confirmed devices ("
                    << (int) nrOfConfirmedDevices << ") < count of devices (" << (int)nrOfDevices << ")";
                lastNrOfConfirmedDevices[subnetID] = nrOfConfirmedDevices;
                LOG_ERROR("System unstable: curentTime:" << currentTime << ", brStart:" << brStartTime
                            << ", devicesCount=" << nrOfDevices << ", confirmed:" << nrOfConfirmedDevices
                            << ", previousConfirmedCount:" << lastNrOfConfirmedDevices[subnetID]);
                break;
            }

            lastNrOfConfirmedDevices[subnetID] = nrOfConfirmedDevices;
        }

        if (!checkContraints) {
            LOG_ERROR("System unstable -- write unstable file: curentTime:" << currentTime << " " << reasonStream.str());
            std::ofstream unstableLogFile(pathToUnstableLogFile.c_str(), ios::trunc | ios::out);
            if (!unstableLogFile.is_open()) {
                LOG_ERROR("Error create/open file << " << pathToUnstableLogFile);
                return;
            }

//            std::string currentTimeStr;
//            Time::toString(currentTime, currentTimeStr);

            std::ostringstream logFileStream;
            logFileStream << "time=" << time(NULL) << std::endl;
            logFileStream << "reason=\"" << reasonStream.str() << "\""; //format in file must be: reason="reason_string"

            unstableLogFile.write(logFileStream.str().c_str(), logFileStream.str().size());
            unstableLogFile.close();
        }

    } catch (std::exception & ex) {
        LOG_ERROR(LOG_OI << ex.what());
    }
}

void MVO::cleanupOnError() {
    LOG_DEBUG("cleanupOnError()");
}

Isa100::AL::ObjectID::ObjectIDEnum MVO::getObjectID() const {
    return Isa100::AL::ObjectID::ID_MVO;
}

bool MVO::expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    LOG_DEBUG("expectIndicate() : " << indication->toString());

    return true;
}

bool MVO::expectConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
    return true;
}

void MVO::indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    LOG_DEBUG("indicateExecute()");

    readCommandFile();

}

void MVO::confirmWrite(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
    LOG_DEBUG("confirmWrite...Not implemented yet.");
    throw NE::Common::NEException("confirmWrite...Not implemented yet.");
}

void MVO::confirmExecute(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
    LOG_DEBUG("confirmExecute");
    Address128 serverAddress128 = confirm->serverNetworkAddress;

    int indexOfSentRequest = getSentRequestIndexForRequestId(confirm->apduResponse->appHandle);
    if (indexOfSentRequest == -1) {
        LOG_ERROR("confirmRead() : MVO received unknown response reqId=" << (int) confirm->apduResponse->appHandle);
        return;
    }

    ExecuteResponsePDUPointer executeResponse = PDUUtils::extractExecuteResponse(confirm->apduResponse);
    if (executeResponse->feedbackCode != SFC::success) {
        LOG_ERROR("confirmExecute() : returned error code : " << executeResponse->feedbackCode);
        return;
    }

    std::ostringstream stream;
    stream << "Device : ";
    stream << lastRequestsSent[indexOfSentRequest].deviceAddress64.toString();
    stream << ", objID=";
    stream << lastRequestsSent[indexOfSentRequest].objectId;
    stream << ", methodID=" << lastRequestsSent[indexOfSentRequest].methodId;
    stream << ", response:";
    stream << executeResponse->toString();
    stream << std::endl;
    stream << confirm->toString();

    LOG_INFO(stream.str());

    //remove request from list
    lastRequestsSent.erase(lastRequestsSent.begin() + indexOfSentRequest);

    return;
}

void MVO::confirmRead(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
    LOG_DEBUG("confirmRead()");
    Address128 serverAddress128 = confirm->serverNetworkAddress;
    //RequestHandle requestID = confirm->apduResponse->requestID;

    int indexOfSentRequest = getSentRequestIndexForRequestId(confirm->apduResponse->appHandle);
    if (indexOfSentRequest == -1) {
        LOG_ERROR("confirmRead() : MVO received unknown response reqId=" << (int) confirm->apduResponse->appHandle);
        return;
    }

    //    if (lastRequestsSent[indexOfSentRequest].requestId == requestID
    //                && lastRequestsSent[indexOfSentRequest].deviceAddress == serverAddress128) {

    ReadResponsePDUPointer readResponse = PDUUtils::extractReadResponse(confirm->apduResponse);
    if (readResponse->feedbackCode != SFC::success) {
        LOG_ERROR("confirmRead() : returned error code : " << readResponse->feedbackCode);
        return;
    }

    std::ostringstream stream;
    stream << "Device : ";
    stream << lastRequestsSent[indexOfSentRequest].deviceAddress64.toString();
    stream << ", objID=";
    stream << lastRequestsSent[indexOfSentRequest].objectId;
    stream << ", attribID=" << lastRequestsSent[indexOfSentRequest].attributeID;
    stream << ", response:";
    stream << readResponse->toString();
    stream << std::endl;
    stream << confirm->toString();

    LOG_INFO(stream.str());

    //remove request from list
    lastRequestsSent.erase(lastRequestsSent.begin() + indexOfSentRequest);

    return;
    //    }

    //LOG_ERROR("confirmRead() : invalid response");
}


int MVO::getSentRequestIndexForRequestId(int requestId) {
    for (int i = 0; i < (int) lastRequestsSent.size(); ++i) {
        if (lastRequestsSent[i].requestId == requestId) {
            return i;
        }
    }

    return -1;
}

void MVO::readCommandFile() {
//    LOG_DEBUG("readCommandFile()");
    printf("Signal received: User2 - read command from file\n");
    SimpleIni::CSimpleIniCaseA iniParser(false, true);
    std::string commandFileName = Isa100::Common::SmSettingsLogic::instance().debugCommandsFileName;
    if (commandFileName.size() > 0) {
        if (iniParser.LoadFile(commandFileName.c_str()) != SimpleIni::SI_OK) {
            LOG_WARN("Could not open command file " << commandFileName);
            printf("Could not open command file: %s\n", commandFileName.c_str());

            std::string path(NIVIS_PROFILE);
            commandFileName = path + commandFileName;
            if (iniParser.LoadFile(commandFileName.c_str()) != SimpleIni::SI_OK) {
                LOG_ERROR("Could not open command file " << commandFileName);
                printf("Could not open command file: %s\n", commandFileName.c_str());
                return;
            }
        }
    }
    printf("Opened command file: %s\n", commandFileName.c_str());
    LOG_DEBUG("Opened command file: " << commandFileName.c_str());


    // reads all the commands from the file
    SimpleIni::CSimpleIniCaseA::TNamesDepend commands;
    iniParser.GetAllValues("SM_COMMANDS", "COMMAND", commands);

    SimpleIni::CSimpleIniCaseA::TNamesDepend::const_iterator it = commands.begin();
    for (; it != commands.end(); ++it) {
        // COMMAND=10,0102:0304:0506:0701,1,6

        Uint16 commandId;
        Address64 ip64;

        try {
            std::string p_line = (*it).pItem;
            uint paramsIdx = 0;
            // split the input line into tokens using the comma separator
            tokenizer tokens(p_line, boost::char_separator<char>(","));
            Uint16 paramsNo = distance(tokens.begin(), tokens.end());
            if (paramsNo < 1) {
                throw NE::Common::NEException("missing elements: need comma separated : commandId, 64 bit address");
            }

            tokenizer::const_iterator tok_iter = tokens.begin();

            // read the command id
            try {
                commandId = boost::lexical_cast<Uint16>(*tok_iter);
                paramsIdx++;
            } catch (boost::bad_lexical_cast &) {
                throw NE::Common::NEException("invalid commandId value.");
            }

            if (commandId == (Uint16) DebugCommands::READ_ATTRIBUTE //
                        || commandId == (Uint16) DebugCommands::PRINT_ATTRIBUTES_OF_DEVICE
                        || commandId == (Uint16) DebugCommands::PRINT_ENTITIES_OF_DEVICE
                        || commandId == (Uint16) DebugCommands::EXECUTE_METHOD) {
                // read the 64 bit address
                try {
                    ip64.loadString(*++tok_iter);
                    paramsIdx++;
                } catch (const InvalidArgumentException& ex) {
                    throw NEException(string("invalid 64 bit address: ") + ex.what());
                }
            }

            // read command parameters
            std::vector<std::string> cmdParameters;
            for (uint i = paramsIdx; i < paramsNo; i++) {
                cmdParameters.push_back(*++tok_iter);
            }
            LOG_INFO("Command : " << p_line);
            processCommand(commandId, ip64, cmdParameters);

        } catch (const NE::Common::NEException& ex) {
            LOG_ERROR("In " << commandFileName << ", option SM_COMMANDS.COMMAND, item no " << (*it).nOrder << ": " << ex.what());
        }

    }
}

void MVO::processCommand(Uint16 commandId, const Address64& ip64, std::vector<std::string>& cmdParameters) {
    LOG_INFO("Command : " << commandId << ", " << ip64.toString() << ", " << cmdParameters.size() << " parameters.");
    using boost::bad_lexical_cast;
    switch (commandId) {
        case DebugCommands::PRINT_GRAPHS_FROM_SUBNET: {
            if (cmdParameters.size() != 1) {
                LOG_ERROR("Command PRINT_GRAPHS_FROM_SUBNET must have 1 parameter (subnetID)!");
                return;
            }
            try {
                Uint16 subnetId = boost::lexical_cast<Uint16>(cmdParameters[0]);
                SubnetsMap& subnets = engine->getSubnetsList();
                if (subnets.find(subnetId) != subnets.end()) {
                    LOG_DEBUG("Print graphs from subnet" << cmdParameters[0]);
                    Subnet::PTR subnet = subnets[subnetId];
                    SMState::SMStateLog::logNetworkTopology(subnet);
                }
            } catch(bad_lexical_cast&) {
                return;
            }

            break;
        }
        case DebugCommands::PRINT_RESERVED_MNG_CHUNCKS_SUBNT: {
            if (cmdParameters.size() != 1) {
                LOG_ERROR("Command PRINT_RESERVED_MNG_CHUNCKS_SUBNT must have 1 parameter (subnetID)!");
                return;
            }
            try {
                Uint16 subnetId = boost::lexical_cast<Uint16>(cmdParameters[0]);
                SubnetsMap& subnets = engine->getSubnetsList();
                if (subnets.find(subnetId) != subnets.end()) {
                    LOG_DEBUG("Print management chunks from subnet" << cmdParameters[0]);
                    Subnet::PTR subnet = subnets[subnetId];
                    SMState::SMStateLog::logMngBandwidthChuncks(subnet);
                }
            }

            catch(bad_lexical_cast&)  {
                return;
            }

            break;
        }
        case DebugCommands::PRINT_RESERVED_MNG_CHUNCKS_DVC: {
            if (cmdParameters.size() != 2) {
                LOG_ERROR("Command PRINT_RESERVED_MNG_CHUNCKS_DVC must have 2 parameters (subnetID, device_addr_16)!");
                return;
            }
            try {
                Uint16 subnetId = boost::lexical_cast<Uint16>(cmdParameters[0]);
                Address16 device = boost::lexical_cast<Uint16>(cmdParameters[1]);
                SubnetsMap& subnets = engine->getSubnetsList();
                if (subnets.find(subnetId) != subnets.end()) {
                    LOG_DEBUG("Print management chunks for device" << cmdParameters[0]);
                    Subnet::PTR subnet = subnets[subnetId];
                    SMState::SMStateLog::logMngBandwidthChuncksForDevice(subnet, device);
                }
            }

            catch(bad_lexical_cast&)  {
                return;
            }

            break;
        }
        case DebugCommands::READ_ATTRIBUTE: {
            if (cmdParameters.size() != 3) {
                LOG_ERROR("Command READ_ATTRIBUTE must have 3 parameters!");
                return;
            }
            LOG_DEBUG("read attribute");
            generateReadAttributeCommand(ip64, cmdParameters);

            break;
        }
        case DebugCommands::SM_CURRENT_TAI: {
            std::ostringstream stream;
            stream << "SM time: " << ClockSource::getCurrentTime() << "; SM_TAI=" << std::hex << ClockSource::getTAI(
                        engine->getSettingsLogic());
            LOG_INFO(stream.str());
            return;

            break;
        }
        case DebugCommands::PRINT_ATTRIBUTES_OF_DEVICE: {
            Address32 deviceAddress32 = engine->getAddress32(ip64);
            NE::Model::Device * device = engine->getDevice(deviceAddress32);
            if (!device) {
                std::ostringstream stream;
                stream << "PRINT_ATTRIBUTES_OF_DEVICE - Device " << Address::toString(deviceAddress32) << " not found.";
                LOG_ERROR(LOG_OI << stream.str());
                return;
            }
            int slotsPerSec = 0;
            if (device->capabilities.isManager() || device->capabilities.isGateway()){
                slotsPerSec = 0;
            } else {
                const Subnet::PTR& subnet = engine->getSubnetsContainer().getSubnet(device->address32);
                if (subnet){
                    slotsPerSec = subnet->getSubnetSettings().getSlotsPerSec();
                }
            }
            SMState::SMStateLog::logDeviceAttributes(device, loggingDetailLevel, slotsPerSec);

            break;
        }
        case DebugCommands::PRINT_ATTRIBUTES_OF_ALL_DEVICES: {
            SubnetsMap& subnets = engine->getSubnetsList();
            for (SubnetsMap::iterator itSubnets = subnets.begin(); itSubnets != subnets.end(); ++itSubnets) {
                SMState::SMStateLog::logSubnetDevicesAttributes(itSubnets->second, loggingDetailLevel);
            }

            break;
        }
        case DebugCommands::PRINT_ENTITIES_OF_DEVICE: {
            Address32 deviceAddress32 = engine->getAddress32(ip64);

            // don't change this->loggingDetailLevel.entityType that is used for all next commands!!!
            LogDeviceDetail logDetailLevel = loggingDetailLevel;
            logDetailLevel.entityType = boost::lexical_cast<int>(cmdParameters[0]);

            NE::Model::Device * device = engine->getDevice(deviceAddress32);
            if (!device) {
                std::ostringstream stream;
                stream << "PRINT_ENTITIES_OF_DEVICE - Device " << Address::toString(deviceAddress32) << " not found.";
                LOG_ERROR(LOG_OI << stream.str());
                return;
            }

            int slotsPerSec = 0;
            if (device->capabilities.isManager() || device->capabilities.isGateway()){
                slotsPerSec = 0;
            } else {
                const Subnet::PTR& subnet = engine->getSubnetsContainer().getSubnet(device->address32);
                if (subnet){
                    slotsPerSec = subnet->getSubnetSettings().getSlotsPerSec();
                }
            }

            SMState::SMStateLog::logDeviceAttributes(device, logDetailLevel, slotsPerSec);

            break;
        }
        case DebugCommands::PRINT_ENTITIES_OF_ALL_DEVICES: {

            LOG_DEBUG("PRINT_ENTITIES_OF_ALL_DEVICES");

            // don't change this->loggingDetailLevel.entityType that is used for all next commands!!!
            LogDeviceDetail logDetailLevel = loggingDetailLevel;
            try {
                logDetailLevel.entityType = boost::lexical_cast<int>(cmdParameters[0]);

                SubnetsMap& subnets = engine->getSubnetsList();
                for (SubnetsMap::iterator itSubnets = subnets.begin(); itSubnets != subnets.end(); ++itSubnets) {
                    SMState::SMStateLog::logSubnetDevicesAttributes(itSubnets->second, logDetailLevel);
                }
            } catch(bad_lexical_cast&)  {
                return;
            }
            break;
        }
        case DebugCommands::EXECUTE_METHOD: {
            if (cmdParameters.size() < 3) {
                LOG_ERROR("Command EXECUTE_METHOD must have at least 3 parameters!");
                return;
            }
            generateExecuteCommand(ip64, cmdParameters);
            break;
        }
        default: {
            std::ostringstream streamEx;
            streamEx << "Unknown DebugCommand " << (int) commandId;
            throw NEException(streamEx.str());
        }
    }


    //    if (!SmAddressTable::instance().existsAddress64(ip64)) {
    //        std::ostringstream stream;
    //        stream << "processCommand() device with address : " << ip64.toString()<< "does not exist!";
    //        LOG_DEBUG(stream.str());
    //        Isa100StateAttributes::logSmAttribute(stream.str());
    //        return;
    //
    //    } else if (engine->getDeviceStatus(SmAddressTable::instance().getAddress32(ip64)) != NE::Model::DeviceStatus::JOIN_CONFIRMED) {
    //        std::ostringstream stream;
    //        stream << "processCommand() device with address : " << ip64.toString()<< "does not not have JOIN_CONFIRMED status!";
    //        LOG_DEBUG(stream.str());
    //        Isa100StateAttributes::logSmAttribute(stream.str());
    //        return;
    //    }

     //else if (commandId == (Uint16) DebugCommands::WRITE_ALL_GRAPHS) {
    //        LOG_DEBUG("write all graphs");
    //
    //        std::map<Uint16, NE::Model::Routing::SubnetTopology>& subnetTopologies =
    //                    Isa100::Model::engine->getSubnetTopologies();
    //
    //        for (std::map<Uint16, NE::Model::Routing::SubnetTopology>::iterator itSubnets = subnetTopologies.begin(); itSubnets
    //                    != subnetTopologies.end(); ++itSubnets) {
    //            //if (itSubnets->second.getSubnetId() != 0) {
    //            std::ostringstream stream;
    //            stream << "SubTop_" << itSubnets->first;
    //            writeTopologyToFile(itSubnets->second, stream.str());
    //
    //            std::map<Uint16, NE::Model::Routing::RoutePointer>& subnetRoutes = itSubnets->second.getRoutes();
    //            std::map<Uint16, NE::Model::Routing::RoutePointer>::iterator itRoute = subnetRoutes.begin();
    //            for (; itRoute != subnetRoutes.end(); ++itRoute) {
    //                std::ostringstream stream;
    //                stream << "SubTop_" << itSubnets->first;
    //                stream << "_graph_" << itRoute->first;
    //                writeTopologyToFile(itSubnets->second, itRoute->first, stream.str());
    //
    //            }
    //            //}
    //        }
    //        return;
    //    } else {
    //        LOG_ERROR("Invalid command to process : " << commandId << " for device " << ip64.toString());
    //        return;
    //    }
    //
    //NE::Common::Address128 address128 = engine->getSettingsLogic().managerAddress128;


}

void MVO::generateReadAttributeCommand(const Address64& ip64, std::vector<std::string>& cmdParameters) {
    AppHandle requestId = HandleFactory::CreateHandle();
    Isa100::AL::ObjectID::ObjectIDEnum const objectIDSource = Isa100::AL::ObjectID::ID_MVO;
    try {

        TSAP::TSAP_Enum tsapDestination = (TSAP::TSAP_Enum) boost::lexical_cast<Uint16>(cmdParameters[0]);
        Isa100::AL::ObjectID::ObjectIDEnum objectIDDestination = boost::lexical_cast<Uint16>(cmdParameters[1]);
        Uint16 attributeID = boost::lexical_cast<Uint16>(cmdParameters[2]);

        //more parameters mean indexed attribute
        Uint16 firstIndex = 0;
        Uint16 secondIndex = 0;
        std::vector<std::string>::iterator itParams = cmdParameters.begin() + 3;
        if (itParams != cmdParameters.end()) {
            firstIndex = boost::lexical_cast<Uint16>(*itParams);
            LOG_INFO("Reading indexed attribute...firstIndex=" << (int) firstIndex);
            ++itParams;
            if (itParams != cmdParameters.end()) {
                secondIndex = boost::lexical_cast<Uint16>(*itParams);
                LOG_INFO("Reading indexed attribute...secondIndex=" << (int) secondIndex);
            }
        }

        ClientServerPDUPointer clientServerPDU(new ClientServerPDU( //
                    Isa100::Common::PrimitiveType::request, //
                    Isa100::Common::ServiceType::read, //
                    objectIDSource, //
                    objectIDDestination, //
                    requestId));

        ExtensibleAttributeIdentifier eai(attributeID);
        Isa100::ASL::PDU::ReadRequestPDUPointer readRequestPDU(new Isa100::ASL::PDU::ReadRequestPDU(eai));

        if (firstIndex != 0) {
            if (secondIndex != 0) {
                ExtensibleAttributeIdentifier eai(attributeID, firstIndex, secondIndex); //double-indexed
                readRequestPDU.reset(new Isa100::ASL::PDU::ReadRequestPDU(eai));
            } else {
                ExtensibleAttributeIdentifier eai(attributeID, firstIndex); //single-indexed
                readRequestPDU.reset(new Isa100::ASL::PDU::ReadRequestPDU(eai));
            }
        }

        PDUUtils::appendReadRequest(clientServerPDU, readRequestPDU);

        Address32 deviceAddress32 = engine->getAddress32(ip64);

        NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(//
                    deviceAddress32, (Uint16) TSAP::TSAP_SMAP, (Uint16) tsapDestination);

        if (!contract_SM_Dev) {
            std::ostringstream stream;
            stream << "Couldn't find contract between manager and " << Address_toStream(deviceAddress32) << ", tsap "
                        << (int) TSAP::TSAP_SMAP << "->" << (int)tsapDestination;
            LOG_ERROR(LOG_OI << stream.str());
            throw NEException(stream.str());
        }

        PrimitiveRequestPointer primitiveRequest(new PrimitiveRequest( //
                    contract_SM_Dev->contractID, //
                    deviceAddress32, //
                    ServicePriority::medium, //
                    false, //
                    tsapDestination, //
                    objectIDDestination, //
                    TSAP::TSAP_SMAP, //
                    objectIDSource, //
                    clientServerPDU));

        messageDispatcher->Request(primitiveRequest);

        LOG_INFO("Generated read command with reqID=" << (int) requestId);

        NE::Common::Address128 address128; //TODO - set the address of device if this is needed

        SentRequest sentRequest(requestId, address128, objectIDDestination, 1); //method id not used here
        sentRequest.attributeID = attributeID;
        sentRequest.deviceAddress64 = ip64;
        lastRequestsSent.push_back(sentRequest);
    }
    catch(bad_lexical_cast& ex)  {
        LOG_ERROR("Exception occured: " << ex.what());
        return;
    }
}

void MVO::generateExecuteCommand(const Address64& ip64, std::vector<std::string>& cmdParameters) {
    AppHandle requestId = HandleFactory::CreateHandle();
    Isa100::AL::ObjectID::ObjectIDEnum objectIDSource = Isa100::AL::ObjectID::ID_MVO;
    TSAP::TSAP_Enum tsapDestination;
    Isa100::AL::ObjectID::ObjectIDEnum objectIDDestination;
    Uint8 methodID;
    std::string params;

    try {
        tsapDestination = (TSAP::TSAP_Enum) boost::lexical_cast<Uint16>(cmdParameters[0]);
        objectIDDestination = boost::lexical_cast<Uint16>(cmdParameters[1]);
        methodID = (Uint8) boost::lexical_cast<int>(cmdParameters[2]);

        //params.assign(cmdParameters.begin() + 3, cmdParameters.end());
        for (std::vector<std::string>::iterator itParams = cmdParameters.begin() + 3; itParams != cmdParameters.end(); ++itParams) {
            params += *itParams;
        }

        LOG_DEBUG("generate execute command - methodID=" << (int) methodID << ", params=" << params);

    } catch(bad_lexical_cast&)  {
        return;
    }

    ClientServerPDUPointer clientServerPDU(new ClientServerPDU( //
                Isa100::Common::PrimitiveType::request, //
                Isa100::Common::ServiceType::execute, //
                objectIDSource, //
                objectIDDestination, //
                requestId));

    Bytes paramsBytes = NE::Misc::Convert::string2bytes(params);
    ASL::PDU::ExecuteRequestPDUPointer forwardedRequest(
                new ASL::PDU::ExecuteRequestPDU(methodID, BytesPointer(new Bytes(paramsBytes))));
    clientServerPDU = PDUUtils::appendExecuteRequest(clientServerPDU, forwardedRequest);

    Address32 deviceAddress32 = engine->getAddress32(ip64);

    NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(//
                deviceAddress32, (Uint16) TSAP::TSAP_SMAP, (Uint16) tsapDestination);

    if (!contract_SM_Dev) {
        std::ostringstream stream;
        stream << "Couldn't find contract between manager and " << Address_toStream(deviceAddress32) << ", tsap "
                    << (int) TSAP::TSAP_SMAP << "->" << (int) tsapDestination;
        LOG_ERROR(LOG_OI << stream.str());
        throw NEException(stream.str());
    }

    PrimitiveRequestPointer primitiveRequest(new PrimitiveRequest( //
                contract_SM_Dev->contractID, //
                deviceAddress32, //
                ServicePriority::medium, //
                false, //
                tsapDestination, //
                objectIDDestination, //
                TSAP::TSAP_SMAP, //
                objectIDSource, //
                clientServerPDU));

    messageDispatcher->Request(primitiveRequest);

    LOG_INFO(primitiveRequest->toString());

    LOG_INFO("Generated execute command with reqID=" << (int) requestId);

    NE::Common::Address128 address128; //TODO - set the address of device if this is needed

    SentRequest sentRequest(requestId, address128, objectIDDestination, methodID);
    sentRequest.attributeID = 0; //not used here
    sentRequest.deviceAddress64 = ip64;
    lastRequestsSent.push_back(sentRequest);
}

void MVO::removeUnprovisionedDevices() {
    using namespace NE::Model::Operations;

    std::vector<std::pair<Device*, Subnet::PTR> > unprovisionedDevices;

    NE::Model::SubnetsMap& subnets = engine->getSubnetsList();
    NE::Model::SubnetsMap::iterator itSubnet = subnets.begin();
    for (; (itSubnet != subnets.end()); ++itSubnet) {
        const NE::Common::Address16Set& activeDevices = itSubnet->second->getActiveDevices();
        for (NE::Common::Address16Set::iterator itDevice = activeDevices.begin(); itDevice != activeDevices.end(); ++itDevice) {
            Device* device = itSubnet->second->getDevice(*itDevice);
            if (!device || device->capabilities.isGateway() || device->capabilities.isBackbone()) {
                continue;
            }
            Isa100::Common::ProvisioningItem *provisioningItem =
                SmSettingsLogic::instance().getProvisioningForDevice(device->address64);
            //No provisioning found for device
            if (!provisioningItem) {
                unprovisionedDevices.push_back(std::pair<Device*, Subnet::PTR>(device, itSubnet->second));
            }
        }
    }

    for (std::vector<std::pair<Device*, Subnet::PTR> >::iterator itUnprovisioned = unprovisionedDevices.begin();
                itUnprovisioned != unprovisionedDevices.end(); ++itUnprovisioned) {

        Device* device = itUnprovisioned->first;

        LOG_INFO("Provisioning for " << device->address64.toString() << " not found. Removing device from network.");
        char reason[128];
        sprintf(reason, "RemoveUnprovisionedDevice %s", device->address64.toString().c_str());

        OperationsContainerPointer operationsContainer(new OperationsContainer(reason));

        ChainWaitForConfirmOnEvalGraphPointer chainConfirmGraphOperations(
                   new ChainWaitForConfirmOnEvalGraph(itUnprovisioned->second, DEFAULT_GRAPH_ID));
        operationsContainer->addHandlerResponse(
                   boost::bind(&ChainWaitForConfirmOnEvalGraph::process, chainConfirmGraphOperations, _1, _2, _3));

        engine->removeDeviceOnError(device->address32, operationsContainer, RemoveDeviceReason::provisioning_removed);

        //after this call, no modification of the operationsContainer should be made
        engine->getOperationsProcessor().addOperationsContainer(operationsContainer);

    }
}

}
}
}
