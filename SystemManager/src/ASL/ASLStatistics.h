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

#ifndef ASLSTATISTICS_H_
#define ASLSTATISTICS_H_

#include <map>
#include "Common/NETypes.h"
#include "Common/smTypes.h"
#include "Common/logging.h"

using namespace std;
using namespace Isa100::Common;
using namespace Isa100::Common::ServiceType;

namespace Isa100 {
namespace ASL {
struct RequestStatistic {
    Isa100::Common::ServiceType::ServiceType serviceType;
    Uint32 reqTimeReceived;
    Uint32 reqTimeSent;

    RequestStatistic() {
    }

    RequestStatistic(Isa100::Common::ServiceType::ServiceType servType) {
        serviceType = servType;
        reqTimeReceived = 0; // TODO : get the time in millisec
        reqTimeSent = 0;
    }
};

/**
 * This class keeps different statistic data like the time a package has been
 * in SM. The data will be updated as necessary.
 *
 * @author Radu Pop
 * @version 1.0, 16.04.2008
 */
class ASLStatistics {
    public:

    private:
        std::map<Byte, RequestStatistic> statistics;

        ASLStatistics();
    public:
        virtual ~ASLStatistics();

        static ASLStatistics& instance();

        /**
         * Adds a new request to the statistics and sets the reqTimeReceived with the current time.
         * If there is a request with the given id then it would be overwritten.
         */
        void addRequest(Byte requestId, Isa100::Common::ServiceType::ServiceType serviceType);

        /**
         * Tries to match the response with the given id with a request id received
         * earlier. Sets the time the response leaves the System Manager.
         */
        void addResponse(Byte requestId);

        /**
         * Returns the statistics.
         */
        map<Byte, RequestStatistic> getStatistics();
};
}
}

#endif
