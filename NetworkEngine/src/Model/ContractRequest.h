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
 * ContractRequest.h
 *
 *  Created on: Dec 9, 2008
 *      Author: beniamin.tecar
 *
 *  Feb 2009: update to D2A draft
 */

#ifndef CONTRACTREQUEST_H_
#define CONTRACTREQUEST_H_

#include "Common/NETypes.h"
#include "Common/NEAddress.h"
#include "ContractTypes.h"

namespace NE {
namespace Model {

/**
 * Parameters used to establish a new contract
 * 		modify an existing contract
 * 		renew an existing contract
 * 		by sending request to system manager.
 */
struct ContractRequest {

    /**
     * A numerical value, uniquely assigned by the device sending the request to the system manager,
     * to identify the request being made.
     */
    //Attention: standard size in Uint8 => to be marshalled/unmarshalled as Uint8; kept as Uint16 only internally!
    Uint16 contractRequestId;

    /**
     * Type of request sent to the system manager. (Uint8)
     */
    ContractRequestType::ContractRequestType contractRequestType;

    /**
     * Existing contract ID that needs to be modified or renewed.
     */
    Uint16 contractId;

    /**
     * Type of communication service for which the contract is being requested for; (Uint8)
     */
    CommunicationServiceType::CommunicationServiceType communicationServiceType;

    /**
     * The device that send the application messages to destination;
     * ! Custom - don't marshall/unmarshall
     */
    Address32 sourceAddress;

    /**
     * TDSAP of the application process that generated the request.
     */
    Uint16 sourceSAP;

    /**
     * The device that receives the application messages from the source;
     * note that this information may be provided to the source during provisioning or during configuration
     * of the application process.
     */
    Address32 destinationAddress;

    /**
     * TDSAP in the destination that will be used to send these messages to the application layer; note that this
     * information may be provided to the source during provisioning or during configuration of the application process.
     */
    Uint16 destinationSAP;

    /**
     * Determines if the system manager can change the requested contract to meet the network resources available
     * and if the system manager can revoke this contract to make resources available to higher priority contracts.
     * Uint8
     */
    ContractNegotiability::ContractNegotiability contractNegotiability;

    /**
     * Determines how long the system manager should keep the contract before it is terminated;
     * units in seconds
     */
    Uint32 contractLife;

    /**
     * Requests a base priority for all messages sent using the contract.
     * Network control = 3. Network control may be used for critical management of the network by the system manager.
     * Real time buffer = 2. Real time buffer may be used for periodic communications in which the message buffer is overwritten
     *  whenever a newer message is generated.
     * Real time sequential = 1. Real time sequential may be used for applications such as voice or video that need sequential
     *  delivery of messages.
     * Best effort queued = 0. Best effort queued may be used for client-server communications.
     */
    ContractPriority::ContractPriority contractPriority;

    /**
     * Indicates the maximum APDU size that needs to be supported in octets.
     * max value allowed is 1252 bytes.
     */
    Uint16 payloadSize;

    /**
     * Bit 0 indicates if retransmission of old publish data is necessary if buffer is not overwritten with
     * new publish data, bit 0 is only applicable for periodic communication and is 0 for non-periodic
     * communication; bits 1 to 7 indicate the requested reliability for delivering the transmitted TSDUs to
     * the destination
     * Bit 0 : 0 retransmit (default), 1 do not retransmit
     * Bits 1 to 7 : Enumeration 0 = low 1 = medium 2 = high
     */
    Uint8 reliability_And_PublishAutoRetransmit;

    /**
     * Used for periodic communication; to identify the desired publishing period in the contract request.
     * a positive number is a multiple of 1 second and a negative number is a fraction of 1 second.
     */
    Int16 requestedPeriod;

    /**
     * Used for periodic communication; to identify the desired phase (within the publishing period) of
     * publications in the contract request; valid value set: 0 to 99,
     * any other value indicates that device only cares about period and does not care about phase.
     */
    Uint8 requestedPhase;

    /**
     * Used for periodic communication; to identify the maximum end-to-end transport delay desired;
     * units in milliseconds.
     */
    Uint16 requestedDeadline;

    /**
     * Used for non-periodic communication to identify the long term rate that needs to be supported for
     * client-server or source-sink messages; positive values indicate APDUs per second, negative values
     * indicate seconds per APDU.
     */
    Int16 committedBurst;

    /**
     * Used for non-periodic communication to identify the short term rate that needs to be supported for
     * client-server or source-sink messages; positive values indicate APDUs per second, negative values
     * indicate seconds per APDU.
     */
    Int16 excessBurst;

    /**
     * Used for non-periodic communication; to identify the maximum number of client requests that may be
     * simultaneously awaiting a response.
     */
    Uint8 maxSendWindowSize;

    /**
     * Identifies if the current contract is management contract.
     */
    bool isManagement;

    ContractRequest();

    bool equal(const ContractRequest& contractReq) {

		if (contractReq.contractRequestId == contractRequestId
				&& contractReq.contractRequestType == contractRequestType
				&& contractReq.sourceAddress == sourceAddress
				&& contractReq.sourceSAP == sourceSAP
				&& contractReq.destinationAddress == destinationAddress
				&& contractReq.destinationSAP == destinationSAP) {

			return true;
		}

		return false;
    }

};

typedef boost::shared_ptr<ContractRequest> ContractRequestPointer;

std::ostream& operator<<(std::ostream& stream, const ContractRequest& request);

}
}

#endif /* CONTRACTREQUEST_H_ */
