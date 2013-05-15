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

#ifndef _ALLERT_GENERATOR_H__
#define _ALLERT_GENERATOR_H__


#include "../CommandsManager.h"
#include "../DevicesManager.h"

#include "../../Log.h"

#include "../commandmodel/Nisa100ObjectIDs.h"

namespace nisa100 {
namespace hostapp {

//added by Cristian.Guef
extern bool IsGatewayReconnected;

extern void ResetFirmwaresInProgress(DevicesManager &devices);

/**
 * Generates get device_list commands with specified granularity.
 */
class AlertGenerator
{
public:
	AlertGenerator(CommandsManager& commands_, DevicesManager& devices_) :
		commands(commands_), devices(devices_)
	{
		IsAlertSubSent = false;
		commands.ReceiveCommandResponse.push_back(boost::bind(&AlertGenerator::HandleRespondedCommand, this, _1, _2));
		m_categoryProcessSub = -1;
		m_categoryDeviceSub = -1;
		m_categoryNetworkSub = -1;
		m_categorySecuritySub = -1;
		m_netAddrSMProcessSub = true;
		smDeviceID = -1;
	}

	void Check()
	{
		if (!devices.GatewayConnected())
			return; // no reason to send commands to a disconnected GW

		//added by Cristian.Guef
		if (IsGatewayReconnected == true)
			return; // no reason to send commands without a session created


		// we need sm's address also
		DevicePtr systemManager = devices.SystemManagerDevice();
		if (!systemManager)
		{
			LOG_INFO("Alert subscription: SM not found, so delay alert subscription!");
			return; //no SM or no registered
		}
		smDeviceID = systemManager->id;

		//
		if (!devices.GetAlertProvision(tmp_categoryProcessSub, tmp_categoryDeviceSub, tmp_categoryNetworkSub, tmp_categorySecuritySub))
		{
			LOG_ERROR("alert -> unable to read provision info!");
		}

		
		if (m_categoryProcessSub != tmp_categoryProcessSub || 
			m_categoryDeviceSub != tmp_categoryDeviceSub ||
			m_categoryNetworkSub !=  tmp_categoryNetworkSub ||
			m_categorySecuritySub != tmp_categorySecuritySub ||
			m_netAddrSMProcessSub == true)
		{
			if (!IsAlertSubSent)
			{
				CreateAlertSubCommand();
				IsAlertSubSent = true;
			}
		}

		
		
	
	}	
private:
	void CreateAlertSubCommand()
	{
		try
		{
			Command alertCommand;
			alertCommand.commandCode = Command::ccAlertSubscription;
			alertCommand.deviceID = devices.GatewayDevice()->id;
			alertCommand.generatedType = Command::cgtAutomatic;

			//device
			{
				if (m_categoryDeviceSub != tmp_categoryDeviceSub)
				{
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_Category_Subscribe,
					boost::lexical_cast<std::string>(tmp_categoryDeviceSub/*subscription*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_Category_Enable,
					boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_Category_Category,
					boost::lexical_cast<std::string>(0/*device*/)));
				}
			}

			//network
			{
				if (m_categoryNetworkSub != tmp_categoryNetworkSub)
				{
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_Category_Subscribe,
					boost::lexical_cast<std::string>(tmp_categoryNetworkSub/*subscription*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_Category_Enable,
					boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_Category_Category,
					boost::lexical_cast<std::string>(1/*network*/)));
				}
				
			}

			//security
			{
				if (m_categorySecuritySub != tmp_categorySecuritySub)
				{
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_Category_Subscribe,
					boost::lexical_cast<std::string>(tmp_categorySecuritySub/*subscription*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_Category_Enable,
					boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_Category_Category,
					boost::lexical_cast<std::string>(2/*security*/)));
				}
			}

			//process
			{
				if (m_categoryProcessSub != tmp_categoryProcessSub)
				{
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_Category_Subscribe,
					boost::lexical_cast<std::string>(tmp_categoryProcessSub/*subscription*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_Category_Enable,
					boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_Category_Category,
					boost::lexical_cast<std::string>(3/*process*/)));
				}
			}

			//networkAddress
			{
				if (m_netAddrSMProcessSub)
				{
					// UDO transfer alerts
					//UDO transfer start alert subscription
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Subscribe,boost::lexical_cast<std::string>(1/*subscribe*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Enable,boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_DevID,boost::lexical_cast<std::string>(smDeviceID)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_TLSAPID,boost::lexical_cast<std::string>(apSMAP)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_ObjID,boost::lexical_cast<std::string>(UPLOAD_DOWNLOAD_OBJECT)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_AlertType,boost::lexical_cast<std::string>(0/*event-transfer_started*/)));
					//UDO transfer progress (in progress) alert subscription
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Subscribe,boost::lexical_cast<std::string>(1/*subscribe*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Enable,boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_DevID,boost::lexical_cast<std::string>(smDeviceID)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_TLSAPID,boost::lexical_cast<std::string>(apSMAP)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_ObjID,boost::lexical_cast<std::string>(UPLOAD_DOWNLOAD_OBJECT)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_AlertType,boost::lexical_cast<std::string>(1/*event-transfer_in_progress*/)));
					//UDO transfer end alert subscription
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Subscribe,boost::lexical_cast<std::string>(1/*subscribe*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Enable,boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_DevID,boost::lexical_cast<std::string>(smDeviceID)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_TLSAPID,boost::lexical_cast<std::string>(apSMAP)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_ObjID,boost::lexical_cast<std::string>(UPLOAD_DOWNLOAD_OBJECT)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_AlertType,boost::lexical_cast<std::string>(2/*event-transfer_ended*/)));

					// device join alerts
					//SMO- device join ok alert subscription
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Subscribe,boost::lexical_cast<std::string>(1/*subscribe*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Enable,boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_DevID,boost::lexical_cast<std::string>(smDeviceID)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_TLSAPID,boost::lexical_cast<std::string>(apSMAP)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_ObjID,boost::lexical_cast<std::string>(SYSTEM_MONITORING_OBJECT)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_AlertType,boost::lexical_cast<std::string>(0/*device_join*/)));
					//SMO- device join fail alert subscription
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Subscribe,boost::lexical_cast<std::string>(1/*subscribe*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Enable,boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_DevID,boost::lexical_cast<std::string>(smDeviceID)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_TLSAPID,boost::lexical_cast<std::string>(apSMAP)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_ObjID,boost::lexical_cast<std::string>(SYSTEM_MONITORING_OBJECT)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_AlertType,boost::lexical_cast<std::string>(1/*device_join_failed*/)));
					//SMO- device leave alert subscription
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Subscribe,boost::lexical_cast<std::string>(1/*subscribe*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Enable,boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_DevID,boost::lexical_cast<std::string>(smDeviceID)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_TLSAPID,boost::lexical_cast<std::string>(apSMAP)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_ObjID,boost::lexical_cast<std::string>(SYSTEM_MONITORING_OBJECT)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_AlertType,boost::lexical_cast<std::string>(2/*device_leave*/)));

					// topology alerts
					//SYSTEM_MONITORING_OBJECT- parent change alert subscription
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Subscribe, boost::lexical_cast<std::string>(1/*subscribe*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Enable, boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_DevID, boost::lexical_cast<std::string>(smDeviceID)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_TLSAPID, boost::lexical_cast<std::string>(apSMAP)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_ObjID, boost::lexical_cast<std::string>(SYSTEM_MONITORING_OBJECT)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_AlertType, boost::lexical_cast<std::string>(4/*parent change*/)));
					//SYSTEM_MONITORING_OBJECT- backup change alert subscription
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Subscribe, boost::lexical_cast<std::string>(1/*subscribe*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Enable, boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_DevID, boost::lexical_cast<std::string>(smDeviceID)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_TLSAPID, boost::lexical_cast<std::string>(apSMAP)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_ObjID, boost::lexical_cast<std::string>(SYSTEM_MONITORING_OBJECT)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_AlertType, boost::lexical_cast<std::string>(5/*backup change*/)));

					// contract alerts
					//SCO- contract establish alert subscription
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Subscribe, boost::lexical_cast<std::string>(1/*subscribe*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Enable, boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_DevID, boost::lexical_cast<std::string>(smDeviceID)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_TLSAPID, boost::lexical_cast<std::string>(apSMAP)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_ObjID, boost::lexical_cast<std::string>(SCO)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_AlertType, boost::lexical_cast<std::string>(0/*contract establish*/)));
					//SCO- contract modify alert subscription
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Subscribe, boost::lexical_cast<std::string>(1/*subscribe*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Enable, boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_DevID, boost::lexical_cast<std::string>(smDeviceID)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_TLSAPID, boost::lexical_cast<std::string>(apSMAP)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_ObjID, boost::lexical_cast<std::string>(SCO)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_AlertType, boost::lexical_cast<std::string>(1/*contract modify*/)));
					//SCO- contract refusal alert subscription
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Subscribe, boost::lexical_cast<std::string>(1/*subscribe*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Enable, boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_DevID, boost::lexical_cast<std::string>(smDeviceID)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_TLSAPID, boost::lexical_cast<std::string>(apSMAP)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_ObjID, boost::lexical_cast<std::string>(SCO)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_AlertType, boost::lexical_cast<std::string>(2/*contract refusal*/)));
					//SCO- contract terminate alert subscription
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Subscribe, boost::lexical_cast<std::string>(1/*subscribe*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_Enable, boost::lexical_cast<std::string>(1/*enable*/)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_DevID, boost::lexical_cast<std::string>(smDeviceID)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_TLSAPID, boost::lexical_cast<std::string>(apSMAP)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_ObjID, boost::lexical_cast<std::string>(SCO)));
					alertCommand.parameters.push_back(CommandParameter(CommandParameter::Alert_NetAddr_AlertType, boost::lexical_cast<std::string>(3/*contract terminate*/)));
				}
			}

			commands.CreateCommandForAlert(alertCommand, "system: alert subscription", currentAlertCommandId);
			LOG_INFO("Automatic alert request was made!");
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Error when generate automatic topology request! Error:" << ex.what());
		}

	}

public:
	void HandleRespondedCommand(const int commandID, Command::ResponseStatus status)
	{
		if (commandID != currentAlertCommandId)
			return;
			
		if (status == Command::rsSuccess)
		{
			m_categoryProcessSub = tmp_categoryProcessSub;
			m_categoryDeviceSub = tmp_categoryDeviceSub;
			m_categoryNetworkSub = tmp_categoryNetworkSub;
			m_categorySecuritySub = tmp_categorySecuritySub;
			m_netAddrSMProcessSub = false;
		}

		IsAlertSubSent = false;
	}

	void HandleGWConnect(const std::string& host, int port)
	{
		IsAlertSubSent = false;
	}

	void HandleGWDisconnect()
	{
		devices.GatewayDevice()->ResetContractID(0/*no resource*/, (unsigned char) GContract::Alert_Subscription);
		m_categoryProcessSub = -1;
		m_categoryDeviceSub = -1;
		m_categoryNetworkSub = -1;
		m_categorySecuritySub = -1;
		m_netAddrSMProcessSub = true;

		ResetFirmwaresInProgress(devices);
	}

private:
	CommandsManager& commands;
	DevicesManager& devices;
	
	bool IsAlertSubSent;
	int currentAlertCommandId;

	//
	int m_categoryProcessSub, m_categoryDeviceSub, m_categoryNetworkSub, m_categorySecuritySub;
	int tmp_categoryProcessSub, tmp_categoryDeviceSub, tmp_categoryNetworkSub, tmp_categorySecuritySub;
	bool m_netAddrSMProcessSub;
	int smDeviceID;
};

} //namespace hostapp
} //namespace nisa100

#endif
