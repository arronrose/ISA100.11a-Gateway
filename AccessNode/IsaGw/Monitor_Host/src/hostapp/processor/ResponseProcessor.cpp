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

#include "ResponseProcessor.h"
#include "../CommandsProcessor.h"

#include <../AccessNode/Shared/DurationWatcher.h>

#include <boost/format.hpp>

#include <netinet/in.h>


//added by Cristian.Guef
#include <queue>
#include <string>
#include "ReportsProcessor.h"
#include "PublishedDataMng.h"
#include <stdio.h>

#include <tai.h>
#include <../ISA100/dmap.h>	// ISA port

/// TABLE FirmwareDownloads COLUMN FwType is: 0: sensor board; 3: backbone; 10: device
/// firmware type as a function of device type. DOES NOT work for sensor board firmware
#define FWTYPE(_device_type_) ((Device::dtBackboneRouter == (_device_type_)) ? 3 : 10)
#define FWTYPE_SENZOR_BOARD 0

namespace nisa100 {
namespace hostapp {


//firmware_download_commited_burst
struct FirmDlSpeed
{
	int		committedBurst;
	bool	isSpeedSet;
	FirmDlSpeed(int	committedBurst_):committedBurst(committedBurst_){isSpeedSet = true;}
};
static std::map<IPv6, FirmDlSpeed>	gt_committedBurst_SM_DEV;
void SetGreaterCommittedBurst(IPv6 &ip, int committed)
{
	std::pair<std::map<IPv6,FirmDlSpeed>::iterator, bool > pr = gt_committedBurst_SM_DEV.insert(
									std::map<IPv6,FirmDlSpeed>::value_type(ip,FirmDlSpeed(committed)));
	if (pr.second == false)
	{
		if (pr.first->second.committedBurst == 0 && committed != 0 && committed > -15)
		{
			pr.first->second.committedBurst = committed;
			pr.first->second.isSpeedSet = true;
			return;
		}
		if (committed > pr.first->second.committedBurst)
		{
			pr.first->second.committedBurst = committed;
			pr.first->second.isSpeedSet = true;
			return;
		}
	}
}
bool doGetContractAndRoutes = false;
bool GetContractAndRoutesFlag()
{
	return doGetContractAndRoutes;
}
void ResetContractAndRoutesFlag()
{
	doGetContractAndRoutes = false;
}
bool doGetContractAndRoutesNow= false; //is set only when there is started just one firmware_download
static int FirmDlInProgress = 0;
bool GetContractAndRoutesNowFlag()
{
	return doGetContractAndRoutesNow;
}
void ResetContractAndRoutesNowFlag()
{
	doGetContractAndRoutesNow = false;
}
void ResetFirmDlInProgress()
{
	FirmDlInProgress = 0;
}
struct FirmDlTransfer
{
	int			totalPackets;
	CMicroSec	startTime;
};
static std::map<MAC, FirmDlTransfer> FirmProgress;
void ResetFirmwaresInProgress(DevicesManager &devices)
{
	LOG_INFO("Firmware downloads are being reset now... ");
	static std::map<MAC, FirmDlTransfer>::iterator it = FirmProgress.begin();
	for (; it != FirmProgress.end(); ++it)
	{
		DevicePtr dev = devices.FindDevice(it->first);
		if (!dev)
			devices.SetFirmDlStatus(dev->id, FWTYPE(dev->Type()), DevicesManager::FirmDlStatus_Failed);
	}
	LOG_INFO("Firmware downloads reset is done!");
}

struct bulkData
{
	unsigned short	port;
	unsigned short	objID;
	int				cmdID;
	bulkData(int port_, int objID_, int cmdID_):port(port_), objID(objID_), cmdID(cmdID_){}
};
static std::map<int/*deviceID*/, bulkData>	BulkInProgress;

bool IsBulkInProgreass(int deviceID)
{
	return BulkInProgress.find(deviceID) != BulkInProgress.end() ? true : false;
}

void AddBulkInProgress(int deviceID, unsigned short port, unsigned short objID, int cmdID)
{
	if (BulkInProgress.insert(std::map<int/*deviceID*/, bulkData>::value_type(deviceID, bulkData(port,objID,cmdID))).second == false)
		LOG_ERROR("already bulk in progress!");
}



void RemoveBulkInProgress(int deviceID)
{
	std::map<int/*deviceID*/, bulkData>::iterator i = BulkInProgress.find(deviceID);
	if (i == BulkInProgress.end())
	{
		LOG_ERROR("bulk was not found in progress!");
		return;
	}
		
	BulkInProgress.erase(i);
}
void GetBulkInProgressVal(int deviceID, unsigned short &port, unsigned short &objID, int &cmdID)
{
	std::map<int/*deviceID*/, bulkData>::iterator i = BulkInProgress.find(deviceID);
	if (i == BulkInProgress.end())
	{
		LOG_ERROR("bulk was not found in progress!");
		return;
	}
	port = i->second.port;
	objID = i->second.objID;
	cmdID = i->second.cmdID;
}

//added by Cristian.Guef - for publish_indication, watchdog_timer and sub_timer
static PublishedDataMng	PublishingManager;

//added by Cristian.Guef
unsigned int MonitH_GW_SessionID = 0;

//added by Cristian.Guef
static boost::shared_ptr<GNetworkHealthReport>		NetHealthRepPtr;
static std::map<IPv6, boost::shared_ptr<GDeviceHealthReport> >	DeviceHealthRep;
static std::map<IPv6, boost::shared_ptr<GNeighbourHealthReport> >	NeighbourHealthRep;
static boost::shared_ptr<GTopologyReport>			TopoRepPtr;
static boost::shared_ptr<GDeviceListReport>			DevListRepPtr;
static std::map<unsigned int, bool> DiagIDtoFirstTopoStatus;  // true - success
															  // false - failure
static ReportsProcessor			RepProcessor;



bool IsFullAPI(int pubHandle)
{
	if (pubHandle != -1)
	{
		if (PublishingManager.IsHandleValid(pubHandle))
			return PublishingManager.GetInterfaceType(pubHandle) == 1/*FULL API*/;
	}
	return false;
}

//added by Cristian.Guef
void ChangeDataVersionNo(int handle, unsigned char version)
{
	PublishingManager.SaveContentVersion((PublishedDataMng::PublishHandle) handle, version);
}
void ChangeInterfaceType(int handle, unsigned char interfaceType)
{
	PublishingManager.SaveInterfaceType((PublishedDataMng::PublishHandle) handle, interfaceType);
}

static unsigned short GetSize(unsigned char format)
{
	if (format == 0 || format == 3)
		return 1;
	if (format == 1 || format == 4)
		return 2;
	if (format == 2 || format == 5 || format == 6)
		return 4;

	LOG_ERROR("ERROR GetSize: unknown size for format " << format);
	return 0;
	//assert(false);
}

void NewPublishChannles(int handle, const PublisherConf::COChannelListT &list)
{
	//build parsing info
	std::vector<Subscribe::ObjAttrSize> parseList;
	parseList.resize(list.size());

	for(unsigned int i = 0; i < list.size(); i++)
	{
		parseList[i].ObjID = list[i].objID;
		parseList[i].AttrID = list[i].attrID;
		parseList[i].AttrIndex = list[i].index1 << 8 | list[i].index2;
		parseList[i].Size = GetSize(list[i].format);
		parseList[i].withStatus = list[i].withStatus;
		LOG_DEBUG("publish set enviroment withstaus=" << list[i].withStatus << "for index=" << i);
	}
	PublishingManager.SaveParsingInfo((PublishedDataMng::PublishHandle)handle, parseList);

	//build interpret list
	PublishedDataMng::InterpretInfoListT interpretList;
	interpretList.resize(list.size());
	for(unsigned int i = 0; i < list.size(); i++)
	{
		interpretList[i].channelDBNo = list[i].dbChannelNo;
		interpretList[i].dataFormat = list[i].format;
	}
	PublishingManager.SetInterpretInfo((PublishedDataMng::PublishHandle)handle, interpretList);
}

void SetFreshStatus(int handle, bool val)
{
	PublishingManager.SetFreshDataStatus(handle, val);
}
int GetCurrentReadingsNo()
{
	return PublishingManager.GetAllReadingsNo();
}
void SaveCurrentReadings(DevicesManager* devices)
{
	devices->SaveReadings(&PublishingManager);
}


void SetSaveToDBSigFlag(bool val)
{
	PublishingManager.SetSaveToDBSigFlag(val);
}
bool GetSaveToDBSigFlag()
{
	return PublishingManager.GetSaveToDBSigFlag();
}

//added by Cristian.Guef
struct ForReadMultipleObjAttrsCmd
{
	ReadMultipleObjectAttributes::AttributesList MultipleReadObjAttrs;
	int BadFragmentCount;
	int GoodFragmentCount;
};
static std::map<unsigned int, ForReadMultipleObjAttrsCmd> DialogueIDToReadMultipleObjs;

/* commented by Cristian.Guef
void ResponseProcessor::Process(AbstractGService& response, Command& command_, CommandsProcessor& processor_)
{
	processor = &processor_;
	command = &command_;
	response.Accept(*this);
}
*/
//added by Cristian.Guef
void ResponseProcessor::Process(AbstractGServicePtr response_, Command& command_, CommandsProcessor& processor_)
{
	processor = &processor_;
	command = &command_;
	response = response_;
	response->Accept(*this);
}

void ResponseProcessor::IssueReportReqs()
{
	////to add JointCount for every device in topology
	//boost::shared_ptr<GNetworkHealthReport> netHealthReport(new GNetworkHealthReport());
	//processor->SendRequest(netHealthReport);

	////add signal quality for every neighbour in topology
	//for(int i = 0; i < TopoRepPtr->DevicesList.size(); i++)
	//{
	//	if (TopoRepPtr->DevicesList[i].DeviceType == 2/*gateway*/ || 
	//			TopoRepPtr->DevicesList[i].DeviceType == 1/*system manager*/)
	//		continue;
	//	GNeighbourHealthReport *pNeighbour = new GNeighbourHealthReport();
	//	pNeighbour->m_forDeviceIP = TopoRepPtr->DevicesList[i].DeviceIP;
	//	boost::shared_ptr<GNeighbourHealthReport> neighbour(pNeighbour);
	//	processor->SendRequest(neighbour);
	//}
}

void ResponseProcessor::LoadSubscribeLeaseForDel()
{
	//now add new contracts to be deleted
	LOG_INFO("the no. of leases to delete is = " << 
				(boost::uint32_t)processor->devices.queueToDeleteLease.size());
	for ( ;processor->devices.queueToDeleteLease.size() != 0; )
	{
		DevicesManager::InfoToDelete info = processor->devices.queueToDeleteLease.front();
		processor->devices.queueToDeleteLease.pop();
		
		DoDeleteLeases::InfoForDelContract infoToDelete;
		unsigned int ContractID = 0;

		infoToDelete.IPAddress = info.IPAddress;
		infoToDelete.DevID = info.DevID;
		infoToDelete.LeaseType = info.LeaseType;
		infoToDelete.ResourceID = info.ResourceID;
		ContractID = info.ContractID;

		processor->AddNewInfoForDelLease(ContractID, infoToDelete);
	}
}

void ResponseProcessor::SaveTopology(Command::ResponseStatus topoStatus)
{

	if (topoStatus != Command::rsSuccess || TopoRepPtr == NULL)
	{
		GTopologyReport noTopolgy;
		processor->devices.ProcessTopology(NULL, noTopolgy);
	
		//clear also net_health_rep
		GNetworkHealthReport *pnoHealthReport = new GNetworkHealthReport();
		NetHealthRepPtr.reset(pnoHealthReport);
		
		//clear also neigbour_list
		NeighbourHealthRep.clear();
		DeviceHealthRep.clear();
		
		processor->CommandFailed(*command, nlib::CurrentUniversalTime(), topoStatus);
		return;
	}

	//we have topology so...
	//IssueReportReqs();

	RepProcessor.FillTopoStructWithSignalQuality(*TopoRepPtr, NeighbourHealthRep.begin(), NeighbourHealthRep.end(), NeighbourHealthRep.size());
	RepProcessor.FillTopoStructWithDevHealth(*TopoRepPtr, DeviceHealthRep.begin(), DeviceHealthRep.end(), DeviceHealthRep.size());
	if (NetHealthRepPtr)
		RepProcessor.FillTopoStructWithJoinCount(*TopoRepPtr, *NetHealthRepPtr);
	if (DevListRepPtr)
		RepProcessor.FillTopoStructWithMACAndType(*TopoRepPtr, *DevListRepPtr);
				
	try
	{
		processor->devices.ProcessTopology(&RepProcessor, *TopoRepPtr);
	}
	catch(std::exception &ex)
	{
		//delete topology
		GTopologyReport noTopolgy;
		processor->devices.ProcessTopology(NULL, noTopolgy);
	
		LOG_ERROR("topo_struct couldn't be processed");
				
		throw ex;
	}
	
	LoadSubscribeLeaseForDel();
	processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), "success");
}

void ResponseProcessor::Visit(GTopologyReport& topologyReport)
{
	//added by Cristian.Guef
	LOG_INFO("topo_rep_resp with diagID = " << (boost::uint32_t)topologyReport.m_DiagID
			<< "and with status =  " << (boost::int32_t)topologyReport.Status);
	
	if (topologyReport.Status == Command::rsSuccess)
	{
		//added by Cristian.guef
		TopoRepPtr = boost::shared_dynamic_cast<GTopologyReport>(response);
		
		//added by Cristian.Guef
		RepProcessor.CreateIndexForTopoStruct(*TopoRepPtr);
		RepProcessor.FillDevListIndexForNeighbours(*TopoRepPtr);

		//added by Cristian.Guef
		std::map<unsigned int, bool>::iterator i = DiagIDtoFirstTopoStatus.find(topologyReport.m_DiagID);
		if (i != DiagIDtoFirstTopoStatus.end()/*there is first topo_status and its value doesn't matter*/)
		{
			SaveTopology(topologyReport.Status);
			DiagIDtoFirstTopoStatus.erase(i);
		}
		else
		{
			//do nothing cause there is no dev_list_rep
			DiagIDtoFirstTopoStatus[topologyReport.m_DiagID] = true;
		}
		
		
		/* commented by Cristian.Guef
		processor->devices.ProcessTopology(topologyReport);
		processor->commands.SetCommandResponded(*command, nlib::CurrentLocalTime(), "success");
		*/
	}
	else
	{
		/* commented by Cristian.Guef
		processor->CommandFailed(*command, nlib::CurrentLocalTime(), deviceListReport.Status);
		*/

		//added by Cristian.Guef
		std::map<unsigned int, bool>::iterator i = DiagIDtoFirstTopoStatus.find(topologyReport.m_DiagID);
		if (i != DiagIDtoFirstTopoStatus.end()) //there is first topo_status
		{
			if (i->second == true /*topo_status = succes*/)
			{
				SaveTopology(Command::rsSuccess);
			}
			else
			{
				SaveTopology(topologyReport.Status);
			}
			DiagIDtoFirstTopoStatus.erase(i);
		}
		else
		{
			//do not signal failed cause there is no dev_list_rep
			DiagIDtoFirstTopoStatus[topologyReport.m_DiagID] = false;
		}
	}
	
}

//added by Cristian.Guef
void ResponseProcessor::Visit(GDeviceListReport& deviceListReport)
{

	//added by Cristian.Guef
	LOG_INFO("dev_list_rep_resp with diagID = " << (boost::uint32_t)deviceListReport.m_DiagID
			<< "and with status =  " << (boost::int32_t)deviceListReport.Status);
	
	if (deviceListReport.m_DiagID == -1)
	{
		if (deviceListReport.Status == Command::rsSuccess)
		{
			DevListRepPtr = boost::shared_dynamic_cast<GDeviceListReport>(response);
			processor->devices.ProcessDeviceList(DevListRepPtr);
			LoadSubscribeLeaseForDel();
			processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), "success");
			return;
		}
		processor->CommandFailed(*command, nlib::CurrentUniversalTime(), deviceListReport.Status);
		return;
	}

	if (deviceListReport.Status == Command::rsSuccess)
	{

		//added by Cristian.guef
		DevListRepPtr = boost::shared_dynamic_cast<GDeviceListReport>(response);
		
		//added by Cristian.Guef
		std::map<unsigned int, bool>::iterator i = DiagIDtoFirstTopoStatus.find(deviceListReport.m_DiagID);
		if (i != DiagIDtoFirstTopoStatus.end()/*there is first topo_status and its value doesn't matter*/)
		{
			SaveTopology(deviceListReport.Status);
			DiagIDtoFirstTopoStatus.erase(i);
		}
		else
		{
			//do nothing cause there is no topo_rep
			DiagIDtoFirstTopoStatus[deviceListReport.m_DiagID] = true;
		}
		

		/* commented by Cristian.Guef
		processor->devices.ProcessTopology(topologyReport);
		processor->commands.SetCommandResponded(*command, nlib::CurrentLocalTime(), "success");
		*/
	}
	else
	{
		/* commented by Cristian.Guef
		processor->CommandFailed(*command, nlib::CurrentLocalTime(), deviceListReport.Status);
		*/

		//added by Cristian.Guef
		std::map<unsigned int, bool>::iterator i = DiagIDtoFirstTopoStatus.find(deviceListReport.m_DiagID);
		if (i != DiagIDtoFirstTopoStatus.end())
		{
			if (i->second == true /*topo_status = succes*/)
			{
				SaveTopology(Command::rsSuccess);
			}
			else
			{
				SaveTopology(deviceListReport.Status);
			}
			DiagIDtoFirstTopoStatus.erase(i);
		}
		else
		{
			//do not signal failed cause there is no topo_rep
			DiagIDtoFirstTopoStatus[deviceListReport.m_DiagID] = false;
		}
	}

}

//added by Cristian.Guef
void ResponseProcessor::Visit(GNetworkHealthReport& networkHealthReport)
{
	if (networkHealthReport.Status == Command::rsSuccess)
	{
		
	  //only not automatic netHeathReport request are saved
		//if (command->generatedType == Command::cgtAutomatic)
	    //{
		   NetHealthRepPtr = boost::shared_dynamic_cast<GNetworkHealthReport>(response);
		   if (TopoRepPtr)
			   RepProcessor.FillNetworkHealthRepWithDevID(*NetHealthRepPtr, *TopoRepPtr);
		   
		   try
		   {
			   processor->devices.factoryDal.BeginTransaction();
				processor->devices.ChangeNetworkHealthInfo(*NetHealthRepPtr);
				processor->devices.factoryDal.CommitTransaction();
		   }
		   catch(std::exception ex)
		   {
			   processor->devices.factoryDal.RollbackTransaction();
			   LOG_ERROR("network_health_couldn't be saved");
			   throw ex;
		   }

	    //}
		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), "success");
	}
	else
	{
		 processor->CommandFailed(*command, nlib::CurrentUniversalTime(), networkHealthReport.Status);
	}
}

//added by Cristian.Guef
void ResponseProcessor::Visit(GScheduleReport& scheduleReport)
{
	if (scheduleReport.Status == Command::rsSuccess)
	{
		try
		{
			processor->devices.factoryDal.BeginTransaction();
			processor->devices.ChangeScheduleInfo(scheduleReport);
			processor->devices.factoryDal.CommitTransaction();
		}
		catch(std::exception ex)
		{
			processor->devices.factoryDal.RollbackTransaction();
			LOG_ERROR("network_health_couldn't be saved");
			throw ex;
		}

		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), "success");
	}
	else
	{
		processor->CommandFailed(*command, nlib::CurrentUniversalTime(), scheduleReport.Status);
	}
}

//added by Cristian.Guef
void ResponseProcessor::SaveNeighbourHealthHistoryToDB(GNeighbourHealthReport& neighbourHealthReport)
{
	std::pair< std::map<IPv6, boost::shared_ptr<GNeighbourHealthReport> >::iterator, bool > pr;

	pr = NeighbourHealthRep.insert(std::map<IPv6, boost::shared_ptr<GNeighbourHealthReport> >::value_type(
				neighbourHealthReport.m_forDeviceIP, boost::shared_dynamic_cast<GNeighbourHealthReport>(response)));

	if (pr.second == false)
	{
		
		int dbDevID = 0;
		if (pr.first->second->m_topoStructIndex == -1 || pr.first->second->m_topoStructIndex >= (int)TopoRepPtr->DevicesList.size())
		{
			DevicePtr devPtr = processor->devices.FindDevice(neighbourHealthReport.m_devID); 
			if (!devPtr) //no device in db
				return;
			dbDevID = devPtr->id;
		}
		else
		{
			dbDevID = TopoRepPtr->DevicesList[pr.first->second->m_topoStructIndex].device_dbID;
			//it may happen a device to be in topology but not in dev_list...
			if (dbDevID == 0)
			{
				DevicePtr devPtr = processor->devices.FindDevice(neighbourHealthReport.m_devID); 
				if (!devPtr) //no device in db
					return;
				dbDevID = devPtr->id;
			}
		}
		//we should
		neighbourHealthReport.m_topoStructIndex = pr.first->second->m_topoStructIndex;

		try
		{

			processor->devices.factoryDal.BeginTransaction();

			GNeighbourHealthReport::NeighboursMapT::const_iterator k = pr.first->second->NeighboursMap.begin();	
			for(GNeighbourHealthReport::NeighboursMapT::iterator j = neighbourHealthReport.NeighboursMap.begin();
					j != neighbourHealthReport.NeighboursMap.end(); j++)
			{
				int dbNeighbID = 0;
				if (k == pr.first->second->NeighboursMap.end())
				{
					DevicePtr neighbPtr = processor->devices.repository.Find(j->first);
					if (!neighbPtr)
						continue;
					dbNeighbID = neighbPtr->id;
					processor->devices.AddNeighbourHealthHistory(dbDevID,
									neighbourHealthReport.timestamp, dbNeighbID, j->second);
					continue;
				}

				if (j->first == k->first)
				{
					// found a match
				}
				else 
				{
					if (k->first < j->first)
					{
						bool finished = false;
						do
						{
							k++;
							if (k == pr.first->second->NeighboursMap.end())
							{
								finished = true;
								break;
							}
						}while (k->first < j->first);
						if (finished)
						{
							DevicePtr neighbPtr = processor->devices.repository.Find(j->first);
							if (!neighbPtr)
								continue;
							dbNeighbID = neighbPtr->id;
							processor->devices.AddNeighbourHealthHistory(dbDevID,
									neighbourHealthReport.timestamp, dbNeighbID, j->second);
							continue;
						}
						if (j->first < k->first)
						{
							DevicePtr neighbPtr = processor->devices.repository.Find(j->first);
							if (!neighbPtr)
								continue;
							dbNeighbID = neighbPtr->id;
							processor->devices.AddNeighbourHealthHistory(dbDevID,
									neighbourHealthReport.timestamp, dbNeighbID, j->second);
							continue;
						}

						//found a match;
					}
					else
					{
						DevicePtr neighbPtr = processor->devices.repository.Find(j->first);
						if (!neighbPtr)
							continue;
						dbNeighbID = neighbPtr->id;
						processor->devices.AddNeighbourHealthHistory(dbDevID,
									neighbourHealthReport.timestamp, dbNeighbID, j->second);
						continue;
					}	
				}

				
				if (k->second.topoStructIndex == -1 || k->second.topoStructIndex >= (int)TopoRepPtr->DevicesList.size())
				{
					DevicePtr neighbPtr = processor->devices.repository.Find(j->first);
					if (!neighbPtr)
						continue;
					dbNeighbID = neighbPtr->id;
				}
				else
				{
					dbNeighbID = TopoRepPtr->DevicesList[k->second.topoStructIndex].device_dbID;
					//it may happen a device to be in topology but not in dev_list...
					if (dbNeighbID == 0)
					{
						DevicePtr neighbPtr = processor->devices.repository.Find(j->first);
						if (!neighbPtr)
							continue;
						dbNeighbID = neighbPtr->id;
					}
				}
				
				LOG_DEBUG("neighborsHealth -> root dev=" << neighbourHealthReport.m_forDeviceIP.ToString() <<" with id=" << dbDevID
					 << " neighborsHealth -> neighb dev=" << k->first.ToString() <<" with id=" << dbNeighbID);
				
				//we should
				j->second.topoStructIndex = k->second.topoStructIndex;

				if (j->second.linkStatus != k->second.linkStatus || j->second.DPDUsTransmitted != k->second.DPDUsTransmitted ||
						j->second.DPDUsReceived != k->second.DPDUsReceived || j->second.DPDUsFailedTransmission != k->second.DPDUsFailedTransmission ||
						j->second.DPDUsFailedReception != k->second.DPDUsFailedReception || j->second.signalStrength != k->second.signalStrength ||
						j->second.signalQuality != k->second.signalQuality)
				{

					processor->devices.AddNeighbourHealthHistory(dbDevID,
									neighbourHealthReport.timestamp, dbNeighbID, j->second);
				}
				k++;
			}

			processor->devices.factoryDal.CommitTransaction();
			pr.first->second = boost::shared_dynamic_cast<GNeighbourHealthReport>(response);
		}
		catch(std::exception ex)
		{
			processor->devices.factoryDal.RollbackTransaction();
			LOG_ERROR("neighbours_health_couldn't be saved");
			throw ex;
		}
	}
	else
	{
		DevicePtr devPtr = processor->devices.FindDevice(neighbourHealthReport.m_devID);
		if (!devPtr)	//no device in db
			return;

		try
		{

			processor->devices.factoryDal.BeginTransaction();
			for(GNeighbourHealthReport::NeighboursMapT::iterator j = neighbourHealthReport.NeighboursMap.begin();
				j != neighbourHealthReport.NeighboursMap.end(); j++)
			{
				DevicePtr neighbPtr = processor->devices.repository.Find(j->first);
				if (!neighbPtr)
					continue;
				
				processor->devices.AddNeighbourHealthHistory(devPtr->id,
							neighbourHealthReport.timestamp, neighbPtr->id, j->second);
				
			}
			processor->devices.factoryDal.CommitTransaction();
			pr.first->second = boost::shared_dynamic_cast<GNeighbourHealthReport>(response);
		}
		catch(std::exception ex)
		{
			processor->devices.factoryDal.RollbackTransaction();
			LOG_ERROR("first neighbours_health_couldn't be saved");
			throw ex;
		}
	}

}

//added by Cristian.Guef
void ResponseProcessor::Visit(GNeighbourHealthReport& neighbourHealthReport)
{
	if (neighbourHealthReport.Status == Command::rsSuccess)
	{
		//if (command->generatedType == Command::cgtAutomatic)
			//SaveNeighbourHealthHistoryToDB(neighbourHealthReport);

		DevicePtr devPtr = processor->devices.FindDevice(neighbourHealthReport.m_devID);
		if (!devPtr)	//no device in db
		{
			LOG_WARN("neighbors_health -> device with dbID = " << neighbourHealthReport.m_devID << "not found in cache");
			processor->commands.SetCommandResponded(*command, neighbourHealthReport.timestamp, "success");
			return;
		}

		for(GNeighbourHealthReport::NeighboursMapT::iterator j = neighbourHealthReport.NeighboursMap.begin();
			j != neighbourHealthReport.NeighboursMap.end(); j++)
		{
			DevicePtr neighbPtr = processor->devices.repository.Find(j->first);
			if (!neighbPtr)
				continue;
			
			processor->devices.AddNeighbourHealthHistory(devPtr->id,
						neighbourHealthReport.timestamp, neighbPtr->id, j->second);
			
		}
			
		//add to cache
		std::pair< std::map<IPv6, boost::shared_ptr<GNeighbourHealthReport> >::iterator, bool > pr;
		pr = NeighbourHealthRep.insert(std::map<IPv6, boost::shared_ptr<GNeighbourHealthReport> >::value_type(
				neighbourHealthReport.m_forDeviceIP, boost::shared_dynamic_cast<GNeighbourHealthReport>(response)));
		if (pr.second == false) //already in map
			pr.first->second = boost::shared_dynamic_cast<GNeighbourHealthReport>(response);

		processor->commands.SetCommandResponded(*command, neighbourHealthReport.timestamp, "success");
	}
	else
	{
		processor->CommandFailed(*command, nlib::CurrentUniversalTime(), neighbourHealthReport.Status);
	}
}

//added by Cristian.Guef
void ResponseProcessor::SaveDeviceHealthHistoryToDB(GDeviceHealthReport& devHealthReport)
{
	std::pair< std::map<IPv6, boost::shared_ptr<GDeviceHealthReport> >::iterator, bool > pr;

	pr = DeviceHealthRep.insert(std::map<IPv6, boost::shared_ptr<GDeviceHealthReport> >::value_type(
				devHealthReport.m_forDeviceIPs[0], boost::shared_dynamic_cast<GDeviceHealthReport>(response)));

	if (pr.second == false)	//already in map
	{
		int dbDevID;
		if (pr.first->second->DeviceHealthList[0].topoStructIndex == -1)
		{
			DevicePtr devPtr = processor->devices.FindDevice(devHealthReport.m_devIDs[0]); 
			if (!devPtr) //no device in db
				return;
			dbDevID = devPtr->id;
		}
		else
		{
			dbDevID = TopoRepPtr->DevicesList[pr.first->second->DeviceHealthList[0].topoStructIndex].device_dbID;
			//it may happen a device to be in topology but not in dev_list...
			if (dbDevID == 0)
			{
				DevicePtr devPtr = processor->devices.FindDevice(devHealthReport.m_devIDs[0]); 
				if (!devPtr) //no device in db
					return;
				dbDevID = devPtr->id;
			}
		}
		
		processor->devices.UpdateDeviceHealth(dbDevID, devHealthReport.DeviceHealthList[0]);
		
		//we should
		devHealthReport.DeviceHealthList[0].topoStructIndex = pr.first->second->DeviceHealthList[0].topoStructIndex;

		if (devHealthReport.DeviceHealthList[0].DPDUsFailedReception != pr.first->second->DeviceHealthList[0].DPDUsFailedReception ||
				devHealthReport.DeviceHealthList[0].DPDUsFailedTransmission != pr.first->second->DeviceHealthList[0].DPDUsFailedTransmission ||
				devHealthReport.DeviceHealthList[0].DPDUsReceived != pr.first->second->DeviceHealthList[0].DPDUsReceived ||
				devHealthReport.DeviceHealthList[0].DPDUsTransmitted != pr.first->second->DeviceHealthList[0].DPDUsTransmitted)
		{
			
			processor->devices.AddDeviceHealthHistory(dbDevID,
									devHealthReport.timestamp,
									devHealthReport.DeviceHealthList[0]);
					
			pr.first->second->DeviceHealthList[0].DPDUsFailedReception = devHealthReport.DeviceHealthList[0].DPDUsFailedReception;
			pr.first->second->DeviceHealthList[0].DPDUsFailedTransmission = devHealthReport.DeviceHealthList[0].DPDUsFailedTransmission;
			pr.first->second->DeviceHealthList[0].DPDUsReceived = devHealthReport.DeviceHealthList[0].DPDUsReceived;
			pr.first->second->DeviceHealthList[0].DPDUsTransmitted = devHealthReport.DeviceHealthList[0].DPDUsTransmitted;
		}
	}
	else
	{

		DevicePtr devPtr = processor->devices.FindDevice(devHealthReport.m_devIDs[0]); 
		if (!devPtr) //no device in db
			return;
		
		processor->devices.AddDeviceHealthHistory(devPtr->id,
								devHealthReport.timestamp,
								devHealthReport.DeviceHealthList[0]);
								
		processor->devices.UpdateDeviceHealth(devPtr->id, devHealthReport.DeviceHealthList[0]);
	}

}

//added by Cristian.Guef
void ResponseProcessor::Visit(GDeviceHealthReport& devHealthReport)
{
	if (devHealthReport.Status == Command::rsSuccess)
	{
		//if (command->generatedType == Command::cgtAutomatic)
		//{
			if (devHealthReport.DeviceHealthList.size() != 1)
			{
				LOG_WARN("dev_health_rep response for automatic processing has no size = " << devHealthReport.DeviceHealthList.size());
				processor->commands.SetCommandResponded(*command, devHealthReport.timestamp, "success");
				return;
			}

			//for comparing reports
			//SaveDeviceHealthHistoryToDB(devHealthReport);
			
			//for not comparing reports
			DevicePtr devPtr = processor->devices.FindDevice(devHealthReport.m_devIDs[0]); 
			if (!devPtr) //no device in db
			{
				LOG_WARN("device_health -> device with dbID = " << devHealthReport.m_devIDs[0] << "not found in cache");
				processor->commands.SetCommandResponded(*command, devHealthReport.timestamp, "success");
				return;
			}
			processor->devices.AddDeviceHealthHistory(devPtr->id,
									devHealthReport.timestamp,
									devHealthReport.DeviceHealthList[0]);				
			processor->devices.UpdateDeviceHealth(devPtr->id, devHealthReport.DeviceHealthList[0]);
		//}

			//add to cache
			std::pair< std::map<IPv6, boost::shared_ptr<GDeviceHealthReport> >::iterator, bool > pr;
			pr = DeviceHealthRep.insert(std::map<IPv6, boost::shared_ptr<GDeviceHealthReport> >::value_type(
					devHealthReport.m_forDeviceIPs[0], boost::shared_dynamic_cast<GDeviceHealthReport>(response)));
			if (pr.second == false)	//already in map
			{
				pr.first->second->DeviceHealthList[0].DPDUsFailedReception = devHealthReport.DeviceHealthList[0].DPDUsFailedReception;
				pr.first->second->DeviceHealthList[0].DPDUsFailedTransmission = devHealthReport.DeviceHealthList[0].DPDUsFailedTransmission;
				pr.first->second->DeviceHealthList[0].DPDUsReceived = devHealthReport.DeviceHealthList[0].DPDUsReceived;
				pr.first->second->DeviceHealthList[0].DPDUsTransmitted = devHealthReport.DeviceHealthList[0].DPDUsTransmitted;
			}


			processor->commands.SetCommandResponded(*command, devHealthReport.timestamp, "success");
	}
	else
		processor->CommandFailed(*command, nlib::CurrentUniversalTime(), devHealthReport.Status);
}

//added by Cristian.Guef
void ResponseProcessor::Visit(GNetworkResourceReport& netResourceReport)
{
	if (netResourceReport.Status == Command::rsSuccess)
		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), "success");
	else
		processor->CommandFailed(*command, nlib::CurrentUniversalTime(), netResourceReport.Status);
}
//added by Cristian.Guef
void ResponseProcessor::Visit(GAlert_Subscription& alertSubscription)
{
	if (alertSubscription.Status == Command::rsSuccess)
	{
		DevicePtr dev = processor->devices.GatewayDevice();
		dev->HasMadeAlertSubscription(true);

		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), "success");
	}
	else
	{
		if (alertSubscription.Status == Command::rsFailure_GatewayUnknown)
		{
			DevicePtr device = GetDevice(command->deviceID);
			device->ResetContractID(0/*no resource*/, GContract::Alert_Subscription);
			processor->UnregisterAlertIndication();
			//set command_status as new
			//processor->commands.SetDBCmdID_as_new(command->commandID);
			LOG_INFO(boost::str(boost::format(" lease expired for DeviceID=%1%") % command->deviceID));
		}
		else
		{
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), alertSubscription.Status);
		}
	}
}

static void gettime(nlib::DateTime &ReadingTime, short &milisec, struct timeval &tv)
{
  time_t curtime;
  curtime=tv.tv_sec;
  struct tm * timeinfo;

  timeinfo = gmtime(&curtime);
  ReadingTime = nlib::CreateTime(timeinfo->tm_year+1900, timeinfo->tm_mon + 1,
  									timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, 
  									timeinfo->tm_sec);
  milisec = tv.tv_usec/1000;
  
  //printf ("\n Current local time and date: %s.%d", nlib::ToString(myDT.ReadingTime).c_str(), myDT.milisec);
  //fflush(stdout);
  
}

//added by Cristian.Guef
#define UDO_ALERT_START_ENTRY_SIZE	(MAC::SIZE + sizeof(unsigned short) + sizeof(unsigned short) + sizeof(unsigned long))
#define UDO_ALERT_PROGRESS_ENTRY_SIZE	(MAC::SIZE + sizeof(unsigned short))
void ResponseProcessor::Visit(GAlert_Indication& alertIndication)
{
	if (alertIndication.Status == Command::rsSuccess)
	{

		DevicePtr device = processor->devices.repository.Find(alertIndication.NetworkAddress);
		if (!device)
		{
			LOG_WARN("alert indication received from unregistered device_ip=" << alertIndication.NetworkAddress.ToString());
			return;
		}
		nlib::DateTime dateTime;
		short milisec;
		/// convert Time fields to standard struct timeval meaning: seconds UTC, microseconds (10^-6 increments)
		/// Time fields as received from GW are: Time.Seconds TAI, Time.FractionOfSeconds TAI (2^-15 increments)
		struct timeval tv ={alertIndication.Time.Seconds - (TAI_OFFSET + CurrentUTCAdjustment), ((uint32_t)alertIndication.Time.FractionOfSeconds * 1000000) >> 16 };
		gettime(dateTime, milisec, tv);


		if (device->Type() == Device::dtSystemManager)
		{
			if (alertIndication.ObjectID == UPLOAD_DOWNLOAD_OBJECT)//we have firmware_download
			{
				switch (alertIndication.Type)
				{
				case 0/*transfer_started*/:
					{
						int entries = alertIndication.Data.size()/UDO_ALERT_START_ENTRY_SIZE;
						unsigned char *pData = (unsigned char*)alertIndication.Data.c_str();
						int offset = 0;
						for (int k = 0; k < entries; ++k)
						{
							//read data
							MAC sourceAddress = MAC(pData + offset);
							offset += MAC::SIZE;
							//unsigned short bytesPerPacket = htons(*((unsigned short*)(pData + offset)));
							offset += sizeof(unsigned short);	//bytesPerPacket
							unsigned short totalPackets = htons(*((unsigned short*)(pData + offset)));
							offset += sizeof(unsigned short);
							//unsigned int totalBytes = htonl(*((unsigned long*)(pData + offset)));
							offset += sizeof(unsigned long); //totalBytes

							//process
							DevicePtr device = processor->devices.FindDevice(sourceAddress);
							if (!device)
							{
								LOG_WARN("AlertIndication: (transfer_started) device not found, mac=" << sourceAddress.ToString());
								continue;
							}
							FirmDlInProgress++;
							if (FirmDlInProgress == 1)
								doGetContractAndRoutesNow = true;
							processor->devices.SetFirmDlSize(device->id, FWTYPE(device->Type() ), totalPackets);
							processor->devices.SetFirmDlPercent(device->id, FWTYPE(device->Type()), 0);
							
							std::pair<std::map<MAC,FirmDlTransfer>::iterator, bool> pr = FirmProgress.insert(std::map<MAC,FirmDlTransfer>::value_type(sourceAddress, FirmDlTransfer()));
							CMicroSec now;
							pr.first->second.totalPackets = totalPackets;
							pr.first->second.startTime = now;

							processor->devices.SetFirmDlSpeed(device->id, FWTYPE(device->Type()), 0);
							processor->devices.SetFirmDlAvgSpeed(device->id, FWTYPE(device->Type()), 0);
							doGetContractAndRoutes = true;
						}
					}
					break;
				case 1/*transfer_progress*/:
					{
						//read data
						int entries = alertIndication.Data.size()/UDO_ALERT_PROGRESS_ENTRY_SIZE;
						unsigned char *pData = (unsigned char*)alertIndication.Data.c_str();
						int offset = 0;
						for(int k = 0; k < entries; ++k)
						{
							MAC sourceAddress = MAC(pData + offset);
							offset += MAC::SIZE;
							unsigned short packetsTransfered = htons(*((unsigned short*)(pData + offset)));
							offset += sizeof(unsigned short);

							//process
							DevicePtr device = processor->devices.FindDevice(sourceAddress);
							if (!device)
							{
								LOG_WARN("AlertIndication: (transfer_started) device not found, mac=" << sourceAddress.ToString());
								continue;
							}
							std::map<MAC, FirmDlTransfer>::iterator i = FirmProgress.find(sourceAddress);
							if (i == FirmProgress.end())
							{
								LOG_ERROR("AlertIndication: (transfer_started) mac not found in transfer_cache, mac=" << sourceAddress.ToString());
								continue;
							}
							else
							{
								processor->devices.SetFirmDlPercent(device->id, FWTYPE(device->Type()), (packetsTransfered*100)/i->second.totalPackets);
							}
							
							std::map<IPv6,FirmDlSpeed>::iterator j = gt_committedBurst_SM_DEV.find(device->IP());
							if (j != gt_committedBurst_SM_DEV.end())
							{
								if (j->second.isSpeedSet)
								{
									int committedBurst = -15;
									if ((committedBurst = j->second.committedBurst) == 0)
										committedBurst = -15;

									if (committedBurst < 0)
										processor->devices.SetFirmDlSpeed(device->id, FWTYPE(device->Type()), 60/(-committedBurst));
									else
										processor->devices.SetFirmDlSpeed(device->id, FWTYPE(device->Type()), 60*committedBurst);
									j->second.isSpeedSet = false;
								}
							}
							else
							{
								LOG_WARN("AlertIndication: (transfer_started) ip not found in transfer_cache, ip=" << device->IP().ToString());	
							}
							/// packets per minute since transfer start
							int nTime = i->second.startTime.GetElapsedMSec();
							if(!nTime) nTime = 1;	/// SIGFPE protection
							processor->devices.SetFirmDlAvgSpeed(device->id, FWTYPE(device->Type()), 60*1000*packetsTransfered/nTime);
						}
					}
					break;
				case 2/*transfer_ended*/:
					{
						//read data
						unsigned char *pData = (unsigned char*)alertIndication.Data.c_str();
						int offset = 0;
						MAC sourceAddress = MAC(pData);
						offset += MAC::SIZE;
						unsigned char error = pData[offset];

						//process
						DevicePtr device = processor->devices.FindDevice(sourceAddress);
						if (!device)
						{
							LOG_WARN("AlertIndication: (transfer_started) device not found, mac=" << sourceAddress.ToString());
							break;
						}
						FirmDlInProgress--;
						doGetContractAndRoutesNow = doGetContractAndRoutes = (FirmDlInProgress > 0);

						if (error == 0)//UDOALERT_OK
						{
							processor->devices.SetFirmDlPercent(device->id, FWTYPE(device->Type()), 100);
						}
						processor->devices.SetFirmDlStatus( device->id, FWTYPE(device->Type()),
							(error == 0) /*UDOALERT_OK*/     ? DevicesManager::FirmDlStatus_Completed :
							(error == 1) /*UDOALERT_CANCEL*/ ? DevicesManager::FirmDlStatus_Cancelled :
							/*(error == 2) UDOALERT_FAIL */    DevicesManager::FirmDlStatus_Failed);
					}
					break;
				default:
					LOG_WARN("AlertIndication: (firmware_download) -> unknown alert_type");
					break;
				}
			}

			if (alertIndication.ObjectID == SYSTEM_MONITORING_OBJECT)//we have join/unjoin
			{
				//we have device join/leave
				switch (alertIndication.Type)
				{
				case 0/*device join*/:

					break;
				case 1/*device */:

					break;
				case 2/*transfer_ended*/:

					break;
				default:
					LOG_WARN("AlertIndication: (device_join/leave) -> unknown alert_type");
					break;
				}
			}
		}

		std::string data;
		char hexVal[20];
		for (unsigned int i = 0; i < alertIndication.Data.size(); ++i)
		{
			sprintf(hexVal, "%02X", alertIndication.Data[i]);
			data += hexVal;
		}
		processor->devices.SaveAlertIndication(device->id, alertIndication.EndpointPort - ISA100_START_PORTS, 
				alertIndication.ObjectID, dateTime, milisec, alertIndication.Class, alertIndication.Direction, 
				alertIndication.Category, alertIndication.Type, alertIndication.Priority, data);
			
	}
}


void ResponseProcessor::ClearDataForDev(GDelContract& contract, int	&DevID, IPv6 &IPAddress)
{
	DoDeleteLeases::InfoForDelContract info = processor->GetInfoForDelLease(contract.ContractID);
	if (info.ResourceID == 0)
	{
		/* it was before
		LOG_ERROR(" response for deleting lease for lease_id =" << (boost::uint32_t)contract.ContractID 
					<< "for dev ip = " << info.IPAddress.ToString() << "-> has been made with resourceID = 0");
		processor->CommandFailed(*command, nlib::CurrentUniversalTime(), contract.Status);
		*/
		//the case when bulk and C/S type so do nothing...
		
		IPAddress = contract.IPAddress;
		DevicePtr device = processor->devices.repository.Find(IPAddress);
		if (!device)
		{
			LOG_WARN("dev_ip = " << IPAddress.ToString() << "not found in db - deleted_LeaseID = " << contract.ContractID);
		}
		else
		{	
			DevID = device->id;
		}
		return;
	}

	DevID = info.DevID;
	IPAddress = info.IPAddress;


	//force to recreate lease if dev_id in db
	DevicePtr device = processor->devices.FindDevice(info.DevID);
	if (!device)
	{
		LOG_WARN("dev_id = " << (boost::uint32_t)info.DevID << "not found in db - deleted_LeaseID = " << contract.ContractID);
	}
	else
	{	
		device->ResetContractID(contract.m_unResourceID, contract.ContractType);
	}

	//for (lease type = 3 -> delete indication from pendings)
	if (info.LeaseType == GContract::Subscriber)
	{
		processor->CancelPublishIndication2(contract.ContractID, (info.ResourceID >> 16), info.IPAddress);

		if (device)
		{
			processor->configApp.ResetLeaseFlagForPub(device->mac);
			if (device->GetPublishHandle() != -1/*invalid handle*/)
			{
				//PublishedDataMng::InterpretInfoListT *pList = PublishingManager.GetInterpretInfo((PublishedDataMng::PublishHandle)device->GetPublishHandle());
				//std::vector<int> list;
				//list.resize(pList->size());
				//for(int i = 0; i < pList->size(); i++)
				//	list[i] = (*pList)[i].channelDBNo;
				//processor->devices.DeletePublishChannels(list);
				PublishingManager.EraseData((PublishedDataMng::PublishHandle)device->GetPublishHandle());
				device->SetPublishHandle(-1);
			}
		}
	}
	

	//deleted from stored data
	processor->DelInfoForDelLease(contract.ContractID);	

}

//added by Cristian.Guef
void ResponseProcessor::Visit(GDelContract& contract)
{

	if (contract.Status == Command::rsSuccess)
	{
		int				DevID;
		IPv6			IPAddress;
		ClearDataForDev(contract, DevID, IPAddress);
		
		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(),
		    boost::str(boost::format("deleted_LeaseID=%1%, objID=%2%, tlsap_id=%3% lease_type=%4% for dev_ip=%5% and dev_id = %6%")
						% contract.ContractID 
						% (boost::uint32_t)(contract.m_unResourceID >> 16)
						% (boost::uint32_t)(contract.m_unResourceID & 0xFFFF)
						% (boost::uint32_t)contract.ContractType
						% IPAddress.ToString()
						% (boost::int32_t)DevID));

	}
	else
	{
		
		if(contract.Status == Command::rsFailure_InvalidReturnedLeaseID || 
			contract.Status == Command::rsFailure_InvalidReturnedLeasePeriod)
		{
			//error deleting lease so retry
			processor->SetDeleteContractID_NotSent(contract.ContractID);

			LOG_ERROR("error resp for deleting lease for lease ID = " << (boost::uint32_t)contract.ContractID << 
					 "and lease type =" << (boost::uint32_t)contract.ContractType << 
					 " and obj_id = " << (boost::uint32_t)(contract.m_unResourceID >> 16) << 
					" and tlsap_id = " << (boost::uint32_t)(contract.m_unResourceID & 0xFFFF) << 
					 " and lease status = " << (boost::int32_t)contract.Status);
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), contract.Status);
		}
		else
		{
			
			int				DevID;
			IPv6			IPAddress;
			ClearDataForDev(contract, DevID, IPAddress);

			LOG_INFO(" lease deleted for lease ID = " << (boost::uint32_t)contract.ContractID << 
					 "and lease_type =" << (boost::uint32_t)contract.ContractType <<
					 " and obj_id = " << (boost::uint32_t)(contract.m_unResourceID >> 16) << 
					" and tlsap_id = " << (boost::uint32_t)(contract.m_unResourceID & 0xFFFF) << 
					 " and lease status = " << (boost::int32_t)contract.Status
					 << "for dev ip = " << IPAddress.ToString()
					 << "and dev id = " << (boost::int32_t)DevID);

			processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(),
				boost::str(boost::format("deleted_LeaseID=%1%, objID=%2%, tlsap_id=%3% lease_type=%4% for dev ip=%5% and dev_id = %6%")
							% contract.ContractID 
							% (boost::uint32_t)(contract.m_unResourceID >> 16)
							% (boost::uint32_t)(contract.m_unResourceID & 0xFFFF)
							% (boost::uint32_t)contract.ContractType
							% IPAddress.ToString()
							% (boost::int32_t)DevID));

		}
	}
}

void ResponseProcessor::Visit(GContract& contract)
{

	DevicePtr device = GetDevice(command->deviceID);

	/* commented by Cristian.Guef
	if (contract.Status == Command::rsSuccess || contract.Status == Command::rsSuccess_LowerPeriod)
	*/
	if (contract.Status == Command::rsSuccess)
	{
		/* commented by CristianGuef
		processor->devices.SetDeviceContract(device, contract.ContractID);
		*/

		//added by Cristian.Guef
		if(contract.ContractID  == 0)
		 {
		 	LOG_ERROR(" error resp for creating lease for lease period = " << 
		  					(boost::uint32_t)contract.ContractPeriod << 
							" with lease ID = " << (boost::uint32_t)contract.ContractID <<
							"the expected lease_id value should be greater than 0" <<
							" for obj_id = " << (boost::uint16_t)(contract.m_unResourceID >> 16) << 
							" and tlsap_id = " << (boost::uint16_t)(contract.m_unResourceID & 0xFFFF) << 
							" and lease_type = " << (boost::uint16_t)contract.ContractType
							<< "and dev id = " << (boost::int32_t)command->deviceID);
		  				
			device->WaitForContract(contract.m_unResourceID, contract.ContractType, false);
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), Command::rsFailure_InvalidReturnedLeaseID);
			Command cmd;
			cmd.commandID = contract.m_dbCmdID;
			processor->CommandFailed(cmd, nlib::CurrentUniversalTime(), Command::rsFailure_ContractFailed);
			return;
		 }

		//added by Cristian.Guef
		/* it was before
		if(contract.ContractPeriod  != 0)
		 {
		 	LOG_ERROR(" error resp for creating lease for lease period = " 
		  				<< (boost::uint32_t)contract.ContractPeriod <<
						"the expected lease period value should be 0" 
		  				<< " with lease ID = " 
		  				<< (boost::uint32_t)contract.ContractID <<
						" for obj_id = " << (boost::uint16_t)(contract.m_unResourceID >> 16) << 
						" and tlsap_id = " << (boost::uint16_t)(contract.m_unResourceID & 0xFFFF) <<
						" and lease_type = " << (boost::uint16_t)contract.ContractType
						<< "and dev id = " << (boost::int32_t)command->deviceID);
		  	
			device->WaitForContract(contract.m_unResourceID, contract.ContractType, false);
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), Command::rsFailure_InvalidReturnedLeasePeriod);
			return;
		 }
		 */

		//added by Cristian.Guef
		unsigned char leaseTypeReplaced = 10;
		processor->devices.SetDeviceContract(device, contract.ContractID, contract.m_unResourceID, contract.ContractType, contract.m_committedBurst, leaseTypeReplaced);
		

		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(),
		    boost::str(boost::format("created_LeaseID=%d, objID=%d, tlsap_id=%d, Lease_type=%d, lease_period=%d and committedBurst=%d")
						% contract.ContractID 
						% (boost::uint32_t)(contract.m_unResourceID >> 16)
						% (boost::uint32_t)(contract.m_unResourceID & 0xFFFF)
						% (boost::uint32_t)contract.ContractType
						% (boost::uint32_t)contract.ContractPeriod
						% (boost::int32_t)contract.m_committedBurst));
						
		if (contract.ContractType == GContract::Alert_Subscription)
		{
			boost::shared_ptr<GAlert_Indication> alertIndication(new GAlert_Indication());
			processor->RegisterAlertIndication(alertIndication);
		}

	}
	else
	{
		/* commented by Cristian.Guef
		device->WaitForContract(false);
		*/
		
		//added by Cristian.Guef
		LOG_ERROR(" error resp for creating lease for lease period = "  << 
					(boost::uint32_t)contract.ContractPeriod << 
					" with lease ID = " << (boost::uint32_t)contract.ContractID <<
					" for obj_id = " << (boost::uint16_t)(contract.m_unResourceID >> 16) << 
					" and tlsap_id = " << (boost::uint16_t)(contract.m_unResourceID & 0xFFFF) <<
					 "and lease type =" << (boost::uint32_t)contract.ContractType <<
					  "and dev id = " << (boost::int32_t)command->deviceID <<
					 " and lease status = " << (boost::int32_t)contract.Status);
			
		//added by Cristian.Guef
		device->WaitForContract(contract.m_unResourceID, contract.ContractType, false);

		//added by Cristian.Guef
		if (contract.Status == Command::rsFailure_InvalidDevice)
		{
			device->Status(Device::dsUnregistered);
		}

		processor->CommandFailed(*command, nlib::CurrentUniversalTime(), contract.Status);
		Command cmd;
		cmd.commandID = contract.m_dbCmdID;
		processor->CommandFailed(cmd, nlib::CurrentUniversalTime(), Command::rsFailure_ContractFailed);
	}
}

//added by Cristian.Guef
void ResponseProcessor::Visit(GSession& session)
{
	LOG_INFO(" session with session period = " << (boost::int32_t)session.m_nSessionPeriod 
		  		<< " and session ID = "  << (boost::int32_t)session.m_unSessionID 
				<< "status = " << (boost::int32_t)session.Status);
		  				
	if (session.Status == Command::rsSuccess)
	{

		if(session.m_nSessionPeriod != -1)
		{
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), Command::rsFailure_InvalidReturnedSessionPeriod);
			return;
		}

		if(session.m_unSessionID == 0)
		{
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), Command::rsFailure_InvalidReturnedSessionID);
			return;
		}

		MonitH_GW_SessionID = session.m_unSessionID;

		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), boost::str(boost::format("SessionID=%1d") % session.m_unSessionID));
	}
	else
	{
		processor->CommandFailed(*command, nlib::CurrentUniversalTime(), session.Status);
	}
}

void ResponseProcessor::Visit(GBulk& bulk)
{
	
	//added by Cristian.Guef
	LOG_INFO(" bulk -> current_state = " << (boost::uint32_t)bulk.m_currentBulkState << 
				" and status = " << (boost::int32_t)bulk.Status << " on C " << bulk.ContractID);
		
	
	if (bulk.Status == Command::rsSuccess)
	{
		//added by Cristian.Guef
		GBulk::BulkStates nextState;
		unsigned int nextCurrentBlockSize = 0;
		unsigned int nextCurrentBlockCount = 0;
		
		//added by Cristian.Guef
		switch (bulk.m_currentBulkState)
		{
		case GBulk::BulkOpen:
			bulk.maxBlockCount = (bulk.Data.size() % bulk.maxBlockSize) == 0 ? 
										bulk.Data.size()/bulk.maxBlockSize - 1 : 
										bulk.Data.size()/bulk.maxBlockSize;

			nextCurrentBlockSize = bulk.maxBlockSize < bulk.Data.size() ? bulk.maxBlockSize : bulk.Data.size();
			nextCurrentBlockCount = 0;
			nextState = GBulk::BulkTransfer;

			//FirmwareDownloadStatus
			if (bulk.type == GBulk::BULK_WITH_DEV)
			{
				CMicroSec now;
				bulk.transferTime = now;
				if (bulk.committedBurst == 0)
					bulk.committedBurst = -15;
				/// TODO: transfer speed should consider both contracts GW -> Device AND Device -> GW committedBurst
				int nSpeed = (bulk.committedBurst < 0) ? (60/(-bulk.committedBurst)) : (60*bulk.committedBurst) ;
				
				/// TODO: concentrate multiple calls SetFirmDl* into a single one. Each individual SetFirmDl* method does a DB update
				processor->devices.SetFirmDlSize(    bulk.devID, FWTYPE_SENZOR_BOARD, bulk.maxBlockCount);
				processor->devices.SetFirmDlPercent( bulk.devID, FWTYPE_SENZOR_BOARD, 0);
				processor->devices.SetFirmDlSpeed(   bulk.devID, FWTYPE_SENZOR_BOARD, nSpeed);
				processor->devices.SetFirmDlAvgSpeed(bulk.devID, FWTYPE_SENZOR_BOARD, nSpeed);
			}

			break;

		case GBulk::BulkTransfer:
			
			//FirmwareDownloadStatus
			if (bulk.type == GBulk::BULK_WITH_DEV)
			{
				/// TODO: concentrate multiple calls SetFirmDl* into a single one. Each individual SetFirmDl* method does a DB update
				processor->devices.SetFirmDlPercent( bulk.devID, FWTYPE_SENZOR_BOARD, (bulk.currentBlockCount*100)/bulk.maxBlockCount);

				int nTime = bulk.transferTime.GetElapsedMSec();
				if(!nTime) nTime = 1;	/// SIGFPE protection
				processor->devices.SetFirmDlAvgSpeed(bulk.devID, FWTYPE_SENZOR_BOARD, 60*1000*(bulk.currentBlockCount+1)/nTime);	///currentBlockCount is zero-based
			}

			nextCurrentBlockCount = bulk.currentBlockCount + 1;
			if ((int)nextCurrentBlockCount > bulk.maxBlockCount)
			{
				nextState = GBulk::BulkEnd;
				break;
			}
			else
			{
				nextState = GBulk::BulkTransfer;
				nextCurrentBlockSize = bulk.maxBlockSize < bulk.Data.size() ? 
								bulk.maxBlockSize : bulk.Data.size();
			}
			break;
		case GBulk::BulkEnd:
			
			if ((bulk.type == GBulk::BULK_WITH_DEV && IsBulkInProgreass(bulk.devID)) || (bulk.type != GBulk::BULK_WITH_DEV))
			{
				if (bulk.type == GBulk::BULK_WITH_DEV)
				{
					unsigned short objID = 0;
					unsigned short tsapid = 0;
					int cmdID = 0;
					GetBulkInProgressVal(bulk.devID, tsapid, objID, cmdID);
					if (cmdID != command->commandID)
					{
						LOG_ERROR(" bulk -> command ID inconsistent " << cmdID << " != " << command->commandID );
						processor->devices.SetFirmDlStatus( bulk.devID, FWTYPE_SENZOR_BOARD, DevicesManager::FirmDlStatus_Failed);
						return;
					}
				}

				if (bulk.type == GBulk::BULK_WITH_DEV)
					RemoveBulkInProgress(bulk.devID);
				if (bulk.maxBlockCount < 0)// ERROR
				{
					//FirmwareDownloadStatus
					if (bulk.type == GBulk::BULK_WITH_DEV)
					{
						processor->devices.SetFirmDlStatus( bulk.devID, FWTYPE_SENZOR_BOARD, DevicesManager::FirmDlStatus_Failed);
					}
					processor->CommandFailed(*command, nlib::CurrentUniversalTime(), (Command::ResponseStatus)bulk.maxBlockCount);
				}
				else
				{
					//FirmwareDownloadStatus
					if (bulk.type == GBulk::BULK_WITH_DEV)
					{
						processor->devices.SetFirmDlPercent(bulk.devID, FWTYPE_SENZOR_BOARD, 100);
						processor->devices.SetFirmDlStatus( bulk.devID, FWTYPE_SENZOR_BOARD, DevicesManager::FirmDlStatus_Completed);
					}
					processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), "success");
				}

				//lease mng
				unsigned short TLDE_SAPID = bulk.port;
				unsigned short ObjID = bulk.objID;
				unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;		
				DevicePtr device = GetDevice(bulk.devID);
				processor->leaseTrackMng.AddLease(bulk.ContractID, device, resourceID, GContract::BulkTransforClient, nlib::util::seconds(processor->configApp.LeasePeriod()));
			}
			return;
		}
		
		//added by Cristian.Guef
		boost::shared_ptr<GBulk> nextBulk(new GBulk());
		nextBulk->transferTime = bulk.transferTime;
		nextBulk->type = bulk.type;
		nextBulk->devID = bulk.devID;
		nextBulk->objID = bulk.objID;
		nextBulk->port = bulk.port;
		nextBulk->ContractID = bulk.ContractID;
		nextBulk->CommandID = command->commandID;
		nextBulk->m_currentBulkState = nextState;
		nextBulk->bulkDialogID = bulk.bulkDialogID;

		//added by Cristian.Guef
		if (nextState == GBulk::BulkTransfer)
		{
			nextBulk->currentBlockSize = nextCurrentBlockSize;
			nextBulk->currentBlockCount = nextCurrentBlockCount;
			nextBulk->maxBlockSize = bulk.maxBlockSize;
			nextBulk->maxBlockCount = bulk.maxBlockCount;
			/// TODO: HERE data is actually copied here!
			/// TODO: REDESIGN for a single buffer allocated once and GBULK are actuay walking a pointer in buffer
			nextBulk->Data = bulk.Data;
			
			 LOG_DEBUG(" bulk -> transfer with maxBlockSize = " << 
		  	 			(boost::uint32_t)bulk.maxBlockSize << 
		  	 			" and nextcurrentBlockSize = " << 
		  	 			 (boost::uint32_t)nextCurrentBlockSize  << 
		  	 			 " and maxBlockCount = " << 
		  	 			 (boost::uint32_t)bulk.maxBlockCount <<
		  	 			 " nextcurrentBlockCount = " <<
			 			(boost::uint32_t)nextCurrentBlockCount);
			 			
			//[andrei.petrut] leave paranthesis around define to avoid dataSize / 10 * 1024 cases...
			int timeoutMinutes = 1 + (nextCurrentBlockSize / (processor->configApp.BulkDataTransferRate()));
			nlib::TimeSpan timeout = nlib::util::minutes(timeoutMinutes);
			LOG_DEBUG("Setting timeout for bulk command at " << nlib::ToString(timeout));
			
			//added by Cristian.Guef
			try
			{
				processor->SendRequest(*command, nextBulk, timeout);

				//lease mng
				processor->leaseTrackMng.RemoveLease(nextBulk->ContractID);
			}
			catch(std::exception ex)
			{
				if (bulk.type == GBulk::BULK_WITH_DEV)
					RemoveBulkInProgress(bulk.devID);
				throw ex;
			}
			return;
		}
		
		//added by Cristian.Guef
		try
		{
			processor->SendRequest(*command, nextBulk);

			//lease mng
			processor->leaseTrackMng.RemoveLease(nextBulk->ContractID);
		}
		catch(std::exception ex)
		{
			if (bulk.type == GBulk::BULK_WITH_DEV)
				RemoveBulkInProgress(bulk.devID);
			throw ex;
		}

		/* commented by Cristian.Guef	
		processor->commands.SetCommandResponded(*command, nlib::CurrentLocalTime(), "OK");
		*/
	}
	else	//bulk.Status != Command::rsSuccess
	{
		if(bulk.type == GBulk::BULK_WITH_DEV)
		{	/// Fail the transfer in BD
			processor->devices.SetFirmDlStatus( bulk.devID, FWTYPE_SENZOR_BOARD, DevicesManager::FirmDlStatus_Failed);
		}
		
		//added by Cristian.Guef
		if (bulk.m_currentBulkState == GBulk::BulkTransfer)
		{
			boost::shared_ptr<GBulk> nextBulk(new GBulk());
			nextBulk->transferTime = bulk.transferTime;
			nextBulk->type = bulk.type;
			nextBulk->devID = bulk.devID;
			nextBulk->objID = bulk.objID;
			nextBulk->port = bulk.port;
			nextBulk->ContractID = bulk.ContractID;
			nextBulk->CommandID = command->commandID;
			nextBulk->m_currentBulkState = GBulk::BulkEnd;
			nextBulk->bulkDialogID = bulk.bulkDialogID;
			bulk.maxBlockCount = bulk.Status; //when < 0 it means there was an error
			
			if ((bulk.type == GBulk::BULK_WITH_DEV && IsBulkInProgreass(bulk.devID)) || bulk.type != GBulk::BULK_WITH_DEV)
			{
				if (bulk.type == GBulk::BULK_WITH_DEV)
				{
					unsigned short objID = 0;
					unsigned short tsapid = 0;
					int cmdID = 0;
					GetBulkInProgressVal(bulk.devID, tsapid, objID, cmdID);
					if (cmdID != command->commandID)
						return;
				}

				LOG_ERROR(" bulk -> transfer_error, so send bulk_close...");
				try
				{
					processor->SendRequest(*command, nextBulk);

					//lease mng
					processor->leaseTrackMng.RemoveLease(nextBulk->ContractID);
				}
				catch(std::exception ex)
				{
					if (bulk.type == GBulk::BULK_WITH_DEV)
						RemoveBulkInProgress(bulk.devID);
					throw ex;
				}
			}
			return;
		}
		
		if (bulk.m_currentBulkState == GBulk::BulkOpen && bulk.Status == Command::rsFailure_GatewayUnknown)
		{
			boost::shared_ptr<GBulk> nextBulk(new GBulk());
			nextBulk->transferTime = bulk.transferTime;
			nextBulk->type = bulk.type;
			nextBulk->devID = bulk.devID;
			nextBulk->objID = bulk.objID;
			nextBulk->port = bulk.port;
			nextBulk->ContractID = bulk.ContractID;
			nextBulk->CommandID = command->commandID;
			nextBulk->m_currentBulkState = GBulk::BulkEnd;
			nextBulk->bulkDialogID = bulk.bulkDialogID; //we don't have parallel transfers
			bulk.maxBlockCount = bulk.Status; //when < 0 it means there was an error
			
			if ((bulk.type == GBulk::BULK_WITH_DEV && IsBulkInProgreass(bulk.devID)) || bulk.type != GBulk::BULK_WITH_DEV)
			{
				if (bulk.type == GBulk::BULK_WITH_DEV)
				{
					unsigned short objID = 0;
					unsigned short tsapid = 0;
					int cmdID = 0;
					GetBulkInProgressVal(bulk.devID, tsapid, objID, cmdID);
					if (cmdID != command->commandID)
						return;
				}

				
				LOG_ERROR(" bulk -> open_error, so send bulk_close...");
				try
				{
					processor->SendRequest(*command, nextBulk);

					//lease mng
					processor->leaseTrackMng.RemoveLease(nextBulk->ContractID);
				}
				catch(std::exception ex)
				{
					if (bulk.type == GBulk::BULK_WITH_DEV)
						RemoveBulkInProgress(bulk.devID);
					throw ex;
				}
			}
			return;
		}
		
		/* commented by Cristian.Guef
		processor->HandleInvalidContract(command->commandID, command->deviceID, bulk.Status);
		*/
		//added by Cristian.Guef
		if(bulk.Status == Command::rsFailure_UnknownResource){
			unsigned short TLDE_SAPID = bulk.port;
			unsigned short ObjID = bulk.objID;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			DevicePtr device = GetDevice(bulk.devID);
			//it will be recreated by the period_task
			device->ResetContractID(resourceID, (unsigned char) GContract::BulkTransforClient);					
			LOG_INFO(boost::str(boost::format(" bulk -> lease expired for DeviceID=%1%") % bulk.devID));

			if (bulk.type == GBulk::BULK_WITH_SM)
			{
				processor->commands.SetDBCmdID_as_new(command->commandID);
			}
			else
			{
				if (IsBulkInProgreass(bulk.devID))
				{
					RemoveBulkInProgress(bulk.devID);
					processor->commands.SetDBCmdID_as_new(command->commandID);
				}
			}
			
			return;
		}
		
		if ((bulk.type == GBulk::BULK_WITH_DEV && IsBulkInProgreass(bulk.devID)) || bulk.type != GBulk::BULK_WITH_DEV)
		{
			if (bulk.type == GBulk::BULK_WITH_DEV)
			{
				unsigned short objID = 0;
				unsigned short tsapid = 0;
				int cmdID = 0;
				GetBulkInProgressVal(bulk.devID, tsapid, objID, cmdID);
				if (cmdID != command->commandID)
					return;
			}

			if (bulk.type == GBulk::BULK_WITH_DEV)
				RemoveBulkInProgress(bulk.devID);
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), bulk.Status);

			if (bulk.Status == Command::rsFailure_LostConnection)
			{
				return;
			}

			//lease mng
			unsigned short TLDE_SAPID = bulk.port;
			unsigned short ObjID = bulk.objID;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;		
			DevicePtr device = GetDevice(bulk.devID);
			processor->leaseTrackMng.AddLease(bulk.ContractID, device, resourceID, GContract::BulkTransforClient, nlib::util::seconds(processor->configApp.LeasePeriod()));
		}
	}
}

void ResponseProcessor::Visit(GClientServer<ReadMultipleObjectAttributes>& multipleReadObjectAttributes)
{
	//added by Cristian.Guef
	LOG_DEBUG(" ReadMultipleObjectAttributes  with dialogID = " << (boost::uint32_t)multipleReadObjectAttributes.Client.m_unDialogueID << 
				" and status = " << (boost::int32_t)multipleReadObjectAttributes.Status <<
				" and total_attr_no = " << (boost::int32_t)multipleReadObjectAttributes.Client.m_unTotalAttributesNo << 
				" and sequence_no = " << (boost::int32_t) multipleReadObjectAttributes.Client.m_unSequenceNo);


	//added by Cristian.Guef
	static std::map<unsigned int, ForReadMultipleObjAttrsCmd>::iterator iter = DialogueIDToReadMultipleObjs.end(); 


	//added by Cristian.Guef
	if(DialogueIDToReadMultipleObjs[multipleReadObjectAttributes.Client.m_unDialogueID].MultipleReadObjAttrs.size() == 0){
		//we have a new dialog_id created
		iter = DialogueIDToReadMultipleObjs.find(multipleReadObjectAttributes.Client.m_unDialogueID);
		
		iter->second.MultipleReadObjAttrs.resize(multipleReadObjectAttributes.Client.m_unTotalAttributesNo);
		iter->second.BadFragmentCount = 0;
		iter->second.GoodFragmentCount = 0;
	}
	
	if (iter == DialogueIDToReadMultipleObjs.end()) //we have a fragmented data
	{
		iter = DialogueIDToReadMultipleObjs.find(multipleReadObjectAttributes.Client.m_unDialogueID);
		if (iter == DialogueIDToReadMultipleObjs.end())
		{
			LOG_ERROR("ReadMultipleObjectAttributes  with dialogID = " << (boost::uint32_t)multipleReadObjectAttributes.Client.m_unDialogueID << 
				" there should be one element in the map");
			return;
		}
	}


	//added by Cristian.Guef - store all fragments
	iter->second.MultipleReadObjAttrs[multipleReadObjectAttributes.Client.m_unSequenceNo] 
						= multipleReadObjectAttributes.Client.attributes[0];


	if (multipleReadObjectAttributes.Status == Command::rsSuccess)
	{

		//added by Cristian.Guef
		iter->second.GoodFragmentCount++;

		//added by Cristian.Guef
		if(multipleReadObjectAttributes.Client.m_unTotalAttributesNo == 1){
			//there is no fragmentation
			SaveSingleReadAttribute(multipleReadObjectAttributes.Client.attributes[0]);
		}
		else{
			if (((int)multipleReadObjectAttributes.Client.m_unTotalAttributesNo) == (iter->second.BadFragmentCount + iter->second.GoodFragmentCount))
				SaveMultipleReadAttribute(iter->second.MultipleReadObjAttrs);
		}
	

		/* commented by Cristian.Guef
		if (multipleReadObjectAttributes.Client.attributes.size() == 1)
		{
			SaveSingleReadAttribute(multipleReadObjectAttributes.Client.attributes[0]);
		}
		else
		{
			SaveMultipleReadAttribute(multipleReadObjectAttributes.Client.attributes);
		}
		*/
	}
	else
	{

		//added by Cristian.Guef
		iter->second.BadFragmentCount--;
	
		/* commented by Cristian.Guef
		processor->CommandFailed(*command, nlib::CurrentLocalTime(), multipleReadObjectAttributes.Status);
		*/
		
		/* commented by Cristian.Guef
		//processor->HandleInvalidContract(command->commandID, command->deviceID, multipleReadObjectAttributes.Status);
		*/

		//added by Cristian.Guef
		if(multipleReadObjectAttributes.Status == Command::rsFailure_GatewayContractExpired){

			unsigned short TLDE_SAPID = multipleReadObjectAttributes.Client.attributes[0].channel.mappedTSAPID;
			unsigned short ObjID = multipleReadObjectAttributes.Client.attributes[0].channel.mappedObjectID;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;

			DevicePtr device = GetDevice(command->deviceID);
			device->ResetContractID(resourceID, (unsigned char) GContract::Client);	
			//set command_status as new
			processor->commands.SetDBCmdID_as_new(command->commandID);
			LOG_INFO(boost::str(boost::format(" lease expired for DeviceID=%1%") % command->deviceID));
		}
		else
		{
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), multipleReadObjectAttributes.Status);

			//lease mng
			if (multipleReadObjectAttributes.Status == Command::rsFailure_LostConnection)
				return;
			unsigned short TLDE_SAPID = multipleReadObjectAttributes.Client.attributes[0].channel.mappedTSAPID;
			unsigned short ObjID = multipleReadObjectAttributes.Client.attributes[0].channel.mappedObjectID;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			DevicePtr device = GetDevice(command->deviceID);
			processor->leaseTrackMng.AddLease(multipleReadObjectAttributes.ContractID, device, resourceID, GContract::Client, nlib::util::seconds(processor->configApp.LeasePeriod()));
		}
	}

	//added by Cristian.Guef
	//delete all fragments when it's time
	if(multipleReadObjectAttributes.Client.m_unTotalAttributesNo == iter->second.MultipleReadObjAttrs.size()){
		//we have all fragments ---
		//erase all fragments
		iter->second.MultipleReadObjAttrs.resize(0);
		//delete dialog from map
		DialogueIDToReadMultipleObjs.erase(iter);
	}
}

void ResponseProcessor::SaveSingleReadAttribute(ReadMultipleObjectAttributes::ObjectAttribute& singleRead)
{
	if (singleRead.confirmStatus == Command::rsSuccess)
	{
		DeviceReading reading;
		reading.deviceID = command->deviceID;
		//reading.channel = singleRead.channel;
		reading.channelNo = singleRead.channel.channelNumber;
		/* commented by Cristian.Guef
		reading.readingTime = nlib::CurrentLocalTime();
		*/
		reading.rawValue = singleRead.confirmValue;
		reading.readingType = DeviceReading::ClientServer;

		//added by Cristian.Guef
		//reading.readingTime = singleRead.ReadingTime;
		//reading.milisec = singleRead.milisec;
		reading.tv = singleRead.tv;

		reading.IsISA = singleRead.IsISA;
		if (reading.IsISA)
			reading.ValueStatus = singleRead.ValueStatus;

		processor->devices.SaveReading(reading);

		/* commented by Cristian.Guef -just value, no raw value
		std::string readingString = boost::str(boost::format("RawValue=%1%, Value=%2%") % reading.rawValue.ToString()
		    % reading.value);
		    */
		//added by cristian.Guef
		std::string readingString = boost::str(boost::format("Value=%1%") % reading.rawValue.ToString());
		
		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), readingString);
	}
	else
	{
		processor->CommandFailed(*command, nlib::CurrentUniversalTime(), (Command::ResponseStatus) singleRead.confirmStatus);
	}

	//lease mng
	if (singleRead.confirmStatus == Command::rsFailure_LostConnection)
		return;
	unsigned short TLDE_SAPID = singleRead.channel.mappedTSAPID;
	unsigned short ObjID = singleRead.channel.mappedObjectID;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;	
	DevicePtr device = GetDevice(command->deviceID);
	processor->leaseTrackMng.AddLease(device->ContractID(resourceID, GContract::Client), device, resourceID, GContract::Client, nlib::util::seconds(processor->configApp.LeasePeriod()));
}

void ResponseProcessor::SaveMultipleReadAttribute(ReadMultipleObjectAttributes::AttributesList& multipleRead)
{
	DeviceReadingsList readings;
	/*commented by Cristian.Guef
	readings.reserve(multipleRead.size());
	*/

	std::stringstream multipleReadingsString;//build formated string with all readings
	for (ReadMultipleObjectAttributes::AttributesList::const_iterator it = multipleRead.begin(); it != multipleRead.end(); it++)
	{
		if (it->confirmStatus == Command::rsSuccess)
		{
			DeviceReading reading;
			reading.deviceID = command->deviceID;
			//reading.channel = it->channel;
			reading.channelNo = it->channel.channelNumber;
			//reading.readingTime = nlib::CurrentLocalTime();
			reading.tv = it->tv;
			reading.rawValue = it->confirmValue;
			reading.readingType = DeviceReading::ClientServer;

			//added by Cristian.Guef
			reading.IsISA = it->IsISA;
			if (reading.IsISA)
				reading.ValueStatus = it->ValueStatus;

			readings.push_back(reading);
		}
		else
		{
			multipleReadingsString << boost::str(boost::format("[ChannelNo=%1% failed %2% ] ") % it->channel.channelNumber
			    % FormatResponseCode(it->confirmStatus));
		}
	}

	processor->devices.SaveReadings(readings);

	for (DeviceReadingsList::const_iterator it = readings.begin(); it != readings.end(); it++)
	{
		/*
		multipleReadingsString << boost::str(boost::format("[ChannelNo=%1%, RawValue=%2%] ")
		    % it->channel.channelNumber % it->rawValue.ToString());
			*/
		//lease mng
		//unsigned short TLDE_SAPID = multipleReadObjectAttributes.Client.attributes[0].channel.mappedTSAPID;
		//unsigned short ObjID = multipleReadObjectAttributes.Client.attributes[0].channel.mappedObjectID;
		//unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;	
		//DevicePtr device = GetDevice(command->deviceID);
		//processor->leaseTrackMng.AddLease(bulk.ContractID, device, resourceID, GContract::BulkTransforClient, nlib::util::seconds(processor->configApp.LeasePeriod()));
	}
	processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), multipleReadingsString.str());
}

void ResponseProcessor::Visit(GClientServer<WriteObjectAttribute>& writeAttribute)
{
	if (writeAttribute.Status == Command::rsSuccess)
	{
		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), "success");
	}
	else
	{
		/* commented by Cristian.Guef
		processor->CommandFailed(*command, nlib::CurrentLocalTime(), writeAttribute.Status);
		*/

		/* commented by Cristian.Guef
		processor->HandleInvalidContract(command->commandID, command->deviceID, writeAttribute.Status);
		*/
		//added by Cristian.Guef
		if(writeAttribute.Status == Command::rsFailure_GatewayContractExpired){
			unsigned short TLDE_SAPID = writeAttribute.Client.channel.mappedTSAPID;
			unsigned short ObjID = writeAttribute.Client.channel.mappedObjectID;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetDevice(command->deviceID);

			device->ResetContractID(resourceID, (unsigned char) GContract::Client);	
			//set command_status as new
			processor->commands.SetDBCmdID_as_new(command->commandID);
			LOG_INFO(boost::str(boost::format(" lease expired for DeviceID=%1%") % command->deviceID));
		}
		else
		{
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), writeAttribute.Status);
		}
	}
}

void ResponseProcessor::Visit(PublishSubscribe& publishSubscribe)
{
}

void ResponseProcessor::Visit(GClientServer<Publish>& publish)
{

	publish.Client.ps->PublisherStatus = publish.Status;
	publish.Client.ps->CommandID = publish.CommandID;

	if (publish.Client.ps->PublisherStatus != Command::rsNoStatus)
	{
		if (publish.Client.ps->PublisherStatus == Command::rsSuccess)
		{
			LOG_DEBUG("Also the publisher responded with success!");
			DevicePtr publisherDevice = GetDevice(publish.Client.ps->PublisherDeviceID);
			DevicePtr subscriberDevice = GetDevice(publish.Client.ps->SubscriberDeviceID);
			
			/* commented by Cristian.Guef
			if (!publisherDevice->HasContract()) //this is hard to happen
			*/
			
			
		}
		else
		{
			LOG_DEBUG("Publisher has failed!");
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), publish.Client.ps->PublisherStatus,
			    publish.Client.ps->SubscriberStatus);

			/* commented by Cristian.Guef
			processor->HandleInvalidContract(command->commandID, publish.Client.ps->PublisherDeviceID,
			    publish.Client.ps->PublisherStatus);
			*/

			//HACK:[Ovidiu-Rauca] find another solution to avoid
			//conflict between pub-sub automatic(by devicese links manger) and manaual by user
			if (command->generatedType == Command::cgtManual)
			{
				if ((publish.Client.ps->PublisherStatus == Command::rsFailure_GatewayTimeout
				    || publish.Client.ps->PublisherStatus == Command::rsSuccess) || (publish.Client.ps->SubscriberStatus
				    == Command::rsFailure_GatewayTimeout || publish.Client.ps->SubscriberStatus == Command::rsSuccess))
				{
					CreateCancelCommand(publish.Client.ps->PublisherDeviceID, publish.Client.ps->SubscriberDeviceID,
					    publish.Client.ps->PublisherChannel.channelNumber, publish.Client.ps->CommandID,
					    publish.Client.ps->isLocalLoop);
				}
			}
		}
	}
}

void ResponseProcessor::Visit(GClientServer<Subscribe>& subscribe)
{

	//added by Cristian.Guef
	LOG_INFO(" subscribe  with current_state = " << (boost::uint32_t)subscribe.Client.m_currentSubscribeState << 
				" and status = " << (boost::int32_t)subscribe.Status <<
				" and handed publisherIP = " << subscribe.Client.m_handedPublisherIP.ToString() <<
				" with publisher_device_id = " << (boost::int32_t)subscribe.Client.ps->PublisherDeviceID <<
				" and  concetrator ObjID = " << (boost::int32_t) subscribe.Client.ps->PublisherChannel.mappedObjectID <<
				" and  concetrator TLSAP_ID = " << (boost::int32_t) subscribe.Client.ps->PublisherChannel.mappedTSAPID);


	subscribe.Client.ps->SubscriberStatus = subscribe.Status;
	subscribe.Client.ps->CommandID = subscribe.CommandID;
	
	//added by cristian.Guef
	DevicePtr publisherDevice;

	if (subscribe.Client.ps->SubscriberStatus != Command::rsNoStatus)
	{
		if (subscribe.Client.ps->SubscriberStatus == Command::rsSuccess)
		{
			/* commented by Cristian.Guef
			LOG_DEBUG("Subscriber responded with success! So...send publish command")
			*/

						
			//added by Cristian.Guef
			switch (subscribe.Client.m_currentSubscribeState)
			{
			case Subscribe::SubscribeReadPubEndpoint:
				{
				LOG_ERROR("subscribe.Client.m_currentSubscribeState = SubscribeReadPubEndpoint.   Shouldn't get here.");
				return;
				}
			case Subscribe::SubscribeReadPubObjAttrSizeArray:
				{
				LOG_ERROR("subscribe.Client.m_currentSubscribeState = SubscribeReadPubObjAttrSizeArray.  Shouldn't get here.");
				return;
				}

			case Subscribe::SubscribeAddPubObjIDAttrID:
				{
				LOG_ERROR("subscribe.Client.m_currentSubscribeState = SubscribeAddPubObjIDAttrID.   Shouldn't get here.");
				return;
				}
	
			case Subscribe::SubscribeIncrementNumItemsSubscribing:
				{
				LOG_ERROR("subscribe.Client.m_currentSubscribeState = SubscribeIncrementNumItemsSubscribing.  Shouldn't get here.");
				return;
				}

			case Subscribe::SubscribeWriteNumItemsSubscribing_with_one:
				{
				LOG_ERROR("subscribe.Client.m_currentSubscribeState = SubscribeWriteNumItemsSubscribing_with_one. Shouldn't get here.");
				return;
				}

			case Subscribe::SubscribeWriteSubEndpoint:
				{
				LOG_ERROR("subscribe.Client.m_currentSubscribeState = SubscribeWriteSubEndpoint.  Shouldn't get here.");	
				return;
				}

			case Subscribe::SubscribeDoLeaseSubscriber:
				{
				publisherDevice = GetDevice(subscribe.Client.ps->PublisherDeviceID);
				
				if(subscribe.Client.m_ObtainedLeaseID_S  == 0)
				 {
					publisherDevice->WaitForContract(
						(subscribe.Client.ps->PublisherChannel.mappedObjectID << 16) 
						| subscribe.Client.ps->PublisherChannel.mappedTSAPID,
						GContract::Subscriber, false);
					publisherDevice->issuePublishSubscribeCmd = true;
					processor->CommandFailed(*command, nlib::CurrentUniversalTime(), Command::rsFailure_InvalidReturnedLeaseID);
					return;
				 }

				if(subscribe.Client.m_ObtainedLeasePeriod_S != 0)
				 {
					publisherDevice->WaitForContract(
						(subscribe.Client.ps->PublisherChannel.mappedObjectID << 16) 
						| subscribe.Client.ps->PublisherChannel.mappedTSAPID,
						GContract::Subscriber, false);
					publisherDevice->issuePublishSubscribeCmd = true;
					processor->CommandFailed(*command, nlib::CurrentUniversalTime(), Command::rsFailure_InvalidReturnedLeasePeriod);
					return;
				 }
				
				unsigned char leaseTypeReplaced = 10;
				processor->devices.SetDeviceContract(publisherDevice, subscribe.Client.m_ObtainedLeaseID_S, 
						(subscribe.Client.ps->PublisherChannel.mappedObjectID << 16) 
						| subscribe.Client.ps->PublisherChannel.mappedTSAPID, 
															GContract::Subscriber, 0, leaseTypeReplaced);
			
				publisherDevice->issuePublishSubscribeCmd = true;
				

				LOG_INFO("Subscriber has succeded! so any indication will be available from device with mac = "
				<< publisherDevice->Mac().ToString());

				//we assume all works fine
				try
				{
										
					/* modified by Cristian.Guef
					boost::uint32_t trackingID = (boost::uint32_t) 0x01000000 | (boost::uint32_t) publisherDevice->ContractID();
					*/
					//added by Cristian.Guef
					unsigned short TLDE_SAPID = subscribe.Client.ps->PublisherChannel.mappedTSAPID;
					unsigned short ObjID = subscribe.Client.ps->PublisherChannel.mappedObjectID;
					unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
					boost::uint32_t trackingID = (boost::uint32_t) publisherDevice->ContractID(resourceID, GContract::Subscriber);

					/* commented by Cristian.Guef
					PublishIndicationPtr indication(new PublishIndication(publish.Client.ps->PublisherDeviceID));
					*/

					//added by Cristian.Guef
					PublishIndicationPtr indication(
						new PublishIndication(subscribe.Client.ps->PublisherDeviceID, 
											subscribe.Client.ps->PublisherChannel.mappedTSAPID,
											//5, //de test
											subscribe.Client.ps->PublisherChannel.mappedObjectID,
											subscribe.Client.ps->SubscriberLowThreshold,
											subscribe.Client.ps->SubscriberHighThreshold));
						
					processor->RegisterPublishIndication(subscribe.Client.ps->PublisherDeviceID, subscribe.Client.ps->PublisherChannel.mappedAttributeID, trackingID,
							indication);
					
					//save channels in db 
					//processor->devices.SavePublishChannels(subscribe.Client.ps->PublisherDeviceID, 
													//*subscribe.Client.ps->pcoChannelList);
					processor->devices.ProcessDBChannels(subscribe.Client.ps->PublisherDeviceID, 
									*subscribe.Client.ps->pcoChannelList, *subscribe.Client.ps->pcoChannelIndex);


					//save needed publish info
					InitPublishEnviroment(*subscribe.Client.ps->pcoChannelList, subscribe.Client.ps->dataContentVer, publisherDevice);
					PublishingManager.SaveInterfaceType(publisherDevice->GetPublishHandle(), subscribe.Client.ps->interfaceType);
					indication->m_publishHandle = publisherDevice->GetPublishHandle();
					
					command->deviceID = publisherDevice->id;
					
					processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), 
						boost::str(boost::format("created_LeaseID=%1d, objID=%2d, tlsap_id=%3d and lease_type=%4d")
						% (boost::uint32_t) subscribe.Client.m_ObtainedLeaseID_S 
						% (boost::uint32_t)ObjID
						% (boost::uint32_t)TLDE_SAPID
						% (boost::uint32_t)GContract::Subscriber));
				}
				catch (std::exception& ex)
				{
					LOG_ERROR("PublishSubscribeCompleted: Unable to register publish-subscribe indication. error=" << ex.what());
					processor->CommandFailed(*command, nlib::CurrentUniversalTime(), Command::rsFailure_InternalError);
				}
				return;
				}

			}
			

			/* commented by Cristian.Guef
			// now we can also send the command to the publisher
			boost::shared_ptr<GClientServer<Publish> > publish(new GClientServer<Publish>());
			publish->Client.ps = subscribe.Client.ps;
			publish->CommandID = subscribe.Client.ps->CommandID;

			DevicePtr publisherDevice = GetDevice(subscribe.Client.ps->PublisherDeviceID);
			
			publish->ContractID = publisherDevice->ContractID();
		
			processor->SendRequest(*command, publish);
			*/

		}
		else
		{

			/* commented by Cristian.Guef 
			LOG_DEBUG("Subscriber has failed! No need to send the publish command!");
			*/
			
			//added by Cristia.Guef
			publisherDevice = GetDevice(subscribe.Client.ps->PublisherDeviceID);
			publisherDevice->issuePublishSubscribeCmd = true;
			LOG_ERROR("Subscriber has failed! so no indication will be available from device with mac = " << publisherDevice->Mac().ToString());

			// commented by Cristian.Guef
			/*all command marked as failed*/
			/*processor->CommandFailed(*command, nlib::CurrentLocalTime(), Command::rsFailure_CommandNotSent,
			    subscribe.Client.ps->SubscriberStatus);*/

			//added by Cristian.Guef
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), subscribe.Client.ps->SubscriberStatus);

			/* commented by Cristian.Guef
			processor->HandleInvalidContract(command->commandID, subscribe.Client.ps->SubscriberDeviceID,
			    subscribe.Client.ps->SubscriberStatus);*/

			//added by Cristian.Guef
			unsigned short TLDE_SAPID = subscribe.Client.ps->PublisherChannel.mappedTSAPID;
			unsigned short ObjID = subscribe.Client.ps->PublisherChannel.mappedObjectID;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			publisherDevice->WaitForContract(resourceID, GContract::Subscriber, false);

			//added by Cristian.Guef
			if (subscribe.Client.ps->SubscriberStatus == Command::rsFailure_InvalidDevice)
			{
				publisherDevice->Status(Device::dsUnregistered);
			}
		}
	}
}

//added by Cristian.Guef -> this function isn't used anymore
void ResponseProcessor::PublishSubscribeCompleted(PublishSubscribe& ps)
{
	LOG_INFO("PublishSubscribeCompleted: begin Command=" << ps.ToString());

	DevicePtr publisherDevice = GetDevice(ps.PublisherDeviceID);
	
	
	if (ps.PublisherStatus == Command::rsSuccess && ps.SubscriberStatus == Command::rsSuccess)
	{
		try
		{
			processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), "success");
		}
		catch (std::exception& ex)
		{
			LOG_ERROR("PublishSubscribeCompleted: Unable to register publish-subscribe indication. error=" << ex.what());
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), Command::rsFailure_InternalError);
		}

	}
	else // failed flow, so start the cancel

	{
		processor->CommandFailed(*command, nlib::CurrentUniversalTime(), ps.PublisherStatus, ps.SubscriberStatus);
		
		/* commented by Cristian.Guef
		processor->HandleInvalidContract(command->commandID, ps.PublisherDeviceID, ps.PublisherStatus);
		processor->HandleInvalidContract(command->commandID, ps.SubscriberDeviceID, ps.SubscriberStatus);
		*/
	

		//HACK:[Ovidiu-Rauca] find another solution to avoid
		//conflict between pub-sub automatic(by devicese links manger) and manaual by user
		if (command->generatedType == Command::cgtManual)
		{
			if ((ps.PublisherStatus == Command::rsFailure_GatewayTimeout || ps.PublisherStatus == Command::rsSuccess)
			    || (ps.SubscriberStatus == Command::rsFailure_GatewayTimeout || ps.SubscriberStatus == Command::rsSuccess))
			{
				CreateCancelCommand(ps.PublisherDeviceID, ps.SubscriberDeviceID, ps.PublisherChannel.channelNumber,
				    ps.CommandID, ps.isLocalLoop);
			}
		}
	}
}


void ResponseProcessor::FlushDeviceReading(int pubHandle, int devID, std::basic_string<unsigned char> &DataBuff, struct timeval &tv, bool isFirstTime, bool &isDataSaved)
{
	DeviceReading reading;
	reading.deviceID = devID;
	//reading.readingTime = ReadingTime;
	//reading.milisec = milisec;
	reading.tv = tv;
	reading.readingType = DeviceReading::PublishSubcribe;

	if (PublishingManager.GetParsingInfo(pubHandle) == NULL)
	{
		LOG_WARN("FlushDeviceReading -> GetParsingInfo() returned with failure");
		return;
	}
	if (PublishingManager.GetInterpretInfo(pubHandle) == NULL)
	{
		LOG_WARN("FlushDeviceReading -> GetInterpretInfo() returned with failure");
		return;
	}

	std::vector<Subscribe::ObjAttrSize> &parse = *PublishingManager.GetParsingInfo(pubHandle);
	PublishedDataMng::InterpretInfoListT &interpret = *PublishingManager.GetInterpretInfo(pubHandle);
	int dataBuffOffset = 0;

	isDataSaved = false;

	for (unsigned int index = 0; index < parse.size(); index++)
	{
		reading.rawValue.dataType = (ChannelValue::DataType)interpret[index].dataFormat;
		reading.channelNo = interpret[index].channelDBNo;

		if (parse[index].withStatus == 0)
		{
			reading.IsISA = false;

			switch (parse[index].Size)
			{
			case 1/*one byte*/:
				if (DataBuff.size() - dataBuffOffset >= 1/*one byte*/) 
				{
					
					if (reading.rawValue.dataType == ChannelValue::cdtUInt8)
					{
						reading.rawValue.value.uint8 = DataBuff[dataBuffOffset++];
						//LOG_DEBUG("Publish_data --> uint8 = " << (boost::uint16_t)reading.rawValue.value.uint8);
					}
					else if (reading.rawValue.dataType == ChannelValue::cdtInt8)
					{
						reading.rawValue.value.int8 = DataBuff[dataBuffOffset++];
						//LOG_DEBUG("Publish_data -->  int8 = " << (boost::int16_t)reading.rawValue.value.int8);
					}
					else
					{
						LOG_ERROR("Publish -> invalid publish data format in db: expected uint8 or int8 format");
						return; 
					}
				}
				else
				{
					LOG_ERROR("Publish -> invalid publish data size: expected 1 byte long. Idx:" << index << ", offset:" << dataBuffOffset); //we can't save it into the db because we don't have a commanddb attached to this
					return; 
				}
				break;

			case 2/*two bytes*/:
				if (DataBuff.size() - dataBuffOffset >= 2/*two bytes*/)
				{
					//unsigned short val = 0; 
					//memcpy(&val, DataBuff.c_str() + dataBuffOffset, 2/*four bytes*/);

					if (reading.rawValue.dataType == ChannelValue::cdtUInt16)
					{
						reading.rawValue.value.uint16 = htons(*((short*)(DataBuff.c_str() + dataBuffOffset)));
						//LOG_DEBUG("Publish_data --> uint16 = " << (boost::uint16_t)reading.rawValue.value.uint16);
					}
					else if (reading.rawValue.dataType == ChannelValue::cdtInt16)
					{
						reading.rawValue.value.int16 = htons(*((short*)(DataBuff.c_str() + dataBuffOffset)));
						//LOG_DEBUG("Publish_data --> int16 = " << (boost::int16_t)reading.rawValue.value.int16);
					}
					else
					{
						LOG_ERROR("Publish -> invalid publish data format in db: expected uint16 or int16 format. Idx:" << index << ", offset:" << dataBuffOffset);
						return; 
					}
					dataBuffOffset+= 2;
				}
				else
				{
					LOG_ERROR("Publish -> invalid publish data size: expected 2 bytes long. Idx:" << index << ", offset:" << dataBuffOffset); //we can't save it into the db because we don't have a commanddb attached to this
					return; 
				}
				break;

			case 4/*four bytes*/:
				if (DataBuff.size()-dataBuffOffset >= 4/*four bytes*/)
				{
					//unsigned int val = 0; 
					//memcpy(&val, DataBuff.c_str() + dataBuffOffset, 4/*four bytes*/);

					if (reading.rawValue.dataType == ChannelValue::cdtFloat32)
					{	uint32_t* p = (uint32_t*)&(reading.rawValue.value.float32);
						*p = htonl(*((int*)(DataBuff.c_str() + dataBuffOffset)));
						//LOG_DEBUG("Publish_data --> float32 = " << boost::str(boost::format("%.4f") % reading.rawValue.value.float32));
					}
					else if (reading.rawValue.dataType == ChannelValue::cdtUInt32)
					{
						reading.rawValue.value.uint32 = htonl(*((int*)(DataBuff.c_str() + dataBuffOffset)));
						//LOG_DEBUG("Publish_data --> uint32 = " << (boost::uint32_t)reading.rawValue.value.uint32);

					}
					else if(reading.rawValue.dataType == ChannelValue::cdtInt32)
					{
						reading.rawValue.value.int32 = htonl(*((int*)(DataBuff.c_str() + dataBuffOffset)));
						LOG_DEBUG("Publish_data --> int32 = " << (boost::int32_t)reading.rawValue.value.int32);
					}
					else
					{
						LOG_ERROR("Publish -> invalid publish data format in db: expected uint32, int32 or float32 format. Idx:" << index << ", offset:" << dataBuffOffset);
						return; 
					}
					dataBuffOffset+=4;
				}
				else
				{
					LOG_ERROR("Publish -> invalid publish data size: expected 4 bytes long. Idx:" << index << ", offset:" << dataBuffOffset); //we can't save it into the db because we don't have a commanddb attached to this
					return; 
				}
				break;
			default:
				LOG_ERROR("Publish -> invalid publish data size= " << parse[index].Size << ". Idx:" << index << ", offset:" << dataBuffOffset); //we can't save it into the db because we don't have a commanddb attached to this
				return; 
			}
		}
		else //ISA standard
		{
			reading.IsISA = true;

			switch (parse[index].Size + 1)
			{
			case 2/*one byte status + one byte data*/:
				if (DataBuff.size()-dataBuffOffset >= 2/*one byte status + one byte data*/) 
				{
					//status
					reading.ValueStatus = DataBuff[dataBuffOffset++];
					//LOG_DEBUG("Publish_data --> status = " << (boost::uint16_t)reading.ValueStatus);
					
					//data
					if (reading.rawValue.dataType == ChannelValue::cdtUInt8)
					{
						reading.rawValue.value.uint8 = DataBuff[dataBuffOffset++];
						//LOG_DEBUG("Publish_data --> uint8 = " << (boost::uint16_t)reading.rawValue.value.uint8);
					}
					else if (reading.rawValue.dataType == ChannelValue::cdtInt8)
					{
						reading.rawValue.value.int8 = DataBuff[dataBuffOffset++];
						//LOG_DEBUG("Publish_data -->  int8 = " << (boost::int16_t)reading.rawValue.value.int8);
					}
					else
					{
						LOG_ERROR("Publish -> invalid publish data format in db: expected uint8 or int8 format. Idx:" << index << ", offset:" << dataBuffOffset);
						return; 
					}
				}
				else
				{
					LOG_ERROR("Publish -> invalid publish data size: expected 1 byte long. Idx:" << index << ", offset:" << dataBuffOffset); //we can't save it into the db because we don't have a commanddb attached to this
					return; 
				}
				break;

			case 3/*one byte status + two bytes data*/:
				if (DataBuff.size()-dataBuffOffset >= 3/*one byte status + two bytes data*/)
				{
					//status
					reading.ValueStatus = DataBuff[dataBuffOffset++];
					//LOG_DEBUG("Publish_data --> status = " << (boost::uint16_t)reading.ValueStatus);
					
					//data
					//unsigned short val = 0; 
					//memcpy(&val, DataBuff.c_str()+dataBuffOffset + 1/*one byte status*/, 2/*four bytes*/);

					if (reading.rawValue.dataType == ChannelValue::cdtUInt16)
					{
						reading.rawValue.value.uint16 = htons(*((short*)(DataBuff.c_str() + dataBuffOffset)));
						//LOG_DEBUG("Publish_data --> uint16 = " << (boost::uint16_t)reading.rawValue.value.uint16);
					}
					else if (reading.rawValue.dataType == ChannelValue::cdtInt16)
					{
						reading.rawValue.value.int16 = htons(*((short*)(DataBuff.c_str() + dataBuffOffset)));
						//LOG_DEBUG("Publish_data --> int16 = " << (boost::int16_t)reading.rawValue.value.int16);
					}
					else
					{
						LOG_ERROR("Publish -> invalid publish data format in db: expected uint16 or int16 format. Idx:" << index << ", offset:" << dataBuffOffset);
						return; 
					}
					dataBuffOffset+=2;
				}
				else
				{
					LOG_ERROR("Publish -> invalid publish data size: expected 2 bytes long. Idx:" << index << ", offset:" << dataBuffOffset); //we can't save it into the db because we don't have a commanddb attached to this
					return; 
				}
				break;

			case 5/*one byte status + four bytes data*/:
				if (DataBuff.size()-dataBuffOffset >= 5/*one byte status + four bytes data*/)
				{
					//status
					reading.ValueStatus = DataBuff[dataBuffOffset++];
					//LOG_DEBUG("Publish_data --> status = " << (boost::uint16_t)reading.ValueStatus);
					
					//data
					//unsigned int val = 0; 
					//memcpy(&val, DataBuff.c_str()+dataBuffOffset + 1/*one byte status*/, 4/*four bytes*/);

					if (reading.rawValue.dataType == ChannelValue::cdtFloat32)
					{	uint32_t* p = (uint32_t*)&(reading.rawValue.value.float32);
						*p = htonl(*((int*)(DataBuff.c_str() + dataBuffOffset)));
						//LOG_DEBUG("Publish_data --> float32 = " << boost::str(boost::format("%.4f") % reading.rawValue.value.float32));
					}
					else if (reading.rawValue.dataType == ChannelValue::cdtUInt32)
					{
						reading.rawValue.value.uint32 = htonl(*((int*)(DataBuff.c_str() + dataBuffOffset)));
						//LOG_DEBUG("Publish_data --> uint32 = " << (boost::uint32_t)reading.rawValue.value.uint32);

					}
					else if(reading.rawValue.dataType == ChannelValue::cdtInt32)
					{
						reading.rawValue.value.int32 = htonl(*((int*)(DataBuff.c_str() + dataBuffOffset)));
						//LOG_DEBUG("Publish_data --> int32 = " << (boost::int32_t)reading.rawValue.value.int32);
					}
					else
					{
						LOG_ERROR("Publish -> invalid publish data format in db: expected uint32, int32 or float32 format. Idx:" << index << ", offset:" << dataBuffOffset);
						return; 
					}
					dataBuffOffset+=4;
				}
				else
				{
					LOG_ERROR("Publish -> invalid publish data size: expected 4 bytes long. Idx:" << index << ", offset:" << dataBuffOffset); //we can't save it into the db because we don't have a commanddb attached to this
					return; 
				}
				break;
			default:
				LOG_ERROR("Publish -> invalid publish data size= " << (parse[index].Size + 1) << ". Idx:" << index << ", offset:" << dataBuffOffset); //we can't save it into the db because we don't have a commanddb attached to this
				return; 
			}
		}

		
		PublishingManager.AddReading(pubHandle, reading);
		if (PublishingManager.GetAllReadingsNo() > processor->configApp.ReadingsMaxEntriesSaver())
			processor->devices.SaveReadings(&PublishingManager);

		/* no need
		if (!isFirstTime)
		{
			PublishingManager.AddReading(pubHandle, reading);
		}
		else
		{
			//processor->devices.DeleteReading(reading.channelNo);
			if (!isDataSaved)
			{
				processor->devices.DeleteReadings(reading.deviceID);
				processor->devices.CreateEmptyReadings(reading.deviceID);
			}
			isDataSaved = true;
			//if (!processor->devices.IsDeviceChannelInReading(reading.deviceID, reading.channelNo))
			//	processor->devices.CreateReading(reading);
			//PublishingManager.SetFreshDataStatus(pubHandle, true);
			PublishingManager.AddReading(pubHandle, reading);
		}
		*/
	}
}

//added by Cristian.Guef
void ResponseProcessor::Visit(GClientServer<GetSizeForPubIndication>& getSizeForPubIndication)
{

	if (getSizeForPubIndication.Status != Command::rsSuccess)
	{
		if (PublishingManager.IsContentVersion((PublishedDataMng::PublishHandle)getSizeForPubIndication.Client.m_publishHandle) == false)
		{
			LOG_WARN("get_size_cmd with status = " << (boost::int32_t) getSizeForPubIndication.Status <<
					"for CO on deviceID = " << (boost::uint32_t)getSizeForPubIndication.Client.m_PublisherDeviceID <<
					"there is no version no. stored, so do nothing...");
			return;
		}

		//check version
		if (getSizeForPubIndication.Client.m_ContentVersion != 
			PublishingManager.GetContentVersion((PublishedDataMng::PublishHandle)getSizeForPubIndication.Client.m_publishHandle))
		{
			LOG_ERROR("get_size_cmd with status = " << (boost::int32_t) getSizeForPubIndication.Status << "for CO on deviceID = " 
					<<  (boost::uint32_t)getSizeForPubIndication.Client.m_PublisherDeviceID
					<< "-> there is other version no. stored, so do nothing...");
			return;	
		}
		PublishingManager.SetParsingInfoErrorFlag((PublishedDataMng::PublishHandle)getSizeForPubIndication.Client.m_publishHandle, true);	

		if (getSizeForPubIndication.Status == Command::rsFailure_GatewayContractExpired)
		{
			
			LOG_ERROR("get_size_cmd with status = " << (boost::int32_t) getSizeForPubIndication.Status <<
				"for CO on deviceID = " << (boost::uint32_t)getSizeForPubIndication.Client.m_PublisherDeviceID <<
				", so recreate c/s lease...");
			
			unsigned short TLDE_SAPID = getSizeForPubIndication.Client.m_TLSAP_ID;
			unsigned short ObjID = getSizeForPubIndication.Client.m_Concentrator_id;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetDevice(getSizeForPubIndication.Client.m_PublisherDeviceID);
			
			device->ResetContractID(resourceID, (unsigned char) GContract::Client);
			processor->HandleInvalidContract(resourceID, (unsigned char) GContract::Client, -1,
								device->id, getSizeForPubIndication.Status);					
			return;
		}
		
		LOG_ERROR("get_size_cmd with status = " << (boost::int32_t) getSizeForPubIndication.Status <<
			"for CO on deviceID = " << (boost::uint32_t)getSizeForPubIndication.Client.m_PublisherDeviceID);

		return;
	}

	LOG_INFO("get_size_cmd received successfully with content version =" << (boost::uint32_t)getSizeForPubIndication.Client.m_ContentVersion <<
		"for deviceID = " << (boost::uint32_t)getSizeForPubIndication.Client.m_PublisherDeviceID);

	
	if (PublishingManager.IsContentVersion((PublishedDataMng::PublishHandle)getSizeForPubIndication.Client.m_publishHandle) == false)
	{
		LOG_WARN("get_size_cmd with status = " << (boost::int32_t) getSizeForPubIndication.Status <<
				"for CO on deviceID = " << (boost::uint32_t)getSizeForPubIndication.Client.m_PublisherDeviceID <<
				"there is no version no. stored, so do nothing...");
		return;
	}

	if (getSizeForPubIndication.Client.m_ContentVersion != 
		PublishingManager.GetContentVersion((PublishedDataMng::PublishHandle)getSizeForPubIndication.Client.m_publishHandle))
	{

		LOG_WARN("get_size_cmd with diffrent content version =" <<
			(boost::uint32_t)getSizeForPubIndication.Client.m_ContentVersion <<
			"for deviceID = " <<
			(boost::uint32_t)getSizeForPubIndication.Client.m_PublisherDeviceID <<
			", so do not parse data cause data with other content version");
		return;
	}

	LOG_INFO("get_size_cmd with content version =" << (boost::uint32_t)getSizeForPubIndication.Client.m_ContentVersion <<
		"for deviceID = " << (boost::uint32_t)getSizeForPubIndication.Client.m_PublisherDeviceID << ", do parse data...");
	
	
	PublishingManager.SaveParsingInfo((PublishedDataMng::PublishHandle)getSizeForPubIndication.Client.m_publishHandle, 
								getSizeForPubIndication.Client.m_vecReadObjAttrSize);

	std::queue<PublishedDataMng::PublishedData> &dataList = *PublishingManager.GetPublishedData((PublishedDataMng::PublishHandle)getSizeForPubIndication.Client.m_publishHandle);
	for (; dataList.size() != 0;)
	{

		/* old_way
		PublishedDataMng::PublishedData data = dataList.front();
		dataList.pop();
		
		LOG_DEBUG("Publish -> DeviceID = " << getSizeForPubIndication.Client.m_PublisherDeviceID << " timestamp: " << nlib::ToString(data.ReadingTime) << ", " << data.milisec);
		std::vector<Subscribe::ObjAttrSize> &parse = *PublishingManager.GetParsingInfo((PublishedDataMng::PublishHandle)getSizeForPubIndication.Client.m_publishHandle);
		PublishedDataMng::InterpretInfoListT &interpret = *PublishingManager.GetInterpretInfo(getSizeForPubIndication.Client.m_publishHandle);
		for (int index = 0; index < parse.size(); index++)
			FlushDeviceReading(getSizeForPubIndication.Client.m_publishHandle, interpret[index].channelDBNo, interpret[index].dataFormat, 
			parse[index], getSizeForPubIndication.Client.m_PublisherDeviceID, 
			getSizeForPubIndication.Client.m_TLSAP_ID, data);

		if (data.DataBuff.size()>0)
			LOG_WARN("Publish -> DeviceID = " << getSizeForPubIndication.Client.m_PublisherDeviceID << "-> extra bytes = " << data.DataBuff.size() << "discovered");
			*/

	}/*end of all data published items for dev*/

}


//added by Cristian.Guef
bool ResponseProcessor::Is_CS_LeaseToSendGetSizeReq(DevicePtr publisherDevice,
								PublishIndication& publishIndication)
{
	unsigned int ContractID = publisherDevice->ContractID(
							(publishIndication.m_MappedPublisherConcentrator_ID << 16) 
							| publishIndication.m_MappedPublisherTLSAP_ID,
							 GContract::Client);

	//don't sent any request until we have a valid contract
	if (ContractID == 0)
	{
		LOG_INFO("Publish_resp -> no c/s lease for CO on deviceID = " << 
			(boost::uint32_t)publishIndication.PublisherDeviceID);
		return false;
	}

	return true;
}

//added by Cristian.Guef
void ResponseProcessor::Get_CS_LeaseToSendGetSizeReq(DevicePtr publisherDevice,
													 PublishIndication& publishIndication)
{

	LOG_INFO("Publish_resp -> create c/s lease for CO on deviceID = " << 
			(boost::uint32_t)publishIndication.PublisherDeviceID);

	processor->HandleInvalidContract((publishIndication.m_MappedPublisherConcentrator_ID << 16) 
						| publishIndication.m_MappedPublisherTLSAP_ID,
						(unsigned char) GContract::Client, -1,
						publisherDevice->id, Command::rsFailure_DeviceHasNoContract);
}

//added by Cristian.Guef
void ResponseProcessor::SendGetSizeReq(DevicePtr publisherDevice,
				PublishIndication& publishIndication)
{

	LOG_INFO("Publish_resp -> for CO on deviceID = " << 
			(boost::uint32_t)publishIndication.PublisherDeviceID <<
			" send get_size_cmd...");

	boost::shared_ptr<GClientServer<GetSizeForPubIndication> > getSizeForIndication
									(new GClientServer<GetSizeForPubIndication>);

	getSizeForIndication->ContractID = publisherDevice->ContractID(
							(publishIndication.m_MappedPublisherConcentrator_ID << 16) 
							| publishIndication.m_MappedPublisherTLSAP_ID,
							 GContract::Client);

	getSizeForIndication->Client.m_TLSAP_ID = publishIndication.m_MappedPublisherTLSAP_ID;
	getSizeForIndication->Client.m_Concentrator_id = publishIndication.m_MappedPublisherConcentrator_ID;
	getSizeForIndication->Client.m_ContentVersion = publishIndication.m_ContentVersion;
	getSizeForIndication->Client.m_PublisherDeviceID = publishIndication.PublisherDeviceID;
	getSizeForIndication->Client.m_SubscriberLowThreshold = publishIndication.m_SubscriberLowThreshold;
	getSizeForIndication->Client.m_SubscriberHighThreshold = publishIndication.m_SubscriberHighThreshold;
	getSizeForIndication->Client.m_publishHandle = publishIndication.m_publishHandle;

	processor->SendRequest(getSizeForIndication);

	//lease mng
	//processor->leaseTrackMng.RemoveLease(nextBulk->ContractID);
}

//added by Cristian.Guef
void ResponseProcessor::SaveFirstPublishedData(PublishIndication& publishIndication, DevicePtr device)
{
	PublishedDataMng::PublishHandle handle = PublishingManager.GetNewPublishHandle();
	publishIndication.m_publishHandle = (int)handle;
	device->SetPublishHandle(publishIndication.m_publishHandle); //used when deleting subscriber lease
	//PublishingManager.AddPublishedData(handle, PublishedDataMng::PublishedData (publishIndication.m_FreshSeqNo, 
	//				publishIndication.m_DataBuff, publishIndication.ReadingTime,
	//				publishIndication.milisec));
	PublishingManager.SaveContentVersion(handle, publishIndication.m_ContentVersion);
	PublishingManager.ChangeLastSequence(handle, publishIndication.m_FreshSeqNo);
	PublishingManager.SetParsingInfoErrorFlag(handle, false); //no parsing info yet
	//now we have to get parsing info
}



void ResponseProcessor::InitPublishEnviroment(PublisherConf::COChannelListT &list, 
											  unsigned char dataVersion, DevicePtr device)
{
	PublishedDataMng::PublishHandle handle = PublishingManager.GetNewPublishHandle();
	device->SetPublishHandle(handle); //used when deleting subscriber lease
	PublishingManager.SaveContentVersion(handle, dataVersion);
	PublishingManager.SetParsingInfoErrorFlag(handle, true); //we have parsing info

	//build parsing info
	std::vector<Subscribe::ObjAttrSize> parseList;
	parseList.resize(list.size());
	for(unsigned int i = 0; i < list.size(); i++)
	{
		parseList[i].ObjID = list[i].objID;
		parseList[i].AttrID = list[i].attrID;
		parseList[i].AttrIndex = list[i].index1 << 8 | list[i].index2;
		parseList[i].Size = GetSize(list[i].format);
		parseList[i].withStatus = list[i].withStatus;
	}
	PublishingManager.SaveParsingInfo(handle, parseList);

	//build interpret list
	PublishedDataMng::InterpretInfoListT interpretList;
	interpretList.resize(list.size());
	for(unsigned int i = 0; i < list.size(); i++)
	{
		interpretList[i].channelDBNo = list[i].dbChannelNo;
		interpretList[i].dataFormat = list[i].format;
	}
	PublishingManager.SetInterpretInfo(handle, interpretList);
	processor->devices.SetPublishErrorFlag(device->id, 1);
}

void ResponseProcessor::Visit(PublishIndication& publishIndication)
{

	if (publishIndication.Status != Command::rsSuccess)
	{
		LOG_WARN("Publish_resp received failed! =" << publishIndication.ToString());
		return;
	}

	if (!PublishingManager.IsHandleValid(publishIndication.m_publishHandle))
	{
		LOG_WARN("Publish_resp received failed, there is no lease for it! =" << publishIndication.ToString());
		return;
	}
	
	//new content version
	if (publishIndication.m_publishHandle == -1) //case: first time for published data
	{
		LOG_ERROR("Publish_resp with new content version = " <<  (boost::uint32_t) publishIndication.m_ContentVersion << "was not configured");
		return;
		
		/* old type
		//get device from db
		DevicePtr publisherDevice = processor->devices.FindDevice(publishIndication.PublisherDeviceID);
		if (!publisherDevice)
		{
			LOG_ERROR("Publish_resp device_id not found in db, so do nothing...");
			return;
		}

		SaveFirstPublishedData(publishIndication, publisherDevice);

		//prepare data for c/s
		if (Is_CS_LeaseToSendGetSizeReq(publisherDevice, publishIndication) == false)
			Get_CS_LeaseToSendGetSizeReq(publisherDevice, publishIndication);
		else
			SendGetSizeReq(publisherDevice, publishIndication);
		return;
		*/
	}
	
	//renew content version?
	if (publishIndication.m_ContentVersion != 
		PublishingManager.GetContentVersion(publishIndication.m_publishHandle))
	{
		LOG_WARN("PUBLISH (ContentVer received=" << (boost::uint32_t) publishIndication.m_ContentVersion 
			 << " and set=" << (boost::uint32_t) PublishingManager.GetContentVersion(publishIndication.m_publishHandle) << ") SKIP");
		if (PublishingManager.IsPublishError(publishIndication.m_publishHandle) == false)
		{
			PublishingManager.SetPublishError(publishIndication.m_publishHandle, true);
			processor->devices.SetPublishErrorFlag(publishIndication.PublisherDeviceID, 1);
		}
		return;
	}
	
	if (PublishingManager.IsPublishError(publishIndication.m_publishHandle) == true)
	{
		PublishingManager.SetPublishError(publishIndication.m_publishHandle, false);
		processor->devices.SetPublishErrorFlag(publishIndication.PublisherDeviceID, 0);
	}
		
	//LOG_DEBUG("Publish_resp with content version = " << (boost::uint32_t) publishIndication.m_ContentVersion);
			
	//new data?
	switch (publishIndication.m_IndicationType)
	{
		case PublishIndication::Publish_Indication:
			if (PublishingManager.IsNextSequenceValid(publishIndication.m_publishHandle, publishIndication.m_FreshSeqNo))
			{
				PublishingManager.ChangeLastSequence(publishIndication.m_publishHandle, publishIndication.m_FreshSeqNo);
				bool isDataSaved;
				FlushDeviceReading(publishIndication.m_publishHandle, publishIndication.PublisherDeviceID,
					publishIndication.m_DataBuff, publishIndication.tv, publishIndication.m_isFirstTime, isDataSaved);
				
				if (publishIndication.m_isFirstTime)
					if (isDataSaved)
						publishIndication.m_isFirstTime = false;
				//LOG_DEBUG("Publish_resp curr. seq.=" << (boost::uint32_t)publishIndication.m_FreshSeqNo << " is valid!");
			}
			else
			{
				LOG_WARN("Publish_resp curr. seq.=" << (boost::uint32_t)publishIndication.m_FreshSeqNo << " is not valid! because last stored seq = " << 
							(int)PublishingManager.GetLastSequence(publishIndication.m_publishHandle));
			}
			break;

		case PublishIndication::Subscriber_Timer:
			//LOG_DEBUG("Publish_resp curr. seq.=" << (boost::uint32_t)publishIndication.m_FreshSeqNo << " doesn't need to be checked");
			PublishingManager.ClearLastSequence(publishIndication.m_publishHandle);
			break;
			
		case PublishIndication::Watchdog_Timer:
			PublishingManager.ClearLastSequence(publishIndication.m_publishHandle);
			//if (PublishingManager.IsStaleData(publishIndication.m_publishHandle) == false)
			//{
				//LOG_INFO("WATCHDOG in check! with devID = " << publishIndication.PublisherDeviceID);
				PublishingManager.SetStaleDataStatus(publishIndication.m_publishHandle, true);
				processor->devices.UpdatePublishFlag(publishIndication.PublisherDeviceID, 2/*stale data*/);
			//}
			return;
		
		default:
			LOG_ERROR("Publish_Timer unknown!");
			return;
	}
}

void ResponseProcessor::Visit(GClientServer<GetFirmwareVersion>& getFirmwareVersion)
{
	if (getFirmwareVersion.Status == Command::rsSuccess)
	{
		std::string opStatus = "";
		switch (getFirmwareVersion.Client.OperationStatus)
		{
		case 0:
			opStatus = boost::str(boost::format("Version=%1%") % getFirmwareVersion.Client.FirmwareVersion.ToString());
			break;
		case 1:
			opStatus = "device does not exist";
			break;
		default:
			opStatus = "unknown status";
			break;
		}
		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), opStatus);
	}
	else
	{
		if(getFirmwareVersion.Status == Command::rsFailure_GatewayContractExpired){
			unsigned short TLDE_SAPID = apSMAP;
			unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetDevice(command->deviceID);

			device->ResetContractID(resourceID, (unsigned char) GContract::Client);	
			//set command_status as new
			processor->commands.SetDBCmdID_as_new(command->commandID);
			LOG_INFO(boost::str(boost::format(" lease expired for DeviceID=%1%") % command->deviceID));
			return;
		}
		else
		{
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), getFirmwareVersion.Status);
		}
	}

	//lease mng
	if (getFirmwareVersion.Status == Command::rsFailure_LostConnection)
		return;
	unsigned short TLDE_SAPID = apSMAP;
	unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;	
	DevicePtr device = GetDevice(command->deviceID);
	processor->leaseTrackMng.AddLease(getFirmwareVersion.ContractID, device, resourceID, GContract::Client, nlib::util::seconds(processor->configApp.LeasePeriod()));
}

void ResponseProcessor::Visit(GClientServer<FirmwareUpdate>& startFirmwareUpdate)
{

	if (startFirmwareUpdate.Status == Command::rsSuccess)
	{
		std::string opStatus = "";
		switch (startFirmwareUpdate.Client.OperationStatus)
		{
		case 0:
			opStatus = "upload started";
			break;
		case 1:
			opStatus = "device does not exist";
			break;
		case 2:
			opStatus = "already exists an active firmware update for this device";
			break;
		case 3:
			opStatus = "file error";
			break;
		default:
			opStatus = "unknown status";
			break;
		}
		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), opStatus);

	}
	else
	{
		/* commented by Cristian.Guef
		processor->CommandFailed(*command, nlib::CurrentLocalTime(), startFirmwareUpdate.Status);
		*/
		
		/* commented by Cristian.Guef
		processor->HandleInvalidContract(command->commandID, command->deviceID, startFirmwareUpdate.Status);
		*/ 
		//added by Cristian.Guef
		if(startFirmwareUpdate.Status == Command::rsFailure_GatewayContractExpired){
			unsigned short TLDE_SAPID = apSMAP;
			unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetDevice(command->deviceID);

			device->ResetContractID(resourceID, (unsigned char) GContract::Client);	
			//set command_status as new
			processor->commands.SetDBCmdID_as_new(command->commandID);
			LOG_INFO(boost::str(boost::format(" lease expired for DeviceID=%1%") % command->deviceID));
			return;
		}
		else
		{
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), startFirmwareUpdate.Status);
		}
	}

	//lease mng
	if (startFirmwareUpdate.Status == Command::rsFailure_LostConnection)
		return;
	unsigned short TLDE_SAPID = apSMAP;
	unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;	
	DevicePtr device = GetDevice(command->deviceID);
	processor->leaseTrackMng.AddLease(startFirmwareUpdate.ContractID, device, resourceID, GContract::Client, nlib::util::seconds(processor->configApp.LeasePeriod()));
}

std::string GetFWUpdateStatus(boost::uint8_t byte)
{
	switch (byte)
	{
	case 0:
		return "Idle";
	case 1:
		return "Downloading";
	case 2:
		return "Uploading";
	case 3:
		return "Applying";
	case 4:
		return "Download Complete";
	case 5:
		return "Upload Complete";
	case 6:
		return "Download Error";
	case 7:
		return "Upload Error";
	}
	return "unknown";
}

void ResponseProcessor::Visit(GClientServer<GetFirmwareUpdateStatus>& getFirmwareUpdateStatus)
{
	if (getFirmwareUpdateStatus.Status == Command::rsSuccess)
	{
		std::string opStatus = "";
		switch (getFirmwareUpdateStatus.Client.OperationStatus)
		{
		case 0:
		{
			opStatus = boost::str(boost::format("Current Phase=%1%; Percent Done=%2%")
			    % GetFWUpdateStatus(getFirmwareUpdateStatus.Client.CurrentPhase)
			    % (int) getFirmwareUpdateStatus.Client.PercentDone);
			break;
		}
		case 1:
			opStatus = "firmware update done";
			break;
		case 2:
			opStatus = "cancel done";
			break;
		case 3:
			opStatus = "no active firmware update";
			break;
		case 4:
			opStatus = "applying not confirmed";
			break;
		case 5:
			opStatus = "cancel not confirmed";
			break;
		case 6:
			opStatus = "fail";
			break;
		default:
			opStatus = "unknown status";
			break;
		}
		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), opStatus);
	}
	else
	{
		/* commented by Cristian.Guef
		processor->CommandFailed(*command, nlib::CurrentLocalTime(), getFirmwareUpdateStatus.Status);
		*/
		
		
		/* commented by Cristian.Guef
		processor->HandleInvalidContract(command->commandID, command->deviceID, getFirmwareUpdateStatus.Status);
		*/
		//added by Cristian.Guef
		if(getFirmwareUpdateStatus.Status == Command::rsFailure_GatewayContractExpired){
			unsigned short TLDE_SAPID = apSMAP;
			unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetDevice(command->deviceID);

			device->ResetContractID(resourceID, (unsigned char) GContract::Client);	
			//set command_status as new
			processor->commands.SetDBCmdID_as_new(command->commandID);
			LOG_INFO(boost::str(boost::format(" lease expired for DeviceID=%1%") % command->deviceID));
			return;
		}
		else
		{
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), getFirmwareUpdateStatus.Status);
		}
	}

	//lease mng
	if (getFirmwareUpdateStatus.Status == Command::rsFailure_LostConnection)
		return;
	unsigned short TLDE_SAPID = apSMAP;
	unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;	
	DevicePtr device = GetDevice(command->deviceID);
	processor->leaseTrackMng.AddLease(getFirmwareUpdateStatus.ContractID, device, resourceID, GContract::Client, nlib::util::seconds(processor->configApp.LeasePeriod()));
}

void ResponseProcessor::Visit(GClientServer<CancelFirmwareUpdate>& cancelFirmwareUpdate)
{
	if (cancelFirmwareUpdate.Status == Command::rsSuccess)
	{
		std::string opStatus = "";
		switch (cancelFirmwareUpdate.Client.OperationStatus)
		{
		case 0:
			opStatus = "success";
			break;
		case 1:
			opStatus = "device does not exist";
			break;
		case 2:
			opStatus = "timeout";
			break;
		case 3:
			opStatus = "no active firmware update";
			break;
		default:
			opStatus = "unknown status";
			break;
		}
		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), opStatus);
	}
	else
	{
		/* commented by Cristian.Guef
		processor->CommandFailed(*command, nlib::CurrentLocalTime(), cancelFirmwareUpdate.Status);
		*/
		
		/* commented by Cristian.Guef
		processor->HandleInvalidContract(command->commandID, command->deviceID, cancelFirmwareUpdate.Status);
		*/
		//added by Cristian.Guef
		if(cancelFirmwareUpdate.Status == Command::rsFailure_GatewayContractExpired){
			unsigned short TLDE_SAPID = apSMAP;
			unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;

			DevicePtr device = GetDevice(command->deviceID);

			device->ResetContractID(resourceID, (unsigned char) GContract::Client);	
			//set command_status as new
			processor->commands.SetDBCmdID_as_new(command->commandID);
			LOG_INFO(boost::str(boost::format(" lease expired for DeviceID=%1%") % command->deviceID));
			return;
		}
		else
		{
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), cancelFirmwareUpdate.Status);
		}
	}

	//lease mng
	if (cancelFirmwareUpdate.Status == Command::rsFailure_LostConnection)
		return;
	unsigned short TLDE_SAPID = apSMAP;
	unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;	
	DevicePtr device = GetDevice(command->deviceID);
	processor->leaseTrackMng.AddLease(cancelFirmwareUpdate.ContractID, device, resourceID, GContract::Client, nlib::util::seconds(processor->configApp.LeasePeriod()));
}


// 1 - IP found (src or dest)
// 0  - no IP found (src or dest)
// -1 - no graphID
static int FindNextDev(int currVertexNo, int graphID, int VertexNo/*src or dest*/, int &nextVertexNo/*[in/out]*/)
{
	//LOG_DEBUG("curre = " << currVertexNo << "vertex = " <<  VertexNo << "size=" << (int)TopoRepPtr->DevicesList.size());
	assert(currVertexNo >= 0 && currVertexNo < (int)TopoRepPtr->DevicesList.size());
	assert(VertexNo >= 0 && VertexNo < (int)TopoRepPtr->DevicesList.size());

	nextVertexNo = -1; //no next ip

	//find graphID
	for (unsigned int i = 0; i < TopoRepPtr->DevicesList[currVertexNo].GraphsList.size(); i++)
	{
		if (TopoRepPtr->DevicesList[currVertexNo].GraphsList[i].GraphID == graphID)
		{
			//find srcIP or destIP
			for(unsigned int j = 0; j < TopoRepPtr->DevicesList[currVertexNo].GraphsList[i].infoList.size(); j++)
			{
				if (TopoRepPtr->DevicesList[currVertexNo].GraphsList[i].infoList[j].DevListIndex != -1)
				{
					nextVertexNo = TopoRepPtr->DevicesList[currVertexNo].GraphsList[i].infoList[j].DevListIndex; //when IP not found choose a next hop
					if (TopoRepPtr->DevicesList[currVertexNo].GraphsList[i].infoList[j].DevListIndex == VertexNo)
						return 1;
				}
			}
			return 0;
		}
	}
	
	//if destIP/source is SM or GW
	//LOG_DEBUG("src/dest node = " << TopoRepPtr->DevicesList[VertexNo].DeviceIP.ToString() << "curr node = " << TopoRepPtr->DevicesList[currVertexNo].DeviceIP.ToString());
	if ((TopoRepPtr->DevicesList[VertexNo].DeviceType == 1/*sm*/ || 
			TopoRepPtr->DevicesList[VertexNo].DeviceType == 2/*gw*/) && 
			TopoRepPtr->DevicesList[currVertexNo].DeviceType == 3/*bbr*/)
		{
			return 1;
		}
	
	return -1;
}
// 2 - destIP found
// 1 - srcIP found
// 0  - no srcIP and DestIP found
// -1 - no graphID 
static int FindNextDev(int currVertexNo, int graphID, int srcVertexNo, int destVertexNo, int &nextVertexNo/*[in/out]*/)
{
	//LOG_DEBUG("curre = " << currVertexNo << "srcvertex = " <<  srcVertexNo << "destvertex = " <<  destVertexNo << "size=" << (int)TopoRepPtr->DevicesList.size());
	assert(currVertexNo >= 0 && currVertexNo < (int)TopoRepPtr->DevicesList.size());
	assert(srcVertexNo >= 0 && srcVertexNo < (int)TopoRepPtr->DevicesList.size());
	assert(destVertexNo >= 0 && destVertexNo < (int)TopoRepPtr->DevicesList.size());

	nextVertexNo = -1; //no next vertex

	//find graphID
	for (unsigned int i = 0; i < TopoRepPtr->DevicesList[currVertexNo].GraphsList.size(); i++)
	{
		if (TopoRepPtr->DevicesList[currVertexNo].GraphsList[i].GraphID == graphID)
		{
			//find srcIP or destIP
			for(unsigned int j = 0; j < TopoRepPtr->DevicesList[currVertexNo].GraphsList[i].infoList.size(); j++)
			{
				if (TopoRepPtr->DevicesList[currVertexNo].GraphsList[i].infoList[j].DevListIndex != -1)
				{
					nextVertexNo = TopoRepPtr->DevicesList[currVertexNo].GraphsList[i].infoList[j].DevListIndex; //when no srcIP or destIP found we have to choose a next vertex
					if (TopoRepPtr->DevicesList[currVertexNo].GraphsList[i].infoList[j].DevListIndex == srcVertexNo)
						return 1;
					if (TopoRepPtr->DevicesList[currVertexNo].GraphsList[i].infoList[j].DevListIndex == destVertexNo)
						return 2;
				}
			}
			return 0;
		}
	}
	return -1;
}
//added 
// 0 -not ok
// 1 - ok
static int FindRoute(DevicesManager &devices, int currVertexNo, const ContractsAndRoutes::Route::RouteElementsT& route, unsigned int currIndex, 
			   int srcVertexNo, int destVertexNo, std::deque<int/*deviceIDs*/> &devIDs, int storeDirection /*0 - no srcIP or destIP found
																									 1 - queue(srcIP found first)
																									 2 - stack(destIP found first)*/, int fowardLimit, int count)
{
	
	if (!(currVertexNo >= 0 && currVertexNo < (int)TopoRepPtr->DevicesList.size()))
		return 0;
	if (!(srcVertexNo >= 0 && srcVertexNo < (int)TopoRepPtr->DevicesList.size()))
		return 0;
	if (!(destVertexNo >= 0 && destVertexNo < (int)TopoRepPtr->DevicesList.size()))
		return 0;
	
	if (count == fowardLimit)
		return 0;
	
	if (route[currIndex].isGraph == 1/*graphID*/)
	{
		if (storeDirection == 0)
		{
			int nextVertexNo = -1;
			switch(FindNextDev(currVertexNo, route[currIndex].elem.graphID, srcVertexNo, destVertexNo, nextVertexNo))
			{
			case 2/*destIP found*/:
				devIDs.push_back(TopoRepPtr->DevicesList[destVertexNo].device_dbID);
				return FindRoute(devices, currVertexNo, route, currIndex, srcVertexNo, destVertexNo, devIDs, 2, fowardLimit, ++count);
			case 1/*srcIP found*/:
				return FindRoute(devices, currVertexNo, route, currIndex, srcVertexNo, destVertexNo, devIDs, 1, fowardLimit, ++count);
			case 0/*no srcIP and DestIP found*/:
				if (nextVertexNo == -1)
					return 0;
				return FindRoute(devices, nextVertexNo, route, currIndex, srcVertexNo, destVertexNo, devIDs, 0, fowardLimit, ++count);
			case -1/*no graphID*/:
				if (++currIndex == route.size())
					return 0; //no route found
				if (route[currIndex].isGraph == 1/*graphID*/)
					return FindRoute(devices, currVertexNo, route, currIndex, srcVertexNo, destVertexNo, devIDs, 0, fowardLimit, ++count);
				//let it flow to the other type of route_elem
				--currIndex;
				break;
			default:
				assert(false);
			}
		}
		if (storeDirection == 1)
		{
			int nextVertexNo = -1;
			switch(FindNextDev(currVertexNo, route[currIndex].elem.graphID, destVertexNo, nextVertexNo))
			{
			case 1/*IP found - dest*/:
				devIDs.push_back(TopoRepPtr->DevicesList[destVertexNo].device_dbID);
				return 1;
			case 0/*IP not found - dest*/:
				if (nextVertexNo == -1)
					return 0;
				devIDs.push_back(TopoRepPtr->DevicesList[nextVertexNo].device_dbID);
				return FindRoute(devices, nextVertexNo, route, currIndex, srcVertexNo, destVertexNo, devIDs, 1, fowardLimit, ++count);
			case -1/*no graphID*/:
				if (++currIndex == route.size())
					return 0; //no route found
				if (route[currIndex].isGraph == 1/*graphID*/)
					return FindRoute(devices, currVertexNo, route, currIndex, srcVertexNo, destVertexNo, devIDs, 1, fowardLimit, ++count);
				//let it flow to the other type of route_elem
				--currIndex;
				break;
			default:
				assert(false);
			}
		}
		if (storeDirection == 2)
		{
			int nextVertexNo = -1;
			switch(FindNextDev(currVertexNo, route[currIndex].elem.graphID, srcVertexNo, nextVertexNo))
			{
			case 1/*IP found - src*/:
				return 1;
			case 0/*IP not found - src*/:
				if (nextVertexNo == -1)
					return 0;
				devIDs.push_front(TopoRepPtr->DevicesList[nextVertexNo].device_dbID);
				return FindRoute(devices, nextVertexNo, route, currIndex, srcVertexNo, destVertexNo, devIDs, 2, fowardLimit, ++count);
			case -1/*no graphID*/:
				if (++currIndex == route.size())
					return 0; //no route found
				if (route[currIndex].isGraph == 1/*graphID*/)
					return FindRoute(devices, currVertexNo, route, currIndex, srcVertexNo, destVertexNo, devIDs, 2, fowardLimit, ++count);
				//let it flow to the other type of route_elem
				--currIndex;
				break;
			default:
				assert(false);
			}
		}
	}
	
	if (storeDirection == 0)
	{
		DevicePtr node;
				
		if (++currIndex == route.size())
			return 0; //no route found

		if (route[currIndex].isGraph == 1/*graphID*/)
			return FindRoute(devices, currVertexNo, route, currIndex, srcVertexNo, destVertexNo, devIDs, 0, fowardLimit, ++count);

		if (!(node = devices.repository.Find(IPv6(route[currIndex].elem.nodeAddress))))
			return 0;

		if (node->GetVertexNo() == srcVertexNo)
			return FindRoute(devices, node->GetVertexNo(), route, currIndex, srcVertexNo, destVertexNo, devIDs, 1, fowardLimit, ++count);
		if (node->GetVertexNo() == destVertexNo)
			return FindRoute(devices, node->GetVertexNo(), route, currIndex, srcVertexNo, destVertexNo, devIDs, 2, fowardLimit, ++count);

		return FindRoute(devices, node->GetVertexNo(), route, currIndex, srcVertexNo, destVertexNo, devIDs, 0, fowardLimit, ++count);
	}
	if (storeDirection == 1)
	{
		DevicePtr node;
				
		if (++currIndex == route.size())
			return 0; //no route found

		if (route[currIndex].isGraph == 1/*graphID*/)
			return FindRoute(devices, currVertexNo, route, currIndex, srcVertexNo, destVertexNo, devIDs, 1, fowardLimit, ++count);

		if (!(node = devices.repository.Find(IPv6(route[currIndex].elem.nodeAddress))))
			return 0;

		if (node->GetVertexNo() == destVertexNo)
		{
			devIDs.push_back(TopoRepPtr->DevicesList[destVertexNo].device_dbID);
			return 1;
		}

		return FindRoute(devices, node->GetVertexNo(), route, currIndex, srcVertexNo, destVertexNo, devIDs, 1, fowardLimit, ++count);
	}
	if (storeDirection == 2)
	{
		DevicePtr node;
				
		if (++currIndex == route.size())
			return 0; //no route found

		if (route[currIndex].isGraph == 1/*graphID*/)
			return FindRoute(devices, currVertexNo, route, currIndex, srcVertexNo, destVertexNo, devIDs, 2, fowardLimit, ++count);

		if (!(node = devices.repository.Find(IPv6(route[currIndex].elem.nodeAddress))))
			return 0;

		if (node->GetVertexNo() == srcVertexNo)
			return 1;

		return FindRoute(devices, node->GetVertexNo(), route, currIndex, srcVertexNo, destVertexNo, devIDs, 2, fowardLimit, ++count);
	}
	return 0;
}
static void SaveContractElements(DevicesManager &devices, int contactID, const IPv6 &startIP, 
								 const ContractsAndRoutes::Route::RouteElementsT& route, 
								 const IPv6 &srcIP, const IPv6 &destIP, int fowardLimit)
{
	if(!route.size() )
	{
		LOG_ERROR("ERROR SaveContractElements: route.size() == 0");
		return;
	} 
	

	//check in cache
	DevicePtr node;
	int startVertexNo;
	bool isStartDevBBR = false;
	int startID;
	DevicePtr nullDev;
	if ((node = devices.repository.Find(startIP)) != nullDev)
	{
		if (node->Type() == Device::dtBackboneRouter)
			isStartDevBBR = true;

		startID = node->id;
		startVertexNo = node->GetVertexNo();
		LOG_DEBUG("start node = " << node->IP().ToString() << "has vertex no = " << startVertexNo);
	}
	else
	{
		LOG_ERROR("no startIP found in cache");
		return;
	}
	int srcVertexNo;
	int srcID;
	if ((node = devices.repository.Find(srcIP)) != nullDev)
	{
		srcVertexNo = node->GetVertexNo();
		srcID = node->id;
		LOG_DEBUG("src node = " << node->IP().ToString() << "has vertex no = " << srcVertexNo << "and dbID= " << srcID);
	}
	else
	{
		LOG_ERROR("no srcIP found in cache");
		return;
	}
	int destVertexNo;
	int destDevID;
	if ((node = devices.repository.Find(destIP)) != nullDev)
	{
		destVertexNo = node->GetVertexNo();
		destDevID = node->id;
		LOG_DEBUG("dest node = " << node->IP().ToString() << "has vertex no = " << destVertexNo << "and dbID= " << destDevID);
	}
	else
	{
		LOG_ERROR("no destIP found in cache");
		return;
	}


	//find route
	std::deque<int/*deviceIDs*/> devIDs;
	
	if (isStartDevBBR)
	{
		devIDs.push_back(startID);
		if (FindRoute(devices, startVertexNo, route, 0, srcVertexNo, destVertexNo, devIDs, 1, fowardLimit, 0))
			devices.InsertContractElements(contactID, srcID, devIDs);
		return;
	}

	if (startVertexNo == srcVertexNo)
	{
		if (FindRoute(devices, startVertexNo, route, 0, srcVertexNo, destVertexNo, devIDs, 1, fowardLimit, 0))
			devices.InsertContractElements(contactID, srcID, devIDs);
		return;
	}

	if (startVertexNo == destVertexNo)
	{
		devIDs.push_back(destDevID);
		if (FindRoute(devices, startVertexNo, route, 0, srcVertexNo, destVertexNo, devIDs, 2, fowardLimit, 0))
			devices.InsertContractElements(contactID, srcID, devIDs);
		return;
	}

	if (FindRoute(devices, startVertexNo, route, 0, srcVertexNo, destVertexNo, devIDs, 0, fowardLimit, 0))
		devices.InsertContractElements(contactID, srcID, devIDs);
}

static void SaveRouteForContracts(DevicesManager &devices, const IPv6 &sourceIP, 
								  ContractsAndRoutes::ContractIDsListT &idsList, 
								  const ContractsAndRoutes::RoutesListT &routesList)
{
	//delete previous info
	DevicePtr sourceDev= devices.repository.Find(sourceIP);
	if (!sourceDev)
	{
		LOG_WARN("saving routes: no sourcedev_IP = " << sourceIP.ToString() << "found in cache!");
		return;
	}

	//devices.RemoveContractElements(sourceDev->id);

	//make routes for gw, bbr and sm 
	if (sourceDev->Type() == Device::dtGateway || sourceDev->Type() == Device::dtBackboneRouter || 
			sourceDev->Type() == Device::dtSystemManager)
	{
		//find dest: gw, sm , bbr
		std::deque<int/*deviceIDs*/> devIDs;
		for (ContractsAndRoutes::ContractIDsListT::iterator j = idsList.begin(); j != idsList.end();)
		{
			DevicePtr node;
			DevicePtr nullDev;
			if ((node = devices.repository.Find(j->second)) != nullDev)
			{
				if (node->Type() == Device::dtGateway || node->Type() == Device::dtBackboneRouter || 
					node->Type() == Device::dtSystemManager)
				{
					devIDs.push_back(node->id);
					devices.InsertContractElements(j->first, sourceDev->id, devIDs);
					LOG_DEBUG("start node = " << sourceDev->IP().ToString() << " and dest node = " << j->second.ToString() << "and contractID=" << j->first << " save route in db");

					ContractsAndRoutes::ContractIDsListT::iterator currIt = j++;
					idsList.erase(currIt);
					continue;
				}
			}
			else
			{
				LOG_WARN("destination address not found = " << j->second.ToString());
				ContractsAndRoutes::ContractIDsListT::iterator currIt = j++;
				idsList.erase(currIt);
				continue;
			}
			++j;
		}
	}

	for (unsigned int i = 0; i < routesList.size(); i++)
	{
		if (routesList[i].alternative == 0)
		{
			ContractsAndRoutes::ContractIDsListT::const_iterator j = idsList.find(routesList[i].selectorn.contractID);
			if (j == idsList.end())
			{
				LOG_WARN("contract id has not been found in contracts list for alternative = 0");
				continue;
			}
			SaveContractElements(devices, routesList[i].selectorn.contractID, IPv6(routesList[i].srcAddress),
				routesList[i].routes, sourceIP, j->second, routesList[i].forwardLimit);
		}
		if (routesList[i].alternative == 1)
		{
			ContractsAndRoutes::ContractIDsListT::const_iterator j = idsList.find(routesList[i].selectorn.contractID);
			if (j == idsList.end())
			{
				LOG_WARN("contract id has not been found in contracts list for alternative = 1");
				continue;
			}
			DevicePtr dev= devices.BBRDevice();
			if (!dev)
			{
				LOG_WARN("no device bbr for altternative =1");
				continue;
			}
			SaveContractElements(devices, routesList[i].selectorn.contractID, dev->IP(),
				routesList[i].routes, sourceIP, j->second, routesList[i].forwardLimit);
		}
		if (routesList[i].alternative == 2)
		{
			DevicePtr dev= devices.BBRDevice();
			if (!dev)
			{
				LOG_WARN("no device bbr for altternative =2");
				continue;
			}
			for (ContractsAndRoutes::ContractIDsListT::const_iterator j = idsList.begin();
				j != idsList.end(); j++)
			{
				LOG_DEBUG("startIP = " << dev->IP().ToString() << "sourceIP=" << sourceIP.ToString() << "destIP=" << j->second.ToString());
				
				//if (IPv6(routesList[i].selectorn.nodeAddress) == j->second)
					SaveContractElements(devices, j->first, dev->IP(),
						routesList[i].routes, sourceIP, j->second, routesList[i].forwardLimit);
				//else
					//LOG_WARN("destination addr = " << IPv6(routesList[i].selectorn.nodeAddress).ToString() << "doesn't match with addr = " << j->second.ToString()
					   //<< "in alternative = 1");
			}
		}
		if (routesList[i].alternative == 3)
		{
			for (ContractsAndRoutes::ContractIDsListT::const_iterator j = idsList.begin();
				j != idsList.end(); j++)
			{
				
				LOG_DEBUG("startIP = " << sourceIP.ToString() << "sourceIP=" << sourceIP.ToString() << "destIP=" << j->second.ToString());
				SaveContractElements(devices, j->first, sourceIP,
					routesList[i].routes, sourceIP, j->second, routesList[i].forwardLimit);
			}
		}
	}
}

static void SaveRoutes(DevicesManager &devices,
							 const IPv6 &startIP, const IPv6 &sourceIP,
							const ContractsAndRoutes::ContractIDsListT &idsList, 
							const ContractsAndRoutes::Altern2DestsListT &destsList, 
							const ContractsAndRoutes::RoutesListT &routesList)
{
	for (ContractsAndRoutes::ContractIDsListT::const_iterator i = idsList.begin(); i != idsList.end(); ++i)
	{
		ContractsAndRoutes::Altern2DestsListT::const_iterator j = destsList.find(i->second);
		if (j == destsList.end())
			continue;

		LOG_DEBUG("startIP = " << startIP.ToString() << "sourceIP=" << sourceIP.ToString() << "destIP=" << j->first.ToString());
		SaveContractElements(devices, i->first, startIP,
			routesList[j->second].routes, sourceIP, j->first, routesList[j->second].forwardLimit);
	}
}

//added
void ResponseProcessor::Visit(GClientServer<GetContractsAndRoutes>& getContractsAndRoutes)
{
	//test time
	WATCH_DURATION_INIT(oDurationWatcher,1000,1000);
	
	if (getContractsAndRoutes.Status == Command::rsSuccess)
	{

		if (!getContractsAndRoutes.Client.isForFirmDl)
		{

			try
			{

				processor->devices.factoryDal.BeginTransaction();
				processor->devices.DeleteContracts();
				processor->devices.RemoveContractElements();
				
				int gwIndex=-1;
				int smIndex=-1;
				int bbrIndex=-1;
				for (unsigned int i = 0; i < getContractsAndRoutes.Client.DevicesInfo.size(); i++)
				{
					if (getContractsAndRoutes.Client.DevicesInfo[i].ContractsList.size() == 0)
						continue;

					DevicePtr sourceDev= processor->devices.repository.Find(
								getContractsAndRoutes.Client.DevicesInfo[i].ContractsList[0].sourceAddress);
					if (!sourceDev)
					{
						LOG_WARN("saving routes: no sourcedev_IP = " << 
							getContractsAndRoutes.Client.DevicesInfo[i].ContractsList[0].sourceAddress.ToString() << "found in cache!");
						continue;
					}
					switch(sourceDev->Type())
					{
					case Device::dtSystemManager:
						smIndex = i;
						break;
					case Device::dtGateway:
						gwIndex = i;
						break;
					case Device::dtBackboneRouter:
						bbrIndex = i;
						break;
					default:
						break;
					}

					processor->devices.ChangeContracts(getContractsAndRoutes.Client.DevicesInfo[i].ContractsList[0].sourceAddress, 
										getContractsAndRoutes.Client.DevicesInfo[i].ContractsList);
					processor->devices.ChangeRoutes(getContractsAndRoutes.Client.DevicesInfo[i].ContractsList[0].sourceAddress,
						getContractsAndRoutes.Client.DevicesInfo[i].RoutesList);
					
					SaveRouteForContracts(processor->devices, 
						getContractsAndRoutes.Client.DevicesInfo[i].ContractsList[0].sourceAddress, 
						getContractsAndRoutes.Client.DevicesInfo[i].ContractIDsList,
						getContractsAndRoutes.Client.DevicesInfo[i].RoutesList);
				}

				//process routes for gw
				if (gwIndex != -1 && bbrIndex != -1)
				{
					
					SaveRoutes(processor->devices,
						getContractsAndRoutes.Client.DevicesInfo[bbrIndex].ContractsList[0].sourceAddress,
						getContractsAndRoutes.Client.DevicesInfo[gwIndex].ContractsList[0].sourceAddress, 
						getContractsAndRoutes.Client.DevicesInfo[gwIndex].ContractIDsList,
						getContractsAndRoutes.Client.DevicesInfo[bbrIndex].Altern2DestsList,
						getContractsAndRoutes.Client.DevicesInfo[bbrIndex].RoutesList);
					
				}
				
				//process routes for sm
				if (smIndex != -1 && bbrIndex != -1)
				{
					SaveRoutes(processor->devices,
						getContractsAndRoutes.Client.DevicesInfo[bbrIndex].ContractsList[0].sourceAddress,
						getContractsAndRoutes.Client.DevicesInfo[smIndex].ContractsList[0].sourceAddress, 
						getContractsAndRoutes.Client.DevicesInfo[smIndex].ContractIDsList,
						getContractsAndRoutes.Client.DevicesInfo[bbrIndex].Altern2DestsList,
						getContractsAndRoutes.Client.DevicesInfo[bbrIndex].RoutesList);
						
				}
				
				
				processor->devices.factoryDal.CommitTransaction();
				WATCH_DURATION_OBJ(oDurationWatcher);
			}
			catch(std::exception ex)
			{
				processor->devices.factoryDal.RollbackTransaction();
				LOG_ERROR("contrats_and routes couldn't be saved");
				throw ex;
			}
			
		}
		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), "success");
	}
	else
	{
		processor->CommandFailed(*command, nlib::CurrentUniversalTime(), getContractsAndRoutes.Status);
		
		if(getContractsAndRoutes.Status == Command::rsFailure_GatewayContractExpired){
			unsigned short TLDE_SAPID = apUAP1;
			unsigned short ObjID = SYSTEM_MONITORING_OBJECT;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetDevice(command->deviceID);
			//it will be recreated by the period_task
			device->ResetContractID(resourceID, (unsigned char) GContract::Client);	
			LOG_INFO(boost::str(boost::format(" lease expired for DeviceID=%1%") % command->deviceID));

			//test
			return;
		}
	}

	//lease mng
	if (getContractsAndRoutes.Status == Command::rsFailure_LostConnection)
	{
		//test
		
		return;
	}
	//unsigned short TLDE_SAPID = apUAP1;
	//unsigned short ObjID = SYSTEM_MONITORING_OBJECT;
	//unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
	//DevicePtr device = GetDevice(command->deviceID);
	//no need to add it for there is an automatic task using it
	//processor->leaseTrackMng.AddLease(getContractsAndRoutes.ContractID, device, resourceID, GContract::Client, nlib::util::seconds(processor->configApp.LeasePeriod()));

	//test
	
}

//added by Cristian.Guef
static void SavePublish(int devID, ISACSInfo &csInfo, CommandsProcessor* processor)
{
	DeviceChannel channel;
	channel.channelName = "N/A";
	channel.channelNumber = csInfo.m_dbChannelNo;
	channel.mappedTSAPID = csInfo.m_tsapID;
	channel.mappedObjectID = csInfo.m_objID;
	channel.mappedAttributeID = csInfo.m_objResID;

	DeviceReading reading;
	reading.deviceID = devID;
	//reading.channel = channel;
	reading.channelNo = csInfo.m_dbChannelNo;
	reading.rawValue.dataType = (nisa100::hostapp::ChannelValue::DataType) csInfo.m_format; //=-1 /*no format has been chosen*/
	reading.rawValue.value.uint32 = csInfo.m_rawData; //use the largest space for union
	reading.readingType = DeviceReading::ClientServer;

	//reading.readingTime = csInfo.m_readTime;
	//reading.milisec = csInfo.m_milisec;
	reading.tv = csInfo.tv;
	reading.IsISA = csInfo.m_isISA;
	if (reading.IsISA)
		reading.ValueStatus = csInfo.m_valueStatus;

	processor->devices.SaveReading(reading);
}

//added by Cristian.Guef
void ResponseProcessor::Visit(GClientServer<GISACSRequest>& CSRequest)
{
	if (CSRequest.Status == Command::rsSuccess)
	{
		//save reading
		//if (CSRequest.Client.Info.m_reqType != 4 /*write*/ && CSRequest.Client.Info.m_strRespDataBuff.size() != 0)
		//	processor->devices.SaveISACSConfirm(command->deviceID, CSRequest.Client.Info);
		//if (CSRequest.Client.Info.m_ReadAsPublish == 1)
		//	SavePublish(command->deviceID, CSRequest.Client.Info, processor);

		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), CSRequest.Client.Info.m_strRespDataBuff);
	}
	else
	{
		if(CSRequest.Status == Command::rsFailure_GatewayContractExpired){
			unsigned short TLDE_SAPID = CSRequest.Client.Info.m_tsapID;
			unsigned short ObjID = CSRequest.Client.Info.m_objID;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetDevice(command->deviceID);
			
			device->ResetContractID(resourceID, (unsigned char) GContract::Client);	

			//set command_status as new
			processor->commands.SetDBCmdID_as_new(command->commandID);

			LOG_INFO(boost::str(boost::format(" lease expired for DeviceID=%1%") % command->deviceID));
			return;
		}
		
		processor->CommandFailed(*command, nlib::CurrentUniversalTime(), CSRequest.Status);
	}

	//lease mng
	if (CSRequest.Status == Command::rsFailure_LostConnection)
		return;
	unsigned short TLDE_SAPID = CSRequest.Client.Info.m_tsapID;
	unsigned short ObjID = CSRequest.Client.Info.m_objID;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
	DevicePtr device = GetDevice(command->deviceID);
	processor->leaseTrackMng.AddLease(CSRequest.ContractID, device, resourceID, GContract::Client, nlib::util::seconds(processor->configApp.LeasePeriod()));
}

void ResponseProcessor::Visit(GClientServer<ResetDevice>& resetDevice)
{
	if (resetDevice.Status == Command::rsSuccess)
	{
		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), "success");
	}
	else
	{
		/* commented by Cristian.Guef
		processor->CommandFailed(*command, nlib::CurrentLocalTime(), resetDevice.Status);
		*/
		
		/* commented by Cristian.Guef
		processor->HandleInvalidContract(command->commandID, command->deviceID, resetDevice.Status);
		*/
		//added by Cristian.Guef
		if(resetDevice.Status == Command::rsFailure_GatewayContractExpired){
			unsigned short TLDE_SAPID = apUAP1;
			unsigned short ObjID = SYSTEM_MONITORING_OBJECT;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetDevice(command->deviceID);

			device->ResetContractID(resourceID, (unsigned char) GContract::Client);	
			//set command_status as new
			processor->commands.SetDBCmdID_as_new(command->commandID);
			LOG_INFO(boost::str(boost::format(" lease expired for DeviceID=%1%") % command->deviceID));
			return;
		}
		else
		{
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), resetDevice.Status);
		}
	}

	//lease mng
	if (resetDevice.Status == Command::rsFailure_LostConnection)
		return;
	unsigned short TLDE_SAPID = apUAP1;
	unsigned short ObjID = SYSTEM_MONITORING_OBJECT;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
	DevicePtr device = GetDevice(command->deviceID);
	processor->leaseTrackMng.AddLease(resetDevice.ContractID, device, resourceID, GContract::Client, nlib::util::seconds(processor->configApp.LeasePeriod()));
}

void ResponseProcessor::Visit(GSensorFrmUpdateCancel &sensorFrmUpdateCancel)
{
	if ( IsBulkInProgreass(sensorFrmUpdateCancel.devID) )
	{	/// Fail command in BD regardless if bulk transfer cancel failed or not. There is nothing else to do
		processor->devices.SetFirmDlStatus( sensorFrmUpdateCancel.devID, FWTYPE_SENZOR_BOARD, DevicesManager::FirmDlStatus_Cancelled);
	}
	
	if (sensorFrmUpdateCancel.Status == Command::rsSuccess)
	{
		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), "success");
		if (IsBulkInProgreass(sensorFrmUpdateCancel.devID))
		{
			unsigned short objID = 0;
			unsigned short tsapid = 0;
			GetBulkInProgressVal(sensorFrmUpdateCancel.devID, tsapid, objID, command->commandID);
			processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), "firmware upload cancelled");
			RemoveBulkInProgress(sensorFrmUpdateCancel.devID);

			//lease mng
			unsigned short TLDE_SAPID = sensorFrmUpdateCancel.port;
			unsigned short ObjID = sensorFrmUpdateCancel.objID;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			DevicePtr device = GetDevice(sensorFrmUpdateCancel.devID);
			processor->leaseTrackMng.AddLease(sensorFrmUpdateCancel.ContractID, device, resourceID, GContract::BulkTransforClient, nlib::util::seconds(processor->configApp.LeasePeriod()));
		}
		return;
	}

	if(sensorFrmUpdateCancel.Status == Command::rsFailure_UnknownResource)
	{
		unsigned short TLDE_SAPID = sensorFrmUpdateCancel.port;
		unsigned short ObjID = sensorFrmUpdateCancel.objID;
		unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
		
		DevicePtr device = GetDevice(sensorFrmUpdateCancel.devID);
		//it will be recreated by the period_task
		device->ResetContractID(resourceID, (unsigned char) GContract::BulkTransforClient);					

		LOG_INFO(boost::str(boost::format(" lease expired for DeviceID=%1%") % sensorFrmUpdateCancel.devID));

		//set command_status as new
		processor->commands.SetDBCmdID_as_new(command->commandID);
		return;
	}
	
	processor->CommandFailed(*command, nlib::CurrentUniversalTime(), sensorFrmUpdateCancel.Status);
}
void ResponseProcessor::Visit(GClientServer<GetChannelsStatistics>& getChannels)
{
	if (getChannels.Status == Command::rsSuccess)
	{
		processor->devices.DeleteChannelsStatistics(getChannels.Client.DeviceID);
		processor->devices.SaveChannelsStatistics(getChannels.Client.DeviceID, getChannels.Client.strCCABackoffList);
		//processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), "success");
		processor->commands.SetCommandResponded(*command, nlib::CurrentUniversalTime(), getChannels.Client.strCCABackoffList);
		
	}
	else
	{
		if(getChannels.Status == Command::rsFailure_GatewayContractExpired){
			unsigned short TLDE_SAPID = apUAP1;
			unsigned short ObjID = SYSTEM_MONITORING_OBJECT;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetDevice(command->deviceID);

			device->ResetContractID(resourceID, (unsigned char) GContract::Client);	
			//set command_status as new
			processor->commands.SetDBCmdID_as_new(command->commandID);
			LOG_INFO(boost::str(boost::format(" lease expired for DeviceID=%1%") % command->deviceID));
			return;
		}
		else
		{
			processor->CommandFailed(*command, nlib::CurrentUniversalTime(), getChannels.Status);
		}
	}

	//lease mng
	if (getChannels.Status == Command::rsFailure_LostConnection)
		return;
	unsigned short TLDE_SAPID = apUAP1;
	unsigned short ObjID = SYSTEM_MONITORING_OBJECT;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
	DevicePtr device = GetDevice(command->deviceID);
	processor->leaseTrackMng.AddLease(getChannels.ContractID, device, resourceID, GContract::Client, nlib::util::seconds(processor->configApp.LeasePeriod()));
}

DevicePtr ResponseProcessor::GetDevice(int deviceID)
{
	DevicePtr device = processor->devices.FindDevice(deviceID);
	if (!device)
	{
		THROW_EXCEPTION1(DeviceNotFoundException, deviceID);
	}
	return device;
}

void ResponseProcessor::CreateCancelCommand(int deviceID, int subscriberDeviceID, int publisherChannelID,
  int failedPublishSubscribeCommandID, bool isLocalLoop)
{
	Command cancelCommand;
	cancelCommand.commandCode = isLocalLoop
	  ? Command::ccCancelLocalLoop
	  : Command::ccCancelPublishSubscribe;
	cancelCommand.deviceID = deviceID;
	cancelCommand.generatedType = Command::cgtAutomatic;

	if (!isLocalLoop)
	{
		cancelCommand.parameters.push_back(CommandParameter(CommandParameter::PublishSubscribe_SubscriberDeviceID,
		    boost::lexical_cast<std::string>(subscriberDeviceID)));
		    cancelCommand.parameters.push_back(CommandParameter(CommandParameter::PublishSubscribe_PublisherChannelNo,
		    boost::lexical_cast<std::string>(publisherChannelID)));
	    }
	    else
	    {
		    cancelCommand.parameters.push_back(CommandParameter(CommandParameter::LocalLoop_SubscriberDeviceID,
		    boost::lexical_cast<std::string>(subscriberDeviceID)));
		    cancelCommand.parameters.push_back(CommandParameter(CommandParameter::LocalLoop_PublisherChannelNo,
		    boost::lexical_cast<std::string>(publisherChannelID)));
	    }
	    processor->commands.CreateCommand(cancelCommand, boost::str(boost::format("system: cause failed CommandID=%1%")
	    % failedPublishSubscribeCommandID));
    }



} //namespace hostapp
} //namespace nisa100
