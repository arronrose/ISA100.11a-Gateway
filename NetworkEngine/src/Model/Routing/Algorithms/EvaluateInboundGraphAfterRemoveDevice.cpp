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
 * EvaluateInboundGraphAfterRemoveDevice.cpp
 *
 *  Created on: Mar 3, 2010
 *      Author: flori.parauan, mulderul(Catalin Pop), beniamin.tecar
 */


#include "Common/NETypes.h"
#include "Model/IEngine.h"
#include "EvaluateInboundGraphAfterRemoveDevice.h"

namespace NE {

namespace Model {

namespace Routing {

namespace Algorithms {

void EvaluateInboundGraphAfterRemoveDevice::evaluateGraph(Subnet::PTR subnet, Uint16 graphId, Address32 destination,
                DoubleExitEdges &usedEdges) {

    GraphPointer graph = subnet->getGraph(graphId);
    if(!graph) {
        LOG_INFO("GRAPH is NULL :" << (int) graphId);
        return;
    }

    DoubleExitEdges graphEdges = graph->getGraphEdges();

    for(DoubleExitEdges::iterator itEdges = graphEdges.begin(); itEdges != graphEdges.end(); ++itEdges) {
        Address16Set::iterator isRemovedDevice = graph->removedDevices.find(itEdges->first);
        if(isRemovedDevice == graph->removedDevices.end()) {
            bool hasChanged = false;
            if(itEdges->second.prefered) {
                isRemovedDevice = graph->removedDevices.find(itEdges->second.prefered);
                if(isRemovedDevice != graph->removedDevices.end()) {
                    hasChanged = true;
                }
            }

            if(itEdges->second.backup) {
                isRemovedDevice = graph->removedDevices.find(itEdges->second.backup);
                if(isRemovedDevice != graph->removedDevices.end()) {
                    hasChanged = true;
                }
            }

            if(hasChanged) {
                Device* device = subnet->getDevice(itEdges->first);
                if(!device) {
                    continue;
                }
                DoubleExitDestinations destinations;
                destinations.prefered = Address::getAddress16(device->parent32);
                Address16Set::iterator isDevToRemove = graph->removedDevices.find(destinations.prefered);
                destinations.backup = 0;
                if(destinations.prefered && (isDevToRemove == graph->removedDevices.end())) {
                    usedEdges.insert(std::make_pair(itEdges->first, destinations));
                    if(itEdges->second.prefered != destinations.prefered) {
                        //device has changed parent with backup
                        EdgePointer edge = subnet->getEdge(itEdges->first,  destinations.prefered);
                        if(edge) {
                            edge->resetEdgeDiagnosticsOnChangeParent();
                        }
                    }
                }
            }

            else {
                usedEdges.insert(std::make_pair(itEdges->first, itEdges->second));
            }
        } else {
            if(graph->rejoinedDevices.find(itEdges->first) != graph->rejoinedDevices.end()) {
                Device* device = subnet->getDevice(itEdges->first);
                if(device) {
                    DoubleExitDestinations destinations;
                    destinations.prefered = Address::getAddress16(device->parent32);
                    destinations.backup = 0;
                    usedEdges.insert(std::make_pair(itEdges->first, destinations));
                }
            }
        }

    }

    graph->removedDevices.clear();
    graph->rejoinedDevices.clear();


}

}
}
}
}
