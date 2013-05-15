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
 * AlertCommunicationEndpoint.h
 *
 *  Created on: Jun 4, 2009
 *      Author: Sorin.Bidian
 */

#ifndef ALERTCOMMUNICATIONENDPOINT_H_
#define ALERTCOMMUNICATIONENDPOINT_H_

#include "Common/Address128.h"

namespace Isa100 {
namespace Common {
namespace Objects {

struct AlertCommunicationEndpoint {
    /**
     * Network address of remote endpoint.
     */
    NE::Common::Address128 remoteAddress;

    /**
     * Transport layer port at remote endpoint.
     */
    Uint16 remotePort;

    /**
     * Object ID at remote endpoint.
     */
    Uint16 remoteObjectID;

	AlertCommunicationEndpoint();

    void marshall(NE::Misc::Marshall::OutputStream& stream);

    std::string toString();
};

}
}
}

#endif /* ALERTCOMMUNICATIONENDPOINT_H_ */
