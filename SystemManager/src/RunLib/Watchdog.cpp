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
 * @author george.petrehus, catalin.pop
 */
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <iostream>

#include "Watchdog.h"
#include "Application.h"

using std::cout;
using std::endl;

namespace run_lib {

Watchdog::Watchdog() {
}

Watchdog::~Watchdog() {
}

#define MAX_SIGNAL 32

void Watchdog::Alarm(int sig, int sec) {
    static AlarmStatus pSignals[MAX_SIGNAL];

    if (sig < 1 || sig >= MAX_SIGNAL) {
        return;
    }

    time_t nTime = time(NULL);

    if (pSignals[sig].pid && nTime < pSignals[sig].sched) {
        cout << (int) getpid() << " : " << "Watchdog::Alarm: Killing child " << pSignals[sig].pid
                    << endl;
        kill(pSignals[sig].pid, SIGKILL);
        waitpid(pSignals[sig].pid, NULL, 0);
        cout << (int) getpid() << " : " << "Watchdog::Alarm: Child " << pSignals[sig].pid
                    << " killed" << endl;
    }

    if (sec == 0) {
        cout << (int) getpid() << " : " << "Watchdog::Alarm: Called on application close" << endl;
        pSignals[sig].pid = 0;
        pSignals[sig].sched = 0;
        return;
    }

    if ((pSignals[sig].pid = fork()) == 0) {
        cout << (int) getpid() << " : " << "Watchdog::Alarm: Child process sleeping " << sec
                    << " seconds" << endl;
        run_lib::Application::Sleep(sec * 1000000);
        cout << (int) getpid() << " : " << "Watchdog::Alarm: Killing parent " << getppid() << endl;
        kill(getppid(), sig);
        cout << (int) getpid() << " : " << "Watchdog::Alarm: We are done, exiting" << endl;
        _exit(0);
    }
    cout << (int) getpid() << " : " << "Watchdog::Alarm: Parent - continue close process" << endl;
    pSignals[sig].sched = time(NULL) + sec;
}

}
