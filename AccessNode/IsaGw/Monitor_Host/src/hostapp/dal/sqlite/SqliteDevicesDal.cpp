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

#include "SqliteDevicesDal.h"

#include <math.h>
#include <stdio.h>
#include <arpa/inet.h>

#include <sqlite/sqlite3.h>
#include <tai.h>

extern int history_status_size;

namespace nisa100 {
namespace hostapp {

SqliteDevicesDal::SqliteDevicesDal(sqlitexx::Connection& connection_) :
	connection(connection_), m_oUpdateDeviceReadings_Prepare(connection_), m_oAddDeviceReadings_compiled(connection_), 
		m_oUpdatePublishFlag_compiled(connection_),
		m_oUpdateFirmDl_compiled_status(connection_),
		m_oUpdateFirmDl_compiled_percent(connection_),
		m_oUpdateFirmDl_compiled_size(connection_),
		m_oUpdateFirmDl_compiled_speed(connection_),
		m_oUpdateFirmDl_compiled_avgspeed(connection_),
		m_oUpdateFirmDl_compiled_completed(connection_)
{
}

SqliteDevicesDal::~SqliteDevicesDal()
{
}

void SqliteDevicesDal::VerifyTables()
{
	LOG_DEBUG("Verify devices tables structure...");
	{
		const char*  query =
		        "SELECT DeviceID, DeviceRole, Address128, Address64, DeviceTag, DeviceStatus, LastRead, DeviceLevel, PublishErrorFlag"
			        " FROM Devices WHERE 0";
		try
		{
			sqlitexx::Command::ExecuteQuery(connection.GetRawDbObj(), query);
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}
	{
		const char*  query =
		        "SELECT DeviceID, DPDUsTransmitted, DPDUsReceived, DPDUsFailedTransmission, DPDUsFailedReception, "
				" PowerSupplyStatus, Manufacturer, Model, Revision, SubnetID"
			        " FROM DevicesInfo WHERE 0";
		try
		{
			sqlitexx::Command::ExecuteQuery(connection.GetRawDbObj(), query);
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}

	{
		const char*  query =
		        ""
			        "SELECT DeviceID, ChannelNo, ChannelName, UnitOfMeasurement, ChannelFormat, SourceTSAPID, SourceObjID, SourceAttrID, SourceIndex1, SourceIndex2"
			        " FROM DeviceChannels WHERE 0";
		try
		{
			sqlitexx::Command::ExecuteQuery(connection.GetRawDbObj(), query);
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}

	{
		const char*  query =
			"SELECT FromDeviceID, ToDeviceID, SignalQuality, ClockSource"
			" FROM TopologyLinks WHERE 0";
		try
		{
			sqlitexx::Command::ExecuteQuery(connection.GetRawDbObj(), query);
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}

	{
		const char*  query =
			"SELECT FromDeviceID, ToDeviceID, GraphID"
			" FROM TopologyGraphs WHERE 0";
		try
		{
			sqlitexx::Command::ExecuteQuery(connection.GetRawDbObj(), query);
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}

	{
		const char*  query = "SELECT DeviceID, ReadingTime, ChannelNo, Value, RawValue, ReadingType, Miliseconds, ValueStatus"
			" FROM DeviceReadings WHERE 0";
		try
		{
			sqlitexx::Command::ExecuteQuery(connection.GetRawDbObj(), query);
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}
	{
		const char*  query = "SELECT HistoryID, DeviceID, Timestamp, DeviceStatus FROM DeviceHistory WHERE 0";
		try
		{
			sqlitexx::Command::ExecuteQuery(connection.GetRawDbObj(), query);
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}
	{
		const char*  query =
		        ""
			        "SELECT FirmwareID, FileName, Version, Description, UploadDate, FirmwareType, UploadStatus, UploadRetryCount, LastFailedUploadTime"
			        " FROM Firmwares WHERE 0";
		try
		{
			sqlitexx::Command::ExecuteQuery(connection.GetRawDbObj(), query);
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}

	{
		const char*  query = ""
			"SELECT DeviceID, IP, Port"
			" FROM DeviceConnections WHERE 0";
		try
		{
			sqlitexx::Command::ExecuteQuery(connection.GetRawDbObj(), query);
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}
}

void SqliteDevicesDal::ResetDevices(Device::DeviceStatus newStatus)
{
	LOG_DEBUG("Reset all devices from db");
	
	sqlitexx::Command sqlCommand(connection, ""
		"UPDATE Devices SET DeviceStatus = ?001");
	sqlCommand.BindParam(1, (int)newStatus);

	sqlCommand.ExecuteNonQuery();

	//also history
	sqlitexx::Command sqlCommand1(connection, ""
		"INSERT INTO DeviceHistory(DeviceID, Timestamp, DeviceStatus)"
		" SELECT DeviceID, datetime('now'), DeviceStatus FROM Devices;");
	sqlCommand1.ExecuteNonQuery();

}

void SqliteDevicesDal::GetDevices(DeviceList& list)
{
	LOG_DEBUG("Get all devices from database");

	sqlitexx::CSqliteStmtHelper oSqlDev (connection);

	if (oSqlDev.Prepare( "SELECT DeviceID, DeviceRole, Address128, Address64, DeviceTag, DeviceStatus, DeviceLevel FROM Devices;") 
		!= SQLITE_OK)
	{
		return ;
	}

	
	if (oSqlDev.Step_GetRow() != SQLITE_ROW)
	{
		return ;
	}

	list.reserve(8);

	do 
	{
		list.push_back(Device());
		Device& device = *list.rbegin();

		device.id = oSqlDev.Column_GetInt(0);//it->Value<int>(0);
		device.deviceType = (Device::DeviceType)oSqlDev.Column_GetInt(1);//(Device::DeviceType)it->Value<int>(1);
		device.ip = IPv6(oSqlDev.Column_GetText(2));//IPv6(it->Value<std::string>(2));
		device.mac = MAC(oSqlDev.Column_GetText(3));//MAC(it->Value<std::string>(3));
		device.m_deviceTAG = oSqlDev.Column_GetText(4);//it->Value<std::string>(4);
		device.status = (Device::DeviceStatus)oSqlDev.Column_GetInt(5);//(Device::DeviceStatus)it->Value<int>(5);
		device.deviceLevel = oSqlDev.Column_GetInt(6);//it->Value<int>(6);

		//LOG_DEBUG("GetDevices: mac="<<device.mac.ToString() << " type=" <<device.deviceType);
	}
	while (oSqlDev.Step_GetRow() == SQLITE_ROW);

	LOG_DEBUG("GetDevices: no="<<list.size());

	if (list.empty())
	{
		return;
	}

	

	sqlitexx::CSqliteStmtHelper oSql (connection);

	if (oSql.Prepare("SELECT DPDUsTransmitted, DPDUsReceived, DPDUsFailedTransmission, DPDUsFailedReception,"
					" PowerSupplyStatus FROM DevicesInfo WHERE DeviceID = ?001") 
			!= SQLITE_OK)
	{
		return;
	}

	////for device info
	for (unsigned int i = 0; i < list.size(); i++)
	{
		oSql.BindInt(1, list[i].id);

		if (oSql.Step_GetRow() == SQLITE_ROW)
		{				
			list[i].m_DPDUsTransmitted = oSql.Column_GetInt(0); //itDevice->Value<int>(0);
			list[i].m_DPDUsReceived = oSql.Column_GetInt(1); //itDevice->Value<int>(1);
			list[i].m_DPDUsFailedTransmission = oSql.Column_GetInt(2); //itDevice->Value<int>(2);
			list[i].m_DPDUsFailedReception = oSql.Column_GetInt(3); //itDevice->Value<int>(3);
			list[i].powerStatus = oSql.Column_GetInt(4); //itDevice->Value<int>(4);
			oSql.ResetStep();
		}
	}

	//read history
	sqlitexx::CSqliteStmtHelper oSqlHistory (connection);
	if (oSqlHistory.Prepare("SELECT Timestamp FROM DeviceHistory WHERE DeviceID = ?001") != SQLITE_OK)
	{
		return;
	}
	for (unsigned int i = 0; i < list.size(); i++)
	{
		
		oSqlHistory.BindInt(1, list[i].id);
		
		if (oSqlHistory.Step_GetRow() != SQLITE_ROW)
			continue;

		do 
		{
			std::string str = oSqlHistory.Column_GetText(0);
			list[i].changedStatus.push(str);
			LOG_DEBUG("HISTORY FOUND for devID=" << list[i].id << " with date=" << str);
		}
		while (oSqlHistory.Step_GetRow() == SQLITE_ROW);

		oSqlHistory.ResetStep();
	}

}

void SqliteDevicesDal::DeleteDevice(int id)
{
	sqlitexx::Command sqlCommand(connection, ""
			"DELETE FROM Devices WHERE DeviceID = ?001");
		sqlCommand.BindParam(1, id);
		sqlCommand.ExecuteNonQuery();
}


//added by Cristian.Guef
bool SqliteDevicesDal::IsDeviceInDB(int deviceID)
{
	LOG_DEBUG("Try to find device with Device ID:" << deviceID);

	sqlitexx::CSqliteStmtHelper oSql (connection);


	if (oSql.Prepare("SELECT DeviceID FROM Devices WHERE DeviceID = ?001;") != SQLITE_OK)
	{
		return false;
	}

	oSql.BindInt(1, deviceID);

	return oSql.Step_GetRow() == SQLITE_ROW;
}


void SqliteDevicesDal::AddDevice(Device& device)
{
	LOG_DEBUG("Add device with MAC:" << device.mac.ToString());
	
	//create device
	sqlitexx::Command
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
		sqlitexx::Command
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
		sqlitexx::Command
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
		sqlitexx::Command sqlCommand(connection, ""
			"INSERT INTO DeviceHistory(DeviceID, Timestamp, DeviceStatus)"
			" VALUES (?001, ?002, ?003)");

		sqlCommand.BindParam(1, device.id);
		sqlCommand.BindParam(2, nlib::CurrentUniversalTime());
		sqlCommand.BindParam(3, (int)device.status);
		sqlCommand.ExecuteNonQuery();
	}
}


void SqliteDevicesDal::UpdateDevice(Device& device)
{
	LOG_DEBUG("Update device:" << device.ToString());
	{
		//create history if status changed
		// this should be called before status updated ., :)
		if (device.IsStatusChanged())
		{
			sqlitexx::Command sqlCommand(connection, ""
				"INSERT INTO DeviceHistory(DeviceID, Timestamp, DeviceStatus)"
				" VALUES (?001, ?002, ?003)");
			sqlCommand.BindParam(1, device.id);
			sqlCommand.BindParam(2, device.changedStatus.back());
			sqlCommand.BindParam(3, (int)device.status);
			sqlCommand.ExecuteNonQuery();

			while ( (int)device.changedStatus.size() > history_status_size)
			{
				sqlitexx::Command sqlCommand(connection, ""
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
	sqlitexx::Command
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
		sqlitexx::Command
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
		sqlitexx::Command
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
void SqliteDevicesDal::UpdateReadingTimeforDevice(int deviceID, const struct timeval &tv)
{
	//update time for device_id
	/*sqlitexx::Command sqlCommand(connection, ""
		"UPDATE Devices SET"
		" LastRead = ?001"
		" WHERE DeviceID = ?002");

	sqlCommand.BindParam(1, readingTime);
	sqlCommand.BindParam(2, deviceID);
	sqlCommand.ExecuteNonQuery();	*/
	char sqlCmd[500];
	sprintf(sqlCmd, "UPDATE Devices SET LastRead = '%s' WHERE DeviceID = %d", gettime(tv), deviceID);
	sqlitexx::Command sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();
}

void SqliteDevicesDal::CleanDeviceNeighbours()
{
	sqlitexx::Command sqlCommand(connection, ""
		"DELETE FROM TopologyLinks");
	sqlCommand.ExecuteNonQuery();
}

void SqliteDevicesDal::CreateDeviceNeighbour(int fromDevice, int toDevice, int signalQuality, int clockSource)
{
	//LOG_DEBUG("Create neighbour for the device:" << device.mac.ToString());
	sqlitexx::Command sqlCommand(connection, ""
		"INSERT INTO TopologyLinks(FromDeviceID, ToDeviceID, SignalQuality, ClockSource)"
		" VALUES (?001, ?002, ?003, ?004)");

	sqlCommand.BindParam(1, fromDevice);
	sqlCommand.BindParam(2, toDevice);
	sqlCommand.BindParam(3, signalQuality);
	sqlCommand.BindParam(4, clockSource);
	sqlCommand.ExecuteNonQuery();
}


void SqliteDevicesDal::CleanDeviceGraphs()
{
	sqlitexx::Command sqlCommand(connection, ""
		"DELETE FROM TopologyGraphs");
	sqlCommand.ExecuteNonQuery();
}

void SqliteDevicesDal::CreateDeviceGraph(int fromDevice, int toDevice, int graphID)
{
	//LOG_DEBUG("Create neighbour for the device:" << device.mac.ToString());
	sqlitexx::Command sqlCommand(connection, ""
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

void SqliteDevicesDal::AddReading(const DeviceReading& reading, bool p_bHistory)
{
	LOG_DEBUG("Add reading to the device:" << reading.deviceID);

	if	(!m_oAddDeviceReadings_compiled.GetStmt())
	{
		if (m_oAddDeviceReadings_compiled.Prepare ("INSERT INTO DeviceReadingsHistory(DeviceID, ReadingTime, ChannelNo, Value, RawValue, ReadingType, Miliseconds, ValueStatus)"
			" VALUES (?001, ?002, ?003, ?004, ?005, ?006, ?007, ?008)") != SQLITE_OK)
		{
			return;
		}

	}

	m_oAddDeviceReadings_compiled.BindInt	(1, reading.deviceID);
	m_oAddDeviceReadings_compiled.BindText	(2, gettime(reading.tv));
	m_oAddDeviceReadings_compiled.BindInt	(3, (int)reading.channelNo);
	m_oAddDeviceReadings_compiled.BindText	(4, reading.rawValue.ToString().c_str());
	m_oAddDeviceReadings_compiled.BindInt	(5, reading.rawValue.GetIntValue());
	m_oAddDeviceReadings_compiled.BindInt	(6, (int)reading.readingType);
	m_oAddDeviceReadings_compiled.BindInt	(7, (int)(reading.tv.tv_usec/1000));

	if (reading.IsISA == true)
	{
		m_oAddDeviceReadings_compiled.BindInt(8, (int)reading.ValueStatus);
	}
	else
	{
		m_oAddDeviceReadings_compiled.BindNull(8);
	}

	m_oAddDeviceReadings_compiled.Step_Exec();

}

void SqliteDevicesDal::AddEmptyReadings(int DeviceID)
{
	char sqlCmd[1000];
	sprintf(sqlCmd, "INSERT INTO DeviceReadings(DeviceID, ReadingTime, ChannelNo, Value, RawValue, ReadingType, Miliseconds) \
					 SELECT DeviceID, '1970-01-01 00:00:00', ChannelNo, 0, '0', -1, 0 FROM DeviceChannels Where DeviceID=%d;", DeviceID);
	sqlitexx::Command sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();
}
void SqliteDevicesDal::AddEmptyReading(int DeviceID, int channelNo)
{
	char sqlCmd[1000];
	sprintf(sqlCmd, "INSERT INTO DeviceReadings(DeviceID, ReadingTime, ChannelNo, Value, RawValue, ReadingType, Miliseconds) \
					 SELECT %d, '1970-01-01 00:00:00', %d, 0, '0', -1, 0;", DeviceID, channelNo);
	sqlitexx::Command sqlCommand(connection, sqlCmd);
	sqlCommand.ExecuteNonQuery();
}

void SqliteDevicesDal::UpdateReading(const DeviceReading& reading)
{
	int res=0;
	if (!m_oUpdateDeviceReadings_Prepare.GetStmt())
	{
		if ((res=m_oUpdateDeviceReadings_Prepare.Prepare(	"UPDATE DeviceReadings SET ReadingTime=?, Value=?, RawValue=?, ReadingType=?, Miliseconds=?"
			",ValueStatus=?, DeviceID=? WHERE ChannelNo=?;")) != SQLITE_OK)
			{
				connection.LogIfLastError(res);
				LOG_ERROR("SqliteDevicesDal::UpdateReading: FAILED bind 1 err=" << res);
				std::exception ex;
				throw ex;
			}
	}

	if (!m_oUpdateDeviceReadings_Prepare.GetStmt())
	{
		LOG_ERROR("SqliteDevicesDal::UpdateReading: FAILED to get prepare stmt");
		std::exception ex;
		throw ex;
	}

	// bind
	sqlite3_stmt *stmt = m_oUpdateDeviceReadings_Prepare.GetStmt();

	int index = 1;

	if ((res = sqlite3_bind_text ( stmt, index++, gettime(reading.tv), -1, SQLITE_STATIC))!= SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::UpdateReading: FAILED bind 1 err=" << res);
		std::exception ex;
		throw ex;
	};
	

	if ((res = sqlite3_bind_text ( stmt, index++, reading.rawValue.ToString().c_str(), -1, SQLITE_STATIC)) != SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::UpdateReading: FAILED bind 2 err=" << res);
		std::exception ex;
		throw ex;
	};


	if ((res = sqlite3_bind_int ( stmt, index++, reading.rawValue.GetIntValue()))!= SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::UpdateReading: FAILED bind 3 err=" << res);
		std::exception ex;
		throw ex;
	};


	if((res = sqlite3_bind_int ( stmt, index++, (int)reading.readingType))!= SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::UpdateReading: FAILED bind 4 err=" << res);
		std::exception ex;
		throw ex;
	};

	if ((res = sqlite3_bind_int ( stmt, index++, (int)(reading.tv.tv_usec/1000)))!= SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::UpdateReading: FAILED bind 5 err=" << res);
		std::exception ex;
		throw ex;
	};


	if (reading.IsISA == true)
	{
		if((res = sqlite3_bind_int ( stmt, index++, (int)reading.ValueStatus))!= SQLITE_OK)
		{
			connection.LogIfLastError(res);
			LOG_ERROR("SqliteDevicesDal::UpdateReading: FAILED bind 6 err=" << res);
			std::exception ex;
			throw ex;
		};
	}
	else
	{
		if((res = sqlite3_bind_null (stmt, index++))!= SQLITE_OK)
		{
			connection.LogIfLastError(res);
			LOG_ERROR("SqliteDevicesDal::UpdateReading: FAILED bind 6' err =" << res);
			std::exception ex;
			throw ex;
		};	
	}


	if((res = sqlite3_bind_int ( stmt, index++, reading.deviceID))!= SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::UpdateReading: FAILED bind 7 err=" << res);
		std::exception ex;
		throw ex;
	};


	if((res = sqlite3_bind_int ( stmt, index++, reading.channelNo))!= SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::UpdateReading: FAILED bind 8 err=" << res);
		std::exception ex;
		throw ex;
	};


	if ((res=m_oUpdateDeviceReadings_Prepare.Step_Exec()) != SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::UpdateReading: FAILED exec err=" << res);
		std::exception ex;
		throw ex;
	}

	//sqlitexx::Command::ExecuteNonQuery(connection.GetRawDbObj(), sqlCmd); 	
}

void SqliteDevicesDal::DeleteReading(int p_nChannelNo)
{
	char sqlCmd[500];
	sprintf(sqlCmd, "DELETE from DeviceReadings WHERE ChannelNo=%d;", p_nChannelNo);
	sqlitexx::Command::ExecuteNonQuery(connection.GetRawDbObj(), sqlCmd); 
}
void SqliteDevicesDal::DeleteReadings(int deviceID)
{
	char sqlCmd[500];
	sprintf(sqlCmd, "DELETE from DeviceReadings WHERE DeviceID=%d;", deviceID);
	sqlitexx::Command::ExecuteNonQuery(connection.GetRawDbObj(), sqlCmd); 
}
bool SqliteDevicesDal::IsDeviceChannelInReading(int deviceID, int channelNo)
{
	sqlitexx::CSqliteStmtHelper oSql (connection);

	if (oSql.Prepare(
		"SELECT ChannelNo "
		"FROM DeviceReadings "
		"WHERE DeviceID = ?001 AND ChannelNo = ?002;" ) 
		!= SQLITE_OK)
	{
		return false;
	}
	
	oSql.BindInt(1, deviceID);
	oSql.BindInt(2, channelNo);
	
	if (oSql.Step_GetRow() != SQLITE_ROW)
	{
		return false;
	}
	return true;
}
void SqliteDevicesDal::UpdatePublishFlag(int devID, int flag/*0-no data, 1-fresh data, 2-stale data*/)
{
	/*
	sqlitexx::Command sqlCommand(connection, ""
		"UPDATE Devices SET"
		" PublishStatus = ?001"
		" WHERE DeviceID = ?002");

	sqlCommand.BindParam(1, flag);
	sqlCommand.BindParam(2, devID);
	sqlCommand.ExecuteNonQuery();
	*/

	/*
	int res = 0;
	if (!m_oUpdatePublishFlag_compiled.GetStmt())
	{
		if ((res=m_oUpdatePublishFlag_compiled.Prepare("UPDATE Devices SET PublishStatus = ? WHERE DeviceID = ?")) != SQLITE_OK)
		{
			connection.LogIfLastError(res);
			LOG_ERROR("SqliteDevicesDal::m_oUpdatePublishFlag: FAILED prepare err=" << res);
			std::exception ex;
			throw ex;
		}
	}

	if ((res=m_oUpdatePublishFlag_compiled.BindInt(1,flag))!= SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::m_oUpdatePublishFlag: FAILED bind 1 err=" << res);
		std::exception ex;
		throw ex;
	};
	if ((res=m_oUpdatePublishFlag_compiled.BindInt(2,devID))!= SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::m_oUpdatePublishFlag: FAILED bind 2 err=" << res);
		std::exception ex;
		throw ex;
	};

	
	if ((res=m_oUpdatePublishFlag_compiled.Step_Exec())!= SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::m_oUpdatePublishFlag: exec FAILED err=" << res);
		std::exception ex;
		throw ex;
	};

	*/
}

//added by Cristian.Guef
void SqliteDevicesDal::DeleteContracts(int deviceID)
{
	sqlitexx::Command sqlCommand(connection, ""
		"DELETE FROM Contracts WHERE SourceDeviceID = ?001;");
	sqlCommand.BindParam(1, deviceID);
	sqlCommand.ExecuteNonQuery();
}
void SqliteDevicesDal::DeleteContracts()
{
	sqlitexx::Command sqlCommand(connection, ""
		"DELETE FROM Contracts;");
	sqlCommand.ExecuteNonQuery();
}
void SqliteDevicesDal::AddContract(int fromDevice, int toDevice, const ContractsAndRoutes::Contract &contract)
{
	sqlitexx::Command sqlCommand(connection, ""
		"INSERT INTO Contracts(ContractID, ServiceType, ActivationTime, SourceDeviceID, "
							"SourceSAP, DestinationDeviceID, DestinationSAP, ExpirationTime, Priority, "
							"NSDUSize, Reliability, Period, Phase, ComittedBurst, ExcessBurst, Deadline, MaxSendWindow)"
		" VALUES(?001, ?002, ?003, ?004, ?005, "
				"?006, ?007, ?008, ?009, ?010, "
				"?011, ?012, ?013, ?014, ?015, "
				"?016, ?017);");

	char szActivation [ 256 ], szExpiration[ 256 ];
	const char *pszActivationTime = szActivation, *pszExpirationTime = szExpiration;
	if( contract.activationTime )
	{	time_t nActivation = contract.activationTime - (TAI_OFFSET + CurrentUTCAdjustment);	// Convert TAI to UTC
		struct tm * pTm = gmtime( &nActivation );
		snprintf( szActivation, sizeof(szActivation), "%4d-%02d-%02d %02d:%02d:%02d", pTm->tm_year+1900,
			pTm->tm_mon+1,pTm->tm_mday, pTm->tm_hour,pTm->tm_min,pTm->tm_sec);
		szActivation[ sizeof(szActivation)-1 ] = 0;
	}
	else
		pszActivationTime = "immediate";

	if( contract.expirationTime && (-1 != (int)contract.expirationTime) )
	{	//Contract_Expiration_Time determines how long the SystemManager should keep the contract before it is terminated; It is an offset related to the Contract_Activation_Time.
		///TODO: take care of contract.activationTime == 0. Is this really possible?
		time_t nExpiration =  contract.activationTime - (TAI_OFFSET + CurrentUTCAdjustment) + contract.expirationTime;	// Convert TAI to UTC
		struct tm * pTm = gmtime( &nExpiration );
		snprintf( szExpiration, sizeof(szExpiration), "%4d-%02d-%02d %02d:%02d:%02d", pTm->tm_year+1900,
			pTm->tm_mon+1,pTm->tm_mday, pTm->tm_hour,pTm->tm_min,pTm->tm_sec);
		szExpiration[ sizeof(szExpiration)-1 ] = 0;
	}
	else
		pszExpirationTime = "never";
	
	sqlCommand.BindParam(1, (int)contract.contractID);
	sqlCommand.BindParam(2, (int)contract.serviceType);
	sqlCommand.BindParam(3, pszActivationTime);
	sqlCommand.BindParam(4, (int)fromDevice);
	sqlCommand.BindParam(5, (int)contract.sourceSAP);
	sqlCommand.BindParam(6, (int)toDevice);
	sqlCommand.BindParam(7, (int)contract.destinationSAP);
	sqlCommand.BindParam(8, pszExpirationTime );
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
void SqliteDevicesDal::DeleteRoutes(int deviceID)
{
	sqlitexx::Command sqlCommand(connection, ""
		"DELETE FROM RoutesInfo WHERE DeviceID = ?001;");
	sqlCommand.BindParam(1, deviceID);
	sqlCommand.ExecuteNonQuery();

	sqlitexx::Command sqlCommand2(connection, ""
		"DELETE FROM RouteLinks WHERE DeviceID = ?001;");
	sqlCommand2.BindParam(1, deviceID);
	sqlCommand2.ExecuteNonQuery();
}

static const char * SQLs[] = {"INSERT INTO RoutesInfo(DeviceID, RouteID, Alternative, FowardLimit, Selector, SrcAddr) VALUES(?001, ?002, ?003, ?004, ?005, ?006);", 
						"INSERT INTO RoutesInfo(DeviceID, RouteID, Alternative, FowardLimit, Selector) VALUES(?001, ?002, ?003, ?004, ?005);",
						"INSERT INTO RoutesInfo(DeviceID, RouteID, Alternative, FowardLimit, Selector) VALUES(?001, ?002, ?003, ?004, ?005);",
						"INSERT INTO RoutesInfo(DeviceID, RouteID, Alternative, FowardLimit) VALUES(?001, ?002, ?003, ?004);"};
void SqliteDevicesDal::AddRouteInfo(int deviceID, int routeID, int alternative, int fowardLimit, int selector, int srcAddr)
{
	
	if (alternative < 0 || alternative > 3)
	{
		LOG_ERROR("invalid alternative");
		return;
	}

	sqlitexx::Command sqlCommand(connection, SQLs[alternative]);

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
void SqliteDevicesDal::AddRouteLink(int deviceID, int routeID, int routeIndex, int graphID)
{
	sqlitexx::Command sqlCommand(connection, ""
		"INSERT INTO RouteLinks(DeviceID, RouteID, RouteIndex, GraphID)"
		" VALUES(?001, ?002, ?003, ?004);");

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, routeID);
	sqlCommand.BindParam(3, routeIndex);
	sqlCommand.BindParam(4, graphID);
	sqlCommand.ExecuteNonQuery();
}
void SqliteDevicesDal::AddRouteLink(int deviceID, int routeID, int routeIndex, const DevicePtr neighbour)
{
	sqlitexx::Command sqlCommand(connection, ""
		"INSERT INTO RouteLinks(DeviceID, RouteID, RouteIndex, NeighbourID)"
		" VALUES(?001, ?002, ?003, ?004);");

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, routeID);
	sqlCommand.BindParam(3, routeIndex);
	sqlCommand.BindParam(4, neighbour->id);
	sqlCommand.ExecuteNonQuery();
}
void SqliteDevicesDal::RemoveContractElements(int sourceID)
{
	sqlitexx::Command sqlCommand(connection, ""
		"DELETE FROM ContractElements WHERE SourceDeviceID = ?001;");
	sqlCommand.BindParam(1, sourceID);
	sqlCommand.ExecuteNonQuery();
}
void SqliteDevicesDal::RemoveContractElements()
{
	sqlitexx::Command sqlCommand(connection, ""
		"DELETE FROM ContractElements;");
	sqlCommand.ExecuteNonQuery();
}
void SqliteDevicesDal::AddContractElement(int contractID, int sourceID, int index, int deviceID)
{
	sqlitexx::Command sqlCommand(connection, ""
		"INSERT INTO ContractElements(ContractID, SourceDeviceID, Idx, DeviceID)"
		" VALUES(?001, ?002, ?003, ?004);");

	sqlCommand.BindParam(1, contractID);
	sqlCommand.BindParam(2, sourceID);
	sqlCommand.BindParam(3, index);
	sqlCommand.BindParam(4, deviceID);
	sqlCommand.ExecuteNonQuery();
}


//added by Cristian.Guef
void SqliteDevicesDal::SaveISACSConfirm(int deviceID, ISACSInfo &confirm)
{
	sqlitexx::Command sqlCommand(connection, ""
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
	sqlCommand.BindParam(3, (int)milisec);
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
void SqliteDevicesDal::AddRFChannel(const GScheduleReport::Channel &channel)
{
	sqlitexx::Command sqlCommand(connection, ""
		"INSERT INTO RFChannels(ChannelNumber, ChannelStatus)"
						" VALUES(?001, ?002);");

	sqlCommand.BindParam(1, (int)channel.channelNumber);
	sqlCommand.BindParam(2, (int)channel.channelStatus);
	sqlCommand.ExecuteNonQuery();
}

//added by Cristian.Guef
void SqliteDevicesDal::AddScheduleSuperframe(int deviceID, const GScheduleReport::Superframe &superFrame)
{
	sqlitexx::Command sqlCommand(connection, ""
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
void SqliteDevicesDal::AddScheduleLink(int deviceID, int superframeID, const GScheduleReport::Link &link)
{
	sqlitexx::Command sqlCommand(connection, ""
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
void SqliteDevicesDal::RemoveRFChannels()
{
	sqlitexx::Command sqlCommand(connection, "DELETE FROM RFChannels;");
	sqlCommand.ExecuteNonQuery();
}
//added by Cristian.Guef
void SqliteDevicesDal::RemoveScheduleSuperframes()
{
	sqlitexx::Command sqlCommand(connection, "DELETE FROM DeviceScheduleSuperframes;");
	sqlCommand.ExecuteNonQuery();
}
//added by Cristian.Guef
void SqliteDevicesDal::RemoveScheduleLinks()
{
	sqlitexx::Command sqlCommand(connection, "DELETE FROM DeviceScheduleLinks;");
	sqlCommand.ExecuteNonQuery();
}

void SqliteDevicesDal::RemoveScheduleSuperframes(int deviceID)
{
	sqlitexx::Command sqlCommand(connection, "DELETE FROM DeviceScheduleSuperframes WHERE DeviceID = ?001;");
	sqlCommand.BindParam(1, deviceID);
	sqlCommand.ExecuteNonQuery();
}
void SqliteDevicesDal::RemoveScheduleLinks(int deviceID)
{
	sqlitexx::Command sqlCommand(connection, "DELETE FROM DeviceScheduleLinks WHERE DeviceID = ?001;");
	sqlCommand.BindParam(1, deviceID);
	sqlCommand.ExecuteNonQuery();
}

//added by Cristian.Guef
void SqliteDevicesDal::AddNetworkHealthInfo(const GNetworkHealthReport::NetworkHealth &netHealth)
{
	sqlitexx::Command sqlCommand(connection, ""
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
void SqliteDevicesDal::AddNetHealthDevInfo(const GNetworkHealthReport::NetDeviceHealth &devHealth)
{
	sqlitexx::Command sqlCommand(connection, ""
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
void SqliteDevicesDal::RemoveNetworkHealthInfo()
{
	sqlitexx::Command sqlCommand(connection, "DELETE FROM NetworkHealth;");
	sqlCommand.ExecuteNonQuery();
}
void SqliteDevicesDal::RemoveNetHealthDevInfo()
{
	sqlitexx::Command sqlCommand(connection, "DELETE FROM NetworkHealthDevices;");
	sqlCommand.ExecuteNonQuery();
}

//added by Cristian.Guef
void SqliteDevicesDal::AddDeviceHealthHistory(int deviceID, const nlib::DateTime &timestamp, const GDeviceHealthReport::DeviceHealth &devHeath)
{
	sqlitexx::Command sqlCommand(connection, ""
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
void SqliteDevicesDal::UpdateDeviceHealth(int deviceID, const GDeviceHealthReport::DeviceHealth &devHeath)
{
	sqlitexx::Command
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
void SqliteDevicesDal::AddNeighbourHealthHistory(int deviceID, const nlib::DateTime &timestamp, int neighbID, const GNeighbourHealthReport::NeighbourHealth &neighbHealth)
{
	sqlitexx::Command sqlCommand(connection, ""
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
int SqliteDevicesDal::CreateChannel(int deviceID, PublisherConf::COChannel &channel)
{
	sqlitexx::Command sqlCommand(connection, ""
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
void SqliteDevicesDal::DeleteChannel(int channelNo)
{
	sqlitexx::Command sqlCommand(connection, ""
		"DELETE FROM DeviceChannels "
				"WHERE ChannelNo = ?001");

	sqlCommand.BindParam(1, channelNo);
	sqlCommand.ExecuteNonQuery();
}

void SqliteDevicesDal::DeleteAllChannels()
{
	sqlitexx::Command::ExecuteNonQuery(connection.GetRawDbObj(), "DELETE FROM DeviceChannels;");
	sqlitexx::Command::ExecuteNonQuery(connection.GetRawDbObj(), "DELETE FROM DeviceReadings;"); 
}

void SqliteDevicesDal::GetOrderedChannels(int deviceID, std::vector<PublisherConf::COChannel> &channels)
{
	sqlitexx::CSqliteStmtHelper oSql (connection);

	if (oSql.Prepare( "SELECT ChannelNo, ChannelName, UnitOfMeasurement, ChannelFormat, "
			"SourceTSAPID, SourceObjID, SourceAttrID, SourceIndex1, SourceIndex2, WithStatus "
				"FROM DeviceChannels "
				"WHERE DeviceID = ?001 "
				"ORDER BY SourceTSAPID, SourceObjID, SourceAttrID, SourceIndex1, SourceIndex2;") 
		!= SQLITE_OK)
	{
		return ;
	}

	oSql.BindInt(1, deviceID);

	if (oSql.Step_GetRow() != SQLITE_ROW)
	{
		return ;
	}

	channels.reserve(8);

	do 
	{
		channels.push_back(PublisherConf::COChannel());
		PublisherConf::COChannel& channel = *channels.rbegin();

		channel.dbChannelNo = oSql.Column_GetInt(0);
		channel.name = oSql.Column_GetText(1);
		channel.unitMeasure = oSql.Column_GetText(2);
		channel.format = oSql.Column_GetInt(3);
		channel.tsapID = oSql.Column_GetInt(4);
		channel.objID = oSql.Column_GetInt(5);
		channel.attrID = oSql.Column_GetInt(6);
		channel.index1 = oSql.Column_GetInt(7);
		channel.index2 = oSql.Column_GetInt(8);
		channel.withStatus = oSql.Column_GetInt(9);
	}
	while (oSql.Step_GetRow() == SQLITE_ROW);
}

void SqliteDevicesDal::GetOrderedChannelsDevMACs(std::vector<MAC> &macs)
{
	sqlitexx::CSqliteStmtHelper oSql (connection);

	if (oSql.Prepare( "SELECT Address64 FROM devices WHERE DeviceID IN (SELECT DISTINCT DeviceID FROM DeviceChannels) "
			"ORDER BY Address64;") 
		!= SQLITE_OK)
	{
		return ;
	}

	if (oSql.Step_GetRow() != SQLITE_ROW)
	{
		return ;
	}

	macs.reserve(8);

	do 
	{
		macs.push_back(MAC(oSql.Column_GetText(0)));
	}
	while (oSql.Step_GetRow() == SQLITE_ROW);
}

bool SqliteDevicesDal::GetChannel(int channelNo, int &deviceID, PublisherConf::COChannel &channel)
{

	sqlitexx::CSqliteStmtHelper oSql (connection);

	if (oSql.Prepare(
			"SELECT DeviceID, ChannelName, UnitOfMeasurement, ChannelFormat, "
			"SourceTSAPID, SourceObjID, SourceAttrID, SourceIndex1, SourceIndex2 "
			"FROM DeviceChannels "
			"WHERE ChannelNo = ?001;") 
		!= SQLITE_OK)
	{
		return false;
	}

	oSql.BindInt(1, channelNo);

	if (oSql.Step_GetRow() != SQLITE_ROW)
	{
		return false;
	}

	deviceID =  oSql.Column_GetInt(0);//itDeviceChannel->Value<int>(0);
	
	channel.name = oSql.Column_GetText(1); // itDeviceChannel->Value<std::string>(1);

	channel.unitMeasure = oSql.Column_GetInt(2);//itDeviceChannel->Value<std::string>(2);
	channel.format = oSql.Column_GetInt(3);//itDeviceChannel->Value<int>(3);
	channel.tsapID = oSql.Column_GetInt(4);//itDeviceChannel->Value<int>(4);
	channel.objID = oSql.Column_GetInt(5);//itDeviceChannel->Value<int>(5);
	channel.attrID = oSql.Column_GetInt(6);//itDeviceChannel->Value<int>(6);
	channel.index1 = oSql.Column_GetInt(7);//itDeviceChannel->Value<int>(7);
	channel.index2 = oSql.Column_GetInt(8);//itDeviceChannel->Value<int>(8);

	return true;
}

void SqliteDevicesDal::UpdateChannel(int channelNo, const std::string &channelName, const std::string &unitOfMeasure, int channelFormat, int withStatus)
{
	sqlitexx::Command sqlCommand(connection, ""
		"UPDATE DeviceChannels SET ChannelName = ?001 , "
		"UnitOfMeasurement = ?002, ChannelFormat = ?003, WithStatus = ?004 WHERE ChannelNo = ?005");

	sqlCommand.BindParam(1, channelName);
	sqlCommand.BindParam(2, unitOfMeasure);
	sqlCommand.BindParam(3, (int)channelFormat);
	sqlCommand.BindParam(4, withStatus);
	sqlCommand.BindParam(5, channelNo);
	sqlCommand.ExecuteNonQuery();
}

bool SqliteDevicesDal::FindDeviceChannel(int channelNo, DeviceChannel& channel)
{
	sqlitexx::CSqliteStmtHelper oSql (connection);

	if (oSql.Prepare(       "SELECT ChannelName, ChannelFormat, SourceTSAPID, SourceObjID, SourceAttrID, WithStatus "
							"FROM DeviceChannels "
							"WHERE ChannelNo = ?001;") 
		!= SQLITE_OK)
	{
		return false;
	}

	oSql.BindInt(1, channelNo);


	if (oSql.Step_GetRow() != SQLITE_ROW)
	{
		return false;
	}
	
	channel.channelName = oSql.Column_GetText(0); //itDeviceChannel->Value<std::string>(0);
	channel.channelDataType = (ChannelValue::DataType)oSql.Column_GetInt(1); //(ChannelValue::DataType)itDeviceChannel->Value<int>(1);
	channel.mappedTSAPID = oSql.Column_GetInt(2); //itDeviceChannel->Value<int>(2);
	channel.mappedObjectID = oSql.Column_GetInt(3); //itDeviceChannel->Value<int>(3);
	channel.mappedAttributeID = oSql.Column_GetInt(4); //itDeviceChannel->Value<int>(4);
	channel.withStatus = oSql.Column_GetInt(5);

	//
	channel.channelNumber = channelNo;

	return true;
}

bool SqliteDevicesDal::IsDeviceChannel(int deviceID, const PublisherConf::COChannel &channel)
{
	sqlitexx::CSqliteStmtHelper oSql (connection);

	if (oSql.Prepare(
		"SELECT ChannelNo "
		"FROM DeviceChannels "
		"WHERE DeviceID = ?001 AND SourceTSAPID = ?002 AND SourceObjID = ?003 "
		"AND SourceAttrID = ?004 AND SourceIndex1 = ?005 AND SourceIndex2 = ?006;" ) 
		!= SQLITE_OK)
	{
		return false;
	}
	
	oSql.BindInt(1, deviceID);
	oSql.BindInt(2, (int)channel.tsapID);
	oSql.BindInt(3, (int)channel.objID);
	oSql.BindInt(4, (int)channel.attrID);
	oSql.BindInt(5, (int)channel.index1);
	oSql.BindInt(6, (int)channel.index2);

	if (oSql.Step_GetRow() != SQLITE_ROW)
	{
		return false;
	}

	return true;
}



bool SqliteDevicesDal::HasDeviceChannels(int deviceID)
{
	sqlitexx::CSqliteStmtHelper oSql (connection);

	if (oSql.Prepare(
		"SELECT ChannelNo "
		"FROM DeviceChannels "
		"WHERE DeviceID = ?001;" ) 
		!= SQLITE_OK)
	{
		return false;
	}
	
	oSql.BindInt(1, deviceID);
	if (oSql.Step_GetRow() != SQLITE_ROW)
	{
		return false;
	}

	return true;
}

void SqliteDevicesDal::SetPublishErrorFlag(int deviceID, int flag)
{
	sqlitexx::Command sqlCommand(connection, ""
		"UPDATE Devices SET PublishErrorFlag = ?001 "
		"WHERE DeviceID = ?002");

	sqlCommand.BindParam(1, flag);
	sqlCommand.BindParam(2, deviceID);
	sqlCommand.ExecuteNonQuery();
}

void SqliteDevicesDal::MoveChannelToHistory(int channelNo)
{
	{
		sqlitexx::Command sqlCommand(connection, ""
			"INSERT INTO DeviceChannelsHistory "
			"SELECT DeviceID, ChannelNo, ChannelName, UnitOfMeasurement, "
			"ChannelFormat, SourceTSAPID, SourceObjID, SourceAttrID, "
			"SourceIndex1, SourceIndex2, WithStatus, datetime('now') FROM DeviceChannels WHERE ChannelNo = ?001");

		sqlCommand.BindParam(1, channelNo);
		sqlCommand.ExecuteNonQuery();
	}
	{
		sqlitexx::Command sqlCommand(connection, ""
			"DELETE FROM DeviceChannels WHERE ChannelNo = ?001");

		sqlCommand.BindParam(1, channelNo);
		sqlCommand.ExecuteNonQuery();
	}
}
void SqliteDevicesDal::MoveChannelsToHistory(int deviceID)
{
	{
		sqlitexx::Command sqlCommand(connection, ""
			"INSERT INTO DeviceChannelsHistory "
			"SELECT DeviceID, ChannelNo, ChannelName, UnitOfMeasurement, "
			"ChannelFormat, SourceTSAPID, SourceObjID, SourceAttrID, "
			"SourceIndex1, SourceIndex2, WithStatus, datetime('now') FROM DeviceChannels WHERE DeviceID = ?001");
	
		sqlCommand.BindParam(1, deviceID);
		sqlCommand.ExecuteNonQuery();
	}
	{
		sqlitexx::Command sqlCommand(connection, ""
			"DELETE FROM DeviceChannels WHERE DeviceID = ?001");
	
		sqlCommand.BindParam(1, deviceID);
		sqlCommand.ExecuteNonQuery();
	}
}

bool SqliteDevicesDal::GetAlertProvision(int &categoryProcessSub, int &categoryDeviceSub, int &categoryNetworkSub, int &categorySecuritySub)
{
	sqlitexx::CSqliteStmtHelper oSql (connection);

	if (oSql.Prepare(
		"SELECT CategoryProcess, CategoryDevice, CategoryNetwork, CategorySecurity"
		        " FROM AlertSubscriptionCategory;") 
		!= SQLITE_OK)
	{
		LOG_ERROR("ERROR GetAlertProvision: cannot prepare");
		return false;
	}

	if (oSql.Step_GetRow() != SQLITE_ROW)
	{
		LOG_ERROR("ERROR GetAlertProvision: cannot read");
		return false;
	}

	categoryProcessSub = oSql.Column_GetInt(0); 
	categoryDeviceSub = oSql.Column_GetInt(1); 
	categoryNetworkSub = oSql.Column_GetInt(2); 
	categorySecuritySub = oSql.Column_GetInt(3); 

	return true;
}

void SqliteDevicesDal::SaveAlertInfo(int devID, int TSAPID, int objID, const nlib::DateTime &timestamp, int milisec, 
							int classType, int direction, int category, int type, int priority, std::string &data)
{
	if (data.size() > 0)
	{
		sqlitexx::Command sqlCommand(connection, ""
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
		sqlitexx::Command sqlCommand(connection, ""
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

void SqliteDevicesDal::DeleteChannelsStatistics(int deviceID)
{
	sqlitexx::Command sqlCommand(connection, ""
			"DELETE FROM ChannelsStatistics WHERE DeviceID = ?001");
	
		sqlCommand.BindParam(1, deviceID);
		sqlCommand.ExecuteNonQuery();
}
void SqliteDevicesDal::SaveChannelsStatistics(int deviceID, const std::string& strChannelsStatistics)
{
	sqlitexx::Command sqlCommand(connection, ""
		"INSERT INTO ChannelsStatistics(DeviceID, ByteChannelsArray)"
						" VALUES(?001, ?002);");

	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, strChannelsStatistics);
	sqlCommand.ExecuteNonQuery();
	
	return;
}

void SqliteDevicesDal::ResetPendingFirmwareUploads()
{
	sqlitexx::Command sqlCommand(connection, ""
		"UPDATE Firmwares SET UploadStatus = ?001 , LastFailedUploadTime = ?002 WHERE UploadStatus = ?003");
	sqlCommand.BindParam(1, (int)FirmwareUpload::fusWaitRetry);
	sqlCommand.BindParam(2, nlib::CurrentUniversalTime());
	sqlCommand.BindParam(3, (int)FirmwareUpload::fusUploading);

	sqlCommand.ExecuteNonQuery();
}

bool SqliteDevicesDal::GetNextFirmwareUpload(int notFailedForMinutes, FirmwareUpload& result)
{
	sqlitexx::CSqliteStmtHelper oSql (connection);

	if (oSql.Prepare(
		"SELECT FirmwareID, FileName, UploadStatus, UploadRetryCount, LastFailedUploadTime"
		" FROM Firmwares"
		" WHERE ((LastFailedUploadTime IS NULL OR LastFailedUploadTime = \"no date\" OR LastFailedUploadTime = \"no time\")"
		" OR (LastFailedUploadTime < ?001)) "
		"				AND (UploadStatus = ?002 OR UploadStatus = ?003)") 
		!= SQLITE_OK)
	{
		return false;
	}

	oSql.BindDateTime(1, nlib::CurrentUniversalTime() - nlib::util::minutes(notFailedForMinutes));
	oSql.BindInt(2, (int)FirmwareUpload::fusNew);
	oSql.BindInt(3, (int)FirmwareUpload::fusWaitRetry);

	if (oSql.Step_GetRow() != SQLITE_ROW)
	{
		return false;
	}

	result.Id = (boost::uint32_t)oSql.Column_GetInt(0); //(boost::uint32_t)resultSet->Begin()->Value<int>(0);
	result.FileName= oSql.Column_GetText(1);//resultSet->Begin()->Value<std::string>(1);
	result.Status = (FirmwareUpload::FirmwareUploadStatus)oSql.Column_GetInt(2); //(FirmwareUpload::FirmwareUploadStatus)resultSet->Begin()->Value<int>(2);
	result.RetriesCount = oSql.Column_GetInt(3);//resultSet->Begin()->Value<int>(3);
	if (!oSql.Column_IsNull(4)) //if (!resultSet->Begin()->IsNull(4))
	{
		result.LastFailedUploadTime = oSql.Column_GetDateTime(4); //resultSet->Begin()->ValueDateTime(4);
	}

	return true;
}

void SqliteDevicesDal::UpdateFirmwareUpload(const FirmwareUpload& firmware)
{
	sqlitexx::Command sqlCommand(connection, ""
		"UPDATE Firmwares SET FileName = ?001, UploadStatus = ?002, UploadRetryCount = ?003, LastFailedUploadTime = ?004"
		" WHERE FirmwareID = ?005");
	sqlCommand.BindParam(1, firmware.FileName);
	sqlCommand.BindParam(2, (int)firmware.Status);
	sqlCommand.BindParam(3, (int)firmware.RetriesCount);
	sqlCommand.BindParam(4, firmware.LastFailedUploadTime);
	sqlCommand.BindParam(5, (int)firmware.Id);

	sqlCommand.ExecuteNonQuery();
}

bool SqliteDevicesDal::GetFirmwareUpload(boost::uint32_t id, FirmwareUpload& result)
{

	sqlitexx::CSqliteStmtHelper oSql (connection);

	if (oSql.Prepare(
		"SELECT FirmwareID, FileName, UploadStatus, UploadRetryCount, LastFailedUploadTime"
		" FROM Firmwares "
		" WHERE FirmwareID = ?001;") 
		!= SQLITE_OK)
	{
		return false;
	}

	oSql.BindInt(1, id); 	//sqlCommand.BindParam(1, (int)id);

	if (oSql.Step_GetRow() != SQLITE_ROW)
	{
		return false;
	}

	result.Id = (boost::uint32_t)oSql.Column_GetInt(0); //(boost::uint32_t)resultSet->Begin()->Value<int>(0);
	result.FileName= oSql.Column_GetText(1);//resultSet->Begin()->Value<std::string>(1);
	result.Status = (FirmwareUpload::FirmwareUploadStatus)oSql.Column_GetInt(2); //(FirmwareUpload::FirmwareUploadStatus)resultSet->Begin()->Value<int>(2);
	result.RetriesCount = oSql.Column_GetInt(3);//resultSet->Begin()->Value<int>(3);
	if (!oSql.Column_IsNull(4)) //if (!resultSet->Begin()->IsNull(4))
	{
		result.LastFailedUploadTime = oSql.Column_GetDateTime(4); //resultSet->Begin()->ValueDateTime(4);
	}

	return true;
}

void SqliteDevicesDal::GetFirmwareUploads(FirmwareUploadsT& firmwaresList)
{

	sqlitexx::CSqliteStmtHelper oSql (connection);

	if (	oSql.Prepare("SELECT FirmwareID, FileName, UploadStatus, UploadRetryCount, LastFailedUploadTime FROM Firmwares; ") 
			!= SQLITE_OK)
	{
		return;
	}


	while (oSql.Step_GetRow() == SQLITE_ROW)
	{
		firmwaresList.push_back(FirmwareUpload());
		FirmwareUpload& result = *firmwaresList.rbegin();

		result.Id = (boost::uint32_t)oSql.Column_GetInt(0); //(boost::uint32_t)resultSet->Begin()->Value<int>(0);
		result.FileName= oSql.Column_GetText(1);//resultSet->Begin()->Value<std::string>(1);
		result.Status = (FirmwareUpload::FirmwareUploadStatus) oSql.Column_GetInt(2); //(FirmwareUpload::FirmwareUploadStatus)resultSet->Begin()->Value<int>(2);
		result.RetriesCount = oSql.Column_GetInt(3);//resultSet->Begin()->Value<int>(3);
		if (!oSql.Column_IsNull(4)) //if (!resultSet->Begin()->IsNull(4))
		{
			result.LastFailedUploadTime = oSql.Column_GetDateTime(4); //resultSet->Begin()->ValueDateTime(4);
		}
	}
}

void SqliteDevicesDal::AddDeviceConnections(int deviceID, const std::string& host, int port)
{
	sqlitexx::Command sqlCommand(connection, ""
		"INSERT OR REPLACE INTO DeviceConnections(DeviceID, IP, Port)"
		" VALUES(?001, ?002, ?003);");
	sqlCommand.BindParam(1, deviceID);
	sqlCommand.BindParam(2, host);
	sqlCommand.BindParam(3, port);
	sqlCommand.ExecuteNonQuery();
}

void SqliteDevicesDal::RemoveDeviceConnections(int deviceID)
{
	sqlitexx::Command sqlCommand(connection, ""
		"DELETE FROM DeviceConnections "
		" WHERE DeviceID = ?001;");
	sqlCommand.BindParam(1, deviceID);
	sqlCommand.ExecuteNonQuery();

}

//firmwaredownloads
static const char* gettime( void )
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

/// @remarks only set the status for transfers in progress or cancelling
/// @remarks only set the completion date whre is not already set (to cope with multiple transfer end messages)
/// @note 	FwStatus is: FirmDlStatus_Progress = 1,FirmDlStatus_Cancelling = 2
/// @note FirmDlStatus_Cancelled = 3, FirmDlStatus_Completed = 4, FirmDlStatus_Failed = 5
void SqliteDevicesDal::SetFirmDlStatus(int deviceID, int fwType, int status)
{
	LOG_ERROR("SetFirmDlStatus(id " << deviceID << ", fw_type " << fwType << ", status " << status << ")");
	int res = 0;
	if (!m_oUpdateFirmDl_compiled_status.GetStmt())
	{
		if ((res=m_oUpdateFirmDl_compiled_status.Prepare(
			"UPDATE FirmwareDownloads SET FwStatus = ?, CompletedOn = ? WHERE DeviceID = ? AND FwType = ? AND FwStatus IN (1,2) AND CompletedOn IS NULL AND StartedOn = (SELECT MAX(StartedOn) FROM FirmwareDownloads WHERE DeviceID = ? AND FwType = ? AND FwStatus IN (1,2) AND CompletedOn IS NULL)")) != SQLITE_OK)
		{
			connection.LogIfLastError(res);
			LOG_ERROR("SqliteDevicesDal::SetFirmDlStatus: prepare FAILED err=" << res);
			std::exception ex;
			throw ex;
		}
	}

	if( ((res=m_oUpdateFirmDl_compiled_status.BindInt(1,status))    != SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_status.BindText(2,gettime())) != SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_status.BindInt(3,deviceID))  != SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_status.BindInt(4,fwType))    != SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_status.BindInt(5,deviceID))  != SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_status.BindInt(6,fwType))    != SQLITE_OK) )
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::SetFirmDlStatus: bind FAILED err=" << res);
		std::exception ex;
		throw ex;
	}


	if ((res=m_oUpdateFirmDl_compiled_status.Step_Exec())!= SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::SetFirmDlStatus: exec FAILED err=" << res);
		std::exception ex;
		throw ex;
	}
}
void SqliteDevicesDal::SetFirmDlPercent(int deviceID, int fwType, int percent)
{
	int res = 0;
	if (!m_oUpdateFirmDl_compiled_percent.GetStmt())
	{
		if ((res=m_oUpdateFirmDl_compiled_percent.Prepare(
			"UPDATE FirmwareDownloads SET CompletedPercent = ? WHERE DeviceID = ? AND FwType = ? AND StartedOn = (SELECT MAX(StartedOn) FROM FirmwareDownloads WHERE DeviceID = ? AND FwType = ? )")) != SQLITE_OK)
		{
			connection.LogIfLastError(res);
			LOG_ERROR("SqliteDevicesDal::SetFirmDlPercent: prepare FAILED err=" << res);
			std::exception ex;
			throw ex;
		}
	}

	if( ((res=m_oUpdateFirmDl_compiled_percent.BindInt(1,percent)) != SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_percent.BindInt(2,deviceID))!= SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_percent.BindInt(3,fwType))  != SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_percent.BindInt(4,deviceID))!= SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_percent.BindInt(5,fwType))  != SQLITE_OK) )
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::SetFirmDlPercent: bind FAILED err=" << res);
		std::exception ex;
		throw ex;
	}

	if ((res=m_oUpdateFirmDl_compiled_percent.Step_Exec())!= SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::SetFirmDlPercent: exec FAILED err=" << res);
		std::exception ex;
		throw ex;
	}
}
void SqliteDevicesDal::SetFirmDlSize(int deviceID, int fwType, int size)
{
	int res = 0;
	if (!m_oUpdateFirmDl_compiled_size.GetStmt())
	{
		if ((res=m_oUpdateFirmDl_compiled_size.Prepare(
			"UPDATE FirmwareDownloads SET DownloadSize = ? WHERE DeviceID = ? AND FwType = ? AND StartedOn = (SELECT MAX(StartedOn) FROM FirmwareDownloads WHERE DeviceID = ? AND FwType = ?)")) != SQLITE_OK)
		{
			connection.LogIfLastError(res);
			LOG_ERROR("SqliteDevicesDal::SetFirmDlSize: prepare FAILED err=" << res);
			std::exception ex;
			throw ex;
		}
	}

	if( ((res=m_oUpdateFirmDl_compiled_size.BindInt(1,size))       != SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_size.BindInt(2,deviceID))!= SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_size.BindInt(3,fwType))  != SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_size.BindInt(4,deviceID))!= SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_size.BindInt(5,fwType))  != SQLITE_OK) )
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::SetFirmDlSize: bind FAILED err=" << res);
		std::exception ex;
		throw ex;
	}

	if ((res=m_oUpdateFirmDl_compiled_size.Step_Exec())!= SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::SetFirmDlSize: exec FAILED err=" << res);
		std::exception ex;
		throw ex;
	}
}
void SqliteDevicesDal::SetFirmDlSpeed(int deviceID, int fwType, int speed)
{
	int res = 0;
	if (!m_oUpdateFirmDl_compiled_speed.GetStmt())
	{
		if ((res=m_oUpdateFirmDl_compiled_speed.Prepare(
			"UPDATE FirmwareDownloads SET Speed = ? WHERE DeviceID = ? AND FwType = ? AND StartedOn = (SELECT MAX(StartedOn) FROM FirmwareDownloads WHERE DeviceID = ? AND FwType = ?)")) != SQLITE_OK)
		{
			connection.LogIfLastError(res);
			LOG_ERROR("SqliteDevicesDal::SetFirmDlSpeed: prepare FAILED err=" << res);
			std::exception ex;
			throw ex;
		}
	}

	if( ((res=m_oUpdateFirmDl_compiled_speed.BindInt(1,speed))    != SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_speed.BindInt(2,deviceID))!= SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_speed.BindInt(3,fwType))  != SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_speed.BindInt(4,deviceID))!= SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_speed.BindInt(5,fwType))  != SQLITE_OK) )
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::SetFirmDlSpeed: bind FAILED err=" << res);
		std::exception ex;
		throw ex;
	}

	if ((res=m_oUpdateFirmDl_compiled_speed.Step_Exec())!= SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::SetFirmDlSpeed: exec FAILED err=" << res);
		std::exception ex;
		throw ex;
	}
}
void SqliteDevicesDal::SetFirmDlAvgSpeed(int deviceID, int fwType, int speed)
{
	int res = 0;
	if (!m_oUpdateFirmDl_compiled_avgspeed.GetStmt())
	{
		if ((res=m_oUpdateFirmDl_compiled_avgspeed.Prepare(
			"UPDATE FirmwareDownloads SET AvgSpeed = ? WHERE DeviceID = ? AND FwType = ? AND StartedOn = (SELECT MAX(StartedOn) FROM FirmwareDownloads WHERE DeviceID = ? AND FwType = ?)")) != SQLITE_OK)
		{
			connection.LogIfLastError(res);
			LOG_ERROR("SqliteDevicesDal::SetFirmDlAvgSpeed: prepare FAILED prepare err=" << res);
			std::exception ex;
			throw ex;
		}
	}

	if( ((res=m_oUpdateFirmDl_compiled_avgspeed.BindInt(1,speed))    != SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_avgspeed.BindInt(2,deviceID))!= SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_avgspeed.BindInt(3,fwType))  != SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_avgspeed.BindInt(4,deviceID))!= SQLITE_OK)
	||	((res=m_oUpdateFirmDl_compiled_avgspeed.BindInt(5,fwType))  != SQLITE_OK) )
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::SetFirmDlAvgSpeed: bind FAILED err=" << res);
		std::exception ex;
		throw ex;
	}

	if ((res=m_oUpdateFirmDl_compiled_avgspeed.Step_Exec())!= SQLITE_OK)
	{
		connection.LogIfLastError(res);
		LOG_ERROR("SqliteDevicesDal::SetFirmDlAvgSpeed: exec FAILED err=" << res);
		std::exception ex;
		throw ex;
	}
}


}//namespace hostapp
}//namespace nisa100
