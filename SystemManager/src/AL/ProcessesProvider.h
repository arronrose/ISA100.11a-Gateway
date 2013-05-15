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
 * @author george.petrehus, radu.pop, andrei.petrut, sorin.bidian
 */
#ifndef PROCESSESPROVIDER_H_
#define PROCESSESPROVIDER_H_

#include "Process.h"
#include "MessageDispatcher.h"

using namespace Isa100::Common;

namespace Isa100 {
namespace AL {

/**
 * Provider for process. This class is used to obtain a reference to a process for a specified TSAP_ID.
 * @throws Isa100::Common::Isa100Exception - if no process for tsap_id is found.
 */
class ProcessesProvider {
        LOG_DEF("I.A.ProcessesProvider")

    public:

        static MessageDispatcher::Ptr messageDispatcher;
        static NE::Model::IEngine* engine;
        static Isa100::Security::SecurityManager* securityManager;

        static ProcessPointer& getProcessByTSAP_ID(TSAP_ID tsap_id);

        static MessageDispatcher::Ptr MessageDispatcherInstance();
};
}
}

#endif /*PROCESSESPROVIDER_H_*/
