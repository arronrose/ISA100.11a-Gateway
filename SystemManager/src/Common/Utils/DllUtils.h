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
 * DllUtils.h
 *
 *  Created on: Nov 17, 2008
 *      Author: sorin.bidian
 */

#ifndef DLLUTILS_H_
#define DLLUTILS_H_

#include "Misc/Marshall/Stream.h"

namespace Isa100 {
namespace Common {
namespace Utils {

inline Uint16 unmarshallExtDLUint(NE::Misc::Marshall::InputStream& stream) {
    Uint16 value;
    Uint8 octet;
    stream.read(octet);
    if ((octet & 0x01) == 0) { //ExtDLUint, one-octet variant
        value = octet >> 1;
    }
    else {  //ExtDLUint, two-octet variant
        Uint8 nextOctet;
        stream.read(nextOctet);
        value = ((Uint16)nextOctet << 7) | (octet >> 1);
    }
    return value;
}

inline void marshallExtDLUint(NE::Misc::Marshall::OutputStream& stream, Uint16 value) {
    Uint8 octet;
    if (value > 0x7FFF) {
        std::ostringstream msg;
        msg << "ExtDLUint does not allow values greater than 32767(0x7FFF)! value=" << (int)value;
        throw NE::Common::NEException(msg.str());
    }
    if (value < 0x80) {
        octet = value << 1;
        stream.write((Uint8)(octet & 0xFE));
    }
    else {
        octet = (value << 1) | 0x01;
        stream.write((Uint8)octet);
        octet = value >> 7;
        stream.write((Uint8)octet);
    }
}

}
}
}


#endif /* DLLUTILS_H_ */
