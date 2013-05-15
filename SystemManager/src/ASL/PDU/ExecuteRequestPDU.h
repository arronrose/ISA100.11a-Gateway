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
 * ExecuteRequestPDU.h
 *
 *  Created on: Oct 13, 2008
 *      Author: beniamin.tecar
 */

#ifndef EXECUTEREQUESTPDU_H_
#define EXECUTEREQUESTPDU_H_

#include "Common/NETypes.h"
#include "Common/smTypes.h"

namespace Isa100 {
namespace ASL {
namespace PDU {

class ExecuteRequestPDU {
public:
	Uint8 methodID;
	BytesPointer parameters;
public:
	ExecuteRequestPDU(Uint8 methodID, BytesPointer parameters);
	~ExecuteRequestPDU();

	static Uint16 getSize(BytesPointer payload, Uint16 position);
	std::string toString();
};

typedef boost::shared_ptr<ExecuteRequestPDU> ExecuteRequestPDUPointer;

}  // namespace PDU
} // namespace ASL
} // namespace Isa100

#endif /* EXECUTEREQUESTPDU_H_ */
