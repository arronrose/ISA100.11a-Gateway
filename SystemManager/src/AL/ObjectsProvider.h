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
#ifndef OBJECTSPROVIDER_H_
#define OBJECTSPROVIDER_H_

#include "AL/Isa100Object.h"
#include "IObjectsProvider.h"
#include "MessageDispatcher.h"
#include "Security/SecurityManager.h"

using namespace Isa100::Common;

namespace Isa100 {
namespace AL {

typedef std::map<Uint8, Isa100ObjectPointer> ObjectsMap;

class ObjectsProvider: public Isa100::AL::IObjectsProvider {
        LOG_DEF("I.A.ObjectsProvider");

    public:
        ObjectsProvider(MessageDispatcher::Ptr messageDispatcher, NE::Model::IEngine* engine,
                    Isa100::Security::SecurityManager* securityManager);
        virtual ~ObjectsProvider();

        /**
         * Implementation of method from  Isa100::AL::IObjectsProvider.
         * @see Isa100::AL::IObjectsProvider
         * @param objectID
         * @return
         */
        void createIsa100Object(Isa100::AL::ObjectID::ObjectIDEnum objectID, Isa100::Common::TSAP::TSAP_Enum tsap,
                    Isa100ObjectPointer &isa100Object);

        /**
         * Obtains the instance of ISAObject that has the objectID ID.
         * @param ObjectID::ObjectIDEnum objectID - the ID of the desired object.
         * @return IIsa100ObjectPointer - the instance of the object with id objectID.
         * @throws Isa100Exception - if object not found.
         */
        //        static Isa100ObjectPointer getObjectByID(Uint8 objectID);

        static void addObjectMapping(Uint8 const objectID, Isa100ObjectPointer const instance);

    private:
        static ObjectsMap objectsMapping;

        MessageDispatcher::Ptr messageDispatcher;
        NE::Model::IEngine* engine;
        Isa100::Security::SecurityManager* securityManager;
};

}
}

#endif /*OBJECTSPROVIDER_H_*/
