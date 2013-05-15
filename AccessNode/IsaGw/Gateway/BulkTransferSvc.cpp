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
/// @file BulkTransferSvc.cpp
/// @author Marcel Ionescu
/// @brief Bulk Transfer service - implementation
////////////////////////////////////////////////////////////////////////////////
#include "../../Shared/Common.h"

#include "GwApp.h"
#include "BulkTransferSvc.h"

////////////////////////////////////////////////////////////////////////////////
/// @class CBulkTransferService
/// @brief Bulk Transfer service
////////////////////////////////////////////////////////////////////////////////

bool CBulkTransferService::readMaxDownloadSize( MSG * p_pMSG, std::list<RemUDOData>::iterator& it)
{
	READ_REQ_SRVC stReadRec={
		m_unSrcOID:	m_ushInterfaceOID,
		m_unDstOID:	it->m_ushObjId,
		m_ucReqID:	0,
		m_stAttrIdtf:{	m_ucAttrFormat: ATTR_NO_INDEX, m_unAttrID: UDO_MAX_DWLD_SIZE, 0, 0  }
	};
	if(!(it->m_unAppHandle = g_stApp.m_oGwUAP.SendRequestToASL( &stReadRec, SRVC_READ_REQ, it->m_ushTSAPID, it->m_nContractId, p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, it->m_ucServiceType, p_pMSG->m_unAppHandle )))
	{ /// Remove also from tracker
		LOG("ERROR BULK OPEN readMaxDownloadSize(S %u T %u) can't send request to ASL", p_pMSG->m_nSessionID, p_pMSG->m_TransactionID);
		confirmBulkOpen( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKO_FAIL_OTHER, it->m_unBlockSize, it->m_unBulkSize );
		cleanupUDO(it);
		return false;
	}
	return true;
}

bool CBulkTransferService::setFile( MSG * p_pMSG, std::list<RemUDOData>::iterator& it)
{
	uint8 cDataToTransfer[it->m_unResDescSize+2];
	EXEC_REQ_SRVC stExecRec={
		m_unSrcOID:	m_ushInterfaceOID,
		m_unDstOID:	it->m_ushObjId,
		m_ucReqID:	0,
		m_ucMethID:	UDO_SET_FILE,
		m_unLen:	it->m_unResDescSize + 2, //TODO: +2 is the size of the string prepended as per Pocol; will probably be deleted (shouldn't this be NO?)
		p_pReqData:	cDataToTransfer
	};
	
	*(uint16*)stExecRec.p_pReqData = htons(it->m_unResDescSize);
	memcpy(stExecRec.p_pReqData+2, it->m_pucResDesc, it->m_unResDescSize);
	
	if(!(it->m_unAppHandle = g_stApp.m_oGwUAP.SendRequestToASL( &stExecRec, SRVC_EXEC_REQ, it->m_ushTSAPID, it->m_nContractId, p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, it->m_ucServiceType, p_pMSG->m_unAppHandle )))
	{ /// Remove also from tracker
		LOG("ERROR BULK OPEN setFile(S %u T %u) can't send request to ASL", p_pMSG->m_nSessionID, p_pMSG->m_TransactionID);
		confirmBulkOpen( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKO_FAIL_OTHER, it->m_unBlockSize, it->m_unBulkSize );
		cleanupUDO(it);
		return false;
	}
	return true;
}

bool CBulkTransferService::startDownload( MSG * p_pMSG, std::list<RemUDOData>::iterator& it)
{
	struct
	{
		uint16 m_unBSize;
		uint32 m_unDSize;
		uint8 m_nOMode;
	}__attribute__ ((packed)) startDLParams = {
		m_unBSize:	htons(it->m_unBlockSize),
		m_unDSize:	htonl(it->m_unBulkSize),
		m_nOMode:	UDO_MODE_DOWNLOAD
	};
	
	EXEC_REQ_SRVC stExecRec = {
		m_unSrcOID: m_ushInterfaceOID,
		m_unDstOID: it->m_ushObjId,
		m_ucReqID: 0,
		m_ucMethID: UDO_START_DWLD,
		m_unLen: sizeof(startDLParams),
		p_pReqData: (uint8 *)&startDLParams
	};
	
	if(!(it->m_unAppHandle = g_stApp.m_oGwUAP.SendRequestToASL( &stExecRec, SRVC_EXEC_REQ, it->m_ushTSAPID, it->m_nContractId, p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, it->m_ucServiceType, p_pMSG->m_unAppHandle )))
	{ /// Remove also from tracker
		LOG("ERROR BULK OPEN startDownload(S %u T %u) can't send request to ASL", p_pMSG->m_nSessionID, p_pMSG->m_TransactionID);
		confirmBulkOpen( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKO_FAIL_OTHER, it->m_unBlockSize, it->m_unBulkSize);
		cleanupUDO(it);
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Request a bulk open
/// @param p_pHdr		GSAP request header (has member m_nDataSize)
/// @param pData		GSAP request data (data size is in p_pHdr)
/// @return tbd
/// @remarks 
////////////////////////////////////////////////////////////////////////////////
bool  CBulkTransferService::requestBulkOpen( CGSAP::TGSAPHdr * p_pHdr, void * p_pData )
{
	std::list<RemUDOData>::iterator it;
	TBulkOpenRequestData * pOpenReq = (TBulkOpenRequestData *)p_pData;
	if( p_pHdr->m_nDataSize < offsetof(TBulkOpenRequestData, m_eResourceType) + 4)
	{
		LOG("ERROR BULK OPEN requestBulkOpen(S %u T %u) buffer too small %d", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->m_nDataSize);
		confirmBulkOpen( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_FAILURE : BULKO_FAIL_OTHER, pOpenReq->m_ushBlockSize, pOpenReq->m_unItemSize);
		return false;
	}
	pOpenReq->NTOH();
	
	const lease *pLease=g_pLeaseMgr->FindLease( pOpenReq->m_unLeaseID, p_pHdr->m_nSessionID );
	if ( (!pLease) || (LEASE_BULK_CLIENT != pLease->eLeaseType) || (!pLease->nContractId))
	{	char szExtra[64] = {0};
		if( pLease )
			sprintf(szExtra, " Type %u Contract %u", pLease->eLeaseType, pLease->nContractId);
		LOG("ERROR BULK OPEN requestBulkOpen(S %u T %u) invalid/expired lease id %u%s", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pOpenReq->m_unLeaseID, szExtra);
		confirmBulkOpen( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_INVALID_LEASE : BULKO_FAIL_UNKNOWN_RES, pOpenReq->m_ushBlockSize, pOpenReq->m_unItemSize);
		return false;
	}
	if(findTransferByLease( it, pOpenReq->m_unLeaseID))
	{
		LOG("ERROR BULK OPEN requestBulkOpen(S %u T %u) transfer in progress for lease id %u with transferID %u",
			p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pOpenReq->m_unLeaseID, it->m_nTransferID );
		confirmBulkOpen( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_FAILURE : BULKO_FAIL_OTHER, pOpenReq->m_ushBlockSize, pOpenReq->m_unItemSize);
		return false;
	}

	if( (pOpenReq->m_ucMode != UDO_MODE_DOWNLOAD) && (pOpenReq->m_ucMode != UDO_MODE_UPLOAD))
	{	LOG("ERROR BULK OPEN requestBulkOpen(S %u T %u) transfer ID %d : invalid mode %u", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pOpenReq->m_unTransferID, pOpenReq->m_ucMode);
		confirmBulkOpen( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_INVALID_MODE : BULKO_FAIL_INVALID_MODE, pOpenReq->m_ushBlockSize, pOpenReq->m_unItemSize);
		return false;
	}

	NLME_CONTRACT_ATTRIBUTES* pContract = NLME_FindContract(pLease->nContractId);
	if(!pContract)
	{
		LOG("ERROR BULK OPEN requestBulkOpen(S %u T %u) transfer ID %d : no contract! (%d)", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pOpenReq->m_unTransferID, pLease->nContractId);
		confirmBulkOpen( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_INVALID_LEASE : BULKO_FAIL_UNKNOWN_RES, pOpenReq->m_ushBlockSize, pOpenReq->m_unItemSize);
		return false;
	}
	uint16_t nMaxGWBlockSize = pContract->m_unAssignedMaxNSDUSize -16 /*UDP+Sec+MIC*/ -9 /*EXEC_REQ_SRVC*/ -2 /*BlockNumber/Unsigned16*/ ;

	RemUDOData stRUdoData=
	{
		m_nTransferID:	pOpenReq->m_unTransferID,
		m_nLeaseID:	pOpenReq->m_unLeaseID,
		m_eResourceType:(UDO_RESOURCE_TYPE)pOpenReq->m_eResourceType,
		m_ushTSAPID:	pLease->m_ushTSAPID,
		m_ushObjId:	pLease->nObjId,
		m_nContractId:	pLease->nContractId,
		m_unResDescSize:pOpenReq->m_eResourceType == RES_TYPE_FILE ? pOpenReq->m_ucNameSize : 0,
		m_ucMode:	pOpenReq->m_ucMode,
		m_unBulkSize:	pOpenReq->m_unItemSize,
		m_unBlockSize:	pOpenReq->m_ushBlockSize && pOpenReq->m_ushBlockSize < nMaxGWBlockSize ? pOpenReq->m_ushBlockSize : nMaxGWBlockSize,
		m_ucServiceType:BULK_TRANSFER_OPEN,
		m_eState:	UDO_STATE_IDLE,
		0, {0}, 0, 0
	};
	stRUdoData.m_pucResDesc = stRUdoData.m_unResDescSize ? new uint8[stRUdoData.m_unResDescSize] : NULL;
	memcpy( stRUdoData.m_pucResDesc, pOpenReq->m_aFilename, stRUdoData.m_unResDescSize );
	memcpy( stRUdoData.m_aucAddr, pLease->netAddr, sizeof(pLease->netAddr) );

	READ_REQ_SRVC stReadRec={
		m_unSrcOID:	m_ushInterfaceOID,
		m_unDstOID:	stRUdoData.m_ushObjId,
		m_ucReqID: 0,
		m_stAttrIdtf: {m_ucAttrFormat: ATTR_NO_INDEX, m_unAttrID: UDO_MAX_BLCK_SIZE, 0, 0}
	};
	if(!(stRUdoData.m_unAppHandle = g_stApp.m_oGwUAP.SendRequestToASL( &stReadRec, SRVC_READ_REQ, pLease->m_ushTSAPID, pLease->nContractId, pLease->nSessionId, p_pHdr->m_unTransactionID, stRUdoData.m_ucServiceType )))
	{ /// Remove also from tracker
		LOG("ERROR BULK OPEN requestBulkOpen(S %u T %u) can't send request to ASL", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID);
		confirmBulkOpen( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_FAILURE : BULKO_FAIL_OTHER, pOpenReq->m_ushBlockSize, pOpenReq->m_unItemSize);
		return false;
	}
	RemUDOList.push_back(stRUdoData);
	return true;
}

bool CBulkTransferService::findTransferByLease(std::list<RemUDOData>::iterator& it, uint32 p_unLeaseID )
{
	for(it=RemUDOList.begin();it!=RemUDOList.end();it++)
	{
		if(it->m_nLeaseID == p_unLeaseID)
		{
			break;
		}
	}
	if( it==RemUDOList.end() )
	{
		return false;
	}else{
		return true;
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Search transfer by transaction id
/// @param it [OUT] reference to iterator identifying the transaction, if found, otherwise undefined
/// @return true if found, false if not found
////////////////////////////////////////////////////////////////////////////////
bool CBulkTransferService::findTransferByAppHandle(std::list<RemUDOData>::iterator& it, uint16 p_unAppHandle )
{
	for( it = RemUDOList.begin(); it != RemUDOList.end(); ++it )
	{
		if(it->m_unAppHandle == p_unAppHandle)
		{
			break;
		}
	}
	return ( it != RemUDOList.end() );
}


bool  CBulkTransferService::requestBulkTransfer(CGSAP::TGSAPHdr * p_pHdr, void * p_pData)
{
	TBulkTransferRequestData *pTransferReq=(TBulkTransferRequestData *)p_pData;
	if( p_pHdr->m_nDataSize < sizeof(TBulkTransferRequestData) + sizeof (uint16)/*BlockNumber (included in BulkData)*/  + 4)
	{
		LOG("ERROR BULK TRANSFER requestBulkTransfer(S %u T %u) buffer too small %d", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->m_nDataSize);
		confirmBulkTransfer( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_FAILURE : BULKO_FAIL_OTHER );
		return false;
	}
	pTransferReq->NTOH();

	std::list<RemUDOData>::iterator it;
	if( !findTransferByLease( it, pTransferReq->m_unLeaseID ) )
	{
		LOG("ERROR BULK TRANSFER requestBulkTransfer(S %u T %u) Couldn't find transfer for Lease id %u", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pTransferReq->m_unLeaseID);
		confirmBulkTransfer( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID,  p_pHdr->IsYGSAP() ? YGS_FAILURE : BULKT_FAIL_OTHER );
		return false;
	}
	
	const lease *pLease=g_pLeaseMgr->FindLease( pTransferReq->m_unLeaseID, p_pHdr->m_nSessionID );
	if ( (!pLease) || (LEASE_BULK_CLIENT != pLease->eLeaseType) || (!pLease->nContractId))
	{	char szExtra[64] = {0};
		if( pLease )
			sprintf(szExtra, " Type %u Contract %u", pLease->eLeaseType, pLease->nContractId);
		LOG("ERROR BULK TRANSFER requestBulkTransfer(S %u T %u) invalid/expired lease id %u%s", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pTransferReq->m_unLeaseID, szExtra );
		confirmBulkTransfer( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID,  p_pHdr->IsYGSAP() ? YGS_ABORTED : BULKT_FAIL_ABORT );
		cleanupUDO(it);
		return false;
	}
	
	EXEC_REQ_SRVC stExecReq = {
		m_unSrcOID: m_ushInterfaceOID, 
		m_unDstOID: it->m_ushObjId,
		m_ucReqID: 0,
		m_ucMethID: UDO_DWLD_DATA,
		m_unLen: p_pHdr->m_nDataSize - 3*sizeof(uint32), //we send the 'Bulk block nr' and 'Bulk data' elements. Substract the size of leaseID, transferID, data CRC
		p_pReqData: pTransferReq->m_ucBulkData
	};
	it->m_ucServiceType = BULK_TRANSFER_TRANSFER;
	if(!(it->m_unAppHandle = g_stApp.m_oGwUAP.SendRequestToASL(&stExecReq, SRVC_EXEC_REQ, pLease->m_ushTSAPID, pLease->nContractId,  pLease->nSessionId, p_pHdr->m_unTransactionID, it->m_ucServiceType)))
	{ /// Remove also from tracker
		LOG("ERROR BULK TRANSFER requestBulkTransfer(S %u T %u) can't send request to ASL", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID);
		confirmBulkTransfer( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_FAILURE : BULKT_FAIL_OTHER );
		return false;
	}
	LOG("BULK TRANSFER %s C %3d block %d of %d ", GetHex(pLease->netAddr, 16), pLease->nContractId, it->m_unCurrentBlock, it->m_unBulkSize/it->m_unBlockSize+1);
	++it->m_unCurrentBlock;
	return true;
}

bool  CBulkTransferService::requestBulkClose(CGSAP::TGSAPHdr * p_pHdr, void * p_pData)
{
	TBulkCloseRequestData * pCloseReq=(TBulkCloseRequestData *)p_pData;
	if( p_pHdr->m_nDataSize != sizeof(TBulkCloseRequestData) + 4)
	{
		LOG("ERROR BULK CLOSE requestBulkClose(S %u T %u) buffer: %d != expected: %d", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->m_nDataSize, sizeof(TBulkCloseRequestData));
		confirmBulkClose( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_FAILURE : BULKO_FAIL_OTHER );
		return false;
	}
	pCloseReq->NTOH();
	
	std::list<RemUDOData>::iterator it;
	if( !findTransferByLease( it, pCloseReq->m_unLeaseID ) )
	{
		LOG("ERROR BULK CLOSE requestBulkClose(S %u T %u) Couldn't find transfer for lease id %u", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pCloseReq->m_unLeaseID);
		confirmBulkClose( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID,  p_pHdr->IsYGSAP() ? YGS_FAILURE : BULKC_FAIL );
		return false;
	}

	const lease *pLease=g_pLeaseMgr->FindLease( pCloseReq->m_unLeaseID, p_pHdr->m_nSessionID );
	if( (!pLease) || (LEASE_BULK_CLIENT != pLease->eLeaseType) || (!pLease->nContractId))
	{	char szExtra[64] = {0};
		if( pLease ) sprintf(szExtra, " Type %u Contract %u", pLease->eLeaseType, pLease->nContractId);
		LOG("ERROR BULK CLOSE requestBulkClose(S %u T %u) invalid/expired lease id %u%s", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pCloseReq->m_unLeaseID, szExtra );
		confirmBulkClose ( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_INVALID_LEASE : BULKC_FAIL);
		cleanupUDO(it);
		return false;
	}
	it->m_ucServiceType = BULK_TRANSFER_CLOSE;
	endDownload(it, it->m_unCurrentBlock >= it->m_unBulkSize/it->m_unBlockSize+1 ? UDO_END_DOWNLOAD_SUCCESS : UDO_END_DOWNLOAD_ABORT, p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Costin Cosoveanu
/// @brief (USER -> GW) Process a user request
/// @param p_pHdr		GSAP request header (has member m_nDataSize)
/// @param pData		GSAP request data (data size is in p_pHdr)
/// @retval TBD
/// @remarks CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
/// @note [marcel] DONE
////////////////////////////////////////////////////////////////////////////////
bool CBulkTransferService::ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData )
{
	switch( p_pHdr->m_ucServiceType )
	{
		case BULK_TRANSFER_OPEN:
			requestBulkOpen( p_pHdr, p_pData);
		return true;
		
		case BULK_TRANSFER_TRANSFER:
			requestBulkTransfer( p_pHdr, p_pData );
		return true;

		case BULK_TRANSFER_CLOSE:
			requestBulkClose( p_pHdr, p_pData );
		return true;
		
		default:	/// PROGRAMMER ERROR: We should never get to default
			LOG("ERROR CBulkTransferService::ProcessUserRequest: unknown/unimplemented service type %u", p_pHdr->m_ucServiceType );
			return false;/// Caller will SendConfirm with G_STATUS_UNIMPLEMENTED
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Mihai Buha
/// @brief Generate a READ REQ for the state attribute
/// @param it	the management structure for the current transfer
/// @retval true - success
/// @retval false - can't send to ASL
/// @remarks This function is only called on the error recovery flow. The user has already received his confirm(failure) so we don't pass any MSG parameter
////////////////////////////////////////////////////////////////////////////////
bool CBulkTransferService::askState( std::list<RemUDOData>::iterator& it )
{
	READ_REQ_SRVC stReadRec={
		m_unSrcOID: m_ushInterfaceOID,
		m_unDstOID: it->m_ushObjId,
		m_ucReqID: 0,
		m_stAttrIdtf: {m_ucAttrFormat: ATTR_NO_INDEX, m_unAttrID: UDO_STATE, 0, 0}
	};
	LOG("BULK askState (known state %s)", getBulkStateName(it->m_eState) );
	if( !(it->m_unAppHandle = g_stApp.m_oGwUAP.SendRequestToASL( &stReadRec, SRVC_READ_REQ, it->m_ushTSAPID, it->m_nContractId, 0, 0, it->m_ucServiceType )))
	{
		LOG("ERROR CBulkTransferService::askState can't send request to ASL");
		cleanupUDO(it);
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Mihai Buha
/// @brief Generate an EXEC REQ for the endDownload method
/// @param it			the management structure for the current transfer
/// @param p_nRationale		the input argument for the endDownload method. Can be either UDO_END_DOWNLOAD_SUCCESS or UDO_END_DOWNLOAD_ABORT
/// @param p_nSessionId
/// @param p_nTransactionId
/// @retval TBD
/// @remarks This function can be called in three basic use cases:
/// @remarks  1)by requestBulkClose() when the user initiates a BULK CLOSE request: p_nSessionId != 0, p_nTransactionId != 0
/// @remarks  2)by tryReset() outside of the user's control, via OnLeaseDelete() or the recovery procedure(askState): p_nSessionId = p_nTransactionId = 0
////////////////////////////////////////////////////////////////////////////////
bool CBulkTransferService::endDownload( std::list<RemUDOData>::iterator& it, uint8 p_nRationale, uint32 p_nSessionId = 0, uint32 p_nTransactionId = 0)
{
	EXEC_REQ_SRVC stExecReq = {
		m_unSrcOID:	m_ushInterfaceOID, 
		m_unDstOID:	it->m_ushObjId,
		m_ucReqID:	0,
		m_ucMethID:	UDO_END_DWLD,
		m_unLen:	1,
		p_pReqData:	&p_nRationale
	};
	if( !(it->m_unAppHandle = g_stApp.m_oGwUAP.SendRequestToASL( &stExecReq, SRVC_EXEC_REQ, it->m_ushTSAPID, it->m_nContractId, p_nSessionId, p_nTransactionId, it->m_ucServiceType, 0 )))
	{ /// Remove also from tracker
		LOG("ERROR CBulkTransferService::endDownload can't send request to ASL");
		cleanupUDO(it);
		return false;
	}
	advanceState( it, p_nRationale ? UDO_STATE_DL_ERROR : UDO_STATE_DL_COMPLETE);
	return true;
}

void CBulkTransferService::apply( MSG * p_pMSG, std::list<RemUDOData>::iterator& it )
{
	uint8 cCommand = BULK_APPLY; // 1=Apply (used for Download only)
	WRITE_REQ_SRVC stWriteRec={
		m_unSrcOID:	m_ushInterfaceOID,
		m_unDstOID:	it->m_ushObjId,
		m_ucReqID:	0,
		m_stAttrIdtf: { m_ucAttrFormat: ATTR_NO_INDEX, m_unAttrID: UDO_COMM, 0, 0 },
		m_unLen:	1,
		p_pReqData:	&cCommand
	};

	if(!(it->m_unAppHandle = g_stApp.m_oGwUAP.SendRequestToASL( &stWriteRec, SRVC_WRITE_REQ, it->m_ushTSAPID, it->m_nContractId, p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, it->m_ucServiceType, p_pMSG->m_unAppHandle)))
	{ /// Remove also from tracker
		LOG("ERROR BULK CLOSE apply(S %u T %u) can't send request to ASL", p_pMSG->m_nSessionID, p_pMSG->m_TransactionID);
		confirmBulkClose ( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID , g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKC_FAIL);
		cleanupUDO(it);
	}
	LOG("CBulkTransferService::apply sent.");
	advanceState( it, UDO_STATE_APPLYING);
}

////////////////////////////////////////////////////////////////////////////////
/// @author Costin Cosoveanu
/// @brief (ISA -> GW) Process an ADPU
/// @param p_unAppHandle - the request handle, used to find SessionID/TransactionID in tracker
/// @param p_pAPDUIdtf - needed for device TX time with usec resolution
/// @param p_pRsp The response from field, unpacked
/// @param p_pReq The original request
/// @retval false - field APDU not dispatched - default handler does not do processing, just log a message
/// @remarks This is the default handler, called if the derivated classess do not implement anything
/// (ISA -> GW) Dispatch an ADPU. Call from CInterfaceObjMgr::DispatchAPDU
////////////////////////////////////////////////////////////////////////////////
bool CBulkTransferService::ProcessAPDU(uint16 p_unAppHandle, APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq )
{
	MSG * pMSG = g_stApp.m_oGwUAP.m_tracker.FindMessage( p_unAppHandle, "CBulkTransferService::ProcessAPDU" );
	std::list<RemUDOData>::iterator it;
	if( !findTransferByAppHandle( it, p_unAppHandle ) )
	{
		/// this is an orphan packet (the transfer has been aborted)
		LOG("ERROR CBulkTransferService::ProcessAPDU(H %u S %u T %u) no active transfer!", p_unAppHandle, pMSG?pMSG->m_nSessionID:0, pMSG?pMSG->m_TransactionID:0);
		/// We should send a BULK CONFIRM packet here, but since we didn't find the transfer, we can't know which kind of CONFIRM to send.
		/// This isn't a problem, though, because we arrive here after either 1) a lease deletion has hijacked this transfer, so the client shouldn't expect a response anyway
		/// or 2) the user himself requested a bulk abort, so he has given up waiting for this CONFIRM
		g_stApp.m_oGwUAP.m_tracker.RemoveMessage( p_unAppHandle );
		return false;
	}

	LOG("CBulkTransferService::ProcessAPDU(H %u) current state %s", p_unAppHandle, getBulkStateName(it->m_eState) );
	switch(it->m_eState)
	{
	case UDO_STATE_IDLE:
		return processStateIdle( pMSG, it, p_pRsp, p_pReq );
		
	case UDO_STATE_DOWNLOADING:
		return processStateDownload( pMSG, it, p_pRsp, p_pReq );
		
	case UDO_STATE_DL_COMPLETE:
		return processStateDowldCompl( pMSG, it, p_pRsp, p_pReq );
		
	case UDO_STATE_APPLYING:
		return processStateApply( pMSG, it, p_pRsp, p_pReq );
		
	case UDO_STATE_DL_ERROR:
		return processStateDLError( pMSG, it, p_pRsp, p_pReq );

	default:
		LOG("PROGRAMMER ERROR CBulkTransferService::ProcessAPDU: Unsupported state %d", it->m_eState);
		g_stApp.m_oGwUAP.m_tracker.RemoveMessage( p_unAppHandle );
		cleanupUDO(it);
		break;
	}
	return true;
}

bool CBulkTransferService::ProcessISATimeout( uint16 p_unAppHandle, GENERIC_ASL_SRVC * p_pOriginalReq )
{
	LOG("CBulkTransferService::ProcessISATimeout(H %u)", p_unAppHandle );
	MSG * pMSG = g_stApp.m_oGwUAP.m_tracker.FindMessage( p_unAppHandle, "CBulkTransferService::ProcessISATimeout" );
	std::list<RemUDOData>::iterator it;
	if( !findTransferByAppHandle( it, p_unAppHandle ) )
	{
		/// this is an orphan packet (the transfer has been aborted)
		LOG("ERROR CBulkTransferService::ProcessISATimeout(H %u S %u T %u) no active transfer!", p_unAppHandle, pMSG?pMSG->m_nSessionID:0, pMSG?pMSG->m_TransactionID:0);
		/// We should send a BULK CONFIRM packet here, but since we didn't find the transfer, we can't know which kind of CONFIRM to send.
		/// This isn't a problem, though, because we arrive here after either 1) a lease deletion has hijacked this transfer, so the client shouldn't expect a response anyway
		/// or 2) the user himself requested a bulk abort, so he has given up waiting for this CONFIRM
		g_stApp.m_oGwUAP.m_tracker.RemoveMessage( p_unAppHandle );
		return false;
	}
	if(pMSG){
		switch(it->m_eState)
		{
		case UDO_STATE_IDLE:
			LOG("ERROR BULK OPEN (H %u S %u T %u L %u) TIMEOUT", p_unAppHandle, pMSG->m_nSessionID, pMSG->m_TransactionID, it->m_nLeaseID);
			confirmBulkOpen( pMSG->m_nSessionID, pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(pMSG->m_nSessionID) ? YGS_COMM_FAILED : BULKO_FAIL_OTHER, it->m_unBlockSize, it->m_unBulkSize );
			break;
		case UDO_STATE_DOWNLOADING:
			LOG("ERROR BULK TRANSFER (H %u S %u T %u L %u) TIMEOUT", p_unAppHandle, pMSG->m_nSessionID, pMSG->m_TransactionID, it->m_nLeaseID);
			confirmBulkTransfer( pMSG->m_nSessionID, pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(pMSG->m_nSessionID) ? YGS_COMM_FAILED : BULKT_FAIL_COMM );
			break;
		case UDO_STATE_DL_COMPLETE:
		case UDO_STATE_APPLYING:
		case UDO_STATE_DL_ERROR:
			LOG("ERROR BULK CLOSE (H %u S %u T %u L %u) TIMEOUT", p_unAppHandle, pMSG->m_nSessionID, pMSG->m_TransactionID, it->m_nLeaseID);
			confirmBulkClose( pMSG->m_nSessionID, pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(pMSG->m_nSessionID) ? YGS_COMM_FAILED : BULKC_FAIL );
			break;
		default:
			LOG("PROGRAMMER ERROR CBulkTransferService::ProcessISATimeout(H %u): Unsupported state %d", p_unAppHandle, it->m_eState);
			g_stApp.m_oGwUAP.m_tracker.RemoveMessage( p_unAppHandle );
		}
	}
	cleanupUDO(it);
	return true;
}

bool CBulkTransferService::processStateIdle( MSG * p_pMSG, std::list<RemUDOData>::iterator& it , GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq )
{
	switch(p_pRsp->m_ucType)
	{
	case SRVC_EXEC_RSP://this is the answer to setFile, startDownload
		//this happens only during the BulkOpen flow. Therefore p_pMSG is valid.
		if(!p_pMSG){
			LOG("PROGRAMMER ERROR: CBulkTransferService::processStateIdle: no tracked message for processing setFile, startDownload!");
			cleanupUDO(it);
			break;
		}
		if(p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC != SFC_SUCCESS)
		{
			//report failure here and try to reset by asking the state first
			LOG("ERROR BULK OPEN processStateIdle(S %u T %u L%u) exec MethID=%d, SFC=%d", p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, it->m_nLeaseID,
				p_pReq->m_stSRVC.m_stExecReq.m_ucMethID, p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC );
			confirmBulkOpen( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKO_FAIL_OTHER, it->m_unBlockSize, it->m_unBulkSize );
			askState( it );
		}else{
			switch( p_pReq->m_stSRVC.m_stExecReq.m_ucMethID )
			{
			case UDO_SET_FILE:
				if(p_pRsp->m_stSRVC.m_stExecRsp.p_pRspData[0] == 0)
				{
					return startDownload( p_pMSG, it );
				}else{
					LOG("ERROR BULK OPEN processStateIdle(S %u T %u L%u) exec MethID=UDO_SET_FILE, SFC=SUCCESS, response=%d",
						p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, it->m_nLeaseID, p_pRsp->m_stSRVC.m_stExecRsp.p_pRspData[0] );
					confirmBulkOpen( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE :BULKO_FAIL_OTHER, it->m_unBlockSize, it->m_unBulkSize );
					cleanupUDO(it);
				}
				break;
			case UDO_START_DWLD:
				confirmBulkOpen( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_SUCCESS : BULKO_SUCCESS, it->m_unBlockSize, it->m_unBulkSize );
				advanceState( it, UDO_STATE_DOWNLOADING);
				break;
			default:
				LOG("ERROR CBulkTransferService::processStateIdle: received exec response for unknown method %d", p_pReq->m_stSRVC.m_stExecReq.m_ucMethID);
				/// TODO: switch by it->m_ucServiceType confirmBulkOpen/confirmBulkTransfer/confirmBulkClose
				confirmBulkOpen( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE :BULKO_FAIL_OTHER, it->m_unBlockSize, it->m_unBulkSize );
				cleanupUDO(it);
			}
		}
		break;
	case SRVC_READ_RSP: //this is the answer to readMaxBlockSize, readMaxDownloadSize, askState
		if(p_pRsp->m_stSRVC.m_stReadRsp.m_ucSFC != SFC_SUCCESS)
		{
			// Can't have an read rsp error sfc unless we really are not talking to an UDO.
			// Therefore, we couldn't have departed the Idle state/BULK OPEN phase. And there is no point in reseting.
			LOG("ERROR BULK OPEN processStateIdle(S %u T %u L%u) read attrID=%d, SFC=%d", p_pMSG?p_pMSG->m_nSessionID:0, p_pMSG?p_pMSG->m_TransactionID:0, it->m_nLeaseID,
				p_pReq->m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unAttrID, p_pRsp->m_stSRVC.m_stReadRsp.m_ucSFC );
			if(p_pMSG){
				confirmBulkOpen( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKO_FAIL_OTHER, it->m_unBlockSize, it->m_unBulkSize );
			}
			cleanupUDO(it);
		}else{	
			switch( p_pReq->m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unAttrID )
			{
			case UDO_MAX_BLCK_SIZE:
				{
					//this happens only during the BulkOpen flow. Therefore p_pMSG is valid.
					if(!p_pMSG){
						LOG("PROGRAMMER ERROR: CBulkTransferService::processStateIdle: no tracked message for processing readMaxBlockSize!");
						cleanupUDO(it);
						break;
					}
					/// m_pRspData is aligned (GENERIC_ASL_SRVC is not packed), so it should work OK on ARM
					uint16 rspData = ntohs(*(uint16*)p_pRsp->m_stSRVC.m_stReadRsp.m_pRspData);
					if( !it->m_unBlockSize || rspData < it->m_unBlockSize )
					{
						it->m_unBlockSize = rspData;
					}
				}
				return readMaxDownloadSize( p_pMSG, it);
			case UDO_MAX_DWLD_SIZE:
				{
					//this happens only during the BulkOpen flow. Therefore p_pMSG is valid.
					if(!p_pMSG){
						LOG("PROGRAMMER ERROR: CBulkTransferService::processStateIdle: no tracked message for processing readMaxDownloadSize!");
						cleanupUDO(it);
						break;
					}
					/// m_pRspData is aligned (GENERIC_ASL_SRVC is not packed), so it should work OK on ARM
					uint32 rspData = ntohl(*(uint32*)p_pRsp->m_stSRVC.m_stReadRsp.m_pRspData);
					if( rspData < it->m_unBulkSize )
					{	/// Max download size cannot be accepted by destination
						LOG("ERROR BULK OPEN processStateIdle(S %u T %u L%u) read AttrID=UDO_MAX_DWLD_SIZE, SFC=SUCCESS, response=%d, requested=%d",
							p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, it->m_nLeaseID, rspData, it->m_unBulkSize );
						confirmBulkOpen( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_LIMIT_EXCEEDED : BULKO_FAIL_EXCEED_LIM, it->m_unBlockSize, it->m_unBulkSize );
						cleanupUDO(it);
						return true;
					}
				}
				return it->m_unResDescSize ? setFile( p_pMSG, it) : startDownload( p_pMSG, it );
			case UDO_STATE:
				if(UDO_STATE_IDLE == p_pRsp->m_stSRVC.m_stReadRsp.m_pRspData[0])
				{	
					LOG("WARN: CBulkTransferService::processStateIdle remote UDO already in IDLE state, can't reset");
					cleanupUDO(it);
				}else{
					LOG("WARN: CBulkTransferService::processStateIdle: server state %d", p_pRsp->m_stSRVC.m_stReadRsp.m_pRspData[0]);
					advanceState( it, (E_UDO_STATE)p_pRsp->m_stSRVC.m_stReadRsp.m_pRspData[0]);
					tryReset(it);
				}
				break;
			default:
				LOG("ERROR CBulkTransferService::processStateIdle: received read response for unknown attribute %d", p_pReq->m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unAttrID);
				if(p_pMSG){
					confirmBulkOpen( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKO_FAIL_OTHER, it->m_unBlockSize, it->m_unBulkSize );
				}
				cleanupUDO(it);
			}
		}
		break;
	case SRVC_WRITE_RSP://this is the answer to sendReset, but normal flow shouldn't arrive here
		LOG("WARNING CBulkTransferService::processStateIdle: reset done SFC=%d", p_pRsp->m_stSRVC.m_stWriteRsp.m_ucSFC );
		if(p_pMSG){
			/// TODO: switch by it->m_ucServiceType confirmBulkOpen/confirmBulkTransfer/confirmBulkClose
			confirmBulkOpen( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKO_FAIL_OTHER, it->m_unBlockSize, it->m_unBulkSize );
		}
		cleanupUDO(it);
		break;
	default:
		LOG("ERROR CBulkTransferService::processStateIdle: Unknown packet type=%02X", p_pRsp->m_ucType);
		if(p_pMSG){
			confirmBulkOpen( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKO_FAIL_OTHER, it->m_unBlockSize, it->m_unBulkSize );
		}
		cleanupUDO(it);
		return false;
	}
	return true;
}

bool CBulkTransferService::processStateDownload( MSG * p_pMSG, std::list<RemUDOData>::iterator& it, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq )
{
	switch(p_pRsp->m_ucType)
	{
	case SRVC_EXEC_RSP://this is the answer to DownloadData
		//this happens only during the BulkTransfer flow. Therefore p_pMSG is valid.
		if(!p_pMSG){
			LOG("PROGRAMMER ERROR: CBulkTransferService::processStateDownload: no tracked message for processing DownloadData!");
			cleanupUDO(it);
			break;
		}
		if( p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC == SFC_INVALID_BLOCK_NUMBER  )
		{	/// INVALID
			LOG("ERROR BULK TRANSFER (S %u T %u) seq INVALID, SFC=%u", p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC );
			confirmBulkTransfer( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_COMM_FAILED : BULKT_FAIL_COMM );
		}else if( p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC == SFC_DATA_SEQUENCE_ERROR )
		{	/// [marcel] DUPLICATE, CONTINUE, there is no reason to reset...
			LOG("WARN BULK TRANSFER (S %u T %u) seq DUPLICATE, SFC=%u. Try to continue", p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC );
			confirmBulkTransfer( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_SUCCESS : BULKT_SUCCESS );
		}else if(p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC != SFC_SUCCESS )
		{
			//report failure here and try to reset by asking the state first
			LOG("ERROR BULK TRANSFER (S %u T %u) unknown failure, SFC=%u", p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC );
			confirmBulkTransfer( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_ABORTED : BULKT_FAIL_ABORT );
			askState( it );
		}else
		{
			confirmBulkTransfer( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_SUCCESS : BULKT_SUCCESS );
		}
		break;
	case SRVC_READ_RSP://this is the answer to askState
		if(p_pRsp->m_stSRVC.m_stReadRsp.m_ucSFC != SFC_SUCCESS)
		{
			LOG("ERROR CBulkTransferService::processStateDownload: SFC=%d read failure", p_pRsp->m_stSRVC.m_stReadRsp.m_ucSFC );
		}
		else
		{
			advanceState( it, (E_UDO_STATE)*p_pRsp->m_stSRVC.m_stReadRsp.m_pRspData);
		}
		return tryReset(it);
	default:
		LOG("ERROR CBulkTransferService::processStateDownload: Not implemented state %s, servicetype=%X", getBulkStateName(it->m_eState), p_pRsp->m_ucType);
		if(p_pMSG){
			confirmBulkTransfer( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_ABORTED : BULKT_FAIL_ABORT );
		}
		cleanupUDO(it);
		return false;
	}
	return true;
}

bool CBulkTransferService::processStateDowldCompl( MSG * p_pMSG, std::list<RemUDOData>::iterator& it, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq )
{
	switch(p_pRsp->m_ucType)
	{
	case SRVC_EXEC_RSP://this is the answer to EndDownload
		//this happens only during the BulkClose flow. Therefore p_pMSG is valid.
		if(!p_pMSG){
			LOG("PROGRAMMER ERROR: CBulkTransferService::processStateDowldCompl: no tracked message for processing EndDownload!");
			cleanupUDO(it);
			break;
		}
		if(p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC != SFC_SUCCESS)
		{
			//report failure here and try to reset by asking the state first
			LOG("ERROR CBulkTransferService::processStateDowldCompl failure at method=%d SFC=%d",
				p_pReq->m_stSRVC.m_stExecReq.m_ucMethID, p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC );
			confirmBulkClose( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID , g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKC_FAIL );
			askState( it );
		}else{
			apply( p_pMSG, it );
		}
		break;
	case SRVC_READ_RSP://this is the answer to askState
		if(p_pRsp->m_stSRVC.m_stReadRsp.m_ucSFC != SFC_SUCCESS)
		{
			LOG("ERROR CBulkTransferService::processStateDowldCompl: SFC=%d read failure", p_pRsp->m_stSRVC.m_stReadRsp.m_ucSFC );
		}
		else
		{
			advanceState( it, (E_UDO_STATE)*p_pRsp->m_stSRVC.m_stReadRsp.m_pRspData);
		}
		return tryReset(it);
	case SRVC_WRITE_RSP:
	default:
		LOG("ERROR CBulkTransferService::processStateDowldCompl: Not implemented state %s,servicetype=%X", getBulkStateName(it->m_eState), p_pRsp->m_ucType);
		if(p_pMSG){
			confirmBulkClose( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID , g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKC_FAIL );
		}
		askState( it );
	}
	return true;
}

bool CBulkTransferService::processStateApply( MSG * p_pMSG, std::list<RemUDOData>::iterator& it, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq )
{
	switch(p_pRsp->m_ucType)
	{
	case SRVC_WRITE_RSP://this is the answer to apply
		//this happens only during the BulkClose flow. Therefore p_pMSG is valid.
		if(!p_pMSG){
			LOG("PROGRAMMER ERROR: CBulkTransferService::processStateApply: no tracked message for processing apply!");
			cleanupUDO(it);
			break;
		}
		if(p_pRsp->m_stSRVC.m_stWriteRsp.m_ucSFC != SFC_OPERATION_ACCEPTED)
		{
			//report failure here and try to reset by asking the state first
			LOG("ERROR CBulkTransferService::processStateApply: SFC=%d write failure at attribute=%d value=%d",
				p_pRsp->m_stSRVC.m_stWriteRsp.m_ucSFC, p_pReq->m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_unAttrID, p_pReq->m_stSRVC.m_stWriteReq.p_pReqData[0] );
			confirmBulkClose( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKC_FAIL );
			askState( it );
		}
		else
		{
			double fElapsed = it->m_uSec.GetElapsedSec();
			unsigned nElapsed = (unsigned)fElapsed;
			if( !nElapsed )
				nElapsed = 99999;
			LOG("BULK processStateApply (Transfer %u L %lu C %u) OK %u bytes in %.3f seconds, %u b/s", it->m_nTransferID,
				it->m_nLeaseID, it->m_nContractId, it->m_unBulkSize, fElapsed, it->m_unBulkSize/nElapsed );
			confirmBulkClose( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_SUCCESS : BULKC_SUCCESS );
			cleanupUDO(it);
		}
		break;
	case SRVC_READ_RSP://this is the answer to askState
		if(p_pRsp->m_stSRVC.m_stReadRsp.m_ucSFC != SFC_SUCCESS)
		{
			LOG("ERROR CBulkTransferService::processStateApply: SFC=%d read failure", p_pRsp->m_stSRVC.m_stReadRsp.m_ucSFC );
		}
		else
		{
			advanceState(it, (E_UDO_STATE)*p_pRsp->m_stSRVC.m_stReadRsp.m_pRspData);
		}
		return tryReset(it);
	case SRVC_EXEC_RSP:
	default:
		LOG("ERROR CBulkTransferService::processStateApply: Not implemented state %s, servicetype=%X", getBulkStateName(it->m_eState), p_pRsp->m_ucType );
		if(p_pMSG){
			confirmBulkClose( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKC_FAIL );
		}
		askState( it );
	}
	return true;
}

bool CBulkTransferService::processStateDLError( MSG * p_pMSG, std::list<RemUDOData>::iterator& it, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq )
{
	// If we are in recovery, p_pMSG == NULL
	// If we are in user abort, p_pMSG != NULL
	switch(p_pRsp->m_ucType)
	{
	case SRVC_WRITE_RSP://this is the answer to sendReset
		if( p_pRsp->m_stSRVC.m_stWriteRsp.m_ucSFC != SFC_SUCCESS )
		{
			LOG("ERROR CBulkTransferService::processStateDLError: write failure at attribute=%d value=%d SFC=%d",
				p_pReq->m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_unAttrID, p_pReq->m_stSRVC.m_stWriteReq.p_pReqData[0], p_pRsp->m_stSRVC.m_stWriteRsp.m_ucSFC);
			if(p_pMSG)
			{
				confirmBulkClose ( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKC_FAIL);
				askState( it );
			}else{
				LOG("WARNING Unable to recover from failure. Giving up");
				cleanupUDO(it);
			}
		}else{
			LOG("BULK: Reset complete for remote UDO - lease id %u", it->m_nLeaseID);
			if(p_pMSG)
			{
				confirmBulkClose ( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_ABORTED : BULKC_SUCCESS);
			}
			cleanupUDO(it);
		}
		break;
	case SRVC_READ_RSP://this is the answer to askState
		advanceState( it, (E_UDO_STATE)*p_pRsp->m_stSRVC.m_stReadRsp.m_pRspData );
		return tryReset(it);
	case SRVC_EXEC_RSP://this is the answer to endDownload(UDO_END_DOWNLOAD_ABORT)
		if( p_pRsp->m_stSRVC.m_stWriteRsp.m_ucSFC != SFC_SUCCESS )
		{
			LOG("ERROR CBulkTransferService::processStateDLError: failure executing method=%d SFC=%d",
				p_pReq->m_stSRVC.m_stExecReq.m_ucMethID, p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC);
			if(p_pMSG)
			{
				confirmBulkClose ( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKC_FAIL);
				askState( it );
			}else{
				LOG("WARNING Unable to recover from failure. Giving up");
				cleanupUDO(it);
			}
		}else{
			LOG("BULK: Resetting remote UDO - lease id %u", it->m_nLeaseID);
			sendReset( p_pMSG, it );
		}
		break;
	default:
		LOG("PROGRAMMER ERROR CBulkTransferService::processStateDLError: Not implemented state=%d,servicetype=%X", it->m_eState, p_pRsp->m_ucType );
		if(p_pMSG)
		{
			confirmBulkClose ( p_pMSG->m_nSessionID, p_pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKC_FAIL);
		}
		cleanupUDO(it);
	}
	return true;
}

/// Process a lease deletion: free resources
void CBulkTransferService::OnLeaseDelete( lease* p_pLease )
{
	unsigned nCount = 0;
	for( std::list<RemUDOData>::iterator it = RemUDOList.begin(); it != RemUDOList.end(); )
	{
		if (it->m_nLeaseID==p_pLease->nLeaseId)
		{	
			LOG(" BULK OnLeaseDelete(%lu) transfer in state %s", p_pLease->nLeaseId, getBulkStateName(it->m_eState) );
			std::list<RemUDOData>::iterator itTmp = it;
			++itTmp;
			tryReset(it);	/// invalidates it
			it = itTmp;
			++nCount;
		}
		else
		{
			++it;
		}
	}
	if( nCount)
		LOG(" BULK OnLeaseDelete(%lu) sent %u resets", p_pLease->nLeaseId, nCount);
}

bool CBulkTransferService::sendReset( MSG * p_pMSG, std::list<RemUDOData>::iterator& it )
{
	uint32 nSessionID=0, TransactionID=0;
	if(p_pMSG)
	{
		nSessionID = p_pMSG->m_nSessionID;
		TransactionID = p_pMSG->m_TransactionID;
	}
	uint8 cCommand = BULK_RESET;
	WRITE_REQ_SRVC stWriteRec={
		m_unSrcOID:	m_ushInterfaceOID,
		m_unDstOID:	it->m_ushObjId,
		m_ucReqID:	0,
		m_stAttrIdtf: { m_ucAttrFormat: ATTR_NO_INDEX, m_unAttrID: UDO_COMM, 0, 0 },
		m_unLen:	1,
		p_pReqData:	&cCommand
	};
	if(!(it->m_unAppHandle = g_stApp.m_oGwUAP.SendRequestToASL( &stWriteRec, SRVC_WRITE_REQ, it->m_ushTSAPID, it->m_nContractId, nSessionID, TransactionID, it->m_ucServiceType, p_pMSG?p_pMSG->m_unAppHandle:0 )))
	{
		if(p_pMSG)
		{
			LOG("ERROR BULK CLOSE sendReset(S %u T %u) can't send request to ASL", nSessionID, TransactionID);
			confirmBulkClose( nSessionID, TransactionID, g_pSessionMgr->IsYGSAPSession(p_pMSG->m_nSessionID) ? YGS_FAILURE : BULKC_FAIL );
		}else{
			LOG("ERROR CBulkTransferService::sendReset can't send request to ASL");
			/// nSessionID & TransactionID have invalid values, cannot send confirm
		}
		cleanupUDO(it);
		return false;
	}
	return true;
}

///TAKE CARE: invalidates it!!!
/// TAKE CARE: an eventual BULK CLOSE from client will fail because we delete the transfer (Couldn't find transfer for lease id)
void CBulkTransferService::cleanupUDO(std::list<RemUDOData>::iterator& it)
{
	LOG("BULK cleanupUDO(Transfer %u L %lu C %u): %.3f seconds", it->m_nTransferID,
		it->m_nLeaseID, it->m_nContractId, it->m_uSec.GetElapsedSec() );
	if(it->m_pucResDesc)
		delete[] it->m_pucResDesc;
	RemUDOList.erase(it);
}

bool CBulkTransferService::tryReset( std::list<RemUDOData>::iterator& it)
{
	LOG("BULK: tryReset, state %s", getBulkStateName(it->m_eState) );
	switch(it->m_eState){
	case UDO_STATE_DOWNLOADING:
		return endDownload(it, UDO_END_DOWNLOAD_ABORT);
	case UDO_STATE_DL_COMPLETE:
	case UDO_STATE_DL_ERROR:
		return sendReset(NULL, it);
	case UDO_STATE_IDLE:
	case UDO_STATE_APPLYING:
	default:
		LOG("WARN: CBulkTransferService::tryReset state %s -> nothing to do!", getBulkStateName(it->m_eState) );
		cleanupUDO(it);
	}
	return false;
}

bool CBulkTransferService::confirmBulkOpen(/*uint8 p_nServiceType,*/ unsigned p_nSessionID, unsigned p_nTransactionID, uint8 p_ucStatus, uint32 p_nBlockSize, uint32 p_nItemSize )
{
	CGSAP* pGSAP = g_stApp.m_oGwUAP.FindGSAP( p_nSessionID );
	if(pGSAP)
	{
/* TODO MARK TX time on all services
		TODO: HOW TO GET nLeaseId ?? probaly the lease id should also be stored on tracker
		TODO: ADD lease id in tracker
		const lease* pLease = g_pLeaseMgr->FindLease( nLeaseId, p_pHdr->m_nSessionID );
		if( pLease )
			//	p_pLease->m_oLastTx.MarkStartTime();	///< Mark in lease Last TX as now()
*/
		TBulkOpenConfirmData stConfirm = { m_ucStatus:p_ucStatus ,m_ushBlockSize:p_nBlockSize, m_unItemSize:p_nItemSize, };
		stConfirm.HTON();
		pGSAP->SendConfirm( p_nSessionID, p_nTransactionID, BULK_TRANSFER_OPEN, (byte*)&stConfirm, sizeof(stConfirm) );
	}
	else 
		LOG("WARN confirmBulkOpen(S %u T %u B %u I %u Stat %u): invalid session, CONFIRM not sent", p_nSessionID, p_nTransactionID, p_nBlockSize, p_nItemSize, p_ucStatus);
			
	return true;
}


bool CBulkTransferService::confirmBulkTransfer(/*uint8 p_nServiceType,*/  unsigned  p_nSessionID, unsigned p_nTransactionID,  uint8 p_ucStatus )
{
	CGSAP* pGSAP = g_stApp.m_oGwUAP.FindGSAP( p_nSessionID );
	
	if(pGSAP)
	{
/* TODO MARK TX time on all services
		TODO: HOW TO GET nLeaseId ?? probaly the lease id should also be stored on tracker
		TODO: ADD lease id in tracker
		const lease* pLease = g_pLeaseMgr->FindLease( nLeaseId, p_pHdr->m_nSessionID );
		if( pLease )
			//	p_pLease->m_oLastTx.MarkStartTime();	///< Mark in lease Last TX as now()
*/
		pGSAP->SendConfirm( p_nSessionID, p_nTransactionID, BULK_TRANSFER_TRANSFER , (byte*)&p_ucStatus , sizeof(p_ucStatus) );
	}
	else 
		LOG("WARN confirmBulkTransfer(S %u T %u Stat %u): invalid session, CONFIRM not sent", p_nSessionID, p_nTransactionID, p_ucStatus);

	return true;
}

bool CBulkTransferService::confirmBulkClose(/*uint8 p_nServiceType,*/ unsigned  p_nSessionID, unsigned p_nTransactionID, uint8 p_ucStatus )
{
	CGSAP* pGSAP = g_stApp.m_oGwUAP.FindGSAP( p_nSessionID );
	
	if(pGSAP)
	{
/* TODO MARK TX time on all services
		TODO: HOW TO GET nLeaseId ?? probaly the lease id should also be stored on tracker
		TODO: ADD lease id in tracker
		const lease* pLease = g_pLeaseMgr->FindLease( nLeaseId, p_pHdr->m_nSessionID );
		if( pLease )
			//	p_pLease->m_oLastTx.MarkStartTime();	///< Mark in lease Last TX as now()
*/
		pGSAP->SendConfirm( p_nSessionID, p_nTransactionID, BULK_TRANSFER_CLOSE , (byte*)&p_ucStatus , sizeof(p_ucStatus) );
	}
	else 
		LOG("WARN confirmBulkClose(S %u T %u Stat %u): invalid session, CONFIRM not sent", p_nSessionID, p_nTransactionID, p_ucStatus);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Check if this worker can handle service type received as parameter
/// @param p_ucServiceType - service type tested
/// @retval true - this service can handle p_ucServiceType
/// @retval false - the service cannot handle p_ucServiceType
/// @remarks The method is called from CInterfaceObjMgr::DispatchUserRequest
/// to decide where to dispatch a user request
////////////////////////////////////////////////////////////////////////////////
bool CBulkTransferService::CanHandleServiceType( uint8 p_ucServiceType ) const
{	switch(p_ucServiceType)
	{ 	case BULK_TRANSFER_OPEN:
		case BULK_TRANSFER_TRANSFER:
		case BULK_TRANSFER_CLOSE:
			return true;
	}
	return false;
};

void CBulkTransferService::Dump(void)
{
	LOG("%s: OID %u", Name(), m_ushInterfaceOID);
	LOG(" %d bulk transfers in progress", RemUDOList.size() );

	for (std::list<RemUDOData>::iterator it=RemUDOList.begin();it!=RemUDOList.end();++it)
	{
		LOG("   Bulk Transfer id %u H %u L %lu C %u BulkSz %u BlockSz %u State %s CrtBlock %u",
			it->m_nTransferID, it->m_unAppHandle, it->m_nLeaseID, it->m_nContractId,
			it->m_unBulkSize, it->m_unBlockSize, getBulkStateName(it->m_eState), it->m_unCurrentBlock );
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Return a static string with the bulk state name
/// @param p_eState the state number 
/// @return state name, or state number + unknown if state is not known
////////////////////////////////////////////////////////////////////////////////
const char * CBulkTransferService::getBulkStateName( E_UDO_STATE p_eState )
{	
	static const char * sStateName[] = {
	"IDLE", 		//UDO_STATE_IDLE = 		0,
	"DOWNLOADING",	//UDO_STATE_DOWNLOADING,1
	"UPLOADING",	//UDO_STATE_UPLOADING,	2
	"APPLYING",		//UDO_STATE_APPLYING,	3
	"DL_COMPLETE",	//UDO_STATE_DL_COMPLETE,4
	"UL_COMPLETE",	//UDO_STATE_UL_COMPLETE,5
	"DL_ERROR",		//UDO_STATE_DL_ERROR, 	6
	"UL_ERROR"		// UDO_STATE_UL_ERROR 	7
	};
	static char szErrRet[ 32 ] = {0};
	
	uint8 u8MaxIdx = (sizeof(sStateName) / sizeof(sStateName[0]));

	if( p_eState >= u8MaxIdx )
	{
		snprintf(szErrRet, sizeof(szErrRet), "%u: UNKNOWN", p_eState);
		szErrRet[ sizeof(szErrRet) - 1 ] = 0;
		return szErrRet;
	}

	return sStateName[ p_eState ];
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Advance transfer state machine to state p_eNewState
/// @param p_it iterator identifying the transfer
/// @param p_eNewState new state
/// @todo THIS SHOULD BE MEMBER OF RemUDOData,  WHILE RemUDOData::m_eState SHOULD BE PRIVATE
////////////////////////////////////////////////////////////////////////////////
void CBulkTransferService::advanceState( std::list<RemUDOData>::iterator p_it, E_UDO_STATE p_eNewState )
{
	LOG("BULK advanceState(Transfer %u L %lu C %u) state %u -> %u %s", p_it->m_nTransferID, p_it->m_nLeaseID,
		p_it->m_nContractId, p_it->m_eState, p_eNewState, getBulkStateName( p_eNewState ) );

	p_it->m_eState = p_eNewState; /// Do the actual state advance
}

