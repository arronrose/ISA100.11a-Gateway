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
 * SubnetsContainer.h
 *
 *  Created on: Sep 18, 2009
 *      Author: Catalin Pop
 */

#ifndef SUBNETSCONTAINER_H_
#define SUBNETSCONTAINER_H_
#include <boost/noncopyable.hpp>
#include "Model/Subnet.h"
#include "Common/logging.h"

#include "Model/Operations/OperationsProcessor.h"

namespace NE {
namespace Model {

class SubnetsContainer: public boost::noncopyable {
        LOG_DEF("N.M.SubnetsContainer");

        NE::Model::SubnetsMap subnets;

        Device * gateway;

    public:

        Device * manager;


        typedef SubnetsMap::iterator iterator;

        SubnetsContainer();

        virtual ~SubnetsContainer();

        void addSubnet(NE::Model::Subnet::PTR subnet);

        Subnet::PTR getSubnet(const Uint16 subnetId);

        Subnet::PTR getSubnet(const Address32 address32) {
            if (address32 == ADDRESS16_GATEWAY || address32 == ADDRESS16_MANAGER){
//                LOG_ERROR("No subnet for gateway or manager:" << Address_toStream(address32));
                return Subnet::PTR();
            }
            return getSubnet(Address::extractSubnet(address32));
        }

        Uint16 getSubnetId(const Uint32 address32) {
            return Address::extractSubnet(address32);
        }

        SubnetsMap& getSubnetsList() {
            return subnets;
        }

        void removeSubnet(const Uint16 subnetId){
            subnets.erase(subnetId);
        }

        NE::Model::Device * getDevice(const Address32 address32);

        bool existsDevice(const Address32 address32) {
            return (getDevice(address32) != NULL);
        }

        bool existsDevice(const Address64& address64);

        bool existsConfirmedDevice(const Address32 address32);

        bool existsConfirmedDevice(const Address64& address64);

        Address64 getAddress64(const Address32 address32);

        Address32 getAddress32(const Address64& address64);

        Address32 getAddress32(const Address128& address128);

        Address32 getManagerAddress32() {
            return manager->address32;
        }

        Address32 getGatewayAddress32() {
            return ADDRESS16_GATEWAY;
        }

        bool isGatewayAddress(Address32 address) {
            return (ADDRESS16_GATEWAY == address);
        }

        void addGateway(Device * device);

        Device * getGateway() {
            return gateway;
        }

        Uint16 getNumberOfDevicesOnNetwork();

        void periodicPhyConsistencyCheck(NE::Model::Operations::OperationsProcessor & perationsProcessor);

        //some of the operations from std Container interface

        /**
         *  Returns a read/write iterator that points to the first subnet in this container.
         */
        iterator begin() {
            return subnets.begin();
        }

        iterator end() {
            return subnets.end();
        }

};

}

}

#endif /* SUBNETSCONTAINER_H_ */
