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
 * OutboundAlgorithm.cpp
 *
 *  Created on: Feb 18, 2010
 *      Author: flori.parauan, mulderul(Catalin Pop), beniamin.tecar
 */

#include "Common/NETypes.h"
#include "Model/IEngine.h"
#include "OutboundAlgorithm.h"

namespace NE {

namespace Model {

namespace Routing {

namespace Algorithms {

void OutboundAlgorithm::evaluateGraph(Subnet::PTR subnet, Uint16 graphId, Address32 destination,
                DoubleExitEdges &usedEdges) {



    GraphPointer outBoundGraph = subnet->getGraph(graphId);

    GraphPointer graph = subnet->getGraph(graphId);
    if( !graph) {
        LOG_ERROR("evaluateGraph: in subnet:" << (int)subnet->getSubnetId() << " doesn't exists graph " << (int)graphId);
        return ; // was evaluated
    }

    LOG_DEBUG("OutboundAlgorithm::evaluateOutBoundGraph : ( outbound graphID =" << outBoundGraph->getGraphId() << ")");

    GraphPointer inBoundGraph = subnet->getGraph(DEFAULT_GRAPH_ID);
    RETURN_ON_NULL_MSG(inBoundGraph, "Inbound graph is NULL");

    subnet->markAllDevicesUnVisited();
    OutboundGraphMap outboundGraph;
    Device* backbone = subnet->getBackbone();
    if(!backbone) {
        return;
    }

    if(inBoundGraph->deviceHasDoubleExit(graph->getDestination())) {
        DoubleExitDestinations edges(0,0);
        inBoundGraph->getEdgesForDevice(graph->getDestination(),  edges);
        nodeCost.insert(std::make_pair(graph->getDestination(), -1));
        std::list<NodeLevel> listOutbound;
        NodeLevel destinationNode(graph->getDestination(), -1);
        listOutbound.push_back(destinationNode);
        outboundGraph.insert(std::make_pair(edges.prefered,listOutbound));
        outboundGraph.insert(std::make_pair(edges.backup,listOutbound));
        subnet->setDeviceVisited(graph->getDestination());

        reverseEdge(subnet, outboundGraph,  inBoundGraph->getGraphEdges(),edges.prefered, edges.prefered, edges.prefered );
        reverseEdge(subnet, outboundGraph,  inBoundGraph->getGraphEdges(), edges.backup, edges.backup, edges.backup );
    }
    else {
        reverseEdge(subnet, outboundGraph,  inBoundGraph->getGraphEdges(), graph->getDestination(), destination, destination );
    }

    for(OutboundGraphMap::iterator itEdges = outboundGraph.begin(); itEdges != outboundGraph.end(); ++itEdges) {
        for(std::list<NodeLevel>::iterator it =  itEdges->second.begin(); it != itEdges->second.end(); ++it) {
            std::map<Address16, Int16>::iterator existsSource = nodeCost.find(it->nodeAddress);
            if(existsSource != nodeCost.end() && existsSource->second < it->level) {
                it->level = existsSource->second;
            }
        }
        itEdges->second.sort(nodeLevelCompareFunction());
    }

    subnet->markAllDevicesUnVisited();
    LOG_DEBUG("BEFORE CREATE OUTBOUND GRAPH");
    for(OutboundGraphMap::iterator itEdges = outboundGraph.begin(); itEdges != outboundGraph.end(); ++itEdges) {

         for(std::list<NodeLevel>::iterator it =  itEdges->second.begin(); it != itEdges->second.end(); ++it) {
             LOG_DEBUG("(" << std::hex << (int)itEdges->first << "-->" << std::hex <<(int)it->nodeAddress << ","<<(int)it->level <<")");
         }
    }

    createOutboundGraph(subnet, outboundGraph, usedEdges, Address::getAddress16(backbone->address32), destination);
    subnet->markAllDevicesUnVisited();

    LOG_DEBUG("USED EDGES on OUTBOUND GRAPH EVALUATION graphId=" << outBoundGraph->getGraphId() << " destination device =" << std::hex << destination);

    for(DoubleExitEdges::iterator it = usedEdges.begin(); it != usedEdges.end(); ++it) {
        LOG_DEBUG(std::hex << it->first << "-->" << std::hex << it->second.prefered << "," << std::hex << it->second.backup << std::endl);
    }


    nodeCost.clear();
}


void OutboundAlgorithm::reverseEdge( Subnet::PTR subnet, OutboundGraphMap &outboundGraph,
                              const DoubleExitEdges &usedEdgesInBound,
                              Address16 graphDestination, Address16 iterationSource, Address16 iterationDestination) {
    if(0 == iterationDestination){
        return;
    }

    Device* device = subnet->getDevice(iterationDestination);

    if (!device){
        return;
    }

    if(device->isAlreadyVisited() && (iterationSource != iterationDestination)) {
        std::map<Address16, Int16>::iterator existsSource = nodeCost.find(iterationSource);
        std::map<Address16, Int16>::iterator existsDestination = nodeCost.find(iterationDestination);
        if(existsSource != nodeCost.end() && existsDestination !=  nodeCost.end()) {
            if(existsDestination->second > (existsSource->second +1)) {
                existsDestination->second = existsSource->second + 1;

            }

            return;
        }
        return;
    }

    device->setVisited();

    DoubleExitEdges::const_iterator itUsedEdgesInbound = usedEdgesInBound.find(iterationDestination);

    if( itUsedEdgesInbound == usedEdgesInBound.end()) {
        return;
    }

    Int8 destinationLevel = 0;
    if(iterationDestination != graphDestination) {
        std::map<Address16, Int16>::iterator existsSource = nodeCost.find(iterationSource);
        std::map<Address16, Int16>::iterator existsDestination = nodeCost.find(iterationDestination);
        if(existsSource != nodeCost.end()) {
            destinationLevel = existsSource->second + 1;
        }

        if(existsDestination != nodeCost.end()) {
            existsDestination->second = destinationLevel;
        }
        else {
            nodeCost.insert(std::make_pair(iterationDestination, destinationLevel));
        }

    }
    else {
        nodeCost.insert(std::make_pair(iterationDestination, destinationLevel));
    }


    NodeLevel destinationNode(iterationDestination, destinationLevel);

    if(itUsedEdgesInbound->second.prefered) {
        OutboundGraphMap::iterator existsPrefered =  outboundGraph.find(itUsedEdgesInbound->second.prefered);
        if(existsPrefered != outboundGraph.end()) {

            existsPrefered->second.push_back(destinationNode);
        }
        else {
            std::list<NodeLevel> listOutbound;

            listOutbound.push_back(destinationNode);
            outboundGraph.insert(std::make_pair(itUsedEdgesInbound->second.prefered,listOutbound));
        }

        reverseEdge(subnet, outboundGraph,   usedEdgesInBound, graphDestination, iterationDestination, itUsedEdgesInbound->second.prefered );
    }

    if (itUsedEdgesInbound->second.backup) {
        OutboundGraphMap::iterator existsBackup=  outboundGraph.find(itUsedEdgesInbound->second.backup);
        if(existsBackup != outboundGraph.end()) {
            existsBackup->second.push_back(destinationNode);
        }
        else {
            std::list<NodeLevel> listOutbound;
            listOutbound.push_back(destinationNode);
            outboundGraph.insert(std::make_pair(itUsedEdgesInbound->second.backup,listOutbound));
        }

        reverseEdge(subnet, outboundGraph,   usedEdgesInBound, graphDestination, iterationDestination, itUsedEdgesInbound->second.backup);
    }

}

void  OutboundAlgorithm::createOutboundGraph(Subnet::PTR subnet, OutboundGraphMap &outboundGraph,
            DoubleExitEdges &usedEdges,
            Address16 source,
           Address16 destination) {

    if((!destination) || (!source) || (source == destination) ){
         return;
     }

     Device* sourceDevice = subnet->getDevice(source);

     if (!sourceDevice){
         return;
     }

     if(sourceDevice->isAlreadyVisited()) {
         return;
     }

     sourceDevice->setVisited();

    OutboundGraphMap::iterator existsSource = outboundGraph.find(source);

    if (existsSource == outboundGraph.end()) {
        return;
    }

    if(existsSource->second.empty()) {
        return;
    }

    Address16 prefered = 0;
    Address16 backup = 0;

    std::list<NodeLevel>::iterator itNode = existsSource->second.begin();
    prefered = itNode->nodeAddress;

    if( existsSource->second.size() > 1) {
        ++itNode;
        if(itNode != existsSource->second.end())   {
        	if (itNode->nodeAddress != prefered) {
				if(itNode->nodeAddress == destination) {
					backup = prefered;
					prefered = itNode->nodeAddress;
				}
				else {
					backup = itNode->nodeAddress;
				}
        	}
            ++itNode;
            while (itNode != existsSource->second.end() ) {
                if(itNode->nodeAddress == destination && itNode->nodeAddress != prefered)   {
                    backup = prefered;
                    prefered = itNode->nodeAddress;
                }
                ++itNode;
            }
        }
    }




    DoubleExitDestinations destinations(prefered,backup);
    DoubleExitEdges::iterator existsEdge = usedEdges.find(source);
    if(existsEdge != usedEdges.end()) {
        existsEdge->second = destinations;
    }
    else {
        usedEdges.insert(std::make_pair(source, destinations));
    }


    if(prefered) {
        createOutboundGraph(subnet, outboundGraph,
                   usedEdges, prefered, destination);
    }

    if (backup) {
        createOutboundGraph(subnet, outboundGraph,
                   usedEdges, backup, destination);
    }



}

}
}
}
}
