/*
 * bfVFS : vfs/Tools/vfs_hp_timer.cpp
 *  - high performance/precision timer, used by profiler
 *
 * Copyright (C) 2008 - 2012 (BF) john.bf.smith@googlemail.com
 *
 * This file is part of the bfVFS library
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <vfs/Tools/vfs_hp_timer.h>

#ifndef WIN32
static void time_diff(timespec const& t1, timespec const& t2, timespec& diff)
{
	diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
	if(diff.tv_nsec < 0)
	{
		diff.tv_sec   = t2.tv_sec - t1.tv_sec - 1;
		diff.tv_nsec += 1000000000;
	}
	else
	{
		diff.tv_sec   = t2.tv_sec - t1.tv_sec;
	}
}
#endif

vfs::HPTimer::HPTimer() : is_running(false)
{
#ifdef WIN32
	QueryPerformanceFrequency(&ticksPerSecond);
#else
	/*
	 * Returned precision seems always to be 1 nanosecond.
	 * Use that "fact" to compute elapsed time with a static value.
	 * Please inform me if you notice errors in the timing computations on your system.
	 */
	clock_getres(CLOCK_MONOTONIC, &precision);
#endif
}

vfs::HPTimer::~HPTimer()
{
}

void vfs::HPTimer::startTimer()
{
#ifdef WIN32
	QueryPerformanceCounter(&tick);
#else
	clock_gettime(CLOCK_MONOTONIC, &t1);
#endif
	is_running = true;
}

long long vfs::HPTimer::ticks()
{
	if(is_running)
	{
#ifdef WIN32
		QueryPerformanceCounter(&tick2);
		return tick2.QuadPart - tick.QuadPart;
#else
		clock_gettime(CLOCK_MONOTONIC, &t2);
		time_diff(t1,t2, diff);
		return  1000000000*diff.tv_sec + diff.tv_nsec;
#endif
	}
	return 0;
}

double vfs::HPTimer::running()
{
	if(is_running)
	{
#ifdef WIN32
		QueryPerformanceCounter(&tick2);
		return (double)(tick2.QuadPart - tick.QuadPart)/(double)ticksPerSecond.QuadPart;
#else
		clock_gettime(CLOCK_MONOTONIC, &t2);
		time_diff(t1,t2, diff);
		return  diff.tv_sec + diff.tv_nsec/(1000000000.0);
#endif
	}
	return 0;
}

void vfs::HPTimer::stopTimer()
{
#ifdef WIN32
	QueryPerformanceCounter(&tick2);
#else
	clock_gettime(CLOCK_MONOTONIC, &t2);
#endif
	is_running = false;
}

double vfs::HPTimer::getElapsedTimeInSeconds()
{
#ifdef WIN32
	return (double)(tick2.QuadPart - tick.QuadPart)/(double)ticksPerSecond.QuadPart;
#else
	timespec diff;
	time_diff(t1,t2, diff);
	return diff.tv_sec + diff.tv_nsec/(1000000000.0);
#endif
}

