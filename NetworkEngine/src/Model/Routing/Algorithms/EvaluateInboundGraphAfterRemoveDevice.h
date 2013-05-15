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
 * EvaluateInboundGraphAfterRemoveDevice.h
 *
 *  Created on: Mar 3, 2010
 *      Author: flori.parauan, mulderul(Catalin Pop), beniamin.tecar
 */

#ifndef EVALUATEINBOUNDGRAPHAFTERREMOVEDEVICE_H_
#define EVALUATEINBOUNDGRAPHAFTERREMOVEDEVICE_H_



#include "Model/ModelUtils.h"
#include "../GraphRoutingAlgorithmInterface.h"

namespace NE {

namespace Model {

namespace Routing {

namespace Algorithms {

class EvaluateInboundGraphAfterRemoveDevice : public NE::Model::Routing::GraphRoutingAlgorithmInterface {
    LOG_DEF("I.M.R.A.EvaluateInboundGraphAfterRemoveDevice");
    public:

        void  evaluateGraph(Subnet::PTR subnet,
                    Uint16 graphId,
                    Address32 destination,
                    DoubleExitEdges &usedEdges);

};
}
}
}
}



#endif /* EVALUATEINBOUNDGRAPHAFTERREMOVEDEVICE_H_ */
