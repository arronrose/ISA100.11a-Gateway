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

#ifndef DEVICEREADINGSBULKSAVER_H
#define DEVICEREADINGSBULKSAVER_H

#include "../dal/IFactoryDal.h"
#include "../../ConfigApp.h"
#include <nlib/datetime.h>


namespace nisa100 {
namespace hostapp {


//added
extern int GetCurrentReadingsNo();
extern void SaveCurrentReadings(DevicesManager* devices);

//for publishing separate flow
extern void SetSaveToDBSigFlag(bool val);


class DeviceReadingsBulkSaver
{
	LOG_DEF("nisa100.hostapp.DeviceReadingsBulkSaver");

public:
	DeviceReadingsBulkSaver(DevicesManager& devicesManager, const ConfigApp& configApp_)
	: devices(devicesManager), configApp(configApp_)
	{
		periodSeconds = configApp.ReadingsSaverPeriod();
		maxEntriesBeforeSave = configApp.ReadingsMaxEntriesSaver();

		lastSaveTime = nlib::CurrentUniversalTime();
		/*commented by Cristian.Guef
		cachedReadings.reserve(maxEntriesBeforeSave + 2);
		*/

		LOG_INFO("ctor: DevicesReadingsBulkSaver initialized with parameters: period=" << nlib::ToString(periodSeconds)
		    << ", maxEntriesBeforeSave=" << maxEntriesBeforeSave);

		m_saveNow = false;
	}

	void CheckForSave();


	//for publishing separate flow
	void SaveReadingsWithSignal();
	void SaveReadingsWithoutSignal();

	void SetConfigAlarmTimer()
	{
		itimerval itime;
		itime.it_value.tv_sec = configApp.SavePublishPeriod()/1000;
		itime.it_value.tv_usec = (configApp.SavePublishPeriod()%1000) * 1000;
		itime.it_interval.tv_sec = configApp.SavePublishPeriod()/1000;
		itime.it_interval.tv_usec = (configApp.SavePublishPeriod()%1000) * 1000;

		if (setitimer(ITIMER_REAL, &itime, NULL) != 0)
		//if (setitimer(ITIMER_VIRTUAL, &itime, NULL) != 0)
		{
			LOG_ERROR("ERROR setting timer interval");
			return;
		}
		
		LOG_INFO("setting timer interval OK with values: sec=" << itime.it_value.tv_sec << " and usec=" << itime.it_value.tv_usec);
	}


	void AddReading(const DeviceReading& reading)
	{
		LOG_DEBUG("AddReading: to queue.");
		cachedReadings.push_back(reading);
	}

private:
	DevicesManager& devices;
	const ConfigApp& configApp;

	nlib::TimeSpan periodSeconds;
	int maxEntriesBeforeSave;

	nlib::DateTime lastSaveTime;

	DeviceReadingsList cachedReadings;

private:
	bool m_saveNow;
};

}// namespace hostapp
}// namespace nisa100

#endif /*DEVICEREADINGSBULKSAVER_H*/
