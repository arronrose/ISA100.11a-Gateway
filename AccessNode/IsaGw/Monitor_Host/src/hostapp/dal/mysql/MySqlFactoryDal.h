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


#ifndef MYSQLFACTORYDAL_H_
#define MYSQLFACTORYDAL_H_

#include "../IFactoryDal.h"
#include "MySQLDatabase.h"

#include "MySqlCommandsDal.h"
#include "MySqlDevicesDal.h"

namespace nisa100 {
namespace hostapp {

class MySqlFactoryDal : public IFactoryDal
{
	LOG_DEF("nisa100.hostapp.MySqlFactoryDal");
public:
	MySqlFactoryDal();
	virtual ~MySqlFactoryDal();

	void Open(const std::string& serverName, const std::string& user, const std::string& password, const std::string& dbName, int timeoutSeconds = 10);

	//added by Cristian.Guef
	void Reconnect();

	void VacuumDatabase();
	void CleanupOldRecords(nlib::DateTime olderThanDate, nlib::DateTime olderThanAlertDate);
	void CleanupOldAlarmsAndFW(nlib::DateTime olderThanDate);

private:
	void BeginTransaction();
	void CommitTransaction();
	void RollbackTransaction();

	bool IsTransactionOn();

	void VerifyDb();
private:
	ICommandsDal& Commands() const;
	IDevicesDal& Devices() const;

	MySQLConnection connection;
	MySQLTransaction transaction;

	MySqlCommandsDal commandsDal;
	MySqlDevicesDal	devicesDal;

//added by Cristian.guef
private:
	std::string m_serverName;
	std::string m_user;
	std::string m_password;
    std::string m_dbName;
public:
	int m_timeoutSeconds;

private:
	bool m_isTransactionOn;

};

} //namespace hostapp
} //namespace nisa100


#endif /*MYSQLFACTORYDAL_H_*/

#endif
