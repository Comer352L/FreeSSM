/*
 * J2534_API.cpp - API for accessing SAE-J2534 compliant interfaces
 *
 * Copyright (C) 2009-2010 Comer352l
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

#include "J2534_API.h"



J2534_API::J2534_API()
{
	_J2534LIB = NULL;
        _api_version = J2534_API_v0404;
	_PassThruOpen = NULL;
	_PassThruClose = NULL;
	_PassThruConnect_0202 = NULL;
	_PassThruConnect_0404 = NULL;
	_PassThruDisconnect = NULL;
	_PassThruReadVersion_0202 = NULL;
	_PassThruReadVersion_0404 = NULL;
	_PassThruGetLastError = NULL;
	_PassThruReadMsgs = NULL;
	_PassThruStartMsgFilter = NULL;
	_PassThruStopMsgFilter = NULL;
	_PassThruWriteMsgs = NULL;
	_PassThruStartPeriodicMsg = NULL;
	_PassThruStopPeriodicMsg = NULL;
	_PassThruIoctl = NULL;
	_PassThruSetProgrammingVoltage_0202 = NULL;
	_PassThruSetProgrammingVoltage_0404 = NULL;
}


J2534_API::~J2534_API()
{
	if (_J2534LIB)
	{
#ifdef __J2534_API_DEBUG__
		if (!dlclose( _J2534LIB ))
			std::cout << "J2534interface::~J2534interface(): dlclose() failed with error " << dlerror() << "\n";
#else
		dlclose( _J2534LIB );
#endif
	}
}


bool J2534_API::selectLibrary(std::string libPath)
{
	if (!libPath.size()) return false;
	void *newJ2534LIB = NULL;
	newJ2534LIB = dlopen( libPath.c_str(), RTLD_LAZY | RTLD_GLOBAL );	// RTLD_GLOBAL ???
	if (newJ2534LIB)
	{
		// Check if library is a valid J2534-library:
		if (!dlsym( newJ2534LIB, "PassThruConnect" ) || !dlsym( newJ2534LIB, "PassThruDisconnect" ))
		{
#ifdef __J2534_API_DEBUG__
			std::cout << "J2534interface::selectLibrary(): Error: the library doesn't provide the PassThruConnect(), and/or PassThruDisconnect() mehtods !\n";
			if (!dlclose( newJ2534LIB ))
				std::cout << "J2534interface::selectLibrary(): dlclose() failed with error " << dlerror() << "\n";
#else
			dlclose( newJ2534LIB );
#endif
			return false;
		}
		// Check API-version of the library:
		if (!dlsym( newJ2534LIB, "PassThruOpen" ) || !dlsym( newJ2534LIB, "PassThruClose" ))
			_api_version = J2534_API_v0202;
		else
			_api_version = J2534_API_v0404;
		// Close old library:
		if (_J2534LIB)
		{
#ifdef __J2534_API_DEBUG__
			if (!dlclose( _J2534LIB ))
				std::cout << "J2534interface::selectLibrary(): dlclose() failed with error " << dlerror() << "\n";
#else
			dlclose( _J2534LIB );
#endif
		}
		// Save data:
		_J2534LIB = newJ2534LIB;
		_lib_path = libPath;
		assignJ2534fcns();
	}
#ifdef __J2534_API_DEBUG__
	else
		std::cout << "J2534interface::selectLibrary(): dlopen() failed with error " << dlerror() << "\n";
#endif
	return newJ2534LIB;
}


std::string J2534_API::library()
{
	if (_J2534LIB)
		return _lib_path;
	else
		return "";
}


J2534_API_version J2534_API::libraryAPIversion()
{
	return _api_version;
}


void J2534_API::assignJ2534fcns()
{
	_PassThruOpen = reinterpret_cast< J2534_PassThruOpen >( dlsym( _J2534LIB, "PassThruOpen" ) );
	_PassThruClose = reinterpret_cast< J2534_PassThruClose >( dlsym( _J2534LIB, "PassThruClose" ) );
	if (_api_version == J2534_API_v0202)
	{
		_PassThruConnect_0202 = reinterpret_cast< J2534_PassThruConnect_0202 >( dlsym( _J2534LIB, "PassThruConnect" ) );
		_PassThruReadVersion_0202 = reinterpret_cast< J2534_PassThruReadVersion_0202 >( dlsym( _J2534LIB, "PassThruReadVersion" ) );
		_PassThruSetProgrammingVoltage_0202 = reinterpret_cast< J2534_PassThruSetProgrammingVoltage_0202 >( dlsym( _J2534LIB, "PassThruSetProgrammingVoltage" ) );
		_PassThruConnect_0404 = NULL;
		_PassThruReadVersion_0404 = NULL;
		_PassThruSetProgrammingVoltage_0404 = NULL;
	}
	else
	{
		_PassThruConnect_0202 = NULL;
		_PassThruReadVersion_0202 = NULL;
		_PassThruSetProgrammingVoltage_0202 = NULL;
		_PassThruConnect_0404 = reinterpret_cast< J2534_PassThruConnect_0404 >( dlsym( _J2534LIB, "PassThruConnect" ) );
		_PassThruReadVersion_0404 = reinterpret_cast< J2534_PassThruReadVersion_0404 >( dlsym( _J2534LIB, "PassThruReadVersion" ) );
		_PassThruSetProgrammingVoltage_0404 = reinterpret_cast< J2534_PassThruSetProgrammingVoltage_0404 >( dlsym( _J2534LIB, "PassThruSetProgrammingVoltage" ) );
	}
	_PassThruDisconnect = reinterpret_cast< J2534_PassThruDisconnect >( dlsym( _J2534LIB, "PassThruDisconnect" ) );
	_PassThruGetLastError = reinterpret_cast< J2534_PassThruGetLastError >( dlsym( _J2534LIB, "PassThruGetLastError" ) );
	_PassThruReadMsgs = reinterpret_cast< J2534_PassThruReadMsgs >( dlsym( _J2534LIB, "PassThruReadMsgs" ) );
	_PassThruStartMsgFilter = reinterpret_cast< J2534_PassThruStartMsgFilter >( dlsym( _J2534LIB, "PassThruStartMsgFilter" ) );
	_PassThruStopMsgFilter = reinterpret_cast< J2534_PassThruStopMsgFilter >( dlsym( _J2534LIB, "PassThruStopMsgFilter" ) );
	_PassThruWriteMsgs = reinterpret_cast< J2534_PassThruWriteMsgs >( dlsym( _J2534LIB, "PassThruWriteMsgs" ) );
	_PassThruStartPeriodicMsg = reinterpret_cast< J2534_PassThruStartPeriodicMsg >( dlsym( _J2534LIB, "PassThruStartPeriodicMsgs" ) );
	_PassThruStopPeriodicMsg = reinterpret_cast< J2534_PassThruStopPeriodicMsg >( dlsym( _J2534LIB, "PassThruStopPeriodicMsg" ) );
	_PassThruIoctl = reinterpret_cast< J2534_PassThruIoctl >( dlsym( _J2534LIB, "PassThruIoctl" ) );
}


std::vector<J2534Library> J2534_API::getAvailableJ2534Libs()
{
	std::vector<J2534Library> PTlibraries;


	// TODO

	J2534Library lib1;
	lib1.name = "libj2534client.so";
	lib1.path = "/home/martin/cpp/J2534Client/libj2534client.so";
	lib1.api = J2534_API_v0404;
	lib1.protocols = PROTOCOL_FLAG_ISO15765 | PROTOCOL_FLAG_ISO14230 | PROTOCOL_FLAG_ISO9141;

	PTlibraries.push_back(lib1);


#ifdef __J2534_API_DEBUG__
	printLibraryInfo(PTlibraries);
#endif
	return PTlibraries;
}


long J2534_API::PassThruOpen(void* pName, unsigned long *pDeviceID)	// 0404-API only
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruOpen) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruOpen(pName, pDeviceID);
}


long J2534_API::PassThruClose(unsigned long DeviceID)			// 0404-API only
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruClose) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruClose(DeviceID);
}


long J2534_API::PassThruConnect(unsigned long ProtocolID, unsigned long Flags, unsigned long *pChannelID)	// 0202-API
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruConnect_0202) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruConnect_0202(ProtocolID, Flags, pChannelID);
}


long J2534_API::PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long BaudRate, unsigned long *pChannelID)	// 0404-API
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruConnect_0404) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruConnect_0404(DeviceID, ProtocolID, Flags, BaudRate, pChannelID);
}


long J2534_API::PassThruDisconnect(unsigned long ChannelID)
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruDisconnect) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruDisconnect(ChannelID);
}


long J2534_API::PassThruReadVersion(char *pFirmwareVersion, char * pDllVersion, char *pApiVersion)	// 0202-API
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruReadVersion_0202) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruReadVersion_0202(pFirmwareVersion, pDllVersion, pApiVersion);
}


long J2534_API::PassThruReadVersion(unsigned long DeviceID, char *pFirmwareVersion, char * pDllVersion, char *pApiVersion)	// 0404-API
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruReadVersion_0404) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruReadVersion_0404(DeviceID, pFirmwareVersion, pDllVersion, pApiVersion);
}


long J2534_API::PassThruGetLastError(char *pErrorDescription)
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruGetLastError) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruGetLastError(pErrorDescription);
}


long J2534_API::PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruReadMsgs) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruReadMsgs(ChannelID, pMsg, pNumMsgs, Timeout);
}


long J2534_API::PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, PASSTHRU_MSG *pMaskMsg, PASSTHRU_MSG *pPatternMsg, PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID)
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruStartMsgFilter) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruStartMsgFilter(ChannelID, FilterType, pMaskMsg, pPatternMsg, pFlowControlMsg, pMsgID);
}


long J2534_API::PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID)
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruStopMsgFilter) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruStopMsgFilter(ChannelID, MsgID);
}


long J2534_API::PassThruWriteMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruWriteMsgs) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruWriteMsgs(ChannelID, pMsg, pNumMsgs, Timeout);
}


long J2534_API::PassThruStartPeriodicMsg(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval)
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruStartPeriodicMsg) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruStartPeriodicMsg(ChannelID, pMsg, pMsgID, TimeInterval);
}


long J2534_API::PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID)
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruStopPeriodicMsg) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruStopPeriodicMsg(ChannelID, MsgID);
}


long J2534_API::PassThruIoctl(unsigned long HandleID, unsigned long IoctlID, void *pInput, void *pOutput)
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruIoctl) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruIoctl(HandleID, IoctlID, pInput, pOutput);
}


long J2534_API::PassThruSetProgrammingVoltage(unsigned long PinNumber, unsigned long Voltage)	// 0202-API
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruSetProgrammingVoltage_0202) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruSetProgrammingVoltage_0202(PinNumber, Voltage);
}


long J2534_API::PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long PinNumber, unsigned long Voltage)	// 0404-API
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruSetProgrammingVoltage_0404) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruSetProgrammingVoltage_0404(DeviceID, PinNumber, Voltage);
}


#ifdef __J2534_API_DEBUG__
void J2534_API::printLibraryInfo(std::vector<J2534Library> PTlibraries)
{
	if (PTlibraries.size())
		std::cout << "Found " << PTlibraries.size() << " registered J2534-libraries:\n";
	else
		std::cout << "No J2534-libraries found.\n";
	for (unsigned int k=0; k<PTlibraries.size(); k++)
	{
		std::cout << "  Name:        " << PTlibraries.at(k).name << '\n';
		std::cout << "  Path:        " << PTlibraries.at(k).path << '\n';
		std::cout << "  API-version: ";
		if (PTlibraries.at(k).api == J2534_API_v0202)
			std::cout << "02.02\n";
		else
			std::cout << "04.04\n";
		std::cout << "  Protocols:   ";
		if (PTlibraries.at(k).protocols & PROTOCOL_FLAG_J1850VPW)
			std::cout << "J1850VPW ";
		if (PTlibraries.at(k).protocols & PROTOCOL_FLAG_J1850PWM)
			std::cout << "J1850PWM ";
		if (PTlibraries.at(k).protocols & PROTOCOL_FLAG_ISO9141)
			std::cout << "ISO9141 ";
		if (PTlibraries.at(k).protocols & PROTOCOL_FLAG_ISO14230)
			std::cout << "ISO14230 ";
		if (PTlibraries.at(k).protocols & PROTOCOL_FLAG_CAN)
			std::cout << "CAN ";
		if (PTlibraries.at(k).protocols & PROTOCOL_FLAG_ISO15765)
			std::cout << "ISO15765 ";
		if (PTlibraries.at(k).protocols & PROTOCOL_FLAG_SCI_A_ENGINE)
			std::cout << "SCI_A_ENGINE ";
		if (PTlibraries.at(k).protocols & PROTOCOL_FLAG_SCI_A_TRANS)
			std::cout << "SCI_A_TRANS ";
		if (PTlibraries.at(k).protocols & PROTOCOL_FLAG_SCI_B_ENGINE)
			std::cout << "SCI_B_ENGINE ";
		if (PTlibraries.at(k).protocols & PROTOCOL_FLAG_SCI_B_TRANS)
			std::cout << "SCI_B_TRANS ";
		std::cout << "\n\n";
	}
}
#endif

