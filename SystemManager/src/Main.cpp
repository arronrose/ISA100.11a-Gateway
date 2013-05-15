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
 * @author catalin.pop, radu.pop, beniamin.tecar, eduard.budulea
 */
#include "Main.h"
#include <iostream>
#include <fstream>
#include <sys/times.h>

#include <ASL/StackWrapper.h>

#include "Common/SmSettingsLogic.h"
#include "Common/SettingsLogic.h"
#include "AL/ProcessesProvider.h"
#include "AL/DMAP/TLMO.h"
#include "AL/ProcessMessages.h"
#include <Model/EngineProvider.h>
#include <Model/IEngine.h>
#include "Version.h"
#include "AppRunner.h"
#include "malloc.h"

using namespace Isa100;
using namespace Isa100::Common;
using namespace Isa100::AL;
using namespace Isa100::ASL;

Main::Main() {
}

Main::~Main() {
    delete networkEngine;
    delete securityManager;
}

int Main::runImplementation() {
    try {
        WritePIDFile();//write the PID as first task. Otherwise my produce a false detection of lock(by watchdog) on SM startup

        networkEngine = new NE::Model::NetworkEngine(&SmSettingsLogic::instance());
        Isa100::Model::EngineProvider::setEngine(networkEngine);

        securityManager = new Isa100::Security::SecurityManager();

        Uint16 smAddress16 = (Uint16) Isa100::Model::EngineProvider::getEngine()->getAddress32(
                    SmSettingsLogic::instance().managerAddress64);

        LOG_DEBUG("Added address for SM: 16=" << (int) smAddress16
                    << ", 64=" << SmSettingsLogic::instance().managerAddress64.toString()
                    << ", 128=" << SmSettingsLogic::instance().managerAddress128.toString());

        Stack_Init(SmSettingsLogic::instance().managerAddress128, // sm address
                    (Uint16) SmSettingsLogic::instance().listenPort, // listen port
                    SmSettingsLogic::instance().managerAddress64, // sm address64
                    SmSettingsLogic::instance().managerAddress64); // sec manager address64

        TSAP_ID tsap_id_DMAP = 0;
        TSAP_ID tsap_id_SMAP = 1;

        ProcessesProvider::engine = networkEngine;
        ProcessesProvider::securityManager = securityManager;

        ProcessPointer processDMAP = ProcessesProvider::getProcessByTSAP_ID(tsap_id_DMAP);
        ProcessPointer processSMAP = ProcessesProvider::getProcessByTSAP_ID(tsap_id_SMAP);

        ProcessMessages messagesProcessor(processDMAP, processSMAP);

        Isa100::AppRunnerPointer appRunner(new Isa100::AppRunner(processDMAP, processSMAP, messagesProcessor));

        appRunner->threadRun();

        Stack_Release();

    } catch (NEException& ex) {
        LOG_ERROR("Exception generated in main.");
        LOG_ERROR(ex.what());
    } catch (...) {
        LOG_ERROR("Unknown exception has occurred in runImplementation().");
    }

    LOG_DEBUG("SM main ended.");
    return 0;
}

/**
 * Main function of the app.
 * Argument 1: log4cpp.properties file path
 * Argument 2: config.ini file path
 * Error codes on return:
 * 0 - ended successfully
 * 1 - the file log4cpp.properties could not be found.
 * 2 - exited because of a hanged thread.
 * 3 - exited because the pid file could not be written.
 * 4 - exited because another instance of this app is running
 * 5 - other error wile working with pid file
 */
int main(int argc, char **argv) {
	mallopt(M_MMAP_THRESHOLD, 1024);
    Main mainNetworkManager;
    time_t ltime;
    ltime = time(NULL);
    struct tm *Tm;
    Tm = localtime(&ltime);

    std::cout << "Starting SM version " << SYSTEM_MANAGER_VERSION << ". Started on  " << std::setw(2) << std::setfill('0')
                << Tm->tm_mday << ":" << std::setw(2) << std::setfill('0') << Tm->tm_mon + 1 << ":" << std::setw(2)
                << std::setfill('0') << Tm->tm_year + 1900 << "," << std::setw(2) << std::setfill('0') << Tm->tm_hour << ":"
                << std::setw(2) << std::setfill('0') << Tm->tm_min << ":" << std::setw(2) << std::setfill('0') << Tm->tm_sec;
    //	std::cout << "Started at" << time(NULL);
    std::cout << std::endl;

    struct tms t;
    srand( (time(NULL) + times(&t)) ^ 0x1F2A15C3);//Initialize seed for random number generator

    std::string log4cppConfFileName;
    if (argc > 1) {
        log4cppConfFileName = argv[1];
        if (argc > 2) {
            SmSettingsLogic::instance().commonIniFileName = argv[2];
        }
    } else {
        log4cppConfFileName = LOG4CPP_CONF_FILE;
    }

    std::ifstream log4cppConfFile(log4cppConfFileName.c_str());
    if (!log4cppConfFile.is_open()) {
        log4cppConfFileName = NIVIS_PROFILE LOG4CPP_CONF_FILE;
        log4cppConfFile.open(log4cppConfFileName.c_str());
        if (!log4cppConfFile.is_open()) {
            std::cout << "Unable to open the log4cpp configuration file.";
            exit(1);
        }
    } else {
        //		std::cout << "log4cpp.properties is open by other application.";
    }

    mainNetworkManager.init(log4cppConfFileName, "SystemManager.pid", "SystemManager.flock");
    mainNetworkManager.run();
    mainNetworkManager.close();
    std::cout << "SM main ended.";

}

