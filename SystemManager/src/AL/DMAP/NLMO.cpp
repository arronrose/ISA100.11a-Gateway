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
 * NLMO.cpp
 *
 *  Created on: Mar 19, 2009
 *      Author: Andy, sorin.bidian, beniamin.tecar
 */

#include "NLMO.h"
#include <ASL/StackWrapper.h>
#include <porting.h>
#include <Model/EngineProvider.h>
#include <Stats/Isa100SMStateLog.h>

using namespace NE::Common;
using namespace Isa100::Common;

namespace Isa100 {
namespace AL {
namespace DMAP {

Stack_OutgoingInterfaceEnum GetOI(Isa100::AL::DMAP::NLMO::OutgoingInterfaceEnum oi) {
    switch (oi) {
        case Isa100::AL::DMAP::NLMO::oiDL:
            return soiDL;
        case Isa100::AL::DMAP::NLMO::oiBackbone:
            return soiBackbone;
        default:
            return soiBackbone;
    }
}

SFC::SFCEnum NLMO::AddRoute(Address128 destination, Address128 nextHop, Uint8 hopLimit,
            OutgoingInterfaceEnum outgoingInterface) {
    SFC::SFCEnum en = Stack_NLME_AddRoute(destination, nextHop, hopLimit, GetOI(outgoingInterface));

    std::ostringstream stream;
    stream << "STACK: route Added: " << destination.toString() << ", Hop: " << nextHop.toString();
//    LOG_DEBUG(stream.str());
    Isa100SMState::Isa100SMStateLog::logSmStackRoutes(stream.str());
    return en;
}

SFC::SFCEnum NLMO::DeleteRoute(Address128 destination) {
    SFC::SFCEnum en =  Stack_NLME_DeleteRoute(destination);

    std::ostringstream stream;
    stream << "STACK: route deleted: " << destination.toString();
//    LOG_DEBUG(stream.str());
    Isa100SMState::Isa100SMStateLog::logSmStackRoutes(stream.str());

    return en;
}

SFC::SFCEnum NLMO::AddContract(Uint16 contractID, Address128 sourceAddress,
            Address128 destAddress, Uint8 priority, bool includeContractFlag,
            Uint16 assignedMaxNSDUSize, Uint8 wndLen, Int16 comittedBurst, Int16 excessBurst) {
    SFC::SFCEnum en = Stack_NLME_AddContract(contractID, sourceAddress, destAddress, priority, includeContractFlag,
    		assignedMaxNSDUSize, wndLen, comittedBurst, excessBurst);
    std::ostringstream stream;
    stream << "Stack contract added: " << (int)contractID << " [dest=" << destAddress.toString() << "]";
//    LOG_DEBUG("STACK: " << stream.str());
    Isa100SMState::Isa100SMStateLog::logSmStackContracts(stream.str());

        //check the contracts in the stack and compare with contracts table
        checkStackContractsSyncro();
    return en;
}

SFC::SFCEnum NLMO::DeleteContract(Uint16 contractID) {
    assert(contractID != 0);
    SFC::SFCEnum en = Stack_NLME_DeleteContract(contractID);
    std::ostringstream stream;
    stream << "Stack contract deleted: " << (int)contractID ;
//    LOG_DEBUG(stream.str());
    Isa100SMState::Isa100SMStateLog::logSmStackContracts(stream.str());
//    if (en != SFC::success){
        //check the contracts in the stack and compare with contracts table
        checkStackContractsSyncro();
//    }
    return en;
}

void NLMO::checkStackContractsSyncro(){
}

SFC::SFCEnum NLMO::AddAddressTranslation(Address128 longAddress, Address16 shortAddress) {
//    LOG_DEBUG("STACK: ATT added: " << longAddress.toString() << "=" << Address::toString(shortAddress));
    return Stack_NLME_AddAddressTranslation(longAddress, shortAddress);
}

SFC::SFCEnum NLMO::DeleteAddressTranslation(Address128 longAddress) {
//    LOG_DEBUG("STACK: ATT deleted: " << longAddress.toString());
    return Stack_NLME_DeleteAddressTranslation(longAddress);
}

}
}
}
