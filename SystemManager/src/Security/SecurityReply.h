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
 * @author radu.pop, andrei.petrut, catalin.pop
 */
#ifndef SECURITYREPLY_H_
#define SECURITYREPLY_H_

#include "Common/NEAddress.h"

using namespace Isa100::Common;
using namespace NE::Misc::Marshall;

namespace Isa100 {
namespace Security {

struct SecurityReply {
    /**
     * 0 - join request security
     * 1 - join confirmation security
     * 2 - join reply security
     */
    Uint8 ucType;
    Uint32 newNodeChallenge;
    Uint32 secMngChallenge;
    Address128 masterKey;
    Address128 dllKey;
    Address128 sysManKey;
    Address128 policies;
    Uint32 mic;

    virtual ~SecurityReply() {
    }

    void marshall(OutputStream& stream) {
        stream.write(ucType);
        stream.write(newNodeChallenge);
        stream.write(secMngChallenge);
        masterKey.marshall(stream);
        dllKey.marshall(stream);
        sysManKey.marshall(stream);
        policies.marshall(stream);
        stream.write(mic);
    }

    void unmarshall(InputStream& stream) {
        stream.read(ucType);
        stream.read(newNodeChallenge);
        stream.read(secMngChallenge);
        masterKey.unmarshall(stream);
        dllKey.unmarshall(stream);
        sysManKey.unmarshall(stream);
        policies.unmarshall(stream);
        stream.read(mic);
    }

};

}
}

#endif /*SECURITYREPLY_H_*/
