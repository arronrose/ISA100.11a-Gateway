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

/**
 * @author beniamin.tecar
 */
#include "NetworkContractData.h"

namespace Isa100 {
namespace Model {

NetworkContractData::NetworkContractData() {
    contractID = 0;
}

void NetworkContractData::marshall(OutputStream& stream) {

    stream.write(contractID);
    sourceAddress.marshall(stream);
    destinationAddress.marshall(stream);

    // MSB used to match device expectations
    Uint8 contractInfo = ((Uint8)contract_Priority << 6) | ((Uint8)(include_Contract_Flag << 5));
    stream.write(contractInfo);

}

void NetworkContractData::unmarshall(InputStream& stream) {

    stream.read(contractID);
    sourceAddress.unmarshall(stream);
    destinationAddress.unmarshall(stream);

    Uint8 contractInfo;
    stream.read(contractInfo);

    contract_Priority = (NE::Model::ContractPriority::ContractPriority) ((Uint8)contractInfo & 0xC0);
    include_Contract_Flag = contractInfo & 0x20;
}

std::string NetworkContractData::toString() {
     std::ostringstream stream;
     stream << ", contractID=" << (int)contractID
		 << ", sourceAddress128=" << sourceAddress.toString()
         << ", destinationAddress128=" << destinationAddress.toString()
         << ", contract_Priority=" << contract_Priority
         << ", include_Contract_Flag=" << include_Contract_Flag;
     return stream.str();
 }

}
}
