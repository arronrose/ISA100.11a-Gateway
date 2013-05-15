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
#include "ProcessesProvider.h"
#include "AL/ObjectsProvider.h"
#include "Common/smTypes.h"
#include "AL/ObjectsIDs.h"
using namespace Isa100::Common;

namespace Isa100 {
namespace AL {
namespace Detail {

ProcessObjectsIDsList getDMAPObjectsID() {
    ProcessObjectsIDsList list;
    list.push_back(ObjectID::ID_DMO);
    list.push_back(ObjectID::ID_ARMO);
    list.push_back(ObjectID::ID_DSMO);
    list.push_back(ObjectID::ID_DLMO);
    list.push_back(ObjectID::ID_NLMO);
    list.push_back(ObjectID::ID_TLMO);
    list.push_back(ObjectID::ID_ASLMO);
    list.push_back(ObjectID::ID_UDO);
    list.push_back(ObjectID::ID_HRCO);
    return list;
}

ProcessObjectsIDsList getSMAPObjectsID() {
    ProcessObjectsIDsList list;
    list.push_back(ObjectID::ID_STSO);
    list.push_back(ObjectID::ID_DSO);
    list.push_back(ObjectID::ID_SCO);
    list.push_back(ObjectID::ID_SMO);
    list.push_back(ObjectID::ID_DMSO);
    list.push_back(ObjectID::ID_PSMO);
    list.push_back(ObjectID::ID_UDO1);
    list.push_back(ObjectID::ID_ARO);
    list.push_back(ObjectID::ID_HRCO);
    list.push_back(ObjectID::ID_DO1);
    list.push_back(ObjectID::ID_DIO);
    list.push_back(ObjectID::ID_MVO);
    list.push_back(ObjectID::ID_PCO);
    list.push_back(ObjectID::ID_PERO);
    list.push_back(ObjectID::ID_PCSCO);
    list.push_back(ObjectID::ID_BLO);

    return list;
}


} // namespace Detail

MessageDispatcher::Ptr ProcessesProvider::messageDispatcher;
NE::Model::IEngine* ProcessesProvider::engine = NULL;
Isa100::Security::SecurityManager* ProcessesProvider::securityManager = NULL;

MessageDispatcher::Ptr ProcessesProvider::MessageDispatcherInstance() {
    if (!ProcessesProvider::messageDispatcher) {
        ProcessesProvider::messageDispatcher.reset(new MessageDispatcher());
    }

    return ProcessesProvider::messageDispatcher;
}

ProcessPointer& ProcessesProvider::getProcessByTSAP_ID(TSAP_ID tsap_id) {
    if (tsap_id == Isa100::Common::TSAP::TSAP_DMAP) {//is DMAP process
        static ProcessPointer processDMAP(
                    new Process(Isa100::Common::TSAP::TSAP_DMAP, Detail::getDMAPObjectsID(), Isa100::AL::IObjectsProviderPointer(
                                new Isa100::AL::ObjectsProvider(MessageDispatcherInstance(), engine, securityManager))));
        return processDMAP;
    } else if (tsap_id == Isa100::Common::TSAP::TSAP_SMAP) {
        static ProcessPointer processSMAP(
                    new Process(Isa100::Common::TSAP::TSAP_SMAP, Detail::getSMAPObjectsID(), Isa100::AL::IObjectsProviderPointer(
                                new Isa100::AL::ObjectsProvider(MessageDispatcherInstance(), engine, securityManager))));
        return processSMAP;
    } else {
        static ProcessPointer processUnknownTSAP;
        std::ostringstream stream;
        stream << "ProcessesProvider::getProcessByTSAP_ID -> No process for tsap_id=" << (int) tsap_id;
        LOG_ERROR(stream.str());
        return processUnknownTSAP;
    }
}

}
}
