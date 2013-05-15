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
 * DoubleExitFirstLayer.cpp
 *
 *  Created on: Oct 28, 2009
 *      Author: flori.parauan, mulderul(Catalin Pop), beniamin.tecar
 */

#include "DoubleExitFirstLayer.h"
#include "Common/NETypes.h"
#include "Model/IEngine.h"

namespace NE {

namespace Model {

namespace Routing {

namespace Algorithms {

    void DoubleExitFirstLayer::findDoubleExitFirstLayer(Subnet::PTR subnet, Uint16 graphID, DoubleExitEdges &usedEdges, Address16Set& nodesSkippedForBackup) {
        Device* backbone = subnet->getBackbone();

        RETURN_ON_NULL(backbone);

        //populate firstLayerDevices with devices that have edges with backbone
        backbone->getEdges(firstLayerDevices);
        backbone->setVisited();
        //erase the manager from  firstLayerDevices
        firstLayerDevices.erase(Address::getAddress16(subnet->getManagerAddress32()));
        GraphPointer graph = subnet->getGraph(graphID);
        if(!graph) {
            LOG_INFO("GRAPH is NULL :" << (int) graphID);
            return;
        }

        //delete from firstLayerDevices those devices that don't have the backbone as parent.
        //as result, the list will contain the level one devices
        filterFirstLayerDevices(subnet, graph, backbone->address32);

        DoubleExitEdges graphEdges = graph->getGraphEdges();

        DoubleExitEdges::iterator existsOnGraph;
        Address16Set withoutDoubleExit;
        Uint8 const neighborsNo = 1;
        Device* device = NULL;

        Uint32 currentTime = ClockSource::getCurrentTime();
        NE::Common::SubnetSettings subnetSettings = subnet->getSubnetSettings();

        for(Address16Set::iterator it = firstLayerDevices.begin(); it != firstLayerDevices.end(); ++it) {
            subnet->setDeviceVisited(*it);
        }

        for(Address16Set::iterator it = firstLayerDevices.begin(); it != firstLayerDevices.end(); ++it) {
            device = subnet->getDevice(*it);
            if (!device) {
                continue;
            }
            existsOnGraph = graphEdges.find(*it);
            if(existsOnGraph != graphEdges.end()) {
                if(subnet->getDeviceNrOfNeighbors(*it) > neighborsNo) {
                    if (!graph->deviceHasDoubleExit(*it)) {// if device has no redundancy
                        if ((currentTime - device->joinConfirmTime >= subnetSettings.delayActivateMultiPathForDevice)) {
                            withoutDoubleExit.insert(*it);
                            DoubleExitDestinations destinations;
                            destinations.prefered = Address::getAddress16(device->parent32);
                            destinations.backup = 0;
                            parentsMap.insert(std::make_pair(*it, destinations));
                        } else {
                            parentsMap.insert(std::make_pair(*it, existsOnGraph->second));
                        }
                    } else {
                        if(subnet->neighborsAreValid(existsOnGraph->first, existsOnGraph->second)) {
                            parentsMap.insert(std::make_pair(*it, existsOnGraph->second));
                        } else {
                            withoutDoubleExit.insert(*it);
                            DoubleExitDestinations destinations;
                            destinations.prefered = Address::getAddress16(device->parent32);
                            destinations.backup = 0;
                            parentsMap.insert(std::make_pair(*it, destinations));
                        }
                    }
                } else {
                    DoubleExitDestinations destinations;
                    destinations.prefered = Address::getAddress16(device->parent32);
                    destinations.backup = 0;
                    parentsMap.insert(std::make_pair(*it, destinations));
                }
            }
        }


       for(Address16Set::iterator it = withoutDoubleExit.begin(); it != withoutDoubleExit.end(); ) {
            device = subnet->getDevice(*it);
            if(NULL == device) {
                continue;
            }

            if (device->parent32 != backbone->address32) {
                continue;
            }

            getBackupForDevice(subnet, device, nodesSkippedForBackup);
            subnet->setDeviceVisited(*it);
            DoubleExitEdges::iterator deviceParents = parentsMap.find(*it);
            if(deviceParents != parentsMap.end() && (deviceParents->second.backup != 0)) {//if device has backup delete it from withoutDoubleExit list
                nodesSkippedForBackup.erase(*it);//remove from skipped nodes..because it have backup
                LOG_INFO("add to nodesSkippedForBackup" << std::hex << *it );
                withoutDoubleExit.erase(it++);
            } else {
                ++it;
            }
        }

       if(withoutDoubleExit.size() > 1) {
           //add devices without doubleExit to discoveryAcceleratedlist
           subnet->listDiscoveryToBeAccelerated.insert( subnet->listDiscoveryToBeAccelerated.end(), withoutDoubleExit.begin(), withoutDoubleExit.end());
       }

       usedEdges = parentsMap;


    }

    void DoubleExitFirstLayer::getBackupForDevice(Subnet::PTR subnet, Device* device, Address16Set& nodesSkippedForBackup) {
        if(!device) {
            return;
        }

        for (Address16 i = 0; i < DEFAULT_MAX_NO_CANDIDATES; ++i) {
            Device* candidate = subnet->getDevice(device->theoAttributes.candidates[i]);
            if (candidate
                        && (candidate->statusForReports == StatusForReports::JOINED_AND_CONFIGURED )
                        && candidate->capabilities.isRouting()
                        && device->hasCandidate(device->theoAttributes.candidates[i])
                        && !device->theoAttributes.candidateIsBadRate(device->theoAttributes.candidates[i])) {

                if(subnet->candidateIsEligibleAsBackup(Address::getAddress16(device->address32), device->theoAttributes.candidates[i]) ) {

                    if (candidate->theoAttributes.isSelectedAsBackup){//if device is skipped for backup add it to the list
                        nodesSkippedForBackup.insert(Address::getAddress16(device->address32));
                        LOG_INFO("add to nodesSkippedForBackup" << std::hex << Address::getAddress16(device->address32)
                        << " due to " << std::hex << device->theoAttributes.candidates[i]);
                        continue;
                    }

                    if (isBackupSuitable(Address::getAddress16(device->address32),device->theoAttributes.candidates[i])) {
                        if((device->address32 != candidate->address32) &&
                            !subnet->isBackupForDevice(Address::getAddress16(device->address32), Address::getAddress16(candidate->address32))){
                            candidate->theoAttributes.isSelectedAsBackup = true;
                            LOG_DEBUG("Candidate is selected as backup " << std::hex << device->theoAttributes.candidates[i]
                                << "by device" << std::hex << Address::getAddress16(device->address32));
                        }
                        return;
                    }
                }
            }
        }

    }

    bool DoubleExitFirstLayer::isBackupSuitable(Address16 device, Address16 backup) {
        Address16Set::iterator  isOnFirstLayer = firstLayerDevices.find(backup);
        DoubleExitEdges::iterator  deviceOnParents = parentsMap.find(device);
        DoubleExitEdges::iterator  backupOnParents = parentsMap.find(backup);
        if( (isOnFirstLayer !=  firstLayerDevices.end()) &&
                    (deviceOnParents != parentsMap.end())  &&
                    (backupOnParents != parentsMap.end() &&
                                !hasCycle(device, backup)) ) {
            if ((deviceOnParents->second.prefered != backup) &&
                (deviceOnParents->second.backup != backup) &&
                (backupOnParents->second.prefered != device) &&
                (backupOnParents->second.backup != device) ) {
                deviceOnParents->second.backup = backup;
                return true;
            }

        }

     return false;
    }

    bool DoubleExitFirstLayer::hasCycle(Address16 device, Address16 backup) {
        DoubleExitEdges::iterator  hasBackup = parentsMap.find(backup);
        while(hasBackup != parentsMap.end()) {
            if(!hasBackup->second.backup) {
                return false;
            }
            if(hasBackup->second.backup == device) {
                return true;
            }
            hasBackup = parentsMap.find(hasBackup->second.backup);

        }

        return false;
    }

    void DoubleExitFirstLayer::filterFirstLayerDevices(Subnet::PTR subnet, GraphPointer& graph, Address32 backboneAddr) {

        for(Address16Set::iterator it = firstLayerDevices.begin(); it != firstLayerDevices.end(); ) {
             Device* device = subnet->getDevice(*it);
             if(!device) {
                 firstLayerDevices.erase(it++);
             } else if(device->parent32 !=  backboneAddr) {
                 firstLayerDevices.erase(it++);
             } else if(graph->existsSourceDevice(*it)) {
                 ++it;
             } else {
                 firstLayerDevices.erase(it++);
             }
        }
    }


    Uint8 DoubleExitFirstLayer::getNoOfFirstLayerDevices() {
        return firstLayerDevices.size();
    }


}
}
}
}
