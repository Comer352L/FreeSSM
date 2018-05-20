/*
 * SerialPassThroughDiagInterface.h - Serial port pass-through diagnostic interface
 *
 * Copyright (C) 2010-2018 Comer352L
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

#ifndef SERIALPASSTHROUGHDIAGINTERFACE_H
#define SERIALPASSTHROUGHDIAGINTERFACE_H


#include <string>
#include <vector>
#include "AbstractDiagInterface.h"
#ifdef __WIN32__
	#include "windows\serialCOM.h"
	#include "windows\TimeM.h"
	#define waitms(x) Sleep(x)
#elif defined __linux__
	#include <unistd.h>
	#include "linux/serialCOM.h"
	#include "linux/TimeM.h"
	#define waitms(x) usleep(1000*x)
#else
	#error "Operating system not supported !"
#endif
#ifdef __FSSM_DEBUG__
	#include <iostream>
#endif


class SerialPassThroughDiagInterface : public AbstractDiagInterface
{

public:
	SerialPassThroughDiagInterface();
	~SerialPassThroughDiagInterface();
	interface_type interfaceType();
	bool open( std::string name );
	bool isOpen();
	bool close();
	bool connect(AbstractDiagInterface::protocol_type protocol);
	bool isConnected();
	bool disconnect();
	bool read(std::vector<char> *buffer);
	bool write(std::vector<char> buffer);
	bool clearSendBuffer();
	bool clearReceiveBuffer();

private:
	serialCOM *_port;
	bool _connected;

};


#endif
