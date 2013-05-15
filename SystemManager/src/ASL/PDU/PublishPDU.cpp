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
 * PublishPDU.cpp
 *
 *  Created on: Mar 17, 2009
 *      Author: sorin.bidian
 */

#include "PublishPDU.h"
#include "Misc/Convert/Convert.h"

using namespace NE::Misc::Convert;

namespace Isa100 {
namespace ASL {
namespace PDU {

PublishPDU::PublishPDU(Uint8 freshnessSequenceNumber_, BytesPointer data_) :
    freshnessSequenceNumber(freshnessSequenceNumber_), data(data_) {

}

PublishPDU::~PublishPDU() {
}

Uint16 PublishPDU::getSize(BytesPointer payload, Uint16 position) {
    Uint16 size = position + 1; //freshnessSequenceNumber
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

std::string PublishPDU::toString() {
    std::ostringstream stream;
    stream << " freshnessSequenceNumber: " << (int)freshnessSequenceNumber;
    stream << ", data: " << bytes2string(*data);

    return stream.str();
}

}
}
}
