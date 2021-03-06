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
 * ChainWaitForConfirmOnEvalGraph.h
 *
 *  Created on: Feb 24, 2010
 *      Author: flori.parauan
 */

#ifndef CHAINWAITFORCONFIRMONEVALGRAPH_H_
#define CHAINWAITFORCONFIRMONEVALGRAPH_H_

#include "Model/Subnet.h"
#include "Common/logging.h"

namespace NE {

namespace Model {

class ChainWaitForConfirmOnEvalGraph {
        LOG_DEF("I.M.ChainWaitForConfirmOnEvalGraph");
        Subnet::PTR subnet;
        Uint16 graphId;

    public:
        ChainWaitForConfirmOnEvalGraph(Subnet::PTR subnet, Uint16 graphId);
        virtual ~ChainWaitForConfirmOnEvalGraph();
        void process(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status);
};
typedef boost::shared_ptr<ChainWaitForConfirmOnEvalGraph> ChainWaitForConfirmOnEvalGraphPointer;

}

}
#endif /* CHAINWAITFORCONFIRMONEVALGRAPH_H_ */
