/*
 * J2534_API.cpp - API for accessing SAE-J2534 compliant interfaces
 *
 * Copyright (C) 2009-2019 Comer352L
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
#include "tinyxml2/tinyxml2.h"



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
		// On success, dlclose() returns 0; on error, it returns a nonzero value.
		if (dlclose( _J2534LIB ))
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
			std::cout << "J2534interface::selectLibrary(): Error: the library doesn't provide the PassThruConnect(), and/or PassThruDisconnect() methods !\n";
			if (dlclose( newJ2534LIB ))
				std::cout << "J2534interface::selectLibrary(): dlclose() failed with error " << dlerror() << "\n";
#else
			dlclose( newJ2534LIB );
#endif
			return false;
		}
		// Check API-version of the library:
		if (!dlsym( newJ2534LIB, "PassThruOpen" ) || !dlsym( newJ2534LIB, "PassThruClose" ))
			_api_version = J2534_API_version::v0202;
		else
			_api_version = J2534_API_version::v0404;
		// Close old library:
		if (_J2534LIB)
		{
#ifdef __J2534_API_DEBUG__
			if (dlclose( _J2534LIB ))
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
	if (_api_version == J2534_API_version::v0202)
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
	// HACK
	const std::string libsDefXmlFile = "./definitions/J2534libs.xml";

	const std::vector<J2534Library> PTlibraries = parseJ2534LibsXml(libsDefXmlFile);

#ifdef __J2534_API_DEBUG__
	J2534misc::printLibraryInfo(PTlibraries);
#endif
	return PTlibraries;
}


std::vector<J2534Library> J2534_API::parseJ2534LibsXml(const std::string& libsDefXmlFile)
{
	std::vector<J2534Library> PTlibraries;

	tinyxml2::XMLDocument xmldoc;
#ifdef __J2534_API_DEBUG__
	std::cout << "Trying to load J2534 library definitions XML file: \"" << libsDefXmlFile << "\"\n";
#endif
	if (xmldoc.LoadFile(libsDefXmlFile.c_str()) == tinyxml2::XML_SUCCESS)
	{
#ifdef __J2534_API_DEBUG__
		std::cout << "Parsing J2534 library definitions XML data...\n";
#endif
		tinyxml2::XMLElement* rootelement = xmldoc.FirstChildElement("J2534LIBS");
		if (!rootelement)
			goto error;

		tinyxml2::XMLElement* libelement = rootelement->FirstChildElement("J2534LIB");
		while (libelement)
		{
			J2534Library lib;
			std::string api_str;

			const tinyxml2::XMLAttribute* attrib = libelement->FirstAttribute();
			while (attrib)
			{
				if (std::string(attrib->Name()) == "name")
				{
					if (lib.name.empty())
						lib.name = attrib->Value();
					else // attribute specified multiple times
						goto error;
				}
				else if (std::string(attrib->Name()) == "path")
				{
					if (lib.path.empty())
						lib.path = attrib->Value();
					else // attribute specified multiple times
						goto error;
				}
				else if (std::string(attrib->Name()) == "api")
				{
					if (api_str.empty())
						api_str = attrib->Value();
					else // attribute specified multiple times
						goto error;
				}
				attrib=attrib->Next();
			}

			if (lib.name.empty())
			{
				std::cout << "J2534LIB: attribute 'name' missing!\n";
				goto error;
			}
			if (lib.path.empty())
			{
				std::cout << "J2534LIB: attribute 'path' missing!\n";
				goto error;
			}
			if (api_str.empty())
			{
				std::cout << "J2534LIB: attribute 'api' missing!\n";
				goto error;
			}
			lib.api = J2534misc::parseApiVersion(api_str);
			if (lib.api == J2534_API_version::undefined) {
				std::cout << "J2534LIB: unknown 'api': \"" << api_str << "\", assuming \"" << J2534misc::apiVersionToStr(J2534_API_version::v0404) << "\"\n";
				lib.api = J2534_API_version::v0404;
			}

			tinyxml2::XMLElement* protocolselement = libelement->FirstChildElement("PROTOCOLS");
			if (!protocolselement)
			{
				std::cout << "J2534LIB: element 'PROTOCOLS' missing!\n";
				goto error;
			}

			tinyxml2::XMLElement* protocolelement = protocolselement->FirstChildElement("PROTOCOL");
			while (protocolelement)
			{
				std::string protocolstr(protocolelement->GetText());
				lib.protocols = lib.protocols | J2534misc::parseProtocol(protocolstr);
				protocolelement = protocolelement->NextSiblingElement();
			}

			PTlibraries.push_back(lib);
			libelement = libelement->NextSiblingElement("J2534LIB");
		}
	}
	else
		std::cout << "J2534 library definitions XML file not found: \"" << libsDefXmlFile << "\"\n";

error:
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
