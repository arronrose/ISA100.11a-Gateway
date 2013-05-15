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
 * ChainAddNewContainerOnOldContainerConfirm.h
 *
 *  Created on: Mar 4, 2010
 *      Author: flori.parauan
 */

#ifndef CHAINADDNEWCONTAINERONOLDCONTAINERCONFIRM_H_
#define CHAINADDNEWCONTAINERONOLDCONTAINERCONFIRM_H_

#include "Model/Subnet.h"
#include "Common/logging.h"
#include "Model/Operations/OperationsContainer.h"
#include "Model/Operations/OperationsProcessor.h"

namespace NE {

namespace Model {

class TheoreticEngine;

class ChainAddNewContainerOnOldContainerConfirm {
        LOG_DEF("I.M.ChainAddNewContainerOnOldContainerConfirm");
        Subnet::PTR subnet;
        TheoreticEngine * theoreticEngine;
        NE::Model::Operations::OperationsProcessor* operationsProcessor;
        Address32 deviceAddress32;
        Address32 newParentAddress32;
        Address32 oldParentAddress32;

    public:
        ChainAddNewContainerOnOldContainerConfirm(Subnet::PTR subnet,TheoreticEngine * theoreticEngine_, NE::Model::Operations::OperationsProcessor* operationsProcessor_, Address32 deviceAddress32_, Address32 newParentAddress32_, Address32 oldParentAddress32_);
        virtual ~ChainAddNewContainerOnOldContainerConfirm();
        void process(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status);
};

typedef boost::shared_ptr<ChainAddNewContainerOnOldContainerConfirm> ChainAddNewContainerOnOldContainerConfirmPointer;

}
}
#endif /* CHAINADDNEWCONTAINERONOLDCONTAINERCONFIRM_H_ */
