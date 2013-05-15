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

#ifndef COMMANDSTHREAD_H_
#define COMMANDSTHREAD_H_

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../Log.h"

#include <nlib/datetime.h>
/*commented by Cristian.Guef
#include <boost/thread.hpp> //for condition & mutex
*/
#include <list>
#include <boost/function.hpp> //for callback

#include "CommandsProcessor.h"

namespace nisa100 {
namespace hostapp {

/**
 * This thread is responsable for processing commands response.
 * Also supports running some tasks periodically.
 */
class CommandsThread
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.CommandsThread");
	*/

public:
	CommandsThread(CommandsProcessor& processor, int checkPeriod, ConfigApp& p_rConfig);

	void RegisterPeriodicTask(const boost::function0<void>& task);
	void ClearPeriodicTasks();

	void Run();
	
	void Stop();

private:
	CommandsProcessor& processor;
	ConfigApp&			m_rConfig;
	int checkPeriod;

	typedef std::list<boost::function0<void> > PeriodicTasksList;
	PeriodicTasksList periodicTasks;
	
	bool stopped;
};

} //namespace hostapp
} //namespace nisa100
#endif /*COMMANDSTHREAD_H_*/


