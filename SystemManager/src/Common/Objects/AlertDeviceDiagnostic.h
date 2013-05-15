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

#ifndef ALERTDEVICEDIAGNOSTIC_H_
#define ALERTDEVICEDIAGNOSTIC_H_

namespace Isa100 {
namespace Common {
namespace Objects {

// AlertDeviceDiagnostic ::= ENUMERATED ( -- 1 octet )

enum AlertDeviceDiagnostic {
    deviceDiagnosticAlarmRecoveryStart = 0,
    deviceDiagnosticAlarmRecoveryEnd = 1,
    deviceFailure = 2,
    offSpecification = 3,
    diagnosticMaintenance = 4,
    diagnosticCheckFunction = 5
};

}
}
}

#endif /*ALERTDEVICEDIAGNOSTIC_H_*/
