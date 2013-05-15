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

#include "FptCfg.h"
#include <ctype.h>
#include <netinet/in.h>
#include <termios.h>

#include "../Shared/Utils.h"

CFptCfg::CFptCfg()
{
	//memset( this, 0, sizeof(*this) ); // not virtual functions present so we can do in that way ...
	
}

CFptCfg::~CFptCfg()
{
	;
}


#define READ_BUF_SIZE 256

int CFptCfg::Init()
{
	/// @todo check for potential conflicts between  this CIniParser::Load
	/// and the CIniParser::Load which occurs in CConfig::Init
	if(!CConfig::Init("fp_translator", INI_FILE))
	{
		return 0;
	}
	
	READ_DEFAULT_VARIABLE_YES_NO( "RAW_LOG", m_useRawLog, "NO" );


	LOG("Config loaded");
	//--------------
	return 1;
}


