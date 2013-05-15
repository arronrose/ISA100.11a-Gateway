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

#ifndef TASKSPOOL_H_
#define TASKSPOOL_H_

#include <list>

#include "../interfaces/TaskRun.h"


namespace tunnel {
namespace comm {

/**
 * This class is responsable for running tasks periodically.
 * 
 */
class TasksPool
{
public:

	void RegisterPeriodicTask(ITaskRun* pTask);
	void ClearPeriodicTasks();

	void Run();

private:
	typedef std::list<ITaskRun*> PeriodicTasksList;
	PeriodicTasksList periodicTasks;
};

} //namespace comm
} //namespace tunnel
#endif 


