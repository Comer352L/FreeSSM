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


enum J2534_API_version {J2534_API_UNDEFINED, J2534_API_v0202, J2534_API_v0404};

enum J2534_protocol_flags
{
	PROTOCOL_FLAG_J1850VPW     = 0x01,
	PROTOCOL_FLAG_J1850PWM     = 0x02,
	PROTOCOL_FLAG_ISO9141      = 0x04,
	PROTOCOL_FLAG_ISO14230     = 0x08,
	PROTOCOL_FLAG_CAN          = 0x10,
	PROTOCOL_FLAG_ISO15765     = 0x20,
	PROTOCOL_FLAG_SCI_A_ENGINE = 0x40,
	PROTOCOL_FLAG_SCI_A_TRANS  = 0x80,
	PROTOCOL_FLAG_SCI_B_ENGINE = 0x100,
	PROTOCOL_FLAG_SCI_B_TRANS  = 0x200,
};

class J2534Library
{
public:
	std::string path;
	std::string name;
	J2534_protocol_flags protocols;
	J2534_API_version api;

	J2534Library() { protocols = J2534_protocol_flags(0); api = J2534_API_v0404; }
};

class J2534misc {
public:
	static J2534_protocol_flags parseProtocol(const std::string& s);
	static std::vector<std::string> protocolsToStr(const J2534_protocol_flags pflags);
	static J2534_API_version parseApiVersion(const std::string& s);
	static std::string apiVersionToStr(const J2534_API_version api);

#ifdef __J2534_API_DEBUG__
	static void printLibraryInfo(const std::vector<J2534Library>& PTlibraries);
#endif

private:
	const static std::map<std::string, J2534_protocol_flags> protocolsMap;

	static const std::map<std::string, J2534_protocol_flags> initProtocolsMap();
};

#endif
