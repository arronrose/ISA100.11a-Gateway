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

#ifndef COMMANDPROCESSOR_H_
#define COMMANDPROCESSOR_H_

/*commented by Cristian.Guef
#include <boost/thread.hpp>
*/
#include <boost/function.hpp> //for callback
#include <nlib/datetime.h>
#include "model/Command.h"
#include "commandmodel/AbstractGService.h"
#include "commandmodel/PublishSubscribe.h"
#include "../ConfigApp.h"

#include "CommandsManager.h"
#include "DevicesManager.h"

//added by Cristian.Guef
#include "periodictasks/DoDeleteLeases.h"

#include "LeaseTrackingMng.h"

namespace nisa100 {
namespace hostapp {

/**
 * @brif Process requests & handle responses.
 * Updates commands status, Updates devices status, Updates devices readings.
 */
class CommandsProcessor
{
public:
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.CommandsProcessor")
	*/

	;
public:
	CommandsProcessor(CommandsManager& commands, DevicesManager& devices, IFactoryDal& factoryDal, ConfigApp& configApp, LeaseTrackingMng &LeaseTrackMng);

	boost::function3<void, AbstractGServicePtr, nlib::TimeSpan, int> SendRequestCallback;
	boost::function4<void, int, boost::uint8_t, boost::int32_t, PublishIndicationPtr> RegisterPublishIndication;
	boost::function2<void, int, boost::uint8_t> CancelPublishIndication;

	//added by Cristian.Guef
	boost::function3<void, boost::int32_t, boost::uint8_t, IPv6> CancelPublishIndication2;

	//added by Cristian.Guef
	boost::function1<void, AbstractGServicePtr> RegisterAlertIndication;
	boost::function0<void> UnregisterAlertIndication;

	boost::function1<void, int> CancelPublishIndications;

	boost::function1<void, const DeviceReading&> AddDeviceReading;

	//added by Cristian.Guef
	boost::function2<void, unsigned int, DoDeleteLeases::InfoForDelContract&> AddNewInfoForDelLease;
	boost::function1<DoDeleteLeases::InfoForDelContract, unsigned int > GetInfoForDelLease;
	boost::function1<void, unsigned int > DelInfoForDelLease;
	boost::function1<void, unsigned int > SetDeleteContractID_NotSent;
	


	//added by Cristian.Guef
	void SendRequest(AbstractGServicePtr service);
	void SendRequest(AbstractGServicePtr service, nlib::TimeSpan timeout);


	void SendRequest(Command& command, AbstractGServicePtr service);
	void SendRequest(Command& command, AbstractGServicePtr service, nlib::TimeSpan timeout);
	void SendRequests(Command& command, AbstractGServicePtr service1, AbstractGServicePtr service2);

	/**
	 * Process a request.
	 * @throw exception if command not processed.
	 */
	void ProcessRequest(Command& command);

	/**
	 * Process a responde
	 */
	/* commented by Cristian.Guef
	void ProcessResponse(AbstractGServicePtr service);
	*/
	//added by Cristian.Guef
	void ProcessResponse(AbstractGServicePtr service, bool now);

	/**
	 * Process the queued responses.
	 */
	void ProcessResponses();

	void WaitForResponses(int seconds);

	//readings
	boost::function0<void> m_SaveReadings;
	void WaitOnSocket(int msec);
	void WaitOnSocket(int msec, struct timeval &from_time);

	void CommandFailed(Command& command, const nlib::DateTime& failedTime, Command::ResponseStatus errorCode);
	void CommandFailed(Command& command, const nlib::DateTime& failedTime, Command::ResponseStatus publishErrorCode,
		Command::ResponseStatus subscriberErrorCode);


	/* commented by Cristian.Guef
	void HandleInvalidContract(int failedCommandID, int deviceID, Command::ResponseStatus errorCode);
	*/
	//added by Cristian.Guef
	void HandleInvalidContract(unsigned int resourceID, unsigned char leaseType,
					int failedCommandID, int deviceID,
					Command::ResponseStatus errorCode);

private:

	DevicePtr GetDevice(int deviceID);

public:
	CommandsManager& commands;
	DevicesManager& devices;

private:
	IFactoryDal& factoryDal;

	typedef std::list<AbstractGServicePtr> ResponsesT;

	ResponsesT responsesBuffer;

	/* commented by Cristian.Guef
	boost::mutex monitor;
	boost::condition waitCondition;

	typedef boost::mutex::scoped_lock lock;
	*/

public:
	ConfigApp& configApp;
	LeaseTrackingMng &leaseTrackMng;

//added by Cristian.Guef
public:
	boost::function1<void, int> GatewayRun;

};

} //namspace hostapp
} //namspace nisa100

#endif /*COMMANDPROCESSOR_H_*/
