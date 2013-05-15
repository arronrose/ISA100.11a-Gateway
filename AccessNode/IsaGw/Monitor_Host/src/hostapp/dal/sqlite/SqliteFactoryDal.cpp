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

#include "SqliteFactoryDal.h"

namespace nisa100 {
namespace hostapp {

extern bool TransactionStarted;

SqliteFactoryDal::SqliteFactoryDal() :
	transaction(connection), commandsDal(connection), devicesDal(connection)
{
}

SqliteFactoryDal::~SqliteFactoryDal()
{
	connection.Close();
}

void SqliteFactoryDal::VerifyDb()
{
	commandsDal.VerifyTables();
	devicesDal.VerifyTables();
}

void SqliteFactoryDal::BeginTransaction()
{
	m_isTransactionOn = true;
	transaction.Begin();
}

void SqliteFactoryDal::CommitTransaction()
{
	transaction.Commit();
	m_isTransactionOn = false;
}

void SqliteFactoryDal::RollbackTransaction()
{
	transaction.Rollback();
	m_isTransactionOn = false;
}

bool SqliteFactoryDal::IsTransactionOn()
{
	return m_isTransactionOn;
}

void SqliteFactoryDal::Open(const std::string& dbPath, int timeoutSeconds)
{
	connection.Open(dbPath, timeoutSeconds);
	VerifyDb();
}

ICommandsDal& SqliteFactoryDal::Commands() const
{
	SqliteFactoryDal* pThis = const_cast<SqliteFactoryDal*>(this);
	return pThis->commandsDal;
}

IDevicesDal& SqliteFactoryDal::Devices() const
{
	SqliteFactoryDal* pThis = const_cast<SqliteFactoryDal*>(this);
	return pThis->devicesDal;
}

void SqliteFactoryDal::VacuumDatabase()
{
	std::string vacuum = "VACUUM";
	sqlitexx::Command(connection, vacuum).ExecuteNonQuery();
}

/// @remarks delete commands even if no response is received after 7 days
void SqliteFactoryDal::CleanupOldRecords(nlib::DateTime olderThanDate, nlib::DateTime olderThanAlertDate)
{
	nlib::DateTime currTime = nlib::CurrentUniversalTime();

	const char *pszQry[] ={
		"DELETE FROM CommandParameters WHERE CommandID IN (SELECT C.CommandID from Commands C WHERE C.TimePosted < \"%s\");",	// +24*7
		"DELETE FROM Commands WHERE TimePosted < \"%s\";",			// +24*7
		"DELETE FROM FirmwareDownloads WHERE StartedOn < \"%s\";",	// +24*7

		"DELETE FROM AlertNotifications WHERE AlertTime < \"%s\";", //olderThanAlertDate

		"DELETE FROM CommandParameters WHERE CommandID IN (SELECT C.CommandID from Commands C WHERE C.TimePosted < \"%s\" AND (C.CommandStatus = 2 OR C.CommandStatus = 3));",
		"DELETE FROM Commands WHERE TimePosted < \"%s\" AND (CommandStatus = 2 OR CommandStatus = 3);",
		"DELETE FROM DeviceReadingsHistory WHERE ReadingTime < \"%s\";",
		//there is other approach
		//"DELETE FROM DeviceHistory WHERE Timestamp < \"%s\";",
		"DELETE FROM DeviceHealthHistory WHERE Timestamp < \"%s\";",
		"DELETE FROM NeighborHealthHistory WHERE Timestamp < \"%s\";",
		"DELETE FROM DeviceChannelsHistory WHERE Timestamp < \"%s\";",
		"DELETE FROM ISACSConfirmDataBuffer WHERE Timestamp < \"%s\";",
		"DELETE FROM AlertNotifications WHERE (Category != 3) AND (AlertTime < \"%s\");",
		"DELETE FROM Firmwares WHERE UploadStatus = 5 AND UploadDate < \"%s\";"};//all non-application alerts

	LOG("DEBUG CleanupOldRecords START [%s] [%s]", nlib::ToString(olderThanDate).c_str(), nlib::ToString(currTime - nlib::util::hours(24*7)).c_str());
	for( int i = 0; i< sizeof(pszQry) / sizeof(pszQry[0]); ++i)
	{	char szQry[ 1024 ];
		snprintf( szQry, sizeof(szQry), pszQry[i],
			(i<3)
				? (nlib::ToString(currTime - nlib::util::hours(24*7)).c_str())
				: (i == 3)
					? nlib::ToString(olderThanAlertDate).c_str()
					: nlib::ToString(olderThanDate).c_str() );
		szQry[ sizeof(szQry)-1 ] = 0;
		std::string obQry( szQry );
		sqlitexx::Command command(connection, obQry);
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

} // namespace hostapp
} // namespace nisa100
