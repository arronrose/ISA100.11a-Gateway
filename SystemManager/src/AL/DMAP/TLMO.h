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

#ifndef TLMO_H_
#define TLMO_H_

#include <Common/NEAddress.h>
#include <Model/SecurityKey.h>
#include "AL/Isa100Object.h"
#include <Common/smTypes.h>
#include <Common/logging.h>

namespace Isa100 {
namespace AL {
namespace DMAP {

class TLMO;
typedef boost::shared_ptr<TLMO> TLMOPointer;

/**
 * Transport layer management object (TLMO).
 * This object facilitates the management of the device's transport layer.
 */
class TLMO {

    public:

    	Isa100::Common::Objects::SFC::SFCEnum SetKey(NE::Common::Address128 peerAddress, Common::TSAP::TSAP_Enum sourcePort,
                    Common::TSAP::TSAP_Enum destinationPort, Uint8 keyID, NE::Model::SecurityKey key,
                    NE::Common::Address64 issuerEUI64, Uint32 notValidBefore, Uint32 softLifeTime, Uint32 hardLifeTime,
                    Uint8 type, Uint8 policy);

    	Isa100::Common::Objects::SFC::SFCEnum DeleteKey(NE::Common::Address128 peerAddress, Common::TSAP::TSAP_Enum sourcePort,
                       Common::TSAP::TSAP_Enum destinationPort, Uint8 keyID, Uint8 keyType);
};

}
}
}
#endif /*TLMO_H_*/
