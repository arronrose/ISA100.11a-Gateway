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

#ifndef INCLUDE_ALL_H_
#define INCLUDE_ALL_H_

#ifdef __USE_W32_SOCKETS
#	include <winsock2.h> //hack [nicu.dascalu] - should be included before asio
#endif
#include <boost/asio.hpp>


//add here all includes for precompiled header


//for stl
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <stdexcept>


//for boost
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>

#include <boost/format.hpp>


#include <boost/cstdint.hpp>
#include <boost/thread.hpp>
#include <boost/regex.hpp>

//for log
#include <nlib/log.h>
#include <nlib/exception.h>


//for sqlitexx
#include <nlib/sqlitexx/Connection.h>

//for asio



#endif /*INCLUDE__ALL_H_*/
