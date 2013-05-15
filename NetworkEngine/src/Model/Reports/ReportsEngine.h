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
 * ReportsEngine.h
 *
 *  Created on: Oct 2, 2009
 *      Author: flori.parauan
 */

#ifndef REPORTSENGINE_H_
#define REPORTSENGINE_H_

#include "Common/logging.h"
#include "Model/Subnet.h"
#include "ReportsStructures.h"
#include "Common/ClockSource.h"
#include "Model/SubnetsContainer.h"
#include "Common/ISettingsProvider.h"
#include "Common/SettingsLogic.h"

namespace NE {

namespace Model {

namespace Reports {

class ReportsEngine {

        LOG_DEF("I.M.ReportsEngine");

    private:
        SubnetsContainer* subnetsContainer;

        NE::Common::SettingsLogic * settingsLogic;

        /**
         * The time the NetworkEngine was started.
         */
        Uint32 startTime;

        /**
         * Count of all join processes in which devices reach the status JOINED_AND_CONFIGURED.
         */
        Uint32 globalJoinCounter;

        /**
         * Packet statistics for the entire network.
         */
        Uint32 DPDUsSent; // GS_DPDUs_Sent
        Uint32 DPDUsLost; // GS_DPDUs_Lost

    private:
        void getDeviceReport(Address16 deviceAddress, Subnet::PTR subnet, DeviceListReport& deviceListReport);
        void getDeviceReport(Device *device, NetworkOrderStream& stream, Uint16 & nrOfDevices);

        void getTopologyReportEntry(Subnet::PTR & subnet, Device& device, TopologyReport& topologyReport);
        void getTopologyReportEntry(Subnet::PTR & subnet, Device& device, NetworkOrderStream& stream, NetworkOrderStream & backboneStream, Uint16 & nrOfDevices, Uint16 & nrOfBackbones);

        void getTopologyEntryForManager(TopologyReport& topologyReport);
        void getTopologyEntryForManager(NetworkOrderStream& stream, Uint16 & nrOfDevices);

        void getDeviceSchedule(Address16 deviceAddress, Subnet::PTR subnet, ScheduleReport& scheduleReport);
        void getDeviceSchedule(Subnet::PTR subnet, Device* device, NetworkOrderStream & stream, Uint16 & nrOfSchedules);


        void getDeviceHealth(Address16 deviceAddress, Subnet::PTR subnet, DeviceHealthReport& deviceHealthReport);
        void getDeviceHealth(Device * device, NetworkOrderStream& stream, Uint16 & nrOfHealth);

    public:

        ReportsEngine(NE::Model::SubnetsContainer * subnetsContainer, NE::Common::SettingsLogic * settingsLogic,
                     Uint32 startTime);

        Uint32 getGlobalJoinCounter() {
            return globalJoinCounter;
        }

        void incrementGlobalJoinCounter() {
            ++globalJoinCounter;
        }

        /**
         * Creates a device list report.
         */
        void createDeviceListReport(DeviceListReport& deviceListReport);
        void createDeviceListReport(NetworkOrderStream& stream);

        /**
         * Populates the TopologyReport structure.
         * @param subnet
         * @param deviceListReport
         */
        void createTopologyReport(TopologyReport& topologyReport);
        void createTopologyReport(NetworkOrderStream& stream);

        /**
         * Creates a schedule report.
         */
        void createScheduleReport(ScheduleReport& scheduleReport);
        void createScheduleReport(NetworkOrderStream & stream);

        /**
         * Creates a device health report.
         */
        void createDeviceHealthReport(DeviceHealthReport& deviceHealthReport);
        void createDeviceHealthReport(NetworkOrderStream & stream);

        /**
         * Creates a neighbor health report.
         */
        void createNeighborHealthReport(NeighborHealthReport& neighborHealthReport, const Address32 &neighborAddress);
        bool createNeighborHealthReport(const Address32 &neighborAddress, NetworkOrderStream & stream);

        /**
         * Creates a network health report.
         */
        void createNetworkHealthReport(NetworkHealthReport& networkHealthReport);
        void createNetworkHealthReport(NetworkOrderStream & stream);

        /**
         * Creates a network resource report.
         */
        void createNetworkResourceReport(NetworkResourceReport& networkResourceReport);
        void createNetworkResourceReport(NetworkOrderStream & stream);

};

}
}
}

#endif /* REPORTSENGINE_H_ */
