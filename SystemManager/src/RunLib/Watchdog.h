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
#ifndef WATCHDOG_H_
#define WATCHDOG_H_

#include "Common/logging.h"

namespace run_lib {

class Watchdog {
    private:
        /**
         * Declare the static log method.
         */
        LOG_DEF("I.r.Watchdog")

    public:
        Watchdog();

        virtual ~Watchdog();

        static void Alarm(int sig, int sec);

    private:
        struct AlarmStatus {
                AlarmStatus(int p_pid = 0, time_t p_sched = 0) :
                    pid(p_pid), sched(p_sched) {
                }

                int pid;
                time_t sched;
        };

};

}

#endif /*WATCHDOG_H_*/
