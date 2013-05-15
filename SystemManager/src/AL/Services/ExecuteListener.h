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

/**
 * @author george.petrehus, catalin.pop
 */
#ifndef EXECUTELISTENER_H_
#define EXECUTELISTENER_H_

#include "Common/Objects/ExecuteRequest.h"
#include "Common/Objects/ExecuteResponse.h"

namespace Isa100 {
namespace AL {
namespace Services {

using namespace Isa100::Common;
using namespace Isa100::Common::Objects;

class ExecuteListener;
typedef boost::shared_ptr<ExecuteListener> ExecuteListenerPointer;

class ExecuteListener {
        ObjectIDEnum idListener;

    public:
        ExecuteListener(ObjectIDEnum idListener) {
            idListener = idListener;
        }
        virtual ~ExecuteListener();

        ObjectIDEnum id() {
            return idListener;
        }

        void indication(ServicePriority::ServicePriority priority, TransmissionTime endToEndTransmissionTime,
                    TLDE_SAP_Number serverTLDE_SAP, ClientNetworkAddress clientNetworkAddress,
                    TLDE_SAP_Number clientTLDE_SAP, const ExecuteRequest& executeRequest);

        void confirmation(ServicePriority::ServicePriority priority, TransmissionTime endToEndTransmissionTime,
                    ServerNetworkAddress serverNetworkAddress, TLDE_SAP_Number serverTLDE_SAP,
                    TLDE_SAP_Number clientTLDE_SAP, const ExecuteResponse& executeResponse);

};

}
}
}

#endif /*EXECUTE_H_*/
