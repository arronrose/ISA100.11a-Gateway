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
 * DoubleExitMultiSinkGraphRouting.cpp
 *
 *  Created on: Aug 20, 2009
 *      Author: flori.parauan, mulderul(Catalin Pop), beniamin.tecar
 */

#include "DoubleExitMultiSinkGraphRouting.h"

#include "Common/NEAddress.h"

#include "Model/Routing/Edge.h"
#include "DoubleExitFirstLayer.h"
#include "Common/ClockSource.h"

//#include "Model/Routing/Node.h"

namespace NE {

namespace Model {

namespace Routing {

namespace Algorithms {

using namespace NE::Model::Operations;
DoubleExitMultiSinkGraphRouting::DoubleExitMultiSinkGraphRouting() {
}

DoubleExitMultiSinkGraphRouting::~DoubleExitMultiSinkGraphRouting() {
}

void DoubleExitMultiSinkGraphRouting::evaluateGraph(NE::Model::Subnet::PTR subnet, Uint16 graphID, Address32 destination,
            DoubleExitEdges &usedEdges) {
    //evaluate the inbound  graph G1
    evaluateInBoundRoute(subnet, graphID, destination, usedEdges);
}


void DoubleExitMultiSinkGraphRouting::evaluateInBoundRoute( NE::Model::Subnet::PTR subnet, Uint16 graphID, Address32 destination,
            DoubleExitEdges &usedEdges) {
    Address16Set targetsList;
    Address16Set nodesWithotDoubleExit;
    Address16Set nodesSkippedForBackup;

    subnet->markAllDevicesUnVisited();
    DoubleExitFirstLayer doubleExFirstLayer;
    doubleExFirstLayer.findDoubleExitFirstLayer(subnet, graphID, usedEdges, nodesSkippedForBackup);

    if (LOG_DEBUG_ENABLED()){
        std::ostringstream stream;
        for(DoubleExitEdges::iterator it = usedEdges.begin(); it != usedEdges.end(); ++it) {
            stream << std::hex << it->first << "(" << it->second.prefered << "," << it->second.backup << "), ";
//            LOG_DEBUG(std::hex << it->first << "-->" << std::hex << it->second.prefered << std::endl);
//            if (it->second.backup != 0) {
//                LOG_DEBUG(std::hex << it->first << "-->" << std::hex << it->second.backup << std::endl);
//            }
        }
        LOG_DEBUG("FirstLayer: " << stream.str());
    }

    Uint8 firstLayerDeviceNo = doubleExFirstLayer.getNoOfFirstLayerDevices();
    LOG_INFO("firstLayerDeviceNo = " << (int)firstLayerDeviceNo);

    // TODO 2 = the gw and bbr that are on one network (the sm does not have a place in the subnet)
    if(subnet->getNumbetOfDevicesFromSubnet() > (firstLayerDeviceNo + 2)) { //if there are device on levels > 1
        DoubleExit doubleExit;
        doubleExit.SetTargetsNumber(firstLayerDeviceNo + 1); //first layer devices + backbone

        doubleExit.FindDoubleExits(subnet, graphID, nodesSkippedForBackup);
        LOG_DEBUG("dupa double exit..inainte de update" );
        for(DoubleExitEdges::iterator it = usedEdges.begin(); it != usedEdges.end(); ++it) {
            LOG_DEBUG(std::hex << it->first << "-->" << std::hex << it->second.prefered << std::endl);
            if (it->second.backup != 0) {
                LOG_DEBUG(std::hex << it->first << "-->" << std::hex << it->second.backup << std::endl);
            }
        }

        doubleExit.GetUsedEdges(subnet, usedEdges);
        doubleExit.GetNodesWithoutDoubleExit(subnet, graphID, targetsList, nodesWithotDoubleExit);
        LOG_DEBUG("Nodes without double exit");
        for(Address16Set::iterator it = nodesWithotDoubleExit.begin(); it != nodesWithotDoubleExit.end(); it++)  {
            LOG_DEBUG(std::hex << *it);
        }


        doubleExit.PrintVertexesWithDoubleExit();

        Address16List visitedDevicesList;
        LOG_DEBUG("VISITED DEVICES");
        for(Address16List::iterator it = visitedDevicesList.begin(); it != visitedDevicesList.end(); it++)  {
            LOG_DEBUG(std::hex << *it);
        }



        bool processed = true;
        while (!nodesWithotDoubleExit.empty() && processed) {
            Address16Set::iterator it = nodesWithotDoubleExit.begin();
            processed = false;
            for (; it != nodesWithotDoubleExit.end(); ) {
                Device * device = subnet->getDevice(*it);
                if (device == NULL){
                    LOG_WARN("Device NULL on evaluate: " << Address_toStream(*it) << ", SubnetID:" << (int) subnet->getSubnetId());
                    ++it;
                    continue;
                }

                //if device's parent is visited, add device to the final solution trying to find him a backup through its visited candidates
                if (subnet->isDeviceVisited(Address::getAddress16(device->parent32))) {
                    addEdgesToSolution(subnet, *it, Address::getAddress16(device->parent32), usedEdges, nodesSkippedForBackup);
                    subnet->setDeviceVisited(*it);
                    processed = true;
                    nodesWithotDoubleExit.erase(it++);
                }
                else {
                    ++it;
                }
            }

        }
        subnet->markAllDevicesUnVisited();

        LOG_DEBUG("dupa multiSink") ;
    }

    if (!nodesSkippedForBackup.empty() && subnet){
    	 LOG_INFO("add graph 1 to evaluation due to nodesSkippedForBackup" );
    	 subnet->addGraphToBeEvaluated(graphID);
    }


    LOG_INFO("after the 'if' firstLayerDeviceNo = " << (int)firstLayerDeviceNo);

    for(DoubleExitEdges::iterator it = usedEdges.begin(); it != usedEdges.end(); ++it) {
        LOG_DEBUG(std::hex << it->first << "-->" << std::hex << it->second.prefered);
        if (it->second.backup != 0) {
            LOG_DEBUG(std::hex << it->first << "-->" << std::hex << it->second.backup);
        }
    }

}


void DoubleExitMultiSinkGraphRouting::CreateTargetsList(NE::Model::Subnet::PTR subnet, Uint16 graphID, Address16 destination,
            Address16Set &targetsList) {

    Device* destinationDevice = subnet->getDevice(destination);
    RETURN_ON_NULL_MSG(destinationDevice, "Destination is NULL: " << Address_toStream(destination));

    if (destinationDevice->capabilities.isManager()) {
        Device* backbone = subnet->getBackbone();
        RETURN_ON_NULL_MSG( backbone, "Backbone  is NULL ");
        EdgesList edgesBackbone;
        subnet->getOutBoundEdges(Address::getAddress16(backbone->address32), edgesBackbone);
        for (EdgesList::iterator i = edgesBackbone.begin(); i != edgesBackbone.end(); ++i) {
            if ((*i)->getEdgeStatus() == Status::ACTIVE || (*i)->getEdgeStatus() == Status::CANDIDATE) {
                targetsList.insert((*i)->getDestination());

            }
        }
    }

    else {
        EdgesList edges;
        subnet->getOutBoundEdges(destination, edges);
        for (EdgesList::iterator i = edges.begin(); i != edges.end(); ++i) {
            if ((*i)->getEdgeStatus() == Status::ACTIVE || (*i)->getEdgeStatus() == Status::CANDIDATE) {
                targetsList.insert((*i)->getDestination());
            }
        }
    }
}

void DoubleExitMultiSinkGraphRouting::addEdgesToSolution(Subnet::PTR subnet, Address16 device,  Address16 parent, DoubleExitEdges &usedEdges, Address16Set& nodesSkippedForBackup) {
    DoubleExitEdges::iterator existsDevice = usedEdges.find(device);
    DoubleExitDestinations destinations;
    destinations.prefered = parent;
    destinations.backup = 0;

    Device* dvc = subnet->getDevice(device);
    if(!dvc) {
        return;
    }


    Uint32 currentTime = ClockSource::getCurrentTime();
    NE::Common::SubnetSettings subnetSettings = subnet->getSubnetSettings();

    if ((dvc->statusForReports ==  StatusForReports::JOINED_AND_CONFIGURED)
                && (currentTime - dvc->joinConfirmTime >= subnetSettings.delayActivateMultiPathForDevice )) {
        //try to find an eligible candidate to set it as backup
        for(Address16 i = 0; i < DEFAULT_MAX_NO_CANDIDATES; ++i) {
            Device* candidate = subnet->getDevice(dvc->theoAttributes.candidates[i]);
            if (candidate && (candidate->address32 != dvc->parent32) && candidate->statusForReports == StatusForReports::JOINED_AND_CONFIGURED
                        && subnet->candidateIsEligibleAsBackup(device, dvc->theoAttributes.candidates[i], true)) {
                DoubleExitEdges::iterator existsCandidate = usedEdges.find( dvc->theoAttributes.candidates[i]);
                if(Address::getAddress16(candidate->parent32) != device) {
                    if(((existsCandidate != usedEdges.end()) && ((existsCandidate->second.prefered == device) || (existsCandidate->second.backup == device))) ) {
                        continue;
                    }
                    else {
                        if(candidate->theoAttributes.isSelectedAsBackup) {
                            if(subnet->isBackupForDevice(device, dvc->theoAttributes.candidates[i])) {
                                destinations.backup = dvc->theoAttributes.candidates[i];
                                nodesSkippedForBackup.erase(device);
                                LOG_INFO("Erase from nodesSkippedForBackup"<< std::hex<<(int) device);
                                break;
                            }

                        }
                        else {
                            destinations.backup = dvc->theoAttributes.candidates[i];
                            if(!subnet->isBackupForDevice(device, dvc->theoAttributes.candidates[i])) {

                                candidate->theoAttributes.isSelectedAsBackup = true;
                                LOG_DEBUG("Candidate is selected as backup " << std::hex << dvc->theoAttributes.candidates[i]
                                    << "by device" << std::hex << device);
                            }
                            break;
                        }
                    }
                }
            }
        }
        if (destinations.backup == 0){
            //if here means no good candidate was found: accelerate discovery
            subnet->listDiscoveryToBeAccelerated.push_back(Address::getAddress16(dvc->address32));
        }
    }


    if(existsDevice == usedEdges.end()) {
        usedEdges.insert(std::make_pair(device, destinations));
    }

    else {

        existsDevice->second = destinations;
    }

}

bool DoubleExitMultiSinkGraphRouting::nothingChangesFromLastIteration(Subnet::PTR subnet, Uint16 graphID) {
    GraphPointer graph = subnet->getGraph(graphID);
    Address16Set devicesWithout2Exit;
    if(!graph) {
        return true;
    }


    graph->getDevicesWithoutDoubleExit(devicesWithout2Exit);
    for(Address16Set::iterator it = devicesWithout2Exit.begin(); it != devicesWithout2Exit.end(); ++it) {
        Device* device = subnet->getDevice(*it);
        if(device) {
            if(subnet->getDeviceNrOfNeighbors(*it) > 1) {
                return false;
            }
        }
    }

    return true;

}


}
}
}
}
