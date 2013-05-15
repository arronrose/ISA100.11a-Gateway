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
#ifndef NETWORKCONTRACT_H_
#define NETWORKCONTRACT_H_

#include "Common/logging.h"
#include "Common/smTypes.h"
#include "Common/Address128.h"

namespace Isa100 {
namespace Model {
namespace Network {

struct NetworkRoute {
    LOG_DEF("I.M.Route")

    Uint16 contractID;
    Address128 source;
    Address128 destination;
    Uint8 contractPriority;
    bool include_Contract_Flag;

	NetworkRoute() {
    }

    void marshall(NE::Misc::Marshall::OutputStream& stream) {
        try {
				stream.write(contractID);
                source.marshall(stream);
                destination.marshall(stream);

                Uint8 value = contractPriority;
                value = value | ((Uint8)include_Contract_Flag << 2);
                stream.write(value);

        } catch(NE::Common::NEException& ex) {
            LOG_ERROR(ex.what());
        } catch(...) {
            LOG_ERROR("Network marshall unknown exception.");
        }
    }

    void unmarshall(NE::Misc::Marshall::InputStream& stream) {
    	stream.read(contractID);
        source.unmarshall(stream);
        destination.unmarshall(stream);

        Uint8 value;
        stream.read(value);

        contractPriority = (value & 0x03);
        include_Contract_Flag = (value & 0x04);
    }

    std::string toString() {
        std::ostringstream stream;

         stream << "contractID=" << (int)contractID
			<< ", source=" << source.toString()
			<< ", destination=" << destination.toString()
			<< ", contractPriority=" << (int)contractPriority
			<< ", include_Contract_Flag=" << (int)include_Contract_Flag;

        return stream.str();
    }
};

typedef boost::shared_ptr<NetworkRoute> NetworkRoutePointer;

}
}
}



#endif /* NETWORKCONTRACT_H_ */
