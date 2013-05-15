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

#ifndef SIGNALS_H_
#define SIGNALS_H_

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../../Log.h"

#include <boost/function.hpp> //for callback
#include <map>
#include <vector>

#include <signal.h> //used to define SIG_ identifiers


namespace nisa100 {
namespace util {

typedef std::vector<int> SignalsList;

class Signals
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.util.Signals");
	*/

public:
	Signals();
	virtual ~Signals();
	
	void Wait(int signal);
	void Wait(SignalsList signals);	
	void Ignore(int signal);	
	
	void RegisterSignalHandler(int signal, const boost::function0<void>& handler);
	
private:
	static void signal_handler(int signum);

	typedef std::map<int, boost::function0<void> > HandlersMap;
	static HandlersMap registeredHandles;
};

}
}

#endif /*SIGNALS_H_*/
