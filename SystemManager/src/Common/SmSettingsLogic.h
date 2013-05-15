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
#ifndef SMSETTINGSLOGIC_H_
#define SMSETTINGSLOGIC_H_

#include "Common/Address128.h"
#include "Common/NEAddress.h"
#include "Common/NETypes.h"
#include "Common/SettingsLogic.h"
#include "Common/logging.h"
#include "Common/simpleini/SimpleIni.h"
#include "Model/SecurityKey.h"
#include "../../Shared/AnPaths.h"

namespace Isa100 {
namespace Common {

#define SM_CONFIG_INI_FILE_NAME	           "config.ini"
#define SM_CONFIG_INI_FILE			       NIVIS_PROFILE SM_CONFIG_INI_FILE_NAME
#define SM_PROVISIONING_INI_FILE_NAME	   "system_manager.ini"
#define SM_PROVISIONING_INI_FILE		   NIVIS_PROFILE SM_PROVISIONING_INI_FILE_NAME

enum IniFileTag {
    BACKBONE = 4,
    GATEWAY = 8,
    MANAGER = 16,
    DEVICE = 0

};

struct ProvisioningItem {



        enum DeviceType {//the values should be the same as from the Capabilities role types.
            NOT_SET = 0,
            IN_OUT = 1, // if device has IO role (has a sensor attached to it)
            ROUTER = 2,
            ROUTER_IO = 3,
            BACKBONE = 4,
            GATEWAY = 8,
            MANAGER = 16,
            SECURITY_MANAGER = 32,
            TIME_SOURCE = 64,
            PROVISIONING = 128

       };

        NE::Model::SecurityKey key;

        Uint16 subnetId;

        DeviceType deviceType;

        /*
         * Keeps the first ip address for a range of ip's that have the same provisioning information.
         */
        NE::Common::Address64 ipFirst;

        /*
         * Keeps the last ip address for a range of ip's that have the same provisioning information.
         */
        NE::Common::Address64 ipLast;

        /*
         * Kept for a backward compatibility.
         */
        ProvisioningItem(NE::Model::SecurityKey key_, Uint16 subnetId_, DeviceType deviceType_) :
            key(key_), subnetId(subnetId_), deviceType(deviceType_) {
        }

        ProvisioningItem(NE::Model::SecurityKey key_, Uint16 subnetId_, DeviceType deviceType_, Address64 ipFirst_,
                    Address64 ipLast_) :
            key(key_), subnetId(subnetId_), deviceType(deviceType_), ipFirst(ipFirst_), ipLast(ipLast_) {
        }

        ProvisioningItem(const ProvisioningItem& other) {
            key = other.key;
            subnetId = other.subnetId;
            deviceType = other.deviceType;
            ipFirst = other.ipFirst;
            ipLast = other.ipLast;
         }

        bool isGateway() {
            return deviceType == GATEWAY;
        }

        bool isBackbone() {
            return deviceType == BACKBONE;
        }

};

struct UdpEndpoint {
        std::string ipv4;
        Uint16 port;
};

class SmSettingsLogic: public NE::Common::SettingsLogic {

    public:

        /**
         * Represents the addresses of the backbones and the subnet ids.
         */
        // std::map<Uint16, std::vector<Address64> > backbones;

        std::string subnetConfigFileName;

        /**
         * The interval in seconds for the logging of the network state.
         */
        Uint16 logNetworkStateInterval;

        /**
         * The default life time of object(in seconds).
         */
        Uint16 objectLifeTime;

        /**
         * Max retry to send a command with success
         */
        Uint32 maxRetrySendCommand;

        /**
         * The name of the common ini file (default config.ini).
         */
        string commonIniFileName;

        string logConfFileName;
        //static string commonIniFileName;

        /**
         * The UDP server will listen on this port.
         */
        unsigned int listenPort;

        /**
         * The UDP server will listen on this port(UDPTestClient port).
         */
        unsigned int udpTestClientServerListenPort;

        Isa100::Common::UdpEndpoint udpTestClientEndpoint;

        Uint32 keysHardLifeTime;
        Uint16 subnetKeysHardLifeTime;

        //Uint32 keyNotValidBefore; //not needed

        /**
         * Enable processing of Channel Diagnostics and Channel Blacklisting.
         */
        bool channelBlacklistingEnabled;



        /**
         * Alerts
         */
        bool alertsEnabled;
        Uint16 alertsNeiMinUnicast;
        Uint8 alertsNeiErrThresh;
        Uint16 alertsChanMinUnicast;
        Uint8 alertsNoAckThresh;
        Uint8 alertsCcaBackoffThresh;


        /**
         * The maximum number of bytes logged in Cmds log file.
         */
        Uint32 cmdsMaxBytesLogged;

        /**
         *
         */
        bool activateMockKeyGenerator;

        /**
         *
         */
        bool debugMode;

        /**
         *
         */
        std::string lockFilesFolder;

        /**
         * When set to <code>false</code> the packages will not be encrypted.
         */
        bool disableTLEncryption;

        bool disableAuthentication;

        Uint16 publishPeriod;
        Uint16 publishPhase;

        /**
         * The max number of the last diagnostics received.
         */
        Uint16 diagnosticsMaxKeeped;

        /**
         * The path to the firmware files directory.
         */
        std::string firmwareFilesDirectory;

        /**
         * Represents the size of the chunks used to send the firmware update to the device.
         */
        Uint16 fileUploadChunkSize;

        /**
         * The number of seconds that a confirmation for a chunk is expected;
         * After that period the chunk is considered lost.
         */
        Uint16 fileUploadChunkTimeout;

        /**
         * The time interval defined for checking the number of devices' rejoins.
         */
        Uint16 rejoinTimeSpan;

        /**
         * Defines the maximum number of rejoins admitted for a time interval.
         * If the number of rejoins exceeds this limit in the defined time span
         * a restart command will be sent for the transceiver.
         */
        Uint8 rejoinRetryCounter;

        /**
         * A time threshold defined for checking duplicate messages.
         * Messages older than this threshold are not taken in consideration when checking for duplicates.
         */
        Uint16 duplicatesTimeSpan;


        /**
         * The TAI time when the UTC adjustment value will change from the current one.
         */
        Uint32 nextUTCAdjustmentTime;

        /**
         * The next value of the UTC accumulated leap second adjustment.
         */
        Int16 nextUTCAdjustment;

        /**
         * The name of the commands file. Contains one or more commands that will be sent to the devices
         * and the result will be logged into a logging file.
         */
        std::string debugCommandsFileName;

        bool obeyContractBandwidth;

        /**
         * The period between the execution of two tasks.
         */
        Uint16 roleActivationPeriod;

        /**
         * The period between the execution of two tasks.
         */
        Uint16 graphRedundancyPeriod;

        /**
         * The period between the execution of two tasks.
         */
        Uint16 routesEvaluationPeriod;

        /**
         * The period between the execution of two tasks.
         */
        Uint16 garbageCollectionPeriod;

        /**
         * The period for checking physical consistency for a device.
         */
        Uint16 consistencyCheckPeriod;


        Uint16 networkMaxDeviceTimeout;//NETWORK_MAX_DEVICE_TIMEOUT


        /**
         * The period for checking for expired contracts.
         */
        Uint16 expiredContractsPeriod;

        /**
         * Period for task: FindBetterParent (change parent for a devices on level >= 3)
         */
        Uint16 findBetterParentPeriod;

        /**
         * Period for task: evaluate dirty contracts.
         */
        Uint16 dirtyContractCheckPeriod;

        /**
         * Period for task: evaluate firmware download dedicated contracts.
         */
        Uint16 udoContractsCheckPeriod;

        /**
         * Period for task: evaluate client server contracts for subnet.changedDevicesOnOutbound
         */
        // Uint16 gwContractsCkeckPeriod;

        /**
         * Period for task: evaluate dirty edges to delete links
         */
        Uint16 dirtyEdgesCkeckPeriod;

        /**
         * Black Listing Object Period(seconds) - handles black listing.
         */
        Uint16 bloPeriod;

        Uint16 fullAdvertisingRoutersPeriod;

        /**
         * Period for task: fast discovery check.
         */
        Uint16 fastDiscoveryCheck;

        /**
         * Unstable check for devices.
         */
        bool unstableCheck;

        /**
         * Turn on/off custom alerts generated by SM.
         */
        bool enableAlertsGeneratedBySM;

        /**
         * Udo alert period (seconds).
         */
        int udoAlertPeriod;

        /**
         * Period for generating device join/leave alerts.
         */
        int joinLeaveAlertPeriod;

        /**
         * Period for generating contract alerts.
         */
        int contractAlertPeriod;

        /**
         * Period for generating topology alerts.
         */
        int topologyAlertPeriod;

        /**
         * Maximum time that ASL will wait(and make retries) for a response for a request from the moment the packet was added to Stack.
         */
        int maxASLTimeout;

        /**
         * stack log level (3 dbg, 2 inf, 1 err). If 0 will default to stack wrapper log level set in log4cpp.properties
         */
        int logLevelStack;

    public:

		typedef list<ProvisioningItem> SecurityProvisioning;
		/**
		 *
		 */
		SecurityProvisioning provisioningKeys;

		//SmSettingsLogic();
		static SmSettingsLogic& instance();

		void load();

		/**
		 * Called when a signal is received.
		 */
		void reload();

        void loadConfigIni();

        void loadProvisioningIni();

        void loadSubnetDefaultIni(bool isReloadOnSignal);

        void loadSubnetCustomIni(bool isReloadOnSignal);

        /**
         * Obtains the provisioning data for device. If provisioning is not fount throws Isa100Exception.
         * Must be used this method instead of map[] operator because ProvisioningItem does not have default constructor.
         * @param address
         * @return
         */
        ProvisioningItem* getProvisioningForDevice(const Address64& address);
    private:

        /**
         * Declare the static logger.
         */
        LOG_DEF("I.C.SmSettingsLogic");

        /**
         * The name of the system manager initialization file (default system_manager.ini).
         */
        string customIniFileName;

        // container with used subnets settings superframes births
        std::vector<Uint16> subnetsSuperframesBirths;

        SmSettingsLogic();

        void ParseCommonConfigFile(SimpleIni::CSimpleIniCaseA& iniParser);

        void parseProvisioningFile(SimpleIni::CSimpleIniCaseA& iniParser);

        /**
         * Reads the options from the file loaded in iniParser and sets the values in the subnetSettings.
         * @param iniParser
         * @param subnetSettings
         * @param fileName
         * @param isReloadOnSignal
         * @param subnetID - subnet ID for which the file is loaded. When the default file is loaded the subnetID is 0.
         */
        void ParseSubnetConfigFile(SimpleIni::CSimpleIniCaseA& iniParser, NE::Common::SubnetSettings& subnetSettings,
                    const std::string& fileName, bool isReloadOnSignal, bool isDefaultSubnetLoad);

        /**
         * Calls the ParseSubnetConfigFile method for parsing a dedicated subnet configuration file.
         *
         * * @param fileName - the name of the subnet configuration file
         */
//        void ParseSubnetCustomConfigFile(SimpleIni::CSimpleIniCaseA& iniParser, Uint16 subnetId_, const std::string& fileName, bool isReloadOnSignal);

        static void ParseAddrPortLine(const string& IPv6, const string& IPv4, const string& Port, Address128& p_ipv6,
                    Isa100::Common::UdpEndpoint& ipv4Enpoint);

        static void ParseProvisioningLine(const string& p_line, NE::Common::Address64& p_ip64,
                    NE::Model::SecurityKey& p_key, Uint16& p_subnetMask,  Uint16 &p_deviceRole);

        static void ParseProvisioningLine(const string& p_line, NE::Common::Address64& p_ipFirst, NE::Common::Address64& p_ipLast,
                    NE::Model::SecurityKey& p_key, Uint16& p_subnetMask,  Uint16 &p_deviceRole);

        void updateProvisioningByDeviceType(SimpleIni::CSimpleIniCaseA::TNamesDepend iniItems,
                    IniFileTag deviceTypeTag);

        /**
         * Parses the Uint8 values from a comma separated string of Uint8 values.
         */
        std::vector<Uint8> getAsVector(const std::string& text);

        static std::vector<Uint8> getAsChannelsVector(const std::string& channelsText);

        static std::vector<Uint16> getAsQueuePrioritiesVector(const std::string& queuePrioritiesText);

        static void trimSpaces(string& str);
};

}
}
#endif /*SMSETTINGSLOGIC_H_*/
