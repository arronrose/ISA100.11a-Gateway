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

/**
 * @author catalin.pop, radu.pop, beniamin.tecar, eduard.budulea
 */
#ifndef MAIN_H_
#define MAIN_H_
#include "Common/logging.h"
#include "Model/NetworkEngine.h"
#include "RunLib/Application.h"
#include "Security/SecurityManager.h"


/**
 * Main class.
 */
class Main : public run_lib::Application {
private:
	/**
	 * Declare the static log method.
	 */
	LOG_DEF("I.Main");

public:
	NE::Model::NetworkEngine * networkEngine;
	Isa100::Security::SecurityManager * securityManager;

	Main();
	virtual ~Main();

protected:
	virtual int runImplementation();
};

#endif /*MAIN_H_*/
