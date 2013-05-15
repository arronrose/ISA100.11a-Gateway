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
 * DoubleExitFirstLayer.h
 *
 *  Created on: Oct 28, 2009
 *      Author: flori.parauan, mulderul(Catalin Pop), beniamin.tecar
 */

#ifndef DOUBLEEXITFIRSTLAYER_H_
#define DOUBLEEXITFIRSTLAYER_H_

#include "Model/ModelUtils.h"

namespace NE {

namespace Model {

namespace Routing {

namespace Algorithms {

class DoubleExitFirstLayer {
    LOG_DEF("I.M.R.A.DoubleExitFirstLayer");
    public:

        void findDoubleExitFirstLayer(Subnet::PTR subnet, Uint16 graphID, DoubleExitEdges &usedEdges, Address16Set& nodesSkippedForBackup);

        void getBackupForDevice(Subnet::PTR subnet, Device* device, Address16Set& nodesSkippedForBackup);

        bool isBackupSuitable(Address16 device, Address16 backup);

        void filterFirstLayerDevices(Subnet::PTR subnet, GraphPointer& graph, Address32 backboneAddr);

        bool hasCycle(Address16 device, Address16 backup);

        Uint8 getNoOfFirstLayerDevices();

    private:
        DoubleExitEdges parentsMap;

        Address16Set firstLayerDevices;

};
}
}
}
}

#endif /* DOUBLEEXITFIRSTLAYER_H_ */
