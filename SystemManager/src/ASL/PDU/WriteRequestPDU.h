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
 * WriteRequestPDU.h
 *
 *  Created on: Oct 13, 2008
 *      Author: beniamin.tecar
 */

#ifndef WRITEREQUESTPDU_H_
#define WRITEREQUESTPDU_H_

#include "Common/NETypes.h"
#include "Common/Objects/ExtensibleAttributeIdentifier.h"

namespace Isa100 {
namespace ASL {
namespace PDU {

class WriteRequestPDU {
    public:
        Isa100::Common::Objects::ExtensibleAttributeIdentifier targetAttribute;
        BytesPointer value;

    public:
        WriteRequestPDU(Isa100::Common::Objects::ExtensibleAttributeIdentifier targetAttribute,
                    BytesPointer value);
        ~WriteRequestPDU();

        static Uint16 getSize(BytesPointer payload, Uint16 position);
        std::string toString();
};

typedef boost::shared_ptr<WriteRequestPDU> WriteRequestPDUPointer;

} //namesoace PDU
} // namespace ASL
} // namespace Isa100

#endif /* WRITEREQUESTPDU_H_ */
