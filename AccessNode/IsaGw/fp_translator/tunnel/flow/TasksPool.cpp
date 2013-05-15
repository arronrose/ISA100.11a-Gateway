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

#include "TasksPool.h"

#include "../log/Log.h"


namespace tunnel {
namespace comm {


void TasksPool::RegisterPeriodicTask(ITaskRun* pTask)
{
	periodicTasks.push_back(pTask);
}

void TasksPool::ClearPeriodicTasks()
{
	periodicTasks.clear();
}

void TasksPool::Run()
{

	//LOG_DEBUG("performing io operations...");

	for (PeriodicTasksList::iterator it = periodicTasks.begin(); it != periodicTasks.end(); it++)
	{
		try
		{
			(*it)->operator ()();
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

}

} //namespace comm
} //namspace tunnel
