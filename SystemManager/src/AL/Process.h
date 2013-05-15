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
 * @author catalin.pop, sorin.bidian, beniamin.tecar
 */
#ifndef PROCESS_H_
#define PROCESS_H_

#include "IObjectsProvider.h"
#include "ASL/Services/ASL_AlertAcknowledge_PrimitiveTypes.h"
#include "ASL/Services/ASL_AlertReport_PrimitiveTypes.h"
#include "ASL/Services/ASL_Service_PrimitiveTypes.h"
#include "ASL/Services/ASL_Publish_PrimitiveTypes.h"
#include "Misc/Synchronize/Queue.h"
#include "AL/Isa100Object.h"
#include <vector>
#include <queue>

using namespace Isa100::Misc::Synchronize;

namespace Isa100 {
namespace AL {

typedef std::vector<AL::ObjectID::ObjectIDEnum> ProcessObjectsIDsList;

typedef std::vector<Isa100::AL::Isa100ObjectPointer> ObjectsList;
typedef std::map<AL::ObjectID::ObjectIDEnum, ObjectsList> ProcessObjectsMap;

class ObjectNotFoundInProcess: public NE::Common::NEException {
    public:
        ObjectNotFoundInProcess(const std::string& message) :
            NEException(message) {

        }

        ObjectNotFoundInProcess(const char* message) :
            NEException(message) {

        }
};

/**
 *
 * @author Catalin Pop
 */
class Process {
        LOG_DEF("I.A.Process")

    private:
        /**
         * Transport layer service access point id for the process instance.
         */
        TSAP_ID tsap_id;

        /**
         *
         */
        ProcessObjectsMap isa100ObjectsMap;

        /**
         * The list of object ids handled by the process instance.
         */
        ProcessObjectsIDsList processObjectIDs;

        Isa100::AL::IObjectsProviderPointer objectProvider;

        int maxVmSize;
        int maxVmRSS;
        std::string lastObjectProcessed;

    public:

        Process(TSAP_ID tsap_id_, const ProcessObjectsIDsList &processObjectIDList,
                    const Isa100::AL::IObjectsProviderPointer &objectProvider_);

        virtual ~Process();

        void indicate(Isa100::ASL::Services::PrimitiveIndicationPointer primitiveRequest);
        void indicate(Isa100::ASL::Services::ASL_AlertReport_IndicationPointer primitiveIndication);
        void indicate(Isa100::ASL::Services::ASL_Publish_IndicationPointer primitiveIndication);
        void indicate(Isa100::ASL::Services::ASL_AlertAcknowledge_IndicationPointer primitiveIndication);

        void confirm(Isa100::ASL::Services::PrimitiveConfirmationPointer primitiveConfirmation);

        /**
         * Process the messages queue. The queue is processed one message at time and then the execute()
         * returns. The other messages will be processed at the next call of execute().
         * @throws Isa100::Common::Isa100Exception - if the queue contains a message for an unknown object
         * (an object that is not part of this process).
         * @param isOneSecondSignal - true in cycles where 1 second timer is signaling. Has true value 1 time on a second.
         */
        void execute(bool isOneSecondSignal);

        /**
         * Returns the list of object ids handled by the process instance.
         */
        ProcessObjectsIDsList& getProcessObjectsList();

        bool isObjectFromProcess(const AL::ObjectID::ObjectIDEnum objectID);

        int getNumberOfObjectsInstances();
        int getNumberOfObjectsInstances(const AL::ObjectID::ObjectIDEnum objectID);

        /**
         * Returns the transport layer service access point id for the process instance.
         */
        TSAP_ID getProcessTsap_id();

        bool hasJobToPerform();

        void toString(  std::string &objectString );

};

typedef boost::shared_ptr<Process> ProcessPointer;

}
}

#endif /*PROCESS_H_*/
