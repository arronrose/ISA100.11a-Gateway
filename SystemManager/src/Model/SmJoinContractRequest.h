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

#ifndef SMJOINCONTRACTREQUEST_H_
#define SMJOINCONTRACTREQUEST_H_

#include "Model/Capabilities.h"
#include "Misc/Convert/Convert.h"

using namespace NE::Misc::Convert;
using namespace NE::Misc::Marshall;

namespace Isa100 {
namespace Model {

/**
 * The parameters for the system manager contract request.
 * @author Radu Pop
 */
struct SmJoinContractRequest {
        Address64 euidAddress;

        void marshall(OutputStream& stream) {
            euidAddress.marshall(stream);
        }

        void unmarshall(InputStream& stream) {
            euidAddress.unmarshall(stream);
        }

        void toString( std::string &requestString) {
            std::ostringstream stream;
            stream << "SmJoinContractRequest=";
            stream << "euidAddress=" << euidAddress.toString();
            stream << "}";
            requestString = stream.str();
        }
};

}
}

#endif /*SMJOINCONTRACTREQUEST_H_*/
