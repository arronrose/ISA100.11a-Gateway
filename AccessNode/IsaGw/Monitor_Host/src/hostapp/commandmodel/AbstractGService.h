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

#ifndef ABSTRACTGSERVICE_H_
#define ABSTRACTGSERVICE_H_

#include <boost/shared_ptr.hpp>

#include "../model/Command.h"

namespace nisa100 {
namespace hostapp {

class AbstractGService;
typedef boost::shared_ptr<AbstractGService> AbstractGServicePtr;

class IGServiceVisitor;

/**
 * @brief The Visited class.
 */
class AbstractGService
{
public:

	Command::ResponseStatus Status;
	int CommandID; //the command originator id

	//added
	Command::CommandGeneratedType cmdType;
	int DeviceID;
	Command::CommandCode commandCode;

	AbstractGService()
	{
		Status = Command::rsNoStatus;
		CommandID = -1;

		//added
		DeviceID = -1;
		cmdType = Command::cgtManual;
	}

	AbstractGService(const AbstractGService& rhs)
	{
		Status = rhs.Status;
		CommandID = rhs.CommandID;
	}

	virtual ~AbstractGService()
	{
	}

	virtual bool Accept(IGServiceVisitor& visitor) = 0;
	virtual const std::string ToString() const = 0;

	virtual AbstractGServicePtr Clone() const
	{
		return AbstractGServicePtr();
	}
};

} // namespace hostapp
} // namespace nisa100

#endif /*ABSTRACTGSERVICE_H_*/
