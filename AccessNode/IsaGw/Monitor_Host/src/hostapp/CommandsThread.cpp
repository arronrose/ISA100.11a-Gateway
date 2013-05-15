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

#include "CommandsThread.h"


//added by Cristian.Guef
#include <../AccessNode/Shared/Utils.h>
#include <../AccessNode/Shared/AnPaths.h>
#include <../AccessNode/Shared/DurationWatcher.h>
#include <../AccessNode/Shared/SignalsMgr.h>

namespace nisa100 {
namespace hostapp {

//added by Cristian.Guef
extern bool DoStopProcess;


CommandsThread::CommandsThread(CommandsProcessor& processor_, int checkPeriod_, ConfigApp& p_rConfig) :
	processor(processor_), checkPeriod(checkPeriod_), m_rConfig( p_rConfig)
{
	stopped = false;
}

void CommandsThread::RegisterPeriodicTask(const boost::function0<void>& task)
{
	periodicTasks.push_back(task);
}

void CommandsThread::ClearPeriodicTasks()
{
	periodicTasks.clear();
}

void CommandsThread::Stop()
{
	stopped = true;

	//added by Cristian.Guef
	DoStopProcess = true;

	LOG_INFO("Stop requested...");
}

void CommandsThread::Run()
{
	LOG_DEBUG("commands thread started.");
	WATCH_DURATION_INIT_DEF(oDurationWatcher);
	while (!stopped)
	{
		double dMin1, dMin5, dMin15, dTotal;
		if (GetProcessLoad (dMin1, dMin5, dMin15, dTotal, GET_PROCESS_LOAD_AT_ONE_MIN))
		{	char szTmp[2048];
			sprintf(szTmp, "ProcessLoad: 1/5/15 min: %.2f/%.2f/%.2f total: %.2f", dMin1, dMin5, dMin15, dTotal );
			LOG_INFO("" << szTmp);
		}

	

		try
		{
			processor.WaitForResponses(checkPeriod);

			if (CSignalsMgr::IsRaised(SIGTERM))
			{

				LOG_INFO("CommandsThread::Run Stop requested...");
				CSignalsMgr::Reset(SIGTERM);
				return;
			}

			if (CSignalsMgr::IsRaised(SIGHUP))
			{
				m_rConfig.LoadPublisherData();
				CSignalsMgr::Reset(SIGHUP);
			}

			if ( CSignalsMgr::IsRaised(SIGUSR2))
			{
				m_rConfig.LoadPublisherData();
				CSignalsMgr::Reset(SIGUSR2);
			}

			processor.ProcessResponses();
		}
		/* commented by Cristian.Guef
		catch (boost::thread_interrupted& ex)
		{
			LOG_INFO("Commands thread stop request catched...");
			break;
		}
		*/
		catch(std::exception& ex)
		{
			LOG_ERROR("ProcessResponses failed. error=" << ex.what());
		}
		catch(...)
		{
			LOG_ERROR("ProcessResponses failed. unknown error!");
		}

		/* commented by Cristian.Guef
		if (boost::this_thread::interruption_requested())
		{
			LOG_INFO("commands thread stopped.");
			break;
		}
		*/

		int i=0;
		WATCH_DURATION(oDurationWatcher,5000,5000);
		for (PeriodicTasksList::iterator it = periodicTasks.begin(); it != periodicTasks.end(); it++)
		{
			try
			{				
				(*it)();				
				if(WATCH_DURATION_DEF(oDurationWatcher))
				{
					LOG_INFO("Task index in list=" << i);	
				}

				if (CSignalsMgr::IsRaised(SIGTERM))
				{

					LOG_INFO("CommandsThread::Run Stop requested...");
					CSignalsMgr::Reset(SIGTERM);
					return;
				}

				i++;
			}
			catch(std::exception& ex)
			{
				LOG_ERROR("Run automatic task failed. error=" << ex.what());
			}
			catch(...)
			{
				LOG_ERROR("Run automatic task failed. unknown error!");
			}
		}
		WATCH_DURATION_DEF(oDurationWatcher);

#ifdef HW_VR900
		TouchPidFile(NIVIS_TMP"MonitorHost.pid");
#endif
	}
}

} //namespace hostapp
} //namspace nisa100
