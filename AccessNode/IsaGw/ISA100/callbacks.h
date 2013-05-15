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

#ifndef callbacks_h__
#define callbacks_h__

#include "porting.h"


typedef int (* PfnCallbackSendToDL )( IPv6Packet* p_pIPv6, int p_nLen );
typedef int (* PfnCallbackSendAlertToTR ) ( const uint8_t* p_pAlertHeader, int p_nHeaderLen, const uint8_t* p_pData, int p_nDataLen);

/// Register the callback function
void Callback_SetFunc (PfnCallbackSendToDL p_pCallbackSendToDL);
void Callback_SetFunc (PfnCallbackSendAlertToTR p_pCallbackSendAlertTR);




extern PfnCallbackSendToDL			g_pCallbackSendToDL;
extern PfnCallbackSendAlertToTR		g_pCallbackSendAlertTR;

#endif // callbacks_h__
