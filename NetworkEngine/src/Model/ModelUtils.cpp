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
 * ModelUtils.cpp
 *
 *  Created on: Oct 14, 2009
 *      Author: Catalin Pop
 */

#include <Model/ModelUtils.h>
#include <Model/modelDefault.h>
#include <Common/NEAddress.h>


namespace NE {
namespace Model {

PhyContract * ModelUtils::findManagementContract(Device * device) {
    for (ContractIndexedAttribute::iterator itContract = device->phyAttributes.contractsTable.begin(); itContract
                != device->phyAttributes.contractsTable.end(); ++itContract) {
        PhyContract * contract = itContract->second.getValue();
        if (contract && contract->isManagement) {
            return contract;
        }
    }

    LOG_ERROR("Could not find management contract on device " << device->address32);
    return NULL;
}

PhyContract * ModelUtils::findContract(Device * device, Uint8 contractReqID) {
    for (ContractIndexedAttribute::iterator itContract = device->phyAttributes.contractsTable.begin(); itContract
                != device->phyAttributes.contractsTable.end(); ++itContract) {
        PhyContract * contract = itContract->second.getValue();
        if (contract && contract->requestID == contractReqID) {
            return contract;
        }
    }

    LOG_ERROR("Could not find contract with contractReqID=" << (int) contractReqID << " on device " << device->address32);
    return NULL;
}

PhyNetworkContract * ModelUtils::findNetworkContract(Device * device, Uint8 contractID) {
    for (NetworkContractIndexedAttribute::iterator itContract = device->phyAttributes.networkContractsTable.begin(); itContract
                != device->phyAttributes.networkContractsTable.end(); ++itContract) {
        PhyNetworkContract * contract = itContract->second.getValue();
        if (contract && contract->contractID == contractID) {
            return contract;
        }
    }

    LOG_ERROR("Could not find networkContract with contractID=" << (int) contractID << " on device " << device->address32);
    return NULL;
}

PhyNetworkRoute * ModelUtils::findNetworkRouteWithManager(Device * device, NE::Common::Address128 managerAddress) {
    for (NetworkRouteIndexedAttribute::iterator itRoute = device->phyAttributes.networkRoutesTable.begin(); itRoute
                != device->phyAttributes.networkRoutesTable.end(); ++itRoute) {

        PhyNetworkRoute * networkRoute = itRoute->second.getValue();

        if (networkRoute && networkRoute->destination == managerAddress) {
            return networkRoute;
        }
    }

    LOG_ERROR("Could not find network route with manager on device " << device->address32);
    return NULL;
}

bool ModelUtils::phyRouteContainsGraphId(PhyRoute* phyRoute, Uint16 graphId) {
    if (phyRoute) {
        Uint16 pattern = ROUTE_GRAPH_MASK | graphId;
         if (std::find(phyRoute->route.begin(), phyRoute->route.end(), pattern) != phyRoute->route.end()) {
             return true;
         }
    }

    return false;
}

PhyRoute * ModelUtils::findUsedRoute(Device * ownerDevice, NE::Model::SubnetsContainer& subnetsContainer, Address32 sourceAddress, Uint16 contractID, Address32 destinationAddress){
    ownerDevice = NULL;


    if (sourceAddress == ADDRESS16_GATEWAY || sourceAddress == ADDRESS16_MANAGER){//in case of SM or GW route is on the backbone device
        const Subnet::PTR& subnet = subnetsContainer.getSubnet(destinationAddress);
        ownerDevice = subnet->getBackbone();
    } else {
        ownerDevice = subnetsContainer.getDevice(sourceAddress);
    }

    if(!ownerDevice) {
        return NULL;
    }

    if((sourceAddress != ADDRESS16_GATEWAY) && (sourceAddress != ADDRESS16_MANAGER) &&
                (destinationAddress != ADDRESS16_GATEWAY) && (destinationAddress != ADDRESS16_MANAGER)) {// in case of local loop

        Device* dst = subnetsContainer.getDevice(destinationAddress);

        if(!dst) {
            return NULL;
        }

        Subnet::PTR subnet =  subnetsContainer.getSubnet(sourceAddress);
        if(!subnet) {
            return NULL;
        }

        Device* backbone = subnet->getBackbone();
        if(!backbone) {
            return NULL;
        }


        PhyRoute * contractRoute = new PhyRoute();
        contractRoute->index = ownerDevice->getNextRouteID();
        contractRoute->alternative = 1;
        contractRoute->selector = contractID;
        LOG_DEBUG("created contract route with selector = " <<  contractID);
        contractRoute->route.push_back((ROUTE_GRAPH_MASK | DEFAULT_GRAPH_ID));

        Uint16 outboundGraph = backbone->getOutBoundGraph(Address::getAddress16(destinationAddress));
        if (outboundGraph) {
            contractRoute->route.push_back((ROUTE_GRAPH_MASK | outboundGraph));
        }

        contractRoute->evaluationTime = 2;

        return contractRoute;

    }

    PhyRoute * routeOnAddress = NULL;
    PhyRoute * routeDefault = NULL;
    RouteIndexedAttribute::iterator it = ownerDevice->phyAttributes.routesTable.begin();
    for (; it != ownerDevice->phyAttributes.routesTable.end(); ++it){
        PhyRoute * currentRoute = it->second.getValue();

        if(currentRoute && currentRoute->alternative == 1 && currentRoute->selector == contractID){
            return currentRoute;//if route on contract exist return it
        } else if(currentRoute &&  currentRoute->alternative == 2 && currentRoute->selector == NE::Common::Address::getAddress16(destinationAddress) ){
            routeOnAddress = currentRoute;//store it and check the rest. maybe something better is found
        } else if(currentRoute &&  currentRoute->alternative == 3 ){
            routeDefault = currentRoute;//store it and check the rest. maybe something better is found
        }
    }
    return (routeOnAddress ? routeOnAddress : routeDefault);//may contain valid value or NULL
}

/**
 * Find the outbound route for device. This only find route base don destination address (if there are routes on contractId it will not find them).
 * @param subnetsContainer
 * @param destinationAddress
 * @return
 */
PhyRoute * ModelUtils::findOutboundRoute(const Subnet::PTR& subnet, const Address32 destinationAddress){

    Device * backboneDevice = subnet->getBackbone();

    if(!backboneDevice) {
        return NULL;
    }

    PhyRoute * routeOnAddress = NULL;
    PhyRoute * routeDefault = NULL;
    RouteIndexedAttribute::iterator it = backboneDevice->phyAttributes.routesTable.begin();
    for (; it != backboneDevice->phyAttributes.routesTable.end(); ++it){
        PhyRoute * currentRoute = it->second.getValue();

        if(currentRoute &&  currentRoute->alternative == 2 && currentRoute->selector == NE::Common::Address::getAddress16(destinationAddress) ){
//            routeOnAddress = currentRoute;//store it and check the rest. maybe something better is found
            return currentRoute;
        } else if(currentRoute &&  currentRoute->alternative == 3 ){
            routeDefault = currentRoute;//store it and check the rest. maybe something better is found
        }
    }
    return (routeOnAddress ? routeOnAddress : routeDefault);//may contain valid value or NULL
}


void ModelUtils::getAppAvailableSlotsAndPeriod(PhyContract * contract, Uint16 superframeLength, Uint16 &startSlot, Uint16 &maxSlotDelay, Uint16 &periodInSlots) {

    periodInSlots = (contract->communicationServiceType == CommunicationServiceType::Periodic
                            ? contract->assignedPeriod
                            : (-contract->assignedCommittedBurst));

    if( periodInSlots & 0x8000 ) {// negative value -> subsecond period

        periodInSlots = -periodInSlots;
        if( periodInSlots == 1 )  {
            periodInSlots = 100;
        }
        else {
            if( periodInSlots == 2 )  {
                periodInSlots = 50;
            }
            else {
                periodInSlots = 25;
            }
        }
    }
    else {
        if( periodInSlots <= 1 ) {
            periodInSlots = 100;
        }
        else {
            periodInSlots *= 100;
            if( periodInSlots >= superframeLength ) {
                periodInSlots = superframeLength;
            } else if (periodInSlots >= 1500){
                periodInSlots = 1500;
            } else if (periodInSlots >= 1000){
                periodInSlots = 1000;
            } else if (periodInSlots >= 600){
                periodInSlots = 600;
            } else if (periodInSlots >= 500){
                periodInSlots = 500;
            } else if (periodInSlots >= 300){
                periodInSlots = 300;
            } else if (periodInSlots >= 200){
                periodInSlots = 200;
            } else {
                periodInSlots = 100;
            }
        }
    }


    if (contract->communicationServiceType == CommunicationServiceType::Periodic){

        startSlot = (contract->assignedPhase * periodInSlots)/100;//phase is percent of period
        startSlot = startSlot & 0xFFFE;//app slot must be even

        maxSlotDelay = (contract->assignedDeadline && (contract->assignedDeadline < periodInSlots) ? contract->assignedDeadline : periodInSlots);
    }
    else {
        startSlot = 0;
        maxSlotDelay = superframeLength;
    }

}

}
}

