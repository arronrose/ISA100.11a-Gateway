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
 * PDUUtils.h
 *
 *  Created on: Oct 16, 2008
 *      Author: catalin.pop
 */

#ifndef PDUUTILS_H_
#define PDUUTILS_H_

#include "ASL/PDU/ClientServerPDU.h"
#include "ASL/PDU/ExecuteRequestPDU.h"
#include "ASL/PDU/ExecuteResponsePDU.h"
#include "ASL/PDU/ReadResponsePDU.h"
#include "ASL/PDU/ReadRequestPDU.h"
#include "ASL/PDU/WriteRequestPDU.h"
#include "ASL/PDU/WriteResponsePDU.h"
#include "ASL/PDU/AlertReportPDU.h"
#include "ASL/PDU/PublishPDU.h"
#include "Common/logging.h"

namespace Isa100 {
namespace ASL {

using namespace Isa100::ASL::PDU;

/**
 *
 * @author Catalin Pop
 */
class PDUUtils {
        LOG_DEF("I.A.PDUUtils");

    public:

        PDUUtils();

        virtual ~PDUUtils();

        /**
         * Appends ExecuteRequestPDU to ClientServerPDU.
         * Returns the same pointer to ClientServerPDU but with the appended payload.
         */
        static ClientServerPDUPointer appendExecuteRequest(ClientServerPDUPointer sourceAPDU,
                    ExecuteRequestPDUPointer executeRequestPDU);

        /**
         * Appends ExecuteResponsePDU to ClientServerPDU.
         * Returns the same pointer to ClientServerPDU but with the appended payload.
         */
        static ClientServerPDUPointer appendExecuteResponse(ClientServerPDUPointer sourceAPDU,
                    ExecuteResponsePDUPointer executeResponsePDU);

        /**
         * Appends WriteRequestPDU to ClientServerPDU.
         * Returns the same pointer to ClientServerPDU but with the appended payload.
         */
        static ClientServerPDUPointer appendWriteRequest(ClientServerPDUPointer sourceAPDU,
                    WriteRequestPDUPointer writeRequestPDU);

        /**
         * Appends WriteResponsePDU to ClientServerPDU.
         * Returns the same pointer to ClientServerPDU but with the appended payload.
         */
        static ClientServerPDUPointer appendWriteResponse(ClientServerPDUPointer sourceAPDU,
                    WriteResponsePDUPointer writeResponsePDU);

        /**
         * Appends ReadRequestPDU to ClientServerPDU.
         * Returns the same pointer to ClientServerPDU but with the appended payload.
         */
        static ClientServerPDUPointer appendReadRequest(ClientServerPDUPointer sourceAPDU, ReadRequestPDUPointer readRequestPDU);

        /**
         * Appends ReadResponsePDU to ClientServerPDU.
         * Returns the same pointer to ClientServerPDU but with the appended payload.
         */
        static ClientServerPDUPointer appendReadResponse(ClientServerPDUPointer sourceAPDU,
                    ReadResponsePDUPointer readResponsePDU);

        /**
         * Appends alertReportPDU to ClientServerPDU.
         * Returns the same pointer to ClientServerPDU but with the appended payload.
         */
        static ClientServerPDUPointer appendAlertReport(ClientServerPDUPointer sourceAPDU, AlertReportPDUPointer alertReportPDU);

    public:

        /**
         * Gets ExecuteRequestPDU from the received ClientServerPDU.
         */
        static ExecuteRequestPDUPointer extractExecuteRequest(ClientServerPDUPointer sourceAPDU);

        /**
         * Gets ExecuteResponsePDU from the received ClientServerPDU.
         */
        static ExecuteResponsePDUPointer extractExecuteResponse(ClientServerPDUPointer sourceAPDU);

        /**
         * Gets WriteRequestPDU from the received ClientServerPDU.
         */
        static WriteRequestPDUPointer extractWriteRequest(ClientServerPDUPointer sourceAPDU);

        /**
         * Gets WriteResponsePDU from the received ClientServerPDU.
         */
        static WriteResponsePDUPointer extractWriteResponse(ClientServerPDUPointer sourceAPDU);

        /**
         * Gets ReadRequestPDU from the received ClientServerPDU.
         */
        static ReadRequestPDUPointer extractReadRequest(ClientServerPDUPointer sourceAPDU);

        /**
         * Gets ReadResponsePDU from the received ClientServerPDU.
         */
        static ReadResponsePDUPointer extractReadResponse(ClientServerPDUPointer sourceAPDU);

        /**
         * Gets AlertReportPDU from the received ClientServerPDU.
         */
        static AlertReportPDUPointer extractAlertReport(ClientServerPDUPointer sourceAPDU);

        /**
         * Gets AlertReportPDU from the ClientServerPDU received in an alert forwarded by Gateway.
         */
        static CascadedAlertReportPDUPointer extractCascadingAlertReport(BytesPointer pseudoPayload);

        /**
         * Gets PublishPDU from the received ClientServerPDU.
         */
        static PublishPDUPointer extractPublishedData(ClientServerPDUPointer sourceAPDU);

    public:

        /**
         * Returns TRUE if the PDU is a request of Execute service.
         */
        static bool isExecuteRequest(ClientServerPDUPointer sourceAPDU);

        /**
         * Returns TRUE if the PDU is a request of Write service.
         */
        static bool isWriteRequest(ClientServerPDUPointer sourceAPDU);

        /**
         * Returns TRUE if the PDU is a request of Read service.
         */
        static bool isReadRequest(ClientServerPDUPointer sourceAPDU);

        /**
         * Returns TRUE if the PDU is a response of Execute service.
         */
        static bool isExecuteResponse(ClientServerPDUPointer sourceAPDU);

        /**
         * Returns TRUE if the PDU is a response of Write service.
         */
        static bool isWriteResponse(ClientServerPDUPointer sourceAPDU);

        /**
         * Returns TRUE if the PDU is a response of Read service.
         */
        static bool isReadResponse(ClientServerPDUPointer sourceAPDU);

        /**
         * Returns TRUE if the PDU is an alert report.
         */
        static bool isAlertReport(ClientServerPDUPointer sourceAPDU);

        /**
         * Returns TRUE if the PDU is a Publish PDU.
         */
        static bool isPublish(ClientServerPDUPointer sourceAPDU);
};

}
}

#endif /* PDUUTILS_H_ */
