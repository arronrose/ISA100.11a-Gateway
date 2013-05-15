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

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Eduard Erdei
/// Date:         January 2008
/// Description:  This file holds definitions of the application sub-layer data entity
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _NIVIS_ASLSRVC_H_
#define _NIVIS_ASLSRVC_H_

#include "aslde.h"
#include "nlme.h"
#include "typedef.h"

#define GET_NEW_REQ_ID g_ucReqID++


extern const uint8 * ASLSRVC_GetGenericObject(  const uint8 *        p_pInBuffer, 
                                                uint16               p_nInBufLen, 
                                                GENERIC_ASL_SRVC *   p_pGenericObj,
                                                const uint8 *       p_pSrcAddr128 );

uint8 ASLSRVC_AddGenericObject( void *        p_pObj, 
                                uint8         p_ucServiceType,
                                uint8         p_ucPriority,
                                uint8         p_ucSrcTSAPID,
                                uint8         p_ucDstTSAPID,
//                                uint8         p_ucMaxRetries,
//                                uint16        p_unRetryTimeout,
                                uint16        p_unAppHandler,
                                const uint8 * p_pDstAddr, 
                                uint16        p_unContractID,
                                uint16        p_unBinSize,
                                uint8         p_ucObeyContractBandwidth,
								uint8			p_ucNoRspExpected = 0,
								uint16			p_unAPDULifetime = 300);

uint8 ASLSRVC_AddGenericObjectPlain( void *        p_pObj, 
                                uint8         p_ucServiceType,
                                uint8         p_ucPriority,
                                uint8         p_ucSrcTSAPID,
                                uint8         p_ucDstTSAPID,
//                                uint8         p_ucMaxRetries,
//                                uint16        p_unRetryTimeout,
                                uint16        p_unAppHandle,
                                const uint8 * p_pDstAddr, 
                                uint16        p_unContractID,
                                uint16        p_unBinSize,
                                uint8         p_ucObeyContractBandwidth,
								uint8			p_ucNoRspExpected = 0,
								uint16			p_unAPDULifetime = 300);

uint8 ASLSRVC_AddGenericObjectEncryptableCore( void *        p_pObj,
             uint8         p_ucServiceType,
             uint8         p_ucPriority,
             uint8         p_ucSrcTSAPID,
             uint8         p_ucDstTSAPID,
             uint16        p_unAppHandler,
             const uint8 * p_pDstAddr,
             uint16        p_unContractID,
             uint16        p_unBinSize,
             uint8         p_ucObeyContractBandwidth,
             uint8         p_ucAllowEncryption,
             uint8         p_ucNoRspExpected,
             uint16        p_unAPDULifetime,
             CNlmeContractPtr p_pContract = CNlmeContractPtr());

uint8 * ASLSRVC_SetGenericObj(uint8 p_ucServiceType, void * p_pObj, uint16 p_unOutBufLen, uint8 * p_pBuf);

extern uint8 ASLSRVC_GetLastReqID( void );
                                      

#endif // _NIVIS_ASLSRVC_H_
