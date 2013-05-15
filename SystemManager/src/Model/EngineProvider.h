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

/*
 * EngineProvider.h
 *
 *  Created on: Mar 3, 2009
 *      Author: mulderul(catalin.pop)
 */

#ifndef ENGINEPROVIDER_H_
#define ENGINEPROVIDER_H_

#include "Model/IEngine.h"

namespace Isa100 {

namespace Model {

namespace EngineProvider {

	/**
	 * Retrieves the current configured engine instance. If the engine was configured with setEngine,
	 * the instance of engine passed to set method will be returned.
	 * There should not be cases at normal operation when an engine is not configured.
	 * @return an instance of IEngine or null
	 */
	NE::Model::IEngine * getEngine();

	/**
	 * Configures an instance of IEngine to be used by the application.
	 * @param engine
	 */
	void setEngine(NE::Model::IEngine * engine);
}

}

}

#endif /* ENGINEPROVIDER_H_ */
