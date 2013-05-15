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

#include "DuplicateRequests.h"
#include "Common/SmSettingsLogic.h"

namespace Isa100 {
namespace AL {

namespace detail_DuplicateRequests {

class RequestPredicate {
    private:
        NE::Common::Address128 address128;
        AppHandle appHandle;
        Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID;

    public:
        RequestPredicate(const NE::Common::Address128& address128_, const AppHandle& appHandle_,
                    Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID_) :
            address128(address128_), appHandle(appHandle_), clientTSAP_ID(clientTSAP_ID_) {
        }

        bool operator()(const DeviceRequest& deviceRequest) {
            return ((deviceRequest.first == address128) && (deviceRequest.second.appHandle == appHandle)
                        && (deviceRequest.second.clientTSAP_ID == clientTSAP_ID));
        }
};

class RequestTimePredicate {
    private:
        time_t currentTime;

    public:
        RequestTimePredicate(time_t currentTime_) :
            currentTime(currentTime_) {
        }

        bool operator()(const DeviceRequest& deviceRequest) {
            return ((currentTime < deviceRequest.second.requestTimestamp) || (currentTime - deviceRequest.second.requestTimestamp
                        > SmSettingsLogic::instance().duplicatesTimeSpan));
        }
};

class PublishPredicate {
    private:
        NE::Common::Address128 address128;
        Uint8 freshnessNumber;

    public:
        PublishPredicate(const NE::Common::Address128& address128_, const Uint8& freshnessNumber_) :
            address128(address128_), freshnessNumber(freshnessNumber_) {
        }

        bool operator()(const DevicePublish& devicePublish) {
            return ((devicePublish.first == address128) && (devicePublish.second.freshnessNo == freshnessNumber));
        }
};

class PublishTimePredicate {
    private:
        time_t currentTime;

    public:
        PublishTimePredicate(time_t currentTime_) :
            currentTime(currentTime_) {
        }

        bool operator()(const DevicePublish& devicePublish) {
            return ((currentTime < devicePublish.second.publishTimestamp) || (currentTime - devicePublish.second.publishTimestamp
                        > SmSettingsLogic::instance().duplicatesTimeSpan));
        }
};

}

RequestsTable DuplicateRequests::requestsTable;
ResponsesTable DuplicateRequests::responsesTable;
PublishTable DuplicateRequests::publishTable;

DuplicateRequests::DuplicateRequests() {
}

DuplicateRequests::~DuplicateRequests() {
}

bool DuplicateRequests::isDuplicateRequest(const NE::Common::Address128& address128, const AppHandle& appHandle,
            Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID, Uint32 currentTime) {

    // remove entries that are older than the configured duplicatesTimespan
    RequestsTable::iterator itRem = std::remove_if(requestsTable.begin(), requestsTable.end(),
                detail_DuplicateRequests::RequestTimePredicate(currentTime));
    requestsTable.erase(itRem, requestsTable.end());

    RequestsTable::iterator itTable = std::find_if(requestsTable.begin(), requestsTable.end(),
                detail_DuplicateRequests::RequestPredicate(address128, appHandle, clientTSAP_ID));
    if (itTable != requestsTable.end()) {
        return true; //duplicate
    } else {
        RequestInfo request;
        request.appHandle = appHandle;
        request.requestTimestamp = currentTime;
        request.clientTSAP_ID = clientTSAP_ID;

        requestsTable.push_back(DeviceRequest(address128, request));
        // keep only a fixed number of entries; remove the oldest
        if (requestsTable.size() > MAX_TABLE_SIZE) {
            requestsTable.pop_front();
        }
    }
    return false;
}



bool DuplicateRequests::isDuplicatePublish(const NE::Common::Address128& address128, const Uint8& freshnessNumber, Uint32 currentTime) {

    // remove entries that are older than the configured duplicatesTimespan
    PublishTable::iterator itRem = std::remove_if(publishTable.begin(), publishTable.end(),
                detail_DuplicateRequests::PublishTimePredicate(currentTime));
    publishTable.erase(itRem, publishTable.end());

    PublishTable::iterator itTable = std::find_if(publishTable.begin(), publishTable.end(),
                detail_DuplicateRequests::PublishPredicate(address128, freshnessNumber));
    if (itTable != publishTable.end()) {
        return true; //duplicate
    } else {
        PublishTime publish;
        publish.freshnessNo = freshnessNumber;
        publish.publishTimestamp = currentTime;

        publishTable.push_back(DevicePublish(address128, publish));
        // keep only a fixed number of entries; remove the oldest
        if (publishTable.size() > MAX_TABLE_SIZE) {
            publishTable.pop_front();
        }
    }
    return false;
}


void DuplicateRequests::clear() {
    requestsTable.clear();
    responsesTable.clear();
    publishTable.clear();
}

void DuplicateRequests::toString(std::ostringstream &stream) {
    stream << "Stored requests: {";
    for (RequestsTable::iterator it = requestsTable.begin(); it != requestsTable.end(); ++it) {
        stream << std::endl << it->first.toString() << "[";
        stream << (int) it->second.appHandle;
        char *s = ctime(&it->second.requestTimestamp);
        s[strlen(s) - 1] = 0; // remove \n
        stream << ", " << s;
        stream << ", " << (int) it->second.clientTSAP_ID;
        stream << "]";
    }
    stream << std::endl << "}" << std::endl;

    stream << "Stored responses: {";
    for (ResponsesTable::iterator it = responsesTable.begin(); it != responsesTable.end(); ++it) {
        stream << std::endl << it->first.toString() << "[";
        stream << (int) it->second.appHandle << ", ";
        char *s = ctime(&it->second.requestTimestamp);
        s[strlen(s) - 1] = 0; // remove \n
        stream << s << "]";
    }
    stream << std::endl << "}" << std::endl;

    stream << "Stored publishes: {";
    for (PublishTable::iterator it = publishTable.begin(); it != publishTable.end(); ++it) {
        stream << std::endl << it->first.toString() << "[";
        stream << (int) it->second.freshnessNo << ", ";
        char *s = ctime(&it->second.publishTimestamp);
        s[strlen(s) - 1] = 0; // remove \n
        stream << s << "]";
    }
    stream << std::endl << "}" << std::endl;

}

}
}
