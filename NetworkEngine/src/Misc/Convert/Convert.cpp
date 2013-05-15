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

/*
 * Convert.cpp
 *
 *  Created on: Sep 29, 2009
 *      Author: Catalin Pop
 */
#include "Misc/Convert/Convert.h"
#include <iomanip>
#include "Common/NETypes.h"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"

using namespace boost;
using namespace std;
using namespace NE::Common;

namespace NE {
namespace Misc {
namespace Convert {

std::string bytes2string(const std::basic_string<unsigned char>& bytes) {
    std::ostringstream stream;
    for (std::basic_string<unsigned char>::const_iterator it = bytes.begin(); it != bytes.end(); ++it) {
        stream << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int) (*it);
        if (it + 1 != bytes.end())
            stream << " ";
    }
    return stream.str();
}

/**
 * Transform only the first <code>length</code> bytes into a string.
 */
std::string bytes2string(const std::basic_string<unsigned char>& bytes, size_t length) {
    std::ostringstream stream;

    for (std::basic_string<unsigned char>::const_iterator it = bytes.begin(); (it != bytes.end()) && ((size_t)(it - bytes.begin()) <= length); ++it) {
        stream << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int) (*it);
        if (it + 1 != bytes.end())
            stream << " ";
    }

    if (length < bytes.size()) {
        stream << "...";
    }
    return stream.str();
}

typedef vector<string> split_vector_type;
Bytes string2bytes(std::string text) {
    split_vector_type splitVec;
    split(splitVec, text, is_any_of(" "));

    Bytes bytes;
    for (split_vector_type::const_iterator it = splitVec.begin(); it != splitVec.end(); it++) {
        int b;
        std::istringstream(*it) >> std::hex >> b;
        bytes.push_back((Uint8)b);
    }
    return bytes;
}

BytesPointer text2bytes(const char* text, int length) {
    BytesPointer bytes(new Bytes());
    int value;
    const char* crt;

    crt = text;

    for (int i = 0; i < length; i++) {
        sscanf(crt, "%02X", &value);
        bytes->push_back((Uint8) value);
        crt += 3;
    }

    return bytes;
}

std::string array2string(const Uint8* value, Uint16 length) {
    std::ostringstream stream;
    for (Uint16 i = 0; i < length; i++) {
        stream << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)value[i];
        if (i < length - 1)
        stream << " ";
    }
    return stream.str();
}

void binary2text(Uint8* text, int length) {
    for (int i = 0; i < length; i++) {
        printf("%02X ", text[i]);
    }
}

void printToText(BytesPointer text) {
    for (unsigned int i = 0; i < text->length(); i++) {
        printf("%02X ", text->at(i));
    }
}

bool compare(BytesPointer sir1, BytesPointer sir2, int length) {
    if (sir1 == NULL) {
        return false;
    }
    for (int i = 0; i < length; i++) {
        if (sir1->at(i) != sir2->at(i)) {
            return false;
        }
    }

    return true;
}

bool compareBytes(Bytes& sir1, Bytes& sir2) {
    if (sir1.size() != sir2.size()) {
        return false;
    }
    for (Bytes::size_type i = 0; i < sir1.size(); i++) {
        if (sir1.at(i) != sir2.at(i)) {
            return false;
        }
    }

    return true;
}

} //namespace Convert
            } //namespace Misc
        } //namespace Isa100
