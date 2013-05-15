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
/// @file GSAPList.h
/// @author Marcel Ionescu
/// @brief List with all open client connections - interface
////////////////////////////////////////////////////////////////////////////////

#ifndef GSAPLIST_H
#define GSAPLIST_H

#include <list>
#include "GSAP.h"

////////////////////////////////////////////////////////////////////////////////
/// @class CGSAPList
/// @brief List with all client connections
////////////////////////////////////////////////////////////////////////////////
class CGSAPList
{
	typedef std::list<CGSAP*> TGSAPList;
	public:
		CGSAPList(){};
		~CGSAPList();
	
		/// Add a new connection. No need to expose a delete method: ProcessAll delete invalid GSAP's
		void Add( CGSAP* p_pGSAP){ m_aGSAPList.push_back( p_pGSAP ); };
		
		/// Iterate trough all sockets, call ProcessData, delete invalid GSAP's
		/// return true if at least one SAP_IN message was fully processed, false otherwise
		bool ProcessAll( void );

		/// Log the object content
		void Dump( void );

		/// Identify the CGSAP associated with a session ID. Return NULL if not found
		CGSAP* FindGSAP( unsigned p_unSessionID );

		/// Delete the session from all GSAp's (it should appear only one GSAP, only once)
		void DelSession( unsigned p_unSessionID );
	private:
		TGSAPList m_aGSAPList;
};

#endif //GSAPLIST_H
