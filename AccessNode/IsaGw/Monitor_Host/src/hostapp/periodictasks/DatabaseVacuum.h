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

#ifndef DATABASEVACUUM_H_
#define DATABASEVACUUM_H_

#include "../dal/IFactoryDal.h"
#include "../../ConfigApp.h"

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../../Log.h"

#include <nlib/datetime.h>

namespace nisa100 {
namespace hostapp {

class DatabaseVacuum
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.DatabaseVacuum");
	*/

public:
	DatabaseVacuum (IFactoryDal& factoryDal_, const ConfigApp& configApp_, DevicesManager& devices_)
	: factoryDal(factoryDal_), configApp(configApp_), devices(devices_)
	{
		periodVacuumHours = configApp.DatabaseVacuumPeriod();
		periodCleanupHours = configApp.DatabaseRemoveEntriesPeriod();
		removeOlderThanHours = configApp.DatabaseRemoveEntriesOlderThan();
		removeAlertOlderThanHours = configApp.DatabaseRemoveAlarmEntriesOlderThan();

		canBeVacuumed = false;
		if(periodVacuumHours >= nlib::NOTIME)
		{
			canBeVacuumed = true;
		}

		lastVacuumTime = nlib::CurrentUniversalTime() - periodVacuumHours;
		lastCleanupTime = nlib::CurrentUniversalTime() - periodCleanupHours;
		LOG_INFO(".ctor: Vacuuming db at a period of " << nlib::ToString(periodVacuumHours) <<
				" and rm_older_than_hours = " << nlib::ToString(removeOlderThanHours)
				<< "meaning checking for older than = " 
				<< nlib::ToString(nlib::CurrentUniversalTime() - removeOlderThanHours)
				<< " at period of " << nlib::ToString(periodCleanupHours));
	}

	~DatabaseVacuum(void)
	{}

	void Vacuum()
	{
		//process vacuum when gw has made alert subscription for device_join/leave
		DevicePtr gwDev = devices.GatewayDevice();
		if (!gwDev->HasMadeAlertSubscription())
		{
			LOG_INFO("VACUUM_TASK: GW hasn't made alert_subscription, so skip vacuum...");
			return;
		}

		nlib::DateTime currentTime = nlib::CurrentUniversalTime();
		if ((periodCleanupHours> nlib::NOTIME) &&
				(currentTime> lastCleanupTime + periodCleanupHours))
		{
			try
			{
				LOG_INFO("Removing old entries from DB...");
				factoryDal.CleanupOldRecords(currentTime - removeOlderThanHours, currentTime - removeAlertOlderThanHours);
				LOG_INFO("Finished removing old entries from DB!");
			}
			catch(std::exception& ex)
			{
				LOG_WARN("Vacuum: An error occured while removing old entries from the database. error=" << ex.what());
			}

			lastCleanupTime = currentTime;
		}
		if (canBeVacuumed && (currentTime> lastVacuumTime + periodVacuumHours))
		{
			try
			{
				//LOG_INFO("Vacuuming the database...");
				//factoryDal.VacuumDatabase();
				//LOG_INFO("Vacuuming finished!");
			}
			catch(std::exception& ex)
			{
				LOG_WARN("Vacuum: An error occured while vacuuming the database. error=" << ex.what());
			}
			if (periodVacuumHours == nlib::NOTIME) //just at first run db should be vaccumed

			{
				canBeVacuumed = false;
			}

			lastVacuumTime = currentTime;
		}
		
	}

private:
	IFactoryDal& factoryDal;
	const ConfigApp& configApp;
	nlib::DateTime lastVacuumTime;
	nlib::DateTime lastCleanupTime;

	nlib::TimeSpan periodVacuumHours;
	nlib::TimeSpan periodCleanupHours;
	nlib::TimeSpan removeOlderThanHours;
	nlib::TimeSpan removeAlertOlderThanHours;

	bool canBeVacuumed;
	DevicesManager& devices;
};

}
}
#endif /*DATABASEVACUUM_H_*/
