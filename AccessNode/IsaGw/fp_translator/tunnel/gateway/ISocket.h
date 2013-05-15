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

#ifndef ISOCKET_H_
#define ISOCKET_H_

//#include <boost/function.hpp> //for callback
#include <string>

namespace tunnel {
namespace gateway {

class ISocket;
//typedef boost::shared_ptr<ISocket> ISocketPtr;

class ISocket
{
public:
	virtual ~ISocket() {};

	virtual void RunIOService() = 0;
	virtual void StartIOService() = 0;
	virtual void StopIOService() = 0;
		
	virtual void SendBytes(const char* buffer, std::size_t count) = 0;
	virtual std::string Host() = 0;
	virtual int Port() = 0; 

	//boost::function1<void, bool> ConnectionStatus;
	//boost::function2<void, const char*, std::size_t> ReceiveBytes;	

};

} //namespace gateway
} //namespace tunnel

#endif /*ISOCKET_H_*/
