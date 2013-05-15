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
 * StackWrapper.h
 *
 *  Created on: Mar 8, 2009
 *      Author: Andy
 */

#ifndef STACKWRAPPER_H_
#define STACKWRAPPER_H_

#include "Services/ASL_Service_PrimitiveTypes.h"
#include "Services/ASL_AlertAcknowledge_PrimitiveTypes.h"
#include "StackWrapper/Types.h"
#include "StackWrapper/NLMEWrapper.h"
#include "StackWrapper/SLMEWrapper.h"
#include "AL/Process.h"
#include <Common/NEAddress.h>


namespace Isa100 {
namespace AL {
class  ProcessMessages;
}
};

enum TLSecurityLevel
{
	TLSecurityNone,
	TLSecurityMIC_32,
	TLSecurityENC_MIC_32
};


void Stack_Init(const NE::Common::Address128& myIPv6, Uint16 port, const NE::Common::Address64& myEUI64,
		const NE::Common::Address64& secManagerEUI64);
void Stack_Release();
void Stack_SetTLSecurityLevel(TLSecurityLevel securityLevel);

void Stack_PrintContracts(std::ostringstream& stream);
void Stack_ContractsPktsContor();
void Stack_PrintRoutes(std::ostringstream& stream);
void Stack_PrintKeys(std::ostringstream& stream);

Isa100::Common::Objects::SFC::SFCEnum Stack_EnqueueMessage_response(const Isa100::ASL::Services::PrimitiveResponsePointer& message);
Isa100::Common::Objects::SFC::SFCEnum Stack_EnqueueMessage_request(const Isa100::ASL::Services::PrimitiveRequestPointer& message);
Isa100::Common::Objects::SFC::SFCEnum Stack_EnqueueMessage_alertAck(const Isa100::ASL::Services::ASL_AlertAcknowledge_RequestPointer& message);
Isa100::Common::Objects::SFC::SFCEnum Stack_EnqueueMessage_alertReport(const Isa100::ASL::Services::ASL_AlertReport_RequestPointer& message);

void Stack_Run();
void Stack_OneSecondTasks();

void Stack_WaitForAPDU(int tsapID, int timeoutUsec, Isa100::AL::ProcessMessages &processMessages,  const Isa100::AL::ProcessPointer &processPointer, Uint32 currentTime);


#endif /* STACKWRAPPER_H_ */
