/*
 * J2534misc.h - J2534-related miscellaneous functionality
 *
 * Copyright (C) 2009-2016 Comer352l, 2016 MartinX
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

#ifndef J2534MISC_H
#define J2534MISC_H


#include <string>
#include <map>
#include <vector>

#ifdef __J2534_API_DEBUG__
#include <iostream>
#endif

#define		J2534API_ERROR_FCN_NOT_SUPPORTED	-1
#define		J2534API_ERROR_INVALID_LIBRARY		-2


enum class J2534_API_version {undefined, v0202, v0404};

// cannot use ALL_CAPS because of macros in J2534.h
enum class J2534_protocol_flags
{
	j1850vpw     = 0x01,
	j1850pwm     = 0x02,
	iso9141      = 0x04,
	iso14230     = 0x08,
	can          = 0x10,
	iso15765     = 0x20,
	sci_a_engine = 0x40,
	sci_a_trans  = 0x80,
	sci_b_engine = 0x100,
	sci_b_trans  = 0x200,
};

inline J2534_protocol_flags operator&(const J2534_protocol_flags f1, const J2534_protocol_flags f2)
{
	return J2534_protocol_flags(static_cast<unsigned int>(f1) & static_cast<unsigned int>(f2));
}

inline J2534_protocol_flags operator|(const J2534_protocol_flags f1, const J2534_protocol_flags f2)
{
	return J2534_protocol_flags(static_cast<unsigned int>(f1) | static_cast<unsigned int>(f2));
}



class J2534Library
{
public:
	std::string path;
	std::string name;
	J2534_protocol_flags protocols {J2534_protocol_flags(0)};
	J2534_API_version api {J2534_API_version::v0404};
};

class J2534misc {
public:
	static J2534_protocol_flags parseProtocol(const std::string& s);
	static J2534_API_version parseApiVersion(const std::string& s);
	static std::string apiVersionToStr(const J2534_API_version api);

#ifdef __J2534_API_DEBUG__
	static std::vector<std::string> protocolsToStrings(const J2534_protocol_flags pflags);
	static void printLibraryInfo(const std::vector<J2534Library>& PTlibraries);
#endif

private:
	const static std::map<std::string, J2534_protocol_flags> protocolsMap;
};

#endif
