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
 * ChainAddRouteToBeActivated.cpp
 *
 *  Created on: Oct 21, 2009
 *      Author: flori.parauan
 */

#include "ChainAddRouteToBeActivated.h"

namespace NE {

namespace Model {

ChainAddRouteToBeActivated::ChainAddRouteToBeActivated(Subnet::PTR subnet_)
    : subnet(subnet_) {
}

ChainAddRouteToBeActivated::~ChainAddRouteToBeActivated() {
}

void ChainAddRouteToBeActivated::process(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status){
    if (status != ResponseStatus::SUCCESS){
        return;
    }

    Device* device = subnet->getDevice(deviceAddress);
    Device* backbone = subnet->getBackbone();

    if ((device == NULL) || (backbone == NULL)) {
        return;
    }

    RouteIndexedAttribute::iterator itBrRoute = backbone->phyAttributes.routesTable.begin();
      for (; itBrRoute != backbone->phyAttributes.routesTable.end(); ++itBrRoute) {
             if ((itBrRoute->second).getValue() && (itBrRoute->second).getValue()->selector == Address::getAddress16(device->address32)) {
                 subnet->routesToBeEvaluated.push_back(itBrRoute->first);
                 break;
             }
      }

}

}

}
