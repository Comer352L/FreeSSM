/*
 * TimeM.cpp - Time measurement  (Windows version)
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
	t_start = 0;
}


unsigned long int TimeM::start()
{
	unsigned int t_el = elapsed();
	t_start = GetTickCount();
	return t_el;
}


unsigned long int TimeM::elapsed()
{
	DWORD t = GetTickCount();
	if (!t_start) return 0;
	if (t >= t_start)
		return (t - t_start);
	else
		return (ULONG_MAX - t_start + t);
	/* NOTE: we are still getting wrong values, if (t - t_start) > 49.71 days */
}
