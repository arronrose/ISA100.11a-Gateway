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
 * DoubleExitMultiSinkGraphRouting.h
 *
 *  Created on: Aug 20, 2009
 *      Author: flori.parauan, mulderul(Catalin Pop), beniamin.tecar
 */

#ifndef DOUBLEEXITMULTISINKGRAPHROUTING_H_
#define DOUBLEEXITMULTISINKGRAPHROUTING_H_

#include "../GraphRoutingAlgorithmInterface.h"
//#include "../SubnetTopology.h"
#include "Common/logging.h"
#include "DoubleExit.h"
#include <map>
#include <list>

namespace NE {

namespace Model {

namespace Routing {

namespace Algorithms {

using namespace NE::Model::Routing;

/**
 * @author flori.parauan, mulderul(Catalin Pop), beniamin.tecar
 *
 * This class derives the  GraphRoutingAlgorithmInterface  and calls  the inbound traffic algorihms: DoubleExit and MultiSink.
 *
 */

class DoubleExitMultiSinkGraphRouting: public NE::Model::Routing::GraphRoutingAlgorithmInterface {

    LOG_DEF("I.M.R.A.DoubleExitMultiSinkGraphRouting");

    public:

        DoubleExitMultiSinkGraphRouting();

        virtual ~DoubleExitMultiSinkGraphRouting();

        /**
         * Select the nodes&edges that are potential part of the inbound graph.
         */
        void evaluateGraph(NE::Model::Subnet::PTR subnet,
                    Uint16 graphID, Address32 destination, DoubleExitEdges &usedEdges);

        /**
         * Select the nodes&edges that are potential part of the outbound graph.
         */
        void CreateTargetsList(NE::Model::Subnet::PTR subnet, Uint16 graphID, Address16 destination, Address16Set &targetsList);

        void addEdgesToSolution(Subnet::PTR subnet, Address16 device, Address16 parent,  DoubleExitEdges &usedEdges, Address16Set& nodesSkippedForBackup);



    private:

        /**
         * Select the nodes&edges that are potential part of the inbound graph.
         */
        void evaluateInBoundRoute(NE::Model::Subnet::PTR subnet, Uint16 graphID, Address32 destination,
                    DoubleExitEdges &usedEdges);

        /**
         * Select the nodes&edges that are potential part of the outbound graph.
         *
         *
         */
        bool nothingChangesFromLastIteration(Subnet::PTR subnet, Uint16 graphID);
};

}

}

}

}

#endif /* DOUBLEEXITMULTISINKGRAPHROUTING_H_ */
