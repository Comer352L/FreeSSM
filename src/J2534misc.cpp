/*
 * J2534misc.cpp -  J2534-related miscellaneous functionality
 *
 * Copyright (C) 2009-2016 Comer352L, 2016 MartinX
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


#include "J2534misc.h"


static std::string toupper(const std::string& s)
{
	std::string result(s.length(), '\0');
	for (size_t i = 0; i < result.length(); ++i)
		result[i] = toupper(s[i]);
	return result;
}

const std::map<std::string, J2534_protocol_flags> J2534misc::protocolsMap {
	{"J1850VPW", J2534_protocol_flags::j1850vpw},
	{"J1850PWM", J2534_protocol_flags::j1850pwm},
	{"ISO9141", J2534_protocol_flags::iso9141},
	{"ISO14230", J2534_protocol_flags::iso14230},
	{"CAN", J2534_protocol_flags::can},
	{"ISO15765", J2534_protocol_flags::iso15765},
	{"SCI_A_ENGINE", J2534_protocol_flags::sci_a_engine},
	{"SCI_A_TRANS", J2534_protocol_flags::sci_a_trans},
	{"SCI_B_ENGINE", J2534_protocol_flags::sci_b_engine},
	{"SCI_B_TRANS", J2534_protocol_flags::sci_b_trans},
};

J2534_protocol_flags J2534misc::parseProtocol(const std::string& s)
{
	const std::string key = toupper(s);
	const std::map<std::string, J2534_protocol_flags>::const_iterator it = protocolsMap.find(key);
	return (it != protocolsMap.end()) ? it->second : J2534_protocol_flags(0);
}

J2534_API_version J2534misc::parseApiVersion(const std::string& s)
{
	if (s == "02.02") return J2534_API_version::v0202;
	if (s == "04.04") return J2534_API_version::v0404;
	return J2534_API_version::undefined;
}

std::string J2534misc::apiVersionToStr(const J2534_API_version api)
{
	switch(api) {
	case J2534_API_version::v0202 : return "02.02";
	case J2534_API_version::v0404 : return "04.04";
	default: return "UNDEFINED";
	}
}

#ifdef __J2534_API_DEBUG__
std::vector<std::string> J2534misc::protocolsToStrings(const J2534_protocol_flags pflags)
{
	std::vector<std::string> vs;
	for (const std::pair<std::string, J2534_protocol_flags>& pair : protocolsMap) {
		if (bool(pflags & pair.second))
			vs.push_back(pair.first);
	}
	return vs;
}

void J2534misc::printLibraryInfo(const std::vector<J2534Library>& PTlibraries)
{
	const size_t libcount = PTlibraries.size();
	if (libcount)
		std::cout << "Found " << libcount << " registered J2534-libraries:\n";
	else
		std::cout << "No J2534-libraries found.\n";

	for (const J2534Library& lib : PTlibraries)
	{
		std::cout << "\n  Name:        " << lib.name;
		std::cout << "\n  Path:        " << lib.path;
		std::cout << "\n  API-version: " << apiVersionToStr(lib.api);
		std::cout << "\n  Protocols:   ";

		const std::vector<std::string> protocolStrings = protocolsToStrings(lib.protocols);
		for(size_t i = 0; i < protocolStrings.size(); ++i) {
			if (i)
				std::cout << " ";
			std::cout << protocolStrings.at(i);
		}
		std::cout << "\n\n";
	}
}
#endif
