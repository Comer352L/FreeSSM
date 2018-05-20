/*
 * TimeM.h - Time measurement  (Windows version)
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

#ifndef TIMEM_H
#define TIMEM_H


extern "C"
{
	#include <windows.h>
	#include <limits.h>
}


class TimeM
{
public:
	TimeM();
	unsigned long int start();
	unsigned long int elapsed();
	/* NOTE: although we return ms, the accuracy on Windows-systems worse than 16ms ! */
private:
	DWORD t_start; /* DWORD = unsigned long int */
};


#endif
