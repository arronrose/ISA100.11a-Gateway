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

#ifndef ROUTINGGRAPHTYPES_H_
#define ROUTINGGRAPHTYPES_H_

#include "boost/shared_ptr.hpp"
#include "Common/NETypes.h"
#include <map>
//#include <pair>

namespace NE {

namespace Model {

namespace Routing {


namespace RoutingTypes {
/**
 * The routing type.
 */
enum RoutingTypes_Enum {
    SOURCE_ROUTING = 0,
    GRAPH_ROUTING = 1,
};
}


namespace RouteOption {
/**
 * The route option
 */
enum RouteOption {
    UseBbrContractID = 0, //
    UseContractID = 1, // UseContractID
    UseDestinationAddress = 2,// UseDestinationAddress
    UseDefaultRoute = 3 //  UseDefaultRoute

};
}

namespace GraphEdgeStatus {
/**
 * Enumeration with possible edge status on the graph route.
 */
enum GraphEdgeStatus_Enum {
    ACTIVE_IN_GRAPH_EDGE = 0, // this is an edge that is active in graph
    DELETE_IN_GRAPH_EDGE = 1, // this is marked as removed in graph
    NEW_IN_GRAPH_EDGE = 2, // this is marked as new added edge
    CHANGED_IN_GRAPH_EDGE = 3, //this is marked as changed
    CANDIDATE_IN_GRAPH_EDGE  =4
};
}

namespace EvalGraphEdgeStatus {
/**
 * Enumeration with possible edge status on the evaluation of graph route.
 */
enum EvalGraphEdgeStatus_Enum {
    SELECTED_IN_GRAPH_EDGE = 0, // the edge is selected for graph but currently is not used in the graph routing
    USED_IN_GRAPH_EDGE = 1, // the edge is selected and used in graph routing
    DELETED_IN_GRAPH_EDGE = 2 // the edge is not selected for graph but was is not used in the graph routing
};
}

class Edge;
typedef boost::shared_ptr<Edge> EdgePointer;

}

}

}

#endif /* ROUTINGGRAPHTYPES_H_ */
