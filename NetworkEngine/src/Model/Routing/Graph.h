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
 * Graph.h
 *
 *  Created on: Sep 10, 2009
 *      Author: Flori Parauan
 */

#ifndef GRAPH_H_
#define GRAPH_H_

#include <boost/shared_ptr.hpp>
#include <list>
#include <map>
#include <set>
#include "Common/NEAddress.h"
#include "Edge.h"

namespace NE {

namespace Model {

namespace Routing {


struct DoubleExitDestinations{
    Address16 prefered;
    Address16 backup;


    DoubleExitDestinations():
                prefered(0),
                backup(0) {}

    DoubleExitDestinations(Address16 _prefered, Address16 _backup):
        prefered(_prefered),
        backup(_backup){}

    bool operator == (const DoubleExitDestinations& dest) const {
        return ((dest.prefered == prefered) && (dest.backup == backup));
    }

    /**
     * Returns a string representation of this DoubleExitDestinations.
     */
    friend std::ostream& operator<<(std::ostream& stream, const DoubleExitDestinations& doubleExitDestinations) {
        stream << "p=" << std::hex << (int)doubleExitDestinations.prefered
            << ", b=" << (int)doubleExitDestinations.backup;
        return stream;
    }
};



//typedef boost::unordered_map<Address16, DoubleExitDestinations> DoubleExitEdges;
typedef std::map<Address16, DoubleExitDestinations> DoubleExitEdges;


//typedef boost::unordered_set<Edge32> EdgeList ;
typedef std::set<Edge32> EdgeList ;
//typedef boost::unordered_map<Address16, Address16> EdgesMap ;
typedef std::map<Address16, Address16> EdgesMap ;




inline
Address16 getEdgeSource(Edge32 edge) {
    return (edge >> 16);
}


inline
Address16 getEdgeTarget(Edge32 edge) {
    return (edge);
}

typedef unsigned int Edge32;



class Graph {

        LOG_DEF("I.M.G.Graph");
	private:

		Uint16 graphID;
		Address16 destination; // is backbone's address in case of inbound graph..and destination device's address in case of outbound

		DoubleExitEdges edges;

	public:

	    Address16Set removedDevices;
	    Address16Set rejoinedDevices;

		Graph();

	    Graph(Uint16 graphId, Address16 _destination):graphID(graphId), destination(_destination) {
	    }

	    void getDevicesFromGraph(NE::Common::Address16Set &_devices) {
           for(DoubleExitEdges::iterator it = edges.begin(); it != edges.end(); ++it) {
               _devices.insert(it->first);
            }
           _devices.insert(destination);
		}


	    const DoubleExitEdges& getGraphEdges() const{
	        return edges;
		}

	    Address16 getDestination() const {
	        return destination;
	    }

		void setGraphEdges( const DoubleExitEdges &edgeList) {
//		    edges.clear();//By Cata not needed. The assignment below do a replace
		    edges = edgeList;
		}

       Uint16 getGraphId() const {
            return graphID;
        }

		void setGraphId(Uint16 graphId) {
		    graphID = graphId;
		}

		void addEdges(Address16 source,  DoubleExitDestinations &destination);

		void addEdge(Address16 source, Address16 destination);

		bool existsSourceDevice(Address16 src);

		void removeDeviceFromGraph(Address16 deviceToRemove);

		void getOutBoundEdges(Address16 device, NE::Common::Address16Set& outEdgesTargets);

		void getInBoundEdges(Address16 device, NE::Common::Address16Set& inEdgesSources);

		bool deviceHasInBoundEdges(Address16 device);

		Address16 getBackupFor(Address16 device);

		void getEdgesForDevice(Address16 device, DoubleExitDestinations &destinations);

		bool deviceHasDoubleExit(Address16 device);

		bool deviceHasDoubleExit(Address16 device, Address16 parent);

		void getDevicesWithoutDoubleExit(Address16Set& devices);

		bool graphPasesThroughNode(Address16 adr);

		void getSourceRouteToNode(Address16 adr, std::list<Address16> &routeNodes, bool toRevert);

		bool isBackupForDevice(Address16 device, Address16 backup);

		void setDeviceDestination(Address16 device, DoubleExitDestinations &destinations);

		bool existsEdge(Address16 src, Address16 dst);

};


typedef boost::shared_ptr<Graph> GraphPointer;

struct NodeLevel {
    Address16 nodeAddress;
    Int8 level;
    NodeLevel() {
        nodeAddress = 0;
        level = 100;
    }

    NodeLevel(Address16 _address, Int8  _level):
        nodeAddress(_address), level(_level)
        {}
};

struct nodeLevelCompareFunction {
bool operator()(const NodeLevel& first, const NodeLevel& second)
{
return first.level < second.level;
}
};

typedef std::map<Address16, std::list<NodeLevel> > OutboundGraphMap;




inline
Address32 getEdgeSource32(Edge32 edge, Uint16 subnetId) {
    Address32 source32 = subnetId;
    source32  = (source32 << 16) + getEdgeSource(edge);
    return  source32;
}

inline
Address32 getEdgeTarget32(Edge32 edge, Uint16 subnetId) {
    Address32 source32 = subnetId;
    source32  = (source32 << 16) + getEdgeTarget(edge);
    return  source32;
}


};
};
};



#endif /* GRAPH_H_ */
