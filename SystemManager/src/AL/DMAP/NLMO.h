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

/**
 * @author  florin.muresan, andrei.petrut, sorin.bidian, beniamin.tecar
 */
#ifndef NLMO_H_
#define NLMO_H_

#include <Common/Objects/SFC.h>
#include <Common/NEAddress.h>
#include <Common/smTypes.h>
#include "Common/logging.h"


using namespace Isa100::Common::Objects;

namespace Isa100 {
namespace AL {
namespace DMAP {


class NLMO {
        LOG_DEF("I.A.DMAP.NLMO")
        ;
    public:

        enum OutgoingInterfaceEnum {
            oiDL = 0, oiBackbone = 1
        };

        SFC::SFCEnum AddRoute(NE::Common::Address128 destination, NE::Common::Address128 nextHop, Uint8 hopLimit,
                    OutgoingInterfaceEnum outgoingInterface);

        SFC::SFCEnum DeleteRoute(NE::Common::Address128 destination);

//        SFC::SFCEnum DeleteRoute(IIsa100EngineOperationPointer operation);

        SFC::SFCEnum AddContract(Uint16 contractID, NE::Common::Address128 sourceAddress, NE::Common::Address128 destAddress,
        		Uint8 priority, bool includeContractFlag, Uint16 assignedMaxNSDUSize, Uint8 wndLen,
        		Int16 comittedBurst, Int16 excessBurst);

        SFC::SFCEnum DeleteContract(Uint16 contractID);

        SFC::SFCEnum AddAddressTranslation(NE::Common::Address128 longAddress, Address16 shortAddress);

        SFC::SFCEnum DeleteAddressTranslation(NE::Common::Address128 longAddress);

    private:
        void checkStackContractsSyncro();
};

}
}
}

#endif /*NLMO_H_*/
