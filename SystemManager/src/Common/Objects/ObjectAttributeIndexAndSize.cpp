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
 * ObjectAttributeIndexAndSize.cpp
 *
 *  Created on: Mar 13, 2009
 *      Author: sorin.bidian
 */

#include "ObjectAttributeIndexAndSize.h"

using namespace Isa100::Common;
using namespace NE::Misc::Marshall;

namespace Isa100 {
namespace Common {
namespace Objects {

ObjectAttributeIndexAndSize::ObjectAttributeIndexAndSize() :
    objectID(0), attributeID(0), attributeIndex(0), size(0) {

}

void ObjectAttributeIndexAndSize::marshall(OutputStream& stream) {
    stream.write(objectID);
    stream.write(attributeID);
    stream.write(attributeIndex);
    stream.write(size);
}

void ObjectAttributeIndexAndSize::unmarshall(InputStream& stream) {
    stream.read(objectID);
    stream.read(attributeID);
    stream.read(attributeIndex);
    stream.read(size);
}

std::string ObjectAttributeIndexAndSize::toString() {
    std::ostringstream stream;
    stream << "objectID: " << (int) objectID
        << ", attributeID: " << (int) attributeID
        << ", attributeIndex: " << (int) attributeIndex
        << ", size: " << (int) size;
    return stream.str();
}

}
}
}
