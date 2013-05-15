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
 * AlertOperation.cpp
 *
 *  Created on: Apr 12, 2010
 *      Author: Sorin.Bidian
 */

#include "AlertOperation.h"
#include "Model/Operations/smApplicationAlertTypes.h"

namespace NE {
namespace Model {
namespace Operations {

AlertOperation::AlertOperation(int alertType_, void * content_, Uint32 detectionTime_) :
    alertType(alertType_) {

    assert(content_);
    content = content_;
    detectionTime = detectionTime_;
}

AlertOperation::~AlertOperation() {
    //delete content if not null
    if (content) {
        //LOG_INFO("Content not null on destructor for alertOp of type " << alertType);

        switch (alertType) {
            case SMAlertTypes::DeviceJoin:
            {
                delete (AlertJoin*) content;
                content = NULL;
                return;
            }
            case SMAlertTypes::DeviceJoinFailed:
            {
                delete (AlertJoinFailed*) content;
                content = NULL;
                return;
            }
            case SMAlertTypes::DeviceLeave:
            {
                delete (AlertLeave*) content;
                content = NULL;
                return;
            }
            case SMAlertTypes::TransferStarted:
            {
                delete (AlertTransferStarted*) content;
                content = NULL;
                return;
            }
            case SMAlertTypes::TransferProgress:
            {
                delete (AlertTransferProgress*) content;
                content = NULL;
                return;
            }
            case SMAlertTypes::TransferEnded:
            {
                delete (AlertTransferEnded*) content;
                content = NULL;
                return;
            }
            case SMAlertTypes::ContractEstablishment:
            {
                delete (AlertContractEstablishment*) content;
                content = NULL;
                return;
            }
            case SMAlertTypes::ContractModification:
            {
                delete (AlertContractModification*) content;
                content = NULL;
                return;
            }
            case SMAlertTypes::ContractRefusal:
            {
                delete (AlertContractRefusal*) content;
                content = NULL;
                return;
            }
            case SMAlertTypes::ContractTermination:
            {
                delete (AlertContractTermination*) content;
                content = NULL;
                return;
            }
            case SMAlertTypes::ParentChange:
            {
                delete (AlertParentChange*) content;
                content = NULL;
                return;
            }
            case SMAlertTypes::BackupChange:
            {
                delete (AlertBackupChange*) content;
                content = NULL;
                return;
            }

            default:
                LOG_ERROR("Content not null for alert of unknown type: " << alertType);
                return;
        }
    }
}

}
}
}
