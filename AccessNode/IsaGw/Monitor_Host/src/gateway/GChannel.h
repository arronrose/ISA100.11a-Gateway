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

#ifndef GCHANNEL_H_
#define GCHANNEL_H_

#include <string>
#include <deque>
#include <vector>

#include <boost/function.hpp> //for callback definition
#include <boost/smart_ptr.hpp> //for scoped_ptr

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../Log.h"

#include <nlib/exception.h>

#include "GeneralPacket.h"
#include "ISocket.h"
#include "AsyncTCPSocket.h"
#if 0	//disable listen mode
#include "AsyncTCPServerSocket.h"
#endif
#include "../ConfigApp.h"

namespace nisa100 {
namespace gateway {

class ChannelException : public nlib::Exception
{
public:
	ChannelException(const std::string& message) :
		nlib::Exception(message)
	{
	}
};

class GChannel
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.gateway.GChannel")
	*/

public:
	GChannel(const ConfigApp& configApp);

	virtual ~GChannel();

	void SendPacket(GeneralPacket& packet);
	boost::function1<void, const GeneralPacket&> ReceivePacket;

	void Start();
	void Stop();

	void Run(int usec);

	std::vector<boost::function0<void> > Disconnected;
	std::vector<boost::function2<void, const std::string&, int> > Connected;

	
private:
	void ChangeState(bool open);
	void ReceiveBytes(const char* buffer, std::size_t count);

private:
	enum ReaderState
	{
		ReadingHeaderVersion,
		ReadingHeader,
		ReadingData,
		DataReadCompleted
	};

private:
	bool isOpen;

	std::deque<boost::uint8_t> remainingBytes;

	ReaderState currentReadState;
	GeneralPacket packetInProgress;

	ISocketPtr socket;

	const ConfigApp& configApp;
	
	//added
	int				m_countLeft;
	unsigned char	m_readBuffer[2*64*1024];
	
	//added
	std::basic_string<unsigned char>  m_sendBuff;
};

} //namspace gateway
} //namspace nisa100

#endif /*GCHANNEL_H_*/
