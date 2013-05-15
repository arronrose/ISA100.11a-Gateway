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

#ifndef ROUTEENGINE_H_
#define ROUTEENGINE_H_

#include "Common/NETypes.h"
#include "Model/Routing/RouteTypes.h"
#include "Model/Operations/OperationsProcessor.h"
#include "Routing/Algorithms/DoubleExitMultiSinkGraphRouting.h"
#include "Routing/Algorithms/OutboundAlgorithm.h"
#include "Routing/Algorithms/EvaluateInboundGraphAfterRemoveDevice.h"

namespace NE {
namespace Model {


class RouteEngine {

	LOG_DEF("I.M.RouteEngine");
    private:
        /**
         * This is the graph that will be called to optimize an existing route.
         */
        NE::Model::Routing::AlgorithmPointer graphRoutingInboundAlgorithm;
        NE::Model::Routing::AlgorithmPointer graphRoutingAlgorithmRemoveDevices;
        NE::Model::Routing::AlgorithmPointer graphRoutingOutboundAlgorithm;


public:
	RouteEngine();
	virtual ~RouteEngine();

    void periodicEvaluateInboundGraph( Subnet::PTR subnet,
                                       GraphPointer & inBoundGraph,
                                       Address32 destination,
                                       DoubleExitEdges &usedEdges,
                                       DoubleExitEdges &notUsedEdges);


    void periodicEvaluateOutBoundGraph( Subnet::PTR subnet,
                                        GraphPointer & outBoundGraph,
                                        Address32 destination,
                                        DoubleExitEdges &usedEdges,
                                        DoubleExitEdges &notUsedEdges);

    void reverseEdge(   Subnet::PTR subnet,
                        OutboundGraphMap &outboundGraph,
                        const DoubleExitEdges &usedEdgesInBound,
                        Address16 destination);

    void createOutboundGraph(Subnet::PTR subnet, OutboundGraphMap &outboundGraph,
                DoubleExitEdges &usedEdges,
                Address16 source,
               Address16 destination);

    void  recursiveReverseBackupPath(DoubleExitEdges &usedEdges,
                const DoubleExitEdges &usedEdgesInBound,
                Address16 destination);

    void recursiveReversePreferedPath(DoubleExitEdges &usedEdges,
                const DoubleExitEdges &usedEdgesInBound,
                Address16 destination);

    void detectNotUsedEdges(Subnet::PTR subnet,
                            const DoubleExitEdges & edgesFromGraph,
                            const DoubleExitEdges & usedEdges,
                            DoubleExitEdges &notUsedEdges);

    void selectPaths(Subnet::PTR subnet,  DoubleExitEdges &usedEdges, Address16 src, Address16 dst);

    void filterUsedEdges(Subnet::PTR subnet,  DoubleExitEdges &usedEdges);

};

}
}

#endif /* ROUTEENGINE_H_ */
