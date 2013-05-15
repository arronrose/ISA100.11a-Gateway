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
 * GraphPrinter.cpp
 *
 *  Created on: Mar 2, 2010
 *      Author: Catalin Pop
 */

#include "GraphPrinter.h"

namespace NE {

namespace Model {

namespace Routing {

GraphPrinter::GraphPrinter(NE::Model::Subnet * subnet_, const NE::Model::Routing::GraphPointer& graph_) :
    subnet(subnet_), graph(graph_) {
}

GraphPrinter::~GraphPrinter() {
}

std::ostream& operator<<(std::ostream& stream, const GraphPrinter& printer) {
    const DoubleExitEdges & edges = printer.graph->getGraphEdges();
    if (edges.empty()) {
        return stream;
    }
    stream << "{ ";
    for (DoubleExitEdges::const_iterator it = edges.begin(); it != edges.end(); ++it) {
        Device * deviceFirst = printer.subnet->getDevice(it->first);
        std::string rolFirst = deviceFirst ? (deviceFirst->capabilities.isBackbone() ? "B" : (deviceFirst->capabilities.isRouting() ? "R" : "I")) : "U";
        Device * devicePref = printer.subnet->getDevice(it->second.prefered);
        std::string rolPref = devicePref ? (devicePref->capabilities.isBackbone() ? "B" : (devicePref->capabilities.isRouting() ? "R" : "I")) : "U";
        Device * deviceBack = printer.subnet->getDevice(it->second.backup);
        std::string rolBack = deviceBack ? (deviceBack->capabilities.isBackbone() ? "B" : deviceBack->capabilities.isRouting() ? "R" : "I") : "U";

        stream << std::hex << rolFirst << it->first << "(" << rolPref << it->second.prefered << "," << rolBack
                    << it->second.backup << "), ";
    }
    stream << "}";
    return stream;
}

}
}
}
