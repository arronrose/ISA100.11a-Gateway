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
 * ReadRequestPDU.cpp
 *
 *  Created on: Oct 14, 2008
 *      Author: sorin.bidian
 */

#include "ReadRequestPDU.h"
#include "Common/Objects/ExtensibleAttributeIdentifier.h"
#include "Misc/Convert/Convert.h"

using namespace Isa100::Common;
using namespace Isa100::Common::Objects;
using namespace NE::Misc::Convert;

namespace Isa100 {
namespace ASL {
namespace PDU {

ReadRequestPDU::ReadRequestPDU(ExtensibleAttributeIdentifier targetAttribute_)
	: targetAttribute(targetAttribute_) {
}

ReadRequestPDU::~ReadRequestPDU() {
}

Uint16 ReadRequestPDU::getSize(BytesPointer payload, Uint16 position) {
	return ExtensibleAttributeIdentifier::getSize(payload, position);
}

std::string ReadRequestPDU::toString() {
    std::ostringstream stream;
    stream << " targetAttribute: " << targetAttribute.toString();

    return stream.str();
}

}
}
}
