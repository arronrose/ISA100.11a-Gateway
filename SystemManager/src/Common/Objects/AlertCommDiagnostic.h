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

#ifndef ALERTCOMMDIAGNOSTIC_H_
#define ALERTCOMMDIAGNOSTIC_H_

namespace Isa100 {
namespace Common {
namespace Objects {

// AlertCommDiagnostic ::= ENUMERATED (-- 1 octet, to be defined by the ISA100.11a system management TG
// -- standard-specified codes, range 0 .. 0x20
// -- vendor-specific codes range 0x20 .. 0xFF               
// – range subject to change after definition completed

enum AlertCommDiagnostic {
    commDiagnosticAlarmRecoveryStart = 0,
    commDiagnosticAlarmRecoveryEnd = 1,
    malformedPDUAlert = 2,
    contractDeclined = 3,
    contractNegotiatedDown = 4,

    commDiagnosticVendorDefinedEventStart = 32,
    commDiagnosticVendorDefinedEventEnd = 255
};

}
}
}

#endif /*ALERTCOMMDIAGNOSTIC_H_*/
