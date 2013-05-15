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

/*
 * ClockSource.cpp
 *
 *  Created on: Apr 25, 2009
 *      Author: mulderul(catalin.pop)
 */

#include "ClockSource.h"
#include <time.h>
#include "Common/NETypes.h"
#include "Model/NetworkEngine.h"

using namespace NE::Common;

namespace NE {
namespace Common {

Uint32 ClockSource::currentUTCAdjustment = 34;//default value current adjustment for this year, will be updated from Settings
Uint32 ClockSource::currentAproxTime = 0;

/**
 * Returns the current time.
 */
Uint32 ClockSource::getCurrentTime() {
	return time(NULL);
}

/**
 * Returns the current TAI (number of seconds from 1/1/1958).
 */
Uint32 ClockSource::getTAI(const NE::Common::SettingsLogic& settingsLogic) {
	Uint32 tai;

//	tai = time(NULL) + UTC_TAI_OFFSET + settingsLogic.currentUTCAdjustment;
	tai = time(NULL) + get_UTC_TAI_Offset();
	return tai;
}

/**
 * Returns only the first 24 bits of TAI.
 */
Uint32 ClockSource::getTruncatedTai(const NE::Common::SettingsLogic& settingsLogic) {
	Uint32 tai;

	tai = getTAI(settingsLogic);
	tai = (Uint32) (tai << 10);
	tai = (Uint32) (tai >> 10);

	return tai;

}

/**
 * Returns only the first 24 bits of TAI.
 */
Uint32 ClockSource::getTruncatedTai(Uint32 TAI) {
	Uint32 tai;

	tai = (Uint32) (TAI << 10);
	tai = (Uint32) (tai >> 10);

	return tai;

}

}
}
