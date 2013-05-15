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
#include "SMState/SMStateLog.h"
#include "Model/NetworkEngine.h"
#include "Model/SubnetPrinter.h"
#include "Model/DevicePrinter.h"
#include "Model/Routing/GraphPrinter.h"


/**
 * It should be DEBBUG and not INFO or greater priority because if we will put INFO on root logger
 * then this information will also appear in the main logger.
 */
namespace SMState {

struct StateDeviceTable {
        LOG_DEF("SMState.DeviceTable")
        ;

        static void logDeviceTable(NE::Model::Subnet::PTR& subnet) {

            LOG_DEBUG(NE::Model::SubnetShortPrinter(subnet));

        }
};

struct StateNetworkTopology {
        LOG_DEF("SMState.NetworkTopology");


        static void logNetworkTopology(NE::Model::Subnet::PTR subnet) {
            RETURN_ON_NULL_MSG(subnet, "StateNetworkTopology::logNetworkTopology : subnet is NULL");
            NE::Model::GraphsMap graphs = subnet->getGraphsMap();
            if(!graphs.empty()) {
                    LOG_INFO( "SubnetID:" << std::setw(3) << (int)subnet->getSubnetId() << std::endl);
                    for(GraphsMap::iterator it = graphs.begin(); it != graphs.end(); ++it) {
                        if(NULL != it->second) {
                            LOG_INFO( "GraphID=" << std::setw(3) << (int)(it->second)->getGraphId() << ": {" << NE::Model::Routing::GraphPrinter(subnet.get(), it->second) << "}, ");
                        }
                    }
                }
        }

};

struct StateManagementChunks {
    LOG_DEF("SMState.StateManagementChunks");

    static void logMngBandwidthChuncksForDevice(NE::Model::Subnet::PTR& subnet, Address16 dvc) {
        RETURN_ON_NULL_MSG(subnet, "StateManagementChunks::logMngBandwidthChuncksForDevice : subnet is NULL");
        Device* device = subnet->getDevice(dvc);
        RETURN_ON_NULL_MSG(subnet, "StateManagementChunks::logMngBandwidthChuncksForDevice : device is NULL");
        LOG_DEBUG("Chunks reserved by device " << std::hex << dvc << device->theoAttributes);
        /*NE::Common::SubnetSettings & subnetSettings = */subnet->getSubnetSettings();
    }

    static void logMngBandwidthChuncks(NE::Model::Subnet::PTR& subnet) {
        RETURN_ON_NULL_MSG(subnet, "StateManagementChunks::logMngBandwidthChuncks : subnet is NULL");
        NE::Common::SubnetSettings& subnetSettings = subnet->getSubnetSettings();
        for (Uint8 i = MAX_NUMBER_OF_BANDWIDTH_CHUNCKS - 1; i > subnetSettings.join_reserved_set; i -= 2) {
            for (Uint8 j = 1; j < subnet->getSubnetSettings().numberOfFrequencies; ++j) {
                if(subnet->mngChunksTable[i][j].owner) {
                    LOG_DEBUG("Chunks Set No:" << i << " on channel" << j << "reserved by" << subnet->mngChunksTable[i][j].owner);
                }
            }

        }
    }
};

struct StateOperations {
        LOG_DEF("SMState.Operations")
        ;

        static void logOperationReason(const std::string& reason) {
            LOG_INFO(reason);
        }

        static void logOperation(const std::string& sign, NE::Model::Operations::IEngineOperationPointer operation) {
            LOG_INFO(std::setw(3) << sign << " " << *operation);
        }
        static void logOperationConfirm(NE::Model::Operations::IEngineOperationPointer operation) {
            std::string sign = "*";
            sign += EngineOperationType::getSign(operation->getType());
            LOG_INFO(std::setw(3) << sign << " " << IEngineOperationShortPrinter(*operation));
        }

        static void logOperations(const std::string& reason, NE::Model::Operations::OperationsContainer& operations) {
            LOG_INFO("REASON : " << reason << operations);
        }
};

struct StateAttributes {
        LOG_DEF("Isa100SMState.SmAttributes");

        static void logSmAttribute(std::string msg) {
            LOG_DEBUG(msg);
        }

        static void logDevice(Device * device, LogDeviceDetail& level, int slotsPerSec){
            LOG_DEBUG(NE::Model::LevelDeviceDetailedPrinter(device, level, slotsPerSec));
        }

        static void logSubnet(Subnet::PTR subnet, LogDeviceDetail& level){
            LOG_DEBUG(NE::Model::LevelSubnetDetailPrinter(subnet, level));
        }
};

SMStateLog::SMStateLog() {
}

SMStateLog::~SMStateLog() {
}

void SMStateLog::logDeviceTable(NE::Model::Subnet::PTR& subnet) {
    StateDeviceTable::logDeviceTable(subnet);
}

void SMStateLog::logDeviceAttributes(Device* device, LogDeviceDetail& loggingDetailLevel, int slotsPerSec){
	StateAttributes::logDevice(device, loggingDetailLevel, slotsPerSec);
}

void SMStateLog::logDeviceAttributesMessage(std::string msg){
	StateAttributes::logSmAttribute(msg);
}

void SMStateLog::logSubnetDevicesAttributes(Subnet::PTR subnet, LogDeviceDetail& loggingDetailLevel) {
    StateAttributes::logSubnet(subnet, loggingDetailLevel);
}

void SMStateLog::logNetworkTopology(NE::Model::Subnet::PTR& subnet) {
    StateNetworkTopology::logNetworkTopology(subnet);
}
void SMStateLog::logNetworkTopology(NE::Model::SubnetsContainer& subnetContainer) {
    for (NE::Model::SubnetsMap::iterator it = subnetContainer.getSubnetsList().begin(); it != subnetContainer.getSubnetsList().end(); ++it){
        logNetworkTopology(it->second);
    }
}

void   SMStateLog::logMngBandwidthChuncksForDevice(NE::Model::Subnet::PTR& subnet, Address16 device) {
    StateManagementChunks::logMngBandwidthChuncksForDevice(subnet, device);
}

void   SMStateLog::logMngBandwidthChuncks(NE::Model::Subnet::PTR& subnet) {
    StateManagementChunks::logMngBandwidthChuncks(subnet);
}

void SMStateLog::logOperationReason(const std::string& reason) {
    StateOperations::logOperationReason(reason);
}

void SMStateLog::logOperation(const std::string& sign, NE::Model::Operations::IEngineOperationPointer operation) {
    StateOperations::logOperation(sign, operation);
}

void SMStateLog::logOperationConfirm(NE::Model::Operations::IEngineOperationPointer operation) {
    StateOperations::logOperationConfirm(operation);
}

void SMStateLog::logOperationsContainer(const std::string& reason, NE::Model::Operations::OperationsContainer& operationsContainer) {
    StateOperations::logOperations(reason, operationsContainer);
}

void SMStateLog::logSubnetContainer(NE::Model::SubnetsContainer& subnetContainer){
    for (NE::Model::SubnetsMap::iterator it = subnetContainer.getSubnetsList().begin(); it != subnetContainer.getSubnetsList().end(); ++it){
        logDeviceTable(it->second);
    }
}

}
