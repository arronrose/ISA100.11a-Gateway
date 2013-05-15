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
 * AlertOperation.h
 *
 *  Created on: Apr 12, 2010
 *      Author: Sorin.Bidian
 */

#ifndef ALERTOPERATION_H_
#define ALERTOPERATION_H_

#include "Common/NETypes.h"
#include "Common/logging.h"
#include "smApplicationAlertTypes.h"

namespace NE {
namespace Model {
namespace Operations {

class AlertOperation {
    LOG_DEF("N.M.O.AlertOperation");

    protected:
        int alertType;

        void * content;

        Uint32 detectionTime; //TAI

    public:
        AlertOperation(int alertType, void * content, Uint32 detectionTime);
        virtual ~AlertOperation();

        int getAlertType() {
            return alertType;
        }

        void * getContent() {
            return content;
        }

        Uint32 getDetectionTime() {
            return detectionTime;
        }
};

typedef boost::shared_ptr<AlertOperation> AlertOperationPointer;

}
}
}

#endif /* ALERTOPERATION_H_ */
