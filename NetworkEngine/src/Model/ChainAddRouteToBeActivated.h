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
 * ChainAddRouteToBeActivated.h
 *
 *  Created on: Oct 21, 2009
 *      Author: flori.parauan
 */

#ifndef CHAINADDROUTETOBEACTIVATED_H_
#define CHAINADDROUTETOBEACTIVATED_H_

#include "Model/Subnet.h"
#include "Model/Device.h"
#include "Common/logging.h"


namespace NE {

namespace Model {

class ChainAddRouteToBeActivated {
    LOG_DEF("N.M.ChainAddRouteToBeActivated");
    Subnet::PTR subnet;
    public:
        ChainAddRouteToBeActivated(Subnet::PTR subnet);
        virtual ~ChainAddRouteToBeActivated();

        void process(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status);
};
typedef boost::shared_ptr<ChainAddRouteToBeActivated> ChainAddRouteToBeActivatedPointer;

}

}

#endif /* CHAINADDROUTETOBEACTIVATED_H_ */
