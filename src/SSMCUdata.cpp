/*
 * SSMCUdata.cpp - Class to manage SYS_ID, ROM_ID and flagbytes
 *
 * Copyright (C) 2016 MartinX, 2018 Comer352L
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

#include "SSMCUdata.h"


SSMCUdata::SSMCUdata()
{
}


SSMCUdata::~SSMCUdata()
{
}


void SSMCUdata::clear()
{
	SYS_ID.clear();
	ROM_ID.clear();
	flagbytes.clear();
}


void SSMCUdata::from_SSMP2(const char* const data, const std::size_t size)
{
	const std::size_t offset_SYS_ID = 0;
	const std::size_t offset_ROM_ID = offset_SYS_ID + SYS_ID_size;
	const std::size_t offset_flagbytes = offset_ROM_ID + ROM_ID_size;

	clear();

	if (size < offset_SYS_ID + SYS_ID_size)
		return;
	SYS_ID.assign(data + offset_SYS_ID, data + offset_SYS_ID + SYS_ID_size);

	if (size < offset_ROM_ID + ROM_ID_size)
		return;
	ROM_ID.assign(data + offset_ROM_ID, data + offset_ROM_ID + ROM_ID_size);

	if (size <= offset_flagbytes)
		return;
	flagbytes.assign(data + offset_flagbytes, data + size);
}


void SSMCUdata::from_SSMP1(const char* const data, const std::size_t size)
{
	const std::size_t offset_SYS_ID = 0;
	const std::size_t offset_flagbytes = offset_SYS_ID + SYS_ID_size;

	clear();

	if (size < offset_SYS_ID + SYS_ID_size)
		return;
	SYS_ID.assign(data + offset_SYS_ID, data + offset_SYS_ID + SYS_ID_size);

	if (size <= offset_flagbytes)
		return;
	flagbytes.assign(data + offset_flagbytes, data + size);
}


bool SSMCUdata::flagbytebit(const std::size_t byteindex, const unsigned char bitnr) const
{
	if (byteindex >= flagbytes.size())
		return false;
	const unsigned char bitmask = 1 << bitnr;
	return static_cast<unsigned char>(flagbytes.at(byteindex)) & bitmask;
}


bool SSMCUdata::uses_Flagbytes() const
{
	return ((SYS_ID.size() >= 2) && ((SYS_ID.at(0) & '\xF0') == '\xA0'));
}


bool SSMCUdata::uses_Ax10xx_defs() const
{
	return (uses_Flagbytes() && (SYS_ID.at(1) == '\x10'));
}
