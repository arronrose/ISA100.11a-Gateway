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
 * @author catalin.pop, george.petrehus
 */
#include "ObjectsProvider.h"
#include "Common/NEException.h"
#include "AL/Isa100Object.h"
#include "AL/DMAP/ARMO.h"
#include "AL/DMAP/ASLMO.h"
#include "AL/DMAP/DMO.h"
#include "AL/DMAP/DMO.h"
#include "AL/DMAP/TLMO.h"
#include "AL/DMAP/UDO.h"
#include "AL/SMAP/DMSO.h"
#include "AL/SMAP/ARO.h"
#include "AL/SMAP/DSO.h"
#include "AL/SMAP/PSMO.h"
#include "AL/SMAP/STSO.h"
#include "AL/SMAP/SCO.h"
#include "AL/SMAP/SMO.h"
#include "AL/SMAP/DO.h"
#include "AL/SMAP/MVO.h"
#include "AL/SMAP/SOO.h"

using namespace Isa100::Common;
using namespace Isa100::AL::DMAP;
using namespace Isa100::AL::SMAP;

namespace Isa100 {
namespace AL {

ObjectsMap ObjectsProvider::objectsMapping;

ObjectsProvider::ObjectsProvider(MessageDispatcher::Ptr messageDispatcher_, NE::Model::IEngine* engine_,
            Isa100::Security::SecurityManager* securityManager_) :
    messageDispatcher(messageDispatcher_), engine(engine_), securityManager(securityManager_) {
}

ObjectsProvider::~ObjectsProvider() {
}

void ObjectsProvider::createIsa100Object(Isa100::AL::ObjectID::ObjectIDEnum objectID, TSAP::TSAP_Enum tsap,
            Isa100ObjectPointer &isa100Object) {

    if (tsap == TSAP::TSAP_DMAP) {
        switch (objectID) {
            // ---------------- DMAP objects -----------------
            case ObjectID::ID_DMO:
                isa100Object.reset(new DMO(tsap, engine));
            break;
            case ObjectID::ID_ARMO:
                isa100Object.reset(new ARMO(tsap, engine));
            break;

            case ObjectID::ID_UDO:
                isa100Object.reset(new UDO(tsap, engine));
            break;
            default:
                ostringstream msg;
                msg << "Object not found for ID=" << (int) objectID;
                throw NE::Common::NEException(msg.str());
        }
    } else if (tsap == TSAP::TSAP_SMAP) {
        switch (objectID) {
            case ObjectID::ID_DO1:
                isa100Object.reset(new DO(ObjectID::ID_DO1, tsap, engine));
            break;
            case ObjectID::ID_STSO:
                isa100Object.reset(new STSO(tsap, engine));
            break;

            case ObjectID::ID_SCO:
                isa100Object.reset(new SCO(tsap, engine));
            break;
            case ObjectID::ID_DMSO:
                isa100Object.reset(new DMSO(tsap, engine));
            break;
            case ObjectID::ID_SMO:
                isa100Object.reset(new SMO(tsap, engine));
            break;
            case ObjectID::ID_PSMO:
                isa100Object.reset(new PSMO(tsap, engine));
            break;
            case ObjectID::ID_UDO1:
                isa100Object.reset(new UDO(tsap, engine));
            break;
            case ObjectID::ID_ARO:
                isa100Object.reset(new ARO(tsap, engine));
            break;
            case ObjectID::ID_MVO:
                isa100Object.reset(new MVO(tsap, engine));
            break;
            case ObjectID::ID_SOO:
                isa100Object.reset(new SOO(tsap, engine));
            break;
            default: {
                ostringstream msg;
                msg << "Object not found for ID=" << (int) objectID;
                throw NE::Common::NEException(msg.str());
            }
        }
    } else {
        THROW_EX(NE::Common::NEException, "No TSAP with ID=" << (int)tsap)
        ;
    }

    isa100Object->messageDispatcher = messageDispatcher;
    isa100Object->securityManager = securityManager;
}

void ObjectsProvider::addObjectMapping(Uint8 const objectID, Isa100ObjectPointer const instance) {
    ObjectsProvider::objectsMapping[objectID] = instance;
}

}
}
