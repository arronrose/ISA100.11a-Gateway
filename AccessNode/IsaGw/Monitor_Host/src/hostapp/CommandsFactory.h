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

#ifndef COMMANDSFACTORY_H_
#define COMMANDSFACTORY_H_

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../Log.h"

#include <nlib/exception.h>

#include "model/Command.h"
#include "commandmodel/AbstractGService.h"

namespace nisa100 {
namespace hostapp {

class InvalidCommandException : public nlib::Exception
{
public:
	InvalidCommandException(const std::string& message) :
		nlib::Exception(message)
	{
	}
};


/**
 * @brief Creates AbstractGService from a Command object.
 */ 
class CommandsFactory
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.CommandsFactory");
	*/

public:
	
	/**
	 * @throw InvalidCommandException
	 */ 

	/* commented by Cristian.Guef
	AbstractGServicePtr Create(const Command& command);
	*/
	//added by Cristian.Guef
	AbstractGServicePtr Create(Command& command);
};

}
}

#endif /*COMMANDSFACTORY_H_*/
