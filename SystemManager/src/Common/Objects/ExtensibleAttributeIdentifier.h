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

#ifndef EXTENSIBLEATTRIBUTEIDENTIFIER_H
#define EXTENSIBLEATTRIBUTEIDENTIFIER_H

#include "Common/NETypes.h"
#include "Misc/Marshall/Stream.h"
#include "Common/logging.h"


using namespace NE::Common;

namespace Isa100 {
namespace Common {
namespace Objects {

/**
 * @author Sorin Bidian
 * @version 1.0
 *
 *      Updated on: Oct 1, 2008
 */

enum AttributeType {
    notIndexed = 0,
    singlyIndexed = 1,
    doublyIndexed = 2
};

// 13.22.2.5   Object attribute coding - page 519
class ExtensibleAttributeIdentifier {

    LOG_DEF("I.C.O.ExtensibleAttributeIdentifier");

public:
    AttributeType attributeType;
    Uint16 attributeID;
    Uint16 oneIndex;
    Uint16 twoIndex;

    ExtensibleAttributeIdentifier();

    /*
     * Constructs an attribute with no indexing.
     */
    ExtensibleAttributeIdentifier(Uint16 attributeID_) ;

    /*
     * Constructs an attribute with one index.
     */
    ExtensibleAttributeIdentifier(Uint16 attributeID_, Uint16 oneIndex_) ;

    /*
     * Constructs an attribute with two indices.
     */
    ExtensibleAttributeIdentifier(Uint16 attributeID_, Uint16 oneIndex_, Uint16 twoIndex_);

    /*
     * Constructs an attribute with data from outside the class
     */
    ExtensibleAttributeIdentifier(AttributeType attributeType_, Uint16 attributeID_, Uint16 oneIndex_, Uint16 twoIndex_);

    Uint8 getAttributeType() {
        return (Uint8)attributeType;
    }

    void marshall(NE::Misc::Marshall::OutputStream& stream);

    void unmarshall(NE::Misc::Marshall::InputStream& stream);

    static Uint16 getSize(BytesPointer payload, Uint16 position);

    std::string toString();

    std::string toIndentString();

};
}
}
}

#endif
