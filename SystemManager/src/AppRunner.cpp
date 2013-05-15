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
 * @author catalin.pop, radu.pop, beniamin.tecar
 */
#include "AppRunner.h"

#include <exception>
#include "RunLib/Application.h"
#include "ASL/StackWrapper.h"
#include <AL/ProcessesProvider.h>
#include "AL/MessageDispatcher.h"
#include "SMState/SMStateLog.h"
#include "Stats/Isa100SMStateLog.h"
//#include <Misc/SimpleTimer.h>
//#include "Common/SmSettingsLogic.h"
#include <Shared/SimpleTimer.h>
#include "Model/EngineProvider.h"
#include "SMState/SMStateLog.h"

#include "DurationWatcher.h"

#include "Security/SecurityManager.h"
#include "Common/ClockSource.h"

using namespace Isa100::Common;
using namespace Isa100::Model;
using namespace NE::Misc::Marshall;
using namespace run_lib;

namespace Isa100 {

#define ACCELERATED_LINKS_REDUCTION_PERIOD 10

AppRunner::AppRunner(ProcessPointer processDMAP_, ProcessPointer processSMAP_, Isa100::AL::ProcessMessages& messagesProcessor_) :
    processDMAP(processDMAP_), processSMAP(processSMAP_), messagesProcessor(messagesProcessor_) {
    lastLogNetworkTime = 0;

}

AppRunner::~AppRunner() {
}

void AppRunner::threadRun() {
//    pthread_t threadId = pthread_self();
    std::cout << "SM APP THREAD: " << std::endl;
    //	Application::NewThreadFlag();
    LOG_DEBUG("started Application layer processing.");
    int loopCounter = 0;

    CSimpleTimer secondTimer(1000);
    bool loggingTimer = false;
    bool roleActivationTimer = false;
    bool graphRedundancyTimer = false;
    bool routesEvaluationTimer = false;
    bool garbageCollectionTimer = false;
    bool consistencyCheckTimer = false;
    bool acceleratedLinksReductionCheckTimer = false;
    bool inactiveDevicesRemovalTimer = false;
    bool expiredContractsTimer = false;
    bool findBetterParentTimer = false;
    bool dirtyContractsCheckTimer = false;
    bool udoContractsCheckTimer = false;
    // bool evaluateClientServerContracts = false;

    Uint32 rawSetTimer = SmSettingsLogic::instance().keysHardLifeTime * 3600 / 10;
    if (rawSetTimer > 30) {
       rawSetTimer = 30;
    }
    bool keyExpiringTimer = false;

    Uint16 hrcoCheckPeriod = 10;
    bool hrcoReconfigureCheckTimer = false;

    Uint16 ignoredDevicesCheckPeriod = 5 * 60;
    bool ignoredDevicesTimer = false;
    bool evaluateDirtyEdges = false;
    bool evaluateBlackList = false;
    bool evaluateRoutersAdvertising = false;
    bool evaluateFastDiscovery = false;


    CDurationWatcher dw;
    // periodic Engine signal
    NE::Common::EvaluationSignal evaluationSignal;
    bool isSecondSignal;
    Uint32 currentTime = time(NULL);
    NE::Common::ClockSource::setAproxCurrentTime(currentTime);


    while (!Application::IsStopRequest()) {

        loopCounter++;

        try {
            isSecondSignal = secondTimer.IsSignaling();

            if (isSecondSignal) {
                Stack_OneSecondTasks();
                secondTimer.SetTimer(1000);
                WATCH_DURATION_DEF(dw);
                currentTime = time(NULL);
                NE::Common::ClockSource::setAproxCurrentTime(currentTime);
                loggingTimer = loggingTimer ? true: (currentTime % SmSettingsLogic::instance().logNetworkStateInterval == 0);
                roleActivationTimer = roleActivationTimer ? true : (currentTime % SmSettingsLogic::instance().roleActivationPeriod == 0);
                graphRedundancyTimer = graphRedundancyTimer ? true : (currentTime % SmSettingsLogic::instance().graphRedundancyPeriod == 0);
                routesEvaluationTimer = routesEvaluationTimer ? true : (currentTime % SmSettingsLogic::instance().routesEvaluationPeriod == 0);
                garbageCollectionTimer = garbageCollectionTimer ? true : (currentTime % SmSettingsLogic::instance().garbageCollectionPeriod == 0);
                consistencyCheckTimer = consistencyCheckTimer ? true : (currentTime % SmSettingsLogic::instance().consistencyCheckPeriod == 0);
                acceleratedLinksReductionCheckTimer = acceleratedLinksReductionCheckTimer ? true : (currentTime % ACCELERATED_LINKS_REDUCTION_PERIOD == 0);
                inactiveDevicesRemovalTimer = inactiveDevicesRemovalTimer ? true : (currentTime % SmSettingsLogic::instance().networkMaxDeviceTimeout == 0);
                expiredContractsTimer = expiredContractsTimer ? true : (currentTime % SmSettingsLogic::instance().expiredContractsPeriod == 0);
                findBetterParentTimer = findBetterParentTimer ? true : (currentTime % SmSettingsLogic::instance().findBetterParentPeriod == 0);
                dirtyContractsCheckTimer = dirtyContractsCheckTimer ? true : (currentTime % SmSettingsLogic::instance().dirtyContractCheckPeriod == 0);
                udoContractsCheckTimer = udoContractsCheckTimer ? true : (currentTime % SmSettingsLogic::instance().udoContractsCheckPeriod == 0);
                // evaluateClientServerContracts = evaluateClientServerContracts ? true : (currentTime % SmSettingsLogic::instance().gwContractsCkeckPeriod == 0);
                keyExpiringTimer = keyExpiringTimer ? true : (currentTime % rawSetTimer == 0);
                hrcoReconfigureCheckTimer = hrcoReconfigureCheckTimer ? true : (currentTime % hrcoCheckPeriod == 0);
                ignoredDevicesTimer = ignoredDevicesTimer ? true : (currentTime % ignoredDevicesCheckPeriod == 0);
                evaluateDirtyEdges = evaluateDirtyEdges ? true :  (currentTime % SmSettingsLogic::instance().dirtyEdgesCkeckPeriod == 0);
                evaluateBlackList = evaluateBlackList ? true : (currentTime % SmSettingsLogic::instance().bloPeriod == 0);
                evaluateRoutersAdvertising = evaluateRoutersAdvertising ? true : (currentTime % SmSettingsLogic::instance().fullAdvertisingRoutersPeriod == 0);
                evaluateFastDiscovery = evaluateFastDiscovery ? true : (currentTime % SmSettingsLogic::instance().fastDiscoveryCheck == 0);
            }
            Application::checkDeadlock(currentTime);
            //                processReceivedPackets(loopCounter);
            WATCH_DURATION_DEF(dw);//LOG_TRACK_POINT_TIMED(nStepTimeout);

            messagesProcessor.Execute(currentTime);
            WATCH_DURATION_DEF(dw);

            processDMAP->execute(isSecondSignal);
            WATCH_DURATION_DEF(dw);

            processSMAP->execute(isSecondSignal);
            WATCH_DURATION_DEF(dw);

            AL::ProcessesProvider::MessageDispatcherInstance()->Execute();//process LOOPBACKS
            WATCH_DURATION_DEF(dw);

            Stack_Run();
            WATCH_DURATION_DEF(dw);

            if (loggingTimer) {
                Isa100SMState::Isa100SMStateLog::logSmStackContracts("Periodic log");
                Isa100SMState::Isa100SMStateLog::logSmStackRoutes("Periodic log");
                Isa100SMState::Isa100SMStateLog::logSmStackKeys("Periodic log");
                SMState::SMStateLog::logSubnetContainer(EngineProvider::getEngine()->getSubnetsContainer());
//                SMState::SMStateLog::logNetworkTopology(EngineProvider::getEngine()->getSubnetsContainer());

                loggingTimer = false;
                WATCH_DURATION_DEF(dw);
            } else if (consistencyCheckTimer) {
                EngineProvider::getEngine()->getSubnetsContainer().periodicPhyConsistencyCheck(
                            EngineProvider::getEngine()->getOperationsProcessor());
                consistencyCheckTimer = false;
                WATCH_DURATION_DEF(dw);
            } else if (inactiveDevicesRemovalTimer) {
                EngineProvider::getEngine()->verifyInactiveDevices((Uint32) SmSettingsLogic::instance().networkMaxDeviceTimeout);
                inactiveDevicesRemovalTimer = false;
                WATCH_DURATION_DEF(dw);
            } else if (expiredContractsTimer) {
                EngineProvider::getEngine()->periodicTerminateExpiredContracts();
                expiredContractsTimer = false;
                WATCH_DURATION_DEF(dw);
            } else if (keyExpiringTimer) {
                Security::SecurityManager securityManager;
                securityManager.updateExpiringKeys(2400); // detection time before actual soft life time (40 minutes)
                EngineProvider::getEngine()->removeExpiredKeys();

                keyExpiringTimer = false; //OBS - 10 and 9 above are correlated
                //if 10 and 9 are changed, the value replacing 10 should be greater than the value replacing 9
                WATCH_DURATION_DEF(dw);
            } else {
                evaluationSignal = 0;
                if (roleActivationTimer) {
                    setRoleActivationSignal(evaluationSignal);
                    EngineProvider::getEngine()->periodicEvaluation(evaluationSignal, time(NULL));
                    roleActivationTimer = false;
                    WATCH_DURATION_DEF(dw);
                } else if (graphRedundancyTimer) {
                    setGraphRedundancySignal(evaluationSignal);
                    EngineProvider::getEngine()->periodicEvaluation(evaluationSignal, time(NULL));
                    graphRedundancyTimer = false;
                    WATCH_DURATION_DEF(dw);
                } else if (routesEvaluationTimer) {
                    setRoutesEvaluationSignal(evaluationSignal);
                    EngineProvider::getEngine()->periodicEvaluation(evaluationSignal, time(NULL));
                    routesEvaluationTimer = false;
                    WATCH_DURATION_DEF(dw);
                } else if (garbageCollectionTimer) {
                    setGarbageCollectionSignal(evaluationSignal);
                    EngineProvider::getEngine()->periodicEvaluation(evaluationSignal, time(NULL));
                    garbageCollectionTimer = false;
                    WATCH_DURATION_DEF(dw);
                } else if (acceleratedLinksReductionCheckTimer) {
                    setRemoveAccLinksSignal(evaluationSignal);
                    EngineProvider::getEngine()->periodicEvaluation(evaluationSignal, time(NULL));
                    acceleratedLinksReductionCheckTimer = false;
                   WATCH_DURATION_DEF(dw);
                } else if (dirtyContractsCheckTimer) {
                    setDirtyContractsSignal(evaluationSignal);
                    EngineProvider::getEngine()->periodicEvaluation(evaluationSignal, time(NULL));
                    dirtyContractsCheckTimer = false;
                    WATCH_DURATION_DEF(dw);
                } else if (udoContractsCheckTimer) {
                    setUdoContractsSignal(evaluationSignal);
                    EngineProvider::getEngine()->periodicEvaluation(evaluationSignal, time(NULL));
                    udoContractsCheckTimer = false;
                    WATCH_DURATION_DEF(dw);
                } else if (hrcoReconfigureCheckTimer) {
                    setHRCOReconfigureSignal(evaluationSignal);
                    EngineProvider::getEngine()->periodicEvaluation(evaluationSignal, time(NULL));
                    hrcoReconfigureCheckTimer = false;
                    WATCH_DURATION_DEF(dw);
                } else if (ignoredDevicesTimer) {
                    setIgnoredDevicesSignal(evaluationSignal);
                    EngineProvider::getEngine()->periodicEvaluation(evaluationSignal, time(NULL));
                    ignoredDevicesTimer = false;
                    WATCH_DURATION_DEF(dw);
                } else if (findBetterParentTimer){
                    setFindBetterParentSignal(evaluationSignal);
                    EngineProvider::getEngine()->periodicEvaluation(evaluationSignal, time(NULL));
                    findBetterParentTimer = false;
                    WATCH_DURATION_DEF(dw);
                } else if (evaluateDirtyEdges) {
                    setDirtyEdgesSignal(evaluationSignal);
                    EngineProvider::getEngine()->periodicEvaluation(evaluationSignal, time(NULL));
                    evaluateDirtyEdges = false;
                    WATCH_DURATION_DEF(dw);
                } else if (evaluateBlackList) {
                    setBlackListCheckSignal(evaluationSignal);
                    EngineProvider::getEngine()->periodicEvaluation(evaluationSignal, time(NULL));
                    evaluateBlackList = false;
                    WATCH_DURATION_DEF(dw);
                } else if (evaluateRoutersAdvertising) {
                    setRoutersAdvertiseCheckSignal(evaluationSignal);
                    EngineProvider::getEngine()->periodicEvaluation(evaluationSignal, time(NULL));
                    evaluateRoutersAdvertising = false;
                    WATCH_DURATION_DEF(dw);
                } else if (evaluateFastDiscovery) {
                    setFastDiscoveryCheckSignal(evaluationSignal);
                    EngineProvider::getEngine()->periodicEvaluation(evaluationSignal, time(NULL));
                    evaluateFastDiscovery = false;
                    WATCH_DURATION_DEF(dw);
                }
            }
//            WATCH_DURATION_DEF(dw);
        } catch (AddressNotFoundException& exAddress) {
            LOG_ERROR("Address error : " << exAddress.what());
        } catch (StreamException& exStream) {
            LOG_ERROR("Stream error : " << exStream.what());
        } catch (NEException& ex) {
            //TODO verify if is needed to catch and continue loop. or break.
            LOG_ERROR(ex.what());
        } catch (std::exception& er) {
            //TODO verify if is needed to catch and continue loop. or break.
            LOG_ERROR("Unknown error occurred: " << er.what());
        } catch (...) {
            //TODO verify if is needed to catch and continue loop. or break.
            LOG_ERROR("Unknown error occurred");
        }

    }
    LOG_INFO("...APP loop ended");

}

}
