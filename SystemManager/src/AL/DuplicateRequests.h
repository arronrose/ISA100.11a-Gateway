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

#ifndef DUPLICATEREQUESTS_H_
#define DUPLICATEREQUESTS_H_

#include <boost/noncopyable.hpp>
#include <deque>
#include "Common/smTypes.h"

#include "Common/Address128.h"
#include "Common/NETypes.h"

using namespace Isa100::Common;

namespace Isa100 {
namespace AL {

/**
 * The maximum number of entries in the requests table.
 */
#define MAX_TABLE_SIZE 20

struct RequestInfo {
        AppHandle appHandle;
        time_t requestTimestamp;
        Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID;
};

typedef std::pair<NE::Common::Address128, RequestInfo> DeviceRequest;
typedef std::deque<DeviceRequest> RequestsTable;
typedef std::deque<DeviceRequest> ResponsesTable;

struct PublishTime {
        Uint8 freshnessNo;
        time_t publishTimestamp;
};
typedef std::pair<NE::Common::Address128,PublishTime> DevicePublish;
typedef std::deque<DevicePublish> PublishTable;

/**
 * This class holds a table of requests.
 * Every request received on the SM will be checked against the table of requests
 *  in order to detect duplicates.
 * Duplicate requests are discarded.
 *
 * Created on: Aug 19, 2008
 * @author: sorin.bidian
 */
class DuplicateRequests: boost::noncopyable {

    private:

        static RequestsTable requestsTable;
        static ResponsesTable responsesTable;
        static PublishTable publishTable;

    public:
        DuplicateRequests();
        ~DuplicateRequests();

        /**
         * Returns true if the requestID from address128 is already in the requests table. It is a duplicate.
         * If the request is not found it is added to the table.
         */
        static bool isDuplicateRequest(const NE::Common::Address128& address128, const AppHandle& appHandle,
                    Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID, Uint32 currentTime);

        /**
         * Returns true if the freshnessNumber from address128 is already in the publish table. It is a duplicate.
         * If the publish is not found it is added to the table.
         */
        static bool isDuplicatePublish(const NE::Common::Address128& address128, const Uint8& freshnessNumber, Uint32 currentTime);

        /**
         * Clear requestsTable, responsesTable, publishTable.
         */
        static void clear();

        static void toString(std::ostringstream &stream);

};

}
}

#endif /*DUPLICATEREQUESTS_H_*/
