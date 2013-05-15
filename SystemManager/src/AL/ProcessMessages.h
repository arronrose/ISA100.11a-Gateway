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

/*
 * ProcessMessages.h
 *
 *  Created on: Mar 8, 2009
 *      Author: Andy
 */

#ifndef PROCESSMESSAGES_H_
#define PROCESSMESSAGES_H_

#include <boost/bind.hpp>

#include "AL/ActiveDevicesTable.h"
#include "ASL/StackWrapper.h"
#include <typedef.h>
#include "Process.h"
#include "Stats/Cmds.h"
#include "AL/DuplicateRequests.h"
#include "Common/smTypes.h"

namespace Isa100 {
namespace AL {

typedef std::vector<ProcessPointer> ProcessList;
/**
 * Default sleep time for stack. Time to wait for a package before exiting with timout.
 * Value in microseconds (usec). 1000 = 1ms (1 milisecond)
 */
#define DEFAULT_SLEEP_TIME 20000

/**
 * Responsible with dispatching incoming messages from the stack to the appropriate Process.
 */
class ProcessMessages
{
public:
	ProcessMessages( const ProcessPointer& processDMAP, const ProcessPointer& processSMAP) {
	    processes.push_back( processDMAP );
	    processes.push_back( processSMAP );
	}


	/**
	 * Executes the message processing queue once. It will sleep if no message is avaiable to process.
	 */
	void Execute(Uint32 currentTime)
	{
        for (ProcessList::const_iterator it = processes.begin();
            it != processes.end(); ++it) {
            TSAP_ID tsapId = (*it)->getProcessTsap_id();
            Stack_WaitForAPDU( (uint8)tsapId, DEFAULT_SLEEP_TIME, *this, *it, currentTime);
        }
	}

	void IndicateCallbackFn(Isa100::ASL::Services::PrimitiveIndicationPointer indication, ProcessPointer process, Uint32 currentTime)
	{
		// ActiveDevicesTable::reportActivityTime(indication->clientNetworkAddress, time(NULL));

		if (DuplicateRequests::isDuplicateRequest(indication->clientNetworkAddress, indication->apduRequest->appHandle, indication->clientTSAP_ID, currentTime)) {
			// the requests are discarded
			std::string msg = " DUPLICATE MESSAGE !!!";
			Isa100::Stats::Cmds::logInfo(indication, msg);
		}
		else {
			Isa100::Stats::Cmds::logInfo(indication);
			process->indicate(indication);
		}
	}

	void ConfirmCallbackFn(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm, ProcessPointer process)
	{
	    Isa100::Stats::Cmds::logInfo(confirm);
		process->confirm(confirm);
	}

	void IndicatePublishCallbackFn(Isa100::ASL::Services::ASL_Publish_IndicationPointer indication, ProcessPointer process, Uint32 currentTime)
	{

		if (DuplicateRequests::isDuplicatePublish(indication->publisherAddress, indication->apduPublish->appHandle, currentTime)) {
			// the requests are discarded
			std::string msg = " DUPLICATE MESSAGE !!!";
			Isa100::Stats::Cmds::logInfo(indication, msg);
		}
		else {
			Isa100::Stats::Cmds::logInfo(indication);
			process->indicate(indication);
		}
	}

	void IndicateAlertReportCallbackFn(Isa100::ASL::Services::ASL_AlertReport_IndicationPointer indication, ProcessPointer process)
	{
	    Isa100::Stats::Cmds::logInfo(indication);
		process->indicate(indication);
	}

   void IndicateAlertAckCallbackFn(Isa100::ASL::Services::ASL_AlertAcknowledge_IndicationPointer indication, ProcessPointer process)
    {
        Isa100::Stats::Cmds::logInfo(indication);
        process->indicate(indication);
    }

private:
	ProcessList processes;

};

} // namespace AL
} // namespace Isa100
#endif /* PROCESSMESSAGES_H_ */
