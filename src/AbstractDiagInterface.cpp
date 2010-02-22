/*
 * AbstractDiagInterface.cpp - Abstract class for implementing automotive diagnostic interfaces
 *
 * Copyright (C) 2010 Comer352l
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
	_protocoltype = protocol_NONE;
}


AbstractDiagInterface::~AbstractDiagInterface()
{
}


std::string AbstractDiagInterface::name()
{
	return _name;
};


std::string AbstractDiagInterface::version()
{
	return _version;
};


AbstractDiagInterface::protocol_type AbstractDiagInterface::protocolType()
{
	return _protocoltype;
};


// PRIVATE


void AbstractDiagInterface::setName(std::string name)
{
	_name = name;
};


void AbstractDiagInterface::setVersion(std::string version)
{
	_version = version;
};


void AbstractDiagInterface::setProtocolType(protocol_type protocoltype)
{
	_protocoltype = protocoltype;
};

