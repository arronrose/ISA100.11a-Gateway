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
 * @author george.petrehus, catalin.pop, radu.pop, ion.pocol
 */
#ifndef SMSTATELOG_H_
#define SMSTATELOG_H_

#include "Model/NetworkEngineTypes.h"
#include "Model/Operations/OperationsContainer.h"
#include "Model/Operations/IEngineOperation.h"
#include "Model/Subnet.h"
#include "Model/SubnetsContainer.h"
#include "Model/ModelPrinter.h"

using namespace NE::Common;
using namespace NE::Model;

namespace SMState {

/**
 * A utility class that allows the user to log different information about the SystemManager state.
 * Information will be logged to its own file to allow the users to see only the needed data.
 * For each type of information (eg: DeviceTable, ContractsTable ...) there will be a method in this class
 * that calls a method in specific class.
 * We have to have a class for each type of data because the logging mechanism is used to create the file
 * and we have to have a logger for each type of data.
 * @author Radu Pop
 */
class SMStateLog {

    public:

        SMStateLog();

        virtual ~SMStateLog();

        /**
         * Logs the state of the DeviceTable in its own file.
         */
        static void logDeviceTable(NE::Model::Subnet::PTR& subnet);

        /**
         * Log device attributes (in smAttributes.log)
         */
        static void logDeviceAttributes(Device* device, LogDeviceDetail& loggingDetailLevel, int slotsPerSec);

        /**
         * Log attributes of the devices from the specified subnet.
         */
        static void logSubnetDevicesAttributes(Subnet::PTR subnet, LogDeviceDetail& loggingDetailLevel);

        static void logDeviceAttributesMessage(std::string msg);

        /**
         * Logs the state of the NetworkTopology in its own file.
         */
        static void logNetworkTopology(NE::Model::Subnet::PTR& subnet);
        static void logNetworkTopology(NE::Model::SubnetsContainer& subnetContainer);

        static void logMngBandwidthChuncks(NE::Model::Subnet::PTR& subnet);


        static void logMngBandwidthChuncksForDevice(NE::Model::Subnet::PTR& subnet, Address16 device);

        /**
         * Log the operations reason.
         */
        static void logOperationReason(const std::string& reason);

        /**
         * Log the operation.
         */
        static void logOperation(const std::string& sign, NE::Model::Operations::IEngineOperationPointer operation);
        static void logOperationConfirm(NE::Model::Operations::IEngineOperationPointer operation);

        /**
         * Logs the list operations to a specific log file.
         */
        static void logOperationsContainer(const std::string& reason, NE::Model::Operations::OperationsContainer& operationsContainer);

        static void logSubnetContainer(NE::Model::SubnetsContainer& subnetContainer);

};

}

#endif /*SMSTATELOG_H_*/
