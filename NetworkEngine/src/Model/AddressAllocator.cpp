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
 * AddressAllocator.cpp
 *
 *  Created on: Sep 17, 2009
 *      Author: Catalin Pop
 */

#include "AddressAllocator.h"
#include <arpa/inet.h>

namespace NE {

namespace Model {

AddressAllocator::AddressAllocator() {

}

AddressAllocator::~AddressAllocator() {
}


Address128 AddressAllocator::getNextAddress128(const Address64& address64, Address32 address32, Uint16 ipv16AddrPrefix) {
    Address128 addres128;
#warning should be configurable the prefix of IPV6
    //addres128.value[0] = 0xFC;
    //    addres128.value[0]=0xFE;
    //    addres128.value[1]=0x80;

    addres128.value[0] = ipv16AddrPrefix >> 8;
    addres128.value[1] = ipv16AddrPrefix;

    memcpy(addres128.value + 4, address64.value, 8);
    Address32 addressNet = htonl(address32);
    memcpy(addres128.value + 12, &addressNet, 4);
    addres128.setAddressString();

    return addres128;

}

Address32 AddressAllocator::getAddress32(const Address128& address128) {
    Address32 address32;
    memcpy(&address32, address128.value + 12, 4);
    return ntohl(address32);
}

}

}
