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
 * AddressAllocator.h
 *
 *  Created on: Sep 17, 2009
 *      Author: Catalin Pop
 */

#ifndef ADDRESSALLOCATOR_H_
#define ADDRESSALLOCATOR_H_
#include "Common/NETypes.h"
#include "Common/NEAddress.h"
#include "Common/logging.h"

namespace NE {

namespace Model {

class AddressAllocator {

    LOG_DEF("I.M.R.AddressAllocator");

    public:
        AddressAllocator();
        virtual ~AddressAllocator();

        /**
         * Generates the next available node address
         */

        static Address128 getNextAddress128(const Address64& address64, Address32 address32, Uint16 ipv16AddrPrefix);

        static Address32 getAddress32(const Address128& address128);

};

}

}

#endif /* ADDRESSALLOCATOR_H_ */
