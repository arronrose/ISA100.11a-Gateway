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

#ifndef ISACSINFO_H_
#define ISACSINFO_H_

#include <string>
#include <nlib/datetime.h>

namespace nisa100 {
namespace hostapp {

class ISACSInfo
{
public:
	int m_tsapID;
	int m_reqType;	/*
					 * 0x3 - read
					 * 0x4 - write
					 * 0x5 - execute
					 */
	int m_objID;
	int m_objResID;
	int m_attrIndex1;
	int m_attrIndex2;

public:
	int m_ReadAsPublish; /*
						 * 1 - yes
						 * 0 - no
						 */
	int				m_rawData;
	bool			m_isISA;
	unsigned char	m_valueStatus;
	int				m_dbChannelNo;
	int				m_format;

public:
	std::string m_strReqDataBuff;
	std::string m_strRespDataBuff;

public:
	//nlib::DateTime	m_readTime;
	//short			m_milisec;
	struct timeval tv;
};

}// namespace hostapp
}// namespace nisa100

#endif /**/

