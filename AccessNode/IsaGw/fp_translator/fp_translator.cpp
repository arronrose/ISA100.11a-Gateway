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


#include "FptApp.h"
CFptApp* g_pApp;

int main( int argc, char** argv)
{
	log2flash("fp_translator started version %s",  CApp::GetVersion());

	g_pApp = new CFptApp;
	
	if ( !g_pApp->Init() )
	{
		ERR("main: Failed to start fp_translator.");
		log2flash("fp_translator ended <- init failed ");
		delete g_pApp ;
		return EXIT_FAILURE ;
	}
	g_pApp->Run();
	g_pApp->Close();
	log2flash("fp_translator ended");
	delete g_pApp ;
	return EXIT_SUCCESS ;
}
