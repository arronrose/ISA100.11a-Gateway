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

#include "RouteEngine.h"
#include "NetworkEngine.h"
#include "modelDefault.h"


namespace NE {
namespace Model {

using namespace NE::Model::Tdma;

RouteEngine::RouteEngine() {
    graphRoutingInboundAlgorithm = AlgorithmPointer(new Algorithms::DoubleExitMultiSinkGraphRouting());
    graphRoutingAlgorithmRemoveDevices =  AlgorithmPointer(new Algorithms::DoubleExitMultiSinkGraphRouting());
    graphRoutingOutboundAlgorithm = AlgorithmPointer(new Algorithms::OutboundAlgorithm());
}

RouteEngine::~RouteEngine() {
}

void RouteEngine::periodicEvaluateInboundGraph(Subnet::PTR subnet,
                                       GraphPointer & inBoundGraph,
                                       Address32 destination,
                                       DoubleExitEdges &usedEdges,
                                       DoubleExitEdges &notUsedEdges) {
    if(!inBoundGraph ) {
        return;
    }

    if(inBoundGraph->removedDevices.empty()) {
    	if (graphRoutingInboundAlgorithm) {
    		graphRoutingInboundAlgorithm->evaluateGraph(subnet, inBoundGraph->getGraphId(), destination, usedEdges);
    	}
    }
    else {//if there are removed devices on Error...don't look for backups..only update de inbound graph
    	if (graphRoutingAlgorithmRemoveDevices) {
    		graphRoutingAlgorithmRemoveDevices->evaluateGraph(subnet, inBoundGraph->getGraphId(), destination, usedEdges);;
    	}
    }

    detectNotUsedEdges(subnet, inBoundGraph->getGraphEdges(), usedEdges, notUsedEdges);
}

void RouteEngine::periodicEvaluateOutBoundGraph(Subnet::PTR subnet,
                                               GraphPointer & outBoundGraph,
                                               Address32 destination,
                                               DoubleExitEdges &usedEdges,
                                               DoubleExitEdges &notUsedEdges) {
	if (graphRoutingOutboundAlgorithm) {
		graphRoutingOutboundAlgorithm->evaluateGraph( subnet, outBoundGraph->getGraphId(), destination, usedEdges);
	}

    detectNotUsedEdges(subnet, outBoundGraph->getGraphEdges(), usedEdges, notUsedEdges);
}



void RouteEngine::detectNotUsedEdges(Subnet::PTR subnet,
                                     const DoubleExitEdges & edgesFromGraph,
                                     const DoubleExitEdges & usedEdges,
                                     DoubleExitEdges &notUsedEdges
                                     ) {

    for(DoubleExitEdges::const_iterator it = edgesFromGraph.begin(); it != edgesFromGraph.end(); ++it) {
        DoubleExitEdges::const_iterator itUsedEdges = usedEdges.find(it->first);
        if( itUsedEdges == usedEdges.end()) {
            notUsedEdges.insert(std::make_pair(it->first, it->second));
        }
        else {
            DoubleExitDestinations destination(0,0);
            if(     (it->second.prefered != itUsedEdges->second.prefered)
                &&  (it->second.prefered != itUsedEdges->second.backup)) {

                    destination.prefered = it->second.prefered ; // mark for remove the old prefered
            }

            if(     (it->second.backup != itUsedEdges->second.prefered)
                &&  (it->second.backup != itUsedEdges->second.backup)) { // mark for remove the old backup

                if(destination.prefered == 0) {
                    destination.prefered = it->second.backup ;
                }
                else {
                    destination.backup = it->second.backup ;
                }
            }

            if(destination.prefered != 0) {
                notUsedEdges.insert(std::make_pair(it->first, destination));
            }
        }

    }
}



}
}
