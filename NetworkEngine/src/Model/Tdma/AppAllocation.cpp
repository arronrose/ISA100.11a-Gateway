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
 *
 *      Author: mulderul@y(Catalin Pop)
 */
#include "AppAllocation.h"
#include <cstdlib>
#include "Model/ModelPrinter.h"
#include "Model/modelDefault.h"
#include "Model/Tdma/_baseAlgorithms.h"

namespace NE {
namespace Model {
namespace Tdma {

LOG_DEF("I.M.T.AppAllocation");

void searchPreferredEdges(Device* destinationDevice, PhyRoute * route, Subnet::PTR& subnet, DoubleExitEdges& preferredEdges,  Uint16 &graphID) {
    if(1 == route->alternative) { //contract based route
        if (route->route.size() >= 1){
            //check if graph or device address
            Uint16 firstRouteElement = route->route[0];
            if (isRouteGraphElement(firstRouteElement)){
                graphID = getRouteElement(firstRouteElement);
            } else {
                assert((firstRouteElement == Address::getAddress16(destinationDevice->address32)) && "The source route should be for destination device " && destinationDevice->address32);
                DoubleExitDestinations destinations;
                destinations.prefered = Address::getAddress16(destinationDevice->address32);
                destinations.backup = 0;
                preferredEdges.insert(std::make_pair(Address::getAddress16(destinationDevice->address32), destinations));
                return;
            }

            GraphPointer graph = subnet->getGraph(graphID);
            RETURN_ON_NULL_MSG(graph, "graph doesn't exist:graphId=" << graphID);
            preferredEdges = graph->getGraphEdges();

        }
        return;
    }

    Uint8 routeSize = route->route.size();

    if (routeSize == 1){
        //check if graph or device address
        Uint16 firstRouteElement = route->route[0];
        if (isRouteGraphElement(firstRouteElement)){
            graphID = getRouteElement(firstRouteElement);
        } else {
            assert((firstRouteElement == Address::getAddress16(destinationDevice->address32)) && "The source route should be for destination device " && destinationDevice->address32);
            DoubleExitDestinations destinations;
            destinations.prefered = Address::getAddress16(destinationDevice->address32);
            destinations.backup = 0;
            preferredEdges.insert(std::make_pair(Address::getAddress16(destinationDevice->address32), destinations));
            return;
        }

        GraphPointer graph = subnet->getGraph(graphID);
        RETURN_ON_NULL_MSG(graph, "graph doesn't exist:graphId=" << graphID);
        preferredEdges = graph->getGraphEdges();

    }

    if (routeSize == 2){
        //if it's a hybrid route means than last element should be the destination device, and should exist an edge from lastPrefered with destination device.
        assert(route->route[1] == Address::getAddress16(destinationDevice->address32) && "The hybrid route should end with destination device " && destinationDevice->address32);
        Uint16 firstRouteElement = route->route[0];
        if (isRouteGraphElement(firstRouteElement)){
            graphID = getRouteElement(firstRouteElement);
        }
        GraphPointer graph = subnet->getGraph(graphID);
        RETURN_ON_NULL_MSG(graph, "graph doesn't exist:graphId=" << graphID);
        preferredEdges = graph->getGraphEdges();
        DoubleExitDestinations destinations;
        destinations.prefered = Address::getAddress16(destinationDevice->address32);
        destinations.backup = 0;
        preferredEdges.insert(std::make_pair(Address::getAddress16(destinationDevice->address32), destinations));
    }
}

bool isOverlapping(int slot1, int perioada1, int slot2, int perioada2, int lengthSuperframe) {

    if (slot1 == slot2)         return true;
    if (perioada1 == perioada2) return false;
    if ((perioada1 == 0) || (perioada2 == 0)) return true;//if period is 0 the link will not repeat (must avoid divide by 0)

    if( slot1 < slot2 )
    {
        slot2 -= slot1;
        lengthSuperframe -= slot1;

        for( ;slot2 < lengthSuperframe; slot2 += perioada2 )
        {
            if( slot2 % perioada1  == 0)
                return true;
        }
    }
    else
    {
        slot1 -= slot2;
        lengthSuperframe -= slot2;

        for( ;slot1 < lengthSuperframe; slot1 += perioada1 )
        {
            if( slot1 % perioada2 == 0 )
                return true;
        }
    }

    return false;
}

bool isFreeSlot(Uint16 slot, Uint16 period, Device& device, int superframeLength) {
    bool free = true;
    for(LinkIndexedAttribute::iterator it = device.phyAttributes.linksTable.begin(); it != device.phyAttributes.linksTable.end(); ++it) {
        if( it->second.getValue() && isAppLink( it->second.getValue()->role ) )
        {
//            Schedule schedule = it->second.getValue()->schedule;
            PhyLink * link = it->second.getValue();
            if(!link) {
                continue;
            }
            int period2 = device.getLinkPeriod(link);
            if (isOverlapping(slot, period, link->schedule.offset, period2, superframeLength)) {
//                LOG_DEBUG("isFreeSlot=false slot=" << (int)slot << ", device=" << Address::toString(device.address32));
                free = false;
                break;
            }
        }
    }

    if (free){//check in the unsent operation if is free
        NE::Model::Operations::OperationsList::const_iterator itOperation;
        const NE::Model::Operations::OperationsList& unsentOperations = device.getUnsentOperations();
        for (itOperation = unsentOperations.begin(); itOperation != unsentOperations.end(); ++itOperation){
            if ((*itOperation)->getType() != Operations::EngineOperationType::WRITE_ATTRIBUTE){
                continue;
            }
            if (getEntityType((*itOperation)->getEntityIndex()) != EntityType::Link ){
                continue;
            }
            if ((*itOperation)->getPhysicalEntity() == NULL){
                continue;
            }
            const Operations::IEngineOperationPointer& operation = *itOperation;
            const PhyLink * link = (PhyLink *)(operation->getPhysicalEntity());
            int period2 = device.getLinkPeriod(link);
            if (isOverlapping(slot, period, link->schedule.offset, period2, superframeLength)) {
//                LOG_DEBUG("isFreeSlot=false slot=" << (int)slot << ", device=" << Address::toString(device.address32) << " unsentOperations");
                free = false;
                break;
            }

        }
    }

//    LOG_DEBUG("isFreeSlot=" << free << " slot=" << (int)slot << ", device=" << Address::toString(device.address32));
    return free;
}



bool reserveFreeSlot(AppSlots& appSlots, Uint16 slot, Uint16 freq, Uint16 period, const int superframeLengthAPP) {

    Uint16 freqMask = (1 << freq);
    for(Uint16 currSlot = slot; currSlot < superframeLengthAPP; currSlot += period ) {
        if (appSlots[currSlot] & freqMask) {
            LOG_DEBUG("reserveFreeSlot=false appSlots[" << (int)currSlot << "]=" << std::hex << (int)appSlots[currSlot]
                      << std::dec << ", slot=" << (int)slot << ", freq=" << (int)freq << ", period=" << (int)period);
            return false;
        }
    }

    for(Uint16 currSlot = slot; currSlot < superframeLengthAPP; currSlot += period ) {
        appSlots[currSlot] =  (appSlots[currSlot] | freqMask);
    }

    LOG_DEBUG("isFreeSlotFreq=true slot=" << (int)slot << ", freq=" << (int)freq << ", period=" << (int)period);
    return true;
}

Uint32 reserveFreeSlot(AppSlots& appSlots, Uint16 startSlot, Uint16 & maxSlotDelay, Uint16 period, Device& sourceDevice, Device& destinationDevice, const int superframeLengthAPP, Uint8 FreqCount) {

//    LOG_DEBUG("reserveFreeSlot startSlot=" << (int)startSlot << ", maxSlotDelay=" << (int)maxSlotDelay << ", period=" << (int)period
//                << ", sourceDevice=" << Address_toStream(sourceDevice.address32) << ", destinationDevice=" << Address_toStream(destinationDevice.address32)
//                << ", AppSlots: " << AppSlots_toString(appSlots));

    Uint32 startFreq = (destinationDevice.capabilities.isBackbone() ? 0 : 1); // reserve freq offset 0 for backbone (link optimizations)
    Uint32 maxDelay = maxSlotDelay;

    for(Uint32 currSlot = startSlot & 0xFFFE; maxDelay >= 2; currSlot += 2) {
        if( currSlot >= AppSlotsLength ) { // slot rollover
            currSlot -= AppSlotsLength;
        }
        maxDelay -= 2;

        if (isFreeSlot(currSlot, period, sourceDevice, superframeLengthAPP)
        		&& ( destinationDevice.capabilities.isBackbone()
        				|| (!destinationDevice.capabilities.isBackbone() && isFreeSlot(currSlot, period, destinationDevice, superframeLengthAPP) ) ) ) {
            if(destinationDevice.capabilities.isBackbone()) {
                Uint8 currFreq = 0;
                if (reserveFreeSlot(appSlots, currSlot, currFreq, period, superframeLengthAPP)) {

                    maxSlotDelay = maxDelay;

                    return ( currSlot << 4 ) | currFreq;
                }
            }
            else {
                for (Uint8 currFreq = startFreq; currFreq < FreqCount; ++currFreq) {
                    if (reserveFreeSlot(appSlots, currSlot, currFreq, period, superframeLengthAPP)) {

                        maxSlotDelay = maxDelay;
                        return ( currSlot << 4 ) | currFreq;
                    }
                }
            }

        }
    }

    LOG_ERROR( "No free slot found! startSlot=" << (int)startSlot
            << " maxSlotDelay=" << (int)maxSlotDelay
            << ", period=" << (int)period
            << ", edge: " << Address::toString(sourceDevice.address32)
            << "->" << Address::toString(destinationDevice.address32) );

    return INVALID_FREE_SLOT;
}


}
}
}
