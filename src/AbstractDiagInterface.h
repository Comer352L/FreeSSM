/*
 * AbstractDiagInterface.h - Abstract class for implementing automotive diagnostic interfaces
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

#ifndef ABSTRACTDIAGINTERFACE_H
#define ABSTRACTDIAGINTERFACE_H


#include <string>
#include <vector>


class AbstractDiagInterface
{

public:
	enum interface_type { interface_serialPassThrough, interface_J2534 };
	enum protocol_type { protocol_NONE, protocol_SSM1, protocol_SSM2_ISO14230, protocol_SSM2_ISO15765 };

	AbstractDiagInterface();
	virtual ~AbstractDiagInterface();
	virtual interface_type interfaceType() = 0;
	std::string name();
	std::string version();
	protocol_type protocolType();
	virtual bool open( std::string name ) = 0;
	virtual bool isOpen() = 0;
	virtual bool close() = 0;
	virtual bool connect(protocol_type protocol) = 0;
	virtual bool isConnected() = 0;
	virtual bool disconnect() = 0;
	virtual bool read(std::vector<char> *buffer) = 0;
	virtual bool write(std::vector<char> buffer) = 0;
	virtual bool clearSendBuffer() = 0;
	virtual bool clearReceiveBuffer() = 0;

protected:
	void setName(std::string name);
	void setVersion(std::string version);
	void setProtocolType(protocol_type protocoltype);

private:
	protocol_type _protocoltype;
	std::string _name;
	std::string _version;

};


#endif
