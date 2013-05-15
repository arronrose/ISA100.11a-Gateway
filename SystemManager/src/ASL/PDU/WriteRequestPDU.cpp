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
 * WriteRequestPDU.cpp
 *
 *  Created on: Oct 14, 2008
 *      Author: sorin.bidian
 */

#include "WriteRequestPDU.h"
//#include "Common/Objects/GenericSizeAndValue.h"
#include "Misc/Convert/Convert.h"

using namespace Isa100::Common;
using namespace Isa100::Common::Objects;
using namespace NE::Misc::Convert;

namespace Isa100 {
namespace ASL {
namespace PDU {

WriteRequestPDU::WriteRequestPDU(ExtensibleAttributeIdentifier targetAttribute_, BytesPointer value_)
	: targetAttribute(targetAttribute_), value(value_) {
}

WriteRequestPDU::~WriteRequestPDU() {
}

Uint16 WriteRequestPDU::getSize(BytesPointer payload, Uint16 position) {
	Uint16 size = position + ExtensibleAttributeIdentifier::getSize(payload, position);
	Uint8 value = payload->at(size);
    Uint16 paramsSize = 0;
    if ((value & 0x80) == 0x80) { //params size = 2 bytes
        paramsSize = (value & 0x7F);
        paramsSize = paramsSize << 8;
        ++size;
        value = payload->at(size);
        paramsSize += value;
        ++size;
    } else {
        paramsSize = (value & 0x7F);
        ++size;
    }

    return size + paramsSize - position;
}

std::string WriteRequestPDU::toString() {
    std::ostringstream stream;
    stream << " targetAttribute: " << targetAttribute.toString();
    stream << ", parameters: " << bytes2string(*value);

    return stream.str();
}

}
}
}
