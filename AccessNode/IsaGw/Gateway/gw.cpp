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
                          gw.cpp  -  description
                             -------------------
    begin                : Thu Apr 11 2002
    email                : marcel.ionescu@nivis.com
 ***************************************************************************/
//#include <new>
#include "../../Shared/SignalsMgr.h"
#include "GwApp.h"
/*
typedef struct^M
{  ^M
  uint16   m_unContractID;^M
  uint16   m_unShortSrcAddress; ^M
  uint8    m_aEUI64SrcAddress[8]; ^M
  uint8    m_aEncryptionKey[16];   ^M
  uint8    m_ucEncryptionSettings; ^M
  uint8    m_ucUDPPorts; ^M
}TLME_CONTRACT_ATTRIBUTES;^M
*/

/// This is the main() function of the application. It simply runs the GwApp object
int main(int argc, char *argv[])
{
	if (argc >1 && strcmp( argv[1], "-v") == 0 ) 
	{
		LOG( "Version %s", CApp::GetVersion() );
		return 0;
	}
	CSignalsMgr::Ignore( SIGUSR2 ); ///< Ignore until we install the real handler

	log2flash("ISA_GW Start: version %s", CApp::GetVersion() );
	if( g_stApp.Init() )
	{
		try
		{
			g_stApp.Run(); //entering normal operation loop
			g_stApp.Close();
		}
		catch( std::bad_alloc &ba )
		{
			g_stLog.EmergencyMsg("FATAL ERROR Gateway: catch std::bad_alloc. Probably OUT OF MEMORY");
		}
		catch(...)
		{
			g_stLog.EmergencyMsg("FATAL ERROR Gateway: catch(...)");
		}
		log2flash("ISA_GW end");
		return 0;
	}
	log2flash("ISA_GW end <- Init failed");
	return 1;
}
