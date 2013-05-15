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
#include "Common/NEException.h"
#include "Common/NeUtils.h"
#include "RunLib/Application.h"
#include "AL/ObjectsProvider.h"
#include "Process.h"
#include "Stats/Isa100SMStateLog.h"
#include "Model/IEngineExceptions.h"
#include <algorithm>
#include <string>

using namespace Isa100::ASL::Services;
using namespace Isa100::Common;

namespace Isa100 {
namespace AL {

namespace Predicates {

class JobFinishedPredicate {
    public:

        JobFinishedPredicate() {
        }

        bool operator()(const Isa100ObjectPointer& object) {
            return (object->isJobFinished());
        }
};

class LifeTimeExpiredPredicate {
        LOG_DEF("I.A.LifeTimeExpiredPredicate")

        public:

        LifeTimeExpiredPredicate() {
        }

        bool operator()(const Isa100ObjectPointer& object) {

            bool lifeTimeExpired = object->isLifeTimeExpired();

            if (lifeTimeExpired) {

                LOG_ERROR("LifeTimeExpired for object " << *object);

                //object->cleanupOnError();
                object->lifeTimeExpired();
            }
            return lifeTimeExpired;
        }
};

}

Process::Process(TSAP_ID tsap_id_, const ProcessObjectsIDsList &processObjectIDList_,
            const Isa100::AL::IObjectsProviderPointer &objectProvider_) :
    tsap_id(tsap_id_), processObjectIDs(processObjectIDList_), objectProvider(objectProvider_) {

	maxVmSize = 0;
	maxVmRSS = 0;

    // TODO: Beni - uncomment
    if (tsap_id == TSAP::TSAP_SMAP) {
        {
            Isa100::AL::ObjectID::ObjectIDEnum objectID = Isa100::AL::ObjectID::ID_SOO;
            Isa100::AL::Isa100ObjectPointer isa100Object;
            objectProvider->createIsa100Object(objectID, (TSAP::TSAP_Enum) tsap_id, isa100Object);
            isa100ObjectsMap[objectID].push_back(isa100Object);
        }

        {
            Isa100::AL::ObjectID::ObjectIDEnum objectID = Isa100::AL::ObjectID::ID_MVO;
            Isa100::AL::Isa100ObjectPointer isa100Object;
            objectProvider->createIsa100Object(objectID, (TSAP::TSAP_Enum) tsap_id, isa100Object);
            isa100ObjectsMap[objectID].push_back(isa100Object);
        }
    } else if (tsap_id == TSAP::TSAP_DMAP) {
        {
            Isa100::AL::ObjectID::ObjectIDEnum objectID = Isa100::AL::ObjectID::ID_ARMO;
            Isa100::AL::Isa100ObjectPointer isa100Object;
            objectProvider->createIsa100Object(objectID, (TSAP::TSAP_Enum) tsap_id, isa100Object);
            isa100ObjectsMap[objectID].push_back(isa100Object);
        }
    }
}

Process::~Process() {

}

void Process::indicate(PrimitiveIndicationPointer primitiveIndication) {

    Isa100::AL::Isa100ObjectPointer isa100Object;
    try {
        LOG_DEBUG("TSAP=" << (int) tsap_id << " " << primitiveIndication->toString());
        Isa100::AL::ObjectID::ObjectIDEnum serverObjectID = primitiveIndication->serverObject;
        ProcessObjectsIDsList::iterator iterFindId = std::find(processObjectIDs.begin(), processObjectIDs.end(),
                    serverObjectID);
        if (iterFindId == processObjectIDs.end()) {//if object with that ID is not contained in this process throw
            THROW_EX(ObjectNotFoundInProcess, "Process with TSAP=" << (int)tsap_id << ", does not contain object with ID=" << serverObjectID);
        }

        //Deny the access to DMAP. Allow only for DMO (for join of BBR and GW)
        if (tsap_id == TSAP::TSAP_DMAP && (serverObjectID != Isa100::AL::ObjectID::ID_DMO || primitiveIndication->clientObject != Isa100::AL::ObjectID::ID_DMO)){
            LOG_ERROR("Access to DMAP not allowed for request " << primitiveIndication->toString());
            return;
        }

        ProcessObjectsMap::iterator mapIterator = isa100ObjectsMap.find(serverObjectID);
        if (mapIterator == isa100ObjectsMap.end()) {
            objectProvider->createIsa100Object(serverObjectID, (TSAP::TSAP_Enum) tsap_id, isa100Object);
            isa100ObjectsMap[primitiveIndication->serverObject].push_back(isa100Object);
        } else {
            //iterate through vector with all instances of serverObjectID and ask if it expects current indication
            for (ObjectsList::iterator it = mapIterator->second.begin(); it != mapIterator->second.end(); it++) {
                try {
                    if ((*it)->expectIndicate(primitiveIndication)) {
                        isa100Object = *it;
                        break;
                    }
                } catch (std::exception& ex) {
                    LOG_ERROR("execute() : expectIndicate failed= " << ex.what() << " - " << *it);
                    //(*it)->cleanupOnError();
                }
            }

            if (!isa100Object) {
                objectProvider->createIsa100Object(serverObjectID, (TSAP::TSAP_Enum) tsap_id, isa100Object);
                isa100ObjectsMap[primitiveIndication->serverObject].push_back(isa100Object);
            }
        }

        try {
            isa100Object->indicateObject(primitiveIndication);
        } catch (std::exception& ex) {
            LOG_ERROR("execute() : indicate failed= " << ex.what() << " - " << *isa100Object);
            //isa100Object->cleanupOnError();
        }

    } catch (std::exception& ex2) {
        if (isa100Object != NULL){
            LOG_ERROR("execute() : " << ex2.what() << ", object=" << *isa100Object);
        } else {
            LOG_ERROR("execute() : " << ex2.what() << ", object=NULL");
        }

    } catch (...) {
        if (isa100Object != NULL){
            LOG_ERROR("indicate: unknown exception: " << *isa100Object);
        } else {
            LOG_ERROR("indicate: unknown exception: object=NULL");
        }

    }
}

void Process::indicate(ASL_AlertReport_IndicationPointer primitiveIndication) {
    LOG_DEBUG("indicate() : _TSAP_ID_=" << (int) tsap_id << ", indicate(): " << primitiveIndication->toString());

    Isa100::AL::Isa100ObjectPointer isa100Object;

    Isa100::AL::ObjectID::ObjectIDEnum serverObjectID = primitiveIndication->sinkObject;
    ProcessObjectsIDsList::iterator iterFindId = std::find(processObjectIDs.begin(), processObjectIDs.end(),
                serverObjectID);

    if (iterFindId == processObjectIDs.end()) {//if object with that ID is not contained in this process throw
        THROW_EX(ObjectNotFoundInProcess, "Process with TSAP=" << (int)tsap_id << ", does not contain object with ID=" << serverObjectID);
    }

    //Deny the access to DMAP.
    if (tsap_id == TSAP::TSAP_DMAP){
        LOG_ERROR("Access to DMAP not allowed for alert " << primitiveIndication->toString());
        return;
    }

    ProcessObjectsMap::iterator mapIterator = isa100ObjectsMap.find(serverObjectID);
    if (mapIterator == isa100ObjectsMap.end()) {
        //        THROW_EX(NE::Common::NEException, "No object with ID=" << serverObjectID
        //					<< "to process the alert with ID= " << primitiveIndication->alertReport->requestID);
        objectProvider->createIsa100Object(serverObjectID, (TSAP::TSAP_Enum) tsap_id, isa100Object);
        isa100ObjectsMap[primitiveIndication->sinkObject].push_back(isa100Object);
    } else {
        isa100Object = *mapIterator->second.begin();
    }

    try {
        isa100Object->indicate(primitiveIndication);
    } catch (std::exception& ex) {
        LOG_ERROR("execute() : AlertReport indicate failed= " << ex.what() << " - " << *isa100Object);
        //isa100Object->cleanupOnError();
    }
}

void Process::indicate(ASL_Publish_IndicationPointer primitiveIndication) {
    std::string objectString;
    primitiveIndication->toString(objectString);
    LOG_DEBUG("indicate() : _TSAP_ID_=" << (int) tsap_id << ", indicate(): " << objectString);

    Isa100::AL::ObjectID::ObjectIDEnum serverObjectID = primitiveIndication->subscribingObject;
    ProcessObjectsIDsList::iterator iterFindId = std::find(processObjectIDs.begin(), processObjectIDs.end(),
                serverObjectID);

    if (iterFindId == processObjectIDs.end()) {//if object with that ID is not contained in this process throw
        THROW_EX(ObjectNotFoundInProcess, "Process with TSAP=" << (int)tsap_id << ", does not contain object with ID=" << serverObjectID);
    }

    //Deny the access to DMAP.
    if (tsap_id == TSAP::TSAP_DMAP ){
        std::string value;
        primitiveIndication->toString(value);
        LOG_ERROR("Access to DMAP not allowed for publication " << value);
        return;
    }

    Isa100::AL::Isa100ObjectPointer isa100Object;

    ProcessObjectsMap::iterator mapIterator = isa100ObjectsMap.find(serverObjectID);
    if (mapIterator == isa100ObjectsMap.end()) {
        //THROW_EX(NE::Common::NEException, "No object with ID=" << serverObjectID << "to process the publish with ID= " << primitiveIndication->apduPublish->requestID);
        objectProvider->createIsa100Object(serverObjectID, (TSAP::TSAP_Enum) tsap_id, isa100Object);
        isa100ObjectsMap[primitiveIndication->subscribingObject].push_back(isa100Object);
    } else {
        isa100Object = *mapIterator->second.begin();
    }

    try {
        isa100Object->indicate(primitiveIndication);
    } catch (std::exception& ex) {
        LOG_ERROR("execute() : Publish indicate failed= " << ex.what() << " - " << isa100Object);
        //isa100Object->cleanupOnError();
    }
}

void Process::indicate(ASL_AlertAcknowledge_IndicationPointer primitiveIndication) {
    LOG_DEBUG("indicate() : _TSAP_ID_=" << (int) tsap_id << ", indicate(): " << primitiveIndication->toString());

    Isa100::AL::Isa100ObjectPointer isa100Object;

    //Isa100::AL::ObjectID::ObjectIDEnum serverObjectID = primitiveIndication->destinationObject;
    Isa100::AL::ObjectID::ObjectIDEnum serverObjectID = primitiveIndication->sourceObject;
    ProcessObjectsIDsList::iterator iterFindId = std::find(processObjectIDs.begin(), processObjectIDs.end(),
                serverObjectID);

    if (iterFindId == processObjectIDs.end()) {//if object with that ID is not contained in this process throw
        THROW_EX(ObjectNotFoundInProcess, "Process with TSAP=" << (int)tsap_id << ", does not contain object with ID=" << serverObjectID);
    }

    ProcessObjectsMap::iterator mapIterator = isa100ObjectsMap.find(serverObjectID);
    if (mapIterator == isa100ObjectsMap.end()) {
        //        THROW_EX(NE::Common::NEException, "No object with ID=" << serverObjectID
        //                  << "to process the alert with ID= " << primitiveIndication->alertReport->requestID);
        objectProvider->createIsa100Object(serverObjectID, (TSAP::TSAP_Enum) tsap_id, isa100Object);
        isa100ObjectsMap[primitiveIndication->destinationObject].push_back(isa100Object);
    } else {
        isa100Object = *mapIterator->second.begin();
    }

    try {
        isa100Object->indicate(primitiveIndication);
    } catch (std::exception& ex) {
        LOG_ERROR("execute() : AlertAck indicate failed= " << ex.what() << " - " << *isa100Object);
    }
}

void Process::confirm(PrimitiveConfirmationPointer primitiveConfirmation) {
    //    LOG_DEBUG("confirm(): _TSAP_ID_=" << (int) tsap_id);
    //    LOG_DEBUG("confirm(): _TSAP_ID_=" << (int) tsap_id << " " << primitiveConfirmation->toString());
    //Isa100SMState::Isa100SMStateLog::logObjectInstances("confirm(): Process state: BEGIN ", toString());
    //LOG_DEBUG("confirm(): Process state: BEGIN\n" << toString());

    Isa100::AL::ObjectID::ObjectIDEnum clientObjectID = primitiveConfirmation->clientObject;

    if (isa100ObjectsMap.find(clientObjectID) == isa100ObjectsMap.end()) {
        LOG_ERROR("confirm(): No object with ID=" << (int) clientObjectID << " expect Response for ReqID=" << (int)primitiveConfirmation->apduResponse->appHandle);
        return;
    }

    ObjectsList& objectsList = isa100ObjectsMap[clientObjectID];
    ObjectsList::iterator it = objectsList.begin();

    while (it != objectsList.end()) {
        try {
            if ((*it)->expectConfirm(primitiveConfirmation)) {
                break;
            }
        } catch (std::exception& ex) {
            LOG_ERROR("execute() : expectConfirm failed= " << ex.what() << " - " << *it);
            //(*it)->cleanupOnError();
            break;
        }
        it++;
    }

    if (it != objectsList.end()) {
        LOG_DEBUG("Confirm for " << **it << " " << primitiveConfirmation->toString());

        try {
            (*it)->confirm(primitiveConfirmation);
        } catch (std::exception& ex) {
            LOG_ERROR("execute() : confirm failed= " << ex.what() << " - " << *it);
            //(*it)->cleanupOnError();
        }
        if ((*it)->isJobFinished()) {
            objectsList.erase(it);
        }
    } else {
        LOG_ERROR("confirm(): Unexpected confirm! confirm=" <<  primitiveConfirmation->toString());
    }

    //Isa100SMState::Isa100SMStateLog::logObjectInstances("confirm(): confirm Process state: END", toString());
}

void Process::execute(bool isOneSecondSignal) {
	if (!isOneSecondSignal) { //call objects execute & verify finished objects only ONE time per second, NOT at every cycle
		return;
	}

    try {
		int newVmSize = NeUtils::getVmSize();
		if (newVmSize > maxVmSize) {
			LOG_INFO("STATUS old VmSize = " << maxVmSize << ", new VmSize = " << newVmSize
					<< ", diff = " << newVmSize - maxVmSize << ", lastObjectProcessed=" << lastObjectProcessed);
			maxVmSize = newVmSize;
		}

		int newVmRSS = NeUtils::getVmRSS();
		if (newVmRSS > maxVmRSS) {
			LOG_INFO("STATUS old VmRSS = " << maxVmRSS << ", new VmRSS = " << newVmRSS
					<< ", diff = " << newVmRSS - maxVmRSS << ", lastObjectProcessed=" << lastObjectProcessed);
			maxVmRSS = newVmRSS;
		}

		//std::string beforeState = toString();
        Uint32 currentTime = time(NULL);
		for (ProcessObjectsMap::iterator itMap = isa100ObjectsMap.begin(); itMap != isa100ObjectsMap.end(); itMap++) {

			// remove JobFinished instances
			ObjectsList::iterator itRemJobFinish = std::remove_if(itMap->second.begin(), itMap->second.end(),
						Predicates::JobFinishedPredicate());

			//bool removed = (itRemJobFinish != itMap->second.end());
			itMap->second.erase(itRemJobFinish, itMap->second.end());
			for (int i = itMap->second.size() - 1; i >= 0; --i) {

				try {
					(*(itMap->second.begin() + i))->execute(currentTime);
				} catch (NE::Common::NEException& ex) {
					LOG_ERROR("execute() : " << ex.what() << ", object=" << (*(itMap->second.begin() + i)));
					itMap->second.erase(itMap->second.begin() + i);
				}
			}
		}
    } catch (std::exception& ex) {
        LOG_ERROR("execute() : " << ex.what());
    } catch (...) {
        LOG_ERROR("execute: unknown exception:");
    }
}

ProcessObjectsIDsList& Process::getProcessObjectsList() {
    return this->processObjectIDs;
}

bool Process::isObjectFromProcess(const AL::ObjectID::ObjectIDEnum objectID) {
    return (std::find(processObjectIDs.begin(), processObjectIDs.end(), objectID) != processObjectIDs.end());
}

int Process::getNumberOfObjectsInstances() {
    int numberOfInstances = 0;
    for (ProcessObjectsMap::iterator itMap = isa100ObjectsMap.begin(); itMap != isa100ObjectsMap.end(); itMap++) {
        numberOfInstances += itMap->second.size();
    }
    return numberOfInstances;
}

int Process::getNumberOfObjectsInstances(const AL::ObjectID::ObjectIDEnum objectID) {
    return isa100ObjectsMap[objectID].size();
}

TSAP_ID Process::getProcessTsap_id() {
    return tsap_id;
}

bool Process::hasJobToPerform() {
    return (isa100ObjectsMap.size() > 0);
}

void  Process::toString( std::string &objectString) {
    std::ostringstream stream;
    stream << "_TSAP_ID_=" << (int) tsap_id << std::endl;
    for (ProcessObjectsMap::iterator itMap = isa100ObjectsMap.begin(); itMap != isa100ObjectsMap.end(); itMap++) {
        if (itMap->second.size() > 0) {
//            std::string objectIdString;
//            Isa100::AL::ObjectID::toString((int) itMap->first, (Isa100::Common::TSAP::TSAP_Enum) tsap_id, objectIdString);
//            stream << "Object: " << objectIdString << std::endl;

            for (ObjectsList::iterator itList = itMap->second.begin(); itList != itMap->second.end(); ++itList) {
                try {
                    stream << "\t " << *itList << std::endl;
                } catch (NE::Common::NEException& ex) {
                    LOG_ERROR("toString() : " << ex.what());
                }
            }
        }
    }

    objectString = stream.str();
}

}
}
