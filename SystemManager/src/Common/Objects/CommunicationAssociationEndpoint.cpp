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
 * CommunicationAssociationEndpoint.cpp
 *
 *  Created on: Mar 13, 2009
 *      Author: sorin.bidian
 */

#include "CommunicationAssociationEndpoint.h"

using namespace Isa100::Common;
using namespace NE::Misc::Marshall;

namespace Isa100 {
namespace Common {
namespace Objects {

CommunicationAssociationEndpoint::CommunicationAssociationEndpoint() :
    remotePort(0), remoteObjectID(0), staleDataLimit(0), publicationPeriod(0), idealPublicationPhase(0),
                publishAutoRetransmit(0), configurationStatus(0) {

}

void CommunicationAssociationEndpoint::marshall(OutputStream& stream) {
    remoteAddress.marshall(stream);
    stream.write(remotePort);
    stream.write(remoteObjectID);
    stream.write(staleDataLimit);
    stream.write(publicationPeriod);
    stream.write(idealPublicationPhase);
    stream.write(publishAutoRetransmit);
    stream.write(configurationStatus);
}

std::string CommunicationAssociationEndpoint::toString() {
    std::ostringstream stream;
    stream << "remoteAddress: " << remoteAddress.toString()
        << ", remotePort: " << (int) remotePort
        << ", remoteObjectID: " << (int) remoteObjectID
        << ", staleDataLimit: " << (int) staleDataLimit
        << ", publicationPeriod: " << (int) publicationPeriod
        << ", idealPublicationPhase: " << (int) idealPublicationPhase
        << ", publishAutoRetransmit: " << (int) publishAutoRetransmit
        << ", configurationStatus: " << (int) configurationStatus;
    return stream.str();
}

}
}
}
