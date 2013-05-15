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
 * Graph.cpp
 *
 *  Created on: Jan 29, 2010
 *      Author: Catalin Pop(mulderul@y)
 */
#include "Graph.h"

namespace NE {

namespace Model {

namespace Routing {


void Graph::addEdges(Address16 source,  DoubleExitDestinations &destination) {
    if ((source == 0) || (destination.prefered == 0)){
        LOG_ERROR("An Address16=0 was added to graph " << (int)graphID << " skipping operation.");
        return;

    }
    if (destination.prefered == destination.backup){
        LOG_ERROR("Prefered==Backup edge added");
        destination.backup = 0;
    }
    edges[source] = destination;
}



void Graph::addEdge(Address16 source, Address16 destination) {
    DoubleExitEdges::iterator itEdge = edges.find(source);
    if(itEdge == edges.end()) {
        DoubleExitDestinations destinations;
        destinations.prefered = destination;
        destinations.backup = 0;
        edges[source] = destinations;
    }

    else if(itEdge->second.prefered != destination){
        if(0 == itEdge->second.prefered ) {
            itEdge->second.prefered = destination;
            itEdge->second.backup = 0;
        }
        else {
            itEdge->second.backup = destination;
        }
    }
}

bool Graph::existsSourceDevice(Address16 src) {
    DoubleExitEdges::const_iterator itEdge = edges.find(src);
    return (itEdge != edges.end());

}

void Graph::removeDeviceFromGraph(Address16 deviceToRemove) {
    edges.erase(deviceToRemove);
    for(DoubleExitEdges::iterator it = edges.begin(); it != edges.end(); ++it) {
        if(it->second.prefered  == deviceToRemove) {
            it->second.prefered = 0;
        }

        if(it->second.backup  == deviceToRemove) {
            it->second.backup = 0;
        }
    }

}

void Graph::getOutBoundEdges(Address16 device, NE::Common::Address16Set& outEdgesTargets) {
    DoubleExitEdges::const_iterator itEdge = edges.find(device);
    if(itEdge != edges.end()) {
        Address16 peerDevice = itEdge->second.prefered;
        if (peerDevice != 0) {
            outEdgesTargets.insert(peerDevice);
        }

        peerDevice = itEdge->second.backup;
        if (peerDevice != 0) {
            outEdgesTargets.insert(peerDevice);
        }

    }
}

void Graph::getInBoundEdges(Address16 device, NE::Common::Address16Set& inEdgesSources) {
       for(DoubleExitEdges::const_iterator it = edges.begin(); it != edges.end(); ++it)  {
       if ((it->second.prefered == device) || (it->second.backup == device)) {
            inEdgesSources.insert(it->first);
        }
    }
}

bool Graph::deviceHasInBoundEdges(Address16 device) {
    NE::Common::Address16Set inEdgesSources;
    getInBoundEdges(device,inEdgesSources);
    if(inEdgesSources.empty()) {
        return false;
    }

    return true;
}

Address16 Graph::getBackupFor(Address16 device) {
    DoubleExitEdges::const_iterator it = edges.find(device);
    if(it != edges.end()) {
        return it->second.backup;
    }

    return 0;
}

void Graph::getEdgesForDevice(Address16 device, DoubleExitDestinations &destinations) {
    DoubleExitEdges::const_iterator it = edges.find(device);
    if(it != edges.end()) {
        destinations = it->second;
    }
}

bool Graph::deviceHasDoubleExit(Address16 device) {
      DoubleExitEdges::const_iterator it = edges.find(device);
        if(it != edges.end()) {
            return ((it->second.prefered != 0) && (it->second.backup));
        }

        return false;
}



bool Graph::deviceHasDoubleExit(Address16 device, Address16 parent) {
      DoubleExitEdges::const_iterator it = edges.find(device);
        if(it != edges.end()) {
            return ((it->second.prefered != 0) && (it->second.backup) && (it->second.prefered == parent));
        }

        return false;
}

void Graph::getDevicesWithoutDoubleExit(Address16Set& devices) {
    for(DoubleExitEdges::const_iterator it = edges.begin(); it != edges.end(); ++it)  {
        if ((it->second.prefered != 0) && (it->second.backup == 0)) {
            devices.insert(it->first);
         }
    }
}

bool Graph::graphPasesThroughNode(Address16 adr) {
    DoubleExitEdges::const_iterator it = edges.find(adr);
    if(it != edges.end()) {
        if(it->second.prefered != 0) {
            return true;
        }
    }

    return false;
}


void Graph::getSourceRouteToNode(Address16 adr, std::list<Address16> &routeNodes, bool toRevert) {
    DoubleExitEdges::const_iterator it = edges.find(adr);
    if(it == edges.end()) {
        return;
    }

    while(it != edges.end() ) {
        if(it->second.prefered) {
            if(toRevert) {
                LOG_DEBUG("Added source node" << std::hex << it->first);
                routeNodes.push_front(it->first);
            }
            else {
                LOG_DEBUG("Added source node" << std::hex << it->second.prefered);
                routeNodes.push_back(it->second.prefered);
            }
        }
        it = edges.find(it->second.prefered);
    }
}


bool Graph::isBackupForDevice(Address16 device, Address16 backup) {
    DoubleExitEdges::const_iterator it = edges.find(device);
    if(it == edges.end()) {
        return false;
    }

    if(it->second.backup == backup) {
        return true;
    }

    return false;
}

void Graph::setDeviceDestination(Address16 device, DoubleExitDestinations &destinations) {
    DoubleExitEdges::iterator it = edges.find(device);
    if(it == edges.end()) {
        edges.insert(std::make_pair(device, destinations));
        return;
    }
    it->second = destinations;

}

bool Graph::existsEdge(Address16 src, Address16 dst) {
    if(!dst) {
        return false;
    }

    DoubleExitEdges::iterator it = edges.find(src);
    if(it != edges.end()) {
        if ((it->second.prefered == dst) || (it->second.backup = dst)) {
            return true;
        }
    }

    return false;
}


}
}
}
