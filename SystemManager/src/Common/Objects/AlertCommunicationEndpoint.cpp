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
 * AlertCommunicationEndpoint.cpp
 *
 *  Created on: Jun 4, 2009
 *      Author: Sorin.Bidian
 */

#include "AlertCommunicationEndpoint.h"

using namespace Isa100::Common;
using namespace NE::Misc::Marshall;

namespace Isa100 {
namespace Common {
namespace Objects {

AlertCommunicationEndpoint::AlertCommunicationEndpoint() :
	remotePort(0), remoteObjectID(0) {

}

void AlertCommunicationEndpoint::marshall(OutputStream& stream) {
    remoteAddress.marshall(stream);
    stream.write(remotePort);
    stream.write(remoteObjectID);
}

std::string AlertCommunicationEndpoint::toString() {
    std::ostringstream stream;
    stream << "remoteAddress: " << remoteAddress.toString()
        << ", remotePort: " << (int) remotePort
        << ", remoteObjectID: " << (int) remoteObjectID;

    return stream.str();
}

}
}
}
