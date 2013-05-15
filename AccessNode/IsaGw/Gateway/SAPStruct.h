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

////////////////////////////////////////////////////////////////////////////////
/// @file SAPStruct.h
/// @author Marcel Ionescu
/// @brief SAP structures: GSAP Header/Data IN, YGSAP Header/Data IN, YGSAP Data OUT
////////////////////////////////////////////////////////////////////////////////
#ifndef SAP_STRUCT_H
#define SAP_STRUCT_H
#include <arpa/inet.h>	//ntoh/hton
#include "../ISA100/porting.h"

#include <boost/shared_ptr.hpp>

// Used for PROTO_VERSION_YGSAP
#define Y_TARGET_SELECTOR_TAG  0
#define Y_TARGET_SELECTOR_ADDR 1

////////////////////////////////////////////////////////////////////////////////
/// @namespace SAPStruct
/// @brief SAP structures
////////////////////////////////////////////////////////////////////////////////

//TODO namespace GSAP_IN{}
//TODO namespace GSAP_OUT{}
//TODO namespace YGSAP_OUT{}

namespace YGSAP_IN{
	/// @brief Device Tag, converted to sz (input is size-prefixed)
	struct DeviceTag{
		typedef boost::shared_ptr<DeviceTag> Ptr;
		//typedef std::auto_ptr<DeviceTag> Ptr;
		uint8_t   m_szDeviceTag[ 17 ];	/// Add one byte for null terminator

		DeviceTag( uint8_t m_uchDeviceTagSize, const uint8_t * p_aucDeviceTag )
		{	size_t size = _Min( m_uchDeviceTagSize, sizeof(m_szDeviceTag));
			strncpy((char*)m_szDeviceTag, (char*)p_aucDeviceTag, size );
			m_szDeviceTag[ size ] = 0;
		};
		operator const char *() { return (const char *)m_szDeviceTag; } 
	};
	/// @brief Device Address
	struct DeviceAddr{
		typedef boost::shared_ptr<DeviceAddr> Ptr;
		uint8_t   m_aucAddr[ 16 ];	/// Endpoint (Device) address
		DeviceAddr( const uint8_t * p_aucAddr ) { memcpy(m_aucAddr, p_aucAddr, 16); };
	};
	
	/// Device specifier using both tag and address, structure common to
	/// DEVICE_LIST_REPORT/SCHEDULE_REPORT/ DEVICE_HEALTH_REPORT/ NEIGHBOR_HEALTH_REPORT
	struct YDeviceSpecifier{
		uint8_t		m_uchTargetSelector;
		uint8_t		m_uchDeviceTagSize;		// 0-16
		uint8_t		m_aucDeviceTag[16];
		byte		m_aucNetworkAddress[16];	///<		GS_Network_Address

		int ExtractDeviceAddress( uint8_t** ) const;
		/// Return true if device specifier is empty: Target selector tag and tag size is 0
		/// @TODO TBD,target selector addr and addr all zeros is empty?
		inline bool IsEmpty( void ) const { return (Y_TARGET_SELECTOR_TAG == m_uchTargetSelector ) && (0 == m_uchDeviceTagSize); } ;
	} __attribute__ ((packed));

	// Statuses returned by ExtractDeviceAddress
	enum {STATUS_SUCCESS = 0, STATUS_INVALID_TAG_SELECTOR, STATUS_ALL_TAGS ,STATUS_TAG_NOT_FOUND};

	/// @brief YGSAP DEVICE_LIST_REPORT REQ
	//typedef YDeviceSpecifier YDeviceListReqData;
	
	/// @brief TOPOLOGY_REPORT REQ has no data

	/// @brief YGSAP SCHEDULE_REPORT REQ data, except data CRC
	//typedef YDeviceSpecifier YScheduleReqData;

	/// @brief YGSAP DEVICE_HEALTH_REPORT REQ data, except data CRC
	struct YDeviceHealthReqData	{
		uint16 				m_ushNumberOfDevices;	///< NETWORK ORDER
		YDeviceSpecifier	m_oDevice[0];			///< GS_Network_Address

		size_t SIZE( void ) const { return sizeof(YDeviceHealthReqData) + ntohs( m_ushNumberOfDevices ) * sizeof(YDeviceSpecifier); };
	}__attribute__ ((packed));
	
	/// @brief YGSAP NEIGHBOR_HEALTH_REPORT REQ data, except data CRC
	//typedef YDeviceSpecifier YNeighborHealthReqData;

	/// @brief YGSAP ALERT subscription of network address type
	/// @see AlertSvc.h TSubscNetAddr for GSAP version
	struct YSubscNetAddr
	{
		uint8 	m_ucSubscType;
		uint8  	m_ucSubscribe;
		uint8  	m_ucEnable;

		YDeviceSpecifier	m_oDevice;
		uint16 	m_ushEndpointPort;
		uint16 	m_ushObjId;
		uint8 	m_u8AlertType;

		void NTOH() { m_ushEndpointPort=ntohs(m_ushEndpointPort);m_ushObjId=ntohs(m_ushObjId);}
		
	}__attribute__ ((packed));

}
namespace GSAP_IN{
	////////////////////////////////////////////////////////////////////////////
	/// @brief System Report REQUEST data structures
	////////////////////////////////////////////////////////////////////////////
	/// @brief DEVICE_LIST_REPORT REQ has no data on standard GSAP
	/// @brief TOPOLOGY_REPORT REQ has no data

	/// @brief SCHEDULE_REPORT REQ data, except data CRC
	struct TScheduleReqData {
		byte m_aucNetworkAddress[16];	///<		GS_Network_Address
	}__attribute__ ((packed)) ;
	
	/// @brief DEVICE_HEALTH_REPORT REQ data, except data CRC
	struct TDeviceHealthReqData	{
		uint16 	m_ushNumberOfDevices;		///< 		NETWORK ORDER
		byte m_aucNetworkAddress[0][16];	///<		GS_Network_Address

		size_t SIZE( void ) const { return sizeof(TDeviceHealthReqData) + ntohs( m_ushNumberOfDevices ) * sizeof(m_aucNetworkAddress[0]); };
	}__attribute__ ((packed)) ;

	/// @brief NEIGHBOR_HEALTH_REPORT REQ data, except data CRC
	struct TNeighborHealthReqData {
		byte m_aucNetworkAddress[16];	///<		GS_Network_Address
	}__attribute__ ((packed)) ;
	
	/// @brief NETWORK_HEALTH_REPORT REQ has no data
	/// @brief NETWORK_RESOURCE_REPORT REQ has no data
}
namespace SAPStruct{

	////////////////////////////////////////////////////////////////////////////
	/// @brief System Report RESPONSE data structures
	////////////////////////////////////////////////////////////////////////////
	
	/// @brief TAI used in NetworkHealth and in TIME service
	struct TAI{
		uint32	seconds;
		uint16	fractionOfSeconds;
	} __attribute__((packed));

	/// @brief DEVICE_LIST DeviceListReport RESPONSE data
	struct DeviceListRsp {	// all members are sent network order
		typedef struct {
			byte size;
			byte data[0 /*size*/];
			size_t SIZE( void ) { return sizeof( VisibleString ) + size * sizeof(byte); }
		} __attribute__((packed)) VisibleString;

		struct Device{
			byte 			networkAddress[16];
			uint16 			deviceType; // Assigned_Device_Role ?
			byte			uniqueDeviceID[8];
			byte			powerSupplyStatus;	// 0: line; 1: > 75%; 2: [25%,75%] ;3: < 25%
			byte			joinStatus;	/// JOINED_AND_CONFIGURED = 20. all lower values means NOT joined
			VisibleString	strings[0]; //manufacturer, model, revision, device tag

			VisibleString* getManufacturerPTR( void ) const
				{ return (VisibleString*)((byte*)&strings[0]); };
			VisibleString* getModelPTR( void ) const
				{ return (VisibleString*)((byte*)getManufacturerPTR() + getManufacturerPTR()->SIZE()); };
			VisibleString* getRevisionPTR( void ) const
				{ return (VisibleString*)((byte*)getModelPTR() + getModelPTR()->SIZE()); };
			VisibleString* getDeviceTagPTR( void ) const
				{ return (VisibleString*)((byte*)getRevisionPTR() + getRevisionPTR()->SIZE()); };
			VisibleString* getSerialNoPTR( void ) const
				{ return (VisibleString*)((byte*)getDeviceTagPTR() + getDeviceTagPTR()->SIZE()); };
			
			bool VALID( int p_nPayloadSize ) const;
			
			size_t SIZE( void ) const { return sizeof(Device) + getManufacturerPTR()->SIZE()
				+ getModelPTR()->SIZE() + getRevisionPTR()->SIZE() + getDeviceTagPTR()->SIZE()
				+ getSerialNoPTR()->SIZE() ;
			};
			const Device * NEXT( void ) const { return (const Device *)((byte*)this+SIZE());};
		} __attribute__((packed)) ;

		uint16	numberOfDevices;
		Device	deviceList[ 0/*numberOfDevices*/ ];

		bool VALID( int p_nPayloadSize ) const;
		size_t SIZE( void ) const;
	} __attribute__((packed));

	/// @brief TOPOLOGY_REPORT DeviceListReport RESPONSE data
	struct TopologyReportRsp{
		struct Device{	// all members are sent network order
			struct Graph {
				uint16 	graphIdentifier;
				uint16	numberOfMembers;
				byte	graphMemberList [0 /*numberOfMembers*/ ][ 16 ];

				bool VALID( int p_nPayloadSize ) const;
				size_t SIZE( void ) const { return sizeof(Graph) + numberOfMembers * 16; };
				const Graph * NEXT( void ) const { return (const Graph *)((byte*)this+SIZE()); };
			} __attribute__((packed));
			
			struct Neighbor{
				byte	networkAddress[16];
				byte	clockSource;	//0:no;1:secondary; 2:preferred
			} __attribute__((packed));
			
			byte 		networkAddress[16];
			uint16 		numberOfNeighbors;
			uint16 		numberOfGraphs;
			Neighbor	neighborList[ 0 /*numberOfNeighbors*/ ];
			Graph 		graphList[ 0 /*numberOfGraphs*/ ];

			const Graph * GET_Graph_PTR( void ) const { return (const Graph *) ( &neighborList[ numberOfNeighbors ]);};
			bool VALID( int p_nPayloadSize ) const;
			size_t SIZE( void ) const;
			const Device * NEXT( void ) const { return (const Device *)((byte*)this+SIZE()); };
		} __attribute__((packed));

		struct Backbone{
			byte	networkAddress[16];
			uint16	subnetID;
		} __attribute__((packed));
		
		uint16 		numberOfDevices;
		uint16 		numberOfBackbones;
		Device 		deviceList[ 0 /*numberOfDevices*/ ];
		Backbone	backboneList[ 0 /*numberOfBackbones*/ ];

		const Backbone * GET_Backbone_PTR( void ) const { return (const Backbone *) ( &deviceList[ numberOfDevices ]);};
		bool VALID( int p_nPayloadSize ) const;
		size_t SIZE( void ) const;
	} __attribute__((packed));
	
	/// @brief SCHEDULE ScheduleReport RESPONSE data
	struct ScheduleReportRsp{	// all members are sent network order
		typedef struct {
			uint8 	channelNumber;	// GS_Channel_Number
			uint8 	channelStatus;	// GS_Channel_Status
		} __attribute__((packed)) Channel;

		struct DeviceSchedule{
			struct Superframe{
				typedef struct{
					byte	networkAddress[16];	// GS_Network_Address
					uint16	slotIndex;
					uint16	linkPeriod;
					uint16	slotLength;			// GS_Slot_Length
					uint8 	channelNumber;		// GS_Channel
					uint8 	direction;			// GS_Direction
					uint8 	linkType;			// GS_Link_Type
				} __attribute__((packed)) Link;

				uint16 	superframeID;							// GS_Superframe_ID
				uint16 	numberOfTimeSlots;						// GS_Num_Time_Slots
				int32	startTime;								// GS_Start_Time
				uint16 	numberOfLinks;
				Link	linkList[ 0 /*numberOfLinks*/ ];	// GS_Link_List

				bool VALID( int p_nPayloadSize ) const;
				/// @note work with structure in network order
				size_t SIZE( void ) const { return sizeof(Link)*ntohs(numberOfLinks) + sizeof(Superframe); };
				const Superframe * NEXT( void ) const { return (const Superframe *)((byte*)this+SIZE()); };
			} __attribute__((packed));

			byte 		networkAddress[16];							// GS_Network_Address
			uint16		nrOfSuperframes;
			Superframe	superframeList[ 0 /*nrOfSuperframes*/ ];	// GS_Superframe_List [];

			bool VALID( int p_nPayloadSize ) const;
			/// @note work with structure in network order
			size_t SIZE( void ) const;
			const DeviceSchedule * NEXT( void ) const { return (const DeviceSchedule *)((byte*)this+SIZE()); };
		} __attribute__((packed));			// GS_Device_Schedule

		uint8			numberOfChannels;
		uint16			numberOfDevices;
		Channel			channelList[ 0/*numberOfChannels*/ ];		// GS_Channel_List
		//DeviceSchedule	deviceSchedule[ 0 /*numberOfDevices*/ ];

		const DeviceSchedule * GET_DeviceSchedule_PTR( void ) const { return (const DeviceSchedule *) ( &channelList[ numberOfChannels ]);};
		bool VALID( int p_nPayloadSize ) const;
		/// @note work with structure in network order
		size_t SIZE( void ) const ;
	} __attribute__((packed)) ;

	/// @brief DEVICEHEALTH DeviceHealthReport RESPONSE data
	struct DeviceHealthRsp{	// all members are sent network order
		struct DeviceHealth{
			byte 	networkAddress[16];	// GS_Network_Address
			uint32 	tx;					// GS_DPDUs_Transmitted
			uint32	rx;					// GS_DPDUs_Received
			uint32	failTx;				// GS_DPDUs_Failed_Transmission
			uint32	failRx;				// GS_DPDUs_Failed_Reception
		} __attribute__((packed));

		uint16	numberOfDevices;
		DeviceHealth	deviceList[ 0/*numberOfDevices*/ ];

		bool VALID( int p_nPayloadSize ) const ;
		size_t SIZE( void ) const { return sizeof( DeviceHealth ) * ntohs(numberOfDevices) + sizeof(DeviceHealthRsp); }
	} __attribute__((packed));

	/// @brief NEIGHBOR_HEALTH_REPORT NeighborHealthReport RESPONSE data
	struct NeighborHealthRsp{	// all members are sent network order
		struct NeighborHealth{
			byte 	networkAddress[16];	// GS_Network_Address
			uint8	linkStatus;			// GS_Link_Status
			uint32	tx;					// GS_DPDUs_Transmitted
			uint32	rx;					// GS_DPDUs_Received
			uint32	failTx;				// GS_DPDUs_Failed_Transmission
			uint32	failRx;				// GS_DPDUs_Failed_Reception
			int16	signalStrength;		// GS_Signal_Strength
			uint8	signalQuality;		// GS_Signal_Quality
		} __attribute__((packed));

		uint16			numberOfNeighbors;
		NeighborHealth	neighborHealthList[ 0/*numberOfNeighbors*/ ];
		
		bool VALID( int p_nPayloadSize ) const ;
		size_t SIZE( void ) const { return sizeof( NeighborHealth ) * ntohs(numberOfNeighbors) + sizeof(NeighborHealthRsp); }
	} __attribute__((packed));

	/// @brief NETWORK_HEALTH_REPORT NetworkHealthReport RESPONSE data
	struct NetworkHealthReportRsp{	// all members are sent network order
		/// @brief NetworkHealth, part of NetworkHealthReport RESPONSE data
		struct NetworkHealth{	// all members are sent network order
			uint32	networkID;				// GS_Network_ID
			uint8	networkType;			// GS_Network_Type
			uint16	deviceCount;			// GS_Device_Count
			TAI 	startDate;				// GS_Start_Date
			TAI		currentDate;			// GS_Current_Date
			uint32	DPDUsSent;				// GS_DPDUs_Sent
			uint32	DPDUsLost;				// GS_DPDUs_Lost
			uint8	GPDULatency;			// GS_GPDU_Latency
			uint8	GPDUPathReliability;	// GS_GPDU_Path_Reliability
			uint8	GPDUDataReliability;	// GS_GPDU_Data_Reliability
			uint32	joinCount;				// GS_Join_Count

			bool VALID( int p_nPayloadSize ) const ;
		} __attribute__((packed));

		struct NetDeviceHealth{
			byte 	networkAddress[16];		// GS_Network_Address
			TAI 	startDate;				// GS_Start_Date
			TAI		currentDate;			// GS_Current_Date
			uint32	DPDUsSent;				// GS_DPDUs_Sent
			uint32	DPDUsLost;				// GS_DPDUs_Lost
			uint8	GPDULatency;			// GS_GPDU_Latency
			uint8	GPDUPathReliability;	// GS_GPDU_Path_Reliability
			uint8	GPDUDataReliability;	// GS_GPDU_Data_Reliability
			uint32	joinCount;				// GS_Join_Count
		} __attribute__((packed)) ;

		uint16			numberOfDevices;
		NetworkHealth	networkHealth;		// GS_Network_Health
		NetDeviceHealth	deviceHealthList[ 0 /*numberOfDevices*/ ]; // GS_Device_Health_List

		bool VALID( int p_nPayloadSize ) const ;
		size_t SIZE( void ) const { return sizeof( NetDeviceHealth ) * ntohs(numberOfDevices) + sizeof(NetworkHealthReportRsp); }
	} __attribute__((packed));


	/// @brief NETWORK_RESOURCE_REPORT NetworkResourceReport RESPONSE data
	struct NetworkResourceReportRsp{
		struct SubnetResource{	// all members are sent network order
			byte 	BBRAddress[ 16 ];	// GS_Network_Address
			uint16	SubnetID;			// GS_Subnet_ID associated with BBR
			uint32	SlotLength;			// GS_Slot_Length - Length of
			// timeslot, reported in microSeconds.
			uint32	SlotsOccupied;		// GS_Number_Slots_Occupied
			uint32	AperiodicData_X;	// GS_Slots_Linktype_0_X
			uint32	AperiodicData_Y;	// GS_Slots_Linktype_0_Y
			uint32	AperiodicMgmt_X;	// GS_Slots_Linktype_1_X
			uint32	AperiodicMgmt_Y;	// GS_Slots_Linktype_1_Y
			uint32	PeriodicData_X;		// GS_Slots_Linktype_2_X
			uint32	PeriodicData_Y;		// GS_Slots_Linktype_2_Y
			uint32	PeriodicMgmt_X;		// GS_Slots_Linktype_3_X
			uint32	PeriodicMgmt_Y;		// GS_Slots_Linktype_3_Y
		}__attribute__((packed));

		uint8 numberOfSubnets;
		SubnetResource subnetList[ 0 /*numberOfSubnets*/ ];

		bool VALID( int p_nPayloadSize ) const ;
		size_t SIZE( void ) const { return sizeof( SubnetResource ) * numberOfSubnets + sizeof(NetworkResourceReportRsp); }
	} __attribute__((packed));

	////////////////////////////////////////////////////////////////////////////
	/// @brief C/S RESPONSE data structures for specific requests
	////////////////////////////////////////////////////////////////////////////
	
	/// @brief EXEC SMO.getContractsAndRoutes RESPONSE
	struct ContractsAndRoutes{	/// Defined in standard, Table 29 page 152 section 6.3.11.2.6.5.2
		struct DeviceContractsAndRoutes{
			struct Contract{	// all members are sent network order
				uint16_t	contractID;		// Contract_ID
				uint8_t		serviceType;	// Communication_Service_Type  0:periodic, 1: aperiodic
				uint32_t	activationTime;		// Contract_Activation_Time
				uint16_t	sourceSAP;			// Source_SAP
				uint8_t		destinationAddress[16];	// Destination_Address
				uint16_t	destinationSAP;		// Destination_SAP
				uint32_t	expirationTime;	// Assigned_Contract_Expiration_Time
				uint8_t		priority;		// Assigned_Contract_Priority
				uint16_t	NSDUSize;		// Assigned_Max_NSDU_Size
				uint8_t		reliability;// Assigned_Reliability_And_PublishAutoRetransmit
				int16_t		period;		// Assigned_Period (for serviceType=0)
				uint8_t		phase;		// Assigned_Phase (for serviceType=0)
				uint16_t	deadline;		// Assigned_Deadline (for serviceType=0), expressed in 10 ms slots
				int16_t		comittedBurst;	// Assigned_Committed_Burst (for serviceType=1)
				int16_t		excessBurst;	// Assigned_Excess_Burst (for serviceType=1)
				uint8_t		maxSendWindow;	// Assigned_Max_Send_Window_Size (for serviceType=1)
			}__attribute__((packed));

			struct Route{
				struct RouteElement{
					uint8_t		isGraph;	// 0: is node; 1: is graph
					//union{ uint16_t graphID; uint8_t nodeAddress[16]; }
					bool VALID( int p_nPayloadSize ) const;
					size_t SIZE( void ) const { return sizeof( RouteElement ) + (isGraph ? sizeof(uint16_t) : 16); }
					const RouteElement * NEXT( void ) const { return (const RouteElement *)((byte*)this+SIZE()); };
				}__attribute__((packed));

				uint16_t		index;
				uint8_t 		size;
				uint8_t 		alternative;
				uint8_t			forwardLimit;
				RouteElement	route[ 0/*size*/ ];
				//union SelectorElement{ uint16_t contractID; uint8_t nodeAddress[16];};

				bool VALID( int p_nPayloadSize ) const;
				size_t SIZE( void ) const ;
				const Route * NEXT( void ) const { return (const Route *)((byte*)this+SIZE()); };
			}__attribute__((packed));
			
			uint8_t		networkAddress[16];
			uint8_t 	numberOfContracts;
			uint8_t 	numberOfRoutes;	// uint16?
			Contract 	contractTable[ 0 /*numberOfContracts*/ ];
			//Route		routeTable[ numberOfRoutes ];

			const Route * GET_Route_PTR( void ) const { return (const Route *) &contractTable[ numberOfContracts ];};
			bool VALID( int p_nPayloadSize ) const;
			size_t SIZE( void ) const;
			const DeviceContractsAndRoutes * NEXT( void ) const { return (const DeviceContractsAndRoutes *)((byte*)this+SIZE()); };
		}__attribute__((packed));
		
		uint8_t 				numberOfDevices;
		DeviceContractsAndRoutes	deviceList[ 0 /*numberOfDevices*/ ];

		bool VALID( int p_nPayloadSize ) const;
		//size_t SIZE( void ) const { return sizeof( RouteElement ) * size + sizeof(SelectorElement) + sizeof(Route); }
		//const ContractsAndRoutes * NEXT( void ) const { return (const ContractsAndRoutes *)((byte*)this+SIZE()); };
	}__attribute__((packed));

	////////////////////////////////////////////////////////////////////////////
	/// @brief SM Application ALERT structures
	////////////////////////////////////////////////////////////////////////////

	/// @brief SM application ALERT: JOIN OK
	typedef DeviceListRsp TValAlertJoinOK;

	/// @brief SM application ALERT: JOIN Fail
	struct TValAlertJoinFail{
		uint8_t 	eui64[8];
		uint8_t		u8Phase;
		uint8_t		u8Reason;
	} __attribute__ ((packed));

	/// @brief SM application ALERT: JOIN Leave
	struct TValAlertJoinLeave{
		uint8_t 	eui64[8];
		uint8_t		u8Reason;
	} __attribute__ ((packed));

	/// @brief SM application ALERT: UDO Transfer start
	struct TValAlertUdoStart{
		uint8_t 	eui64[8];
		uint16_t	u16BytesPerPacket;
		uint16_t	u16totalPackets;
		uint32_t	u32totalBytes;
	} __attribute__ ((packed));

	/// @brief SM application ALERT: UDO Transfer progress
	struct TValAlertUdoTransfer{
		uint8_t 	eui64[8];
		uint16_t	u16PacketsTransferred;
	} __attribute__ ((packed));

	/// @brief SM application ALERT: UDO Transfer end
	struct TValAlertUdoEnd{
		uint8_t 	eui64[8];
		uint8_t		u8ErrCode;
	} __attribute__ ((packed));
};

#endif //SAP_STRUCT_H
