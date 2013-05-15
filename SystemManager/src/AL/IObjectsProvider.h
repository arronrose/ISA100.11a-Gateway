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
 * IObjectsProvider.h
 *
 *  Created on: Mar 12, 2009
 *      Author: Catalin Pop
 */

#ifndef IOBJECTSPROVIDER_H_
#define IOBJECTSPROVIDER_H_

#include "Isa100Object.h"
#include "Common/smTypes.h"

namespace Isa100 {

namespace AL {

class IObjectsProvider{
    public:
    virtual void createIsa100Object(Isa100::AL::ObjectID::ObjectIDEnum objectID, Isa100::Common::TSAP::TSAP_Enum tsap,  Isa100ObjectPointer &isa100Object) = 0;
};

typedef boost::shared_ptr<IObjectsProvider> IObjectsProviderPointer;

}  // namespace AL

}  // namespace Isa100


#endif /* IOBJECTSPROVIDER_H_ */
