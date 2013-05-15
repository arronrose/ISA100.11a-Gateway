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

#include "NEAddress.h"
#include "Common/NEException.h"
#include <iomanip>


namespace NE {
namespace Common {
/********************************************************************************************************************************
 *
 * 64 bits address used as EUI-64 address.
 *
 ********************************************************************************************************************************/

Address64::Address64() {
    memset(value, 0, LENGTH);
    address64String = " 0000:0000:0000:0000";
}

void Address64::marshall(OutputStream& stream) const {
    for(Uint8 i = 0; i < LENGTH; i++) {
        stream.write(value[i]);
    }
}

void Address64::unmarshall(InputStream& stream) {
    for(Uint8 i = 0; i < LENGTH; i++) {
        stream.read(value[i]);
    }
    createAddressString ();
}

void Address64::loadBinary(Uint8 tmpValue[LENGTH]) {
    memcpy(value, tmpValue, LENGTH);
    createAddressString ();
}

void Address64::loadString(const std::string& tmpValue) {
    if (tmpValue.size() != STRING_LENGTH) {
        std::ostringstream msg;
        msg << "Address64: invalid address on loadString(";
        msg << tmpValue;
        msg << ")";
        throw InvalidArgumentException(msg.str());
    }
    Uint8 offset;
    Uint8 tmp = 0;
    Uint8 count = 0;

    for (int i = 0; i < STRING_LENGTH; i++) {
        offset = 0;

        if (tmpValue[i] == ':') {
            count++;
            continue;
        } else if (tmpValue[i] >= 'A'&& tmpValue[i] <= 'F') {
            offset = 'A' - 10;
        } else if (tmpValue[i] >= 'a'&& tmpValue[i] <= 'f') {
            offset = 'a' - 10;
        } else if (tmpValue[i] >= '0'&& tmpValue[i] <= '9') {
            offset = '0';
        }

        if ((i - count) % 2 == 1) {
            value[(i - count)/2] = tmp*16 + tmpValue[i] - offset;
        } else {
            tmp = tmpValue[i] - offset;
        }
    }
    createAddressString ();
}

void Address64::loadShortString(const std::string& tmpValue) {
    if (tmpValue.size() != SHORT_STRING_LENGTH) {
        std::ostringstream msg;
        msg << "Address64: invalid address on loadString(";
        msg << tmpValue;
        msg << ")";
        throw InvalidArgumentException(msg.str());
    }

    Uint8 offset;
    Uint8 tmp = 0;

    for (int i = 0; i < SHORT_STRING_LENGTH; i++) {
        offset = 0;

        if (tmpValue[i] >= 'A'&& tmpValue[i] <= 'F') {
            offset = 'A' - 10;
        } else if (tmpValue[i] >= 'a'&& tmpValue[i] <= 'f') {
            offset = 'a' - 10;
        } else if (tmpValue[i] >= '0'&& tmpValue[i] <= '9') {
            offset = '0';
        }

        if (i % 2 == 1) {
            value[i/2] = tmp*16 + tmpValue[i] - offset;
        } else {
            tmp = tmpValue[i] - offset;
        }
    }
    createAddressString ();
}

Address64 Address64::createFromString(const std::string& tmpValue) {
    Address64 address;
    address.loadString(tmpValue);
    return address;
}

Address64 Address64::createFromShortString(const std::string& tmpValue) {
    Address64 address;
    address.loadShortString(tmpValue);
    return address;
}

// FFFF:FFFF:FFDD:XXXX doesn't match the address FFFF:FFFF:FFFF:FFFF
bool Address64::match(const std::string& format) const {
    bool isMatched = true;
    try {
       if (format.size() != STRING_LENGTH) {
           isMatched = false;
       } else {

           std::string addressAsString = toString();
           for (int i = 0; i < STRING_LENGTH; i++) {

                if (format[i] == ':') {
                    continue;
                } else {
                    if ( (format[i] >= 'A' && format[i] <= 'F')
                            || (format[i] >= 'a' && format[i] <= 'f')
                            || (format[i] >= '0' && format[i] <= '9')) {
                        if (addressAsString[i] != format[i]) {
                            //LOG_DEBUG("NO MATCH! position=" << i << ", " << addressAsString << ", " << format);
                            isMatched = false;
                            break;
                        }
                    }
                }
           }
       }
    } catch(NEException& ex) {
        isMatched = false;
    }
    return isMatched;
}

bool Address64::operator<(const Address64 &compare) const {
    int result = memcmp(value, compare.value, LENGTH);
    return result < 0;
}

bool Address64::operator==(const Address64 &address) const {
    int result = memcmp(value, address.value, LENGTH);
    return result == 0;
}

void Address64::createAddressString ()
{
    std::ostringstream stream;
    for (Uint8 i = 0; i < LENGTH; i++) {
        stream << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)value[i];
        if (i % 2 == 1 && i != LENGTH - 1) {
            stream << ':';
        }
    }
    address64String = stream.str();
}

const std::string& Address64::toString() const {
    return address64String;
}


/********************************************************************************************************************************
 *
 * 128 bits address used as IPV6 address.
 *
********************************************************************************************************************************/

Address128::Address128() {
    memset(value, 0, LENGTH);
    stringAddress128 = "0000:0000:0000:0000:0000:0000:0000:0000";
}

void Address128::marshall(OutputStream& stream) const {
    for(Uint8 i = 0; i < LENGTH; i++) {
        stream.write(value[i]);
    }
}

void Address128::unmarshall(InputStream& stream) {
    for(Uint8 i = 0; i < LENGTH; i++) {
        stream.read(value[i]);
    }
    setAddressString();
}

void Address128::loadBinary(Uint8 tmpValue[LENGTH]) {
    memcpy(value, tmpValue, LENGTH);
    setAddressString();
}

void Address128::loadString(const std::string& tmpValue) {
    if (tmpValue.size() != STRING_LENGTH) {
        std::ostringstream msg;
        msg << "Address128: invalid address on loadString(";
        msg << tmpValue;
        msg << ")";
        throw InvalidArgumentException(msg.str());
    }

    Uint8 offset;
    Uint8 tmp = 0;
    Uint8 count = 0;


    for (int i = 0; i < STRING_LENGTH; i++) {
        offset = 0;

        if (tmpValue[i] == ':') {
            count++;
            continue;
        } else if (tmpValue[i] >= 'A'&& tmpValue[i] <= 'F') {
            offset = 'A' - 10;
        } else if (tmpValue[i] >= 'a'&& tmpValue[i] <= 'f') {
            offset = 'a' - 10;
        } else if (tmpValue[i] >= '0'&& tmpValue[i] <= '9') {
            offset = '0';
        }

        if ((i - count) % 2 == 1) {
            value[(i - count)/2] = tmp*16 + tmpValue[i] - offset;
        } else {
            tmp = tmpValue[i] - offset;
        }
    }

    setAddressString();

}

Address128 Address128::createFromString(const std::string& tmpValue) {
    Address128 address;
    address.loadString(tmpValue);
    return address;
}

bool Address128::operator<(const Address128 &compare) const {
    int result = memcmp(value, compare.value, LENGTH);
    return result < 0;
}

bool Address128::operator==(const Address128 &address) const {
    int result = memcmp(value, address.value, LENGTH);
    return result == 0;
}

void Address128::setAddressString()  {
    std::ostringstream stream;

    for (Uint8 i = 0; i < LENGTH; i++) {
        stream << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)value[i];
        if (i % 2 == 1 && i != LENGTH - 1) {
            stream << ':';
        }
    }
    stringAddress128 =  stream.str();
}

const std::string& Address128::toString() const {
    return stringAddress128;
}

unsigned char Address128::getSize() {
    return Address128::LENGTH;
}

}
}

