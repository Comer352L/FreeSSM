/*
 * TimeM.cpp - Time measurement  (Linux version)
 *
 * Copyright (C) 2009 Comer352l
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TimeM.h"


TimeM::TimeM()
{
	t_start.tv_sec = 0;
	t_start.tv_nsec = 0;
}


unsigned long int TimeM::start()
{
	unsigned int t_el = elapsed();
	clock_gettime(CLOCK_REALTIME, &t_start);
	return t_el;
}


unsigned long int TimeM::elapsed()
{
	struct timespec t;
	if (!t_start.tv_sec && !t_start.tv_nsec) return 0;
	unsigned long int delta_s=0, delta_ns=0, t_el=0;
	clock_gettime(CLOCK_REALTIME, &t);	// returns -1 on error, 0 on success
	delta_s = t.tv_sec - t_start.tv_sec;	// field t.tv_sec is of type long (linux) => overflow every 68 years :-) )
	if (t.tv_nsec >= t_start.tv_nsec)
		delta_ns = t.tv_nsec - t_start.tv_nsec;
	else
	{
		delta_ns = 1000000000 - t_start.tv_nsec + t.tv_nsec;
		delta_s--;
	}
	t_el = delta_s*1000 + delta_ns/1000000;
	return t_el;
}
