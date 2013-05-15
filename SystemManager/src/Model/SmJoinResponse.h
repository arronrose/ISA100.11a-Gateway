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

#ifndef SMJOINRESPONSE_H_
#define SMJOINRESPONSE_H_

#include "Common/NEAddress.h"
#include "Common/Address128.h"
#include "Common/smTypes.h"
#include "Model/Capabilities.h"

using namespace Isa100::Common;
using namespace NE::Misc::Marshall;

namespace Isa100 {
namespace Model {

/**
 * The parameters for the join response.
 * @author Radu Pop, beniamin.tecar, flori.parauan
 */
struct SmJoinResponse {
        Address128 deviceAddress128;
        Address16 deviceAddress16;
        //DeviceType::DeviceTypeEnum deviceType;
        Uint16 deviceType;

        Address128 systemManagerAddress128;
        Uint16 managerAddress16;
        Address64 systemManagerAddress64;

        virtual ~SmJoinResponse() {

        }

        void marshall(OutputStream& stream) {
            deviceAddress128.marshall(stream);
            stream.write(deviceAddress16);
            stream.write((Uint16) deviceType);

            systemManagerAddress128.marshall(stream);
            stream.write(managerAddress16);
            systemManagerAddress64.marshall(stream);
        }

        void unmarshall(InputStream& stream) {
            deviceAddress128.unmarshall(stream);

            stream.read(deviceAddress16);

            Uint16 role;
            stream.read(role);
            //deviceType = DeviceType::fromInt(role);
            deviceType = role;
            systemManagerAddress64.unmarshall(stream);

            stream.read(managerAddress16);
            stream.read(managerAddress16);
            systemManagerAddress64.unmarshall(stream);
        }

        void toString( std::ostringstream &stream) {
            stream << "SmJoinResponse {";
            stream << "deviceAddress128=" << deviceAddress128.toString();
            stream << ", deviceAddress16=" << std::hex << deviceAddress16;
            stream << ", deviceType=" << (int) deviceType;
            stream << ", systemManagerAddress128=" << systemManagerAddress128.toString();
            stream << ", deviceAddress16=" << std::hex << (int) deviceAddress16;
            stream << ", systemManagerAddress64=" << systemManagerAddress64.toString();
            stream << "}";
        }
};

}
}

#endif /*SMJOINRESPONSE_H_*/
