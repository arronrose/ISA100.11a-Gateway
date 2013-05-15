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
 * SubnetsContainer.cpp
 *
 *  Created on: Sep 18, 2009
 *      Author: Catalin Pop
 */

#include "SubnetsContainer.h"
#include "Model/Subnet.h"
#include "Model/AddressAllocator.h"
#include "Model/Operations/OperationsProcessor.h"

namespace NE {
namespace Model {

SubnetsContainer::SubnetsContainer() :
    gateway(NULL), manager(NULL) {

}

SubnetsContainer::~SubnetsContainer() {
    if (manager)
        delete manager;
    if (gateway)
        delete gateway;
}

void SubnetsContainer::addSubnet(NE::Model::Subnet::PTR subnet) {
    subnets.insert(SubnetsMap::value_type(subnet->getSubnetId(), subnet));
}

Subnet::PTR SubnetsContainer::getSubnet(const Uint16 subnetId) {
    SubnetsMap::iterator itSubnets = subnets.find(subnetId);
    if (itSubnets == subnets.end()) {
        return Subnet::PTR();
    }
    return itSubnets->second;
}

NE::Model::Device * SubnetsContainer::getDevice(const Address32 address32) {
    if (manager->address32 == address32) {
        return manager;
    }
    if (gateway && gateway->address32 == address32) {
        return gateway;
    }

    Uint16 subnetId = getSubnetId(address32);
    Subnet::PTR subnet = getSubnet(subnetId);
    if (subnet == NULL) {
        return NULL;
    }
    return subnet->getDevice(address32);
}


bool  SubnetsContainer::existsDevice(const Address64& address64) {
    for (SubnetsMap::iterator itSubnet = subnets.begin(); itSubnet != subnets.end(); ++itSubnet) {
        if(itSubnet->second->existsDevice(address64)) {
            //return getDevice(itSubnet->second->getAddress32(address64));
            return true;
        }
    }

    return false;
}

bool SubnetsContainer::existsConfirmedDevice(const Address32 address32) {
    Device * device = getDevice(address32);
    if (device == NULL) {
        return false;
    } else {
        return device->isJoinConfirmed();
    }
}

bool SubnetsContainer::existsConfirmedDevice(const Address64& address64) {
    for (SubnetsMap::iterator itSubnet = subnets.begin(); itSubnet != subnets.end(); ++itSubnet) {
        if(itSubnet->second->existsConfirmedDevice(Address::getAddress16(itSubnet->second->getAddress32(address64)))) {
            return true;
        }
    }
    return false;
}

Address64 SubnetsContainer::getAddress64(const Address32 address32) {
    Device * device = getDevice(address32);
    if (device == NULL) {
        return Address64();
    }
    return device->address64;
}

Address32 SubnetsContainer::getAddress32(const Address64& address64) {
    if (manager->address64 == address64) {
        return manager->address32;
    }

    if (gateway != NULL && gateway->address64 == address64) {
        return gateway->address32;
    }

    for (SubnetsMap::iterator itSubnet = subnets.begin(); itSubnet != subnets.end(); ++itSubnet) {
        AddressMapping64_32::iterator mapping = itSubnet->second->addressMapping.find(address64);
        if(mapping !=  itSubnet->second->addressMapping.end()) {
            return mapping->second;
        }
    }

    return 0;
}

Address32 SubnetsContainer::getAddress32(const Address128& address128) {
    //It is verified first if the 128 address is manager, GW or one of the backbones.
    //These components already have a 128 address allocated and is used in join packets.
    //For field devices the 128 address is configured by SM.
    if (manager->address128 == address128) {
        return manager->address32;
    }
    if (gateway != NULL && gateway->address128 == address128) {
        return gateway->address32;
    }

    for (SubnetsMap::iterator itSubnet = subnets.begin(); itSubnet != subnets.end(); ++itSubnet) {
        Device * backbone = itSubnet->second->getBackbone();
        if (backbone != NULL && backbone->address128 == address128) {
            return backbone->address32;
        }
    }
    Address32 address32 = AddressAllocator::getAddress32(address128);
    Device* device = getDevice(address32);
    if (device == NULL){
        return 0;
    }
    //this verification must be performed in case the address32 contained in address128 was assigned to a different device
    //this can happen when the address128 is cached and device is removed and the address32 is assigned to a diffret device and then this
    //method is called with the cached address.
    if (device->address128 == address128){
        return address32;
    } else {
        return 0;
    }

}

void SubnetsContainer::addGateway(Device * device) {
    gateway = device;
    for (SubnetsMap::iterator itSubnet = subnets.begin(); itSubnet != subnets.end(); ++itSubnet) {
        itSubnet->second->addDevice(gateway);
    }
}

void SubnetsContainer::periodicPhyConsistencyCheck(NE::Model::Operations::OperationsProcessor & perationsProcessor){
    for (SubnetsMap::iterator itSubnet = subnets.begin(); itSubnet != subnets.end(); ++itSubnet) {
        itSubnet->second->periodicPhyConsistencyCheck(perationsProcessor);
    }
}

Uint16 SubnetsContainer::getNumberOfDevicesOnNetwork() {

	bool existGateway = (gateway != NULL);
    Uint16 numberOfDev = (existGateway ? 2 : 1);
    for (SubnetsMap::iterator itSubnet = subnets.begin(); itSubnet != subnets.end(); ++itSubnet) {
        numberOfDev += itSubnet->second->getNumbetOfDevicesFromSubnet();
        if (existGateway) {
        	numberOfDev -= 1;  // -1 for gateway
        }
    }
    return numberOfDev;
}


}

}
