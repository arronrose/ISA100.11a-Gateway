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

#ifndef Isa100TYPES_H
#define Isa100TYPES_H

#include <string>
#include <ctime>
#include <exception>
#include <sstream>
#include <string.h>
#include <ios>
#include <boost/shared_ptr.hpp>
#include <queue>

namespace NetworkEngineEventType {
    /**
     * Enumeration with possible event types.
     */
    enum NetworkEngineEventTypeEnum {
        NONE = 0,
        JOIN_DEVICE = 1,
        JOIN_GATEWAY = 2,
        REMOVE_DEVICES = 3,
        CREATE_CONTRACT = 4,
        TERMINATE_CONTRACT = 5,
        EVALUATE_NEXT_ROUTE = 6,
    };
}

namespace Status {
    /**
     * Enumeration with possible contract statuses.
     */
    enum StatusEnum {
        NEW = 1,
        CHANGED = 2,
        PENDING = 3, // pending ... => ACTIVE or DELETED
        ACTIVE = 4, // after commit =>  (NEW, RECOVERED => ACTIVE; REMOVED => DELETED; NEW => PENDING)
        REMOVED = 5, // used before commit
        RECOVERED = 6, // recovered entity
        CANDIDATE = 7,
        NOT_PRESENT = 8, // as default saved entity status (first save status)
        DELETED = 9 // used after commit - in case of roll back for undo
    };

    inline
    std::string getStatusDescription(StatusEnum status) {

    	if (Status::NEW == status) {
    		return "NEW";
        } else if (Status::PENDING == status) {
            return "PENDING";
    	} else if (Status::CHANGED == status) {
            return "CHANGED";
        } else if (Status::ACTIVE == status) {
            return "ACTIVE";
        } else if (Status::REMOVED == status) {
            return "REMOVED";
        }  else if (Status::RECOVERED == status) {
            return "RECOVERED";
        }  else if (Status::CANDIDATE == status) {
            return "CANDIDATE";
        } else if (Status::NOT_PRESENT == status) {
            return "NOT_PRESENT";
        } else if (Status::DELETED == status) {
        	return "DELETED";
        }

        return "UNKNOWN";
    }

}

namespace ResponseStatus {
enum ResponseStatusEnum {
    SUCCESS = 0, FAIL = 1, REQUEST_DISCARDED = 2, REFUSED_INSUFICIENT_RESOURCES = 3,  INAPPROPRIATE_PROCESS_MODE = 10
};
}

namespace RemoveDeviceReason {
enum RemoveDeviceReason {
    timeout = 1,
    rejoin = 2,
    parentLeft = 3, //when device is removed due to parent leave
    provisioning_removed = 4, //on provisioning file change + reload, devices may be removed
    joinTimeout = 5 //when join is not completed in a certain time interval
};
}

/**
 * If the pointer is NULL logs msg with ERROR and returns.
 */
#define RETURN_ON_NULL_MSG(pointer, msg){\
    if(pointer == NULL){\
        LOG_ERROR(msg);\
        return;\
    }\
}

/**
 * If the pointer is NULL returns.
 */
#define RETURN_ON_NULL(pointer){\
    if(pointer == NULL){\
        return;\
    }\
}

/**
 * If the pointer is NULL logs msg with ERROR and throws NEException(msg).
 */
#define THROW_ON_NULL(pointer, msg){\
    if(pointer == NULL){\
        LOG_ERROR(msg);\
        throw NEException(msg);\
    }\
}

#define MAX_15BITS_VALUE 0x7FFF
#define MAX_32BITS_VALUE 0xFFFFFFFF

typedef unsigned char Byte;
typedef std::basic_string<unsigned char> Bytes;
typedef boost::shared_ptr<Bytes> BytesPointer;

typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;
typedef unsigned long long Uint64;

typedef signed char Int8;
typedef signed short Int16;
typedef signed int Int32;

typedef double Double;
typedef float Float;
typedef std::string VisibleString;

typedef std::_Ios_Openmode std__ios_flags;

typedef std::basic_istringstream<Byte> stdIstringstreamBytes;
typedef boost::shared_ptr<stdIstringstreamBytes> stdIstringstreamBytesPointer;

typedef std::ios_base::openmode stdOpenmode;

typedef std::basic_ostringstream<char> stdOstringstream;
typedef std::basic_ostringstream<Byte> stdOstringstreamBytes;
typedef boost::shared_ptr<stdOstringstreamBytes> stdOstringstreamBytesPointer;

typedef std::streampos stdStreampos;
typedef std::streamsize stdStreamsize;

typedef std::exception stdException;
typedef std::string stdString;

typedef time_t DateTime;

// ISA_SP100_Preliminary-Draft_v2007-12-21- Table 41 Contract parameters
typedef unsigned short ContractId; // Valid range: 0-255
typedef unsigned short ContractRequestId;

/**
 * The handle associates the confirm with the request.
 */
typedef Uint16 AppHandle; // Valid range: 0-65535

typedef Uint16 PayloadLength;

typedef unsigned char TSAP_ID; // Valid range: 0-255

typedef unsigned int TransmissionTime;

typedef struct TransmissionDetailedTime {
    long tv_sec;
    long tv_usec;
} TransmissionDetailedTime;

/**
 *
 */
typedef unsigned short MIBParamID;

/*
 * The address32 is composed from subnetId and nickname
 * The first bit in nickname is the alias flag
 */
typedef unsigned int Address32;
#define ADDRESS32_BRODCAST 0xFFFFFFFF

typedef Uint16 Address16;

namespace Time {

	 inline void toString(time_t time, std::string &timeString){
		char *str = ctime(&time);
		str[strlen(str) - 1] = 0;
		timeString = std::string(str);
	 }
}

namespace Type {

	inline void toString(Uint32 value, std::string &addressString) {
		std::ostringstream stream;
		stream << (long long)value;
		addressString = stream.str();
	}

	inline void toHexString(Uint32 value, std::string &addressString) {
		std::ostringstream stream;
		stream << std::hex << (long long)value;
		addressString = stream.str();
	}
}

#include "NETime.h"

#endif
