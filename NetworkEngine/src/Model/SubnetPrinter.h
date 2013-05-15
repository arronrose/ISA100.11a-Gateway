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
 * SubnetPrinter.h
 *
 *  Created on: Sep 29, 2009
 *      Author: mulderul(catalin.pop)
 */

#ifndef SUBNETPRINTER_H_
#define SUBNETPRINTER_H_

#include "Model/Subnet.h"
#include "Model/ModelPrinter.h"

namespace NE {

namespace Model {

class SubnetShortPrinter {

    public:
        Subnet::PTR subnet;
        SubnetShortPrinter(Subnet::PTR subnet_) :
            subnet(subnet_) {
        }
        virtual ~SubnetShortPrinter() {
        }
};
std::ostream& operator<<(std::ostream& stream, const SubnetShortPrinter& printer);

class SubnetDetailPrinter {

    public:
        Subnet::PTR subnet;
        SubnetDetailPrinter(Subnet::PTR subnet_) :
            subnet(subnet_) {
        }
        virtual ~SubnetDetailPrinter() {
        }
};
std::ostream& operator<<(std::ostream& stream, const SubnetDetailPrinter& printer);

struct LevelSubnetDetailPrinter {

    public:
        Subnet::PTR subnet;
        LogDeviceDetail& level;
        LevelSubnetDetailPrinter(Subnet::PTR subnet_, LogDeviceDetail& level_) :
            subnet(subnet_), level(level_) {
        }
        virtual ~LevelSubnetDetailPrinter() {
        }
};
std::ostream& operator<<(std::ostream& stream, const LevelSubnetDetailPrinter& printer);

}

}

#endif /* SUBNETPRINTER_H_ */
