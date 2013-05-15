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
 * @author catalin.pop, beniamin.tecar, ioan.pocol, radu.pop, andrei.petrut, sorin.bidian, flori.parauan
 */
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>

#include "Common/SmSettingsLogic.h"
#include "Common/Address128.h"
#include "Security/SecurityManager.h"
#include "Common/NEException.h"
#include <iostream>

#include "Shared/Socket.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Model/EngineProvider.h"

namespace Isa100 {
namespace Common {


typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

SmSettingsLogic::SmSettingsLogic() {

    udpTestClientServerListenPort = 8899;
    udpTestClientEndpoint.ipv4 = "127.0.0.1";
    udpTestClientEndpoint.port = 6789;
    firmwareContractsEnabled = false;
}

#define CAST_STR(variable) boost::lexical_cast<std::string>(variable)

#define READ_OPTION_STRING(settingVariable, optionLable){\
	try {\
		settingVariable = options[optionLable];\
		LOG_INFO(optionLable << ": " << subnetConfigFileName);\
	} catch (const std::exception& ex) {\
		LOG_ERROR("In " << commonIniFileName << ", option " << optionLable << ": " << ex.what());\
		exit(1);\
	}\
}

#define READ_OPTION_NUMBER(settingVariable, optionLable, numberType, numberLable){\
	try {\
		std::string value = options[optionLable];\
		if(value.empty()){\
			LOG_FATAL(" Option " << optionLable << ": does not have a default value.");\
			exit(1);\
		}\
		settingVariable = boost::lexical_cast<numberType>(value);\
        LOG_INFO(optionLable << ": " << (int)settingVariable);\
    } catch (boost::bad_lexical_cast & ex) {\
        LOG_ERROR("In " << commonIniFileName << ", option " << optionLable << ": invalid value, expected " << numberLable);\
        exit(1);\
    }\
}

#define READ_OPTION_BOOLEAN(settingVariable, optionLable){\
	{\
	string boolStr = options[optionLable];\
    if (boolStr != "true" && boolStr != "false") {\
        LOG_ERROR("In " << commonIniFileName << ", option " << optionLable << ": invalid value, expected true or false");\
        exit(1);\
    }\
    settingVariable = (boolStr == "true");\
	}\
}

void SmSettingsLogic::ParseCommonConfigFile(SimpleIni::CSimpleIniCaseA& iniParser) {
    // build a map of options in the form group->option
    multimap<string, string> expectedOptions;
    expectedOptions.insert(pair<string, string> ("SYSTEM_MANAGER_IPv6", string()));//No default value. Must exist in config.ini
    expectedOptions.insert(pair<string, string> ("SYSTEM_MANAGER_IPv4", string()));
    expectedOptions.insert(pair<string, string> ("SYSTEM_MANAGER_Port", string()));
    expectedOptions.insert(pair<string, string> ("SECURITY_MANAGER_EUI64", string()));
    expectedOptions.insert(pair<string, string> ("SYSTEM_MANAGER_TAG", "Nivis_SM"));
    expectedOptions.insert(pair<string, string> ("SYSTEM_MANAGER_MODEL", "SM"));
    expectedOptions.insert(pair<string, string> ("SYSTEM_MANAGER_SERIAL_NO", "00000000"));
    expectedOptions.insert(pair<string, string> ("NETWORK_ID", "1"));
    expectedOptions.insert(pair<string, string> ("ROLE_ACTIVATION_PERIOD", "1"));
    expectedOptions.insert(pair<string, string> ("GRAPH_REDUNDANCY_PERIOD", "1"));
    expectedOptions.insert(pair<string, string> ("ROUTES_EVALUATION_PERIOD", "1"));
    expectedOptions.insert(pair<string, string> ("GARBAGE_COLLECTION_PERIOD", "3"));
    expectedOptions.insert(pair<string, string> ("CONSISTENCY_PERIOD", "5"));
    expectedOptions.insert(pair<string, string> ("ACTIVATE_MOCK_KEY_GENERATOR", "false"));
    expectedOptions.insert(pair<string, string> ("LOCK_FILES_FOLDER", "/tmp/"));
    expectedOptions.insert(pair<string, string> ("DISABLE_TL_ENCRYPTION", "false"));
    expectedOptions.insert(pair<string, string> ("DISABLE_AUTHENTICATION", "false"));
    expectedOptions.insert(pair<string, string> ("SNIFFER_DEBUG_MODE", "false"));
    expectedOptions.insert(pair<string, string> ("LOG_NETWORK_STATE_INTERVAL", "40"));
    expectedOptions.insert(pair<string, string> ("PUBLISH_PERIOD", "60"));
    expectedOptions.insert(pair<string, string> ("PUBLISH_PHASE", "50"));
    expectedOptions.insert(pair<string, string> ("NETWORK_MAX_DEVICE_TIMEOUT", "120"));
    expectedOptions.insert(pair<string, string> ("EXPIRED_CONTRACTS_PERIOD", "60"));
    expectedOptions.insert(pair<string, string> ("FIND_BETTER_PARENT_PERIOD", "30"));
    expectedOptions.insert(pair<string, string> ("DIRTY_CONTRACT_CHECK_PERIOD", "1"));
    expectedOptions.insert(pair<string, string> ("UDO_CONTRACTS_CHECK_PERIOD", "5"));
    expectedOptions.insert(pair<string, string> ("DIRTY_EDGES_CHECK", "1"));
    expectedOptions.insert(pair<string, string> ("BLO_PERIOD", "60"));
    expectedOptions.insert(pair<string, string> ("TAI_CUTOVER_OFFSET", "10"));
    expectedOptions.insert(pair<string, string> ("OBJECT_LIFETIME", "600"));//# default object life time (in seconds)
    expectedOptions.insert(pair<string, string> ("JOIN_MAX_DURATION", "300"));
    expectedOptions.insert(pair<string, string> ("MANAGER_NEIGHBOR_COMMITTED_BURST", "20"));
    expectedOptions.insert(pair<string, string> ("NEIGHBOR_MANAGER_COMMITTED_BURST", "1"));
    expectedOptions.insert(pair<string, string> ("IPV6_ADDR_PREFIX", "FC00"));
    expectedOptions.insert(pair<string, string> ("GW_MAX_NSDU_SIZE", "32767"));
    expectedOptions.insert(pair<string, string> ("BBR_MAX_NSDU_SIZE", "96"));//# used for SM-BBR contracts;
	expectedOptions.insert(pair<string, string> ("MAX_NSDU_SIZE", "96"));//# used for SM-devices contracts; OBS: 90 is the maximum safe value (when headers are at max)
    expectedOptions.insert(pair<string, string> ("MAX_SEND_WINDOW_SIZE", "0"));//# should be at least 2; 0 - deactivates window mechanism
    expectedOptions.insert(pair<string, string> ("FIRMWARE_FILES_DIRECTORY", string()));
    expectedOptions.insert(pair<string, string> ("FIRMWARE_CONTRACTS_ENABLED", "true"));
    expectedOptions.insert(pair<string, string> ("FILE_UPLOAD_CHUNK_SIZE", "69"));
    expectedOptions.insert(pair<string, string> ("FILE_UPLOAD_CHUNK_TIMEOUT", "45"));
    expectedOptions.insert(pair<string, string> ("DUPLICATES_TIME_SPAN", "30"));
    expectedOptions.insert(pair<string, string> ("CURRENT_UTC_ADJUSTMENT", string()));
    expectedOptions.insert(pair<string, string> ("NEXT_UTC_ADJUSTMENT_TIME", string()));
    expectedOptions.insert(pair<string, string> ("NEXT_UTC_ADJUSTMENT", string()));
    expectedOptions.insert(pair<string, string> ("DEBUG_COMMANDS_FILENAME", "sm_command.ini"));
    expectedOptions.insert(pair<string, string> ("CHANNEL_BLACKLISTING_ENABLED", "true"));
    expectedOptions.insert(pair<string, string> ("CHANNEL_BLACKLISTING_THRESHOLD_PERCENT", "55"));
    expectedOptions.insert(pair<string, string> ("CHANNEL_BLACKLISTING_KEEP_PERIOD", "600"));
    expectedOptions.insert(pair<string, string> ("CHANNEL_BLACKLISTING_TAI_CUTOVER", "10"));
    expectedOptions.insert(pair<string, string> ("ALERTS_ENABLED", "true"));
    expectedOptions.insert(pair<string, string> ("ALERT_NEIGHBOR_MINUNICAST", "5"));
    expectedOptions.insert(pair<string, string> ("ALERT_NEIGHBOR_ERROR_THRESHOLD", "35"));
    expectedOptions.insert(pair<string, string> ("ALERT_CHANNELDIAG_MINUNICAST", "5"));
    expectedOptions.insert(pair<string, string> ("ALERT_CHANNELDIAG_CCABACKOFF_THRESHOLD", "50"));
    expectedOptions.insert(pair<string, string> ("ALERT_CHANNELDIAG_NOACK_THRESHOLD", "50"));
    expectedOptions.insert(pair<string, string> ("SECURITY_KEY_LIFETIME", "1776"));
    expectedOptions.insert(pair<string, string> ("SUBNET_KEY_LIFETIME", "1164")); //48.5 days (max value)
    expectedOptions.insert(pair<string, string> ("CMDS_MAX_BYTES_LOGGED", "30"));
    expectedOptions.insert(pair<string, string> ("OBEY_CONTRACT_BANDWIDTH", "true"));
    expectedOptions.insert(pair<string, string> ("SUBNET_CONFIG_FILE", "sm_subnet.ini"));
    expectedOptions.insert(pair<string, string> ("MAX_ASL_TIMEOUT", "300"));
    expectedOptions.insert(pair<string, string> ("CONTAINER_EXPIRATION_INTERVAL", "305"));//should be configured not below MAX_ASL_TIMEOUT(see StackWrapper.cpp)
    expectedOptions.insert(pair<string, string> ("NETWORK_MAX_LATENCY", "0"));//default unlimited
    expectedOptions.insert(pair<string, string> ("NETWORK_MAX_NODES", "0"));//default unlimitted
    expectedOptions.insert(pair<string, string> ("PS_CONTRACTS_REFUSAL_TIME_SPAN", "0")); //0 = functionality disabled
    expectedOptions.insert(pair<string, string> ("PS_CONTRACTS_REFUSAL_DEVICE_TIME_SPAN", "0")); //0 = functionality disabled
    expectedOptions.insert(pair<string, string> ("UNSTABLE_CHECK", "false"));
    expectedOptions.insert(pair<string, string> ("ENABLE_ALERTS_GENERATED_BY_SM", "true"));
    expectedOptions.insert(pair<string, string> ("UDO_ALERT_PERIOD", "20")); //defaults to 20. Accepted parameter range is [5-120].
    expectedOptions.insert(pair<string, string> ("JOIN_LEAVE_ALERT_PERIOD", "0")); //defaults to 0. The parameter range is [0-60].
    expectedOptions.insert(pair<string, string> ("CONTRACT_ALERT_PERIOD", "0")); //defaults to 0. The parameter range is [0-60].
    expectedOptions.insert(pair<string, string> ("TOPOLOGY_ALERT_PERIOD", "0")); //defaults to 0. The parameter range is [0-60].
    expectedOptions.insert(pair<string, string> ("FULL_ADV_ROUTERS_PERIOD", "9")); //defaults to 9 seconds.
    expectedOptions.insert(pair<string, string> ("CHANGE_PARENT_BY_LOAD", "false"));
    expectedOptions.insert(pair<string, string> ("FAST_DISCOVERY_CHECK", "4")); //defaults to 14 seconds.
    expectedOptions.insert(pair<string, string> ("LOG_LEVEL_STACK", "1")); //stack log level (3 dbg, 2 inf, 1 err). If 0 will default to stack wrapper log level set in log4cpp.properties

    map<string, string> options;

    // read expected options
    map<string, string>::const_iterator it = expectedOptions.begin();
    for (; it != expectedOptions.end(); ++it) {
        const char* defaultValue = (*it).second.c_str();
        const char* value = iniParser.GetValue("SYSTEM_MANAGER", (*it).first.c_str(), defaultValue);
        if (strcmp(value, "") == 0) {
            LOG_ERROR("Required option " << "SYSTEM_MANAGER" << '.' << (*it).first << " not found in " << commonIniFileName);
            exit(1);
        }
        string key = "SYSTEM_MANAGER." + (*it).first;
        options[key] = string(value);
    }

    try {
        Isa100::Common::UdpEndpoint systemManagerEndpoint;
        SmSettingsLogic::ParseAddrPortLine(options["SYSTEM_MANAGER.SYSTEM_MANAGER_IPv6"],
                    options["SYSTEM_MANAGER.SYSTEM_MANAGER_IPv4"], options["SYSTEM_MANAGER.SYSTEM_MANAGER_Port"],
                    managerAddress128, systemManagerEndpoint);
        managerAddress128.setAddressString();
        listenPort = systemManagerEndpoint.port;
        LOG_INFO("System manager IPv4: " << systemManagerEndpoint.ipv4 << ", IPv6: " << managerAddress128.toString()
                    << ", port: " << listenPort);
    } catch (const NE::Common::NEException& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.SYSTEM_MANAGER: " << ex.what());
        exit(1);
    }

    try {
        managerAddress64.loadShortString(options["SYSTEM_MANAGER.SECURITY_MANAGER_EUI64"]);
        LOG_INFO("SECURITY_MANAGER_EUI64: " << managerAddress64.toString());
    } catch (const NE::Common::NEException& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.SECURITY_MANAGER_EUI64: " << ex.what());
        exit(1);
    }

    try {
        managerTag = options["SYSTEM_MANAGER.SYSTEM_MANAGER_TAG"];
        LOG_INFO("SYSTEM_MANAGER_TAG: " << managerTag);
    } catch (const NE::Common::NEException& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.SYSTEM_MANAGER_TAG: " << ex.what());
        exit(1);
    }

    try {
        managerModel = options["SYSTEM_MANAGER.SYSTEM_MANAGER_MODEL"];
        LOG_INFO("SYSTEM_MANAGER_MODEL: " << managerModel);
    } catch (const NE::Common::NEException& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.SYSTEM_MANAGER_MODEL: " << ex.what());
        exit(1);
    }

    try {
        managerSerialNo = options["SYSTEM_MANAGER.SYSTEM_MANAGER_SERIAL_NO"];
        LOG_INFO("SYSTEM_MANAGER_SERIAL_NO: " << managerSerialNo);
    } catch (const NE::Common::NEException& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.SYSTEM_MANAGER_SERIAL_NO: " << ex.what());
        exit(1);
    }

    READ_OPTION_NUMBER(networkID, "SYSTEM_MANAGER.NETWORK_ID", Uint32, "unsigned integer");

    READ_OPTION_NUMBER(roleActivationPeriod, "SYSTEM_MANAGER.ROLE_ACTIVATION_PERIOD", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(graphRedundancyPeriod, "SYSTEM_MANAGER.GRAPH_REDUNDANCY_PERIOD", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(routesEvaluationPeriod, "SYSTEM_MANAGER.ROUTES_EVALUATION_PERIOD", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(garbageCollectionPeriod, "SYSTEM_MANAGER.GARBAGE_COLLECTION_PERIOD", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(consistencyCheckPeriod, "SYSTEM_MANAGER.CONSISTENCY_PERIOD", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(networkMaxDeviceTimeout, "SYSTEM_MANAGER.NETWORK_MAX_DEVICE_TIMEOUT", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(expiredContractsPeriod, "SYSTEM_MANAGER.EXPIRED_CONTRACTS_PERIOD", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(findBetterParentPeriod, "SYSTEM_MANAGER.FIND_BETTER_PARENT_PERIOD", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(dirtyContractCheckPeriod, "SYSTEM_MANAGER.DIRTY_CONTRACT_CHECK_PERIOD", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(udoContractsCheckPeriod, "SYSTEM_MANAGER.UDO_CONTRACTS_CHECK_PERIOD", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(dirtyEdgesCkeckPeriod,  "SYSTEM_MANAGER.DIRTY_EDGES_CHECK", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(bloPeriod, "SYSTEM_MANAGER.BLO_PERIOD", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(fullAdvertisingRoutersPeriod, "SYSTEM_MANAGER.FULL_ADV_ROUTERS_PERIOD", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(fastDiscoveryCheck, "SYSTEM_MANAGER.FAST_DISCOVERY_CHECK", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(cmdsMaxBytesLogged, "SYSTEM_MANAGER.CMDS_MAX_BYTES_LOGGED", Uint32, "unsigned integer");
    READ_OPTION_BOOLEAN(activateMockKeyGenerator, "SYSTEM_MANAGER.ACTIVATE_MOCK_KEY_GENERATOR");
    READ_OPTION_NUMBER(networkMaxLatencyPercent, "SYSTEM_MANAGER.NETWORK_MAX_LATENCY", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(networkMaxNodes, "SYSTEM_MANAGER.NETWORK_MAX_NODES", Uint16, "unsigned integer");

    READ_OPTION_NUMBER(subnetKeysHardLifeTime, "SYSTEM_MANAGER.SUBNET_KEY_LIFETIME", Uint16, "unsigned integer");

    READ_OPTION_NUMBER(psContractsRefusalTimeSpan, "SYSTEM_MANAGER.PS_CONTRACTS_REFUSAL_TIME_SPAN", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(psContractsRefusalDeviceTimeSpan, "SYSTEM_MANAGER.PS_CONTRACTS_REFUSAL_DEVICE_TIME_SPAN", Uint16, "unsigned integer");

    try {
        keysHardLifeTime = boost::lexical_cast<Uint32>(options["SYSTEM_MANAGER.SECURITY_KEY_LIFETIME"]);
        Uint32 minimumHardLifeTime = 6; //minimum value
        if (keysHardLifeTime < minimumHardLifeTime) {
            keysHardLifeTime = minimumHardLifeTime;
            LOG_WARN("SECURITY_KEY_LIFETIME must be at least " << (int) minimumHardLifeTime
                        << ". Unless a greater value is given, the minimum accepted value (" << (int) minimumHardLifeTime
                        << ") will be used.");
        } else {
            LOG_INFO("SECURITY_KEY_LIFETIME: " << (int) keysHardLifeTime);
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.SECURITY_KEY_LIFETIME: " << ex.what());
        exit(1);
    }

    try {
        channelBlacklistingEnabled = options["SYSTEM_MANAGER.CHANNEL_BLACKLISTING_ENABLED"] == "true";
        LOG_INFO("CHANNEL_BLACKLISTING_ENABLED: " << (channelBlacklistingEnabled ? "true" : "false"));
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.CHANNEL_BLACKLISTING_ENABLED missing. Assuming false!");
        channelBlacklistingEnabled = false;
    }

    READ_OPTION_NUMBER(channelBlacklistingThresholdPercent, "SYSTEM_MANAGER.CHANNEL_BLACKLISTING_THRESHOLD_PERCENT", Uint32, "unsigned integer");
    READ_OPTION_NUMBER(channelBlacklistingKeepPeriod, "SYSTEM_MANAGER.CHANNEL_BLACKLISTING_KEEP_PERIOD", Uint32, "unsigned integer");
    READ_OPTION_NUMBER(channelBlacklistingTAICutover, "SYSTEM_MANAGER.CHANNEL_BLACKLISTING_TAI_CUTOVER", Uint32, "unsigned integer");

    try {
        alertsEnabled = options["SYSTEM_MANAGER.ALERTS_ENABLED"] == "true";
        LOG_INFO("ALERTS_ENABLED: " << (alertsEnabled ? "true" : "false"));
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.ALERTS_ENABLED missing. Assuming false!");
        alertsEnabled = false;
    }

    try {
        alertsNeiMinUnicast = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.ALERT_NEIGHBOR_MINUNICAST"]);
        LOG_INFO("ALERT_NEIGHBOR_MINUNICAST: " << (int)alertsNeiMinUnicast);
    } catch (boost::bad_lexical_cast & ex) {
        LOG_ERROR("In " << commonIniFileName
                    << ", option SYSTEM_MANAGER.ALERT_NEIGHBOR_MINUNICAST: invalid value, expected unsigned integer");
        exit(1);
    }

    try {
        alertsNeiErrThresh = boost::lexical_cast<int>(options["SYSTEM_MANAGER.ALERT_NEIGHBOR_ERROR_THRESHOLD"]);
        LOG_INFO("ALERT_NEIGHBOR_ERROR_THRESHOLD: " << (int)alertsNeiErrThresh);
    } catch (boost::bad_lexical_cast & ex) {
        LOG_ERROR("In " << commonIniFileName
                    << ", option SYSTEM_MANAGER.ALERT_NEIGHBOR_ERROR_THRESHOLD: invalid value, expected unsigned integer");
        exit(1);
    }

    try {
        alertsChanMinUnicast = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.ALERT_CHANNELDIAG_MINUNICAST"]);
        LOG_INFO("ALERT_CHANNELDIAG_MINUNICAST: " << (int)alertsChanMinUnicast);
    } catch (boost::bad_lexical_cast & ex) {
        LOG_ERROR("In " << commonIniFileName
                    << ", option SYSTEM_MANAGER.ALERT_CHANNELDIAG_MINUNICAST: invalid value, expected unsigned integer");
        exit(1);
    }

    try {
        alertsCcaBackoffThresh = boost::lexical_cast<int>(options["SYSTEM_MANAGER.ALERT_CHANNELDIAG_CCABACKOFF_THRESHOLD"]);
        LOG_INFO("ALERT_CHANNELDIAG_CCABACKOFF_THRESHOLD: " << (int)alertsCcaBackoffThresh);
    } catch (boost::bad_lexical_cast & ex) {
        LOG_ERROR("In " << commonIniFileName
                    << ", option SYSTEM_MANAGER.ALERT_CHANNELDIAG_CCABACKOFF_THRESHOLD: invalid value, expected unsigned integer");
        exit(1);
    }

    try {
        alertsNoAckThresh = boost::lexical_cast<int>(options["SYSTEM_MANAGER.ALERT_CHANNELDIAG_NOACK_THRESHOLD"]);
        LOG_INFO("ALERT_CHANNELDIAG_NOACK_THRESHOLD: " << (int)alertsNoAckThresh);
    } catch (boost::bad_lexical_cast & ex) {
        LOG_ERROR("In " << commonIniFileName
                    << ", option SYSTEM_MANAGER.ALERT_CHANNELDIAG_NOACK_THRESHOLD: invalid value, expected unsigned integer");
        exit(1);
    }

    try {
        lockFilesFolder = options["SYSTEM_MANAGER.LOCK_FILES_FOLDER"];
        LOG_INFO("LOCK_FILES_FOLDER: " << lockFilesFolder);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.LOCK_FILES_FOLDER: " << ex.what());
        exit(1);
    }

    try {
        debugMode = options["SYSTEM_MANAGER.SNIFFER_DEBUG_MODE"] == "true";
        LOG_INFO("SNIFFER_DEBUG_MODE: " << (debugMode ? "true" : "false"));
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.SNIFFER_DEBUG_MODE missing. Assuming false!");
        debugMode = false;
    }

    string strDisableTLEncryption = options["SYSTEM_MANAGER.DISABLE_TL_ENCRYPTION"];
    disableTLEncryption = strDisableTLEncryption == "true";
    LOG_INFO("DISABLE_TL_ENCRYPTION:" << disableTLEncryption);

    disableAuthentication = options["SYSTEM_MANAGER.DISABLE_AUTHENTICATION"] == "true";
    LOG_INFO("DISABLE_AUTHENTICATION:" << disableAuthentication);

    try {
        logNetworkStateInterval = boost::lexical_cast<int>(options["SYSTEM_MANAGER.LOG_NETWORK_STATE_INTERVAL"]);
        LOG_INFO("LOG_NETWORK_STATE_INTERVAL: " << logNetworkStateInterval);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.LOG_NETWORK_STATE_INTERVAL: " << ex.what());
        exit(1);
    }

    try {
        publishPeriod = boost::lexical_cast<int>(options["SYSTEM_MANAGER.PUBLISH_PERIOD"]);
        LOG_INFO("PUBLISH_PERIOD: " << (int) publishPeriod);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.PUBLISH_PERIOD: " << ex.what());
        exit(1);
    }

    try {
        publishPhase = boost::lexical_cast<int>(options["SYSTEM_MANAGER.PUBLISH_PHASE"]);
        LOG_INFO("PUBLISH_PHASE: " << (int) publishPhase);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.PUBLISH_PHASE: " << ex.what());
        exit(1);
    }

    try {
        tai_cutover_offset = boost::lexical_cast<int>(options["SYSTEM_MANAGER.TAI_CUTOVER_OFFSET"]);
        LOG_INFO("TAI_CUTOVER_OFFSET: " << (int) tai_cutover_offset);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.TAI_CUTOVER_OFFSET: " << ex.what());
        exit(1);
    }

    try {
        objectLifeTime = boost::lexical_cast<int>(options["SYSTEM_MANAGER.OBJECT_LIFETIME"]);
        LOG_INFO("OBJECT_LIFETIME: " << (int) objectLifeTime);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.OBJECT_LIFETIME: " << ex.what());
        exit(1);
    }

    try {
        joinMaxDuration = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.JOIN_MAX_DURATION"]);
        LOG_INFO("JOIN_MAX_DURATION: " << (int) joinMaxDuration);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.JOIN_MAX_DURATION: " << ex.what());
        exit(1);
    }

    try {
        manager2NeighborCommittedBurst = boost::lexical_cast<int>(options["SYSTEM_MANAGER.MANAGER_NEIGHBOR_COMMITTED_BURST"]);
        LOG_INFO("MANAGER_NEIGHBOR_COMMITTED_BURST: " << (int) manager2NeighborCommittedBurst);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.MANAGER_NEIGHBOR_COMMITTED_BURST: " << ex.what());
        exit(1);
    }

    try {
        neighbor2ManagerCommittedBurst = boost::lexical_cast<int>(options["SYSTEM_MANAGER.NEIGHBOR_MANAGER_COMMITTED_BURST"]);
        LOG_INFO("NEIGHBOR_MANAGER_COMMITTED_BURST: " << (int) neighbor2ManagerCommittedBurst);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.NEIGHBOR_MANAGER_COMMITTED_BURST: " << ex.what());
        exit(1);
    }

    try {
        if ((strncmp(options["SYSTEM_MANAGER.IPV6_ADDR_PREFIX"].c_str(), "FE80", 4) == 0) || (strncmp(
                    options["SYSTEM_MANAGER.IPV6_ADDR_PREFIX"].c_str(), "fe80", 4) == 0)) {
            ipv6AddrPrefix = 0xFE80;
        } else {
            ipv6AddrPrefix = 0xFC00;
        }
        LOG_INFO("IPV6_ADDR_PREFIX: " << (int) ipv6AddrPrefix);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.IPV6_ADDR_PREFIX: " << ex.what());
        exit(1);
    }

    try {
        gw_max_NSDU_Size = boost::lexical_cast<int>(options["SYSTEM_MANAGER.GW_MAX_NSDU_SIZE"]);
        LOG_INFO("GW_MAX_NSDU_SIZE: " << (int) gw_max_NSDU_Size);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.GW_MAX_NSDU_SIZE: " << ex.what());
        exit(1);
    }

    try {
        bbr_max_NSDU_Size = boost::lexical_cast<int>(options["SYSTEM_MANAGER.BBR_MAX_NSDU_SIZE"]);
        LOG_INFO("BBR_MAX_NSDU_SIZE: " << (int) bbr_max_NSDU_Size);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.BBR_MAX_NSDU_SIZE: " << ex.what());
        exit(1);
    }

    try {
        max_NSDU_Size = boost::lexical_cast<int>(options["SYSTEM_MANAGER.MAX_NSDU_SIZE"]);
        LOG_INFO("MAX_NSDU_SIZE: " << (int) max_NSDU_Size);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.MAX_NSDU_SIZE: " << ex.what());
        exit(1);
    }

    try {
        maxSendWindowSize = boost::lexical_cast<int>(options["SYSTEM_MANAGER.MAX_SEND_WINDOW_SIZE"]);
        LOG_INFO("MAX_SEND_WINDOW_SIZE: " << (int) maxSendWindowSize);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.MAX_SEND_WINDOW_SIZE: " << ex.what());
        exit(1);
    }

    try {
        firmwareFilesDirectory = options["SYSTEM_MANAGER.FIRMWARE_FILES_DIRECTORY"];
        LOG_INFO("FIRMWARE_FILES_DIRECTORY: " << firmwareFilesDirectory);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.FIRMWARE_FILES_DIRECTORY: " << ex.what());
        exit(1);
    }

    try {
        firmwareContractsEnabled = options["SYSTEM_MANAGER.FIRMWARE_CONTRACTS_ENABLED"] == "true";
        LOG_INFO("FIRMWARE_CONTRACTS_ENABLED: " << (firmwareContractsEnabled ? "true" : "false"));
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.FIRMWARE_CONTRACTS_ENABLED missing. Assuming false!");
        firmwareContractsEnabled = false;
    }

    try {
        fileUploadChunkSize = boost::lexical_cast<int>(options["SYSTEM_MANAGER.FILE_UPLOAD_CHUNK_SIZE"]);
        LOG_INFO("FILE_UPLOAD_CHUNK_SIZE: " << (int) fileUploadChunkSize);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.FILE_UPLOAD_CHUNK_SIZE: " << ex.what());
        exit(1);
    }

    try {
        fileUploadChunkTimeout = boost::lexical_cast<int>(options["SYSTEM_MANAGER.FILE_UPLOAD_CHUNK_TIMEOUT"]);
        LOG_INFO("FILE_UPLOAD_CHUNK_TIMEOUT: " << (int) fileUploadChunkTimeout);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.FILE_UPLOAD_CHUNK_TIMEOUT: " << ex.what());
        exit(1);
    }

    try {
        duplicatesTimeSpan = boost::lexical_cast<int>(options["SYSTEM_MANAGER.DUPLICATES_TIME_SPAN"]);
        LOG_INFO("DUPLICATES_TIME_SPAN: " << (int) duplicatesTimeSpan);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.DUPLICATES_TIME_SPAN: " << ex.what());
        exit(1);
    }

    try {
        currentUTCAdjustment = boost::lexical_cast<int>(options["SYSTEM_MANAGER.CURRENT_UTC_ADJUSTMENT"]);
        LOG_INFO("CURRENT_UTC_ADJUSTMENT: " << currentUTCAdjustment);
        NE::Common::ClockSource::currentUTCAdjustment = currentUTCAdjustment;
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.CURRENT_UTC_ADJUSTMENT: " << ex.what());
        exit(1);
    }

    try {
        nextUTCAdjustmentTime = boost::lexical_cast<int>(options["SYSTEM_MANAGER.NEXT_UTC_ADJUSTMENT_TIME"]);
        LOG_INFO("NEXT_UTC_ADJUSTMENT_TIME: " << nextUTCAdjustmentTime);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.NEXT_UTC_ADJUSTMENT_TIME: " << ex.what());
        exit(1);
    }

    try {
        nextUTCAdjustment = boost::lexical_cast<int>(options["SYSTEM_MANAGER.NEXT_UTC_ADJUSTMENT"]);
        LOG_INFO("NEXT_UTC_ADJUSTMENT: " << nextUTCAdjustment);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.NEXT_UTC_ADJUSTMENT: " << ex.what());
        exit(1);
    }

    READ_OPTION_STRING(debugCommandsFileName, "SYSTEM_MANAGER.DEBUG_COMMANDS_FILENAME");
    READ_OPTION_STRING(subnetConfigFileName, "SYSTEM_MANAGER.SUBNET_CONFIG_FILE");
    READ_OPTION_BOOLEAN(obeyContractBandwidth, "SYSTEM_MANAGER.OBEY_CONTRACT_BANDWIDTH");

    READ_OPTION_BOOLEAN(unstableCheck, "SYSTEM_MANAGER.UNSTABLE_CHECK");
    READ_OPTION_BOOLEAN(enableAlertsGeneratedBySM, "SYSTEM_MANAGER.ENABLE_ALERTS_GENERATED_BY_SM");
    READ_OPTION_NUMBER(logLevelStack, "SYSTEM_MANAGER.LOG_LEVEL_STACK", Uint16, "unsigned integer");

    try {
        udoAlertPeriod = boost::lexical_cast<int>(options["SYSTEM_MANAGER.UDO_ALERT_PERIOD"]);
        if (udoAlertPeriod < 5 || udoAlertPeriod > 120) {
            LOG_WARN("Parameter range for UDO_ALERT_PERIOD is [5-120]. Invalid value set=" << udoAlertPeriod
                        << ". Setting the default value 20.");
            udoAlertPeriod = 20;
        }
        LOG_INFO("UDO_ALERT_PERIOD: " << udoAlertPeriod);
    } catch (boost::bad_lexical_cast & ex) {
        LOG_ERROR("In " << commonIniFileName
                    << ", option SYSTEM_MANAGER.UDO_ALERT_PERIOD: invalid value, expected unsigned integer");
        exit(1);
    }

    try {
        joinLeaveAlertPeriod = boost::lexical_cast<int>(options["SYSTEM_MANAGER.JOIN_LEAVE_ALERT_PERIOD"]);
        if (joinLeaveAlertPeriod < 0 || joinLeaveAlertPeriod > 60) {
            LOG_WARN("Parameter range for JOIN_LEAVE_ALERT_PERIOD is [0-60]. Invalid value set=" << joinLeaveAlertPeriod
                        << ". Setting the default value 0.");
            joinLeaveAlertPeriod = 0;
        }
        LOG_INFO("JOIN_LEAVE_ALERT_PERIOD: " << joinLeaveAlertPeriod);
    } catch (boost::bad_lexical_cast & ex) {
        LOG_ERROR("In " << commonIniFileName
                    << ", option SYSTEM_MANAGER.JOIN_LEAVE_ALERT_PERIOD: invalid value, expected unsigned integer");
        exit(1);
    }

    try {
        contractAlertPeriod = boost::lexical_cast<int>(options["SYSTEM_MANAGER.CONTRACT_ALERT_PERIOD"]);
        if (contractAlertPeriod < 0 || contractAlertPeriod > 60) {
            LOG_WARN("Parameter range for CONTRACT_ALERT_PERIOD is [0-60]. Invalid value set=" << contractAlertPeriod
                        << ". Setting the default value 0.");
            contractAlertPeriod = 0;
        }
        LOG_INFO("CONTRACT_ALERT_PERIOD: " << contractAlertPeriod);
    } catch (boost::bad_lexical_cast & ex) {
        LOG_ERROR("In " << commonIniFileName
                    << ", option SYSTEM_MANAGER.CONTRACT_ALERT_PERIOD: invalid value, expected unsigned integer");
        exit(1);
    }

    try {
        topologyAlertPeriod = boost::lexical_cast<int>(options["SYSTEM_MANAGER.TOPOLOGY_ALERT_PERIOD"]);
        if (topologyAlertPeriod < 0 || topologyAlertPeriod > 60) {
            LOG_WARN("Parameter range for TOPOLOGY_ALERT_PERIOD is [0-60]. Invalid value set=" << topologyAlertPeriod
                        << ". Setting the default value 0.");
            topologyAlertPeriod = 0;
        }
        LOG_INFO("TOPOLOGY_ALERT_PERIOD : " << topologyAlertPeriod);
    } catch (boost::bad_lexical_cast & ex) {
        LOG_ERROR("In " << commonIniFileName
                    << ", option SYSTEM_MANAGER.TOPOLOGY_ALERT_PERIOD : invalid value, expected unsigned integer");
        exit(1);
    }

    try {
        maxASLTimeout = boost::lexical_cast<int>(options["SYSTEM_MANAGER.MAX_ASL_TIMEOUT"]);
        if (maxASLTimeout < 100 || maxASLTimeout > 400) {
            LOG_WARN("Parameter range for MAX_ASL_TIMEOUT is [100-400]. Invalid value set=" << maxASLTimeout
                        << ". Setting the default value 300.");
            maxASLTimeout = 300;
        }
        LOG_INFO("MAX_ASL_TIMEOUT: " << maxASLTimeout);
    } catch (boost::bad_lexical_cast & ex) {
        LOG_ERROR("In " << commonIniFileName
                    << ", option SYSTEM_MANAGER.MAX_ASL_TIMEOUT: invalid value, expected unsigned integer");
        exit(1);
    }

    //READ_OPTION_NUMBER(containerExpirationInterval, "SYSTEM_MANAGER.CONTAINER_EXPIRATION_INTERVAL", Uint16, "unsigned integer");
    try {
        containerExpirationInterval = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.CONTAINER_EXPIRATION_INTERVAL"]);
        if (containerExpirationInterval <= maxASLTimeout) { //set containerExpirationInterval to be greater than maxASLTimeout
            LOG_WARN("CONTAINER_EXPIRATION_INTERVAL must be greater than MAX_ASL_TIMEOUT. Invalid value set=" << containerExpirationInterval
                        << ". Setting a default value of " << maxASLTimeout + 5);
            containerExpirationInterval = maxASLTimeout + 5;
        }
        LOG_INFO("CONTAINER_EXPIRATION_INTERVAL: " << containerExpirationInterval);
    } catch (boost::bad_lexical_cast & ex) {
        LOG_ERROR("In " << commonIniFileName
                    << ", option SYSTEM_MANAGER.CONTAINER_EXPIRATION_INTERVAL: invalid value, expected unsigned integer");
        exit(1);
    }

    try {
        changeParentByLoad = options["SYSTEM_MANAGER.CHANGE_PARENT_BY_LOAD"] == "true";
        LOG_INFO("FIRMWARE_CONTRACTS_ENABLED: " << (changeParentByLoad ? "true" : "false"));
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << commonIniFileName << ", option SYSTEM_MANAGER.CHANGE_PARENT_BY_LOAD missing. Assuming false!");
        changeParentByLoad = false;
    }


}

void SmSettingsLogic::ParseSubnetConfigFile(SimpleIni::CSimpleIniCaseA& iniParser, SubnetSettings& subnetSettings,
            const std::string& fileName, bool isReloadOnSignal, bool isDefaultSubnetLoad) {
    //subnetSettings has the default values as initialized in SubnetSettings constructor

    //map < option_name , default_value >
    map<string, string> expectedOptions;

    expectedOptions.insert(pair<string, string> ("CHANNEL_LIST", subnetSettings.getChannelListAsString()));
    expectedOptions.insert(pair<string, string> ("REDUCED_CHANNEL_LIST", subnetSettings.getReducedChannelHoppingListAsString()));
    expectedOptions.insert(pair<string, string> ("NEIGHBOR_DIAG_CHANNEL_LIST", subnetSettings.getNeighborDiscoveryListAsString()));
    expectedOptions.insert(pair<string, string> ("JOIN_RESERVED_SET", CAST_STR( subnetSettings.join_reserved_set)));
    expectedOptions.insert(pair<string, string> ("JOIN_ADV_PERIOD", CAST_STR( subnetSettings.join_adv_period)));
    expectedOptions.insert(pair<string, string> ("JOIN_BBR_ADV_PERIOD", CAST_STR( subnetSettings.join_bbr_adv_period)));
    expectedOptions.insert(pair<string, string> ("JOIN_LINKS_PERIOD", CAST_STR( subnetSettings.join_rxtx_period)));
    expectedOptions.insert(pair<string, string> ("MNG_LINK_R_OUT", CAST_STR(subnetSettings.mng_link_r_out)));
    expectedOptions.insert(pair<string, string> ("MNG_LINK_R_IN", CAST_STR(subnetSettings.mng_link_r_in)));
    expectedOptions.insert(pair<string, string> ("MNG_LINK_S_OUT", CAST_STR(subnetSettings.mng_link_s_out)));
    expectedOptions.insert(pair<string, string> ("MNG_LINK_S_IN", CAST_STR(subnetSettings.mng_link_s_in)));
    expectedOptions.insert(pair<string, string> ("MNG_LINK_S_IN_STAR", CAST_STR(subnetSettings.mng_link_s_in_Star)));
    expectedOptions.insert(pair<string, string> ("MNG_R_IN_BAND", CAST_STR(subnetSettings.mng_r_in_band)));
    expectedOptions.insert(pair<string, string> ("MNG_S_IN_BAND", CAST_STR(subnetSettings.mng_s_in_band)));
    expectedOptions.insert(pair<string, string> ("MNG_S_IN_BAND_STAR", CAST_STR(subnetSettings.mng_s_in_band_Star)));
    expectedOptions.insert(pair<string, string> ("MNG_R_OUT_BAND", CAST_STR(subnetSettings.mng_r_out_band)));
    expectedOptions.insert(pair<string, string> ("MNG_S_OUT_BAND", CAST_STR(subnetSettings.mng_s_out_band)));
    expectedOptions.insert(
                pair<string, string> ("MNG_ALLOC_BAND", CAST_STR(subnetSettings.mng_alloc_band)));
    expectedOptions.insert(pair<string, string> ("MNG_DISC_BBR", CAST_STR(subnetSettings.mng_disc_bbr)));
    expectedOptions.insert(pair<string, string> ("MNG_DISC_R", CAST_STR(subnetSettings.mng_disc_r)));
    expectedOptions.insert(pair<string, string> ("MNG_DISC_S", CAST_STR(subnetSettings.mng_disc_s)));
    expectedOptions.insert(pair<string, string> ("RETRY_PROC", CAST_STR(subnetSettings.retry_proc)));
    expectedOptions.insert(pair<string, string> ("SLOTS_NEIGHBOR_DISCOVERY", "" )); //CAST_STR(subnetSettings.slotsNeighborDiscovery)));
    expectedOptions.insert(pair<string, string> ("ENABLE_ALERTS_FOR_GATEWAY", subnetSettings.enableAlertsForGateway ? "true"
                : "false"));
    expectedOptions.insert(pair<string, string> ("ALERT_RECEIVING_OBJECT_ID", CAST_STR(subnetSettings.alertReceivingObjectID)));
    expectedOptions.insert(pair<string, string> ("ALERT_RECEIVING_PORT", CAST_STR(subnetSettings.alertReceivingPort)));
    expectedOptions.insert(pair<string, string> ("ALERT_TIMEOUT", CAST_STR(subnetSettings.alertTimeout)));
    expectedOptions.insert(pair<string, string> ("NR_ROUTERS_PER_BBR", CAST_STR(subnetSettings.nrRoutersPerBBR)));
    expectedOptions.insert(pair<string, string> ("NR_NON_ROUTERS_PER_BBR", CAST_STR(subnetSettings.nrNonRoutersPerBBR)));
    expectedOptions.insert(pair<string, string> ("ENABLE_STAR_TOPOLOGY", subnetSettings.enableStarTopology ? "true" : "false"));
    expectedOptions.insert(pair<string, string> ("ENABLE_STAR_TOPOLOGY_ROUTERS", subnetSettings.enableStarTopologyRouters ? "true" : "false"));
    expectedOptions.insert(pair<string, string> ("NR_ROUTERS_PER_BBR_STAR", CAST_STR(subnetSettings.nrRoutersPerBBRinStar)));
    expectedOptions.insert(pair<string, string> ("NR_NON_ROUTERS_PER_BBR_STAR", CAST_STR(subnetSettings.nrNonRoutersPerBBRinStar)));
    expectedOptions.insert(pair<string, string> ("NR_ROUTERS_PER_ROUTER", CAST_STR(subnetSettings.nrRoutersPerRouter)));
    expectedOptions.insert(pair<string, string> ("NR_NON_ROUTERS_PER_ROUTER", CAST_STR(subnetSettings.nrNonRoutersPerRouter)));
    expectedOptions.insert(pair<string, string> ("MAX_NUMBER_OF_LAYERS", CAST_STR(subnetSettings.maxNumberOfLayers)));
    expectedOptions.insert(pair<string, string> ("ENABLE_MULTI_PATH", subnetSettings.enableMultiPath ? "true" : "false"));
    expectedOptions.insert(pair<string, string> ("RANDOMIZE_CHANNELS_ON_STARTUP", subnetSettings.randomizeChannelsOnStartup ? "true" : "false"));
    expectedOptions.insert(pair<string, string> ("TIME_TO_IGNORE_BAD_RATE",  CAST_STR(subnetSettings.timeToIgnoreBadRateDevice)));
    expectedOptions.insert(pair<string, string> ("BAD_TRANSFER_RATE_THRESHOLD",  CAST_STR(subnetSettings.badTransferRateThreshlod)));
    expectedOptions.insert(pair<string, string> ("BAD_TRANSFER_RATE_THRESHOLD_ON_BACKUP",  CAST_STR(subnetSettings.badTransferRateThreshlodOnBackup)));
    expectedOptions.insert(pair<string, string> ("BAD_TRANSFER_RATE_THRESHOLD_SHORT",  CAST_STR(subnetSettings.badTransferRateThreshlodShort)));
    expectedOptions.insert(pair<string, string> ("BAD_TRANSFER_RATE_THRESHOLD_ON_BACKUP_SHORT",  CAST_STR(subnetSettings.badTransferRateThreshlodOnBackupShort)));
    expectedOptions.insert(pair<string, string> ("NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE_SHORT",  CAST_STR(subnetSettings.numberOfSentPackagesForFailRateShort)));
    expectedOptions.insert(pair<string, string> ("NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE_ON_BACKUP_SHORT",  CAST_STR(subnetSettings.numberOfSentPackagesForFailRateOnBackupShort)));
     expectedOptions.insert(pair<string, string> ("NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE",  CAST_STR(subnetSettings.numberOfSentPackagesForFailRate)));
    expectedOptions.insert(pair<string, string> ("NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE_ON_BACKUP",  CAST_STR(subnetSettings.numberOfSentPackagesForFailRateOnBackup)));
    expectedOptions.insert(pair<string, string> ("K1_FACTOR_ON_EDGE_COST",  CAST_STR(subnetSettings.k1factorOnEdgeCost)));
    expectedOptions.insert(pair<string, string> ("ACCELERATION_INTERVAL", CAST_STR(subnetSettings.accelerationInterval)));
    expectedOptions.insert(pair<string, string> ("DELAY_ACTIVATE_MULTIPATH", CAST_STR(subnetSettings.delayActivateMultiPathForDevice)));
    expectedOptions.insert(pair<string, string> ("DELAY_CONFIGURE_PUBLISH", CAST_STR(subnetSettings.delayConfigurePublish)));
    expectedOptions.insert(pair<string, string> ("DELAY_CONFIGURE_ALERTS", CAST_STR(subnetSettings.delayConfigureAlerts)));
    expectedOptions.insert(pair<string, string> ("DELAY_CONFIGURE_NEIGHBOR_DISCOVERY", CAST_STR(subnetSettings.delayConfigureNeighborDiscovery)));
    expectedOptions.insert(pair<string, string> ("DELAY_ACTIVATE_ROUTER_ROLE", CAST_STR(subnetSettings.delayActivateRouterRole)));
    expectedOptions.insert(pair<string, string> ("DELAY_POST_JOIN_TASKS", CAST_STR(subnetSettings.delayPostJoinTasks)));

    expectedOptions.insert(pair<string, string> ("SF_NEIGHBOR_DICOVERY_LENGTH", CAST_STR(subnetSettings.superframeNeighborDiscoveryLength)));
    expectedOptions.insert(pair<string, string> ("SF_NEIGHBOR_DICOVERY_FAST_LENGTH", "0"));//if 0 it will be calculated as advPer * 100 + 100;
    expectedOptions.insert(pair<string, string> ("FAST_DICOVERY_TIMESPAN", CAST_STR(subnetSettings.fastDiscoveryTimespan)));

    expectedOptions.insert(pair<string, string> ("CONTRACTS_METADATA_THRESHOLD", CAST_STR(subnetSettings.contractsMetadataThreshold)));
    expectedOptions.insert(pair<string, string> ("NEIGHBORS_METADATA_THRESHOLD", CAST_STR(subnetSettings.neighborsMetadataThreshold)));
    expectedOptions.insert(pair<string, string> ("SUPERFRAMES_METADATA_THRESHOLD", CAST_STR(subnetSettings.superframesMetadataThreshold)));
    expectedOptions.insert(pair<string, string> ("GRAPHS_METADATA_THRESHOLD", CAST_STR(subnetSettings.graphsMetadataThreshold)));
    expectedOptions.insert(pair<string, string> ("LINKS_METADATA_THRESHOLD", CAST_STR(subnetSettings.linksMetadataThreshold)));
    expectedOptions.insert(pair<string, string> ("ROUTES_METADATA_THRESHOLD", CAST_STR(subnetSettings.routesMetadataThreshold)));
    expectedOptions.insert(pair<string, string> ("DIAGS_METADATA_THRESHOLD", CAST_STR(subnetSettings.diagsMetadataThreshold)));
    expectedOptions.insert(pair<string, string> ("SUPERFRAME_BIRTH", CAST_STR(subnetSettings.superframeBirth)));
    expectedOptions.insert(pair<string, string> ("ADV_CHANNELS_MASK", CAST_STR(subnetSettings.advChannelsMask)));
    expectedOptions.insert(pair<string, string> ("NO_PUBLISH_FOR_FAIL_RATE", CAST_STR(subnetSettings.noOfPublishForFailRate)));
    expectedOptions.insert(pair<string, string> ("NO_PUBLISH_FOR_FAIL_RATE_ON_BACKUP", CAST_STR(subnetSettings.noOfPublishForFailRateOnBackup)));
    expectedOptions.insert(pair<string, string> ("NO_PUBLISH_FOR_FAIL_RATE_SHORT", CAST_STR(subnetSettings.noOfPublishForFailRateShortPeriod)));
    expectedOptions.insert(pair<string, string> ("NO_PUBLISH_FOR_FAIL_RATE_ON_BACKUP_SHORT", CAST_STR(subnetSettings.noOfPublishForFailRateOnBackupShortPeriod)));
     expectedOptions.insert(pair<string, string> ("DEVICE_COMMANDS_SILENCE_INTERVAL", CAST_STR(subnetSettings.deviceCommandsSilenceInterval)));
    expectedOptions.insert(pair<string, string> ("APP_SLOT_START_OFFSET", CAST_STR(subnetSettings.appSlotsStartOffset)));
    expectedOptions.insert(pair<string, string> ("FREEZE_LEVEL_ONE_ROUTERS", subnetSettings.freezeLevelOneRouters ? "true" : "false"));
    expectedOptions.insert(pair<string, string> ("BACKUP_PING_INTERVAL", CAST_STR(subnetSettings.pingInterval)));
    expectedOptions.insert(pair<string, string> ("NR_OUTBOUND_GRAPHS_TO_EVAL", CAST_STR(subnetSettings.nrOfOutboundGraphsToBeEvaluated)));
    expectedOptions.insert(pair<string, string> ("QUEUE_PRIORITIES",  subnetSettings.getQueuePrioritiesListAsString()));
    expectedOptions.insert(pair<string, string> ("TIMESLOT_LENGTH", CAST_STR(subnetSettings.getTimeslotLength())));
    expectedOptions.insert(pair<string, string> ("MAX_NO_UDO_CONTRACTS_AT_ONE_SECOND", CAST_STR(subnetSettings.maxNoUdoContractsAtOneSecond)));

    map<string, string> options;

    // read expected options
    map<string, string>::const_iterator it = expectedOptions.begin();
    for (; it != expectedOptions.end(); ++it) {
        //GetValue(section, key, default_value); default_value = value to return if the key is not found
        const char* value = iniParser.GetValue("SYSTEM_MANAGER", (*it).first.c_str(), (*it).second.c_str());
        if (value == NULL) {
            std::ostringstream stream;
            stream << "Value for " << (*it).first
                        << " is NULL. Please define this option in the generic subnet configuration file.";
            std::cout << stream.str();
            LOG_ERROR(stream.str());
            exit(1);
        }
        string key = "SYSTEM_MANAGER." + (*it).first;
        options[key] = string(value);
    }

    //**************************
    //set values read from file

    //LOG_DEBUG("*********before" << subnetSettings);
    if(!isReloadOnSignal) {

        try {
            subnetSettings.randomizeChannelsOnStartup =(options["SYSTEM_MANAGER.RANDOMIZE_CHANNELS_ON_STARTUP"] == "true");
            LOG_INFO(fileName << "::RANDOMIZE_CHANNELS_ON_STARTUP: " << (int) subnetSettings.randomizeChannelsOnStartup);
        } catch (const std::exception& ex) {
            LOG_ERROR("In " << fileName << ", option RANDOMIZE_CHANNELS_ON_STARTUP: " << ex.what());
            exit(1);
        }

        std::vector<Uint8> channels;
        bool exceptionOccured = false;
        try {
            channels = getAsChannelsVector(options["SYSTEM_MANAGER.CHANNEL_LIST"]);
        } catch (NEException& ex) {
            LOG_INFO("Loading default channels..");
            exceptionOccured = true;
        }

        if (!exceptionOccured) {
            subnetSettings.channel_list.clear();
            for (std::vector<Uint8>::iterator itCh = channels.begin(); itCh != channels.end(); ++itCh) {
                subnetSettings.channel_list.push_back(*itCh);
            }
        }
        if(subnetSettings.randomizeChannelsOnStartup) {
            subnetSettings.randomizeChannelList();
        }


        exceptionOccured = false;
        std::vector<Uint8> channelHoppingReduced;
        std::vector<Uint8> neighborDiagChannels;
			try {
				channelHoppingReduced = getAsChannelsVector(options["SYSTEM_MANAGER.REDUCED_CHANNEL_LIST"]);
			} catch (NEException& ex) {
				LOG_INFO("Loading default reduced  channel hopping..");
				exceptionOccured = true;
			}

			if (!exceptionOccured) {
				subnetSettings.reduced_hopping.clear();
				for (std::vector<Uint8>::iterator itCh = channelHoppingReduced.begin(); itCh != channelHoppingReduced.end(); ++itCh) {
					subnetSettings.reduced_hopping.push_back(*itCh);
				}
			}

			if ( !subnetSettings.reduced_hopping.empty() ) {
				subnetSettings.createChannelListWithReducedHopping();
				subnetSettings.detectNumberOfUsedFrequencies();
			}

			exceptionOccured = false;
			try {
				neighborDiagChannels = getAsChannelsVector(options["SYSTEM_MANAGER.NEIGHBOR_DIAG_CHANNEL_LIST"]);
			} catch (NEException& ex) {
				LOG_INFO("Loading default neighborDiag");
				exceptionOccured = true;
			}

			if(exceptionOccured && !subnetSettings.reduced_hopping.empty()) {
	            LOG_ERROR("In " << fileName << ", option NEIGHBOR_DIAG_CHANNEL_LIST: " );
				exit(1);
			}

			if (!exceptionOccured) {
				subnetSettings.neigh_disc_channel_list.clear();
				for (std::vector<Uint8>::iterator itCh = neighborDiagChannels.begin(); itCh != neighborDiagChannels.end(); ++itCh) {
					subnetSettings.neigh_disc_channel_list.push_back(*itCh);
				}
			}


	        LOG_INFO(fileName << "::CHANNEL_LIST: " << subnetSettings.getChannelListAsString());
	        LOG_INFO(fileName << "::REDUCED_CHANNEL_LIST: " << subnetSettings.getReducedChannelHoppingListAsString());
	        LOG_INFO(fileName << "::NUMBER_OF_USED_FREQUENCIES: " << std::dec << (int)subnetSettings.numberOfFrequencies);
	        LOG_INFO(fileName << "::MAX_NO_UDO_CONTRACTS_AT_ONE_SECOND: " << std::dec << (int)subnetSettings.maxNoUdoContractsAtOneSecond);
	        LOG_INFO(fileName << "::NEIGHBOR_DIAG_CHANNEL_LIST: " << subnetSettings.getNeighborDiscoveryListAsString());
    }

    try {
        subnetSettings.timeToIgnoreBadRateDevice = boost::lexical_cast<Uint32>(options["SYSTEM_MANAGER.TIME_TO_IGNORE_BAD_RATE"]);
        LOG_INFO(fileName << "::TIME_TO_IGNORE_BAD_RATE: " << (int) subnetSettings.timeToIgnoreBadRateDevice);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option TIME_TO_IGNORE_BAD_RATE: " << ex.what());
        exit(1);
    }


    try {
        subnetSettings.badTransferRateThreshlod = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.BAD_TRANSFER_RATE_THRESHOLD"]);
        LOG_INFO(fileName << "::BAD_TRANSFER_RATE_THRESHOLD: " << (int) subnetSettings.badTransferRateThreshlod);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option BAD_TRANSFER_RATE_THRESHOLD: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.badTransferRateThreshlodShort = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.BAD_TRANSFER_RATE_THRESHOLD_SHORT"]);
        LOG_INFO(fileName << "::BAD_TRANSFER_RATE_THRESHOLD_SHORT: " << (int) subnetSettings.badTransferRateThreshlodShort);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option BAD_TRANSFER_RATE_THRESHOLD_SHORT: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.badTransferRateThreshlodOnBackup = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.BAD_TRANSFER_RATE_THRESHOLD_ON_BACKUP"]);
        LOG_INFO(fileName << "::BAD_TRANSFER_RATE_THRESHOLD_ON_BACKUP: " << (int) subnetSettings.badTransferRateThreshlodOnBackup);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option BAD_TRANSFER_RATE_THRESHOLD_ON_BACKUP: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.badTransferRateThreshlodOnBackupShort = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.BAD_TRANSFER_RATE_THRESHOLD_ON_BACKUP_SHORT"]);
        LOG_INFO(fileName << "::BAD_TRANSFER_RATE_THRESHOLD_ON_BACKUP_SHORT: " << (int) subnetSettings.badTransferRateThreshlodOnBackupShort);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option BAD_TRANSFER_RATE_THRESHOLD_ON_BACKUP_SHORT: " << ex.what());
        exit(1);
    }


    try {
        subnetSettings.numberOfSentPackagesForFailRateShort = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE_SHORT"]);
        LOG_INFO(fileName << "::NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE_SHORT: " << (int) subnetSettings.numberOfSentPackagesForFailRateShort);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE_SHORT: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.numberOfSentPackagesForFailRateOnBackupShort = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE_ON_BACKUP_SHORT"]);
        LOG_INFO(fileName << "::NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE_ON_BACKUP_SHORT: " << (int) subnetSettings.numberOfSentPackagesForFailRateOnBackupShort);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE_ON_BACKUP_SHORT: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.numberOfSentPackagesForFailRate = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE"]);
        LOG_INFO(fileName << "::NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE: " << (int) subnetSettings.numberOfSentPackagesForFailRate);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.numberOfSentPackagesForFailRateOnBackup = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE_ON_BACKUP"]);
        LOG_INFO(fileName << "::NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE_ON_BACKUP: " << (int) subnetSettings.numberOfSentPackagesForFailRateOnBackup);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option NUMBER_OF_SENT_PACKETS_FOR_FAIL_RATE_ON_BACKUP: " << ex.what());
        exit(1);
    }


    try {
        subnetSettings.k1factorOnEdgeCost = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.K1_FACTOR_ON_EDGE_COST"]);
        LOG_INFO(fileName << "::K1_FACTOR_ON_EDGE_COST: " << (int) subnetSettings.k1factorOnEdgeCost);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option K1_FACTOR_ON_EDGE_COST: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.join_reserved_set = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.JOIN_RESERVED_SET"]);
        LOG_INFO(fileName << "::JOIN_RESERVED_SET: " << (int) subnetSettings.join_reserved_set);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option JOIN_RESERVED_SET: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.join_adv_period = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.JOIN_ADV_PERIOD"]);
        LOG_INFO(fileName << "::JOIN_ADV_PERIOD: " << (int) subnetSettings.join_adv_period);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option JOIN_ADV_PERIOD: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.join_bbr_adv_period = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.JOIN_BBR_ADV_PERIOD"]);
        LOG_INFO(fileName << "::JOIN_BBR_ADV_PERIOD: " << (int) subnetSettings.join_bbr_adv_period);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option JOIN_BBR_ADV_PERIOD: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.join_rxtx_period = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.JOIN_LINKS_PERIOD"]);
        LOG_INFO(fileName << "::JOIN_LINKS_PERIOD: " << (int) subnetSettings.join_rxtx_period);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option JOIN_LINKS_PERIOD: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.mng_link_r_out = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MNG_LINK_R_OUT"]);
        if (!subnetSettings.mng_link_r_out) {
            LOG_WARN(fileName << "::MNG_LINK_R_OUT is : 0");
        }
        LOG_INFO(fileName << "::MNG_LINK_R_OUT: " << (int) subnetSettings.mng_link_r_out);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MNG_LINK_R_OUT: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.mng_link_r_in = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MNG_LINK_R_IN"]);
        if (!subnetSettings.mng_link_r_in) {
            LOG_WARN(fileName << "::MNG_LINK_R_IN is : 0");
        }
        LOG_INFO(fileName << "::MNG_LINK_R_IN: " << (int) subnetSettings.mng_link_r_in);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MNG_LINK_R_IN: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.mng_link_s_out = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MNG_LINK_S_OUT"]);
        if (subnetSettings.mng_link_s_out == 0) {
            LOG_WARN(fileName << "::MNG_LINK_S_OUT is : 0");
        }

        LOG_INFO(fileName << "::MNG_LINK_S_OUT: " << (int) subnetSettings.mng_link_s_out);

    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MNG_LINK_S_OUT: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.mng_link_s_in = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MNG_LINK_S_IN"]);
        if (!subnetSettings.mng_link_s_in) {
            LOG_WARN(fileName << "::MNG_LINK_S_IN is : 0");
        }
        LOG_INFO(fileName << "::MNG_LINK_S_IN: " << (int) subnetSettings.mng_link_s_in);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MNG_LINK_S_IN: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.mng_link_s_in_Star = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MNG_LINK_S_IN_STAR"]);
        if (!subnetSettings.mng_link_s_in_Star) {
            LOG_WARN(fileName << "::MNG_LINK_S_IN_STAR is : 0");
        }
        LOG_INFO(fileName << "::MNG_LINK_S_IN_STAR: " << (int) subnetSettings.mng_link_s_in_Star);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MNG_LINK_S_IN_STAR: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.mng_r_in_band = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MNG_R_IN_BAND"]);
        if (!subnetSettings.mng_r_in_band) {
            LOG_WARN(fileName << "::MNG_R_IN_BAND is : 0");
        }
        LOG_INFO(fileName << "::MNG_R_IN_BAND: " << (int) subnetSettings.mng_r_in_band);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MNG_R_IN_BAND: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.mng_s_in_band = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MNG_S_IN_BAND"]);
        if (!subnetSettings.mng_s_in_band) {
            LOG_WARN(fileName << "::MNG_S_IN_BAND is : 0");
        }
        LOG_INFO(fileName << "::MNG_S_IN_BAND: " << (int) subnetSettings.mng_s_in_band);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MNG_S_IN_BAND: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.mng_s_in_band_Star = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MNG_S_IN_BAND_STAR"]);
        if (!subnetSettings.mng_s_in_band_Star) {
            LOG_WARN(fileName << "::MNG_S_IN_BAND_STAR is : 0");
        }
        LOG_INFO(fileName << "::MNG_S_IN_BAND_STAR: " << (int) subnetSettings.mng_s_in_band_Star);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MNG_S_IN_BAND_STAR: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.mng_r_out_band = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MNG_R_OUT_BAND"]);
        if (!subnetSettings.mng_r_out_band) {
            LOG_WARN(fileName << "::MNG_R_OUT_BAND is : 0");
        }
        LOG_INFO(fileName << "::MNG_R_OUT_BAND: " << (int) subnetSettings.mng_r_out_band);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MNG_R_OUT_BAND: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.mng_s_out_band = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MNG_S_OUT_BAND"]);
        if (!subnetSettings.mng_s_out_band) {
            LOG_WARN(fileName << "::MNG_S_OUT_BAND is : 0");
        }

        LOG_INFO(fileName << "::MNG_S_OUT_BAND: " << (int) subnetSettings.mng_s_out_band);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MNG_S_OUT_BAND: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.mng_alloc_band = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MNG_ALLOC_BAND"]);
        if (!subnetSettings.mng_alloc_band) {
            LOG_WARN(fileName << "::MNG_ALLOC_BAND is : 0");
        }
        LOG_INFO(fileName << "::MNG_ALLOC_BAND: " << (int) subnetSettings.mng_alloc_band);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MNG_ALLOC_BAND: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.mng_disc_bbr = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MNG_DISC_BBR"]);
        if (!subnetSettings.mng_disc_bbr) {
            LOG_WARN(fileName << "::MNG_DISC_BBR is : 0");
        }
        LOG_INFO(fileName << "::MNG_DISC_BBR: " << (int) subnetSettings.mng_disc_bbr);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MNG_DISC_BBR: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.mng_disc_r = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MNG_DISC_R"]);
        LOG_INFO(fileName << "::MNG_DISC_R: " << (int) subnetSettings.mng_disc_r);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MNG_DISC_R: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.mng_disc_s = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MNG_DISC_S"]);
        LOG_INFO(fileName << "::MNG_DISC_S: " << (int) subnetSettings.mng_disc_s);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MNG_DISC_S: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.retry_proc = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.RETRY_PROC"]);
        if (!subnetSettings.retry_proc) {
            LOG_WARN(fileName << "::RETRY_PROC is : 0");
        }
        LOG_INFO(fileName << "::RETRY_PROC: " << (int) subnetSettings.retry_proc);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option RETRY_PROC: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.accelerationInterval = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.ACCELERATION_INTERVAL"]);
        LOG_INFO(fileName << "::ACCELERATION_INTERVAL: " << (int) subnetSettings.accelerationInterval);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option ACCELERATION_INTERVAL: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.delayActivateMultiPathForDevice = boost::lexical_cast<Uint16>(
                    options["SYSTEM_MANAGER.DELAY_ACTIVATE_MULTIPATH"]);
        LOG_INFO(fileName << "::DELAY_ACTIVATE_MULTIPATH: " << (int) subnetSettings.delayActivateMultiPathForDevice);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option DELAY_ACTIVATE_MULTIPATH: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.delayConfigurePublish = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.DELAY_CONFIGURE_PUBLISH"]);
        LOG_INFO(fileName << "::DELAY_CONFIGURE_PUBLISH: " << (int) subnetSettings.delayConfigurePublish);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option DELAY_CONFIGURE_PUBLISH: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.delayConfigureAlerts = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.DELAY_CONFIGURE_ALERTS"]);
        LOG_INFO(fileName << "::DELAY_CONFIGURE_ALERTS: " << (int) subnetSettings.delayConfigureAlerts);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option DELAY_CONFIGURE_ALERTS: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.delayConfigureNeighborDiscovery = boost::lexical_cast<Uint16>(
                    options["SYSTEM_MANAGER.DELAY_CONFIGURE_NEIGHBOR_DISCOVERY"]);
        LOG_INFO(fileName << "::DELAY_CONFIGURE_NEIGHBOR_DISCOVERY: " << (int) subnetSettings.delayConfigureNeighborDiscovery);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option DELAY_CONFIGURE_NEIGHBOR_DISCOVERY: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.delayActivateRouterRole = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.DELAY_ACTIVATE_ROUTER_ROLE"]);
        LOG_INFO(fileName << "::DELAY_ACTIVATE_ROUTER_ROLE: " << (int) subnetSettings.delayActivateRouterRole);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option DELAY_ACTIVATE_ROUTER_ROLE: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.delayPostJoinTasks = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.DELAY_POST_JOIN_TASKS"]);
        LOG_INFO(fileName << "::DELAY_POST_JOIN_TASKS: " << (int) subnetSettings.delayPostJoinTasks);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option DELAY_POST_JOIN_TASKS: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.superframeNeighborDiscoveryLength = boost::lexical_cast<Uint16>(
                    options["SYSTEM_MANAGER.SF_NEIGHBOR_DICOVERY_LENGTH"]);
        LOG_INFO(fileName << "::SF_NEIGHBOR_DICOVERY_LENGTH: " << (int) subnetSettings.superframeNeighborDiscoveryLength);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option SF_NEIGHBOR_DICOVERY_LENGTH: " << ex.what());
        exit(1);
    }

    try {
        Uint16 sfLen = boost::lexical_cast<Uint16>( options["SYSTEM_MANAGER.SF_NEIGHBOR_DICOVERY_FAST_LENGTH"]);
        if (sfLen != 0){
            subnetSettings.superframeNeighborDiscoveryFastLength = sfLen;
        }

        LOG_INFO(fileName << "::SF_NEIGHBOR_DICOVERY_FAST_LENGTH: " << (int) subnetSettings.superframeNeighborDiscoveryFastLength);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option SF_NEIGHBOR_DICOVERY_FAST_LENGTH: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.contractsMetadataThreshold = boost::lexical_cast<Uint16>(
                    options["SYSTEM_MANAGER.CONTRACTS_METADATA_THRESHOLD"]);
        LOG_INFO(fileName << "::CONTRACTS_METADATA_THRESHOLD: " << (int) subnetSettings.contractsMetadataThreshold);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option CONTRACTS_METADATA_THRESHOLD: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.neighborsMetadataThreshold = boost::lexical_cast<Uint16>(
                    options["SYSTEM_MANAGER.NEIGHBORS_METADATA_THRESHOLD"]);
        LOG_INFO(fileName << "::NEIGHBORS_METADATA_THRESHOLD: " << (int) subnetSettings.neighborsMetadataThreshold);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option NEIGHBORS_METADATA_THRESHOLD: " << ex.what());
        exit(1);
    }
    try {
        subnetSettings.superframesMetadataThreshold = boost::lexical_cast<Uint16>(
                    options["SYSTEM_MANAGER.SUPERFRAMES_METADATA_THRESHOLD"]);
        LOG_INFO(fileName << "::SUPERFRAMES_METADATA_THRESHOLD: " << (int) subnetSettings.superframesMetadataThreshold);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option SUPERFRAMES_METADATA_THRESHOLD: " << ex.what());
        exit(1);
    }
    try {
        subnetSettings.graphsMetadataThreshold = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.GRAPHS_METADATA_THRESHOLD"]);
        LOG_INFO(fileName << "::GRAPHS_METADATA_THRESHOLD: " << (int) subnetSettings.graphsMetadataThreshold);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option GRAPHS_METADATA_THRESHOLD: " << ex.what());
        exit(1);
    }
    try {
        subnetSettings.linksMetadataThreshold = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.LINKS_METADATA_THRESHOLD"]);
        LOG_INFO(fileName << "::LINKS_METADATA_THRESHOLD: " << (int) subnetSettings.linksMetadataThreshold);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option LINKS_METADATA_THRESHOLD: " << ex.what());
        exit(1);
    }
    try {
        subnetSettings.routesMetadataThreshold = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.ROUTES_METADATA_THRESHOLD"]);
        LOG_INFO(fileName << "::ROUTES_METADATA_THRESHOLD: " << (int) subnetSettings.routesMetadataThreshold);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option ROUTES_METADATA_THRESHOLD: " << ex.what());
        exit(1);
    }
    try {
        subnetSettings.diagsMetadataThreshold = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.DIAGS_METADATA_THRESHOLD"]);
        LOG_INFO(fileName << "::DIAGS_METADATA_THRESHOLD: " << (int) subnetSettings.diagsMetadataThreshold);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option DIAGS_METADATA_THRESHOLD: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.nrRoutersPerBBR = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.NR_ROUTERS_PER_BBR"]);
        LOG_INFO(fileName << "::NR_ROUTERS_PER_BBR: " << (int) subnetSettings.nrRoutersPerBBR);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option NR_ROUTERS_PER_BBR: " << ex.what());
        exit(1);
    }
    try {
        subnetSettings.nrNonRoutersPerBBR = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.NR_NON_ROUTERS_PER_BBR"]);
        LOG_INFO(fileName << "::NR_NON_ROUTERS_PER_BBR: " << (int) subnetSettings.nrNonRoutersPerBBR);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option NR_NON_ROUTERS_PER_BBR: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.enableStarTopology = (options["SYSTEM_MANAGER.ENABLE_STAR_TOPOLOGY"] == "true");
        LOG_INFO(fileName << "::ENABLE_STAR_TOPOLOGY: " << (int) subnetSettings.enableStarTopology);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option ENABLE_STAR_TOPOLOGY: valid values are false/true.. " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.enableStarTopologyRouters = (options["SYSTEM_MANAGER.ENABLE_STAR_TOPOLOGY_ROUTERS"] == "true");
        LOG_INFO(fileName << "::ENABLE_STAR_TOPOLOGY_ROUTERS: " << (int) subnetSettings.enableStarTopologyRouters);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option ENABLE_STAR_TOPOLOGY_ROUTERS: valid values are false/true.. " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.nrRoutersPerBBRinStar = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.NR_ROUTERS_PER_BBR_STAR"]);
        LOG_INFO(fileName << "::NR_ROUTERS_PER_BBR_STAR: " << (int) subnetSettings.nrRoutersPerBBRinStar);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option NR_ROUTERS_PER_BBR_STAR: " << ex.what());
        exit(1);
    }
    try {
        subnetSettings.nrNonRoutersPerBBRinStar = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.NR_NON_ROUTERS_PER_BBR_STAR"]);
        LOG_INFO(fileName << "::NR_NON_ROUTERS_PER_BBR_STAR: " << (int) subnetSettings.nrNonRoutersPerBBRinStar);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option NR_NON_ROUTERS_PER_BBR_STAR: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.nrRoutersPerRouter = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.NR_ROUTERS_PER_ROUTER"]);
        LOG_INFO(fileName << "::NR_ROUTERS_PER_ROUTER: " << (int) subnetSettings.nrRoutersPerRouter);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option NR_ROUTERS_PER_ROUTER: " << ex.what());
        exit(1);
    }
    try {
        subnetSettings.nrNonRoutersPerRouter = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.NR_NON_ROUTERS_PER_ROUTER"]);
        LOG_INFO(fileName << "::NR_NON_ROUTERS_PER_ROUTER: " << (int) subnetSettings.nrNonRoutersPerRouter);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option NR_NON_ROUTERS_PER_ROUTER: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.maxNumberOfLayers = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.MAX_NUMBER_OF_LAYERS"]);
        LOG_INFO(fileName << "::MAX_NUMBER_OF_LAYERS: " << (int) subnetSettings.maxNumberOfLayers);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option MAX_NUMBER_OF_LAYERS: " << ex.what());
        exit(1);
    }

    std::vector<Uint8> slotsNeighborDiscovery;
    bool exceptionOccuredOnRead = false;

    try {
        slotsNeighborDiscovery = getAsVector(options["SYSTEM_MANAGER.SLOTS_NEIGHBOR_DISCOVERY"]);
    } catch (NEException& ex) {
        LOG_INFO("Loading SLOTS_NEIGHBOR_DISCOVERY..not found");
        exceptionOccuredOnRead = true;
    }
    std::ostringstream slotsStream;

    if (exceptionOccuredOnRead) {
        for (int i = 0; i < 4; ++i) {
            slotsStream << (int) subnetSettings.slotsNeighborDiscovery[i] << " ";
        }
    } else {
        int i = 0;
        for (std::vector<Uint8>::iterator itSlots = slotsNeighborDiscovery.begin(); itSlots != slotsNeighborDiscovery.end(); ++itSlots) {
            subnetSettings.slotsNeighborDiscovery[i] = *itSlots;
            slotsStream << (int) subnetSettings.slotsNeighborDiscovery[i] << " ";
            ++i;
        }
    }
    LOG_INFO(fileName << "::SLOTS_NEIGHBOR_DISCOVERY: " << slotsStream.str());

    try {
        subnetSettings.enableAlertsForGateway = (options["SYSTEM_MANAGER.ENABLE_ALERTS_FOR_GATEWAY"] == "true");
        LOG_INFO(fileName << "::ENABLE_ALERTS_FOR_GATEWAY: " << (int) subnetSettings.enableAlertsForGateway);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option ENABLE_ALERTS_FOR_GATEWAY: valid values are false, true.. " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.alertReceivingObjectID = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.ALERT_RECEIVING_OBJECT_ID"]);
        LOG_INFO(fileName << "::ALERT_RECEIVING_OBJECT_ID: " << (int) subnetSettings.alertReceivingObjectID);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option ALERT_RECEIVING_OBJECT_ID: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.alertReceivingPort = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.ALERT_RECEIVING_PORT"]);
        LOG_INFO(fileName << "::ALERT_RECEIVING_PORT: " << (int) subnetSettings.alertReceivingPort);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option ALERT_RECEIVING_PORT: " << ex.what());
        exit(1);
    }

    try {
        subnetSettings.alertTimeout = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.ALERT_TIMEOUT"]);
        LOG_INFO(fileName << "::ALERT_TIMEOUT: " << (int) subnetSettings.alertTimeout);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option ALERT_TIMEOUT: " << ex.what());
        exit(1);
    }

    READ_OPTION_BOOLEAN(subnetSettings.enableMultiPath, "SYSTEM_MANAGER.ENABLE_MULTI_PATH");

    if (!isReloadOnSignal){
        try {
            Uint16 usedBirth = boost::lexical_cast<Uint16>(options["SYSTEM_MANAGER.SUPERFRAME_BIRTH"]);
            Uint16 configuredBirth = usedBirth;

            bool generateSuperframeBirth = true;
            while (generateSuperframeBirth) {
                if (usedBirth >= MAX_SUPERFRAME_BIRTH) {

                    int random = rand();
                    int mod = (random % (MAX_SUPERFRAME_BIRTH + 1));
                    usedBirth = mod & 0xFFFC;//force it to be 4 multiple to mach the advertise channels
                }

                std::vector<Uint16>::iterator itSuperframesBirths = std::find(subnetsSuperframesBirths.begin(), subnetsSuperframesBirths.end(), usedBirth);
                if (itSuperframesBirths == subnetsSuperframesBirths.end()) {
                    generateSuperframeBirth = false;
                } else {
                    LOG_INFO("Generate new random for superframe birth; current usedBirth(" << (int)usedBirth << ") already exists");
                    usedBirth = MAX_SUPERFRAME_BIRTH + 2;
                }
            }

            if (!isDefaultSubnetLoad){//add the generated birth only when loading for a subnet (not for default file).
                subnetsSuperframesBirths.push_back(usedBirth);
            }
            subnetSettings.superframeBirth = usedBirth;
            LOG_INFO(fileName << "::SUPERFRAME_BIRTH: Used:" << (int) subnetSettings.superframeBirth << " Config: " << (int)configuredBirth);
        } catch (const std::exception& ex) {
            LOG_ERROR("In " << fileName << ", option SUPERFRAME_BIRTH: " << ex.what());
            exit(1);
        }
    }

    READ_OPTION_NUMBER(subnetSettings.advChannelsMask, "SYSTEM_MANAGER.ADV_CHANNELS_MASK", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(subnetSettings.noOfPublishForFailRate, "SYSTEM_MANAGER.NO_PUBLISH_FOR_FAIL_RATE", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(subnetSettings.noOfPublishForFailRateOnBackup, "SYSTEM_MANAGER.NO_PUBLISH_FOR_FAIL_RATE_ON_BACKUP", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(subnetSettings.noOfPublishForFailRateShortPeriod, "SYSTEM_MANAGER.NO_PUBLISH_FOR_FAIL_RATE_SHORT", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(subnetSettings.noOfPublishForFailRateOnBackupShortPeriod, "SYSTEM_MANAGER.NO_PUBLISH_FOR_FAIL_RATE_ON_BACKUP_SHORT", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(subnetSettings.deviceCommandsSilenceInterval, "SYSTEM_MANAGER.DEVICE_COMMANDS_SILENCE_INTERVAL", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(subnetSettings.appSlotsStartOffset, "SYSTEM_MANAGER.APP_SLOT_START_OFFSET", Uint16, "unsigned integer");


    try {
        subnetSettings.freezeLevelOneRouters = (options["SYSTEM_MANAGER.FREEZE_LEVEL_ONE_ROUTERS"] == "true");
        LOG_INFO(fileName << "::FREEZE_LEVEL_ONE_ROUTERS: " << (int) subnetSettings.freezeLevelOneRouters);
    } catch (const std::exception& ex) {
        LOG_ERROR("In " << fileName << ", option FREEZE_LEVEL_ONE_ROUTERS: valid values are false/true.. " << ex.what());
        exit(1);
    }

    READ_OPTION_NUMBER(subnetSettings.pingInterval, "SYSTEM_MANAGER.BACKUP_PING_INTERVAL", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(subnetSettings.nrOfOutboundGraphsToBeEvaluated, "SYSTEM_MANAGER.NR_OUTBOUND_GRAPHS_TO_EVAL", Uint16, "unsigned integer");
    READ_OPTION_NUMBER(subnetSettings.maxNoUdoContractsAtOneSecond, "SYSTEM_MANAGER.MAX_NO_UDO_CONTRACTS_AT_ONE_SECOND", Uint16, "unsigned integer");

    std::vector<Uint16> queuePriorities;;
    bool exceptionOccured = false;
    try {
    	queuePriorities = getAsQueuePrioritiesVector(options["SYSTEM_MANAGER.QUEUE_PRIORITIES"]);
    } catch (NEException& ex) {
        LOG_INFO("Loading default queue priorities..");
        exceptionOccured = true;
    }

    if (!exceptionOccured) {
    	if (!queuePriorities.empty()) {
			subnetSettings.queuePriorities.clear();
			for (std::vector<Uint16>::iterator itPriority = queuePriorities.begin(); itPriority != queuePriorities.end(); ++itPriority) {
				subnetSettings.queuePriorities.push_back(*itPriority);
			}
    	}
    }

    LOG_INFO(fileName << "::QUEUE_PRIORITIES: " << subnetSettings.getQueuePrioritiesListAsString());

    Uint16 timeSlotLength = 0;
    READ_OPTION_NUMBER(timeSlotLength, "SYSTEM_MANAGER.TIMESLOT_LENGTH", Uint16, "unsigned integer");
    subnetSettings.setTimeslotLength(timeSlotLength);

    //LOG_DEBUG("*********after" << subnetSettings);

    //ATTENTION: this reinitialisations have to be done after all values are initialized from file
    if (subnetSettings.enableStarTopology) {
        subnetSettings.nrRoutersPerBBR = subnetSettings.nrRoutersPerBBRinStar;
        subnetSettings.nrNonRoutersPerBBR = subnetSettings.nrNonRoutersPerBBRinStar;
        subnetSettings.accelerationInterval = 0;
        subnetSettings.mng_link_s_in = subnetSettings.mng_link_s_in_Star;
        subnetSettings.mng_s_in_band = subnetSettings.mng_s_in_band_Star;
        subnetSettings.maxNoUdoContractsAtOneSecond = 9;
    }

    //ATTENTION: this reinitialisations have to be done after all values are initialized from file
    if (subnetSettings.enableStarTopologyRouters) {
        subnetSettings.nrRoutersPerBBR = subnetSettings.nrRoutersPerBBRinStarRouters;
        subnetSettings.nrNonRoutersPerBBR = subnetSettings.nrNonRoutersPerBBRinStarRouters;
        subnetSettings.accelerationInterval = 0;
        subnetSettings.mng_link_s_in = subnetSettings.mng_link_s_in_Star;
        subnetSettings.mng_s_in_band = subnetSettings.mng_s_in_band_Star;
        subnetSettings.maxNoUdoContractsAtOneSecond = 6;
    }
}

std::vector<Uint8> SmSettingsLogic::getAsVector(const std::string& text) {
    // split the input line into tokens using the comma separator
    tokenizer tokens(text, boost::char_separator<char>(","));
    std::vector<Uint8> vector;

    for (tokenizer::const_iterator iter = tokens.begin(); iter != tokens.end(); ++iter) {
        Uint8 octet;
        try {
            string value = (*iter);
            trimSpaces(value);
            octet = (Uint8) boost::lexical_cast<int>(value);
        } catch (boost::bad_lexical_cast & ex) {
            std::ostringstream stream;
            stream << "The string of values has invalid format... " << ex.what();
            throw NEException(stream.str());
        }

        vector.push_back(octet);
    }

    return vector;
}

/**
 * Parse channelsText and returns its content as a vector of Uint8. The channelsText must be in the following format:
 * ch1,ch2,ch3,...,chn. Each channel (ch1..chn) must be in the range: 0-15 inclusive.
 * Ex: 8,11,15,2
 */
std::vector<Uint8> SmSettingsLogic::getAsChannelsVector(const std::string& channelsText) {
    // split the input line into tokens using the comma separator
    tokenizer tokens(channelsText, boost::char_separator<char>(","));
    std::vector<Uint8> channelsVector;

    for (tokenizer::const_iterator iter = tokens.begin(); iter != tokens.end(); ++iter) {
        Uint8 channel;
        try {
            string value = (*iter);
            trimSpaces(value);
            channel = (Uint8) boost::lexical_cast<int>(value);
        } catch (boost::bad_lexical_cast & ex) {
            std::ostringstream stream;
            stream << "Initial channels list contains values outside the range 0-15: " << ex.what();
            throw NEException(stream.str());
        }
        if (channel > 15) {
            throw NEException("Initial channels list contains values outside the range 0-15.");
        }
        channelsVector.push_back(channel);
    }

    return channelsVector;
}


/**
 * Parse QueuePrioritiesText and returns its content as a vector of Uint16. The QueuePrioritiesText must be in the following format:
 * Priority1:QMax1,Priority:QMax2,Priority3:QMax3,...,PriorityN:QMaxN. Each  (Priority1..Priority1) must be in the range: 0-15 inclusive.
 * Priority shall be enumerated in increasing order, so that Priority X shall be less than Priority X+1 . Similarly, QMax X shall be less than QMax X+1 .
 *
 * Ex: 1:1,2:2
 */
std::vector<Uint16> SmSettingsLogic::getAsQueuePrioritiesVector(const std::string& queuePrioritiesText) {
    // split the input line into tokens using the comma separator
    tokenizer tokens(queuePrioritiesText, boost::char_separator<char>(","));
    std::vector<Uint16> queuePriorities;

    for (tokenizer::const_iterator iter = tokens.begin(); iter != tokens.end(); ++iter) {
    	Uint16 priorityElem;
        Uint8 priorityNo;
        Uint8 queueMax;
        try {
            string value = (*iter);
			trimSpaces(value);
			if ( value == "0") {
				return queuePriorities;
			}
            tokenizer tokensPriority(value, boost::char_separator<char>(":"));
        	tokenizer::const_iterator iterPriority = tokensPriority.begin();
        	try {
				if(iterPriority != tokensPriority.end()) {
					string valuePriority = (*iterPriority);
					trimSpaces(valuePriority);
					priorityNo = (Uint8) boost::lexical_cast<int>(valuePriority);
					++iterPriority;
					if(iterPriority != tokensPriority.end()) {
						valuePriority = (*iterPriority);
						trimSpaces(valuePriority);
						queueMax = (Uint8) boost::lexical_cast<int>(valuePriority);
					}
				}
        	} catch (boost::bad_lexical_cast & ex) {
                std::ostringstream stream;
                stream << "Initial priorities list contains values outside the range 0-15, or not separated by ':'" << ex.what();
                throw NEException(stream.str());
        	}


        } catch (boost::bad_lexical_cast & ex) {
            std::ostringstream stream;
            stream << "Initial priorities list contains values outside the range 0-15: " << ex.what();
            throw NEException(stream.str());
        }
        if ( priorityNo > 15) {
            throw NEException("Initial priorities list contains values outside the range 0-15.");
        }
        priorityElem = priorityNo;
        priorityElem = (priorityElem << 8) + queueMax;
        queuePriorities.push_back(priorityElem);
    }

    return queuePriorities;
}

void SmSettingsLogic::trimSpaces(string& str) {
    // Trim Both leading and trailing spaces
    size_t startpos = str.find_first_not_of(" \t"); // Find the first character position after excluding leading blank spaces
    size_t endpos = str.find_last_not_of(" \t"); // Find the first character position from reverse af

    // if all spaces or empty return an empty string
    if ((string::npos == startpos) || (string::npos == endpos)) {
        str = "";
    } else {
        str = str.substr(startpos, endpos - startpos + 1);
    }
}

void SmSettingsLogic::updateProvisioningByDeviceType(SimpleIni::CSimpleIniCaseA::TNamesDepend iniItems, IniFileTag deviceTypeTag) {
    SimpleIni::CSimpleIniCaseA::TNamesDepend::const_iterator it = iniItems.begin();
    for (; it != iniItems.end(); ++it) {
        // modified: florin.muresan, 09/25/2009
        Address64 ipFirst, ipLast;

        NE::Model::SecurityKey key;
        Uint16 subnetMask;
        Uint16 deviceRole = 0;
        try {
            ParseProvisioningLine((*it).pItem, ipFirst, ipLast, key, subnetMask, deviceRole);
        } catch (const NE::Common::NEException& ex) {
            LOG_ERROR("In " << customIniFileName << ", option SECURITY_MANAGER.DEVICE, item no " << (*it).nOrder << ": "
                        << ex.what());
            exit(1);
        }

        ProvisioningItem::DeviceType deviceType = ProvisioningItem::NOT_SET;

        switch (deviceTypeTag) {
            case DEVICE:
                if (deviceRole == 1) {
                    deviceType = ProvisioningItem::IN_OUT;
                } else if (deviceRole == 2) {
                    deviceType = ProvisioningItem::ROUTER;
                } else if (deviceRole == 3) {
                    deviceType = ProvisioningItem::ROUTER_IO;
                } else {//if ((deviceRole > 3) || (deviceRole == 0)) {
                    deviceType = ProvisioningItem::NOT_SET;
                }
            break;

            case MANAGER:
                deviceType = ProvisioningItem::MANAGER;
            break;

            case GATEWAY:
                deviceType = ProvisioningItem::GATEWAY;
            break;

            case BACKBONE:
                deviceType = ProvisioningItem::BACKBONE;
            break;

            default:
                deviceType = ProvisioningItem::NOT_SET;
            break;

        }

        provisioningKeys.push_back(ProvisioningItem(key, subnetMask, deviceType, ipFirst, ipLast));
        LOG_DEBUG("Added provisioning " << ipFirst.toString() << " - " << ipLast.toString() << " , " << key.toString() << " , "
                    << subnetMask);

        //add only the subnets of backbones
        if (deviceTypeTag == BACKBONE) {
            std::vector<Uint16>::iterator it = std::find(subnetIds.begin(), subnetIds.end(), subnetMask);
            if (it == subnetIds.end()) {
                subnetIds.push_back(subnetMask);
            }
        }
    }

}

void SmSettingsLogic::parseProvisioningFile(SimpleIni::CSimpleIniCaseA& iniParser) {
    SimpleIni::CSimpleIniCaseA::TNamesDepend devices;
    SimpleIni::CSimpleIniCaseA::TNamesDepend gateways;
    SimpleIni::CSimpleIniCaseA::TNamesDepend backbones;
    iniParser.GetAllValues("SECURITY_MANAGER", "BACKBONE", backbones);
    iniParser.GetAllValues("SECURITY_MANAGER", "GATEWAY", gateways);
    iniParser.GetAllValues("SECURITY_MANAGER", "DEVICE", devices);
    updateProvisioningByDeviceType(gateways, GATEWAY);
    updateProvisioningByDeviceType(backbones, BACKBONE);
    updateProvisioningByDeviceType(devices, DEVICE);
}

void SmSettingsLogic::ParseProvisioningLine(const string& p_line, NE::Common::Address64& p_ip64, NE::Model::SecurityKey& p_key,
            Uint16& p_subnetMask, Uint16 &p_deviceRole) {
    // split the input line into tokens using the comma separator
    tokenizer tokens(p_line, boost::char_separator<char>(","));

    Uint8 tokensNo = distance(tokens.begin(), tokens.end());

    if (tokensNo < 3) {
        throw NE::Common::NEException("missing elements: need comma separated 64 bit address, security key, subnet mask");
    }

    tokenizer::const_iterator tok_iter = tokens.begin();
    // read 64 bit address
    try {
        p_ip64.loadString(*tok_iter);
    } catch (const InvalidArgumentException& ex) {
        throw NEException(string("invalid 64 bit address: ") + ex.what());
    }
    // read the security key
    try {
        p_key.loadString((*++tok_iter).c_str());
    } catch (const InvalidArgumentException& ex) {
        throw NEException(string("security key: ") + ex.what());
    }
    // read the subnet mask
    try {
        p_subnetMask = boost::lexical_cast<Uint16>(*++tok_iter);
    } catch (boost::bad_lexical_cast &) {
        throw NE::Common::NEException("invalid subnet mask value");
    }

    if (tokensNo > 3) {
        try {
            p_deviceRole = boost::lexical_cast<Uint16>(*++tok_iter);
        } catch (boost::bad_lexical_cast &) {
            throw NE::Common::NEException("invalid device role");
        }
    }
}

void SmSettingsLogic::ParseProvisioningLine(const string& p_line, NE::Common::Address64& p_ipFirst,
            NE::Common::Address64& p_ipLast, NE::Model::SecurityKey& p_key, Uint16& p_subnetMask, Uint16 &p_deviceRole) {
    // split the input line into tokens using the comma separator
    tokenizer tokens(p_line, boost::char_separator<char>(","));
    Uint8 tokensNo = distance(tokens.begin(), tokens.end());
    if (tokensNo < 3) {
        throw NE::Common::NEException("missing elements: need comma separated <64_bit_address,security_key,subnet_id>");
    }
    tokenizer::const_iterator tok_iter = tokens.begin();
    // read the address (ex. 6302:0304:0506:0701) or address range (ex. 6302:0304:0506:0701 - 6302:0304:0506:0709)
    try {

        tokenizer ipTokens(*tok_iter, boost::char_separator<char>(" - ", "-"));
        tokenizer::const_iterator ipIter = ipTokens.begin();
        if (distance(ipTokens.begin(), ipTokens.end()) < 1) {
            throw NE::Common::NEException("missing elements: need 64 bit address");
        } else {
            p_ipFirst.loadString(*ipIter);
            if (distance(ipTokens.begin(), ipTokens.end()) > 2) {
                throw NE::Common::NEException("too many values: invalid address range");
            } else {
                if (distance(ipTokens.begin(), ipTokens.end()) < 2) {
                    p_ipLast.loadString(*ipIter);
                } else {
                    p_ipLast.loadString(*++ipIter);
                }
            }
        }
    } catch (const InvalidArgumentException& ex) {
        throw NEException(string("invalid 64 bit address: ") + ex.what());
    }
    // read the security key
    try {
        p_key.loadString((*++tok_iter).c_str());
    } catch (const InvalidArgumentException& ex) {
        throw NEException(string("security key: ") + ex.what());
    }
    // read the subnet mask
    try {
        p_subnetMask = boost::lexical_cast<Uint16>(*++tok_iter);
    } catch (boost::bad_lexical_cast &) {
        throw NE::Common::NEException("invalid subnet mask value");
    }

    if (p_subnetMask == 0){
        throw NE::Common::NEException("SubnetID 0 is not allowed provisioning file.");
    }

    if (tokensNo > 3) {
        try {
            p_deviceRole = boost::lexical_cast<Uint16>(*++tok_iter);
        } catch (boost::bad_lexical_cast &) {
            throw NE::Common::NEException("invalid device role");
        }
    }
}

void SmSettingsLogic::ParseAddrPortLine(const string& IPv6, const string& IPv4, const string& Port, Address128& p_ipv6,
            Isa100::Common::UdpEndpoint& ipv4Enpoint) {
    // split the input line into tokens using the comma separator

    unsigned long ipv4;
    struct in_addr ipv4Address;

    if (IPv4 == "0.0.0.0") {
        ipv4 = CWrapSocket::GetLocalIp();
        ipv4Address.s_addr = ipv4;
        ipv4Enpoint.ipv4 = ::inet_ntoa(ipv4Address);
    } else {
        ipv4 = ::inet_addr(IPv4.c_str());
        // read the IPv4 address .......inet_ntoa(a)
        ipv4Enpoint.ipv4 = IPv4;
    }

    if (ipv4 == INADDR_NONE) {
        LOG_ERROR("ERROR configured IPv4 " << IPv4 << " " << ipv4 << " invalid.");
        THROW_EX(NE::Common::NEException, "ERROR configured IPv4 " << IPv4 << " "<< ipv4 << " invalid.")
        ;
    }

    // read the port
    try {
        ipv4Enpoint.port = boost::lexical_cast<int>(Port);
    } catch (boost::bad_lexical_cast &) {
        throw NE::Common::NEException("invalid port value");
    }

    // read the IPv6 address
    p_ipv6.loadString(IPv6);

    // fill in blanks in IPv6


    //TODO: FIX this - on x64 architectures, sizeof(long) == 8, so stuff gets overwritten!!!
    memcpy(p_ipv6.value + 12, &ipv4, sizeof(ipv4));

    unsigned short usPort = htons((unsigned short) ipv4Enpoint.port);
    memcpy(p_ipv6.value + 10, &usPort, sizeof(usPort));
}

SmSettingsLogic& SmSettingsLogic::instance() {
    static SmSettingsLogic instance;
    return instance;
}

void SmSettingsLogic::load() {
    provisioningKeys.clear();
    // backbones.clear();

    loadConfigIni();
    loadProvisioningIni();
    loadSubnetDefaultIni(false);//false = is not a reload
    loadSubnetCustomIni(false);//false = is not a reload
}

void SmSettingsLogic::reload() {
    provisioningKeys.clear();
    loadConfigIni();
    loadSubnetDefaultIni(true);//true = is a reload
    loadSubnetCustomIni(true);//true = is a reload

    loadProvisioningIni();
    //create subnet instance in model for the new added subnets
    NE::Model::SubnetsContainer& subnetsContainer = Isa100::Model::EngineProvider::getEngine()->getSubnetsContainer();
    NE::Model::SubnetsMap& existingSubnets = subnetsContainer.getSubnetsList();
    for (std::vector<Uint16>::iterator itSub = subnetIds.begin(); itSub != subnetIds.end(); ++itSub) {
        NE::Model::SubnetsMap::iterator itFind = existingSubnets.find(*itSub);
        if (itFind == existingSubnets.end()) {
            NE::Model::Subnet::PTR newSubnet(new Subnet(*itSub, subnetsContainer.manager, this));
            Operations::OperationsProcessor& operationsProcessor =
                        Isa100::Model::EngineProvider::getEngine()->getOperationsProcessor();
            newSubnet->registerDeleteDeviceCallback(&operationsProcessor);
            subnetsContainer.addSubnet(newSubnet);
        }
    }
}

void SmSettingsLogic::loadConfigIni() {
    SimpleIni::CSimpleIniCaseA configIniParser;
    configIniParser.SetMultiKey(); //[andy] - allow multiple keys with the same name

    if (commonIniFileName.size()> 0) {
        if (configIniParser.LoadFile(commonIniFileName.c_str()) != SimpleIni::SI_OK) {
            LOG_ERROR("Could not open configuration file " << commonIniFileName);
            exit(1);
        }
    } else if (configIniParser.LoadFile(SM_CONFIG_INI_FILE) == SimpleIni::SI_OK) {
        commonIniFileName = SM_CONFIG_INI_FILE;
    }
    else if (configIniParser.LoadFile(SM_CONFIG_INI_FILE_NAME) == SimpleIni::SI_OK) {
        commonIniFileName = SM_CONFIG_INI_FILE_NAME;
    }
    else {
        LOG_ERROR("Could not open configuration file " SM_CONFIG_INI_FILE_NAME);
        exit(1);
    }

    ParseCommonConfigFile(configIniParser);
}

void SmSettingsLogic::loadProvisioningIni() {
    SimpleIni::CSimpleIniCaseA provisioningIniParser(false, true);

    if (provisioningIniParser.LoadFile(SM_PROVISIONING_INI_FILE) == SimpleIni::SI_OK) {
        customIniFileName = SM_PROVISIONING_INI_FILE;
    }
    else if (provisioningIniParser.LoadFile(SM_PROVISIONING_INI_FILE_NAME) == SimpleIni::SI_OK) {
        customIniFileName = SM_PROVISIONING_INI_FILE_NAME;
    }
    else {
        LOG_ERROR("Could not open configuration file " SM_PROVISIONING_INI_FILE_NAME);
        exit(1);
    }

    parseProvisioningFile(provisioningIniParser);
}

void SmSettingsLogic::loadSubnetDefaultIni(bool isReloadOnSignal) {
    SimpleIni::CSimpleIniCaseA subnetDefaultIniParser;
    std::string subnetConfigFile = NIVIS_PROFILE + subnetConfigFileName;
    if (subnetDefaultIniParser.LoadFile(subnetConfigFile.c_str()) != SimpleIni::SI_OK) {
        if (subnetDefaultIniParser.LoadFile(subnetConfigFileName.c_str()) != SimpleIni::SI_OK) {
            LOG_INFO("No default subnet settings file provided: " << subnetConfigFileName << ", internal settings will be used");
        } else {
            LOG_INFO("Loaded default subnet settings file: " << subnetConfigFileName);
        }
    }

    ParseSubnetConfigFile(subnetDefaultIniParser, getSubnetSettingsDefault(), subnetConfigFile, isReloadOnSignal, true);//called with subnetID 0

    //load updates in all SubnetSettings instances; if there are custom values they will be updated on corresponding file parsing
    SubnetSettings& defaultSubnetSettings = getSubnetSettingsDefault();

    for (SubnetSettingsMap::iterator itSubnets = subnetSettingsMap.begin(); itSubnets != subnetSettingsMap.end(); ++itSubnets) {
        itSubnets->second.join_adv_period = defaultSubnetSettings.join_adv_period;
        itSubnets->second.join_rxtx_period = defaultSubnetSettings.join_rxtx_period;
    }
}

void SmSettingsLogic::loadSubnetCustomIni(bool isReloadOnSignal) {
    //SimpleIni::CSimpleIniCaseA subnetCustomIniParser;

    std::string subnetCustomFileName, subnetCustomFile;

    for (vector<Uint16>::iterator subnetID = subnetIds.begin(); subnetID != subnetIds.end(); ++subnetID) {
        SimpleIni::CSimpleIniCaseA subnetCustomIniParser; //have a separate parser for each subnet
        subnetCustomFileName = "sm_subnet_" + boost::lexical_cast<std::string>(*subnetID) + ".ini";
        subnetCustomFile = NIVIS_PROFILE + subnetCustomFileName;
        if (subnetCustomIniParser.LoadFile(subnetCustomFile.c_str()) != SimpleIni::SI_OK) {
            if (subnetCustomIniParser.LoadFile(subnetCustomFileName.c_str()) != SimpleIni::SI_OK) {
                LOG_INFO("No custom subnet settings file provided: " << (int) *subnetID << " (" << subnetCustomFileName << "). "
                            << "Default options will be used.");
//                continue;
            } else {
                LOG_INFO("Loaded custom settings settings file: " << (int) *subnetID << " (" << subnetCustomFileName << "). ");
            }
        }
        // initialize object with default values
        SubnetSettings subnetSettings(getSubnetSettings(*subnetID));

        ParseSubnetConfigFile(subnetCustomIniParser, subnetSettings, subnetCustomFileName, isReloadOnSignal, false);

        addCustomSubnetSettings(*subnetID, subnetSettings);

        for (SubnetSettingsMap::iterator it = subnetSettingsMap.begin(); it != subnetSettingsMap.end(); ++it) {
            LOG_INFO("Subnet " << (int) it->first << " : " << it->second);
        }


    }
}

ProvisioningItem* SmSettingsLogic::getProvisioningForDevice(const Address64& address) {
    SecurityProvisioning::iterator itProvisioning;

    for (itProvisioning = provisioningKeys.begin(); itProvisioning != provisioningKeys.end(); ++itProvisioning) {
        //        LOG_DEBUG("ipFirst=" << itProvisioning->ipFirst.toString()
        //                    << ", ipLast=" << itProvisioning->ipLast.toString()
        //                    << ", address=" << address.toString());
        if (itProvisioning->ipFirst == address || itProvisioning->ipLast == address || (itProvisioning->ipFirst < address
                    && address < itProvisioning->ipLast)) {
            return &(*itProvisioning);
        }
    }
    return NULL;
}

} // namespace Common
} // namespace Isa100
