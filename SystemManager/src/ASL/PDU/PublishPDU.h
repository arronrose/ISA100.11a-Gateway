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
 * PublishPDU.h
 *
 *  Created on: Mar 17, 2009
 *      Author: sorin.bidian
 */

#ifndef PUBLISHPDU_H_
#define PUBLISHPDU_H_

#include "Common/NETypes.h"

namespace Isa100 {
namespace ASL {
namespace PDU {


class PublishPDU {
    public:
        Uint8 freshnessSequenceNumber;
        BytesPointer data;

    public:
        PublishPDU(Uint8 freshnessSequenceNumber, BytesPointer data);
        ~PublishPDU();

        static Uint16 getSize(BytesPointer payload, Uint16 position);
        std::string toString();
};

typedef boost::shared_ptr<PublishPDU> PublishPDUPointer;

}
}
}

#endif /* PUBLISHPDU_H_ */
