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
/// @file ClientServerSvc.h
/// @author Marcel Ionescu
/// @brief Client/server GSAP - interface
////////////////////////////////////////////////////////////////////////////////
#ifndef CLIENT_SERVER_SVC_H
#define CLIENT_SERVER_SVC_H

#include "../ISA100/typedef.h"
#include "../ISA100/porting.h" 	// GENERIC_ASL_SRVC
#include "Service.h"

/// ClientServerStatus
#define CLIENTSERVER_SUCCESS		0	/// Success
#define CLIENTSERVER_NOT_AVAILABLE	1	/// Failure; server inaccessible for unbuffered request
#define CLIENTSERVER_BUFFER_INVALID	2	/// Failure; server inaccessible and client buffer invalid for buffered request
#define CLIENTSERVER_LEASE_EXPIRED	3	/// Failure; lease has expired
#define CLIENTSERVER_FAIL_OTHER		4	/// Failure; other

////////////////////////////////////////////////////////////////////////////////
/// @class CClientServerService
/// @brief Client/Server Service
/// @see Service.h for details
////////////////////////////////////////////////////////////////////////////////
class CClientServerService: public CService
{
	////////////////////////////////////////////////////////////////////////////
	/// @brief REQUEST data structures
	////////////////////////////////////////////////////////////////////////////
	/// @brief Client/Server REQ data - read section, part of TClientServerReqData
	/// @note must be outside the main struct to work on ARM with -Os
	typedef struct{	/// GS_Request_Data for native read requests
		uint16	m_ushObjectID;	///< 		NETWORK ORDER
		uint16	m_ushAttrID;	///< 		NETWORK ORDER
		uint16	m_ushAttrIdx1;	///< 		NETWORK ORDER
		uint16	m_ushAttrIdx2;	///< 		NETWORK ORDER
		void NTOH( void ) { m_ushObjectID = ntohs(m_ushObjectID); m_ushAttrID = ntohs(m_ushAttrID);
			m_ushAttrIdx1 = ntohs(m_ushAttrIdx1); m_ushAttrIdx2 = ntohs(m_ushAttrIdx2);};
	} __attribute__ ((packed)) TClientServerReqData_Read;

	/// @brief Client/Server REQ data - write section, part of TClientServerReqData
	/// @note must be outside the main struct to work on ARM with -Os
	typedef struct{	/// GS_Request_Data for native write requests
		uint16	m_ushObjectID;	///< 		NETWORK ORDER
		uint16	m_ushAttrID;	///< 		NETWORK ORDER
		uint16	m_ushAttrIdx1;	///< 		NETWORK ORDER
		uint16	m_ushAttrIdx2;	///< 		NETWORK ORDER
		uint16	m_ushReqLen;	///< 		NETWORK ORDER
		uint8	m_aReqData[0];	/// variable-size req data here
		void NTOH( void ) { m_ushObjectID = ntohs(m_ushObjectID); m_ushAttrID = ntohs(m_ushAttrID);
			m_ushAttrIdx1 = ntohs(m_ushAttrIdx1); m_ushAttrIdx2 = ntohs(m_ushAttrIdx2); m_ushReqLen = ntohs(m_ushReqLen); };
	}  __attribute__ ((packed)) TClientServerReqData_Write;

	/// @brief Client/Server REQ data - exec data, part of TClientServerReqData
	/// @note must be outside the main struct to work on ARM with -Os
	typedef struct{	/// GS_Request_Data for native execute requests
		uint16	m_ushObjectID;	///<		NETWORK ORDER
		uint16	m_ushMethodID;	///<		NETWORK ORDER
		uint16	m_ushReqLen;	///<		NETWORK ORDER
		uint8	m_aReqData[0];	/// variable-size req data here
		void NTOH( void ) { m_ushObjectID=ntohs(m_ushObjectID); m_ushMethodID=ntohs(m_ushMethodID); m_ushReqLen=ntohs(m_ushReqLen); };
	}  __attribute__ ((packed)) TClientServerReqData_Exec;

	/// @brief Client/Server REQ data, except data CRC
	typedef struct	{
		uint32 	m_nLeaseID;			///<		NETWORK ORDER
		uint8	m_ucBuffer;			///<		buffering
		uint8	m_ucTransferMode;	///<		transfer mode
		/// GS_Request_Data start here
		uint8	m_ucReqType;		///< (SRVC_TYPE_READ / SRVC_TYPE_WRITE / SRVC_TYPE_EXECUTE)
		union {
			TClientServerReqData_Read  m_stReadData;
			TClientServerReqData_Write m_stWriteData;
			TClientServerReqData_Exec  m_stExecData;
		} __attribute__ ((packed)) m_uData;	///USER MUST CALL respective member m_stReadData/m_stWriteData/m_stExecData NTOH(), depending on m_ucReqType
		void NTOH( void ) { m_nLeaseID = ntohl(m_nLeaseID); };
	}__attribute__ ((packed)) TClientServerReqData;

public:
	////////////////////////////////////////////////////////////////////////////
	/// @brief CONFIRM data structures
	////////////////////////////////////////////////////////////////////////////
	/// @brief Client/Server CONFIRM data for READ/EXECUTE requests, except data CRC
	typedef struct	{
		byte 	m_ucStatus;		///< GS_Status
		uint32	m_unTxSeconds;	/// Device TX time: seconds unix time
		uint32	m_unTxuSeconds;	/// Device TX time: micro seconds from top of the second
		uint8	m_ucReqType;
		uint16	m_ushObjectID;	///< 		NETWORK ORDER
		uint8	m_ucSFC;		///< Service Feedback Code
		uint16	m_ushRspLen;	///< 		NETWORK ORDER
		uint8	m_aRspData[0];	/// variable-size rsp data here
		void HTON( void ) { m_unTxSeconds = htonl(m_unTxSeconds); m_unTxuSeconds = htonl(m_unTxuSeconds); m_ushObjectID = htons(m_ushObjectID); m_ushRspLen = htons(m_ushRspLen); };
	}__attribute__ ((packed)) TClientServerConfirmData_RX;

	/// @brief Client/Server CONFIRM data for WRITE requests, except data CRC
	typedef struct	{
		byte 	m_ucStatus;		///< GS_Status
		uint32	m_unTxSeconds;		/// Device TX time: seconds unix time
		uint32	m_unTxuSeconds;		/// Device TX time: micro seconds from top of the second
		uint8	m_ucReqType;
		uint16	m_ushObjectID;	///< 		NETWORK ORDER
		uint8	m_ucSFC;		///< Service Feedback Code
		void HTON( void ) { m_unTxSeconds = htonl(m_unTxSeconds); m_unTxuSeconds = htonl(m_unTxuSeconds); m_ushObjectID = htons(m_ushObjectID); };
	}__attribute__ ((packed)) TClientServerConfirmData_W;

	////////////////////////////////////////////////////////////////////////////
	/// @brief Client/Server cache section
	////////////////////////////////////////////////////////////////////////////
	/// @brief The cache key: m_ucReqType (SRVC_TYPE_READ/SRVC_TYPE_WRITE/SRVC_TYPE_EXECUTE) / request data
	struct CSKey{
		uint16  m_unContractID;	///< Destination Addr:TSAPID identified by contract ID
		uint8	m_ucBuffer;		///< buffering. Not considered by cacheCompare::operator()
		uint8	m_ucReqType;	///< GS_Request_Data first byte (SRVC_TYPE_READ / SRVC_TYPE_WRITE / SRVC_TYPE_EXECUTE)
		time_t  m_tCreat;		///< Creation(request) time. TODO: UPDATE on key match on req
		uint16 	m_ushDataLen;	///< Length of REQUEST data
		uint8 * m_pData;		///< Request data, part of the key. Starts with TClientServerReqData::m_uData

		CSKey( uint16 p_unContractID, uint8 p_ucBuffer, uint8 p_ucReqType, const uint8 * p_pData );
		~CSKey();

		/// Identify the key
		const char * Identify( void ) const;

		private:
		CSKey( const CSKey& p_rKey ){}; ///forbid copy
	};

	/// @brief The cache data - the response to ClientServer req
	struct CSData{
		time_t  m_tLastRx;
		uint16 	m_ushDataLen;	///< Length of RESPONSE data
		byte *  m_pData;

		/// Just allocate the memory. Does not fill it
		CSData( uint16 	p_ushDataLen);
		~CSData();

		/// Just resize the memory. Does not fill nor copy it. User must fill the memory later
		/// TAKE CARE: any data stored is LOST
		void resize( uint16 p_ushDataLen )
		{	if( p_ushDataLen != m_ushDataLen)
			{	m_ushDataLen = p_ushDataLen;
				delete m_pData;
				m_pData = new byte[ m_ushDataLen ];
			}/// else do nothing
		}
		void updateLastRx( void ) { m_tLastRx = time(NULL); };
		bool IsValid( time_t p_tLifetime ) const { return (time(NULL) - m_tLastRx) <= p_tLifetime; };

		private:
		CSData( const CSData& p_rData ){};	/// forbid copy
	};
private:

	/// @brief Functor used by TCSMap as comparator
	struct cacheCompare { inline bool operator()( CSKey* p_pK1, CSKey* p_pK2 ) const; };

	/// Map user requests (down to binary details, for exact match) to device responses
	/// Map Key: publish data identifier: device address, publish object id, content version ? data size?
	/// Map Data: pointer to Client/Server response data
	typedef std::map<CSKey*, CSData*, cacheCompare> TCSMap;

	/// Map application handles to user requests.
	/// Map Key = application handler as returned by SendRequestToASL
	/// Map Data: pending C/S request: a pointer to CSKey (store keys here until response or timeout arrives)
	typedef std::map<uint16, CSKey*> TCSPending;

	/// TODO: periodically ERASE EXPIRED entries
	TCSMap		m_mapCache;
	TCSPending	m_mapPendingReq;

public:
	CClientServerService( const char * p_szName ) :CService(p_szName){};
	virtual ~CClientServerService( ) ;

	/// Return true if the service is able to handle the service type received as parameter
	/// Used by (USER -> GW) flow
	virtual bool CanHandleServiceType( uint8 p_ucServiceType ) const;

	/// (USER -> GW) Process a user request
	/// CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
	virtual bool ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData );

	/// (ISA -> GW) Dispatch an ADPU. Call from CInterfaceObjMgr::DispatchAPDU
	virtual bool ProcessAPDU( uint16 p_unAppHandle, APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq );

	/// (ISA -> GW) Process a ISA100 timeout. Call from CInterfaceObjMgr::DispatchISATimeout
	virtual bool ProcessISATimeout( uint16 p_unAppHandle, GENERIC_ASL_SRVC * p_pOriginalReq );

	/// Process a lease deletion: free resources, if necessary. Only some derived classess will implement
	/// Identified services using it: ClientServer service, BulkTransfer Service
	/// Default implementation does nothing
	virtual void OnLeaseDelete( lease* p_pLease );

	/// Called on USR2: Dump status to LOG
	virtual void Dump( void );

	/// Request contract list from SM for GS_GPDU_Latency/GS_GPDU_Data_Reliability parameters in network report
	void RequestContractList( void );

private:
	bool processUserRequestTunnel( CGSAP::TGSAPHdr * p_pHdr, void * p_pData, lease* p_pLease );

	/// (GW -> User) Send CONFIRM to user with status ok and response data, using channel identified by p_nSessionID
	void clientServerConfirm(	unsigned p_nSessionID, unsigned p_unTransactionID, uint8* p_pMsgData, uint32 p_dwMsgDataLen, bool p_bRemoveFromTracker = true  );

	/// (GW -> User) Send CONFIRM to user with status - some error
	void clientServerConfirm(	unsigned p_nSessionID, unsigned p_unTransactionID, byte p_ucStatus, bool p_bRemoveFromTracker = true );

	/// (GW -> User) Send CONFIRM to user with status ok and response data, using channel identified by p_unAppHandle
	void clientServerConfirm(	uint16 p_unAppHandle, uint8* p_pMsgData, uint32 p_dwMsgDataLen );
};

inline bool operator==( const CClientServerService::CSKey& p_rFirst, const CClientServerService::CSKey& p_rSecond)
{
	return 	( p_rFirst.m_unContractID == p_rSecond.m_unContractID )
		&&	( p_rFirst.m_ucReqType    == p_rSecond.m_ucReqType )
		&&	( p_rFirst.m_ushDataLen   == p_rSecond.m_ushDataLen )
		&&	( memcmp( p_rFirst.m_pData, p_rSecond.m_pData, p_rFirst.m_ushDataLen) == 0);
}

#endif //CLIENT_SERVER_SVC_H
