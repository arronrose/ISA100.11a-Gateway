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

#include "TLMO.h"
#include <ASL/StackWrapper.h>
#include <netinet/in.h>
#include <Stats/Isa100SMStateLog.h>

using namespace Isa100::Common;
using namespace NE::Common;
using namespace NE::Model;
using namespace Isa100::Common::Objects;

namespace Isa100 {
namespace AL {
namespace DMAP {

SFC::SFCEnum TLMO::SetKey(Address128 peerAddress, Common::TSAP::TSAP_Enum sourcePort,
                  Common::TSAP::TSAP_Enum destinationPort, Uint8 keyID, NE::Model::SecurityKey key,
                  NE::Common::Address64 issuerEUI64, Uint32 notValidBefore, Uint32 softLifeTime, Uint32 hardLifeTime,
                  Uint8 type, Uint8 policy) {

    SFC::SFCEnum sfc = Stack_SLME_SetKey(peerAddress, (Uint16) sourcePort | 0xF0B0, (Uint16) destinationPort | 0xF0B0,
                      keyID, key, issuerEUI64, notValidBefore, softLifeTime, hardLifeTime, type, policy);

//    std::ostringstream stream;
//    stream << "Key Added: " << peerAddress.toString()
//        << " ID:" <<  (int)keyID;
//    Stack_PrintKeys(stream);
//    Isa100SMState::Isa100SMStateLog::logSmStackKeys(stream.str());

    return sfc;
}

SFC::SFCEnum TLMO::DeleteKey(Address128 peerAddress, Common::TSAP::TSAP_Enum sourcePort,
                     Common::TSAP::TSAP_Enum destinationPort, Uint8 keyID, Uint8 keyType) {
    SFC::SFCEnum sfc = Stack_SLME_DeleteKey(peerAddress, (Uint16) sourcePort | 0xF0B0, (Uint16) destinationPort | 0xF0B0,
                         keyID, keyType);
//    std::ostringstream stream;
//    stream << "Key Deleted: " << peerAddress.toString()
//        << " ID:" <<  (int)keyID;
//    Isa100SMState::Isa100SMStateLog::logSmStackKeys(stream.str());
    return sfc;
}

}
}
}
