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
 * SecurityDeleteKeyReq.h
 *
 *  Created on: Oct 29, 2009
 *      Author: Sorin.Bidian
 */

#ifndef SECURITYDELETEKEYREQ_H_
#define SECURITYDELETEKEYREQ_H_

#include <Common/NETypes.h>
#include <Common/NEAddress.h>

namespace Isa100 {
namespace Security {

/**
 * Parameters for DSMO Delete_key method.
 */

struct SecurityDeleteKeyReq {

    /**
     * This is keyUsage actually.
     */
    Uint8 keyType;

    /**
     * The key identifier of the master key used in protecting this structure.
     */
    Uint8 masterKeyID;

    Uint8 keyID;

    Uint16 sourcePort;

    NE::Common::Address128 destinationAddress;

    Uint16 destinationPort;

    Uint32 nonce;

    Uint8 mic[4];
};

}
}

#endif /* SECURITYDELETEKEYREQ_H_ */
