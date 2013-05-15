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
 * Contract_Data.h
 *
 *  Created on: Dec 15, 2008
 *      Author: beniamin.tecar
 *
 *
 *  Feb 2009: update to D2A draft
 */

#ifndef CONTRACTDATA_H_
#define CONTRACTDATA_H_

#include "Model/ContractTypes.h"
#include "Common/Address128.h"

namespace Isa100 {
namespace Model {


/**
 * The DMO in the source shall maintain a list of all assigned contracts using the Contracts_Table attribute.
 * This attribute shall be based on the data structure Contract_Data.
 */
struct ContractData {

	Uint16 contractID;

	/**
	 * This element is related to ContractResponse.
	 * Uint8
	 */
	NE::Model::ContractStatus::ContractStatus contractStatus;

    /**
     * This element is the same as in ContractResponse.
     * Uint8
     */
	NE::Model::CommunicationServiceType::CommunicationServiceType serviceType;

    /**
     * This element is the same as in ContractResponse.
     */
	Uint32 contractActivationTime;

	Uint16 sourceSAP;

    /**
     * This element is the same as in ContractRequest.
     */
	NE::Common::Address128 destinationAddress;

	/**
	 * This element is the same as in ContractRequest.
	 */
    Uint16 destinationSAP;

    /**
     * This element is the same as in ContractResponse.
     */
    Uint32 assigned_Contract_Life;

    /**
     * This element is the same as in ContractResponse.
     */
    NE::Model::ContractPriority::ContractPriority assigned_Contract_Priority;

    /**
     * This element is the same as in ContractResponse.
     */
    Uint16 assigned_Max_NSDU_Size;

    /**
     * This element is the same as in ContractResponse.
     */
    Uint8 assigned_Reliability_And_PublishAutoRetransmit;

    /**
     * This element is the same as in ContractResponse.
     */
    Int16 assignedPeriod;

    /**
     * This element is the same as in ContractResponse.
     */
    Uint8 assignedPhase;

    /**
     * This element is the same as in ContractResponse.
     */
    Uint16 assignedDeadline;

    /**
     * This element is the same as in ContractResponse.
     */
    Int16 assignedCommittedBurst;

    /**
     * This element is the same as in ContractResponse.
     */
    Int16 assignedExcessBurst;

    /**
     * This element is the same as in ContractResponse.
     */
    Uint8 assigned_Max_Send_Window_Size;




    ContractData();

    void marshall(OutputStream& stream);
    void unmarshall(InputStream& stream);
    std::string toString();

};

typedef boost::shared_ptr<ContractData> ContractDataPointer;

}
}

#endif /* CONTRACTDATA_H_ */
