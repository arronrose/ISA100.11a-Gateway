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

#ifndef ALERTTYPE_H_
#define ALERTTYPE_H_

#include "Common/Objects/AlertDeviceDiagnostic.h"
#include "Common/Objects/AlertCommDiagnostic.h"
#include "Common/Objects/AlertSecurity.h"
#include "Common/Objects/AlertProcess.h"

using namespace NE::Misc::Marshall;

namespace Isa100 {
namespace Common {
namespace Objects {

class AlertType {

        AlertDeviceDiagnostic deviceDiagnostic;
        AlertCommDiagnostic commDiagnostic;
        AlertSecurity security;
        AlertProcess process;

    public:
        virtual ~AlertType() {
        }

        void marshall(OutputStream& stream, Uint8 categ) {
            switch (categ) {
                case 0:
                    stream.write((Uint8) deviceDiagnostic);
                break;
                case 1:
                    stream.write((Uint8) commDiagnostic);
                break;
                case 2:
                    stream.write((Uint8) security);
                break;
                case 3:
                    stream.write((Uint8) process);
                break;
                default:
                    ;
            }
        }

        void unmarshall(InputStream& stream, Uint8 categ) {
            switch (categ) {
                case 0: {
                    Uint8 alert;
                    stream.read(alert);
                    deviceDiagnostic = (AlertDeviceDiagnostic) alert;
                    break;
                }
                case 1: {
                    Uint8 alert;
                    stream.read(alert);
                    commDiagnostic = (AlertCommDiagnostic) alert;
                    break;
                }
                case 2: {
                    Uint8 alert;
                    stream.read(alert);
                    security = (AlertSecurity) alert;
                    break;
                }
                case 3: {
                    Uint8 alert;
                    stream.read(alert);
                    process = (AlertProcess) alert;
                    break;
                }
                default:
                    ;
            }
        }

        std::string toString(Uint8 categ) {
            std::ostringstream stream;
            switch (categ) {
                case 0: {
                    stream << "deviceDiagnostic: " << deviceDiagnostic;
                    break;
                }
                case 1: {
                    stream << "commDiagnostic: " << commDiagnostic;
                    break;
                }
                case 2: {
                    stream << "security: " << security;
                    break;
                }
                case 3: {
                    stream << "process: " << process;
                    break;
                }
                default:
                break;
            }

            return stream.str();
        }
};
}
}
}

#endif /*ALERTTYPE_H_*/
