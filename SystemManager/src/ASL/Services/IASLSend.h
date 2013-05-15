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
 * IASL.h
 *
 *  Created on: Mar 11, 2009
 *      Author: Andy
 */

#ifndef IASLSEND_H_
#define IASLSEND_H_


#include "ASL_Service_PrimitiveTypes.h"
#include "ASL_AlertAcknowledge_PrimitiveTypes.h"


namespace Isa100 {
namespace ASL {
namespace Services {

/**
 * Interface over the send request operations
 */
class IASLSend
{
public:
	virtual ~IASLSend() {}

	/**
	 * Accepts alert acknowledge from the UAP layer.
	 * The request will be send to the TLDE_DATA layer.
	 */
	virtual void Request(ASL_AlertAcknowledge_RequestPointer acknowledgeRequest) = 0;

	/**
	 * Accepts request from the UAP layer, and places the request on the queue.
	 * The request will be send to the TLDE_DATA layer.
	 */
	virtual void Request(PrimitiveRequestPointer primitiveRequest) = 0;

	/**
	 * Accepts response from the UAP layer, and places the response on the queue.
	 * The response will be send to the TLDE_DATA layer.
	 */
	virtual void Response(PrimitiveResponsePointer primitiveResponse) = 0;
};


} // namespace Services
} // namespace ASL
} // namespace Isa100
#endif /* IASLSEND_H_ */
