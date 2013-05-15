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

#include "DoDeleteLeases.h"

namespace nisa100 {
namespace hostapp {

void DoDeleteLeases::IssueDeleteContractCmd(unsigned int contractID, InfoForDelContract& infoForDelContract)
{
	Command DoDeleteLease;
	DoDeleteLease.commandCode = Command::ccDelContract;
	DoDeleteLease.deviceID = devices.GatewayDevice()->id;

	//parameters
	DoDeleteLease.parameters.push_back(CommandParameter(CommandParameter::DelContract_ContractID,
		boost::lexical_cast<std::string>(contractID)));

	DoDeleteLease.generatedType = Command::cgtAutomatic;
	commands.CreateCommand(DoDeleteLease,
		boost::str(boost::format("system: do delete lease_id = %1% with type = %2% for ObjID = %3% and TLSAPID = %4% for device with ip = %5% and dev_id = %6%")
		% (boost::uint32_t) contractID
		% (boost::uint8_t) infoForDelContract.LeaseType
		% (boost::uint16_t) (infoForDelContract.ResourceID >> 16)
		% (boost::uint16_t) (infoForDelContract.ResourceID & 0xFFFF)
		% infoForDelContract.IPAddress.ToString()
		% (boost::int32_t)infoForDelContract.DevID));
}

void DoDeleteLeases::AddNewInfoForDelLease(unsigned int ContractID, InfoForDelContract &infoToDelLease)
{
	std::map<unsigned int, InfoForDelContract>::iterator i = m_InfoForDelContract.find(ContractID);
	if (i != m_InfoForDelContract.end())
	{
		LOG_DEBUG("add_info -----> Already lease id = "  << (boost::uint32_t)ContractID << " in the map " <<
			"for device with ip = " << infoToDelLease.IPAddress.ToString() <<
			"and dev_id = " << (boost::int32_t)infoToDelLease.DevID <<
			"with obj_id = " << (boost::uint16_t) (infoToDelLease.ResourceID >> 16) <<
			"and tlsap_id = " << (boost::uint16_t) (infoToDelLease.ResourceID & 0xFFFF) <<
			"and lease type = " << (boost::uint8_t) infoToDelLease.LeaseType << "so skip... " );
		return;
	}

	LOG_DEBUG("add_info -----> lease id = "  << (boost::uint32_t)ContractID << " in the map" <<
			"for device with ip = " << infoToDelLease.IPAddress.ToString() <<
			"and dev_id = " << (boost::int32_t)infoToDelLease.DevID <<
			"with obj_id = " << (boost::uint16_t) (infoToDelLease.ResourceID >> 16) <<
			"and tlsap_id = " << (boost::uint16_t) (infoToDelLease.ResourceID & 0xFFFF) <<
			"and lease type = " << (boost::uint8_t) infoToDelLease.LeaseType);

	infoToDelLease.GSAPsent = false;
	m_InfoForDelContract[ContractID] = infoToDelLease;
}
void DoDeleteLeases::Check()
{
	if (!devices.GatewayConnected())
		return; // no reason to send commands to a disconnected GW

	if (IsGatewayReconnected == true)
		return; // no reason to send commands without a session created

	//for any contact do delete
	LOG_DEBUG("leases to delete = " << (boost::uint32_t)m_InfoForDelContract.size());
	for (std::map<unsigned int, InfoForDelContract>::iterator i = m_InfoForDelContract.begin();
				i != m_InfoForDelContract.end(); i++)
	{
		if (i->second.GSAPsent == false)
		{
			LOG_DEBUG("send cmd to delete lease = " << (boost::uint32_t)i->first
						<< "for obj_id = " << (boost::uint32_t)(i->second.ResourceID >> 16)
						<< "for tlsap_id = " << (boost::uint32_t)(i->second.ResourceID & 0xFFFF)
						<< "for type = " << (boost::uint32_t)i->second.LeaseType
						<< "for device with ip = " << i->second.IPAddress.ToString()
						<< "and dev_id = " << (boost::int32_t)i->second.DevID);

			IssueDeleteContractCmd(i->first, i->second);
			i->second.GSAPsent = true;
		}
		else
		{
			LOG_DEBUG("already sent cmd to delete lease = " << (boost::uint32_t)i->first
						<< "for obj_id = " << (boost::uint32_t)(i->second.ResourceID >> 16)
						<< "for tlsap_id = " << (boost::uint32_t)(i->second.ResourceID & 0xFFFF)
						<< "for type = " << (boost::uint32_t)i->second.LeaseType
						<< "for device with ip = " << i->second.IPAddress.ToString()
						<< "and dev_id = " << (boost::int32_t)i->second.DevID);
		}

		if (!devices.GatewayConnected())
			return; // no reason to send commands to a disconnected GW
	}
}
DoDeleteLeases::InfoForDelContract DoDeleteLeases::GetInfoForDelLease(unsigned int ContractID)
{
	std::map<unsigned int, InfoForDelContract>::iterator i = m_InfoForDelContract.find(ContractID);
	if (i == m_InfoForDelContract.end())
	{
		LOG_ERROR("get_info -----> lease id = "  << (boost::uint32_t)ContractID << " not found in the map ");
		InfoForDelContract info;
		info.ResourceID = 0;
		return info;
	}

	LOG_DEBUG("get_info -----> lease id = "  << (boost::uint32_t)ContractID << " in the map" <<
			"for device with ip = " << i->second.IPAddress.ToString() <<
			"and dev_id = " <<  (boost::int32_t)i->second.DevID <<
			"with obj_id = " << (boost::uint16_t) (i->second.ResourceID >> 16) <<
			"and tlsap_id = " << (boost::uint16_t) (i->second.ResourceID & 0xFFFF) <<
			"and lease type = " << (boost::uint8_t) i->second.LeaseType);

	return i->second;
}
void DoDeleteLeases::DelInfoForDelLease(unsigned int ContractID)
{
	std::map<unsigned int, InfoForDelContract>::iterator i = m_InfoForDelContract.find(ContractID);
	if (i == m_InfoForDelContract.end())
	{
		LOG_ERROR("delete_info ----> lease id = "  << (boost::uint32_t)ContractID << " not found in the map ");
		return;
	}

	LOG_DEBUG("delete_info -----> lease id = "  << (boost::uint32_t)ContractID << " in the map" <<
			"for device with ip = " << i->second.IPAddress.ToString() <<
			"and dev_id = " <<  (boost::int32_t)i->second.DevID <<
			"with obj_id = " << (boost::uint16_t) (i->second.ResourceID >> 16) <<
			"and tlsap_id = " << (boost::uint16_t) (i->second.ResourceID & 0xFFFF) <<
			"and lease type = " << (boost::uint8_t) i->second.LeaseType);

	m_InfoForDelContract.erase(i);
}
void DoDeleteLeases::SetDeleteContractID_NotSent(unsigned int ContractID)
{
	std::map<unsigned int, InfoForDelContract>::iterator i = m_InfoForDelContract.find(ContractID);
	if (i == m_InfoForDelContract.end())
	{
		LOG_ERROR("set_info ----> lease id = "  << (boost::uint32_t)ContractID << " not found in the map ");
		return;
	}

	LOG_DEBUG("set_info -----> lease id = "  << (boost::uint32_t)ContractID << " in the map" <<
			"for device with ip = " << i->second.IPAddress.ToString() <<
			"and dev_id = " <<  (boost::int32_t)i->second.DevID <<
			"with obj_id = " << (boost::uint16_t) (i->second.ResourceID >> 16) <<
			"and tlsap_id = " << (boost::uint16_t) (i->second.ResourceID & 0xFFFF) <<
			"and lease type = " << (boost::uint8_t) i->second.LeaseType);

	i->second.GSAPsent = false;
}
}//namespace hostapp
}//namespace nisa100
