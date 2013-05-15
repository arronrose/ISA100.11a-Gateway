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
 * DurationWatcher.h
 *
 *  Created on: Jul 16, 2009
 *      Author: Andy
 */

#ifndef DURATIONWATCHER_H_
#define DURATIONWATCHER_H_

#include <sys/times.h>
#include <nlib/log.h>

namespace Isa100 {

class CProcessTimes
{

public:
    const long int CLOCK_TCK;
	CProcessTimes(long int CLOCK_TCK_): CLOCK_TCK(CLOCK_TCK_)
	{
		ReadTime();
	}
	CProcessTimes( const CProcessTimes& p_rProcTime ):CLOCK_TCK(p_rProcTime.CLOCK_TCK)
	{
		*this = p_rProcTime;
	}
	CProcessTimes&  operator = (const CProcessTimes& p_rProcTime)
	{
		m_nLastTimeReal = p_rProcTime.m_nLastTimeReal;
		m_nLastTimeUser = p_rProcTime.m_nLastTimeUser;
		m_nLastTimeSys	= p_rProcTime.m_nLastTimeSys;
		return *this;
	}

	void ReadTime()
	{
		struct tms tms_buf;
		int nCrtTime = times( &tms_buf );

		m_nLastTimeReal = nCrtTime;
		m_nLastTimeUser = tms_buf.tms_utime;
		m_nLastTimeSys = tms_buf.tms_stime;
	}

	int  ProcGetUserDiff (const CProcessTimes& p_rProcTime)
	{
		return (int)(m_nLastTimeUser - p_rProcTime.m_nLastTimeUser) * 1000 / CLOCK_TCK;
	}

	int  ProcGetSysDiff (const CProcessTimes& p_rProcTime)
	{
		return (int)(m_nLastTimeSys - p_rProcTime.m_nLastTimeSys) * 1000 / CLOCK_TCK;
	}

	int  ProcGetTotalDiff (const CProcessTimes& p_rProcTime)
	{
		return ProcGetUserDiff(p_rProcTime) + ProcGetSysDiff(p_rProcTime);
	}

	int  RealGetDiff (const CProcessTimes& p_rProcTime)
	{
		return (m_nLastTimeReal - p_rProcTime.m_nLastTimeReal) * 1000 / CLOCK_TCK;
	}


private:
	clock_t m_nLastTimeUser;
	clock_t m_nLastTimeSys;
	clock_t	m_nLastTimeReal;
};


class CDurationWatcher
{
    const long int CLOCK_TCK;
public:
	LOG_DEF("DurationWatcher");

	CDurationWatcher():CLOCK_TCK( sysconf( _SC_CLK_TCK )), m_oLastTime(CLOCK_TCK){}

	// @  p_nDuration - msec
	int WatchDuration (const char *p_szFile, const char* p_szFunc, int p_nLine, int p_nRealDuration, int p_nProcessDuration)
	{
		CProcessTimes oCrtTime(CLOCK_TCK);

		int ret = (oCrtTime.RealGetDiff(m_oLastTime) > p_nRealDuration || oCrtTime.ProcGetTotalDiff(m_oLastTime) > p_nProcessDuration ) ;
		if (ret)
		{
			char buffer[3000];

			sprintf(buffer, "WatchDuration: %s; %s; line=%d real=%dms (l=%dms), proc=%dms (l=%dms) usr=%dms sys=%dms", p_szFile, p_szFunc, p_nLine,
					oCrtTime.RealGetDiff(m_oLastTime), p_nRealDuration, oCrtTime.ProcGetTotalDiff(m_oLastTime),  p_nProcessDuration,
					oCrtTime.ProcGetUserDiff(m_oLastTime), oCrtTime.ProcGetSysDiff(m_oLastTime)
				 );

			LOG_INFO(buffer);
		}

		m_oLastTime = oCrtTime;

		return ret;
	}

private:
	CProcessTimes m_oLastTime;
};

// duration in ms
#define DurationWatcherReal		(500)
#define DurationWatcherProc		(500)

#define WATCH_DURATION_DEF(obj) ((obj).WatchDuration( __FILE__, __FUNCTION__, __LINE__,DurationWatcherReal,DurationWatcherProc))
#define WATCH_DURATION(obj,real_dur,proc_dur) ((obj).WatchDuration( __FILE__, __FUNCTION__, __LINE__,real_dur,proc_dur))

} // namespace Isa100

#endif /* DURATIONWATCHER_H_ */
