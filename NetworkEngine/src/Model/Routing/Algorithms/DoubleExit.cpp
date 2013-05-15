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
 * DoubleExit.cpp
 *
 *  Created on: Aug 12, 2009
 *      Author: flori.parauan, mulderul(Catalin Pop), beniamin.tecar
 */
#include <map>
#include <limits>
#include "DoubleExit.h"
#include "Common/ClockSource.h"

namespace NE {
namespace Model {
namespace Routing {
namespace Algorithms {

static const float failureProb = 0.1f;
static const float MAX_FLOAT = std::numeric_limits<float>::max();
static const  int stopForDebug = 1000;
static const float costNoBackup = 100000;


void DoubleExit::FindDoubleExits(Subnet::PTR subnet, Uint16 graphID, Address16Set& nodesSkippedForBackup) {

    GraphPointer gp = subnet->getGraph(graphID);
    DoubleExitEdges graphEdges = gp->getGraphEdges();

    //a set of devices without redundancy ..the algorithm will try to find redundancy for devices from this set
    Address16Set toBeVisited;

    Uint32 currentTime = ClockSource::getCurrentTime();

    //iterate through old graph edges , if a device already  has and its backup is still a valid device, don't change it
    for ( DoubleExitEdges::iterator i = graphEdges.begin(); i != graphEdges.end(); ++i) {

        Device *device = subnet->getDevice(i->first);
        if(!device) {
            continue;
        }

        bool exists = false;
        exists = subnet->isDeviceVisited(i->first);// if device is visited, it already has redundancy
        LOG_DEBUG("device =" << std::hex << i->first);
        if (!exists) {
            if(gp->deviceHasDoubleExit(i->first)) {
            	//if device has redundancy from the previous execution of doubleExit algorithm..and its backup is still valid...add device to solution and mark it as visited
                if( subnet->existsEdge(i->first, i->second.prefered )
                		&& subnet->existsEdge(i->first, i->second.backup )
                		&& subnet->isDeviceVisited( i->second.prefered)
                		&& subnet->isDeviceVisited( i->second.backup)
                		&& !device->theoAttributes.candidateIsBadRate(i->second.backup)) {
                    subnet->setDeviceVisited(i->first); //mark device as visited
                    LOG_DEBUG("mark as visited" << i->first);
//                    ++targetsNo;
                    Path path(i->second.prefered,  i->second.backup, 1);
                    DoubleExitPaths.insert(std::make_pair(i->first, path));//add device and its parents to solution
                    LOG_DEBUG("path insert" << i->first << i->second.prefered << "," << i->second.backup);
                }
                else { //if here..device had backup in the previous execution of doubleExit algorithm but it's backup is not a valid device anymore
                    toBeVisited.insert(i->first);// add device to the list of device to be visited in order to search new redundancy
                    LOG_DEBUG("to be visited insert" << i->first);
                }
            } else {// if here...device has no  redundancy from the previous execution of doubleExit algorithm.
                NE::Common::SubnetSettings subnetSettings = subnet->getSubnetSettings();
                //if delayActivateMultiPathForDevice seconds have passed from device's join confirmation , add the device to the list of devices to search redundancy
                if(device && (currentTime - device->joinConfirmTime   >= subnetSettings.delayActivateMultiPathForDevice)) {
                    toBeVisited.insert(i->first);
                    LOG_DEBUG("to be visited insert" << i->first);
                }
            }
        }
    }

    bool added = true;
    //the while loop executes as long as there are target devices(targetsNo > 0) && in the last loop iteration there was not found redundancy for any device(added)
    //at first loop iteration, targetNo represents number of level one devices
    while ((targetsNo > 0) && (added)) {
        float min = MAX_FLOAT;
        Address16 winner = 0;
        Address16 winnerP = 0;
        Address16 winnerS = 0;
        for (Address16Set::iterator it = toBeVisited.begin(); it != toBeVisited.end(); it++) {
            Address16 tmpP = 0;
            Address16 tmpS = 0;
            if (HasDoubleExit(subnet, *it, tmpP, tmpS, nodesSkippedForBackup)) {
            	//if here...device *it has redundant path, tmpP and tmpS contain the addresses of device's preferred and secondary inbound paths
                float cost = MAX_FLOAT;
                //calculate the cost of this solution
                cost = GetParents(subnet, *it,  tmpP, tmpS);
                //if this solution is better than previous one, update minimum cost with the  cost of this solution
                if (cost < min) {
                    min = cost;
                    winner = *it;
                    winnerP = tmpP;
                    winnerS = tmpS;
                }
            }
        }

        //if min was changed( < MAX_FLOAT) => there is a new solution
        if (min < MAX_FLOAT && winner && winnerP) {
        	//set device as visited as long it has redundant path
            subnet->setDeviceVisited(winner);
            //increment the number of target devices(by adding device to the solution, device becomes a possible target device)
            ++targetsNo;
            LOG_DEBUG("added" << std::hex << winner << "to racordList");
            Path path(winnerP, winnerS, min);
            DoubleExitPaths.insert(std::make_pair(winner, path));
            //erase device from list of devices without redundancy
            toBeVisited.erase(winner);
            nodesSkippedForBackup.erase(winner);//if device has backup remove it from the skipped list (if was there)
            LOG_INFO("erase from nodesSkippedForBackup" << std::hex << winner );

            Device* winnerDevice = subnet->getDevice(winner);
            if(winnerDevice && winnerS) {
                Device* backup = subnet->getDevice(winnerS);
                if(backup && ((winnerDevice->parent32 != backup->address32) &&  !subnet->isBackupForDevice(winner, winnerS))) {
                    backup->theoAttributes.isSelectedAsBackup = true;
                    LOG_DEBUG("Candidate is selected as backup " << std::hex << winnerS << "by device" << winner);
                }
            }
            //DeleteVertexesWithoutOffsprings(subnet, toBeVisited);
        } else {
            added = false;
        }
    }
}

void DoubleExit::DeleteVertexesWithoutOffsprings(Subnet::PTR subnet, Address16Set &toBeVisited) {
	Address16List targetsList;
    subnet->getDevicesVisited(targetsList);
    Address16List::iterator i =  targetsList.begin();
    for (;i != targetsList.end(); ++i) {
        bool toRemove = true;
        for (Address16Set::iterator it = toBeVisited.begin(); it != toBeVisited.end(); it++) {
            if (subnet->existsEdge(*it,  *i)) {
                toRemove = false;
            }
        }

        if (toRemove) {
            subnet->unVisitedDevice(*i);
            LOG_DEBUG("remove " << *i);
            --targetsNo;
        }

    }

}

bool DoubleExit::HasDoubleExit(Subnet::PTR subnet, Address16 vertexA, Address16 &tmpP, Address16 &tmpS, Address16Set& nodesSkippedForBackup) {
    Device* dvc = subnet->getDevice(vertexA);
    if (!dvc) {
        return false;
    }
    Device* parent = subnet->getDevice(dvc->parent32);

    if(!parent) {
        return false;
    }

    if(!parent->isAlreadyVisited()) {
        return false;
    }

    tmpP = Address::getAddress16(parent->address32);

    bool hasCandidate = false;

    //iterate through device's candidates list to search for a backup
    for(Uint8 i = 0; i < DEFAULT_MAX_NO_CANDIDATES; ++i)  {
        if((candidateIsVisited(subnet, dvc,dvc->theoAttributes.candidates[i]))  &&
                    (tmpP != dvc->theoAttributes.candidates[i])) {
            EdgePointer edge = subnet->getEdge(vertexA, dvc->theoAttributes.candidates[i]);
            if(edge) {
                std::map<Address16, Path>::iterator exists= DoubleExitPaths.find(dvc->theoAttributes.candidates[i]);
                if((exists != DoubleExitPaths.end()) && ((exists->second.NodeMainPath == vertexA) || (exists->second.NodeSecondaryPath == vertexA))) {
                    continue;
                }
                else {
                    Device* candidate = subnet->getDevice(dvc->theoAttributes.candidates[i]);
                    if(candidate ) {//if candidate is a valid device && was not selected as backup by another device in the current iteration then, select it as backup
                        if((Address::getAddress16(candidate->parent32) != vertexA)
                                  && subnet->candidateIsEligibleAsBackup(vertexA, dvc->theoAttributes.candidates[i]))
                        	if(!candidate->theoAttributes.isSelectedAsBackup){
							  hasCandidate = true;
							  tmpS = dvc->theoAttributes.candidates[i];
                        	}
                        	else {
                             LOG_INFO("add to nodesSkippedForBackup" << std::hex << vertexA << "due to" << std::hex << dvc->theoAttributes.candidates[i]);
                             nodesSkippedForBackup.insert(vertexA);
                         }
                    }
                }
            }
         }

        if (hasCandidate) {
            return true;
        }
    }
    //if here means no good candidate was found: accelerate discovery
    subnet->listDiscoveryToBeAccelerated.push_back(Address::getAddress16(dvc->address32));
    return false;
}


float DoubleExit::GetParents(Subnet::PTR subnet, Address16 vertexA, Address16 tmpP, Address16 tmpS) {
    EdgePointer edgeP = subnet->getEdge(vertexA, tmpP);
    EdgePointer edgeS = subnet->getEdge(vertexA, tmpS);
    Device* prefered = subnet->getDevice(tmpP);
    Device* backup = subnet->getDevice(tmpS);

    if(edgeP && edgeS && prefered && backup) {
        Uint8 k1factor = subnet->getSubnetSettings().k1factorOnEdgeCost;
        Uint16 childsP = subnet->getNumberOfChildren(tmpP);
        Uint16 childsS = subnet->getNumberOfChildren(tmpS);
        return( (1 - failureProb) * edgeP->getEvalEdgeCost(k1factor, childsP) + failureProb * edgeS->getEvalEdgeCost(k1factor, childsS));
    }

    return  MAX_FLOAT;

}

void DoubleExit::PrintVertexesWithDoubleExit() {
    LOG_DEBUG("NODES WITH DOUBLE EXIT: ");
    for (std::map<Address16, Path>::iterator i = DoubleExitPaths.begin(); i != DoubleExitPaths.end(); ++i) {
        LOG_DEBUG(std::hex << (int)i->first << "--> cale P=" << (int)i->second.NodeMainPath
                    << ", Cale S= " << (int)i->second.NodeSecondaryPath << ", COST = " << i->second.Cost);
    }
}


void DoubleExit::GetNodesWithoutDoubleExit(Subnet::PTR subnet, Uint16 graphID, const Address16Set &targetList, Address16Set &nodes) {
    GraphPointer gp = subnet->getGraph(graphID);
    Address16Set  nodesFromGraph;

    gp->getDevicesFromGraph(nodesFromGraph);

	for (Address16Set::iterator i = nodesFromGraph.begin(); i != nodesFromGraph.end(); i++) {
        std::map<Address16, Path>::iterator exists = DoubleExitPaths.find(*i);
        if (exists != DoubleExitPaths.end()) {
            continue;
        } else {
            Address16Set::const_iterator noTarget = targetList.find(*i);
            if (subnet->isDeviceVisited(*i) || subnet->isBackboneDevice(*i)){
                continue;
            } else {
                if (noTarget == targetList.end()) {
                    Device* nodeDevice = subnet->getDevice(*i);
                    if(nodeDevice) {
                        nodes.insert(*i);
                    }
                }
            }
        }
    }
}

void DoubleExit::GetUsedEdges(Subnet::PTR subnet, DoubleExitEdges &usedEdges) {
    for (std::map<Address16, Path>::iterator i = DoubleExitPaths.begin(); i != DoubleExitPaths.end(); i++) {
        DoubleExitDestinations destination;
        LOG_DEBUG("add as prefered " << i->second.NodeMainPath << ", add as backup " << i->second.NodeSecondaryPath);
        destination.prefered =  i->second.NodeMainPath;
        destination.backup =  i->second.NodeSecondaryPath;
        usedEdges.insert(std::make_pair(i->first, destination));
        subnet->setDeviceVisited(i->first);
    }
}

bool DoubleExit::candidateIsVisited(Subnet::PTR subnet, Device* dvc, Address16 candidate) {
    if((subnet->isDeviceVisited(candidate)) &&
                (dvc->hasCandidate(candidate)) &&
                (subnet->isRouter(candidate) || subnet->isBackboneDevice(candidate)) &&
                !dvc->theoAttributes.candidateIsBadRate(candidate)) {
        return true;
    }

    return false;
}
}
}
}
}
