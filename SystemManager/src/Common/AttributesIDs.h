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

#ifndef ATTRIBUTESIDS_H_
#define ATTRIBUTESIDS_H_


namespace Isa100 {
namespace Common {


/**
 *
 */
namespace DMO_Attributes {
	enum DMO_Attributes {
		// Vendor Attributes
	    Assigned_Role = 5,
		Vendor_ID = 6,
		Model_ID = 7,
		Serial_Number = 9,
		Power_Supply_Status = 10,
		Software_Revision_Information = 22,

		// Meta Data Attributes
		Contracts_Table = 26,
		Contract_Request_Timeout = 27,
		Max_ClientServer_Retries = 28,
		Max_Retry_Timeout_Interval = 29,
		Metadata_Contracts_Table = 32, // Metadata (count and capacity) of the Contracts_Table attribute

		JoinReason = 64,
		PackagesStatistics = 66,
		PingInterval = 67
	};
}

namespace DLMO_Attributes {
/*
 * dlmo11a attribute identifiers.
 */
enum DLMO_Attributes {
    ActScanHostFract = 1,
    AdvJoinInfo = 2,
    AdvSuperframe = 3,
    SubnetID = 4,
    SolicTemplate = 5,
    AdvFilter = 6,
    SolicFilter = 7,
    TaiTime = 8,
    TaiAdjust = 9,
    MaxBackoffExp = 10,
    MaxDsduSize = 11,
    MaxLifetime = 12,
    NackBackoffDur = 13,
    LinkPriorityXmit = 14,
    LinkPriorityRcv = 15,
    EnergyDesign = 16,
    EnergyLeft = 17,
    DeviceCapability = 18,
    IdleChannels = 19,
    ClockExpire = 20,
    ClockStale = 21,
    RadioSilence = 22,
    RadioSleep = 23,
    RadioTransmitPower = 24,
    CountryCode = 25,
    Candidates = 26,
    DiscoveryAlert = 27,
    SmoothFactors = 28,
    QueuePriority = 29,
    Ch = 30,
    ChMeta = 31,
    TsTemplate = 32,
    TsTemplateMeta = 33,
    Neighbor = 34,
    NeighborDiagReset = 35,
    NeighborMeta = 36,
    Superframe = 37,
    SuperframeIdle = 38,
    SuperframeMeta = 39,
    Graph = 40,
    GraphMeta = 41,
    Link = 42,
    LinkMeta = 43,
    Route = 44,
    RouteMeta = 45,
    NeighborDiag = 46,
    DiagMeta = 47,
    ChannelDiag = 48,
    AlertPolicy = 49
};
}

/**
 *
 */
namespace NLMO_Attributes {

	enum NLMO_Attributes {
		Backbone_Capable = 1,
		DL_Capable = 2,
		Short_Address = 3,
		Long_Address = 4,
		Route_Table = 5,
		Enable_Default_Route = 6,
		Default_Route_Entry = 7,
		Contract_Table = 8,
		Address_Translation_Table = 9,
		Max_NSDU_size = 10,
		Frag_Reassembly_Timeout = 11,
		Frag_Datagram_Tag = 12,
		NLRouteTblMeta = 13,
		NLContractTblMeta = 14,
		NLATTblMeta = 15,
		DroppedNPDUAlertDescriptor = 16
	};
}

namespace ARMO_Attributes {
	enum ARMO_Attributes {
		Alert_Master_Device_Diagnostics = 1,
		Confirmation_Timeout_Device_Diagnostics = 2,
		Alert_Master_Comm_Diagnostics = 4,
		Confirmation_Timeout_Comm_Diagnostics = 5,
		Alert_Master_Security = 7,
		Confirmation_Timeout_Security = 8,
		Alert_Master_Process = 10,
		Confirmation_Timeout_Process = 11
	};
}

/**
 *
 */
namespace HRCO_Attributes {
    enum HRCO_Attributes {
        CommunicationEndpoint = 2,
        ArrayOfObjectAttributeIndexAndSize = 6
    };
}

namespace DPO_Attributes {
    enum DPO_Attributes {
        FW_TAI = 27,
        FW_Version = 28
    };
}


}
}

#endif /* ATTRIBUTESIDS_H_ */
