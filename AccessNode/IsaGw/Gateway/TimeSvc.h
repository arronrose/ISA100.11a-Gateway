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
/// @file TimeSvc.h
/// @author Marcel Ionescu
/// @brief Time service - interface
////////////////////////////////////////////////////////////////////////////////
#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include "../ISA100/porting.h"
#include "Service.h"
#include "SAPStruct.h"

/// TimeStatus
#define TIME_SUCCESS	 	0	/// Time returned ok
#define TIME_NOT_ALLOWED	1	/// Not allowed to set time
#define TIME_FAIL_OTHER		2	/// Failure; other

/// RequestDataSize - const
#define TIME_REQ_DATA_SIZE	11	/// Command, Time, DataCrc 
#define TIME_FAIL_RES_SIZE	7	/// Status(1), Time(6)
  
////////////////////////////////////////////////////////////////////////////////
/// @class CTimeService
/// @brief Time Service
/// @see Service.h for details
////////////////////////////////////////////////////////////////////////////////
class CTimeService: public CService{
public:
	CTimeService( const char * p_szName ) :CService(p_szName){};

	/// (USER -> GW) Process a user request
	/// CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
	virtual bool ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData );
	
	/// Return true if the service is able to handle the service type received as parameter
	/// Used by (USER -> GW) flow
	virtual bool CanHandleServiceType( uint8 p_ucServiceType ) const;

	private:
		bool confirm( CGSAP::TGSAPHdr * p_pHdr, uint8_t p_ucStatus, const SAPStruct::TAI* p_pTAI = NULL );

};

#endif //TIME_SERVICE_H
