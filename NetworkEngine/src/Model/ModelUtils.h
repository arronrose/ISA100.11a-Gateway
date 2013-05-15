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
 * ModelUtils.h
 *
 *  Created on: Sep 23, 2009
 *      Author: Sorin.Bidian
 */

#ifndef MODELUTILS_H_
#define MODELUTILS_H_

#include <Model/Device.h>
#include <Model/SubnetsContainer.h>

namespace NE {
namespace Model {

/**
 * Class containing static functions that provide functionality in working with the model.
 *
 */

//typedef std::list<Address16> AddressList;


typedef std::map<Address16, Address16> Uint16Map;
typedef std::pair<Address16, Address16> Uint16MapPair;
typedef std::pair<Address16, float> Uint16FloatMapPair;
typedef std::map<Address16, float> Uint16FloatMap;
typedef std::map<float, float> FloatsMap;
//typedef boost::unordered_map<Address16, Address16Set> ParentsMap;
typedef std::map<Address16, Address16Set> ParentsMap;



class ModelUtils {

        LOG_DEF("I.S.ModelUtils");

    public:

        /**
         * Searches for the management contract on device.
         * @param device
         * @return null pointer if the contract is not found.
         */
        static PhyContract * findManagementContract(Device * device);

        /**
         * Searches for the contract with contractRequestID on device.
         * @param device
         * @param contractReqID
         * @return
         */
        static PhyContract * findContract(Device * device, Uint8 contractReqID);

        /**
         * Searches for the network contract with contractID on device.
         *
         * @param device
         * @param contractID
         * @return
         */
        static PhyNetworkContract * findNetworkContract(Device * device, Uint8 contractID);

        /**
         * Searches in device's routes table for the network route with destination = manager.
         * @param device
         * @param managerAddress
         * @return null pointer if the contract is not found.
         */
        static PhyNetworkRoute * findNetworkRouteWithManager(Device * device, NE::Common::Address128 managerAddress);

        static bool phyRouteContainsGraphId(PhyRoute* phyRoute, Uint16 graphId);


        /**
         * Find the route that will be used by device to send a packed based on contractID to destinationAddress.
         * The steps are:
         * - if a route is found based on supplied contractID (alternative==contract, selector==contractID) return route.
         * - if a route is found based on supplied destinationAddress (alternative==address, selector==destinationAddress) return route.
         * - if default route exist (alternative==default) return route.
         * - return NULL.
         * @param device - device that is the source of packet.
         * @param contractID - contractID used to send the packet.
         * @param destinationAddress - destination of the packet.
         * @return PhyRoute if is found or NULL otherwise.
         */
        static PhyRoute * findUsedRoute(Device * ownerDevice, NE::Model::SubnetsContainer& subnetsContainer, Address32 sourceAddress, Uint16 contractID, Address32 destinationAddress);
        static PhyRoute * findOutboundRoute(const Subnet::PTR& subnet, const Address32 destinationAddress);

        static void getAppAvailableSlotsAndPeriod(PhyContract * contract, Uint16 superframeLength, Uint16 &startSlot, Uint16 &maxSlotDelay, Uint16 &period);

};




}
}

#endif /* MODELUTILS_H_ */
