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
 * ReadRequestPDU.h
 *
 *  Created on: Oct 13, 2008
 *      Author: beniamin.tecar
 */

#ifndef READREQUESTPDU_H_
#define READREQUESTPDU_H_

#include "Common/NETypes.h"
#include "Common/Objects/ExtensibleAttributeIdentifier.h"

namespace Isa100 {
namespace ASL {
namespace PDU {

class ReadRequestPDU {
public:
    Isa100::Common::Objects::ExtensibleAttributeIdentifier targetAttribute;

    public:
        ReadRequestPDU(Isa100::Common::Objects::ExtensibleAttributeIdentifier targetAttribute);
        ~ReadRequestPDU();

        static Uint16 getSize(BytesPointer payload, Uint16 position);
        std::string toString();
};

typedef boost::shared_ptr<ReadRequestPDU> ReadRequestPDUPointer;

} //namesoace PDU
} // namespace ASL
} // namespace Isa100

#endif /* READREQUESTPDU_H_ */
