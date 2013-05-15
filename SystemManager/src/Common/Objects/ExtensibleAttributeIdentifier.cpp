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
 * ExtensibleAttributeIdentifier.cpp
 *
 *  Created on: Mar 24, 2009
 *      Author: Catalin Pop
 */
#include "ExtensibleAttributeIdentifier.h"
#include <iostream>

namespace Isa100 {
namespace Common {
namespace Objects {

ExtensibleAttributeIdentifier::ExtensibleAttributeIdentifier() :
    attributeType(notIndexed), attributeID(0), oneIndex(0), twoIndex(0) {
}

ExtensibleAttributeIdentifier::ExtensibleAttributeIdentifier(Uint16 attributeID_) :
    attributeType(notIndexed), oneIndex(0), twoIndex(0) {
    //make sure attributeID is on max 12 bits
    attributeID = attributeID_ & 0x0FFF;
}

/*
 * Constructs an attribute with one index.
 */
ExtensibleAttributeIdentifier::ExtensibleAttributeIdentifier(Uint16 attributeID_, Uint16 oneIndex_) :
    attributeType(singlyIndexed), twoIndex(0) {
    //make sure attributeID is on max 12 bits
    attributeID = attributeID_ & 0x0FFF;
    if (oneIndex_ > 0x7F) {
        oneIndex = 0x8000 | oneIndex_;
    } else {
        oneIndex = oneIndex_;
    }
}

/*
 * Constructs an attribute with two indices.
 */
ExtensibleAttributeIdentifier::ExtensibleAttributeIdentifier(Uint16 attributeID_, Uint16 oneIndex_, Uint16 twoIndex_) :
    attributeType(doublyIndexed) {
    //make sure attributeID is on max 12 bits
    attributeID = attributeID_ & 0x0FFF;
    if (oneIndex_ > 0x7F) {
        oneIndex = 0x8000 | oneIndex_;
    } else {
        oneIndex = oneIndex_;
    }
    if (twoIndex_ > 0x7F) {
        twoIndex = 0x8000 | twoIndex_;
    } else {
        twoIndex = twoIndex_;
    }
}

/*
 * Constructs an attribute with data from outside the class
 */
ExtensibleAttributeIdentifier::ExtensibleAttributeIdentifier(AttributeType attributeType_, Uint16 attributeID_,
            Uint16 oneIndex_, Uint16 twoIndex_) :
    attributeType(attributeType_), attributeID(attributeID_), oneIndex(oneIndex_), twoIndex(twoIndex_) {

}

void ExtensibleAttributeIdentifier::marshall(NE::Misc::Marshall::OutputStream& stream) {
    unsigned char octet1;
    unsigned short offset;

    if ((attributeID >> 6) == 0) { //Six-bit attribute identifier
        octet1 = 0;
        offset = 6;
    } else { //Twelve-bit attribute identifier
        octet1 = 0x03 << 2;
        offset = 4;
    }

    if (attributeType == singlyIndexed) { //singly indexed
        octet1 |= 0x01;
    } else if (attributeType == doublyIndexed) { //doubly indexed
        octet1 |= 0x02;
    }

    //write attribute format and attributeID
    if (offset == 6) {
        octet1 = (octet1 << offset) | attributeID;
        stream.write(octet1);
    } else {
        octet1 = (octet1 << offset) | (attributeID >> 8);
        stream.write(octet1);
        octet1 = attributeID; //low order octet
        stream.write(octet1);
    }

    //write oneIndex and twoIndex
    if (attributeType == singlyIndexed) {
        if ((oneIndex & 0xFF00) == 0) { //seven-bit index
            octet1 = oneIndex;
            stream.write(octet1);
        } else { //fifteen-bit index
            octet1 = oneIndex >> 8;
            stream.write(octet1);
            octet1 = oneIndex;
            stream.write(octet1);
        }
    } else if (attributeType == doublyIndexed) {
        if ((oneIndex & 0xFF00) == 0) { //seven-bit index
            octet1 = oneIndex;
            stream.write(octet1);
        } else { //fifteen-bit index
            octet1 = oneIndex >> 8;
            stream.write(octet1);
            octet1 = oneIndex;
            stream.write(octet1);
        }
        if ((twoIndex & 0xFF00) == 0) { //second index seven-bits long
            octet1 = twoIndex;
            stream.write(octet1);
        } else { //second index fifteen bits long
            octet1 = twoIndex >> 8;
            stream.write(octet1);
            octet1 = twoIndex;
            stream.write(octet1);
        }
    }

}

void ExtensibleAttributeIdentifier::unmarshall(NE::Misc::Marshall::InputStream& stream) {
    unsigned char attributeFormat;
    unsigned char value;
    stream.read(value);

    if ((value & 0xC0) != 0xC0) { //first two bits = 00 or 01 or 10 (Six-bit attribute identifier)
        attributeID = value & 0x3F;
        attributeFormat = (value & 0xC0) >> 6;
    } else { //first two bits = 11 (Twelve-bit attribute identifier)
        attributeID = value & 0x0F;
        attributeFormat = (value & 0xF0) >> 4;
        stream.read(value);
        attributeID = (attributeID << 8) | value;
    }

    // unmarshall indexes
    if ((attributeFormat & 0x01) != 0) { //first two bits = 01 (singly indexed)
        attributeType = singlyIndexed;
        stream.read(value);
        if ((value & 0x80) == 0x00) { //seven-bit index
            oneIndex = value;
        } else { //fifteen-bit index
            oneIndex = value;
            stream.read(value);
            oneIndex = (oneIndex << 8) | value;
        }
    } else if ((attributeFormat & 0x02) != 0) { //first two bits = 10 (doubly indexed)
        attributeType = doublyIndexed;
        stream.read(value);
        if ((value & 0x80) == 0x00) { //first index seven-bits long
            oneIndex = value;
        } else { //first index fifteen bits long
            oneIndex = value;
            stream.read(value);
            oneIndex = (oneIndex << 8) | value;
        }
        stream.read(value);
        if ((value & 0x80) == 0x00) { //second index seven-bits long
            twoIndex = value;
        } else { //second index fifteen bits long
            twoIndex = value;
            stream.read(value);
            twoIndex = (oneIndex << 8) | value;
        }
    } else if ((attributeFormat & 0x03) == 0x03) { //reserved form
        // do nothing for now
    }

}

Uint16 ExtensibleAttributeIdentifier::getSize(BytesPointer payload, Uint16 position) {
    Uint16 size = position;
    Uint8 value = 0;
    Uint8 attributeFormat;
    value = payload->at(size);
    if ((value & 0xC0) != 0xC0) { //(Six-bit attribute identifier)
        attributeFormat = (value & 0xC0) >> 6;
        ++size;
    } else { //(Twelve-bit attribute identifier)
        attributeFormat = (value & 0xF0) >> 4;
        size += 2;
    }

    if ((attributeFormat & 0x03) != 0) { //for attributeFormat == 0 the remaining size is 0
        value = payload->at(size);
        if ((attributeFormat & 0x01) != 0) { //(singly indexed)
            if ((value & 0x80) == 0x00) { //seven-bit index
                ++size;
            } else { //fifteen-bit index
                size += 2;
            }
        } else if ((attributeFormat & 0x02) != 0) { //(doubly indexed)
            if ((value & 0x80) == 0x00) { //first index seven-bits long
                ++size;
            } else { //first index fifteen bits long
                size += 2;
            }
            value = payload->at(size);
            if ((value & 0x80) == 0x00) { //second index seven-bits long
                ++size;
            } else { //second index fifteen bits long
                size += 2;
            }
        }
    }

    return size - position;
}

std::string ExtensibleAttributeIdentifier::toString() {
    std::ostringstream stream;
    stream << " attributeID: " << (int) attributeID;
    if (attributeType == singlyIndexed) {
        stream << ", oneIndex: " << (int) oneIndex;
    } else if (attributeType == doublyIndexed) {
        stream << ", oneIndex: " << (int) oneIndex;
        stream << ", twoIndex: " << (int) twoIndex;
    }
    return stream.str();
}

std::string ExtensibleAttributeIdentifier::toIndentString() {
    std::ostringstream stream;
    stream << (int) attributeID;
    if (attributeType == singlyIndexed) {
        stream << "[" << (int) oneIndex << "]";
    } else if (attributeType == doublyIndexed) {
        stream << "[" << (int) oneIndex << "]";
        stream << "[" << (int) twoIndex << "]";
    }
    return stream.str();
}

}
}
}
