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
                          AppS.h  -  description
                             -------------------
    begin                : Tue May 14 2002
    email                : claudiu.hobeanu@nivis.com
 ***************************************************************************/

#ifndef APPS_H
#define APPS_H


#include "../Shared/pipe.h"
#include "../Shared/Events.h"
#include "../Shared/app.h"
#include "../Shared/Config.h"

//scheduler sleep time in sec
#define SCHEDULER_SLEEP_TIME	1

class CAppS : public CApp
{
public:
	CAppS();
	~CAppS();

	void	Run();
	int		Init();
    
    /** sweeps events_file and sends the commands who are scheduled to be send at this time*/
	void	SweepEvents();
	int		GetSchedSleepTime(){ return m_nSchedulerSleepTime; }

    CConfig m_stConfig;
protected:
	void checkNullRuleFile();

	/**	compose command to be send to DNC from event */
	int getCommandFromEvent( const CEventsStruct* ev, char*& p_pCommands, int& p_nCommandsLen  );

private:
	/** pipe to DNC */
	CPipe		m_oPipeToMesh;
	CPipe		m_oPipeToLocalDN;
	
	CEvents		m_clEvents;
	int         m_nSchedulerSleepTime;	
};

#endif
