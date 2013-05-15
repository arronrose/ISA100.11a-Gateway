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

#ifndef CONTRACTSANDROUTES_H_
#define CONTRACTSANDROUTES_H_

#include <vector>
#include <nlib/datetime.h>
#include <map>

namespace nisa100 {
namespace hostapp {

class ContractsAndRoutes
{
public:
	struct Contract
	{
		IPv6				sourceAddress;		// 
		boost::uint16_t		contractID;		// Contract_ID
		// Communication_Service_Type  0:periodic, 1: aperiodic
		boost::uint8_t		serviceType;								
		time_t				activationTime;		// Contract_Activation_Time
		boost::uint16_t		sourceSAP;			// Source_SAP
		IPv6				destinationAddress;	// Destination_Address
		boost::uint16_t		destinationSAP;		// Destination_SAP
		time_t				expirationTime;	// Assigned_Contract_Expiration_Time - offset related to the Contract_Activation_Time.
		boost::uint8_t		priority;		// Assigned_Contract_Priority
		boost::uint16_t		NSDUSize;		// Assigned_Max_NSDU _Size
		// Assigned_Reliability_And_PublishAutoRetransmit
		boost::uint8_t		reliability;	
		boost::int16_t		period;		// Assigned_Period
		boost::uint8_t		phase;		// Assigned_Phase
		boost::uint16_t		deadline;		// Assigned_Deadline
		boost::int16_t		comittedBurst;	// Assigned_Committed_Burst
		boost::int16_t		excessBurst;	// Assigned_Excess_Burst
		boost::uint8_t		maxSendWindow;	// Assigned_Max_Send_Window_Size
	};
	typedef std::vector<Contract> ContractsListT;

	struct Route
	{
		struct RouteElement{
			boost::uint8_t		isGraph;	// 0: is node; 1: is graph
			union {
				boost::uint16_t	graphID;
				boost::uint8_t	nodeAddress[IPv6::SIZE];
			}elem;
		};
		typedef std::vector<RouteElement> RouteElementsT;
		boost::uint16_t		index;	
		boost::uint8_t 		alternative;
		boost::uint8_t		forwardLimit;
		RouteElementsT		routes;
		boost::uint32_t		selector;	
		boost::uint32_t		srcAddr;

		//new fields
		union Selector
		{
			boost::uint16_t		contractID;
			boost::uint8_t	nodeAddress[IPv6::SIZE];
		}selectorn;
		IPv6				srcAddress;
	};
	typedef std::vector<Route> RoutesListT;

public:
	ContractsListT	ContractsList;
	RoutesListT		RoutesList;

	//for saving route_elements
	typedef std::map<boost::uint16_t/*contractID*/, IPv6/*destAddress*/> ContractIDsListT;
	ContractIDsListT ContractIDsList;
	typedef std::map<IPv6/*destAddress*/, int/*index in RoutesList*/> Altern2DestsListT;
	Altern2DestsListT Altern2DestsList;
};

}// namespace hostapp
}// namespace nisa100

#endif /*CONTRACTSANDROUTES_H_*/

