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
/// @file BulkTransferSvc.h
/// @author Marcel Ionescu
/// @brief Bulk Transfer service - interface
////////////////////////////////////////////////////////////////////////////////
#ifndef BULK_TRANSFER_SERVICE_H
#define BULK_TRANSFER_SERVICE_H

#include "../ISA100/porting.h"
#include "Service.h"

#define UDO_BLOCKSIZE 98
#define BULK_RETRY_TIMEOUT 5
#define BULK_MAX_RETRIES 5

#define BULK_RESET 0
#define BULK_APPLY 1

#define UDO_MODE_DOWNLOAD	0
#define UDO_MODE_UPLOAD		1

enum UDO_RESOURCE_TYPE{
	RES_TYPE_FILE = 1,
	RES_TYPE_UNNAMED_DATA = 2,
};

enum E_UDO_STATE{
	UDO_STATE_IDLE = 0,
	UDO_STATE_DOWNLOADING,
	UDO_STATE_UPLOADING,
	UDO_STATE_APPLYING,
	UDO_STATE_DL_COMPLETE,
	UDO_STATE_UL_COMPLETE,
	UDO_STATE_DL_ERROR,
	UDO_STATE_UL_ERROR
};

typedef struct  
{
	uint32 m_nTransferID;
	unsigned long m_nLeaseID;
	UDO_RESOURCE_TYPE	m_eResourceType;
	uint16	m_ushTSAPID;
	uint16	m_ushObjId;
	uint16	m_nContractId;
	uint16	m_unResDescSize;
	uint8	m_ucMode;
	uint32	m_unBulkSize;
	uint16	m_unBlockSize;
	uint8	m_ucServiceType;
	E_UDO_STATE	m_eState;///< the state of the the remote UDO as we know it
	uint16	m_unAppHandle;
	//--------after initialization
	uint8  m_aucAddr[16];
	uint16	m_unCurrentBlock;
	uint8*	m_pucResDesc;
	CMicroSec m_uSec;
}RemUDOData;

//Status for Bulk Open Confirm:

#define BULKO_SUCCESS			0	// 0 Success
#define BULKO_FAIL_EXCEED_LIM	1	// 1 Failure item exceeds limits
#define BULKO_FAIL_UNKNOWN_RES	2	// 2  Failure unknown resource
#define BULKO_FAIL_INVALID_MODE	3	// 3  Failure invalid mode
#define BULKO_FAIL_OTHER		4	// 4 Failure other

//Status for Bulk Transfer Confirm
#define BULKT_SUCCESS		0	// 0 Success
#define BULKT_FAIL_COMM		1	// 1 Failure communication failed
#define BULKT_FAIL_ABORT	2	// 2 Failure transfer aborted
#define BULKT_FAIL_OTHER	3	// 3 Failure other

// Status Bulk Close Confirm
#define BULKC_SUCCESS	0
#define BULKC_FAIL		1	//This value do not exist in ISA100 standard

enum{
  UDO_RES = 0,        // reserved for future use
  UDO_OPS,            // operations supported:
			//    0=defined size unicast upld only
			//    1=defined size unicast dwld only
			//    2=defined size unicast upd & unicast dwld
			//    3-15 res. for future use
  UDO_DESCR,          // human readable identification of associated content
  UDO_STATE,          // 0=idle, 1=downloading, 2=uploading, 3=applying
			// 4=dwld complete, 5=upld complete, 6=dwld err, 7=upld err
  UDO_COMM,           // 0=reset, 1=apply(dwld only), 2-15 res. for future use
  UDO_MAX_BLCK_SIZE,
  UDO_MAX_DWLD_SIZE,
  UDO_MAX_UPLD_SIZE,
  UDO_DWLD_PREP_TIME, // time required, in seconds, to prepare for a dwld
  UDO_DWLD_ACT_TIME,  // time in seconds for the obj to activate newly dwlded content
  UDO_UPLD_PREP_TIME, // time required in seconds to prepare for an upld
  UDO_UPLD_PR_TIME,   // typical time in seconds for this obj to process a req to
			// upld a block
  UDO_DWLD_PR_TIME,   // typical time in seconds for this object to process a
			// downloaded block
  UDO_CUTOVER_TIME,   // time specified to activate the download content
  UDO_LAST_BLK_DWLD,   // time specified to activate the dwld content
  UDO_LAST_BLK_UPLD,
  UDO_ERROR_CODE,
  UDO_ATTR_NO
};//UDO_ATTRIBUTES;

enum {
 UDO_ZERO,//=0,
 UDO_START_DWLD,//=1,
 UDO_DWLD_DATA,//=2,
 UDO_END_DWLD,//=3,
 UDO_START_UPLD,//=4,
 UDO_UPLD_DATA,//=5,
 UDO_END_UPLD,//=6,
 UDO_NO,
 UDO_SET_FILE=132,
 UDO_GROUP_FW_ACTIVATION_TAI
};//UDO_METHODS;

enum{
	UDO_END_DOWNLOAD_SUCCESS,//=0
	UDO_END_DOWNLOAD_ABORT//=1
};//UDO_RATIONALES;


////////////////////////////////////////////////////////////////////////////////
/// @class CBulkTransferService
/// @brief Bulk Transfer Service
/// @see Service.h for details
////////////////////////////////////////////////////////////////////////////////
class CBulkTransferService: public CService{

	std::list<RemUDOData> RemUDOList;
	
	bool	readMaxDownloadSize( MSG * p_pMSG, std::list<RemUDOData>::iterator& it );
	bool	askState( std::list<RemUDOData>::iterator& it );
	bool	setFile( MSG * p_pMSG, std::list<RemUDOData>::iterator& it );
	bool	startDownload( MSG * p_pMSG, std::list<RemUDOData>::iterator& it );
	bool	endDownload( std::list<RemUDOData>::iterator& it, uint8 p_nRationale, uint32 p_nSessionId, uint32 p_nTransactionId);
	void	apply( MSG * p_pMSG, std::list<RemUDOData>::iterator& it );
	bool	sendReset( MSG * p_pMSG, std::list<RemUDOData>::iterator& it );
	void	cleanupUDO(std::list<RemUDOData>::iterator& it);
	bool	tryReset( std::list<RemUDOData>::iterator& it);

public:
	/// @brief Bulk Open REQUEST data 
	typedef struct
	{
		uint32	m_unLeaseID;		///< 		NETWORK ORDER
		uint32	m_unTransferID;		///< 		NETWORK ORDER
		uint8 	m_ucMode;
		uint16	m_ushBlockSize;		///< 		NETWORK ORDER
		uint32	m_unItemSize;		///< 		NETWORK ORDER
		uint8	m_eResourceType;
		uint8 	m_ucNameSize;
		uint8	m_aFilename[0];
		void NTOH( void ) { m_unLeaseID = ntohl(m_unLeaseID); m_unTransferID = ntohl(m_unTransferID);
					m_ushBlockSize = ntohs(m_ushBlockSize); m_unItemSize = ntohl(m_unItemSize);};
	}__attribute__((packed)) TBulkOpenRequestData;


	/// @brief Bulk Transfer REQUEST data (data starting from lease ID, following header CRC )
	typedef struct 
	{
		uint32 m_unLeaseID;			///< 		NETWORK ORDER
		uint32 m_unTransferID;		///< 		NETWORK ORDER
		//uint16 m_ushBlockNumber;		///< 		NETWORK ORDER - note: this member is actually inluded as the first two bytes of m_ucBulkData
		uint8 m_ucBulkData[0];
		void NTOH( void ) { m_unLeaseID = ntohl(m_unLeaseID);m_unTransferID = ntohl(m_unTransferID); };
	}__attribute__((packed)) TBulkTransferRequestData;

	/// @brief Bulk Close REQUEST data 
	typedef struct 
	{
		uint32 m_unLeaseID;///< 		NETWORK ORDER
		uint32 m_unTransferID;
		void NTOH( void ) { m_unLeaseID = ntohl(m_unLeaseID); m_unTransferID = ntohl(m_unTransferID);};
	}__attribute__((packed)) TBulkCloseRequestData;

	/// @brief Bulk Open CONFIRM data 
	typedef struct 
	{
		uint8 m_ucStatus; ///< 	NETWORK ORDER
		uint16 m_ushBlockSize; ///< 	NETWORK ORDER
		uint32 m_unItemSize; ///< 	NETWORK ORDER
		void HTON(){ m_ushBlockSize=htons(m_ushBlockSize) ; m_unItemSize=htonl(m_unItemSize); }
	}__attribute__((packed)) TBulkOpenConfirmData;

	CBulkTransferService( const char * p_szName ) :CService(p_szName){};
	
	/// (USER -> GW) Process a user request
	/// CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
	virtual bool ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData );
	
	/// Return true if the service is able to handle the service type received as parameter
	virtual bool CanHandleServiceType( uint8 p_ucServiceType ) const;
	
	/// (ISA -> GW) Dispatch an ADPU. Call from CInterfaceObjMgr::DispatchAPDU
	virtual bool ProcessAPDU(uint16 p_unAppHandle, APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq );

	/// (ISA -> GW) Proces a ISA100 timeout. Call from CInterfaceObjMgr::DispatchISATimeout
	virtual bool ProcessISATimeout( uint16 p_unAppHandle, GENERIC_ASL_SRVC * p_pOriginalReq );

	/// (ISA -> GW) Proces a ISA100  
	/// @remarks Method called on Lease delete (explicit delete or expire)
	virtual void OnLeaseDelete( lease* p_pLease );
	
	virtual void Dump(void);
	
private:
	bool requestBulkOpen( CGSAP::TGSAPHdr * p_pHdr, void * p_pData );
	bool requestBulkTransfer( CGSAP::TGSAPHdr * p_pHdr, void * p_pData );
	bool requestBulkClose(	CGSAP::TGSAPHdr * p_pHdr, void * p_pData );
	bool confirmBulkOpen(/*uint8 p_nServiceType,*/  unsigned p_nSessionID, unsigned p_nTransactionID, uint8 p_ucStatus, uint32 p_nBlockSize, uint32 p_nItemSize );
	bool confirmBulkTransfer(/*uint8 p_nServiceType,*/  unsigned  p_nSessionID, unsigned p_nTransactionID,  uint8 p_ucStatus );
	bool confirmBulkClose(/*uint8 p_nServiceType,*/ unsigned  p_nSessionID, unsigned p_nTransactionID, uint8 p_ucStatus );

	bool processStateIdle(MSG * p_pMSG, std::list<RemUDOData>::iterator& it, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq );
	bool processStateDownload(MSG * p_pMSG, std::list<RemUDOData>::iterator& it, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq );
	bool processStateDowldCompl( MSG * p_pMSG, std::list<RemUDOData>::iterator& it, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq );
	bool processStateApply( MSG * p_pMSG, std::list<RemUDOData>::iterator& it, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq );
	bool processStateDLError( MSG * p_pMSG, std::list<RemUDOData>::iterator& it, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq );
	bool findTransferByLease(std::list<RemUDOData>::iterator& it, uint32 p_unLeaseID );
	bool findTransferByAppHandle(std::list<RemUDOData>::iterator& it, uint16 p_unAppHandle);

	const char * getBulkStateName( E_UDO_STATE p_eState );
	void advanceState( std::list<RemUDOData>::iterator p_it, E_UDO_STATE p_eNewState );
};

#endif //BULK_TRANSFER_SERVICE_H
