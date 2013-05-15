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

/***************************************************************************
                          DSTHandler.h  -  description
                             -------------------
    begin                : Tue Aug 24 2004
    email                : marius.chile@nivis.com
 ***************************************************************************/

#ifndef DSTHANDLER_H
#define DSTHANDLER_H


/****************************************
  Class that handles DST operations
  *@author Marius Chile
*****************************************/

class CDSTHandler {
public: 
	CDSTHandler();
	~CDSTHandler();

	bool Scan(int* p_pnOffset);
};

#endif
