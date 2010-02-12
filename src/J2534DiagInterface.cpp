/*
 * J2534DiagInterface.cpp - J2534-pass-through diagnostic interface
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

#include "J2534DiagInterface.h"


J2534DiagInterface::J2534DiagInterface()
{
	_j2534 = NULL;
	_connected = false;
	_DeviceID = 0;
	_ChannelID = 0;
	_FilterID = 0;
	setName("J2534 Pass-Through");
	setVersion("");
}


J2534DiagInterface::~J2534DiagInterface()
{
	disconnect();
	close();
}


AbstractDiagInterface::interface_type J2534DiagInterface::interfaceType()
{
	return interface_J2534;
}


bool J2534DiagInterface::open( std::string name )
{
	if (_j2534)
		return false;
	else
	{
		_j2534 = new J2534_API;
		if (_j2534->selectLibrary(name))
		{
			char FirmwareVersion[80] = {0,};
			char DllVersion[80] = {0,};
			char ApiVersion[80] = {0,};
			long ret = 0;
			// Open interface (only 0404-API):
			if (_j2534->libraryAPIversion() != J2534_API_v0202)
			{
				_DeviceID = 0;
				ret = _j2534->PassThruOpen(NULL, &_DeviceID);
				if (STATUS_NOERROR != ret)
				{
#ifdef __FSSM_DEBUG__
					printErrorDescription("PassThruOpen() failed: ", ret);
#endif
					delete _j2534;
					_j2534 = NULL;
					return false;
				}
			}
			// Read hardware/software information:
			if (_j2534->libraryAPIversion() == J2534_API_v0202)
				ret = _j2534->PassThruReadVersion(FirmwareVersion, DllVersion, ApiVersion);
			else
				ret = _j2534->PassThruReadVersion(_DeviceID, FirmwareVersion, DllVersion, ApiVersion);
			if (STATUS_NOERROR == ret)
			{
				setVersion(std::string(FirmwareVersion) + " (API-library: " + std::string(DllVersion) +")");
#ifdef __FSSM_DEBUG__
				std::cout << "Interface information:\n";
				std::cout << "   Firmware version: " << FirmwareVersion << '\n';
				std::cout << "   DLL version:      " << DllVersion << '\n';
				std::cout << "   API version:      " << ApiVersion << '\n';
#endif
			}
#ifdef __FSSM_DEBUG__
			else
				printErrorDescription("PassThruReadVersion() failed: ", ret);
#endif
			return true;
		}
		else
		{
#ifdef __FSSM_DEBUG__
			std::cout << "Error: invalid library selected\n";
#endif
			delete _j2534;
			_j2534 = NULL;
			return false;
		}
	}
}


bool J2534DiagInterface::isOpen()
{
	return _j2534;
}


bool J2534DiagInterface::close()
{
	if (_j2534)
	{
		long ret = 0;
		if (_connected)
			disconnect();
		// Close interface (only 0404-API):
		if (_j2534->libraryAPIversion() != J2534_API_v0202)
		{
			ret = _j2534->PassThruClose(_DeviceID);
			if (STATUS_NOERROR != ret)
			{
#ifdef __FSSM_DEBUG__
				printErrorDescription("PassThruClose() failed: ", ret);
#endif
				return false;
			}
		}
		// Clean up:
		delete _j2534;
		_j2534 = NULL;
		setVersion("");
		return true;
	}
	else
		return false;
}


bool J2534DiagInterface::connect(AbstractDiagInterface::protocol_type protocol)
{
	if (protocol != AbstractDiagInterface::protocol_SSM2)
	{
#ifdef __FSSM_DEBUG__
		std::cout << "Error: selected protocol is not supported\n";
#endif
		return false;
	}
	if (_j2534)
	{
		long ret = 0;
		// Connect channel:
		if (_j2534->libraryAPIversion() == J2534_API_v0202)
			ret = _j2534->PassThruConnect(ISO9141, ISO9141_NO_CHECKSUM, &_ChannelID);
		else
			ret = _j2534->PassThruConnect(_DeviceID, ISO9141, ISO9141_NO_CHECKSUM, 4800, &_ChannelID);
		if (STATUS_NOERROR != ret)
		{
#ifdef __FSSM_DEBUG__
			printErrorDescription("PassThruConnect() failed: ", ret);
#endif
			return false;
		}
		// Set Timings:
		SCONFIG_LIST Input;
		SCONFIG CfgItems[8];
		CfgItems[0].Parameter = LOOPBACK;   // Echo off/on
		CfgItems[0].Value = ON;
		CfgItems[1].Parameter = P1_MAX;     // max. ECU inter-byte time [ms]
		CfgItems[1].Value = 2;
		CfgItems[2].Parameter = P2_MAX;     // max. ECU response time [ms] to a tester request or between ECU responses
		CfgItems[2].Value = 10;  // laut Norm min. 0ms ODER 25ms (abhaengig von Synchronisierungsart)
		CfgItems[3].Parameter = P3_MIN;     // min. time [ms] between end of ECU reponse and next tester request
		CfgItems[3].Value = 10;  // laut Norm min. 55ms !
		CfgItems[4].Parameter = P4_MIN;     // min. tester inter-byte time [ms]
		CfgItems[4].Value = 0;  // laut Norm min. 5ms !
		CfgItems[5].Parameter = DATA_RATE;	// only needed for 0202-API
		CfgItems[5].Value = 4800;		// only needed for 0202-API
		CfgItems[6].Parameter = DATA_BITS;
		CfgItems[6].Value = 8;
		CfgItems[7].Parameter = PARITY;
		CfgItems[7].Value = NO_PARITY;
		Input.NumOfParams = 8;
		Input.ConfigPtr = CfgItems;
		ret = _j2534->PassThruIoctl(_ChannelID, SET_CONFIG, (void *)&Input, (void *)NULL);
		if (STATUS_NOERROR != ret)
		{
#ifdef __FSSM_DEBUG__
			printErrorDescription("PassThruIoctl() failed: ", ret);
#endif
			goto err_close;
		}
		// Apply Filter (Recieve all messages) => MANDATORY
		PASSTHRU_MSG MaskMsg;
		PASSTHRU_MSG PatternMsg;
		memset(&MaskMsg, 0, sizeof(MaskMsg));   // .Data=0-array means "do not examine any bits"
		memset(&PatternMsg, 0, sizeof(PatternMsg)); // .Data must be zero, if no Data bits are examined
		MaskMsg.ProtocolID = ISO9141;
		PatternMsg.ProtocolID = ISO9141;
		ret = _j2534->PassThruStartMsgFilter(_ChannelID, PASS_FILTER, &MaskMsg, &PatternMsg, NULL, &_FilterID);
		if (STATUS_NOERROR != ret)
		{
#ifdef __FSSM_DEBUG__
			printErrorDescription("PassThruStartMsgFilter() failed: ", ret);
#endif
			goto err_close;
		}
		_connected = true;
		return true;
	}
	else
		return false;

err_close:
	long ret = 0;
	ret = _j2534->PassThruDisconnect(_ChannelID);
#ifdef __FSSM_DEBUG__
	if (STATUS_NOERROR != ret)
		printErrorDescription("PassThruDisconnect() failed: ", ret);
#endif
	return false;
}


bool J2534DiagInterface::isConnected()
{
	return (_j2534 && _connected);
}


bool J2534DiagInterface::disconnect()
{
	if (_j2534)
	{
		long ret = 0;
		// Remove filter:
		ret = _j2534->PassThruStopMsgFilter(_ChannelID, _FilterID);
		if (STATUS_NOERROR != ret)
#ifdef __FSSM_DEBUG__
			printErrorDescription("PassThruStopMsgFilter() failed: ", ret);
#endif
		// Disconnect channel:
		ret = _j2534->PassThruDisconnect(_ChannelID);
		if (STATUS_NOERROR != ret)
		{
#ifdef __FSSM_DEBUG__
			printErrorDescription("PassThruDisconnect() failed: ", ret);
#endif
			return false;
		}
		_connected = false;
		return true;
	}
	else
		return false;
}


bool J2534DiagInterface::read(std::vector<char> *buffer)
{
	if (_j2534 && _connected)
	{
		long ret = 0;
		// Setup message-container:
		PASSTHRU_MSG rx_msg;
		memset(&rx_msg, 0, sizeof(rx_msg));
		rx_msg.ProtocolID = ISO9141;
		unsigned long rxNumMsgs = 1;
		unsigned long timeout = 0;	// return immediately
		// Read message:
		do
		{
			ret = _j2534->PassThruReadMsgs(_ChannelID, &rx_msg, &rxNumMsgs, timeout);
		} while ((STATUS_NOERROR == ret) && !rxNumMsgs);
		if (STATUS_NOERROR == ret/* || ERR_TIMEOUT == ret*/)
		{
			// Extract data:
			buffer->clear();
			for (unsigned int k=0; k<rx_msg.DataSize; k++)
				buffer->push_back(rx_msg.Data[k]);
			return true;
		}
		else if (ERR_BUFFER_EMPTY == ret)
		{
			buffer->clear();
			return true;
		}
		else
		{
#ifdef __FSSM_DEBUG__
			printErrorDescription("PassThruReadMsgs() failed: ", ret);
#endif
			return false;
		}
	}
	else
		return false;

}


bool J2534DiagInterface::write(std::vector<char> buffer)
{
	if (_j2534 && _connected)
	{
		long ret = 0;
		// Setup message:
		PASSTHRU_MSG tx_msg;
		memset(&tx_msg, 0, sizeof(tx_msg));
		tx_msg.ProtocolID = ISO9141;
		for (unsigned int k=0; k<buffer.size(); k++)
			tx_msg.Data[k] = buffer[k];
		tx_msg.DataSize = buffer.size();
		unsigned long txNumMsgs = 1;
		unsigned long timeout = 1000;
		// Send message:
		ret = _j2534->PassThruWriteMsgs(_ChannelID, &tx_msg, &txNumMsgs, timeout);
		if (STATUS_NOERROR == ret)
			return true;
		else
		{
#ifdef __FSSM_DEBUG__
			printErrorDescription("PassThruWriteMsgs() failed: ", ret);
#endif
			return false;
		}
	}
	else
		return false;
}


// Private

#ifdef __FSSM_DEBUG__
void J2534DiagInterface::printErrorDescription(std::string title, long ret)
{
	char ErrorDescription[80] = {0,};
	if (ret > 0)
	{
		if (STATUS_NOERROR == _j2534->PassThruGetLastError(ErrorDescription))
			std::cout << title << ErrorDescription << '\n';
		else
			std::cout << "unknown error (PassThruGetLastError() failed !)\n";
	}
	else if (ret == J2534API_ERROR_FCN_NOT_SUPPORTED)
	{
		std::cout << "the library does not support this function\n";
	}
	else if (ret == J2534API_ERROR_INVALID_LIBRARY)
	{
		std::cout << "invalid library selected\n";
	}
	else
	{
		std::cout << "unknown error\n";
	}		
}
#endif

