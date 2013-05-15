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
 * ContractTypes.h
 *
 *  Created on: Dec 9, 2008
 *      Author: beniamin.tecar
 */

#ifndef CONTRACTTYPES_H_
#define CONTRACTTYPES_H_

#include "Common/NETypes.h"
#include "Misc/Convert/Convert.h"
#include "Misc/Marshall/NetworkOrderStream.h"


namespace NE {
namespace Model {

namespace ContractTDSAP {

enum ContractTDSAPEnum{
    TDSAP_DMAP = 0xF0B0, //int 61616
    TDSAP_SMAP = 0xF0B1  //int 61617
};

}  // namespace ContractTDSAP

/**
 * Used by ContractData
 */
namespace ContractStatus {
	enum ContractStatus {
		SuccessWithImmediateEffect = 0,
		SuccessWithDelayedEffect = 1,
		SuccessWithImmediateEffectButNegotiatedDown = 2,
		SuccessWithDelayedEffectButNegotiatedDown = 3,
	};
}


namespace ContractPriority {
    /**
     * Requests a base priority for all messages sent using the contract.
     */
    enum ContractPriority {
        BestEffortQueued = 0,
        RealTimeSequential = 1,
        RealTimeBuffer = 2,
        NetworkControl = 3
    };
}

// ------------------------------------------- for Contract Request -----------------------------------------

namespace ContractRequestType {
	/**
	 * Type of request sent to the system manager.
	 */
	enum ContractRequestType {
		NewContract = 0,
		ContractModification = 1,
		ContractRenewal = 2
//not used
//		NewDeviceFirstContract = 3
	};
}


namespace CommunicationServiceType {
	/**
	 * Type of communication service for which the contract is being requested for; Enumeration:
	 *
	 */
	enum CommunicationServiceType {
		Periodic = 0, // scheduled
		NonPeriodic = 1 // unscheduled
	};

	inline std::string toString(NE::Model::CommunicationServiceType::CommunicationServiceType type){
	    if (type == NE::Model::CommunicationServiceType::Periodic){
	        return "Per";
	    } else {
	        return "NonPer";
	    }
	}
}

namespace ContractNegotiability {
	/**
	 * Determines if the system manager can change the requested contract to meet the network resources available
	 * and if the system manager can revoke this contract to make resources available to higher priority contracts;
	 */
	enum ContractNegotiability {
		NegotiableAndRevocable = 0,
		NegotiableButNotRevocable = 1,
		NonNegotiableButRevocable = 2,
		NonNegotiableAndNonRevocable = 3
	};
}

// ------------------------------------------- for Contract Response -----------------------------------------

namespace ContractResponseCode {
	enum ContractResponseCode {
		SuccessWithImmediateEffect = 0,
		SuccessWithDelayedEffect = 1,
		SuccessWithImmediateEffectButNegotiatedDown = 2,
		SuccessWithDelayedEffectButNegotiatedDown = 3,
		FailureWithNoFurtherGuidance = 4,
		FailureWithRetryGuidance = 5,
		FailureWithRetryAndNegotiationGuidance = 6
	};
}


namespace ContractServiceType {
	enum ContractServiceType {
		Periodic = 0, // scheduled
		NonPeriodic = 1, // unscheduled
		NewDeviceFirstContract = 2
	};
}


}
}

#endif /* CONTRACTTYPES_H_ */
