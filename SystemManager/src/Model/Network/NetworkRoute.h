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
#ifndef NETWORK_ROUTE_H_
#define NETWORK_ROUTE_H_

#include "Common/logging.h"
#include "Common/smTypes.h"
#include "Common/Address128.h"

namespace Isa100 {
namespace Model {
namespace Network {

struct NetworkRoute {
    LOG_DEF("I.M.Route")

    NE::Common::Address128 destination;
    NE::Common::Address128 nextHop;
	Uint8 nwkHopLimit;
	Uint8 outgoingInterface;

    /**
     * Identifies if the current route is management route.
     */
    bool isManagement;

	NetworkRoute() {
		isManagement = false;
    }

    void marshall(NE::Misc::Marshall::OutputStream& stream) {
        try {
                destination.marshall(stream);
                nextHop.marshall(stream);
                stream.write(nwkHopLimit);
                stream.write(outgoingInterface);

        } catch(NE::Common::NEException& ex) {
            LOG_ERROR(ex.what());
        } catch(...) {
            LOG_ERROR("Network marshall unknown exception.");
        }
    }

    void unmarshall(NE::Misc::Marshall::InputStream& stream) {
        destination.unmarshall(stream);
        nextHop.unmarshall(stream);
        stream.read(nwkHopLimit);
        stream.read(outgoingInterface);
    }

    std::string toString() {
        std::ostringstream stream;

        stream << "destination=" << destination.toString()
			<< ", nextHop=" << nextHop.toString()
			<< ", nwkHopLimit=" << (int)nwkHopLimit
			<< ", outgoingInterface=" << (int)outgoingInterface
			<< ", isManagement=" << isManagement;

        return stream.str();
    }
};

typedef boost::shared_ptr<NetworkRoute> NetworkRoutePointer;

}
}
}

#endif /* NETWORK_ROUTE_H_ */
