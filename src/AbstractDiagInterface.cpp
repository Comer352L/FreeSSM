/*
 * AbstractDiagInterface.cpp - Abstract class for implementing automotive diagnostic interfaces
 *
 * Copyright (C) 2010-2012 Comer352L
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

#include "AbstractDiagInterface.h"


AbstractDiagInterface::AbstractDiagInterface()
{
	_protocoltype = protocol_type::NONE;
	_protocol_baudrate = 0;
}


AbstractDiagInterface::~AbstractDiagInterface()
{
}


std::string AbstractDiagInterface::name()
{
	return _name;
}


std::string AbstractDiagInterface::version()
{
	return _version;
}


std::vector<AbstractDiagInterface::protocol_type> AbstractDiagInterface::supportedProtocols()
{
	return _supportedProtocols;
}


std::vector<std::string> AbstractDiagInterface::supportedProtocolsDescriptions()
{
	std::vector<std::string> pdesclist;
	for (unsigned char p=0; p<_supportedProtocols.size(); p++)
		pdesclist.push_back( protocolDescription( _supportedProtocols.at(p) ) );
	return pdesclist;
}


AbstractDiagInterface::protocol_type AbstractDiagInterface::protocolType()
{
	return _protocoltype;
}


std::string AbstractDiagInterface::protocolDescription()
{
	return protocolDescription( _protocoltype );
}


unsigned int AbstractDiagInterface::protocolBaudRate()
{
	return _protocol_baudrate;
}


// PROTECTED


void AbstractDiagInterface::setName(std::string name)
{
	_name = name;
}


void AbstractDiagInterface::setVersion(std::string version)
{
	_version = version;
}


void AbstractDiagInterface::setSupportedProtocols(std::vector<AbstractDiagInterface::protocol_type> protocols)
{
	_supportedProtocols = protocols;
}


void AbstractDiagInterface::setProtocolType(protocol_type protocoltype)
{
	_protocoltype = protocoltype;
}


void AbstractDiagInterface::setProtocolBaudrate(unsigned int baudrate)
{
	_protocol_baudrate = baudrate;
}


// PRIVATE


std::string AbstractDiagInterface::protocolDescription(protocol_type protocol)
{
	switch (protocol)
	{
		case protocol_type::NONE:
			return "";
		case protocol_type::SSM1:
			return "SSM1 (type 1)";
		case protocol_type::SSM2_ISO14230:
			return "SSM2 / ISO-14230";
		case protocol_type::SSM2_ISO15765:
			return "SSM2 / ISO-15765";
		default:	// BUG
			return "UNKNOWN";
	}
}

