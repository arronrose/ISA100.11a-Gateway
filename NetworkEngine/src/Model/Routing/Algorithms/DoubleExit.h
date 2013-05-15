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
 * DoubleExit.h
 *
 *  Created on: Aug 12, 2009
 *      Author: flori.parauan, mulderul(Catalin Pop), beniamin.tecar
 */
#include "Model/ModelUtils.h"
#ifndef DOUBLEEXIT_H_
#define DOUBLEEXIT_H_

namespace NE {

namespace Model {

namespace Routing {

namespace Algorithms {

/**
 * Implements the graph routing algorithm related to Octavian's idea( double exit).
 *
 * @author fl0r1, mulderul(Catalin Pop)
 */
struct Path {
    public:
        Address16 NodeMainPath;
        Address16 NodeSecondaryPath;
        float Cost;

        Path(Address32 main, Address32 secondary, float cost) :
            NodeMainPath(main), NodeSecondaryPath(secondary), Cost(cost) {
        }

};

class DoubleExit {
    LOG_DEF("I.M.R.A.DoubleExit");
    public:


    private:

        Uint8 targetsNo;

    	Address16Set VisitedList;

        Uint16FloatMap Cost;

        // maps the VertexID to a struct that contains a node for main path and another node for secondary path
        std::map<Address16, Path> DoubleExitPaths;

    public:

        DoubleExit() {
        }


        void FindDoubleExits(Subnet::PTR  subnet, Uint16 graphID, Address16Set& nodesSkippedForBackup);

        void SetTargetsNumber(Uint8 number) {
            targetsNo = number;
        }

        void DeleteVertexesWithoutOffsprings(Subnet::PTR subnet,Address16Set &toBeVisited);

        bool HasDoubleExit(Subnet::PTR subnet, Address16 vertexA, Address16 &tmpP, Address16 &tmpS, Address16Set& nodesSkippedForBackup);

        float GetParents(Subnet::PTR subnet, Address16 vertexA, Address16 tmpP, Address16 tmpS);

        void PrintVertexesWithDoubleExit();

        void GetNodesWithoutDoubleExit(Subnet::PTR subnet, Uint16 graphID, const Address16Set &targetList, Address16Set &nodes) ;

        void GetUsedEdges(Subnet::PTR subnet, DoubleExitEdges &usedEdges);

        void GetUsedDevices(Address16Set &usedDevices) {
            for (std::map<Address16, Path>::iterator i = DoubleExitPaths.begin(); i != DoubleExitPaths.end(); i++) {
                usedDevices.insert(i->first);
            }
        }

        bool candidateIsVisited(Subnet::PTR subnet, Device* dvc, Address16 candidate);

};
}
}
}
}

#endif /* DOUBLEEXIT_H_ */
