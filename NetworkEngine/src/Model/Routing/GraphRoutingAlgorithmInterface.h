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
 * GraphRoutingAlgorithmInterface.h
 *
 *  Created on: 17.08.2009
 *      Author: radu.pop
 */

#ifndef GRAPHROUTINGALGORITHMINTERFACE_H_
#define GRAPHROUTINGALGORITHMINTERFACE_H_

#include "Model/Routing/Edge.h"
#include "Model/Subnet.h"

namespace NE {

namespace Model {

namespace Routing {


/**
 * This interface specifies the interface for algorithms that determines the routing graphs from
 * a source to a destination. These algorithms work directly on the NetworkEngine model.
 */
class GraphRoutingAlgorithmInterface {

    public:

        GraphRoutingAlgorithmInterface() {
        }

        virtual ~GraphRoutingAlgorithmInterface() {
        }

        /**
         * Select the nodes&edges that are potential part of the inbound graph.
         */
        virtual void evaluateGraph(Subnet::PTR subnet, Uint16 graphID, Address32 destination,
                    DoubleExitEdges &usedEdges) = 0;



};

typedef boost::shared_ptr<GraphRoutingAlgorithmInterface> AlgorithmPointer;
}

}

}
#endif /* GRAPHROUTINGALGORITHMINTERFACE_H_ */
