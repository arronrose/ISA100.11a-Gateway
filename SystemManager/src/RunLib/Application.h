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
 * @author catalin.pop, george.petrehus, marcel.ionescu, flori.parauan, florin.muresan, beniamin.tecar
 */
#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <sys/time.h>
#include <time.h>

#include "Common/logging.h"
#include "Common/SmSettingsLogic.h"

#define NETWORK_EVENT "network_event"

namespace run_lib {

#define LOG4CPP_CONF_FILE "log4cpp.properties"

using std::string;

class Application {
    private:
        /**
         * Declare the static log method.
         */
        LOG_DEF("I.r.Application");

    public:
        typedef enum {
            APP_RESET = 0, APP_INITIALIZED, APP_RUNNINIG, APP_STOPPED
        } ApplicationState;

        /**
         * Default constructor
         */
        Application();

        /**
         * Destructor
         */
        virtual ~Application();

        /**
         * Makes the needed initializations
         *
         * @param const string& p_loggerName - the logger object name
         * @param const string& p_logConfFile - the full path of the logger configuration file
         * @param const string& p_pidFile - the full path of the pid file
         * @param const string& p_lockFile - the full path of the lock files
         */
        int init(const string& p_logConfFile, const string& p_pidFile, const string& p_lockFile);

        /**
         * Starts the application; returns the exit code
         */
        int run();

        /**
         * Performs cleaning after application stopped.
         */
        void close();

        /**
         * Produce a sleep of the thread that called this method.
         * @param microseconds - number of microseconds to sleep
         * @param continueIfInterrupted - if true the sleep will continue even if a interruption occurred.
         * @param interrupted will return if a interruption occurred.
         */
        static bool Sleep(unsigned int microseconds, bool continueIfInterrupted = false, bool* interrupted = NULL);

        /**
         * Returns true if the application is not running.
         */
        static bool IsRunning() {
            return m_state == APP_RUNNINIG;
        }

        /**
         * Returns true if application was stopped.
         */
        static bool IsStopped() {
            return m_state == APP_STOPPED;
        }

        /**
         * Returns true if a stop request was made.
         */
        static bool IsStopRequest() {
            return m_stop;
        }

        /**
         * Requests application end; the watchdog will terminate the application
         * after a certain time if the runImplementation() method didn't finish.
         */
        static void Stop() {
            m_stop = true;
        }

        /**
         * Returns the version as a string.
         */
        static const char* Version();

        static void checkDeadlock(Uint32 currentTime);

    protected:
        /**
         * Abstract method; implements the application run method. Returns the
         * exit code.
         */
        virtual int runImplementation() = 0;
        static void WritePIDFile();


    private:
        static void NotifyAllWaitConditions();
        static bool ThreadsFlagsSet();
        static void SetThreadsFlags(bool);

        static void HandlerSIGTERM(int);
        static void HandlerSIG_USR2(int);
        static void HandlerSIG_HUP(int);
        static int SetCloseOnExec(int);


        static ApplicationState m_state;
        static bool m_stop;
        static int m_syncFd;
        static string m_pidFile;
        static string m_lockFile;

        static struct itimerval m_threadCheckTimer;
        static time_t threadCheckTime;
        static const long m_minTimerInterval;

        static struct itimerval m_alarmInterval;
        static const struct timeval m_defaultAlarmInterval;

        static pthread_t threadAPPid;

};

}

#endif /*APPLICATION_H_*/
