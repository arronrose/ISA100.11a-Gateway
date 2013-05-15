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

//#include <boost/function.hpp> //for callback definition
//#include <boost/smart_ptr.hpp> //for scoped_ptr

#include <nlib/exception.h>

#include "GeneralPacket.h"
#include "ISocket.h"
#include "AsyncTCPSocket.h"

#include "../interfaces/TaskRun.h"

class IGWTrigger;

namespace tunnel {
	namespace comm {
		class TrackingManager;
	}
namespace gateway {

class ChannelException : public nlib::Exception
{
public:
	ChannelException(const std::string& message) :
		nlib::Exception(message)
	{
	}
};

class GChannel: public ITaskRun
{
public:
	GChannel(const std::string& host, int port);
	virtual ~GChannel();

public:
	virtual void operator()(){Run();}

public:
	void SendPacket(GeneralPacket& packet);
	void Start();
	void Stop();

private:
	void Run();

public:
	void SetTrackManager(tunnel::comm::TrackingManager *pTrackManager);
private:
	tunnel::comm::TrackingManager *m_pTrackManager;
	
public:
	std::vector<IGWTrigger* > m_disconnected;
	std::vector<IGWTrigger* > m_connected;

	
public:
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
	bool m_isOpen;

	std::deque<boost::uint8_t> m_remainingBytes;

	ReaderState m_currentReadState;
	GeneralPacket m_packetInProgress;

	ISocket* m_socket;
};

} //namspace gateway
} //namspace tunnel

#endif /*GCHANNEL_H_*/
