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
 * TAINetworkTimeValue.h
 *
 *  Created on: Dec 10, 2008
 *      Author: sorin.bidian
 */

#ifndef TAINETWORKTIMEVALUE_H_
#define TAINETWORKTIMEVALUE_H_

#include "Common/NETypes.h"
#include "Misc/Marshall/Stream.h"
#include <iostream>

using namespace NE::Common;
using namespace NE::Misc::Marshall;

namespace Isa100 {
namespace Common {
namespace Objects {

/*
 * TAINetworkTimeValue represents the network time in TAI time.
 * Six octets represent the current TAI time in seconds,
 * and two octets represent the fractional TAI second in units of 2-15 seconds.
 */
struct TAINetworkTimeValue {
    Uint32 currentTAI;
    Uint16 fractionalTAI;

    TAINetworkTimeValue() : currentTAI(0), fractionalTAI(0) {

    }

    TAINetworkTimeValue(Uint32 currentTAI_, Uint16 fractionalTAI_) :
        currentTAI(currentTAI_), fractionalTAI(fractionalTAI_) {

    }

    void marshall(OutputStream& stream) {
        stream.write(currentTAI);
        stream.write(fractionalTAI);
    }

    void unmarshall(InputStream& stream) {
        stream.read(currentTAI);
        stream.read(fractionalTAI);
    }

    std::string toString() {
        std::ostringstream stream;
        stream << "currentTAI: " << (int) currentTAI;
        stream << ", fractionalTAI: " << (int) fractionalTAI;
        return stream.str();
    }

};

}
}
}


#endif /* TAINETWORKTIMEVALUE_H_ */
