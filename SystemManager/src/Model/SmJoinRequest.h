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

#ifndef SMJOINREQUEST_H_
#define SMJOINREQUEST_H_

#include "Common/logging.h"
#include "Model/Capabilities.h"
#include "Model/model.h"
#include "Misc/Convert/Convert.h"
#include "Common/Utils/DllUtils.h"

using namespace NE::Misc::Convert;
using namespace NE::Misc::Marshall;

namespace Isa100 {
namespace Model {

/**
 * The parameters for the join request.
 * @author Radu Pop, ion.pocol, flori.parauan, catalin.pop
 */
struct SmJoinRequest {
        LOG_DEF("I.M.SmJoinRequest");

        NE::Model::Capabilities capabilities;

        Uint8 tagNameLength;
        Uint8 com_SW_major_version;//major version of the software as required by standard: currently should be 0
        Uint8 com_SW_minor_version;//minor version of the software as required by standard: currently should be 0
        Uint8 softwareRevisionInformationLength;//software version of the implementation(NIVIS specific)
        VisibleString softwareRevisionInformation;

        NE::Model::PhyDeviceCapability deviceCapability;

        SmJoinRequest() {

        }

        virtual ~SmJoinRequest() {

        }

        void marshall(OutputStream& stream);

        void unmarshall(InputStream& stream);

};

inline
std::ostream& operator<<(std::ostream & stream, const SmJoinRequest& request) {

    stream << "capabilities={" << request.capabilities << "}";
    stream << ", tagNameLength=" << (int)request.tagNameLength;
    stream << ", com_SW_major_version=" << (int)request.com_SW_major_version;
    stream << ", com_SW_minor_version=" << (int)request.com_SW_minor_version;
    stream << ", softwareRevisionInformationLength=" << (int)request.softwareRevisionInformationLength;
    stream << ", softwareRevisionInformation=" << request.softwareRevisionInformation;

    return stream;
}

}
}

#endif /*SMJOINREQUEST_H_*/
