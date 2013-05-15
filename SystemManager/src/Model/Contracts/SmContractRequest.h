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
 * SmContractRequest.h
 *
 *  Created on: Mar 4, 2009
 *      Author: sorin.bidian
 */

#ifndef SMCONTRACTREQUEST_H_
#define SMCONTRACTREQUEST_H_

#include "Model/ContractRequest.h"

namespace Isa100 {
namespace Model {

/**
 * Used to create a ContractRequest.
 * Shall read the contract request stream and populate the ContractRequest in NetworkEngine.
 */
class SmContractRequest {
    private:

    public:

        static NE::Model::ContractRequestPointer unmarshall(NE::Misc::Marshall::InputStream& stream);


};

}
}

#endif /* SMCONTRACTREQUEST_H_ */
