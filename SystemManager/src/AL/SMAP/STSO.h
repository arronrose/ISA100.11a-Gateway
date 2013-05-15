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
 * STSO.h
 *
 *  Created on: Mar 30, 2009
 *      Author: sorin.bidian, catalin.pop, beniamin.tecar, flori.parauan
 */

#ifndef STSO_H_
#define STSO_H_

#include "Common/NETypes.h"
#include "Common/logging.h"
#include "AL/Isa100Object.h"

namespace Isa100 {
namespace AL {
namespace SMAP {

namespace STSOAttributeID {
enum STSOAttributeID {
    Current_UTC_Adjustment = 1,
    Next_UTC_Adjustment_Time = 2,
    Next_UTC_Adjustment = 3
};
}

class STSO: public Isa100::AL::Isa100Object {
    LOG_DEF("I.A.S.STSO")

    private:
        /**
         * Devices that need to convert TAI time to hh:mm:ss format need this adjustment from the system manager;
         *  units in seconds;
         *  note that the adjustment can be negative;
         *  note that UTC and TAI are based on different start dates but this difference is not covered by this attribute;
         *  on January 1 2009 the value changed from 33 sec to 34 sec.
         */
        Int16 currentUTCAdjustment;

        /**
         * If the system manager knows the next time this UTC adjustment value will change, the SM is expected to indicate
         *   this time in TAI units;
         * If the system manager does not know this time, it is expected to indicate the current TAI time and as a result the
         *   value of the Next_UTC_Adjustment attribute shall be same as the value of the Current_UTC_Adjustment.
         */
        Uint32 nextUTCAdjustmentTime;

        /**
         * The UTC adjustment that will go into affect at the time specified by the Next_UTC_Adjustment_Time attribute.
         */
        Int16 nextUTCAdjustment;

    public:
        STSO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine);
        ~STSO();

        void execute( Uint32 currentTime );

        Isa100::AL::ObjectID::ObjectIDEnum getObjectID() const;

        /**
         * After the object is created (STSO started) its job is to handle all STSO indicates of that kind.
         */
        bool isJobFinished();

    protected:
    	bool expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void indicateRead(Isa100::ASL::Services::PrimitiveIndicationPointer indication);
};

}
}
}

#endif /* STSO_H_ */
