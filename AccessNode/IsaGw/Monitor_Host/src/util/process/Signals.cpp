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

#include "Signals.h"

#ifndef _WINDOWS
#include <signal.h>
#endif

namespace nisa100 {
namespace util {


Signals::HandlersMap Signals::registeredHandles;

Signals::Signals()
{
}

Signals::~Signals()
{
}

void Signals::Wait(int signal)
{
#ifndef _WINDOWS
	sigset_t signalSet;

	sigemptyset(&signalSet);
	sigaddset(&signalSet, signal);

	LOG_DEBUG("Waiting for signal=" << signal << "...");

	int receivedSignal = 0;
	sigwait(&signalSet, &receivedSignal);

	LOG_INFO("Received signal=" << receivedSignal);
#endif
}

void Signals::Wait(SignalsList signals)
{
#ifndef _WINDOWS
	sigset_t signalsSet;
	sigemptyset(&signalsSet);

	for (SignalsList::iterator itSignal = signals.begin(); itSignal != signals.end(); itSignal++)
	{
		LOG_DEBUG("Waiting for signal=" << *itSignal << "...");
		sigaddset(&signalsSet, *itSignal);
	}

	int receivedSignal = 0;
	sigwait(&signalsSet, &receivedSignal);

	LOG_INFO("Received signal=" << receivedSignal);
#endif
}

void Signals::Ignore(int signal)
{
#ifndef _WINDOWS
	sigignore(signal);
	LOG_INFO("Signal=" << signal << " will be ignored!!!");
#endif
}


void Signals::RegisterSignalHandler(int signal, const boost::function0<void>& handler)
{
#ifndef _WINDOWS
	struct sigaction act;
	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0; //SA_SIGINFO
	if(sigaction(signal, &act, 0) ==0)
	{
		registeredHandles[signal] = handler;
	}
	else
		LOG_ERROR("Cannot register handler for signal: " << signal);		
#endif
}

void Signals::signal_handler(int signum)
{
	HandlersMap::const_iterator it = registeredHandles.find(signum);
	if (it != registeredHandles.end())
	{
		it->second();
	}
}


} //namespace util
} //namespace nisa100
