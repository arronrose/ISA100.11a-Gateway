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

#include "CommandsProcessor.h"
#include "CommandsFactory.h"
#include "processor/RequestProcessor.h"
#include "processor/ResponseProcessor.h"
#include "../gateway/GChannel.h"
#include <../AccessNode/Shared/SignalsMgr.h>

#include <boost/format.hpp>



namespace nisa100 {
namespace hostapp {

//added by Cristian.Guef
static unsigned int MultipleReadDiagID = 0;


CommandsProcessor::CommandsProcessor(CommandsManager& commands_, DevicesManager& devices_, IFactoryDal& factoryDal_,
  ConfigApp& configApp_, LeaseTrackingMng &LeaseTrackMng) :
	commands(commands_), devices(devices_), factoryDal(factoryDal_), configApp(configApp_), leaseTrackMng(LeaseTrackMng)
{
}

//added by Cristian.Guef
void CommandsProcessor::SendRequest(AbstractGServicePtr service)
{
	SendRequest(service, configApp.CommandsTimeout());
}
//added by Cristian.Guef
void CommandsProcessor::SendRequest(AbstractGServicePtr service, nlib::TimeSpan timeout)
{
	SendRequestCallback(service, timeout, configApp.RetryCountIfTimeout());
}


void CommandsProcessor::SendRequest(Command& command, AbstractGServicePtr service)
{
	SendRequest(command, service, configApp.CommandsTimeout());
}

void CommandsProcessor::SendRequest(Command& command, AbstractGServicePtr service, nlib::TimeSpan timeout)
{
	SendRequestCallback(service, timeout, configApp.RetryCountIfTimeout());
	commands.SetCommandSent(command);
}

void CommandsProcessor::SendRequests(Command& command, AbstractGServicePtr service1, AbstractGServicePtr service2)
{
	SendRequestCallback(service1, configApp.CommandsTimeout(), configApp.RetryCountIfTimeout());
	SendRequestCallback(service2, configApp.CommandsTimeout(), configApp.RetryCountIfTimeout());

	commands.SetCommandSent(command);
}

extern DevicePtr GetRegisteredDevice(DevicesManager *devices, int deviceID);
extern bool GetLeaseCommittedBurst(const Command::ParametersList& list, int &committedBurst);

void CommandsProcessor::ProcessRequest(Command& command)
{
	//added by Cristian.Guef
	if(command.commandCode == Command::ccReadValue){
		MultipleReadDiagID++;
		command.m_MultipleReadDBCmd.IndexforFragmenting = 0;
		command.m_MultipleReadDBCmd.DiagID = MultipleReadDiagID;
	}

	//added by Cristian.Guef - generate contracts before sending client/server commands
	command.devicesManager = &devices;

//added by Cristian.Guef
loop_for_multiple_read:

	try
	{
		AbstractGServicePtr service = CommandsFactory().Create(command);
		service->CommandID = command.commandID; //set command id

		//added
		service->DeviceID = command.deviceID;
		service->cmdType = command.generatedType;
		service->commandCode = command.commandCode;

		LOG_INFO("ProcessRequest: Command=" << command.ToString() << " Service=" << service->ToString());

		RequestProcessor().Process(service, command, *this);

		if(command.commandCode == Command::ccReadValue &&
			command.m_MultipleReadDBCmd.IndexforFragmenting >= 0){
				//fragmenting
				goto loop_for_multiple_read;
		}
	}
	catch (InvalidCommandException& ex)
	{
		LOG_ERROR("ProcessRequest: failed! Command=" << command.ToString() << " error=" << ex.what());

		std::string str = "active firmware";
		std::string baseStr = ex.what();

		if (command.commandCode == Command::ccCancelBoardFirmwareUpdate)
		{
			if (baseStr.find(str) != std::string::npos)
			{
				commands.SetCommandResponded(command, nlib::CurrentUniversalTime(), "No active firmware update in progress");
				//CommandFailed(command, nlib::CurrentUniversalTime(), Command::rsFailure_NoSensorUpdateInProgress);
				return;
			}
		}
		if (command.commandCode == Command::ccSensorBoardFirmwareUpdate)
		{
			if (baseStr.find(str) != std::string::npos)
			{
				commands.SetCommandResponded(command, nlib::CurrentUniversalTime(), "Already exists an active firmware update");
				//CommandFailed(command, nlib::CurrentUniversalTime(), Command::rsFailure_AlreadySensorUpdateInProgress);
				return;
			}
		}


		CommandFailed(command, nlib::CurrentUniversalTime(), Command::rsFailure_InvalidCommand);
	}
	catch (DeviceHasNoContractException& ex)
	{

		for (DevicesPtrList::const_iterator it = ex.devices.begin(); it != ex.devices.end(); it++)
		{
			/* commented by Cristian.Guef
			commands.CreateContractFor(*(*it), boost::str(boost::format(
			    "system: command failed=%1%, cause device=%2% does not have contract") % command.commandID % (*it)->id));
			*/
			//added by Cristian.Guef
			//1. delete the previous contract if it exists
			if ((*it)->HasContract((*it)->m_unThrownResourceID, (*it)->m_unThrownLeaseType))
			{
				Command DoDeleteLease;
				DoDeleteLease.commandCode = Command::ccDelContract;
				DoDeleteLease.deviceID = devices.GatewayDevice()->id;
				DoDeleteLease.generatedType = Command::cgtAutomatic;
				DoDeleteLease.ContractType = (*it)->m_unThrownLeaseType;
				DoDeleteLease.IPAddress = (*it)->IP();
				DoDeleteLease.ResourceID = (*it)->m_unThrownResourceID;

				//parameters
				int contractID = (*it)->ContractID((*it)->m_unThrownResourceID, (*it)->m_unThrownLeaseType);
				DoDeleteLease.parameters.push_back(CommandParameter(CommandParameter::DelContract_ContractID,
					boost::lexical_cast<std::string>(contractID)));

				
				(*it)->ResetContractID((*it)->m_unThrownResourceID, (*it)->m_unThrownLeaseType);

				commands.CreateCommand(DoDeleteLease, boost::str(boost::format("system: do delete lease_id = %1% with type = %2% for ObjID = %3% and TLSAPID = %4% for device with ip = %5% and dev_id = %6%")
					% (boost::uint32_t) contractID
					% (boost::uint8_t) (*it)->m_unThrownLeaseType
					% (boost::uint16_t) ((*it)->m_unThrownResourceID >> 16)
					% (boost::uint16_t) ((*it)->m_unThrownResourceID & 0xFFFF)
					% (*it)->IP().ToString()
					% (*it)->id));
			}
			//2. create a new one

			//renew lease with new committed burst parameter
			int committedBurst = 0;
			if (GetLeaseCommittedBurst(command.parameters, committedBurst))
			{
				if (!(*it)->IsCommittedBurstGreater((*it)->m_unThrownResourceID, (*it)->m_unThrownLeaseType, committedBurst))
				{
					committedBurst = (*it)->LeaseCommittedBurst((*it)->m_unThrownResourceID, (*it)->m_unThrownLeaseType);
				}
			}
			else
			{
				committedBurst = configApp.LeaseCommittedBurst();
			}
			commands.CreateContractFor(*(*it), boost::str(boost::format(
				"system: command delayed=%1%, cause device=%2% does not have lease") % command.commandID % (*it)->id),
				(*it)->m_unThrownResourceID, (*it)->m_unThrownLeaseType, committedBurst, command.commandID);
		}

		/* commented by Cristian.Guef
		LOG_ERROR("ProcessRequest: failed! Command=" << command.ToString() << " error=" << ex.what());
		CommandFailed(command, nlib::CurrentLocalTime(), Command::rsFailure_DeviceHasNoContract);
		*/
		//added by Cristian.Guef
		LOG_INFO("there was no lease for command = " << command.ToString() << ", so -> recreate lease...");
	}
	catch (DeviceNodeRegisteredException& ex)
	{
		LOG_ERROR("ProcessRequest: failed! Command=" << command.ToString() << " error=" << ex.what());
		CommandFailed(command, nlib::CurrentUniversalTime(), Command::rsFailure_DeviceNotRegistered);
		
		/* commented by Cristian.Guef
		ex.device->WaitForContract(false);
		*/
		

		//added by Cristian.Guef - it should be set the flag on for issueing publish 
									//no need for the case with no contract because the command 
									//remains issued (so it will be retried until contract succeeds)
		ex.device->issuePublishSubscribeCmd = true;
	}
	catch (gateway::ChannelException& ex)
	{
		LOG_ERROR("ProcessRequest: failed! Command=" << command.ToString() << " error=" << ex.what());
		CommandFailed(command, nlib::CurrentUniversalTime(), Command::rsFailure_LostConnection);
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("ProcessRequest: failed! Command=" << command.ToString() << " error=" << ex.what());
		CommandFailed(command, nlib::CurrentUniversalTime(), Command::rsFailure_InternalError);
	}
	catch (...)
	{
		LOG_ERROR("ProcessRequest: failed! Command=" << command.ToString() << " unknown exception!");
		CommandFailed(command, nlib::CurrentUniversalTime(), Command::rsFailure_InternalError);
	}
}

/*commented by Cristian.Guef
void CommandsProcessor::ProcessResponse(AbstractGServicePtr response)
{
	// commented by Cristian.Guef
	{
		lock lk(monitor);
		responsesBuffer.push_back(response);
	}
	waitCondition.notify_one();
	*/
	/*comented by Cristian.Guef
	responsesBuffer.push_back(response);
}
*/
//added by Cristian.Guef
void CommandsProcessor::ProcessResponse(AbstractGServicePtr response, bool now)
{
	//added
	now = true;
	
	if (now == true)
	{
		Command command;
		if (response->CommandID == Command::NO_COMMAND_ID)
		{
			//we have data of publish type...
			LOG_DEBUG("ProcessReponses: " << response->ToString());
		}
		else
		{
			LOG_INFO("ProcessReponses: " << response->ToString());
			
			if (!commands.GetCommand(response->CommandID, command))
			{
				LOG_ERROR("ProcessResponses: failed! CommandID=" << response->CommandID << " not found!"
					<< " Response=" << response->ToString());
				return;
			}

			//added
			command.deviceID = response->DeviceID;
			command.generatedType = response->cmdType;
			command.commandCode = response->commandCode;
		}


		try
		{
			ResponseProcessor().Process(response, command, *this);
		}
		catch (std::exception& ex)
		{
			LOG_ERROR("ProcessResponses: failed! error=" << ex.what() << " Response=" << response->ToString());

			if (command.commandID != Command::NO_COMMAND_ID)
				CommandFailed(command, nlib::CurrentUniversalTime(), Command::rsFailure_InternalError);
		}
		catch (...)
		{
			LOG_ERROR("ProcessResponses: failed! unknown error. Response=" << response->ToString());

			if (command.commandID != Command::NO_COMMAND_ID)
				CommandFailed(command, nlib::CurrentUniversalTime(), Command::rsFailure_InternalError);
		}
		return;
	}
	responsesBuffer.push_back(response);
}


void CommandsProcessor::ProcessResponses()
{
	/* commented by Cristian.Guef
	lock lk(monitor);
	*/

	while (!responsesBuffer.empty())
	{
		AbstractGServicePtr response = responsesBuffer.front();
		responsesBuffer.pop_front();

		LOG_INFO("ProcessReponses: " << response->ToString());

		Command command;
		/* commented by Cristian.Guef
		if (response->CommandID != Command::NO_COMMAND_ID
			&& !commands.GetCommand(response->CommandID, command))
		{
			LOG_ERROR("ProcessResponses: failed! CommandID=" << response->CommandID << " not found!"
					<< " Response=" << response->ToString());
			continue;
		}*/
		
		//added by Cristian.Guef
		if (response->CommandID == Command::NO_COMMAND_ID)
		{
			//we have data of publish type...
		}
		else
		{
			if (!commands.GetCommand(response->CommandID, command))
			{
				LOG_ERROR("ProcessResponses: failed! CommandID=" << response->CommandID << " not found!"
					<< " Response=" << response->ToString());
				continue;
			}

			//added
			command.deviceID = response->DeviceID;
			command.generatedType = response->cmdType;
			command.commandCode = response->commandCode;
		}

		try
		{
			/*commented by Cristian.Guef
			ResponseProcessor().Process(*response, command, *this);
			*/
			//added by Cristian.Guef
			ResponseProcessor().Process(response, command, *this);
		}
		catch (std::exception& ex)
		{
			LOG_ERROR("ProcessResponses: failed! error=" << ex.what() << " Response=" << response->ToString());

			if (command.commandID != Command::NO_COMMAND_ID)
				CommandFailed(command, nlib::CurrentUniversalTime(), Command::rsFailure_InternalError);
		}
		catch (...)
		{
			LOG_ERROR("ProcessResponses: failed! unknown error. Response=" << response->ToString());

			if (command.commandID != Command::NO_COMMAND_ID)
				CommandFailed(command, nlib::CurrentUniversalTime(), Command::rsFailure_InternalError);
		}
	}
}

static int GCD(int a, int b)
{
    while( 1 )
    {
        a = a % b;
		if( a == 0 )
			return b;
		b = b % a;

        if( b == 0 )
			return a;
    }
}

void CommandsProcessor::WaitForResponses(int seconds)
{

	//version with no signal flag
	//int PubSaveTime = configApp.SavePublishPeriod(); /*msec*/
	//int offset = (seconds*1000) %PubSaveTime;
	//int iter = (seconds*1000)/PubSaveTime;

	//if (PubSaveTime > (seconds*1000))
	//{
	//	LOG_ERROR("invalid save_publish_time");
	//	PubSaveTime = seconds*1000;
	//}

	//LOG_DEBUG("time params pub_save_time= " << PubSaveTime << " offset_time" << offset << " iteration_count=" << iter);

	//static bool firstTime = true;
	//static timeval outer_read_tv;

	////gettimeofday(&read_tv, NULL); 
	////LOG_DEBUG("time params with enter time sec= " << outer_read_tv.tv_sec << "usec=" << outer_read_tv.tv_usec);

	////publishTime <= commandsTime, it will alway be an iteration
	//static timeval inner_read_tv;
	//for(int i = 0; i < iter; i++)
	//{
	//	if (firstTime == false)
	//	{
	//		LOG_DEBUG("time params before wait with i =  " << i);
	//		if (i == 0)
	//			WaitOnSocket(PubSaveTime, outer_read_tv);
	//		else
	//			WaitOnSocket(PubSaveTime, inner_read_tv);


	//	}
	//	else
	//	{
	//		LOG_DEBUG("time params before wait with i = " << i);
	//		WaitOnSocket(PubSaveTime);
	//		firstTime = false;
	//	}
	//	//save in db readings...
	//	gettimeofday(&inner_read_tv, NULL); 
	//	m_SaveReadings();
	//}
	//if (offset > 0)
	//{
	//	LOG_DEBUG("time params before offset");
	//	WaitOnSocket(offset, inner_read_tv);
	//}

	////for next calculation
	//gettimeofday(&outer_read_tv, NULL); 
	//
	//LOG_DEBUG("time params with leave time sec= " << outer_read_tv.tv_sec << "usec=" << outer_read_tv.tv_usec);

	//version with signal flag
	static int gcd = GCD(configApp.SavePublishPeriod()/*msec*/, seconds*1000);
	static int ReadingsSaveCount = configApp.SavePublishPeriod()/gcd;
	static int ReadingsSaveIncrm = 0;
	static int DBCmdsReadCount = (seconds*1000)/gcd;
	
	timeval inner_read_tv;
	timeval outer_read_tv;
	gettimeofday(&outer_read_tv, NULL); 
	//LOG_DEBUG("time_params before_loop: ReadingsSaveCount=" << ReadingsSaveCount 
	//	<< " DBCmdsReadCount=" <<  DBCmdsReadCount
	//	<< " sec= " << outer_read_tv.tv_sec << " usec=" << outer_read_tv.tv_usec);

	for (int i = 0; i < DBCmdsReadCount; ++i)
	{
		if (i == 0)
			WaitOnSocket(gcd);
		else
			WaitOnSocket(gcd, inner_read_tv);

		if (CSignalsMgr::IsRaised(SIGHUP) || CSignalsMgr::IsRaised(SIGUSR2) || CSignalsMgr::IsRaised(SIGTERM) )
		{
			LOG_INFO("WaitForResponses: stop waiting goto and handle signal");
			return;
		}

		gettimeofday(&inner_read_tv, NULL); 
		//LOG_DEBUG("time_params in_loop: i = " << i 
		//	<< " ReadingsSaveIncrm = " << ReadingsSaveIncrm
		//	<< " sec= " << inner_read_tv.tv_sec << " usec=" << inner_read_tv.tv_usec);

		ReadingsSaveIncrm++;

		if (ReadingsSaveCount == ReadingsSaveIncrm)
		{
			m_SaveReadings();
			ReadingsSaveIncrm = 0;
		}
	}
	gettimeofday(&outer_read_tv, NULL);
	//LOG_DEBUG("time_params after_loop sec= " << outer_read_tv.tv_sec << " usec=" << outer_read_tv.tv_usec);
}

static bool GetRemainingTime(struct timeval &final_tv, int &wait/*usec*/)
{
	struct timeval read_tv;
	gettimeofday(&read_tv, NULL);
	if (read_tv.tv_sec > final_tv.tv_sec)
		return false;
	
	if (read_tv.tv_sec == final_tv.tv_sec)
	{
		if(read_tv.tv_usec >= final_tv.tv_usec)
			return false;
		wait = final_tv.tv_usec - read_tv.tv_usec;
	}
	else
	{
		wait = (final_tv.tv_sec - read_tv.tv_sec - 1)*1000*1000 + final_tv.tv_usec + (999999 - read_tv.tv_usec);
	}
	
	//LOG_DEBUG("time params with wait=" << wait);
	
	return true;
}

void CommandsProcessor::WaitOnSocket(int msec, struct timeval &from_time)
{
	struct timeval final_tv = from_time;
	final_tv.tv_usec += (msec%1000)*1000;
	if (final_tv.tv_usec > 999999)
	{
		final_tv.tv_sec += (msec/1000 + 1);
		final_tv.tv_usec -= 999999;
	}
	else
	{
		final_tv.tv_sec += msec/1000;
	}
	int wait;
	if (!GetRemainingTime(final_tv, wait))
		return;
	GatewayRun(wait);

	do
	{
		if (!GetRemainingTime(final_tv, wait))
			break;
		GatewayRun(wait);
	}
	while (1);
}

void CommandsProcessor::WaitOnSocket(int msec)
{
	
	struct timeval read_tv;
	gettimeofday(&read_tv, NULL); 
	struct timeval final_tv = read_tv;
	final_tv.tv_usec += (msec%1000)*1000;
	if (final_tv.tv_usec > 999999)
	{
		final_tv.tv_sec += (msec/1000 + 1);
		final_tv.tv_usec -= 999999;
	}
	else
	{
		final_tv.tv_sec += msec/1000;
	}
	int wait = msec*1000/*usec*/;
	GatewayRun(wait);
	do
	{
		if (!GetRemainingTime(final_tv, wait))
			break;
		GatewayRun(wait);
	}
	while (1);
}



DevicePtr CommandsProcessor::GetDevice(int deviceID)
{
	DevicePtr device = devices.FindDevice(deviceID);
	if (!device)
	{
		THROW_EXCEPTION1(DeviceNotFoundException, deviceID);
	}
	return device;
}

void CommandsProcessor::CommandFailed(Command& command, const nlib::DateTime& failedTime,
  Command::ResponseStatus errorCode)
{
	std::string errorReason = FormatResponseCode(errorCode);
	try
	{
		LOG_DEBUG("CommandFailed: Mark Command=" << command.ToString() << " as failed!" << " ErrorCode=" << errorCode
		    << " ErrorReason=" << errorReason);
		commands.SetCommandFailed(command, failedTime, errorCode, errorReason);
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("CommandFailed: Error on setting command as failed! Command=" << command.ToString() << " error="
		    << ex.what());
	}
}

void CommandsProcessor::CommandFailed(Command& command, const nlib::DateTime& failedTime,
  Command::ResponseStatus publishErrorCode, Command::ResponseStatus subscriberErrorCode)
{
	std::string errorReason = boost::str(boost::format("Publisher:%1%, Subscriber:%2%") % FormatResponseCode(
	    publishErrorCode) % FormatResponseCode(subscriberErrorCode));

	Command::ResponseStatus errorCode = publishErrorCode != Command::rsSuccess
	  ? publishErrorCode
	  : subscriberErrorCode;
	try
	{
		LOG_DEBUG("CommandFailed: Mark Command=" << command.ToString() << " as failed!" << " PublishErrorCode="
		    << publishErrorCode << " SubscriberErrorCode=" << subscriberErrorCode << "ErrorReason=" << errorReason);
		commands.SetCommandFailed(command, failedTime, errorCode, errorReason);
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("CommandFailed: Error on setting command as failed! Command=" << command.ToString() << " error="
		    << ex.what());
	}
}

/* commented by Cristian.Guef
void CommandsProcessor::HandleInvalidContract(int failedCommandID, int deviceID, Command::ResponseStatus errorCode)
*/
//added by Cristian.Guef
void CommandsProcessor::HandleInvalidContract(unsigned int resourceID, unsigned char leaseType,
											  int failedCommandID, int deviceID,
											  Command::ResponseStatus errorCode)
{
	try
	{
		switch (errorCode)
		{
		case Command::rsFailure_DeviceHasNoContract:
		{
			//contract has lost so recreate them
			DevicePtr device = GetDevice(deviceID);

			/* commented by Cristian.Guef
			commands.CreateContractFor(*device, boost::str(boost::format(
			    "system: CommandID=%1% failed, cause DeviceID=%2% doesn't have contract") % failedCommandID % deviceID));
				*/
			//added By Cristian.Guef
			commands.CreateContractFor(*device, boost::str(boost::format(
			    " cause DeviceID=%2% doesn't have lease") % failedCommandID % deviceID),
				resourceID, leaseType, 0, 0);
		}
			break;

		case Command::rsFailure_GatewayContractExpired:
		{
			//contract has lost so recreate them
			DevicePtr device = GetDevice(deviceID);
			
			/* commented by Cristian.Guef
			commands.CreateContractFor(*device, boost::str(boost::format(
			    "system: CommandID=%1% failed, cause contract expired for DeviceID=%2%") % failedCommandID % deviceID),
				);
				*/
			//added By Cristian.Guef
			commands.CreateContractFor(*device, boost::str(boost::format(
			    " cause lease expired for DeviceID=%1%") % deviceID),
				resourceID, leaseType, 0, 0);
		}
			break;
		default:
			break; //disable warning that not all errorcode are handled in switch
		}
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("HandleInvalidLease: Error on processing command as failed! error=" << ex.what());
	}
}

} // namespace hostapp
} // namespace nisa100
