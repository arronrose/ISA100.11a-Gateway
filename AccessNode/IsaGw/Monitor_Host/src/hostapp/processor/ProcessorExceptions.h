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

#ifndef PROCESSOREXCEPTIONS_H_
#define PROCESSOREXCEPTIONS_H_

#include <nlib/exception.h>
#include "../model/Device.h"


namespace nisa100 {
namespace hostapp {

class DeviceNotFoundException : public nlib::Exception
{
public:
	DeviceNotFoundException(int deviceID) throw() :
		nlib::Exception(boost::str(boost::format("Device: %1% cannot be found") % deviceID))
	{
	}

	~DeviceNotFoundException() throw()
	{
	}
};

class DeviceNodeRegisteredException : public nlib::Exception
{
public:
	DeviceNodeRegisteredException(DevicePtr device_) throw() :
		nlib::Exception(boost::str(boost::format("Device: %1% is not registered") % device_->Mac().ToString())), device(device_)
	{
	}
	~DeviceNodeRegisteredException() throw()
	{
	}
public:
	DevicePtr device;
};


class DeviceHasNoContractException : public nlib::Exception
{
public:
	DeviceHasNoContractException(const DevicesPtrList& devices_) throw() :
		nlib::Exception("Devices without leases")
	{		
		for (DevicesPtrList::const_iterator it = devices_.begin(); it
				!= devices_.end(); it++)
		{
			devices.push_back(*it);
		}
	}

	~DeviceHasNoContractException() throw()
	{
	}

public:
	DevicesPtrList devices;
};

class CommandNotFoundException : public nlib::Exception
{
public:
	CommandNotFoundException(int commandID) throw() :
		nlib::Exception(boost::str(boost::format("Command: %1% cannot be found") % commandID))
	{
	}

	~CommandNotFoundException() throw()
	{
	}
};

class DeviceChannelNotFound : public nlib::Exception
{
};

}
}

#endif /*PROCESSOREXCEPTIONS_H_*/
