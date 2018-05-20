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
	_api_version = J2534_API_version::v0404;
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
		if (!FreeLibrary( _J2534LIB ))
			std::cout << "J2534interface::~J2534interface(): FreeLibrary() failed with error " << GetLastError() << "\n";
#else
		FreeLibrary( _J2534LIB );
#endif
	}
}


bool J2534_API::selectLibrary(std::string libPath)
{
	if (!libPath.size()) return false;
	HINSTANCE newJ2534LIB = NULL;
	newJ2534LIB = LoadLibraryA( libPath.c_str() );
	if (newJ2534LIB)
	{
		// Check if library is a valid J2534-library:
		if (!GetProcAddress( newJ2534LIB, "PassThruConnect" ) || !GetProcAddress( newJ2534LIB, "PassThruDisconnect" ))
		{
#ifdef __J2534_API_DEBUG__
			std::cout << "J2534interface::selectLibrary(): Error: the library doesn't provide the PassThruConnect(), and/or PassThruDisconnect() methods !\n";
			if (!FreeLibrary( newJ2534LIB ))
				std::cout << "J2534interface::selectLibrary(): FreeLibrary() failed with error " << GetLastError() << "\n";
#else
			FreeLibrary( newJ2534LIB );
#endif
			return false;
		}
		// Check API-version of the library:
		if (!GetProcAddress( newJ2534LIB, "PassThruOpen" ) || !GetProcAddress( newJ2534LIB, "PassThruClose" ))
			_api_version = J2534_API_version::v0202;
		else
			_api_version = J2534_API_version::v0404;
		// Close old library:
		if (_J2534LIB)
		{
#ifdef __J2534_API_DEBUG__
			if (!FreeLibrary( _J2534LIB ))
				std::cout << "J2534interface::selectLibrary(): FreeLibrary() failed with error " << GetLastError() << "\n";
#else
			FreeLibrary( _J2534LIB );
#endif
		}
		// Save data:
		_J2534LIB = newJ2534LIB;
		_lib_path = libPath;
		assignJ2534fcns();
	}
#ifdef __J2534_API_DEBUG__
	else
		std::cout << "J2534interface::selectLibrary(): LoadLibrary() failed with error " << GetLastError() << "\n";
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
	_PassThruOpen = reinterpret_cast< J2534_PassThruOpen >( GetProcAddress( _J2534LIB, "PassThruOpen" ) );
	_PassThruClose = reinterpret_cast< J2534_PassThruClose >( GetProcAddress( _J2534LIB, "PassThruClose" ) );
	if (_api_version == J2534_API_version::v0202)
	{
		_PassThruConnect_0202 = reinterpret_cast< J2534_PassThruConnect_0202 >( GetProcAddress( _J2534LIB, "PassThruConnect" ) );
		_PassThruReadVersion_0202 = reinterpret_cast< J2534_PassThruReadVersion_0202 >( GetProcAddress( _J2534LIB, "PassThruReadVersion" ) );
		_PassThruSetProgrammingVoltage_0202 = reinterpret_cast< J2534_PassThruSetProgrammingVoltage_0202 >( GetProcAddress( _J2534LIB, "PassThruSetProgrammingVoltage" ) );
		_PassThruConnect_0404 = NULL;
		_PassThruReadVersion_0404 = NULL;
		_PassThruSetProgrammingVoltage_0404 = NULL;
	}
	else
	{
		_PassThruConnect_0202 = NULL;
		_PassThruReadVersion_0202 = NULL;
		_PassThruSetProgrammingVoltage_0202 = NULL;
		_PassThruConnect_0404 = reinterpret_cast< J2534_PassThruConnect_0404 >( GetProcAddress( _J2534LIB, "PassThruConnect" ) );
		_PassThruReadVersion_0404 = reinterpret_cast< J2534_PassThruReadVersion_0404 >( GetProcAddress( _J2534LIB, "PassThruReadVersion" ) );
		_PassThruSetProgrammingVoltage_0404 = reinterpret_cast< J2534_PassThruSetProgrammingVoltage_0404 >( GetProcAddress( _J2534LIB, "PassThruSetProgrammingVoltage" ) );
	}
	_PassThruDisconnect = reinterpret_cast< J2534_PassThruDisconnect >( GetProcAddress( _J2534LIB, "PassThruDisconnect" ) );
	_PassThruGetLastError = reinterpret_cast< J2534_PassThruGetLastError >( GetProcAddress( _J2534LIB, "PassThruGetLastError" ) );
	_PassThruReadMsgs = reinterpret_cast< J2534_PassThruReadMsgs >( GetProcAddress( _J2534LIB, "PassThruReadMsgs" ) );
	_PassThruStartMsgFilter = reinterpret_cast< J2534_PassThruStartMsgFilter >( GetProcAddress( _J2534LIB, "PassThruStartMsgFilter" ) );
	_PassThruStopMsgFilter = reinterpret_cast< J2534_PassThruStopMsgFilter >( GetProcAddress( _J2534LIB, "PassThruStopMsgFilter" ) );
	_PassThruWriteMsgs = reinterpret_cast< J2534_PassThruWriteMsgs >( GetProcAddress( _J2534LIB, "PassThruWriteMsgs" ) );
	_PassThruStartPeriodicMsg = reinterpret_cast< J2534_PassThruStartPeriodicMsg >( GetProcAddress( _J2534LIB, "PassThruStartPeriodicMsgs" ) );
	_PassThruStopPeriodicMsg = reinterpret_cast< J2534_PassThruStopPeriodicMsg >( GetProcAddress( _J2534LIB, "PassThruStopPeriodicMsg" ) );
	_PassThruIoctl = reinterpret_cast< J2534_PassThruIoctl >( GetProcAddress( _J2534LIB, "PassThruIoctl" ) );
}


std::vector<J2534Library> J2534_API::getAvailableJ2534Libs()
{
	std::vector<J2534Library> PTlibraries;
	HKEY hKey1, hKey2;
	DWORD index = 0;
	char KeyName[256] = "";
	long ret = 0;

	ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ("Software"), 0, KEY_READ, &hKey1);
	if (ret != ERROR_SUCCESS)
	{
#ifdef __J2534_API_DEBUG__
		std::cout << "J2534interface::getAvailableJ2534Libs():   RegOpenKeyEx(...) for 'HKEY_LOCAL_MACHINE\\Software' failed with error " << ret << "\n";
#endif
		return PTlibraries;
	}
	// Search for keys "PassThruSupport", "PassThruSupport.04.04":
	while ((RegEnumKeyA(hKey1, index, KeyName, 256)) != ERROR_NO_MORE_ITEMS)
	{
		if (!strncmp(KeyName, "PassThruSupport", 15))
		{
			ret = RegOpenKeyExA(hKey1, KeyName, 0, KEY_READ, &hKey2);   // "PassThruSupportXXX"
			if (ret == ERROR_SUCCESS)   // "PassThruSupportXXX"
			{
				// Search for library data in all sub-keys (recursive)
				PTlibraries = searchLibValuesRecursive(hKey2, PTlibraries);
				ret = RegCloseKey(hKey2);
#ifdef __J2534_API_DEBUG__
				if (ret != ERROR_SUCCESS)
					std::cout << "J2534interface::getAvailableJ2534Libs():   RegCloseKey(hKey2) failed with error " << ret << "\n";
#endif
			}
#ifdef __J2534_API_DEBUG__
			else
				std::cout << "J2534interface::getAvailableJ2534Libs():   RegOpenKexEx(...) for key " << KeyName << " failed with error " << ret << "\n";
#endif
		}
		index++;
	}
	ret = RegCloseKey(hKey1);
#ifdef __J2534_API_DEBUG__
	if (ret != ERROR_SUCCESS)
		std::cout << "J2534interface::getAvailableJ2534Libs():   RegCloseKey(hKey1) failed with error " << ret << "\n";
	J2534misc::printLibraryInfo(PTlibraries);
#endif
	return PTlibraries;
}


std::vector<J2534Library> J2534_API::searchLibValuesRecursive(HKEY hKey, std::vector<J2534Library> PTlibs)
{
	HKEY hKey2;
	DWORD index = 0;
	char KeyName[256] = "";
	J2534Library PTlib;
	PTlib.api = J2534_API_version::v0404;
	char ValueName[256] = "";
	unsigned long szValueName = 256;// variable that specifies the size (in characters, including the terminating null char) of the buffer pointed to by the "ValueName" parameter.
	unsigned char Data[256] = "";	// buffer that receives the data for the value entry. This parameter can be NULL if the data is not required
	unsigned long szData = 256;	// variable that specifies the size, in bytes, of the buffer pointed to by the lpData parameter.
	long ret = 0;
	unsigned long ValueDataType = REG_NONE;
	// Check values :
	while ((RegEnumValueA(hKey, index, ValueName, &szValueName, NULL, &ValueDataType, Data, &szData)) != ERROR_NO_MORE_ITEMS)
	{
		if (ValueDataType == REG_SZ)
		{
			if (!strncmp(ValueName,"FunctionLibrary",15))
			{
				PTlib.path = (char*)(Data);
			}
			else if (!strncmp(ValueName,"Name",4))
			{
				PTlib.name = (char*)(Data);
			}
			else if (!strncmp(ValueName,"ProtocolsSupported",18))	// 02.02-API
			{
				PTlib.api = J2534_API_version::v0202;
				std::string protocol_str = (char*)(Data);
				// TODO split string, then use loop using existing parse function instead
				if (protocol_str.find("J1850VPW") != std::string::npos)
					PTlib.protocols = PTlib.protocols | J2534_protocol_flags::j1850vpw;
				if (protocol_str.find("J1850PWM") != std::string::npos)
					PTlib.protocols = PTlib.protocols | J2534_protocol_flags::j1850pwm;
				if (protocol_str.find("ISO9141") != std::string::npos)
					PTlib.protocols = PTlib.protocols | J2534_protocol_flags::iso9141;
				if (protocol_str.find("ISO14230") != std::string::npos)
					PTlib.protocols = PTlib.protocols | J2534_protocol_flags::iso14230;
				if (protocol_str.find("ISO15765") != std::string::npos)
					PTlib.protocols = PTlib.protocols | J2534_protocol_flags::iso15765;
				if (protocol_str.find("CAN") != std::string::npos)
					PTlib.protocols = PTlib.protocols | J2534_protocol_flags::can;
				if (protocol_str.find("SCI_A_ENGINE") != std::string::npos)
					PTlib.protocols = PTlib.protocols | J2534_protocol_flags::sci_a_engine;
				if (protocol_str.find("SCI_A_TRANS") != std::string::npos)
					PTlib.protocols = PTlib.protocols | J2534_protocol_flags::sci_a_trans;
				if (protocol_str.find("SCI_B_ENGINE") != std::string::npos)
					PTlib.protocols = PTlib.protocols | J2534_protocol_flags::sci_b_engine;
				if (protocol_str.find("SCI_B_TRANS") != std::string::npos)
					PTlib.protocols = PTlib.protocols | J2534_protocol_flags::sci_b_trans;
			}
		}
		else if (ValueDataType == REG_DWORD)	// 04.04-API
		{
			PTlib.api = J2534_API_version::v0404;
			DWORD key_value = (DWORD)(*Data);
			if (key_value)
			{
				std::string protocol_str = (char*)(ValueName);
				PTlib.protocols = PTlib.protocols | J2534misc::parseProtocol(protocol_str);
			}
		}
		szValueName = 256;	// because RegEnumValue has changed value !
		szData = 256;		// because RegEnumValue has changed value !
		index++;
	}
	if (PTlib.path.size() > 0)
		PTlibs.push_back( PTlib );
	// Check sub-keys:
	index = 0;
	while (RegEnumKeyA(hKey, index, KeyName, 256) != ERROR_NO_MORE_ITEMS)
	{
		ret = RegOpenKeyExA(hKey, KeyName, 0, KEY_READ, &hKey2);
		if (ret == ERROR_SUCCESS)
		{
			PTlibs = searchLibValuesRecursive(hKey2, PTlibs);
			ret = RegCloseKey(hKey2);
#ifdef __J2534_API_DEBUG__
			if (ret != ERROR_SUCCESS)
				std::cout << "J2534interface::searchLibValuesRecursive():   RegCloseKey(...) failed with error " << ret << "\n";
#endif
		}
#ifdef __J2534_API_DEBUG__
		else
		{
			std::cout << "J2534interface::getAvailableJ2534Libs():   RegOpenKexEx(...) for key " << KeyName << " failed with error " << ret << "\n";
		}
#endif
		index++;
	}
	return PTlibs;
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


long J2534_API::PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, void *pInput, void *pOutput)
{
	if (!_J2534LIB) return J2534API_ERROR_INVALID_LIBRARY;
	if (!_PassThruIoctl) return J2534API_ERROR_FCN_NOT_SUPPORTED;
	return _PassThruIoctl(ChannelID, IoctlID, pInput, pOutput);
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
