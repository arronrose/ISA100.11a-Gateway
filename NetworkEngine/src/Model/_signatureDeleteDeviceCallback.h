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
 * _signatureDeleteDeviceCallback.h
 *
 *  Created on: Sep 28, 2009
 *      Author: Radu Pop
 */

#ifndef _SIGNATUREDELETEDEVICECALLBACK_H_
#define _SIGNATUREDELETEDEVICECALLBACK_H_

#include "boost/function.hpp"
#include "Common/NETypes.h"

namespace NE {
namespace Model {

/**
 * Call back for the NE delete device event.
 * Every interested entity will implement this method.
 */
typedef boost::function1<void, Address32> DeleteDeviceCallback;

}
}

#endif /* _SIGNATUREDELETEDEVICECALLBACK_H_ */
