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
 * CommunicationAssociationEndpoint.h
 *
 *  Created on: Mar 13, 2009
 *      Author: sorin.bidian
 */

#ifndef COMMUNICATIONASSOCIATIONENDPOINT_H_
#define COMMUNICATIONASSOCIATIONENDPOINT_H_

#include "Common/Address128.h"

namespace Isa100 {
namespace Common {
namespace Objects {

/**
 * Structure that describes the type of the 2nd attribute -CommunicationEndpoint- in Concentrator object.
 */
struct CommunicationAssociationEndpoint {
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

    /**
     * Stale data limit (in units of seconds).
     */
    Uint8 staleDataLimit;

    /**
     * Data publication period.
     */
    Int16 publicationPeriod;

    /**
     * Ideal publication phase  (in units of tens of milliseconds).
     */
    Uint8 idealPublicationPhase;

    /**
     * Valid value set:
     *   0: Transmit only if application content changed since last publication
     *   1 : Transmit at every periodic opportunity (regardless of whether application
     *       content changed since last transmission or not)
     */
    bool publishAutoRetransmit;

    /**
     * Valid value set:
     *   0 : not configured (connection endpoint not valid)
     *   1: configured (connection endpoint valid)
     */
    Uint8 configurationStatus;

    CommunicationAssociationEndpoint();

    void marshall(NE::Misc::Marshall::OutputStream& stream);

    std::string toString();
};

}
}
}

#endif /* COMMUNICATIONASSOCIATIONENDPOINT_H_ */
