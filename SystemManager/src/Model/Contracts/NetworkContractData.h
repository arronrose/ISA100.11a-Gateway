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
 * NetworkContractData.h
 *
 *  Created on: Mar 31, 2009
 *      Author: beniamin.tecar
 */

#ifndef NETWORKCONTRACTDATA_H_
#define NETWORKCONTRACTDATA_H_


#include "Model/ContractTypes.h"
#include "Common/Address128.h"

namespace Isa100 {
namespace Model {

/**
 * The DMO in the source shall maintain a list of all assigned contracts using the Contracts_Table attribute.
 * This attribute shall be based on the data structure NetworkContract_Data.
 */
struct NetworkContractData {

	Uint16 contractID;

    /**
     * This element is the same as in ContractRequest.
     */
	NE::Common::Address128 sourceAddress;

    /**
     * This element is the same as in ContractRequest.
     */
	NE::Common::Address128 destinationAddress;

    /**
     *
     */
	NE::Model::ContractPriority::ContractPriority contract_Priority;

    /**
     *
     */
    bool include_Contract_Flag;

    NetworkContractData();

    void marshall(OutputStream& stream);
    void unmarshall(InputStream& stream);

    std::string toString();

};

typedef boost::shared_ptr<NetworkContractData> NetworkContractDataPointer;

}
}


#endif /* NETWORKCONTRACTDATA_H_ */
