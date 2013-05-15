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
#ifndef APPRUNNER_H_
#define APPRUNNER_H_

#include "AL/ProcessMessages.h"
#include "Common/logging.h"

namespace Isa100 {

class AppRunner {
        LOG_DEF("I.AppRunner")

    private:

        /**
         *  The time of the last logging.
         */
        time_t lastLogNetworkTime;

    public:
        //	Isa100::AL::ProcessRunnerPointer processDMAP;
        ProcessPointer processDMAP;
        ProcessPointer processSMAP;
        Isa100::AL::ProcessMessages& messagesProcessor;

    public:

        /**
         * Constructs the AppRunner and make him to execute the Process-es from AL.
         */
        AppRunner(ProcessPointer processDMAP_, ProcessPointer processSMAP_,
                  Isa100::AL::ProcessMessages& messagesProcessor_);

        ~AppRunner();

        void threadRun();


};
typedef boost::shared_ptr<AppRunner> AppRunnerPointer;

}

#endif /*APPRUNNER_H_*/
