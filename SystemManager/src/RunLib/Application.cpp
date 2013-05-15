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
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fstream>
#include <errno.h>
#include <sys/time.h>

#include "Application.h"
#include "SignalsManager.h"
#include "Common/logging.h"
#include "Common/SettingsLogic.h"
#include "Version.h"
#include "AL/SMAP/MVO.h"
#include <iostream>
#include "Model/EngineProvider.h"

using std::cout;
using std::endl;

namespace run_lib {
LOG_DEF("I.run_lib");

void    HandlerFATAL(int p_signal);// SIGABRT/ SIGSEGV

/**
 * A function to handle out of memory situations.
 * Another situation this method can catch is when from a method that declares
 * that throws certain exception(s) a different type of exception is thrown.
 * This will hopefully allow more logging when the program crashes.
 */
void abort_signal_handler(int p_signal) {
    std::cout << "Signal received !!! p_signal : " << p_signal << std::endl;;
    LOG_FATAL("Signal received !!! p_signal : " << p_signal);
    Application::Stop();
}

Application::ApplicationState Application::m_state = APP_RESET;
bool Application::m_stop = false;
int Application::m_syncFd = 0;
string Application::m_pidFile;
string Application::m_lockFile;
struct itimerval Application::m_threadCheckTimer;
time_t Application::threadCheckTime;
const long Application::m_minTimerInterval = 50000;
const struct timeval Application::m_defaultAlarmInterval = { 10, 0 };
pthread_t Application::threadAPPid;


Application::Application() {
}

Application::~Application() {
}

/**
 * Makes the needed initializations
 *
 * @param const string& p_loggerName - the logger object name
 * @param const string& p_logConfFile - the full path of the logger configuration file
 * @param const string& p_pidFile - the full path of the pid file
 * @param const string& p_lockFile - the full path of the lock files
 */
int Application::init(const string& p_logConfFile, const string& p_pidFile, const string& p_lockFile) {

    if (m_state > APP_RESET) {
        return 1;
    }

    signal( SIGABRT, HandlerFATAL );/// do NOT use delayed processing with HandlerFATAL. Stack trace must be dumped on event
    signal( SIGSEGV, HandlerFATAL );/// do NOT use delayed processing with HandlerFATAL. Stack trace must be dumped on event
    signal( SIGFPE , HandlerFATAL );/// do NOT use delayed processing with HandlerFATAL. Stack trace must be dumped on event



    SignalsManager::Ignore(SIGTTOU);
    SignalsManager::Ignore(SIGTTIN);
    SignalsManager::Ignore(SIGTSTP);
    SignalsManager::Ignore(SIGPIPE);
    SignalsManager::Ignore(SIGCHLD);
    SignalsManager::Ignore(SIGALRM);//ignore alarm signal
    SignalsManager::Ignore(SIGUSR1);

    SignalsManager::Install(SIGTERM, HandlerSIGTERM);
    SignalsManager::Install(SIGINT, HandlerSIGTERM);
    SignalsManager::Install(SIGUSR2, HandlerSIG_USR2); // used to create SystemManager commands(sm_commands.ini).
    SignalsManager::Install(SIGHUP, HandlerSIG_HUP); // used to reload provisioning records(system_manager.ini) AND reload configuration files(config.ini, subnet.ini, subnet_xx.ini).

    Isa100::Common::SmSettingsLogic::instance().logConfFileName = p_logConfFile;
    NE::Common::SettingsLogic::managerVersion = SYSTEM_MANAGER_VERSION;
    LOG_INIT(p_logConfFile);

    time_t ltime;
    ltime = time(NULL);
    struct tm *Tm;
    Tm = localtime(&ltime);

    LOG_INFO("Starting System Manager version " << SYSTEM_MANAGER_VERSION << ". Started on  " << std::setw(2) << std::setfill('0')
                << Tm->tm_mday << ":" << std::setw(2) << std::setfill('0') << Tm->tm_mon + 1 << ":" << std::setw(2)
                << std::setfill('0') << Tm->tm_year + 1900 << "," << std::setw(2) << std::setfill('0') << Tm->tm_hour << ":"
                << std::setw(2) << std::setfill('0') << Tm->tm_min << ":" << std::setw(2) << std::setfill('0') << Tm->tm_sec);

    LOG_INFO("signal handlers initialized");

    Isa100::Common::SmSettingsLogic::instance().load();
    m_pidFile = Isa100::Common::SmSettingsLogic::instance().lockFilesFolder;
    m_pidFile += p_pidFile;
    m_lockFile = Isa100::Common::SmSettingsLogic::instance().lockFilesFolder;
    m_lockFile += p_lockFile;

    m_syncFd = open(m_lockFile.c_str(), O_RDWR | O_CREAT, 0666);
    if (flock(m_syncFd, LOCK_EX | LOCK_NB)) {
        LOG_FATAL("another instance of this application is already running - exiting");
        std::cout << "another instance of this application is already running - exiting";
        exit(4);
    }
    if (!SetCloseOnExec(m_syncFd)) {
        exit(5);
    }

    m_state = APP_INITIALIZED;

    return 1;
}

/**
 * Starts the application; returns the exit code
 */
int Application::run() {
    m_state = APP_RUNNINIG;
    m_stop = false;

    threadCheckTime = time(NULL);

    int exitCode = runImplementation();
    m_state = APP_STOPPED;

    return exitCode;
}

/**
 * Performs cleaning after application stopped.
 */
void Application::close() {
    LOG_INFO("APP closing...");
    if (m_syncFd > 0) {
        flock(m_syncFd, LOCK_UN);
        ::close(m_syncFd);
        m_syncFd = 0;
    }
    unlink(m_pidFile.c_str());
    unlink(m_lockFile.c_str());

    LOG_INFO("APP ENDED -- OK");
}

bool Application::Sleep(unsigned int microseconds, bool continueIfInterrupted, bool* interrupted) {
    struct timespec timeToSleep;
    struct timespec remainingTime;

    timeToSleep.tv_sec = microseconds / 1000000;
    timeToSleep.tv_nsec = (microseconds % 1000000) * 1000;
    while (true) {
        int result = nanosleep(&timeToSleep, &remainingTime);
        if (result != 0 && errno == EINTR && continueIfInterrupted && remainingTime.tv_sec != 0 && remainingTime.tv_nsec != 0) {
            timeToSleep.tv_sec = remainingTime.tv_sec;
            timeToSleep.tv_nsec = remainingTime.tv_nsec;
            continue;
        }
        if (interrupted != NULL) {
            *interrupted = result != 0 && errno == EINTR;
        }
        return result == 0;
    }
}

void Application::checkDeadlock(Uint32 currentTime) {

//    time_t currentTime = time(NULL);
    if (currentTime - threadCheckTime > 10) {
        threadCheckTime = currentTime;
        WritePIDFile();

    }
}

//exits gracefully
void Application::HandlerSIGTERM(int signal) {
//    pthread_t threadId = pthread_self();
    cout << getpid() << " : " << "HandlerSIGTERM: Received signal " << signal << endl;
    LOG_INFO(getpid() << " :" << "HandlerSIGTERM: Received signal " << signal);
    SignalsManager::Ignore(SIGTERM);
    SignalsManager::Ignore(SIGINT);
    Stop();
    cout << getpid() << " : " << "HandlerSIGTERM: Termination cleanup" << endl;
    LOG_INFO(getpid() << " :" << "HandlerSIGTERM: Termination cleanup");
    unlink(m_pidFile.c_str());
    unlink(m_lockFile.c_str());
    cout << getpid() << " : " << "HandlerSIGTERM: Exit TERM signal handler" << endl;
    LOG_INFO(getpid() << " : " << "HandlerSIGTERM: Exit TERM signal handler");
}

void Application::HandlerSIG_USR2(int) {
    SignalsManager::Ignore(SIGUSR2);
    //WARNIG: No logging should be made from Signal Hanlder. It may produce a deadlock with the SM process that write normal logging(ostring_stream use mutexes.).
    cout << getpid() << " : " << "HandlerSIGUSR2: Received USR2 signal. Creating SystemManager comand from sm_command.ini." << endl;

    Isa100::AL::SMAP::MVO::setFlagSignalCommands();


    // reregister the signal so that next time will be handles by this handler
    SignalsManager::Install(SIGUSR2, HandlerSIG_USR2);
}
void Application::HandlerSIG_HUP(int) {
    SignalsManager::Ignore(SIGHUP);
    //WARNIG: No logging should be made from Signal Hanlder. It may produce a deadlock with the SM process that write normal logging(ostring_stream use mutexes.).
    cout << getpid() << " : " << "HandlerSIGHUP: Received HUP signal. Reloading provisioning and configuration files: log4cpp.properties, config.ini, sm_subnet{_xx}.ini,system_manager.ini." << endl;

    Isa100::AL::SMAP::MVO::setFlagSignalConfigs();

    // reregister the signal so that next time will be handles by this handler
    SignalsManager::Install(SIGHUP, HandlerSIG_HUP);
}

int Application::SetCloseOnExec(int fd) {
    int flags;

    flags = fcntl(fd, F_GETFD);
    if (flags == -1) {
        LOG_ERROR("SetCloseOnExec: fcntl(F_GETFD)");
        return 0;
    }

    flags |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, flags) == -1) {
        LOG_ERROR("SetCloseOnExec: fcntl(F_SETFD)");
        return 0;
    }

    return 1;
}

void Application::WritePIDFile() {
    if (!Application::m_stop) {
        FILE* pidFile = fopen(Application::m_pidFile.c_str(), "w+");
        if (pidFile == NULL) {
            LOG_FATAL("unable to write the pid file - exiting");
            cout << "unable to write the pid file - exiting" << endl;
            exit(3);
        }

        fprintf(pidFile, "%d", getpid());
        fclose(pidFile);
    }
}

#ifndef APPLICATION_VERSION
#define APPLICATION_VERSION "not defined"
#endif

const char* Application::Version() {
    return APPLICATION_VERSION;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Handle out of memory / invalid access - fatal errros: SIGSEGV/SIGABRT - log2flash fault address
/// @retval none
/// @remarks To identify the crash: build a unstripped version from the
/// same source tree as the crashing program  (tag releases!).
/// The unstripped version can be found in Accessnode/out/<toolchain>/...
/// Use addr2line (from the correct toolchain) to find the crash line. Example for VR900:
/// /opt/nivis/m68k-unknown-linux-uclibc/bin/m68k-unknown-linux-uclibc-addr2line -f -i -C -e <exe> <addr_list_any_format>
/// <exe> is executable WITH debug info
/// <addr_list_any_format> is any of/several 0xXXXXXXXX xXXXXXXXX XXXXXXXX separated by spaces
/// @note
/// @note Do NOT use delayed processing (CSignalsManager) with HandlerFATAL. Stack trace must be dumped on event or is lost
/// @note
////////////////////////////////////////////////////////////////////////////////
#define BKTRACE(a) if(__builtin_frame_address(a)) { n += sprintf((str+n), "%p ", __builtin_return_address(a)); } else { bContinue = false;}

void HandlerFATAL(int p_signal)
{   static char str[4096];   /// 11 bytes per address. Watch for overflow
    int n=0;
    bool bContinue=true;
    *str=0;/// reset every time
    for ( int i=0; bContinue && (i<16) ; ++i)
    {   switch (i)  /// parameter for __builtin_frame_address must be a constant integer
        {   case 0: BKTRACE(0); break ;
// ARM doesn't support more than 1 stack unwind.
#if ! (defined(ARM) || defined(HW_TRINITY2))
            case 1: BKTRACE(1); break ;
            case 2: BKTRACE(2); break ;
            case 3: BKTRACE(3); break ;
            case 4: BKTRACE(4); break ;
            case 5: BKTRACE(5); break ;
            case 6: BKTRACE(6); break ;
            case 7: BKTRACE(7); break ;
            case 8: BKTRACE(8); break ;
            case 9: BKTRACE(9); break ;
            case 10:BKTRACE(10);break ;
            case 11:BKTRACE(11);break ;
            case 12:BKTRACE(12);break ;
            case 13:BKTRACE(13);break ;
            case 14:BKTRACE(14);break ;
            case 15:BKTRACE(15);break ;
#else	// defined(ARM) || defined(HW_TRINITY2)
			case 1:
			n+=snprintf( str+n, sizeof(str)-n-1,
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				);
			break;
#endif
        }
    }

#warning should integrated with log2flash
    std::cout << "PANIC [SystemManager] "
        << ( (p_signal == SIGSEGV) ? "SIGSEGV" : (p_signal == SIGABRT) ? "SIGABRT" : (p_signal == SIGFPE)  ? "SIGFPE" : "UNK" )
        << " " << p_signal << " . Backtrace " << str << std::endl;
    LOG_FATAL("PANIC [SystemManager] "
        << ( (p_signal == SIGSEGV) ? "SIGSEGV" : (p_signal == SIGABRT) ? "SIGABRT" : (p_signal == SIGFPE)  ? "SIGFPE" : "UNK" )
        << " " << p_signal << " . Backtrace " << str );

    exit(EXIT_FAILURE);
}

}

namespace boost
{
LOG_DEF("I.r.BoostAssertion");
/**
 * Function called by BOOST framework when an assert fails.
 */
void assertion_failed(char const * expr, char const * function, char const * file, long line)
{
	std::cerr << "ERROR on assertion: " << expr
	<< std::endl << "Function	: " << function
	<< std::endl << "File		: " << file
	<< std::endl << "Line		:" << line;

	LOG_ERROR("ERROR on boost assertion: " << expr
	<< std::endl << "Function	: " << function
	<< std::endl << "File		: " << file
	<< std::endl << "Line		:" << line);

	static char str[4096];   /// 11 bytes per address. Watch for overflow
	    int n=0;
	    bool bContinue=true;
	    *str=0;/// reset every time
	    for ( int i=0; bContinue && (i<16) ; ++i)
	    {   switch (i)  /// parameter for __builtin_frame_address must be a constant integer
	        {   case 0: BKTRACE(0); break ;
// ARM doesn't support more than 1 stack unwind.
#if !(defined(ARM) || defined(HW_TRINITY2))
	            case 1: BKTRACE(1); break ;
	            case 2: BKTRACE(2); break ;
	            case 3: BKTRACE(3); break ;
	            case 4: BKTRACE(4); break ;
	            case 5: BKTRACE(5); break ;
	            case 6: BKTRACE(6); break ;
	            case 7: BKTRACE(7); break ;
	            case 8: BKTRACE(8); break ;
	            case 9: BKTRACE(9); break ;
	            case 10:BKTRACE(10);break ;
	            case 11:BKTRACE(11);break ;
	            case 12:BKTRACE(12);break ;
	            case 13:BKTRACE(13);break ;
	            case 14:BKTRACE(14);break ;
	            case 15:BKTRACE(15);break ;
#else	// defined(ARM) || defined(HW_TRINITY2)
			case 1:
			n+=snprintf( str+n, sizeof(str)-n-1,
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				"%08x %08x %08x %08x %08x %08x %08x %08x "
				);
			break;
#endif
	        }
	    }

#warning should integrated with log2flash
	std::cout << "PANIC [SystemManager]" << " Backtrace " << std::endl << str << std::endl;
	LOG_FATAL(   "PANIC [SystemManager]" << " Backtrace " << std::endl << str );
}

} // namespace boost
