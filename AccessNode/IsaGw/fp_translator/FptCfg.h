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

#ifndef _FPT_CONFIG_H_
#define _FPT_CONFIG_H_


#include "Shared/Config.h"
#include "Shared/h.h"


//////////////////////////////////////////////////////////////////////////////
/// @class	CFptCfg
/// @ingroup	foreign protocol translator
/// @brief	foreign protocol translator config
//////////////////////////////////////////////////////////////////////////////
class CFptCfg : CConfig
{
public:
	CFptCfg();
	virtual ~CFptCfg();
public:
	int Init();
	bool		UseRawLog()	const { return m_useRawLog; }



public:

	int		m_TtyBauds ;		///< Serial Link Bauds
	char	m_TtyDev[256] ;		///< Serial Link Device
	bool	m_useRawLog ;		///< To use or not to use RawLog.	

};


#endif // _FPT_APP_CONFIG_H_
