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
 * ChainWaitForConfirmOnEvalGraph.cpp
 *
 *  Created on: Feb 24, 2010
 *      Author: flori.parauan
 */

#include "ChainWaitForConfirmOnEvalGraph.h"

namespace NE {

namespace Model {


ChainWaitForConfirmOnEvalGraph::ChainWaitForConfirmOnEvalGraph(Subnet::PTR subnet_, Uint16 graphId_)
:subnet(subnet_), graphId(graphId_){
}

ChainWaitForConfirmOnEvalGraph::~ChainWaitForConfirmOnEvalGraph() {
}

void ChainWaitForConfirmOnEvalGraph::process(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status){
    if(graphId == DEFAULT_GRAPH_ID) {
        LOG_DEBUG("Default graph 1 evaluated and operations confirmed.");
        LOG_DEBUG("GRAPH EVAL enable in ChainWaitForConfirmOnEvalGraph");
        subnet->enableInboundGraphEvaluation();
    }
    else {
        LOG_DEBUG("Graph evaluatation and operations confirmed for graph" << graphId);
        LOG_DEBUG("GRAPH EVAL enable in ChainWaitForConfirmOnEvalGraph");
        subnet->enableOutBoundGraphEvaluation();
    }
}

}
}
