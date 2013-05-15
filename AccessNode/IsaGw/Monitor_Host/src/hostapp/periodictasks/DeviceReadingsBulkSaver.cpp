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

#include "DeviceReadingsBulkSaver.h"
#include "src/hostapp/DevicesManager.h"

namespace nisa100 {
namespace hostapp {

void DeviceReadingsBulkSaver::CheckForSave()
{
			/* commented by Critian.Guef - no need
		periodSeconds = configApp.ReadingsSaverPeriod();
		maxEntriesBeforeSave = configApp.ReadingsMaxEntriesSaver(); 
		*/
		
		nlib::DateTime currentTime = nlib::CurrentUniversalTime();

		bool timeElapsed = (periodSeconds >= nlib::NOTIME) 
			&& (currentTime > lastSaveTime + periodSeconds);
		
		/* commented 
		bool tooMuchEntries = (maxEntriesBeforeSave > 0)
		    && (cachedReadings.size() >= ((unsigned int) maxEntriesBeforeSave));
			*/
		//added 
		bool tooMuchEntries = (maxEntriesBeforeSave > 0)
		    && (GetCurrentReadingsNo() >= ((int) maxEntriesBeforeSave));

		if (timeElapsed || tooMuchEntries)
		{
			/*
			if (!cachedReadings.empty())
			{
				LOG_DEBUG("CheckForSave: BulkReadingsSaver: timeElapsed=" << timeElapsed << " and tooMuchEntries=" << tooMuchEntries);

				devices.SaveReadings(cachedReadings);
				cachedReadings.clear();
				LOG_DEBUG("finished AddReading: queue size is = " << cachedReadings.size());
			}*/
			//added
			SaveCurrentReadings(&devices);
			
			lastSaveTime = currentTime;
		}
}

void DeviceReadingsBulkSaver::SaveReadingsWithSignal()
{
	/*
	if (devices.factoryDal.IsTransactionOn())
	{
		LOG_WARN("DeviceReadings not saved now because we are in the middle of a transaction");
		m_saveNow = true;
		return;
	}

	if (GetAddToCacheFlag())
	{
		SetSaveToDBSigFlag();
		return;
	}
	SaveCurrentReadings(&devices);
	*/
	SetSaveToDBSigFlag(true);	//JUST SET A FLAG

}

void DeviceReadingsBulkSaver::SaveReadingsWithoutSignal()
{
	/*
	if (m_saveNow)
	{
		m_saveNow = false;
		SaveCurrentReadings(&devices);
	}*/
	SaveCurrentReadings(&devices);
}

}// namespace hostapp
}// namespace nisa100
