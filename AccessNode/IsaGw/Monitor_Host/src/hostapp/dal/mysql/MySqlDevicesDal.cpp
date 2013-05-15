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

//added by Cristian.Guef
#ifndef HW_VR900


//added
#include <math.h>
#include <arpa/inet.h>

#include "MySqlDevicesDal.h"
#include <tai.h>

extern int history_status_size;

namespace nisa100 {
namespace hostapp {

MySqlDevicesDal::MySqlDevicesDal(MySQLConnection& connection_) :
	connection(connection_)
{
}

MySqlDevicesDal::~MySqlDevicesDal()
{
}

void MySqlDevicesDal::VerifyTables()
{
	LOG_DEBUG("Verify devices tables structure...");
	{
		std::string
		    query =
		        "SELECT DeviceID, DeviceRole, Address128, Address64, DeviceTag, DeviceStatus, LastRead, DeviceLevel, PublishErrorFlag"
			        " FROM Devices WHERE 0";
		try
		{
			MySQLCommand(connection, query).ExecuteQuery();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}
	{
		std::string
		    query =
		        "SELECT DeviceID, DPDUsTransmitted, DPDUsReceived, DPDUsFailedTransmission, DPDUsFailedReception, "
				" PowerSupplyStatus, Manufacturer, Model, Revision, SubnetID"
			        " FROM DevicesInfo WHERE 0";
		try
		{
			MySQLCommand(connection, query).ExecuteQuery();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}

	{
		std::string
		    query =
		        ""
			        "SELECT DeviceID, ChannelNo, ChannelName, UnitOfMeasurement, ChannelFormat, SourceTSAPID, SourceObjID, SourceAttrID, SourceIndex1, SourceIndex2"
			        " FROM DeviceChannels WHERE 0";
		try
		{
			MySQLCommand(connection, query).ExecuteQuery();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}

	{
		std::string query = ""
			"SELECT FromDeviceID, ToDeviceID, SignalQuality, ClockSource"
			" FROM TopologyLinks WHERE 0";
		try
		{
			MySQLCommand(connection, query).ExecuteQuery();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}

	{
		std::string query = ""
			"SELECT FromDeviceID, ToDeviceID, GraphID"
			" FROM TopologyGraphs WHERE 0";
		try
		{
			MySQLCommand(connection, query).ExecuteQuery();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}

	{
		std::string query = "SELECT DeviceID, ReadingTime, ChannelNo, Value, RawValue, ReadingType, Miliseconds, ValueStatus"
			" FROM DeviceReadings WHERE 0";
		try
		{
			MySQLCommand(connection, query).ExecuteQuery();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}
	{
		std::string query = "SELECT HistoryID, DeviceID, Timestamp, DeviceStatus FROM DeviceHistory WHERE 0";
		try
		{
			MySQLCommand(connection, query).ExecuteQuery();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}
	{
		std::string
		    query =
		        ""
			        "SELECT FirmwareID, FileName, Version, Description, UploadDate, FirmwareType, UploadStatus, UploadRetryCount, LastFailedUploadTime"
			        " FROM Firmwares WHERE 0";
		try
		{
			MySQLCommand(connection, query).ExecuteQuery();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}

	{
		std::string query = ""
			"SELECT DeviceID, IP, Port"
			" FROM DeviceConnections WHERE 0";
		try
		{
			MySQLCommand(connection, query).ExecuteQuery();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}
}

void MySqlDevicesDal::ResetDevices(Device::DeviceStatus newStatus)
{
	LOG_DEBUG("Reset all devices from db");

	//added by Cristian.Guef
	MySQLCommand sqlCommand(connection, ""
		"UPDATE Devices SET DeviceStatus = ?001");
	sqlCommand.BindParam(1, (int)newStatus);
	sqlCommand.ExecuteNonQuery();

	//also history
	MySQLCommand sqlCommand1(connection, ""
		"INSERT INTO DeviceHistory(DeviceID, Timestamp, DeviceStatus)"
		" SELECT DeviceID, datetime('now'), DeviceStatus FROM Devices;");
	sqlCommand1.ExecuteNonQuery();
}

void MySqlDevicesDal::GetDevices(DeviceList& list)
{
	LOG_DEBUG("Get all devices from database");
	MySQLCommand sqlCommand(connection, ""
		"SELECT DeviceID, DeviceRole, Address128, Address64, DeviceTag, DeviceStatus, DeviceLevel"
		" FROM Devices;");

	MySQLResultSet::Ptr rsDevices = sqlCommand.ExecuteQuery();
	list.reserve(list.size() + rsDevices->RowsCount());

	for (MySQLResultSet::Iterator it = rsDevices->Begin(); it != rsDevices->End(); it++)
	{
		list.push_back(Device());
		Device& device = *list.rbegin();
		device.id = it->Value<int>(0);
		device.deviceType = (Device::DeviceType)it->Value<int>(1);
		device.ip = IPv6(it->Value<std::string>(2));
		device.mac = MAC(it->Value<std::string>(3));
		device.m_deviceTAG = it->Value<std::string>(4);
		device.status = (Device::DeviceStatus)it->Value<int>(5);
		device.deviceLevel = it->Value<int>(6);
	}

	//for device info
	for (unsigned int i = 0; i < list.size(); i++)
	{
		MySQLCommand sqlCommand(connection, ""
		"SELECT DPDUsTransmitted, DPDUsReceived, DPDUsFailedTransmission, DPDUsFailedReception,"
			" PowerSupplyStatus"
			" FROM DevicesInfo WHERE DeviceID = ?001");

		sqlCommand.BindParam(1, list[i].id);
		MySQLResultSet::Ptr rsDevices = sqlCommand.ExecuteQuery();
		if (rsDevices->RowsCount())
		{
			MySQLResultSet::Iterator itDevice = rsDevices->Begin();
			list[i].m_DPDUsTransmitted = itDevice->Value<int>(0);
			list[i].m_DPDUsReceived = itDevice->Value<int>(1);
			list[i].m_DPDUsFailedTransmission = itDevice->Value<int>(2);
			list[i].m_DPDUsFailedReception = itDevice->Value<int>(3);
			list[i].powerStatus = itDevice->Value<int>(4);
		}
	}

	//read device history
	for (unsigned int i = 0; i < list.size(); i++)
	{
		MySQLCommand sqlCommand(connection, ""
			"SELECT Timestamp FROM DeviceHistory WHERE DeviceID = ?001");

		sqlCommand.BindParam(1, list[i].id);
		MySQLResultSet::Ptr rsDevices = sqlCommand.ExecuteQuery();
		if (rsDevices->RowsCount())
		{
			MySQLResultSet::Iterator itDevice = rsDevices->Begin();
			list[i].changedStatus.push(itDevice->Value<std::string>(0));
		}
	}

}

void MySqlDevicesDal::DeleteDevice(int id)
{
	{
		MySQLCommand sqlCommand(connection, ""
				"DELETE FROM Devices WHERE DeviceID = ?001");
			sqlCommand.BindParam(1, id);
			sqlCommand.ExecuteNonQuery();
	}
	{
		MySQLCommand sqlCommand(connection, ""
				"DELETE FROM DevicesInfo WHERE DeviceID = ?001");
			sqlCommand.BindParam(1, id);
			sqlCommand.ExecuteNonQuery();
	}
}



//added by Cristian.Guef
bool MySqlDevicesDal::IsDeviceInDB(int deviceID)
{
	LOG_DEBUG("Try to find device with Device ID:" << deviceID);
	MySQLCommand sqlCommand(connection, ""
		"SELECT DeviceID"
		" FROM Devices WHERE DeviceID = ?001");
	sqlCommand.BindParam(1, deviceID);

	MySQLResultSet::Ptr rsDevices = sqlCommand.ExecuteQuery();
	if (rsDevices->RowsCount())
	{
		return true;
	}
	return false;
}

void MySqlDevicesDal::AddDevice(Device& device)
{
	LOG_DEBUG("Add device with MAC:" << device.mac.ToString());
	
	//create device
	MySQLCommand
		sqlCommand(
			connection,
			"INSERT INTO Devices(DeviceRole, Address128, DeviceTag, Address64, DeviceStatus, DeviceLevel, RejoinCount)"
				" VALUES(?001, ?002, ?003, ?004, ?005, ?006, ?007);");
	sqlCommand.BindParam(1, (int)device.deviceType);
	sqlCommand.BindParam(2, device.ip.ToString());
	sqlCommand.BindParam(3, device.GetTAG());
	sqlCommand.BindParam(4, device.mac.ToString());
	sqlCommand.BindParam(5, (int)device.status);
	sqlCommand.BindParam(6, device.deviceLevel);
	sqlCommand.BindParam(7, device.rejoinCount);
	sqlCommand.ExecuteNonQuery();
	device.id = sqlCommand.GetLastInsertRowID();

	if (device.Type() == 1/*sm*/ || device.Type() == 2/*gw*/)
	{
		//create device info
		MySQLCommand
			sqlCommand(
				connection,
				"INSERT INTO DevicesInfo(DeviceID, DPDUsTransmitted, DPDUsReceived, DPDUsFailedTransmission, DPDUsFailedReception, PowerSupplyStatus, "
					" Manufacturer, Model, Revision, SerialNo)"
					" VALUES(?001, ?002, ?003, ?004, ?005, ?006, ?007, ?008, ?009, ?010);");
		sqlCommand.BindParam(1, (int)device.id);
		sqlCommand.BindParam(2, (int)device.GetDPDUsTransmitted());
		sqlCommand.BindParam(3, (int)device.GetDPDUsReceived());
		sqlCommand.BindParam(4, (int)device.GetDPDUsFailedTransmission());
		sqlCommand.BindParam(5, (int)device.GetDPDUsFailedReception());
		sqlCommand.BindParam(6, (int)device.powerStatus);
		sqlCommand.BindParam(7, device.GetManufacturer());
		sqlCommand.BindParam(8, device.GetModel());
		sqlCommand.BindParam(9, device.GetRevision());
		sqlCommand.BindParam(10, device.GetSerialNo());
		sqlCommand.ExecuteNonQuery();
	}
	else
	{
		//create device info
		MySQLCommand
			sqlCommand(
				connection,
				"INSERT INTO DevicesInfo(DeviceID, DPDUsTransmitted, DPDUsReceived, DPDUsFailedTransmission, DPDUsFailedReception, PowerSupplyStatus, "
					" Manufacturer, Model, Revision, SerialNo, SubnetID)"
					" VALUES(?001, ?002, ?003, ?004, ?005, ?006, ?007, ?008, ?009, ?010, ?011);");
		sqlCommand.BindParam(1, (int)device.id);
		sqlCommand.BindParam(2, (int)device.GetDPDUsTransmitted());
		sqlCommand.BindParam(3, (int)device.GetDPDUsReceived());
		sqlCommand.BindParam(4, (int)device.GetDPDUsFailedTransmission());
		sqlCommand.BindParam(5, (int)device.GetDPDUsFailedReception());
		sqlCommand.BindParam(6, (int)device.powerStatus);
		sqlCommand.BindParam(7, device.GetManufacturer());
		sqlCommand.BindParam(8, device.GetModel());
		sqlCommand.BindParam(9, device.GetRevision());
		sqlCommand.BindParam(10, device.GetSerialNo());
		sqlCommand.BindParam(11, device.GetSubnetID());
		sqlCommand.ExecuteNonQuery();
	}
	

	{ //update history
		MySQLCommand sqlCommand(connection, ""
			"INSERT INTO DeviceHistory(DeviceID, Timestamp, DeviceStatus)"
			" VALUES (?001, ?002, ?003)");

		sqlCommand.BindParam(1, device.id);
		sqlCommand.BindParam(2, nlib::CurrentUniversalTime());
		sqlCommand.BindParam(3, (int)device.status);
		sqlCommand.ExecuteNonQuery();
	}
}


void MySqlDevicesDal::UpdateDevice(Device& device)
{
	LOG_DEBUG("Update device:" << device.ToString());
	{
		//create history if status changed
		// this should be called before status updated ., :)
		if (device.IsStatusChanged())
		{
			MySQLCommand sqlCommand(connection, ""
				"INSERT INTO DeviceHistory(DeviceID, Timestamp, DeviceStatus)"
				" VALUES (?001, ?002, ?003)");
			sqlCommand.BindParam(1, device.id);
			sqlCommand.BindParam(2, device.changedStatus.back());
			sqlCommand.BindParam(3, (int)device.status);
			sqlCommand.ExecuteNonQuery();

			while (device.changedStatus.size() > history_status_size)
			{
				MySQLCommand sqlCommand(connection, ""
					"DELETE FROM DeviceHistory"
					" WHERE DeviceID = ?001 AND Timestamp = ?002");
				sqlCommand.BindParam(1, device.id);
				sqlCommand.BindParam(2, device.changedStatus.front());
				sqlCommand.ExecuteNonQuery();
				LOG_DEBUG("HISTORY device_status =" << device.changedStatus.front() << " was removed");
				device.changedStatus.pop();
			}
		}
	}

	//update device
	MySQLCommand
		sqlCommand(
			connection,
			"UPDATE Devices SET" 
				" DeviceRole = ?001, Address128 = ?002, DeviceTag = ?003, Address64 = ?004,"
				" DeviceStatus = ?005, DeviceLevel = ?006, RejoinCount = ?007"
				" WHERE DeviceID = ?008");
	sqlCommand.BindParam(1, (int)device.deviceType);
	sqlCommand.BindParam(2, device.ip.ToString());
	sqlCommand.BindParam(3, device.GetTAG());
	sqlCommand.BindParam(4, device.mac.ToString());
	sqlCommand.BindParam(5, (int)device.status);
	sqlCommand.BindParam(6, device.deviceLevel);
	sqlCommand.BindParam(7, device.rejoinCount);
	sqlCommand.BindParam(8, device.id);
	sqlCommand.ExecuteNonQuery();
	
	if (device.Type() == 1/*sm*/ || device.Type() == 2/*gw*/)
	{
		//update device info
		MySQLCommand
			sqlCommand(
				connection,
				"UPDATE DevicesInfo SET"
					" DPDUsTransmitted = ?001, DPDUsReceived = ?002, "
					" DPDUsFailedTransmission = ?003, DPDUsFailedReception = ?004, PowerSupplyStatus = ?005, "
					" Manufacturer = ?006, Model = ?007, Revision = ?008, SerialNo = ?009"
					" WHERE DeviceID = ?010");
		sqlCommand.BindParam(1, (int)device.GetDPDUsTransmitted());
		sqlCommand.BindParam(2, (int)device.GetDPDUsReceived());
		sqlCommand.BindParam(3, (int)device.GetDPDUsFailedTransmission());
		sqlCommand.BindParam(4, (int)device.GetDPDUsFailedReception());
		sqlCommand.BindParam(5, (int)device.powerStatus);
		sqlCommand.BindParam(6, device.GetManufacturer());
		sqlCommand.BindParam(7, device.GetModel());
		sqlCommand.BindParam(8, device.GetRevision());
		sqlCommand.BindParam(9, device.GetSerialNo());
		sqlCommand.BindParam(10, device.id);
		sqlCommand.ExecuteNonQuery();
	}
	else
	{
		//update device info
		MySQLCommand
			sqlCommand(
				connection,
				"UPDATE DevicesInfo SET"
					" DPDUsTransmitted = ?001, DPDUsReceived = ?002, "
					" DPDUsFailedTransmission = ?003, DPDUsFailedReception = ?004, PowerSupplyStatus = ?005, "
					" Manufacturer = ?006, Model = ?007, Revision = ?008, SerialNo = ?009, SubnetID = ?010"
					" WHERE DeviceID = ?011");
		sqlCommand.BindParam(1, (int)device.GetDPDUsTransmitted());
		sqlCommand.BindParam(2, (int)device.GetDPDUsReceived());
		sqlCommand.BindParam(3, (int)device.GetDPDUsFailedTransmission());
		sqlCommand.BindParam(4, (int)device.GetDPDUsFailedReception());
		sqlCommand.BindParam(5, (int)device.powerStatus);
		sqlCommand.BindParam(6, device.GetManufacturer());
		sqlCommand.BindParam(7, device.GetModel());
		sqlCommand.BindParam(8, device.GetRevision());
		sqlCommand.BindParam(9, device.GetSerialNo());
		sqlCommand.BindParam(10, device.GetSubnetID());
		sqlCommand.BindParam(11, device.id);
		sqlCommand.ExecuteNonQuery();
	}
}


static const char* gettime(const struct timeval &tv)
{
  static char szTime[50];
  static struct tm * timeinfo;

  timeinfo = gmtime(&tv.tv_sec);
  sprintf(szTime, "%04d-%02d-%02d %02d:%02d:%02d", timeinfo->tm_year+1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, 
											timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec); 
  return szTime;
}

//added by Cristian.Guef
void MySqlDevicesDal::UpdateReadingTimeforDevice(int deviceID, const struct timeval &tv)
{
	//update time for device_id
	/*
	MySQLCommand sqlCommand(connection, ""
		"UPDATE Devices SET"
		" LastRead = ?001"
		" WHERE DeviceID = ?002");

	sqlCommand.BindParam(1, readingTime);
	sqlCommand.BindParam(2, deviceID);
	*/
	char sqlCmd[500];
	sprintf(sqlCmd, "UPDATE Devices SET LastRead = '%s' WHERE DeviceID = %d", gettime(tv), deviceID);
	MySQLCommand sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();
}

void MySqlDevicesDal::CleanDeviceNeighbours()
{
	MySQLCommand sqlCommand(connection, ""
		"DELETE FROM TopologyLinks");
	sqlCommand.ExecuteNonQuery();
}

/* commented by Cristian.Guef
void MySqlDevicesDal::CreateDeviceNeighbour(int fromDevice, int toDevice, int signalQuality)
{
	//LOG_DEBUG("Create neighbour for the device:" << device.mac.ToString());
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO TopologyLinks(FromDeviceID, ToDeviceID, SignalQuality)"
		" VALUES (?001, ?002, ?003)");

	sqlCommand.BindParam(1, fromDevice);
	sqlCommand.BindParam(2, toDevice);
	sqlCommand.BindParam(3, signalQuality);
	sqlCommand.ExecuteNonQuery();
}
*/
//added by Cristian.Guef
void MySqlDevicesDal::CreateDeviceNeighbour(int fromDevice, int toDevice, int signalQuality, int clockSource)
{
	//LOG_DEBUG("Create neighbour for the device:" << device.mac.ToString());
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO TopologyLinks(FromDeviceID, ToDeviceID, SignalQuality, ClockSource)"
		" VALUES (?001, ?002, ?003, ?004)");

	sqlCommand.BindParam(1, fromDevice);
	sqlCommand.BindParam(2, toDevice);
	sqlCommand.BindParam(3, signalQuality);
	sqlCommand.BindParam(4, clockSource);
	sqlCommand.ExecuteNonQuery();
}

void MySqlDevicesDal::CleanDeviceGraphs()
{
	MySQLCommand sqlCommand(connection, ""
		"DELETE FROM TopologyGraphs");
	sqlCommand.ExecuteNonQuery();
}

void MySqlDevicesDal::CreateDeviceGraph(int fromDevice, int toDevice, int graphID)
{
	//LOG_DEBUG("Create neighbour for the device:" << device.mac.ToString());
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO TopologyGraphs(FromDeviceID, ToDeviceID, GraphID)"
		" VALUES (?001, ?002, ?003)");

	sqlCommand.BindParam(1, fromDevice);
	sqlCommand.BindParam(2, toDevice);
	sqlCommand.BindParam(3, graphID);
	sqlCommand.ExecuteNonQuery();
}

inline void gettime(nlib::DateTime &ReadingTime, short &milisec, const struct timeval &tv)
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


void MySqlDevicesDal::AddReading(const DeviceReading& reading, bool p_bHistory)
{
	LOG_DEBUG("Add reading to the device:" << reading.deviceID);

	/* commented by Cristian.Guef
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO DeviceReadings(DeviceID, ReadingTime, ChannelNo, Value, RawValue, ReadingType)"
		" VALUES (?001, ?002, ?003, ?004, ?005, ?006)");
		
	sqlCommand.BindParam(1, reading.deviceID);
	sqlCommand.BindParam(2, reading.readingTime);
	sqlCommand.BindParam(3, (int)reading.channel.channelNumber);
	sqlCommand.BindParam(4, reading.value);
	sqlCommand.BindParam(5, reading.rawValue.GetDoubleValue());
	sqlCommand.BindParam(6, (int)reading.readingType);
	sqlCommand.BindParam(7, (int)reading.milisec);
	sqlCommand.ExecuteNonQuery();
		*/

#ifndef __CYGWIN
	if (isnan(reading.rawValue.GetDoubleValue()))
	{
		double val = reading.rawValue.GetDoubleValue();
		unsigned char *puc = (unsigned char*)&val;
		std::string str;
		for(int i = 0; i < (int)sizeof(double); i++)
		{
			char hex[20];
			sprintf(hex, "%02x", puc[i]);
			str += hex;
		}
		LOG_ERROR("NAN is detected in published data from dev_id = " << reading.deviceID << "for channelNo = " << (int)reading.channelNo << "hex value = " << str);
		return;
	}
#endif
	
	//added
	//static nlib::DateTime ReadingTime;
	//static short milisec;
	//gettime(ReadingTime, milisec, reading.tv);

	//added by Cristian.Guef
	/*
	if (reading.IsISA == true)
	{
		MySQLCommand sqlCommand(connection, ""
			"INSERT INTO DeviceReadings(DeviceID, ReadingTime, ChannelNo, Value, RawValue, ReadingType, Miliseconds, ValueStatus)"
			" VALUES (?001, ?002, ?003, ?004, ?005, ?006, ?007, ?008)");

		sqlCommand.BindParam(1, reading.deviceID);
		sqlCommand.BindParam(2, ReadingTime);
		sqlCommand.BindParam(3, (int)reading.channelNo);
		sqlCommand.BindParam(4, reading.rawValue.ToString());
		sqlCommand.BindParam(5, reading.rawValue.GetDoubleValue());
		sqlCommand.BindParam(6, (int)reading.readingType);
		sqlCommand.BindParam(7, (int)milisec);
		sqlCommand.BindParam(8, (int)reading.ValueStatus);
		sqlCommand.ExecuteNonQuery();
	}
	else
	{
		MySQLCommand sqlCommand(connection, ""
			"INSERT INTO DeviceReadings(DeviceID, ReadingTime, ChannelNo, Value, RawValue, ReadingType, Miliseconds)"
			" VALUES (?001, ?002, ?003, ?004, ?005, ?006, ?007)");

		sqlCommand.BindParam(1, reading.deviceID);
		sqlCommand.BindParam(2, ReadingTime);
		sqlCommand.BindParam(3, (int)reading.channelNo);
		sqlCommand.BindParam(4, reading.rawValue.ToString());
		sqlCommand.BindParam(5, reading.rawValue.GetDoubleValue());
		sqlCommand.BindParam(6, (int)reading.readingType);
		sqlCommand.BindParam(7, (int)milisec);
		sqlCommand.ExecuteNonQuery();
	}
	*/
	
	char sqlCmd[1000];
	if (reading.IsISA == true)
	{
		sprintf(sqlCmd, "INSERT INTO DeviceReadingsHistory(DeviceID, ReadingTime, ChannelNo, Value, RawValue, ReadingType, Miliseconds, ValueStatus) \
					   VALUES (%d, '%s', '%d', '%s', %f, %d, %d, %d);", reading.deviceID, gettime(reading.tv), (int)reading.channelNo,
					   reading.rawValue.ToString().c_str(), reading.rawValue.GetFloatValue(), (int)reading.readingType, (int)(reading.tv.tv_usec/1000), (int)reading.ValueStatus);
		MySQLCommand sqlCommand(connection, sqlCmd);
		sqlCommand.ExecuteNonQuery();
	}
	else
	{
		sprintf(sqlCmd, "INSERT INTO DeviceReadingsHistory(DeviceID, ReadingTime, ChannelNo, Value, RawValue, ReadingType, Miliseconds) \
					   VALUES (%d, '%s', '%d', '%s', %f, %d, %d);", reading.deviceID, gettime(reading.tv), (int)reading.channelNo,
					   reading.rawValue.ToString().c_str(), reading.rawValue.GetFloatValue(), (int)reading.readingType, (int)(reading.tv.tv_usec/1000));
		MySQLCommand sqlCommand(connection, sqlCmd);
		sqlCommand.ExecuteNonQuery();
	}
	
}

void MySqlDevicesDal::AddEmptyReadings(int DeviceID)
{
	char sqlCmd[1000];
	sprintf(sqlCmd, "INSERT INTO DeviceReadings(DeviceID, ReadingTime, ChannelNo, Value, RawValue, ReadingType, Miliseconds) \
					 SELECT DeviceID, '1970-01-01 00:00:00', ChannelNo, 0, '0', -1, 0 FROM DeviceChannels Where DeviceID=%d;", DeviceID);
	MySQLCommand sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();
}

void MySqlDevicesDal::AddEmptyReading(int DeviceID, int channelNo)
{
	char sqlCmd[1000];
	sprintf(sqlCmd, "INSERT INTO DeviceReadings(DeviceID, ReadingTime, ChannelNo, Value, RawValue, ReadingType, Miliseconds) \
					 SELECT %d, '1970-01-01 00:00:00', %d, 0, '0', -1, 0;", DeviceID, channelNo);
	MySQLCommand sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();
}

void MySqlDevicesDal::UpdateReading(const DeviceReading& reading)
{
	char sqlCmd[4 * 1024];
	if (reading.IsISA == true)
	{
		sprintf(sqlCmd, "UPDATE DeviceReadings SET ReadingTime='%s', Value='%s', RawValue=%d, ReadingType=%d, Miliseconds=%d, ValueStatus=%d" 
						" WHERE DeviceID=%d AND ChannelNo=%d;"
						, gettime(reading.tv), reading.rawValue.ToString().c_str(), htonl(reading.rawValue.GetIntValue()), (int)reading.readingType, (int)(reading.tv.tv_usec/1000), 
						(int)reading.ValueStatus, reading.deviceID, (int)reading.channelNo);
	}
	else
	{
		sprintf(sqlCmd, "UPDATE DeviceReadings SET ReadingTime='%s', Value='%s', RawValue=%d, ReadingType=%d, Miliseconds=%d" 
			" WHERE DeviceID=%d AND ChannelNo=%d;"
			, gettime(reading.tv), reading.rawValue.ToString().c_str(), htonl(reading.rawValue.GetIntValue()), (int)reading.readingType, (int)(reading.tv.tv_usec/1000), 
			reading.deviceID, (int)reading.channelNo);
	}

	MySQLCommand sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();

	//sqlitexx::Command::ExecuteNonQuery(connection.GetRawDbObj(), sqlCmd); 	
}

void MySqlDevicesDal::DeleteReading(int p_nChannelNo)
{
	char sqlCmd[500];
	sprintf(sqlCmd, "DELETE from DeviceReadings WHERE ChannelNo=%d;", p_nChannelNo);

	MySQLCommand sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();

	//sqlitexx::Command::ExecuteNonQuery(connection.GetRawDbObj(), sqlCmd); 
}
void MySqlDevicesDal::DeleteReadings(int deviceID)
{
	char sqlCmd[500];
	sprintf(sqlCmd, "DELETE from DeviceReadings WHERE DeviceID=%d;", deviceID);

	MySQLCommand sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();
}
bool MySqlDevicesDal::IsDeviceChannelInReading(int deviceID, int channelNo)
{
	MySQLCommand
	    sqlCommand(
	        connection,
	        "SELECT ChannelNo "
				"FROM DeviceReadings "
				"WHERE DeviceID = ?001 AND ChannelNo = ?002;" );

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, channelNo);
		
	MySQLResultSet::Ptr res = sqlCommand.ExecuteQuery();
	if (res->RowsCount()> 0)
		return true;
	return false;
}

//added
void MySqlDevicesDal::UpdatePublishFlag(int devID, int flag/*0-no data, 1-fresh data, 2-stale data*/)
{
	/*
	MySQLCommand sqlCommand(connection, ""
		"UPDATE Devices SET"
		" PublishStatus = ?001"
		" WHERE DeviceID = ?002");

	sqlCommand.BindParam(1, flag);
	sqlCommand.BindParam(2, devID);
	*/

	char sqlCmd[500];
	sprintf(sqlCmd, "UPDATE Devices SET PublishStatus = %d WHERE DeviceID = %d", flag, devID);
	MySQLCommand sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();
}

//added by Cristian.Guef
void MySqlDevicesDal::DeleteContracts(int deviceID)
{
	MySQLCommand sqlCommand(connection, ""
		"DELETE FROM Contracts WHERE SourceDeviceID = ?001;");
	sqlCommand.BindParam(1, deviceID);
	sqlCommand.ExecuteNonQuery();

}
void MySqlDevicesDal::DeleteContracts()
{
	MySQLCommand sqlCommand(connection, ""
		"DELETE FROM Contracts;");
	sqlCommand.ExecuteNonQuery();
}
void MySqlDevicesDal::AddContract(int fromDevice, int toDevice, const ContractsAndRoutes::Contract &contract)
{
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO Contracts(ContractID, ServiceType, ActivationTime, SourceDeviceID, "
							"SourceSAP, DestinationDeviceID, DestinationSAP, ExpirationTime, Priority, "
							"NSDUSize, Reliability, Period, Phase, ComittedBurst, ExcessBurst, Deadline, MaxSendWindow)"
		" VALUES(?001, ?002, ?003, ?004, ?005, "
				"?006, ?007, ?008, ?009, ?010, "
				"?011, ?012, ?013, ?014, ?015, "
				"?016, ?017);");

	char szActivation [ 256 ], szExpiration[ 256 ];
	const char *pszActivationTime = szActivation, *pszExpirationTime = szExpiration;
	
	if(contract.activationTime)
	{	time_t nActivation = contract.activationTime - (TAI_OFFSET + CurrentUTCAdjustment);	// Convert TAI to UTC
		struct tm * pTm = gmtime( &nActivation );
		snprintf( szActivation, sizeof(szActivation), "%4d-%02d-%02d %02d:%02d:%02d", pTm->tm_year+1900,
			pTm->tm_mon+1,pTm->tm_mday, pTm->tm_hour,pTm->tm_min,pTm->tm_sec);
		szActivation[ sizeof(szActivation)-1 ] = 0;
	}
	else
		pszActivationTime = "IMMEDIATE";
	
	if( contract.expirationTime && (-1 != (int)contract.expirationTime) )
	{	///TODO: take care of contract.activationTime == 0. Is this really possible?
		time_t nExpiration =  contract.activationTime - (TAI_OFFSET + CurrentUTCAdjustment) + contract.expirationTime;	// Convert TAI to UTC
		struct tm * pTm = gmtime( &nExpiration );

		snprintf( szActivation, sizeof(szActivation), "%4d-%02d-%02d %02d:%02d:%02d", pTm->tm_year+1900,
			pTm->tm_mon+1,pTm->tm_mday, pTm->tm_hour,pTm->tm_min,pTm->tm_sec);
		szExpiration[ sizeof(szExpiration)-1 ] = 0;
	}
	else
		pszExpirationTime = "NEVER";
	
	sqlCommand.BindParam(1, (int)contract.contractID);
	sqlCommand.BindParam(2, (int)contract.serviceType);
	sqlCommand.BindParam(3, pszActivationTime);
	sqlCommand.BindParam(4, (int)fromDevice);
	sqlCommand.BindParam(5, (int)contract.sourceSAP);
	sqlCommand.BindParam(6, (int)toDevice);
	sqlCommand.BindParam(7, (int)contract.destinationSAP);
	sqlCommand.BindParam(8, pszExpirationTime);
	sqlCommand.BindParam(9, (int)contract.priority);
	sqlCommand.BindParam(10, (int)contract.NSDUSize);
	sqlCommand.BindParam(11, (int)contract.reliability);
	sqlCommand.BindParam(12, (int)contract.period);
	sqlCommand.BindParam(13, (int)contract.phase);
	sqlCommand.BindParam(14, (int)contract.comittedBurst);
	sqlCommand.BindParam(15, (int)contract.excessBurst);
	sqlCommand.BindParam(16, (int)contract.deadline);
	sqlCommand.BindParam(17, (int)contract.maxSendWindow);
	sqlCommand.ExecuteNonQuery();
}


//added by Cristian.Guef
void MySqlDevicesDal::DeleteRoutes(int deviceID)
{
	MySQLCommand sqlCommand(connection, ""
		"DELETE FROM RoutesInfo WHERE DeviceID = ?001;");
	sqlCommand.BindParam(1, deviceID);
	sqlCommand.ExecuteNonQuery();

	MySQLCommand sqlCommand2(connection, ""
		"DELETE FROM RouteLinks WHERE DeviceID = ?001;");
	sqlCommand2.BindParam(1, deviceID);
	sqlCommand2.ExecuteNonQuery();
}

static char * SQLs[] = {"INSERT INTO RoutesInfo(DeviceID, RouteID, Alternative, FowardLimit, Selector, SrcAddr) VALUES(?001, ?002, ?003, ?004, ?005, ?006);", 
						"INSERT INTO RoutesInfo(DeviceID, RouteID, Alternative, FowardLimit, Selector) VALUES(?001, ?002, ?003, ?004, ?005);",
						"INSERT INTO RoutesInfo(DeviceID, RouteID, Alternative, FowardLimit, Selector) VALUES(?001, ?002, ?003, ?004, ?005);",
						"INSERT INTO RoutesInfo(DeviceID, RouteID, Alternative, FowardLimit) VALUES(?001, ?002, ?003, ?004);"};
void MySqlDevicesDal::AddRouteInfo(int deviceID, int routeID, int alternative, int fowardLimit, int selector, int srcAddr)
{
	
	if (alternative < 0 || alternative > 3)
	{
		LOG_ERROR("invalid alternative");
		return;
	}

	MySQLCommand sqlCommand(connection, SQLs[alternative]);

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, routeID);
	sqlCommand.BindParam(3, alternative);
	sqlCommand.BindParam(4, fowardLimit);

	switch(alternative)
	{
	case 0:
		sqlCommand.BindParam(5, selector);
		sqlCommand.BindParam(6, srcAddr);
		break;
	case 1:
		sqlCommand.BindParam(5, selector);
		break;
	case 2:
		sqlCommand.BindParam(5, selector);
		break;
	default:
		break;
	}
	
	sqlCommand.ExecuteNonQuery();
}

void MySqlDevicesDal::AddRouteLink(int deviceID, int routeID, int routeIndex, int graphID)
{
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO RouteLinks(DeviceID, RouteID, RouteIndex, GraphID)"
		" VALUES(?001, ?002, ?003, ?004);");

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, routeID);
	sqlCommand.BindParam(3, routeIndex);
	sqlCommand.BindParam(4, graphID);
	sqlCommand.ExecuteNonQuery();
}
void MySqlDevicesDal::AddRouteLink(int deviceID, int routeID, int routeIndex, const DevicePtr neighbour)
{
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO RouteLinks(DeviceID, RouteID, RouteIndex, NeighbourID)"
		" VALUES(?001, ?002, ?003, ?004);");

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, routeID);
	sqlCommand.BindParam(3, routeIndex);
	sqlCommand.BindParam(4, neighbour->id);
	sqlCommand.ExecuteNonQuery();
}
void MySqlDevicesDal::RemoveContractElements(int sourceID)
{
	MySQLCommand sqlCommand(connection, ""
		"DELETE FROM ContractElements WHERE SourceDeviceID = ?001;");
	sqlCommand.BindParam(1, sourceID);
	sqlCommand.ExecuteNonQuery();
}
void MySqlDevicesDal::RemoveContractElements()
{
	MySQLCommand sqlCommand(connection, ""
		"DELETE FROM ContractElements;");
	sqlCommand.ExecuteNonQuery();
}
void MySqlDevicesDal::AddContractElement(int contractID, int sourceID, int index, int deviceID)
{
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO ContractElements(ContractID, SourceDeviceID, Idx, DeviceID)"
		" VALUES(?001, ?002, ?003, ?004);");

	sqlCommand.BindParam(1, contractID);
	sqlCommand.BindParam(2, sourceID);
	sqlCommand.BindParam(3, index);
	sqlCommand.BindParam(4, deviceID);
	sqlCommand.ExecuteNonQuery();
}

//added by Cristian.Guef
void MySqlDevicesDal::SaveISACSConfirm(int deviceID, ISACSInfo &confirm)
{
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO ISACSConfirmDataBuffer(DeviceID, Timestamp, Miliseconds, TSAPID, RequestType,"
						" ObjectID, ObjResourceID, AttrIndex1, AttrIndex2, DataBuffer)"
						" VALUES(?001, ?002, ?003, ?004, ?005,"
								"?006, ?007, ?008, ?009, ?010);");

	//added
	nlib::DateTime ReadingTime;
	short milisec;
	gettime(ReadingTime, milisec, confirm.tv);

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, ReadingTime);
	sqlCommand.BindParam(3, milisec);
	sqlCommand.BindParam(4, confirm.m_tsapID);
	sqlCommand.BindParam(5, confirm.m_reqType);
	sqlCommand.BindParam(6, confirm.m_objID);
	sqlCommand.BindParam(7, confirm.m_objResID);
	sqlCommand.BindParam(8, confirm.m_attrIndex1);
	sqlCommand.BindParam(9, confirm.m_attrIndex2);
	sqlCommand.BindParam(10, confirm.m_strRespDataBuff);
	sqlCommand.ExecuteNonQuery();
}

//added by Cristian.Guef
void MySqlDevicesDal::AddRFChannel(const GScheduleReport::Channel &channel)
{
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO RFChannels(ChannelNumber, ChannelStatus)"
						" VALUES(?001, ?002);");

	sqlCommand.BindParam(1, (int)channel.channelNumber);
	sqlCommand.BindParam(2, (int)channel.channelStatus);
	sqlCommand.ExecuteNonQuery();
}

//added by Cristian.Guef
void MySqlDevicesDal::AddScheduleSuperframe(int deviceID, const GScheduleReport::Superframe &superFrame)
{
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO DeviceScheduleSuperframes(DeviceID, SuperframeID, NumberOfTimeSlots, StartTime, NumberOfLinks)"
						" VALUES(?001, ?002, ?003, ?004, ?005);");

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, (int)superFrame.superframeID);
	sqlCommand.BindParam(3, (int)superFrame.timeSlotsCount);
	sqlCommand.BindParam(4, superFrame.startTime);
	sqlCommand.BindParam(5, (int)superFrame.linkList.size());
	sqlCommand.ExecuteNonQuery();
}

//added by Cristian.guef
void MySqlDevicesDal::AddScheduleLink(int deviceID, int superframeID, const GScheduleReport::Link &link)
{
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO DeviceScheduleLinks(DeviceID, SuperframeID, NeighborDeviceID, SlotIndex, LinkPeriod, SlotLength,"
						"ChannelNumber, Direction, LinkType)"
						" VALUES(?001, ?002, ?003, ?004, ?005,"
								"?006, ?007, ?008, ?009);");

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, superframeID);
	sqlCommand.BindParam(3, link.devDB_ID);
	sqlCommand.BindParam(4, (int)link.slotIndex);
	sqlCommand.BindParam(5, (int)link.linkPeriod);
	sqlCommand.BindParam(6, (int)link.slotLength);
	sqlCommand.BindParam(7, (int)link.channelNumber);
	sqlCommand.BindParam(8, (int)link.direction);
	sqlCommand.BindParam(9, (int)link.linkType);
	sqlCommand.ExecuteNonQuery();
}

//added by Cristian.Guef
void MySqlDevicesDal::RemoveRFChannels()
{
	MySQLCommand sqlCommand(connection, "DELETE FROM RFChannels;");
	sqlCommand.ExecuteNonQuery();
}
//added by Cristian.Guef
void MySqlDevicesDal::RemoveScheduleSuperframes()
{
	MySQLCommand sqlCommand(connection, "DELETE FROM DeviceScheduleSuperframes;");
	sqlCommand.ExecuteNonQuery();
}
//added by Cristian.Guef
void MySqlDevicesDal::RemoveScheduleLinks()
{
	MySQLCommand sqlCommand(connection, "DELETE FROM DeviceScheduleLinks;");
	sqlCommand.ExecuteNonQuery();
}
void MySqlDevicesDal::RemoveScheduleSuperframes(int deviceID)
{
	MySQLCommand sqlCommand(connection, "DELETE FROM DeviceScheduleSuperframes WHERE DeviceID = ?001;");
	sqlCommand.BindParam(1, deviceID);
	sqlCommand.ExecuteNonQuery();
}
void MySqlDevicesDal::RemoveScheduleLinks(int deviceID)
{
	MySQLCommand sqlCommand(connection, "DELETE FROM DeviceScheduleLinks WHERE DeviceID = ?001;");
	sqlCommand.BindParam(1, deviceID);
	sqlCommand.ExecuteNonQuery();
}


//added by Cristian.Guef
void MySqlDevicesDal::AddNetworkHealthInfo(const GNetworkHealthReport::NetworkHealth &netHealth)
{
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO NetworkHealth(NetworkID, NetworkType, DeviceCount, StartDate, CurrentDate,"
						"DPDUsSent, DPDUsLost, GPDULatency, GPDUPathReliability, GPDUDataReliability, JoinCount)"
						" VALUES(?001, ?002, ?003, ?004, ?005,"
								"?006, ?007, ?008,"
								"?009, ?010, ?011);");

	sqlCommand.BindParam(1, (int)netHealth.networkID);
	sqlCommand.BindParam(2, (int)netHealth.networkType);
	sqlCommand.BindParam(3, (int)netHealth.deviceCount);
	sqlCommand.BindParam(4, netHealth.startDate);
	sqlCommand.BindParam(5, netHealth.currentDate);
	sqlCommand.BindParam(6, (int)netHealth.DPDUsSent);
	sqlCommand.BindParam(7, (int)netHealth.DPDUsLost);
	sqlCommand.BindParam(8, (int)netHealth.GPDULatency);
	sqlCommand.BindParam(9, (int)netHealth.GPDUPathReliability);
	sqlCommand.BindParam(10, (int)netHealth.GPDUDataReliability);
	sqlCommand.BindParam(11, (int)netHealth.joinCount);
	sqlCommand.ExecuteNonQuery();
}
void MySqlDevicesDal::AddNetHealthDevInfo(const GNetworkHealthReport::NetDeviceHealth &devHealth)
{
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO NetworkHealthDevices(DeviceID, StartDate, CurrentDate, DPDUsSent, DPDUsLost,"
						"GPDULatency, GPDUPathReliability, GPDUDataReliability, JoinCount)"
						" VALUES(?001, ?002, ?003, ?004, ?005,"
								"?006, ?007, ?008, ?009);");

	sqlCommand.BindParam(1, (int)devHealth.dbDevID);
	sqlCommand.BindParam(2, devHealth.startDate);
	sqlCommand.BindParam(3, devHealth.currentDate);
	sqlCommand.BindParam(4, (int)devHealth.DPDUsSent);
	sqlCommand.BindParam(5, (int)devHealth.DPDUsLost);
	sqlCommand.BindParam(6, (int)devHealth.GPDULatency);
	sqlCommand.BindParam(7, (int)devHealth.GPDUPathReliability);
	sqlCommand.BindParam(8, (int)devHealth.GPDUDataReliability);
	sqlCommand.BindParam(9, (int)devHealth.joinCount);
	sqlCommand.ExecuteNonQuery();
}
void MySqlDevicesDal::RemoveNetworkHealthInfo()
{
	MySQLCommand sqlCommand(connection, "DELETE FROM NetworkHealth;");
	sqlCommand.ExecuteNonQuery();
}
void MySqlDevicesDal::RemoveNetHealthDevInfo()
{
	MySQLCommand sqlCommand(connection, "DELETE FROM NetworkHealthDevices;");
	sqlCommand.ExecuteNonQuery();
}

//added by Cristian.Guef
void MySqlDevicesDal::AddDeviceHealthHistory(int deviceID, const nlib::DateTime &timestamp, const GDeviceHealthReport::DeviceHealth &devHeath)
{
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO DeviceHealthHistory(DeviceID, Timestamp, DPDUsTransmitted, DPDUsReceived,"
						"DPDUsFailedTransmission, DPDUsFailedReception)"
						" VALUES(?001, ?002, ?003, ?004, ?005,"
								"?006);");

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, timestamp);
	sqlCommand.BindParam(3, (int)devHeath.DPDUsTransmitted);
	sqlCommand.BindParam(4, (int)devHeath.DPDUsReceived);
	sqlCommand.BindParam(5, (int)devHeath.DPDUsFailedTransmission);
	sqlCommand.BindParam(6, (int)devHeath.DPDUsFailedReception);
	sqlCommand.ExecuteNonQuery();
}

void MySqlDevicesDal::UpdateDeviceHealth(int deviceID, const GDeviceHealthReport::DeviceHealth &devHeath)
{
	MySQLCommand
			sqlCommand(
				connection,
				"UPDATE DevicesInfo SET"
					" DPDUsTransmitted = ?001, DPDUsReceived = ?002, "
					" DPDUsFailedTransmission = ?003, DPDUsFailedReception = ?004"
					" WHERE DeviceID = ?005");
		sqlCommand.BindParam(1, (int)devHeath.DPDUsTransmitted);
		sqlCommand.BindParam(2, (int)devHeath.DPDUsReceived);
		sqlCommand.BindParam(3, (int)devHeath.DPDUsFailedTransmission);
		sqlCommand.BindParam(4, (int)devHeath.DPDUsFailedReception);
		sqlCommand.BindParam(5, deviceID);
		sqlCommand.ExecuteNonQuery();
}

void MySqlDevicesDal::AddNeighbourHealthHistory(int deviceID, const nlib::DateTime &timestamp, int neighbID, const GNeighbourHealthReport::NeighbourHealth &neighbHealth)
{
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO NeighborHealthHistory(DeviceID, Timestamp, NeighborDeviceID, LinkStatus,"
						"DPDUsTransmitted, DPDUsReceived, DPDUsFailedTransmission, DPDUsFailedReception, SignalStrength, SignalQuality)"
						" VALUES(?001, ?002, ?003, ?004, ?005,"
								"?006, ?007, ?008,"
								"?009, ?010);");

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, timestamp);
	sqlCommand.BindParam(3, neighbID);
	sqlCommand.BindParam(4, (int)neighbHealth.linkStatus);
	sqlCommand.BindParam(5, (int)neighbHealth.DPDUsTransmitted);
	sqlCommand.BindParam(6, (int)neighbHealth.DPDUsReceived);
	sqlCommand.BindParam(7, (int)neighbHealth.DPDUsFailedTransmission);
	sqlCommand.BindParam(8, (int)neighbHealth.DPDUsFailedReception);
	sqlCommand.BindParam(9, (int)neighbHealth.signalStrength);
	sqlCommand.BindParam(10, (int)neighbHealth.signalQuality);
	sqlCommand.ExecuteNonQuery();
}

//added by Cristian.Guef
int MySqlDevicesDal::CreateChannel(int deviceID, PublisherConf::COChannel &channel)
{
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO DeviceChannels(DeviceID, ChannelName, UnitOfMeasurement,"
						"ChannelFormat, SourceTSAPID, SourceObjID, SourceAttrID, SourceIndex1, SourceIndex2, WithStatus)"
						" VALUES(?001, ?002, ?003, ?004, ?005,"
								"?006, ?007, ?008,"
								"?009, ?010);");

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, channel.name);
	sqlCommand.BindParam(3, channel.unitMeasure);
	sqlCommand.BindParam(4, (int)channel.format);
	sqlCommand.BindParam(5, (int)channel.tsapID);
	sqlCommand.BindParam(6, (int)channel.objID);
	sqlCommand.BindParam(7, (int)channel.attrID);
	sqlCommand.BindParam(8, (int)channel.index1);
	sqlCommand.BindParam(9, (int)channel.index2);
	sqlCommand.BindParam(10, (int)channel.withStatus);
	sqlCommand.ExecuteNonQuery();
	channel.dbChannelNo = sqlCommand.GetLastInsertRowID();

	return channel.dbChannelNo;
}
void MySqlDevicesDal::DeleteChannel(int channelNo)
{
	MySQLCommand sqlCommand(connection, ""
		"DELETE FROM DeviceChannels "
				"WHERE ChannelNo = ?001");

	sqlCommand.BindParam(1, channelNo);
	sqlCommand.ExecuteNonQuery();
}

void MySqlDevicesDal::MySqlDevicesDal::DeleteAllChannels()
{
	{
		MySQLCommand sqlCommand(connection, ""
			"DELETE FROM DeviceChannels ");
		sqlCommand.ExecuteNonQuery();
	}
	{
		char sqlCmd[500];
		sprintf(sqlCmd, "DELETE from DeviceReadings;");

		MySQLCommand sqlCommand(connection, sqlCmd);
		sqlCommand.ExecuteNonQuery();
	}
}

void MySqlDevicesDal::GetOrderedChannels(int deviceID, std::vector<PublisherConf::COChannel> &channels)
{
	MySQLCommand
	    sqlCommand(
	        connection,
	        "SELECT ChannelNo, ChannelName, UnitOfMeasurement, ChannelFormat, "
			"SourceTSAPID, SourceObjID, SourceAttrID, SourceIndex1, SourceIndex2, WithStatus "
				"FROM DeviceChannels "
				"WHERE DeviceID = ?001 "
				"ORDER BY SourceTSAPID, SourceObjID, SourceAttrID, SourceIndex1, SourceIndex2;");

	sqlCommand.BindParam(1, deviceID);
	
	MySQLResultSet::Ptr res = sqlCommand.ExecuteQuery();
	channels.resize(res->RowsCount());
	int i = 0;
	for (MySQLResultSet::Iterator itDeviceChannel = res->Begin(); 
					itDeviceChannel != res->End(); itDeviceChannel++)
	{
		channels[i].dbChannelNo = itDeviceChannel->Value<int>(0);
		channels[i].name = itDeviceChannel->Value<std::string>(1);
		channels[i].unitMeasure = itDeviceChannel->Value<std::string>(2);
		channels[i].format = itDeviceChannel->Value<int>(3);
		channels[i].tsapID = itDeviceChannel->Value<int>(4);
		channels[i].objID = itDeviceChannel->Value<int>(5);
		channels[i].attrID = itDeviceChannel->Value<int>(6);
		channels[i].index1 = itDeviceChannel->Value<int>(7);
		channels[i].index2 = itDeviceChannel->Value<int>(8);
		channels[i].withStatus = itDeviceChannel->Value<int>(9);
		++i;
	}
}
void MySqlDevicesDal::GetOrderedChannelsDevMACs(std::vector<MAC> &macs)
{
	MySQLCommand
	    sqlCommand(
	        connection,
	        "SELECT Address64 FROM Devices WHERE DeviceID IN (SELECT DISTINCT DeviceID FROM DeviceChannels) "
			"ORDER BY Address64;");

	MySQLResultSet::Ptr res = sqlCommand.ExecuteQuery();
	macs.resize(res->RowsCount());

	int i = 0;
	for (MySQLResultSet::Iterator itMAC = res->Begin(); 
					itMAC != res->End(); itMAC++)
	{
		macs[i] = MAC(itMAC->Value<std::string>(0));
		++i;
	}
}

bool MySqlDevicesDal::GetChannel(int channelNo, int &deviceID, PublisherConf::COChannel &channel)
{
	MySQLCommand
	    sqlCommand(
	        connection,
	        "SELECT DeviceID, ChannelName, UnitOfMeasurement, ChannelFormat, "
			"SourceTSAPID, SourceObjID, SourceAttrID, SourceIndex1, SourceIndex2 "
				"FROM DeviceChannels "
				"WHERE ChannelNo = ?001");

	sqlCommand.BindParam(1, channelNo);
	
	MySQLResultSet::Ptr res = sqlCommand.ExecuteQuery();
	if (res->RowsCount()> 0)
	{
		MySQLResultSet::Iterator itDeviceChannel = res->Begin();
		deviceID = itDeviceChannel->Value<int>(0);
		channel.name = itDeviceChannel->Value<std::string>(1);
		channel.unitMeasure = itDeviceChannel->Value<std::string>(2);
		channel.format = itDeviceChannel->Value<int>(3);
		channel.tsapID = itDeviceChannel->Value<int>(4);
		channel.objID = itDeviceChannel->Value<int>(5);
		channel.attrID = itDeviceChannel->Value<int>(6);
		channel.index1 = itDeviceChannel->Value<int>(7);
		channel.index2 = itDeviceChannel->Value<int>(8);
		return true;
	}
	return false;
}
void MySqlDevicesDal::UpdateChannel(int channelNo, const std::string &channelName, const std::string &unitOfMeasure, int channelFormat, int withStatus)
{
	MySQLCommand sqlCommand(connection, ""
		"UPDATE DeviceChannels SET ChannelName = ?001, "
		"UnitOfMeasurement = ?002, ChannelFormat = ?003, WithStatus = ?004 WHERE ChannelNo = ?005");

	sqlCommand.BindParam(1, channelName);
	sqlCommand.BindParam(2, unitOfMeasure);
	sqlCommand.BindParam(3, channelFormat);
	sqlCommand.BindParam(4, withStatus);
	sqlCommand.BindParam(5, channelNo);
	sqlCommand.ExecuteNonQuery();
}

bool MySqlDevicesDal::FindDeviceChannel(int channelNo, DeviceChannel& channel)
{
	MySQLCommand
	    sqlCommand(
	        connection,
	        "SELECT ChannelName, ChannelFormat, "
			"SourceTSAPID, SourceObjID, SourceAttrID, WithStatus "
				"FROM DeviceChannels "
				"WHERE ChannelNo = ?001");

	sqlCommand.BindParam(1, channelNo);
	
	MySQLResultSet::Ptr res = sqlCommand.ExecuteQuery();
	if (res->RowsCount()> 0)
	{
		MySQLResultSet::Iterator itDeviceChannel = res->Begin();
		channel.channelName = itDeviceChannel->Value<std::string>(0);
		channel.channelDataType = (ChannelValue::DataType)itDeviceChannel->Value<int>(1);
		channel.mappedTSAPID = itDeviceChannel->Value<int>(2);
		channel.mappedObjectID = itDeviceChannel->Value<int>(3);
		channel.mappedAttributeID = itDeviceChannel->Value<int>(4);
		channel.withStatus = itDeviceChannel->Value<int>(5);

		//
		channel.channelNumber = channelNo;
		return true;
	}
	return false;
}
		
bool MySqlDevicesDal::IsDeviceChannel(int deviceID, const PublisherConf::COChannel &channel)
{
	MySQLCommand
	    sqlCommand(
	        connection,
	        "SELECT ChannelNo "
				"FROM DeviceChannels "
				"WHERE DeviceID = ?001 AND SourceTSAPID = ?002 AND SourceObjID = ?003 "
				"AND SourceAttrID = ?004 AND SourceIndex1 = ?005 AND SourceIndex2 = ?006" );

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, (int)channel.tsapID);
	sqlCommand.BindParam(3, (int)channel.objID);
	sqlCommand.BindParam(4, (int)channel.attrID);
	sqlCommand.BindParam(5, (int)channel.index1);
	sqlCommand.BindParam(6, (int)channel.index2);
	
	MySQLResultSet::Ptr res = sqlCommand.ExecuteQuery();
	if (res->RowsCount()> 0)
		return true;
	return false;
}


bool MySqlDevicesDal::HasDeviceChannels(int deviceID)
{
	MySQLCommand
	    sqlCommand(
	        connection,
	        "SELECT ChannelNo "
				"FROM DeviceChannels "
				"WHERE DeviceID = ?001;" );

	sqlCommand.BindParam(1, deviceID);
	
	MySQLResultSet::Ptr res = sqlCommand.ExecuteQuery();
	if (res->RowsCount()> 0)
		return true;
	return false;
}

void MySqlDevicesDal::SetPublishErrorFlag(int deviceID, int flag)
{
	MySQLCommand sqlCommand(connection, ""
		"UPDATE Devices SET PublishErrorFlag = ?001 "
		"WHERE DeviceID = ?002");

	sqlCommand.BindParam(1, flag);
	sqlCommand.BindParam(2, deviceID);
	sqlCommand.ExecuteNonQuery();
}

void MySqlDevicesDal::MoveChannelToHistory(int channelNo)
{
	{
		MySQLCommand sqlCommand(connection, ""
			"INSERT INTO DeviceChannelsHistory "
			"SELECT DeviceID, ChannelNo, ChannelName, UnitOfMeasurement, "
			"ChannelFormat, SourceTSAPID, SourceObjID, SourceAttrID, "
			"SourceIndex1, SourceIndex2, WithStatus, UTC_TIMESTAMP() FROM DeviceChannels WHERE ChannelNo = ?001");
	
		sqlCommand.BindParam(1, channelNo);
		sqlCommand.ExecuteNonQuery();
	}
	{
		MySQLCommand sqlCommand(connection, ""
			"DELETE FROM DeviceChannels WHERE ChannelNo = ?001");
	
		sqlCommand.BindParam(1, channelNo);
		sqlCommand.ExecuteNonQuery();
	}
}
void MySqlDevicesDal::MoveChannelsToHistory(int deviceID)
{
	{
		MySQLCommand sqlCommand(connection, ""
			"INSERT INTO DeviceChannelsHistory "
			"SELECT DeviceID, ChannelNo, ChannelName, UnitOfMeasurement, "
			"ChannelFormat, SourceTSAPID, SourceObjID, SourceAttrID, "
			"SourceIndex1, SourceIndex2, WithStatus, UTC_TIMESTAMP() FROM DeviceChannels WHERE DeviceID = ?001");
	
		sqlCommand.BindParam(1, deviceID);
		sqlCommand.ExecuteNonQuery();
	}
	{
		MySQLCommand sqlCommand(connection, ""
			"DELETE FROM DeviceChannels WHERE DeviceID = ?001");
	
		sqlCommand.BindParam(1, deviceID);
		sqlCommand.ExecuteNonQuery();
	}
}

bool MySqlDevicesDal::GetAlertProvision(int &categoryProcessSub, int &categoryDeviceSub, int &categoryNetworkSub, int &categorySecuritySub)
{
	MySQLCommand
	    sqlCommand(
	        connection,
		        "SELECT CategoryProcess, CategoryDevice, CategoryNetwork, CategorySecurity"
		        " FROM AlertSubscriptionCategory;");
	
	MySQLResultSet::Ptr resultSet = sqlCommand.ExecuteQuery();
	if (resultSet->RowsCount()> 0)
	{
		MySQLResultSet::Iterator it = resultSet->Begin();

		categoryProcessSub = it->Value<int>(0);
		categoryDeviceSub = it->Value<int>(1);
		categoryNetworkSub = it->Value<int>(2);
		categorySecuritySub = it->Value<int>(3);
		return true;
	}
	return false;
}

void MySqlDevicesDal::SaveAlertInfo(int devID, int TSAPID, int objID, const nlib::DateTime &timestamp, int milisec,
		int classType, int direction, int category, int type, int priority, std::string &data)
{
	
	if (data.size() > 0)
	{
		MySQLCommand sqlCommand(connection, ""
			"INSERT INTO AlertNotifications(DeviceID, TsapID, ObjID,"
						" AlertTime, AlertTimeMsec, Class, Direction,"
						" Category, Type, " 
						" Priority, AlertData)"
						" VALUES(?001, ?002, ?003, ?004, ?005,"
								"?006, ?007, ?008,"
								"?009, ?010, ?011);");
		sqlCommand.BindParam(1, devID);
		sqlCommand.BindParam(2, TSAPID);
		sqlCommand.BindParam(3, objID);
		sqlCommand.BindParam(4, timestamp);
		sqlCommand.BindParam(5, milisec);
		sqlCommand.BindParam(6, classType);
		sqlCommand.BindParam(7, direction);
		sqlCommand.BindParam(8, category);
		sqlCommand.BindParam(9, type);
		sqlCommand.BindParam(10, priority);
		sqlCommand.BindParam(11, data);
		sqlCommand.ExecuteNonQuery();
	}
	else
	{
		MySQLCommand sqlCommand(connection, ""
			"INSERT INTO AlertNotifications(DeviceID, TsapID, ObjID,"
						" AlertTime, AlertTimeMsec, Class, Direction,"
						" Category, Type, " 
						" Priority)"
						" VALUES(?001, ?002, ?003, ?004, ?005,"
								"?006, ?007, ?008,"
								"?009, ?010);");
		sqlCommand.BindParam(1, devID);
		sqlCommand.BindParam(2, TSAPID);
		sqlCommand.BindParam(3, objID);
		sqlCommand.BindParam(4, timestamp);
		sqlCommand.BindParam(5, milisec);
		sqlCommand.BindParam(6, classType);
		sqlCommand.BindParam(7, direction);
		sqlCommand.BindParam(8, category);
		sqlCommand.BindParam(9, type);
		sqlCommand.BindParam(10, priority);
		sqlCommand.ExecuteNonQuery();
	}

	
}


//channels statistics
void MySqlDevicesDal::DeleteChannelsStatistics(int deviceID)
{
	MySQLCommand sqlCommand(connection, ""
			"DELETE FROM ChannelsStatistics WHERE DeviceID = ?001");
	
		sqlCommand.BindParam(1, deviceID);
		sqlCommand.ExecuteNonQuery();
}
void MySqlDevicesDal::SaveChannelsStatistics(int deviceID, const std::string& strChannelsStatistics)
{
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO ChannelsStatistics(DeviceID, ByteChannelsArray)"
						" VALUES(?001, ?002);");

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, strChannelsStatistics);
	sqlCommand.ExecuteNonQuery();
}

void MySqlDevicesDal::ResetPendingFirmwareUploads()
{
	MySQLCommand sqlCommand(connection, ""
		"UPDATE Firmwares SET UploadStatus = ?001 , LastFailedUploadTime = ?002 WHERE UploadStatus = ?003");
	sqlCommand.BindParam(1, (int)FirmwareUpload::fusWaitRetry);
	sqlCommand.BindParam(2, nlib::CurrentUniversalTime());
	sqlCommand.BindParam(3, (int)FirmwareUpload::fusUploading);

	sqlCommand.ExecuteNonQuery();
}

bool MySqlDevicesDal::GetNextFirmwareUpload(int notFailedForMinutes, FirmwareUpload& result)
{
	//HACK:[andy] - please somehow remove the "no date" and "no time" from the sql query
	MySQLCommand
	    sqlCommand(
	        connection,
	        ""
		        "SELECT FirmwareID, FileName, UploadStatus, UploadRetryCount, LastFailedUploadTime"
		        " FROM Firmwares"
		        " WHERE ((LastFailedUploadTime IS NULL OR LastFailedUploadTime = '1000-01-01 00:00:00' OR LastFailedUploadTime = '1000-01-01 00:00:00')"
		        " OR (LastFailedUploadTime < ?001)) "
		        "	AND (UploadStatus = ?002 OR UploadStatus = ?003)");
	sqlCommand.BindParam(1, nlib::CurrentUniversalTime() - nlib::util::minutes(notFailedForMinutes));
	sqlCommand.BindParam(2, (int)FirmwareUpload::fusNew);
	sqlCommand.BindParam(3, (int)FirmwareUpload::fusWaitRetry);

	MySQLResultSet::Ptr resultSet = sqlCommand.ExecuteQuery();
	if (resultSet->RowsCount()> 0)
	{
		MySQLResultSet::Iterator itFirmwares = resultSet->Begin();

		result.Id = (boost::uint32_t)itFirmwares->Value<int>(0);
		result.FileName= itFirmwares->Value<std::string>(1);
		result.Status = (FirmwareUpload::FirmwareUploadStatus)itFirmwares->Value<int>(2);
		result.RetriesCount = itFirmwares->Value<int>(3);
		if (!itFirmwares->IsNull(4))
		{
			result.LastFailedUploadTime = itFirmwares->Value<nlib::DateTime>(4);
		}

		return true;
	}
	return false;
}

void MySqlDevicesDal::UpdateFirmwareUpload(const FirmwareUpload& firmware)
{
	MySQLCommand sqlCommand(connection, ""
		"UPDATE Firmwares SET FileName = ?001, UploadStatus = ?002, UploadRetryCount = ?003, LastFailedUploadTime = ?004"
		" WHERE FirmwareID = ?005");
	sqlCommand.BindParam(1, firmware.FileName);
	sqlCommand.BindParam(2, (int)firmware.Status);
	sqlCommand.BindParam(3, (int)firmware.RetriesCount);
	sqlCommand.BindParam(4, firmware.LastFailedUploadTime);
	sqlCommand.BindParam(5, (int)firmware.Id);

	sqlCommand.ExecuteNonQuery();
}

bool MySqlDevicesDal::GetFirmwareUpload(boost::uint32_t id, FirmwareUpload& result)
{
	MySQLCommand sqlCommand(connection, ""
		"SELECT FirmwareID, FileName, UploadStatus, UploadRetryCount, LastFailedUploadTime"
		" FROM Firmwares "
		" WHERE FirmwareID = ?001");
	sqlCommand.BindParam(1, (int)id);
	MySQLResultSet::Ptr resultSet = sqlCommand.ExecuteQuery();
	if (resultSet->RowsCount()> 0)
	{
		MySQLResultSet::Iterator itFirmwares = resultSet->Begin();
		result.Id = (boost::uint32_t)itFirmwares->Value<int>(0);
		result.FileName= itFirmwares->Value<std::string>(1);
		result.Status = (FirmwareUpload::FirmwareUploadStatus)itFirmwares->Value<int>(2);
		result.RetriesCount = itFirmwares->Value<int>(3);
		if (!itFirmwares->IsNull(4))
		{
			result.LastFailedUploadTime = itFirmwares->Value<nlib::DateTime>(4);
		}

		return true;
	}
	return false;
}

void MySqlDevicesDal::GetFirmwareUploads(FirmwareUploadsT& firmwaresList)
{
	MySQLCommand sqlCommand(connection, ""
		"SELECT FirmwareID, FileName, UploadStatus, UploadRetryCount, LastFailedUploadTime"
		" FROM Firmwares ");
	MySQLResultSet::Ptr resultSet = sqlCommand.ExecuteQuery();

	for (MySQLResultSet::Iterator it = resultSet->Begin(); it != resultSet->End(); it++)
	{
		firmwaresList.push_back(FirmwareUpload());
		FirmwareUpload& result = *firmwaresList.rbegin();

		result.Id = (boost::uint32_t)it->Value<int>(0);
		result.FileName= it->Value<std::string>(1);
		result.Status = (FirmwareUpload::FirmwareUploadStatus)it->Value<int>(2);
		result.RetriesCount = it->Value<int>(3);

		if (!it->IsNull(4))
		{
			result.LastFailedUploadTime = it->Value<nlib::DateTime>(4);
		}
	}
}

void MySqlDevicesDal::AddDeviceConnections(int deviceID, const std::string& host, int port)
{
	MySQLCommand sqlCommand(connection, ""
			"INSERT INTO DeviceConnections(DeviceID, IP, Port)"
			" VALUES(?001, ?002, ?003) ON DUPLICATE KEY UPDATE IP=?002, Port=?003;");

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, host);
	sqlCommand.BindParam(3, port);
	sqlCommand.ExecuteNonQuery();
}

void MySqlDevicesDal::RemoveDeviceConnections(int deviceID)
{
	MySQLCommand sqlCommand(connection, ""
		"DELETE FROM DeviceConnections "
		" WHERE DeviceID = ?001;");
	sqlCommand.BindParam(1, deviceID);
	sqlCommand.ExecuteNonQuery();

}

//firmwaredownloads
static const char* gettime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL); 
	static char szTime[50];
	static struct tm * timeinfo;

	timeinfo = gmtime(&tv.tv_sec);
	sprintf(szTime, "%04d-%02d-%02d %02d:%02d:%02d", timeinfo->tm_year+1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, 
		timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec); 
	return szTime;
}

///[marcel] TODO: filter by max(StartedOn) and fwType
void MySqlDevicesDal::SetFirmDlStatus(int deviceID, int fwType, int status)
{
	char sqlCmd[500];
	sprintf(sqlCmd, "UPDATE FirmwareDownloads SET FwStatus = %d, CompletedOn = %s WHERE DeviceID = %d", status, gettime(), deviceID);
	MySQLCommand sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();
}

///[marcel] TODO: filter by max(StartedOn) and fwType
void MySqlDevicesDal::SetFirmDlPercent(int deviceID, int fwType, int percent)
{
	char sqlCmd[500];
	sprintf(sqlCmd, "UPDATE FirmwareDownloads SET CompletedPercent = %d WHERE DeviceID = %d", percent, deviceID);
	MySQLCommand sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();
}

///[marcel] TODO: filter by max(StartedOn) and fwType
void MySqlDevicesDal::SetFirmDlSize(int deviceID, int fwType, int size)
{
	char sqlCmd[500];
	sprintf(sqlCmd, "UPDATE FirmwareDownloads SET DownloadSize = %d WHERE DeviceID = %d", size, deviceID);
	MySQLCommand sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();
}

///[marcel] TODO: filter by max(StartedOn) and fwType
void MySqlDevicesDal::SetFirmDlSpeed(int deviceID, int fwType, int speed)
{
	char sqlCmd[500];
	sprintf(sqlCmd, "UPDATE FirmwareDownloads SET Speed = %d WHERE DeviceID = %d", speed, deviceID);
	MySQLCommand sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();
}

///[marcel] TODO: filter by max(StartedOn) and fwType
void MySqlDevicesDal::SetFirmDlAvgSpeed(int deviceID, int fwType, int speed)
{
	char sqlCmd[500];
	sprintf(sqlCmd, "UPDATE FirmwareDownloads SET AvgSpeed = %d WHERE DeviceID = %d", speed, deviceID);
	MySQLCommand sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();
}


}//namespace hostapp
}//namespace nisa100

#endif
