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
 * Types.h
 *
 *  Created on: Mar 13, 2009
 *      Author: Andy
 */

#ifndef TYPES_H_
#define TYPES_H_

#include "../Services/ASL_Service_PrimitiveTypes.h"
#include "../Services/ASL_AlertAcknowledge_PrimitiveTypes.h"
#include "../Services/ASL_AlertReport_PrimitiveTypes.h"
#include "../Services/ASL_Publish_PrimitiveTypes.h"

#include <boost/function.hpp>

typedef boost::function1<void, Isa100::ASL::Services::PrimitiveIndicationPointer> IndicateCallback;
typedef boost::function1<void, Isa100::ASL::Services::PrimitiveConfirmationPointer> ConfirmCallback;
typedef boost::function1<void, Isa100::ASL::Services::ASL_Publish_IndicationPointer> PublishIndicateCallback;
typedef boost::function1<void, Isa100::ASL::Services::ASL_AlertReport_IndicationPointer> AlertReportCallback;

#endif /* TYPES_H_ */
