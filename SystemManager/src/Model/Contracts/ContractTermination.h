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
 * ContractTermination.h
 *
 *  Created on: Oct 2, 2008
 *      Author: beniamin.tecar
 */

#ifndef CONTRACTTERMINATION_H_
#define CONTRACTTERMINATION_H_


#include "Common/NETypes.h"
#include "Model/SecurityKey.h"

using namespace Isa100::Common;
using namespace NE::Misc::Marshall;

namespace Isa100 {
namespace Model {

/**
 * Contains the input arguments for the SCO.Contract_Termination method.
 */
struct ContractTerminationRequest {
    /**
     * A contract id to identify the contract being terminated.
     */
    Uint16 contractID;

    void marshall(OutputStream& stream) {
        stream.write(contractID);
    }

    void unmarshall(InputStream& stream) {
        stream.read(contractID);
    }
};

/**
 * Contains the output arguments for the SCO.Contract_Termination method.
 */
struct ContractTerminationResponse {
    /**
     * A status for the contract termination request.
     * 0 - success, 1 - failure
     */
    Uint8 status;

    ContractTerminationResponse(Uint8 status_) : status(status_) {
    }

    void marshall(OutputStream& stream) {
        stream.write(status);
    }

    void unmarshall(InputStream& stream) {
        stream.read(status);
    }
};

}
}

#endif /* CONTRACTTERMINATION_H_ */
