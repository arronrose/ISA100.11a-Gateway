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
 * AlertTypes.h
 *
 *  Created on: Jun 5, 2009
 *      Author: Sorin.Bidian
 */

#ifndef ALERTTYPES_H_
#define ALERTTYPES_H_

namespace Isa100 {
namespace AL {
namespace Types {

namespace AlertCategory {
enum AlertCategoryEnum {
	deviceDiagnostic = 0,
	communicationsDiagnostic = 1,
	security = 2,
	process = 3
};
}

namespace AlertClass {
enum AlertClassEnum {
    // indicates if it is an event (stateless) or alarm (state-oriented) type of alert.
	event = 0,
	alarm = 1
};
}

namespace ARMOAlerts {
enum ARMOAlertsEnum {
	Alarm_Recovery_Start = 0, //device | communication | security | process
	Alarm_Recovery_End = 1, //device | communication | security | process
};
}

namespace ASLMOAlerts {
enum ASLMOAlertsEnum {
	MalformedAPDUCommunicationAlert = 0
};
}

namespace DLMOAlerts {
enum DLMOAlertsEnum {
	DL_Connectivity = 0,
	NeighborDiscovery = 1
};
}

namespace NLMOAlerts {
enum NLMOAlertsEnum {
	NLDroppedPDU = 0
};
}

namespace TLMOAlerts {
enum TLMOAlertsEnum {
	IllegalUseOfPort = 0,
	TPDUonUnregisteredPort = 1,
	TPDUoutOfSecurityPolicies = 2
};
}

namespace DMOAlerts {
enum DMOAlertsEnum {
	Device_Power_Status_Check = 0,
	Device_Restart = 1
};
}

namespace DSMOAlerts {
enum DSMOAlertsEnum {
	Security_MPDU_Fail_Rate_Exceeded = 0,
	Security_TPDU_Fail_Rate_Exceeded = 1,
	Security_Key_Update_Fail_Rate_Exceeded = 2
};
}

namespace DPSOAlerts {
enum DPSOAlertsEnum {
	Not_On_Whitelist_Alert = 0,
	Inadequate_Join_Capability_Alert = 1
};
}

}
}
}

#endif /* ALERTTYPES_H_ */
