/*
 * J2534DiagInterface.h - J2534-pass-through diagnostic interface
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

#ifndef J2534DIAGINTERFACE_H
#define J2534DIAGINTERFACE_H


#include <string>
#include "AbstractDiagInterface.h"
#ifdef __WIN32__
	#include "windows\J2534_API.h"
#elif defined __linux__
	#include "linux/J2534_API.h"
#else
	#error "Operating system not supported !"
#endif
#ifdef __FSSM_DEBUG__
	#include "libFSSM.h"
#endif

class J2534DiagInterface : public AbstractDiagInterface
{

public:
	J2534DiagInterface();
	~J2534DiagInterface();
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
	J2534_API *_j2534;
	bool _connected;
	unsigned long _DeviceID;
	unsigned long _ChannelID;
	unsigned long _FilterID[10];	// SAE J2534 allows max. 10 filters
	unsigned char _numFilters;

#ifdef __FSSM_DEBUG__
	void printErrorDescription(std::string title, long ret);
#endif

};


#endif
