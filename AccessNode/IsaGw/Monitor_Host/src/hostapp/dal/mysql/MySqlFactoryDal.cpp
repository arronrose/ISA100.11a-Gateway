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


#include "MySqlFactoryDal.h"


namespace nisa100 {
namespace hostapp {

extern bool TransactionStarted;

MySqlFactoryDal::MySqlFactoryDal() :
	transaction(connection), commandsDal(connection), devicesDal(connection)
{
	m_isTransactionOn = false;
}

MySqlFactoryDal::~MySqlFactoryDal()
{
	connection.Close();
}

void MySqlFactoryDal::VerifyDb()
{
	commandsDal.VerifyTables();
	devicesDal.VerifyTables();
	LOG_DEBUG("Tables structure ok!");
}

void MySqlFactoryDal::BeginTransaction()
{
	m_isTransactionOn = true;
	transaction.Begin();
}

void MySqlFactoryDal::CommitTransaction()
{
	transaction.Commit();
	m_isTransactionOn = false;
}

void MySqlFactoryDal::RollbackTransaction()
{
	transaction.Rollback();
	m_isTransactionOn = false;
}

bool MySqlFactoryDal::IsTransactionOn()
{
	return m_isTransactionOn;
}

void MySqlFactoryDal::Open(const std::string& serverName, const std::string& user, const std::string& password,
  const std::string& dbName, int timeoutSeconds)
{

	//added by Cristian.Guef
	m_serverName = serverName;
	m_user = user;
	m_password = password;
	m_dbName = dbName;
	m_timeoutSeconds = timeoutSeconds;

	/*commented by Cristian.Guef
	LOG_DEBUG("Conecting to server:" << serverName << " database:" << dbName);
	*/
	//added by Cristian .Guef
	LOG_INFO("Conecting to server:" << serverName << " database:" << dbName);

	connection.ConnectionString(serverName, user, password, dbName);
	connection.Open();

	/* commented by Cristian.Guef
	LOG_DEBUG("Connection opened...");
	*/
	//added by Cristian.Guef
	LOG_INFO("Connection opened...");

	VerifyDb();
}

//added by Cristian.Guef
void MySqlFactoryDal::Reconnect()
{

	connection.Close();
	LOG_INFO("Reconecting to server:" << m_serverName << " database:" << m_dbName);
	connection.ConnectionString(m_serverName, m_user, m_password, m_dbName);
	connection.Open();
	LOG_INFO("Connection opened...");
	VerifyDb();
}


ICommandsDal& MySqlFactoryDal::Commands() const
{
	MySqlFactoryDal* pThis = const_cast<MySqlFactoryDal*>(this);
	return pThis->commandsDal;
}

IDevicesDal& MySqlFactoryDal::Devices() const
{
	MySqlFactoryDal* pThis = const_cast<MySqlFactoryDal*>(this);
	return pThis->devicesDal;
}

void MySqlFactoryDal::VacuumDatabase()
{
	/* commented by Cristian.Guef -mysql doesn't support VACUUM
	std::string vacuum = "VACUUM";
	MySQLCommand(connection, vacuum).ExecuteNonQuery();
	*/

	/*
	//added by Cristian.Guef
	{
		std::string optimizeQuery = "OPTIMIZE TABLE CommandParameters";
		MySQLCommand command(connection, optimizeQuery);
		try
		{
			BeginTransaction();
			command.ExecuteNonQuery();
			CommitTransaction();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("CleanupOldRecords failed! error=" << ex.what());
			RollbackTransaction();
			throw;
		}
		catch(...)
		{
			RollbackTransaction();
			throw;
		}
	}

	//added by Cristian.Guef
	{
		std::string optimizeQuery = "OPTIMIZE TABLE Commands";
		MySQLCommand command(connection, optimizeQuery);
		try
		{
			BeginTransaction();
			command.ExecuteNonQuery();
			CommitTransaction();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("CleanupOldRecords failed! error=" << ex.what());
			RollbackTransaction();
			throw;
		}
		catch(...)
		{
			RollbackTransaction();
			throw;
		}
	}

	//added by Cristian.Guef
	{
		std::string optimizeQuery = "OPTIMIZE TABLE DeviceReadings";
		MySQLCommand command(connection, optimizeQuery);
		try
		{
			BeginTransaction();
			command.ExecuteNonQuery();
			CommitTransaction();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("CleanupOldRecords failed! error=" << ex.what());
			RollbackTransaction();
			throw;
		}
		catch(...)
		{
			RollbackTransaction();
			throw;
		}
	}

	//added by Cristian.Guef
	{
		std::string optimizeQuery = "OPTIMIZE TABLE DeviceBatteryStatistics";
		MySQLCommand command(connection, optimizeQuery);
		try
		{
			BeginTransaction();
			command.ExecuteNonQuery();
			CommitTransaction();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("CleanupOldRecords failed! error=" << ex.what());
			RollbackTransaction();
			throw;
		}
		catch(...)
		{
			RollbackTransaction();
			throw;
		}
	}

	//added by Cristian.Guef
	{
		std::string optimizeQuery = "FLUSH TABLES CommandParameters, Commands, DeviceReadings, DeviceBatteryStatistics";
		MySQLCommand command(connection, optimizeQuery);
		try
		{
			BeginTransaction();
			command.ExecuteNonQuery();
			CommitTransaction();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Flush tables failed! error=" << ex.what());
			RollbackTransaction();
			throw;
		}
		catch(...)
		{
			RollbackTransaction();
			throw;
		}
	}
	*/
}

void MySqlFactoryDal::CleanupOldRecords(nlib::DateTime olderThanDate, nlib::DateTime olderThanAlertDate)
{
	nlib::DateTime currTime = nlib::CurrentUniversalTime();

	const char *pszQry[] ={
		"DELETE FROM CommandParameters WHERE CommandID IN (SELECT C.CommandID from Commands C WHERE C.TimePosted < \"%s\");",	// +24*7
		"DELETE FROM Commands WHERE TimePosted < \"%s\";",	// +24*7
		"DELETE FROM CommandParameters WHERE CommandID IN (SELECT C.CommandID from Commands C WHERE C.TimePosted < \"%s\" AND (C.CommandStatus = 2 OR C.CommandStatus = 3));",
		"DELETE FROM Commands WHERE TimePosted < \"%s\" AND (CommandStatus = 2 OR CommandStatus = 3);",
		"DELETE FROM DeviceReadingsHistory WHERE ReadingTime < \"%s\";",
		//there is other approach
		//"DELETE FROM DeviceHistory WHERE Timestamp < \"%s\";",
		"DELETE FROM DeviceHealthHistory WHERE Timestamp < \"%s\";",
		"DELETE FROM NeighborHealthHistory WHERE Timestamp < \"%s\";",
		"DELETE FROM AlertNotifications WHERE AlertTime < \"%s\";",
		"DELETE FROM DeviceChannelsHistory WHERE Timestamp < \"%s\";",
		"DELETE FROM ISACSConfirmDataBuffer WHERE Timestamp < \"%s\";",
		"DELETE FROM Firmwares WHERE UploadStatus = 5 AND UploadDate < \"%s\";"
	};

	LOG("DEBUG CleanupOldRecords START [%s] [%s]", nlib::ToString(olderThanDate).c_str(), nlib::ToString(currTime - nlib::util::hours(24*7)).c_str());
	for( int i = 0; i< sizeof(pszQry) / sizeof(pszQry[0]); ++i)
	{	char szQry[ 1024 ];
		snprintf( szQry, sizeof(szQry), pszQry[i], (i>=2)
			? nlib::ToString(olderThanDate).c_str() 
			: (nlib::ToString(currTime - nlib::util::hours(24*7)).c_str()) );
		szQry[ sizeof(szQry)-1 ] = 0;
		std::string obQry( szQry );
		MySQLCommand command(connection, obQry);
		try
		{
			BeginTransaction();
			command.ExecuteNonQuery();
			CommitTransaction();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("CATCH(exception): CleanupOldRecords failed at step=" << i << " error=" << ex.what());
			RollbackTransaction();
		}
		catch(...)
		{
			LOG_ERROR("CATCH(...): CleanupOldRecords failed at step=" << i );
			RollbackTransaction();
		}
	}
	LOG_STR("DEBUG CleanupOldRecords END");

}

/// delete firmware transfers when older than 7 days, regardless of the download status
/// TODO: keep application alerts longer
void MySqlFactoryDal::CleanupOldAlarmsAndFW(nlib::DateTime olderThanDate)
{
	nlib::DateTime currTime = nlib::CurrentUniversalTime();

	const char *pszQry[] ={
		"DELETE FROM FirmwareDownloads WHERE StartedOn < \"%s\";",	// +24*7
		"DELETE FROM AlertNotifications WHERE AlertTime < \"%s\";" };

		LOG("DEBUG CleanupOldRecords for Alarms and FW START [%s] [%s]", nlib::ToString(olderThanDate).c_str(), nlib::ToString(currTime - nlib::util::hours(24*1)).c_str());
		for( int i = 0; i< sizeof(pszQry) / sizeof(pszQry[0]); ++i)
		{	char szQry[ 1024 ];
		snprintf( szQry, sizeof(szQry), pszQry[i], (i>=1)
			? nlib::ToString(olderThanDate).c_str() 
			: (nlib::ToString(currTime - nlib::util::hours(24*7)).c_str()) );
		szQry[ sizeof(szQry)-1 ] = 0;
		std::string obQry( szQry );
		MySQLCommand command(connection, obQry);
		try
		{
			BeginTransaction();
			command.ExecuteNonQuery();
			CommitTransaction();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("CATCH(exception): CleanupOldRecords failed at step=" << i << " error=" << ex.what());
			RollbackTransaction();
		}
		catch(...)
		{
			LOG_ERROR("CATCH(...): CleanupOldRecords failed at step=" << i );
			RollbackTransaction();
		}
		}
		LOG_STR("DEBUG CleanupOldRecords for Alarms and FW END");
}

} // namespace hostapp
} // namespace nisa100

#endif
