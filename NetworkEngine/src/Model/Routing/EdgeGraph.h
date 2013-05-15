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
 * EdgeGraph.h
 *
 *  Created on: May 11, 2009
 *      Author: ioanpocol
 */

#ifndef EDGEGRAPH_H_
#define EDGEGRAPH_H_

namespace NE {

namespace Model {

namespace Routing {

struct EdgeGraph {
        GraphEdgeStatus::GraphEdgeStatus_Enum status;
        Uint16 trafficProbability;
        bool retryEdge;
        bool prefferedEdge;

        EdgeGraph() {
            status = GraphEdgeStatus::NEW_IN_GRAPH_EDGE;
            trafficProbability = 100;
            retryEdge = false;
            prefferedEdge = false;
        }

        /**
         * Returns a string representation of this EdgeGraph.
         */
        friend std::ostream& operator<<(std::ostream& stream, const EdgeGraph& edgeGraph) {
            stream << "status=" << edgeGraph.status << ", trafficProbability=" << (int) edgeGraph.trafficProbability
				<< ", retryEdge=" << edgeGraph.retryEdge << ", prefferedEdge=" << edgeGraph.prefferedEdge;
            return stream;
        }

};

}
}
}

#endif /* EDGEGRAPH_H_ */
