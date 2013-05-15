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
 * _signatureHandlerResponse.h
 *
 *  Created on: Sep 16, 2009
 *      Author: Catalin Pop
 */

#ifndef _SIGNATUREHANDLERRESPONSE_H_
#define _SIGNATUREHANDLERRESPONSE_H_
#include "boost/function.hpp"
#include "Common/NETypes.h"
#include <list>

namespace NE {

namespace Model {



/**
 * Call back function for signaling that NetorkEngine has prepared the data to be sent in response to a request.
 * The Parameters are : address of the requesting device, request id, status of the processing of this request.
 */
typedef boost::function3<void, Address32, int, ResponseStatus::ResponseStatusEnum> HandlerResponse;
typedef std::list<HandlerResponse> HandlerResponseList;

inline
void callHandlerResponsesList(HandlerResponseList& handlerResponses, Address32 requesterAddress32, int requestId, ResponseStatus::ResponseStatusEnum responseStatus){
    for(HandlerResponseList::iterator it = handlerResponses.begin(); it != handlerResponses.end(); ++it) {
        if (*it) {
            (*it)(requesterAddress32, requestId, responseStatus);
        }

    }
}

}
}

#endif /* _SIGNATUREHANDLERRESPONSE_H_ */
