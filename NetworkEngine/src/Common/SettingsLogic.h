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
 * SettingsLogic.h
 *
 *  Created on: Jan 4, 2010
 *      Author: Catalin Pop
 */

#ifndef SETTINGSLOGIC_H_
#define SETTINGSLOGIC_H_

#include "Common/NEAddress.h"
#include "Common/NETypes.h"
#include "Common/SubnetSettings.h"
#include <map>

namespace NE {
namespace Common {

/**
 * Map of key=SubnetId and Value=NE::Common::SubnetSettings.
 */
typedef std::map<Uint16, NE::Common::SubnetSettings> SubnetSettingsMap;

class SettingsLogic {

    public:

        Address64 managerAddress64;

        /**
         * Represent the source address from NetworkNivisHeader.
         */
        Address128 managerAddress128;

        std::string managerTag;
        std::string managerModel;
        std::string managerSerialNo;
        static std::string managerVersion;

        Uint32 networkID;

        /**
         * The array with all the subnet ids for the configured backbones.
         */
        std::vector<Uint16> subnetIds;

        /**
         * The max number of the last diagnostics received.
         */
        Uint16 diagnosticsMaxKeeped;


        /**
         * CommittedBurst used for Manager => GW/BR traffic.
         */
        Int16 manager2NeighborCommittedBurst;

        Int16 neighbor2ManagerCommittedBurst;


        /**
         *The prefix of the ipv6Addresses
         */
        Uint16 ipv6AddrPrefix;

        /**
         * Indicates the maximum NSDU supported in octets which can be converted by the source into max APDU size
         * supported by taking into account the TL, security, AL header and TMIC sizes;
         * valid value set: 70 - 1280 (except for GW where it can be as high as 32k)
         */
        Uint16 max_NSDU_Size;
        Uint16 gw_max_NSDU_Size;
        Uint16 bbr_max_NSDU_Size;

        /**
         * The maximum send window size.
         */
        Uint16 maxSendWindowSize;

        /**
         * The current value of the UTC accumulated leap second adjustment.
         */
        Int16 currentUTCAdjustment;

		/**
		 * The threshold of failed CCA's after which the channel is blacklisted.
		 */
		Uint8 channelBlacklistingThresholdPercent;

		/**
		 * The amount of time a channel is kept in the blacklist.
		 */
		Uint32 channelBlacklistingKeepPeriod;

        Uint32 channelBlacklistingTAICutover;


		/**
         * The time offset when the command sent to the device will become active.
         */
        Uint16 tai_cutover_offset;

        Uint16 containerExpirationInterval;

        /**
         * Maximum latency (as percent of period) used on the network. This param is used on contract request
         * with min (deadline, networkMaxLatencyPercent_transformed_to_slots).
         */
        Uint8 networkMaxLatencyPercent;

        /**
         * Max number of nodes in whole network.
         */
        Uint16 networkMaxNodes;

        /**
         * Enable/disable creation of special contracts and allocation of special links for firmware upgrade sessions.
         */
        bool firmwareContractsEnabled;

        /**
         * Time span from BR join in which PS contracts are refused for devices that don't have backup.
         */
        Uint16 psContractsRefusalTimeSpan;

        /**
         * Time span from Device join in which PS contracts are refused for devices that don't have backup.
         */
        Uint16 psContractsRefusalDeviceTimeSpan;

        /**
         * Interval expected for a device to become JOINED_AND_CONFIGURED.
         * If this interval has elapsed since device's start time and the device is not JOINED_AND_CONFIGURED, it will be removed.
         */
        Uint16 joinMaxDuration;

        /**
         * Change device parent by parent load.
         */
        bool changeParentByLoad;

    protected:
        SubnetSettings subnetSettingsDefault;
        SubnetSettingsMap subnetSettingsMap;

    public:

        SettingsLogic() :
            ipv6AddrPrefix(0xFC00),
            containerExpirationInterval(240),
            networkMaxLatencyPercent(0),
            networkMaxNodes(0),
            psContractsRefusalTimeSpan(0),
            psContractsRefusalDeviceTimeSpan(0),
            joinMaxDuration(300),
            changeParentByLoad(false) {
        }

        virtual ~SettingsLogic() {

        }

        SubnetSettings& getSubnetSettings(Uint16 subnetId){
            SubnetSettingsMap::iterator it = subnetSettingsMap.find(subnetId);
            if(it == subnetSettingsMap.end()){
                return subnetSettingsDefault;
            } else {
                return it->second;
            }
        }

        void addCustomSubnetSettings(Uint16 subnetId, SubnetSettings& settings){
            subnetSettingsMap[subnetId] = settings;
        }

        SubnetSettings& getSubnetSettingsDefault(){
            return subnetSettingsDefault;
        }

        SubnetSettingsMap& getSubnetSettingsMap(){
            return subnetSettingsMap;
        }

};

}
}
#endif /*SETTINGSLOGIC_H_*/
