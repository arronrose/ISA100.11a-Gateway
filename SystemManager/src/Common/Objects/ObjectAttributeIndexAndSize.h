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
 * ObjectAttributeIndexAndSize.h
 *
 *  Created on: Mar 13, 2009
 *      Author: sorin.bidian
 */

#ifndef OBJECTATTRIBUTEINDEXANDSIZE_H_
#define OBJECTATTRIBUTEINDEXANDSIZE_H_

#include "Common/NETypes.h"
#include "Misc/Marshall/NetworkOrderStream.h"

namespace Isa100 {
namespace Common {
namespace Objects {

/**
 * Structure that describes the type of the 6th attribute -Array of ObjectAttributeIndexAndSize- in Concentrator object.
 */
struct ObjectAttributeIndexAndSize {
    Uint16 objectID;
    Uint16 attributeID;
    Uint16 attributeIndex;
    Uint16 size;

    ObjectAttributeIndexAndSize();

    void marshall(NE::Misc::Marshall::OutputStream& stream);

    void unmarshall(NE::Misc::Marshall::InputStream& stream);

    std::string toString();
};

typedef std::vector<ObjectAttributeIndexAndSize> ObjectAttributeIndexAndSizeList;

}
}
}

#endif /* OBJECTATTRIBUTEINDEXANDSIZE_H_ */
