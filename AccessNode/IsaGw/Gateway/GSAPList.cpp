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
/// @file GSAPList.cpp
/// @author Marcel Ionescu
/// @brief List with all open client connections - implementation
////////////////////////////////////////////////////////////////////////////////

#include <algorithm>

#include "../../Shared/Common.h"
#include "GSAPList.h"

/// TODO: use the functor from h.h (promote it in Common.h)
static void delete_object( CGSAP* p_pGSAP )
{
	delete p_pGSAP;
}

static void sDump( CGSAP* p_pGSAP )
{
	p_pGSAP->Dump();
}
////////////////////////////////////////////////////////////////////////////////
/// @class CGSAPList
////////////////////////////////////////////////////////////////////////////////
CGSAPList::~CGSAPList()
{
	for_each( m_aGSAPList.begin(), m_aGSAPList.end(), delete_object );
	m_aGSAPList.clear();
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Iterate trough all connections, call ProcessData, delete invalid entries
/// @retval true if at least one SAP_IN message was fully processed,
/// @retval false otherwise
/// @remarks problem returns true only on fully processed messages.
/// It does not return true on partial messages, when partial data was actually read form clients
/// Correct behawior would be to return true even on partial messages, so the main loop does a new ead immediately
/// Current implementation will limit the number of multi-packet messages which can be read from GSAP clients
/// The limit depends on the delay in CGwUAP::GwUAP_Task processIsaPackets
////////////////////////////////////////////////////////////////////////////////
bool CGSAPList::ProcessAll( void )
{	bool bRet = false;
	TGSAPList::iterator it = m_aGSAPList.begin();
	while( it != m_aGSAPList.end() )
	{
		if( (*it)->IsValid())
		{	/// return true if a least one SAP_IN was fully processed
			bRet  = (*it)->ProcessData() || bRet;
			++it;
		}
		else
		{
			delete (*it);
			it = m_aGSAPList.erase( it );
		}
	}
	return bRet;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief  Identify the CGSAP associated with a session ID
/// @param p_unSessionID Session to search for
/// @return A pointer to GSAP if there is one associated with p_unSessionID
/// @retval NULL if no GSAP is found associated with the session ID
////////////////////////////////////////////////////////////////////////////////
CGSAP* CGSAPList::FindGSAP( unsigned p_unSessionID )
{
	for( TGSAPList::iterator it = m_aGSAPList.begin(); it != m_aGSAPList.end(); ++it )
	{
		if( (*it)->HasSession( p_unSessionID ))
		{
			return (*it)->IsValid() ? (*it) : NULL;
		}
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief  Delete the session from all GSAp's (it should appear only one GSAP, only once)
/// @param p_unSessionID Session to delete
/// @remarks If the session appear more than once, log the inconsistency
////////////////////////////////////////////////////////////////////////////////
void CGSAPList::DelSession( unsigned p_unSessionID )
{	bool bFound = false;
	for( TGSAPList::iterator it = m_aGSAPList.begin(); it != m_aGSAPList.end(); ++it )
	{
		if( (*it)->HasSession( p_unSessionID ))
		{
			if( bFound )
			{
				LOG("WARNING CGSAPList::DelSession: session found/deleted more than once.");
			}
			(*it)->DelSession( p_unSessionID );	/// Will log multiple appeareances
			bFound = true;
		}
	}
	if( !bFound )
		LOG("WARNING CGSAPList::DelSession: session %u not found.", p_unSessionID);
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief  Log the object content
/// @remarks use a signal to log the whole app status
////////////////////////////////////////////////////////////////////////////////
void CGSAPList::Dump( void )
{
	LOG("CGSAPList (%d connections):", m_aGSAPList.size() );
	for_each( m_aGSAPList.begin(), m_aGSAPList.end(), sDump );
}
