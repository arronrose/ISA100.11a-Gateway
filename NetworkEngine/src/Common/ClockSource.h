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
#ifndef CLOCKSOURCE_H_
#define CLOCKSOURCE_H_

#include "Common/NETypes.h"
#include "Common/SettingsLogic.h"


namespace NE {
namespace Common {


class ClockSource {
	private:
    /**
     * WARN: Should be used only where a value of +/- 1 sec is not critical.
     * Holds the value of time in seconds. It is updated at 1 second interval by the main application loop.
     * Is aproximate because is updated by main loop and may be with +/- 1 sec diffrent from the real clock.
     */
    static Uint32 currentAproxTime;


    public:
        /* seconds between 1/1/1958 and 1/1/1970 = 378691200 */
        static const Uint32 UTC_TAI_OFFSET = 0x16925E80;

        /**
         * Will be updated from SettingsLogic.currentUTCAdjustment
         */
        static Uint32 currentUTCAdjustment;


        /**
         * Returns the current time.
         */
        static Uint32 getCurrentTime();

        /**
		 * WARN: Should be used only where a value of +/- 1 sec is not critical.
		 * Return the value of clock time in seconds. It is updated at 1 second interval by the main application loop.
		 * Is aproximate because is updated by main loop and may be with +/- 1 sec diffrent from the real clock.
		 */
        static Uint32 getAproxCurrentTime(){return currentAproxTime;}

        static void setAproxCurrentTime(Uint32 currentAproxTime_){currentAproxTime = currentAproxTime_;}

        /**
         * Returns the current TAI (number of seconds from 1/1/1958).
         */
        static Uint32 getTAI(const NE::Common::SettingsLogic& settingsLogic);

        /**
		 * WARN: Should be used only where a value of +/- 1 sec is not critical.
		 * Return the value of current TAI with aproximation of +/- 1.
		 * It is updated at 1 second interval by the main application loop.
		 * Is aproximate because is updated by main loop and may be with +/- 1 sec diffrent from the real clock.
		 */
        static Uint32 getAproxTAI(const NE::Common::SettingsLogic& settingsLogic){
        	return currentAproxTime + get_UTC_TAI_Offset();
        }

        static Uint32 get_UTC_TAI_Offset(){
            return UTC_TAI_OFFSET + currentUTCAdjustment;
        }

        /**
         * Returns only the first 24 bits of TAI.
         */
        static Uint32 getTruncatedTai(const NE::Common::SettingsLogic& settingsLogic);

        /**
         * Returns only the first 24 bits of TAI.
         */
        static Uint32 getTruncatedTai(Uint32 TAI);
};
//typedef boost::shared_ptr<ClockSource> ClockSourcePointer;

}
}
#endif /*CLOCKSOURCE_H_*/
