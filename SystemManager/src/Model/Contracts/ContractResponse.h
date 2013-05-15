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

/**
 * @author beniamin.tecar, sorin.bidian, catalin.pop
 */
#ifndef CONTRACTRESPONSE_H_
#define CONTRACTRESPONSE_H_

#include "Common/NETypes.h"
#include "Common/Address128.h"
#include "Model/ContractTypes.h"


using namespace Isa100::Common;
using namespace NE::Model;

namespace Isa100 {
namespace Model {

struct NetworkContractRow {

    Uint16 contractID;
    /**
     * This element is the same as in ContractRequest.
     */
    NE::Common::Address128 sourceAddress;

    /**
     * This element is the same as in ContractRequest.
     */
    NE::Common::Address128 destinationAddress;

    /**
     *
     */
    ContractPriority::ContractPriority contract_Priority;

    /**
     *
     */
    bool include_Contract_Flag;

    void marshall(NE::Misc::Marshall::OutputStream& stream) {
        stream.write(contractID);
        sourceAddress.marshall(stream);
        destinationAddress.marshall(stream);

        // MSB used to match device expectations
        Uint8 contractInfo = ((Uint8)contract_Priority << 6) | ((Uint8)(include_Contract_Flag << 5));
        stream.write(contractInfo);

    }
};

/**
 * ISA_100_11a_Draft_D2a-final.pdf - p.145.
 */
struct ContractResponse {
    LOG_DEF("I.M.ContractResponse");
    /**
     * A numerical value, uniquely assigned by the device requesting a contract from the system manager,
     * to identify the request being made.
     */
    Uint8 contractRequestID;

    /**
     * Indicates the system manager�s response for the request; Enumeration:
			0 - success with immediate effect
			1 - success with delayed effect
			2 - success with immediate effect but negotiated down
			3 - success with delayed effect but negotiated down
			4 - failure with no further guidance
			5 - failure with retry guidance
			6 - failure with retry and negotiation guidance
     * Uint8
     */
    ContractResponseCode::ContractResponseCode responseCode;

    /**
     * A numeric value uniquely assigned by the system manager to the contract being established and sent to the source.
     * Contract IDs are unique per device.
     * Depending on the requested resources, multiple contract request IDs from a device may be mapped to a single contract ID.
     * In the device, the contract ID is passed in the DSAP control field of each layer and is used to look up
     * the contracted actions that will be taken on the associated PDU as it goes down the protocol suite at each layer.
     *
     * While contract IDs are 16-bit values, the system manager shall assign contract IDs that only fall within
     * the range of 1 to 255 to field devices as they typically have limited memory.
     * This way, the field devices can store contract IDs using only 8 bits.
     *
     * Value 0 reserved to mean no contract.
     */
    Uint16 contractID;

    /**
     * Type of service supported by this contract; (Uint8)
     */
    CommunicationServiceType::CommunicationServiceType communicationServiceType;

    /**
     * Start time for the source to start using the assigned contract.
     */
    //TAINetworkTimeValue contract_Activation_Time;
    Uint32 contract_Activation_Time;

    /**
     * Determines how long the system manager will keep the contract before it is terminated; units in seconds.
     */
    Uint32 assigned_Contract_Life;

    /**
     * Establishes a base priority for all messages sent using the contract; (Uint8)
     */
    ContractPriority::ContractPriority assigned_Contract_Priority;

    /**
     * Indicates the maximum NSDU supported in octets which can be converted by the source into max APDU size
     * supported by taking into account the TL, security, AL header and TMIC sizes;
     * valid value set: 70 - 1280.
     */
    Uint16 assigned_Max_NSDU_Size;

    /**
     * Bit 0 indicates if retransmission of old publish data is necessary if buffer is not overwritten with
     * new publish data, bit 0 is only applicable for periodic communication and is 0 for non-periodic communication;
     * bits 1 to 7 indicate the supported reliability for delivering the transmitted TSDUs to the destination
     * Bit 0 - 0 retransmit (default), 1 do not retransmit
     * Bits 1 to 7 - Enumeration
		0 = low
		1 = medium
		2 = high
     */
    Uint8 assigned_Reliability_And_PublishAutoRetransmit;

    /**
     * Used for periodic communication; to identify the assigned publishing period in the contract;
     * a positive number is a multiple of 1 second and a negative number is a fraction of 1 second.
     */
    Int16 assignedPeriod;

    /**
     * Used for periodic communication; to identify the assigned phase of publications in the contract;
     * valid value set: 0 to 99.
     */
    Uint8 assignedPhase;

    /**
     * Used for periodic communication; to identify the maximum end-to-end transport delay supported
     * by the assigned contract; units in 10 milliseconds.
     */
    Uint16 assignedDeadline;

    /**
     * Used for non-periodic communication to identify the long term rate that is supported for client-server
     * or source-sink messages; postive values indicate APDUs per second, negative values indicate seconds per APDU.
     */
    Int16 assignedCommittedBurst;

    /**
     * Used for non-periodic communication to identify the short term rate that is supported for client-server
     * or source-sink messages; postive values indicate APDUs per second, negative values indicate seconds per APDU.
     */
    Int16 assignedExcessBurst;

    /**
     * Used for non-periodic communication; to identify the maximum number of client requests that may be
     * simultaneously awaiting a response.
     */
    Uint8 assigned_Max_Send_Window_Size;

    /**
     * Used in the case of failure response code; Indicates the amount of time the source should backoff
     * before resending the contract request; units in seconds.
     */
    Uint16 retry_Backoff_Time;

    /**
     * Used in the case of failure response code; indicates the Contract_Negotiability value supportable by system manager.
     */
    ContractNegotiability::ContractNegotiability negotiationGuidance;

    /**
     * Indicates the base priority supportable by system manager for all messages sent using the contract.
     */
    ContractPriority::ContractPriority supportable_Contract_Priority;

    /**
     * Indicates the maximum NSDU supportable by the system manager;
     * units in octets (valid value set: 70 - 1280).
     */
    Uint16 supportable_max_NSDU_Size;

    /**
     * Bit 0 indicates if retransmission of old publish data is necessary if buffer is not overwritten with
     * new publish data, bit 0 is only applicable for periodic communication and is 0 for non-periodic communication;
     * bits 1 to 7 indicate the reliability supportable for delivering the transmitted TSDUs to the destination
     * Bit 0 - 0 retransmit (default), 1 do not retransmit
     * Bits 1 to 7 - Enumeration
		0 = low
		1 = medium
		2 = high
     */
    Uint8 supportable_Reliability_And_PublishAutoRetransmit;

    /**
     * Used for periodic communication; to identify the supportable publishing period by the system manager;
     * Valid value set:
     * a positive number is a multiple of 1 second and a negative number is a fraction of 1 second.
     */
    Int16 supportable_Period;

    /**
     * Used for periodic communication; to identify the phase (within the publishing period) of publications supportable by the system manager;
     * valid value set: 0 to 99
     */
    Uint8 supportablePhase;

    /**
     * Used for periodic communication; to identify the maximum end-to-end transport delay supportable by
     * the system manager;
     * units in 10 milliseconds.
     */
    Uint16 supportableDeadline;

    /**
     * Used for non-periodic communication to identify the long term rate that can be supported for client-server
     * or source-sink messages; postive values indicate APDUs per second, negative values indicate seconds per APDU
     */
    Int16 supportable_Committed_Burst;

    /**
     * Used for non-periodic communication to identify the short term rate that can be supported for client-server
     * or source-sink messages; postive values indicate APDUs per second, negative values indicate seconds per APDU
     */
    Int16 supportable_Excess_Burst;

    /**
     * Used for non-periodic communication; to identify the maximum number of client requests that may be
     * simultaneously awaiting a response.
     */
    Uint8 supportable_Max_Send_Window_Size;

    ContractResponse();

    void marshall(NE::Misc::Marshall::OutputStream& stream);

    void unmarshall(NE::Misc::Marshall::InputStream& stream);

    std::string toString();
};

typedef boost::shared_ptr<ContractResponse> ContractResponsePointer;

}
}

#endif /*CONTRACTRESPONSE_H_*/
