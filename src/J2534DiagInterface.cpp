/*
 * J2534DiagInterface.cpp - J2534-pass-through diagnostic interface
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

#include "J2534DiagInterface.h"


J2534DiagInterface::J2534DiagInterface()
{
	_j2534 = NULL;
	_connected = false;
	_DeviceID = 0;
	_ChannelID = 0;
	_numFilters = 0;
	setName("J2534 Pass-Through");
	setVersion("");
	setProtocolBaudrate( 0 );
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
				setVersion(std::string(FirmwareVersion) + " (DLL: " + std::string(DllVersion) + ", API: " + std::string(ApiVersion) +")");
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
			// Get and save library data:
			std::vector<J2534Library> libs = J2534_API::getAvailableJ2534Libs();
			for (unsigned int k=0; k<libs.size(); k++)
			{
				if (libs.at(k).path == name)
				{
					// Interface name
					setName(libs.at(k).name);
					// Supported protocols
					std::vector<protocol_type> supportedProtocols;
					if ((libs.at(k).protocols & PROTOCOL_FLAG_ISO9141) ||
					    (libs.at(k).protocols & PROTOCOL_FLAG_ISO14230)   )
						supportedProtocols.push_back(protocol_SSM2_ISO14230);
					if (libs.at(k).protocols & PROTOCOL_FLAG_ISO15765)
						supportedProtocols.push_back(protocol_SSM2_ISO15765);
					setSupportedProtocols(supportedProtocols);
					break;
				}
			}
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
		setName("J2534 Pass-Through");
		setVersion("");
		return true;
	}
	else
		return false;
}


bool J2534DiagInterface::connect(AbstractDiagInterface::protocol_type protocol)
{
	if ((protocol != AbstractDiagInterface::protocol_SSM2_ISO14230) && (protocol != AbstractDiagInterface::protocol_SSM2_ISO15765))
	{
#ifdef __FSSM_DEBUG__
		std::cout << "Error: selected protocol is not supported\n";
#endif
		return false;
	}
	if (_j2534)
	{
		unsigned long ProtocolID = 0;
		unsigned long Flags = 0;
		unsigned long BaudRate = 0;
		long ret = 0;
		if (protocol == AbstractDiagInterface::protocol_SSM2_ISO14230)
		{
			ProtocolID = ISO9141; // also: ISO14230
			Flags = ISO9141_NO_CHECKSUM;
			BaudRate = 4800;
		}
		else if (protocol == AbstractDiagInterface::protocol_SSM2_ISO15765)
		{
			ProtocolID = ISO15765;
			Flags = 0;
			BaudRate = 500000;
		}
		// CONNECT CHANNEL:
		if (_j2534->libraryAPIversion() == J2534_API_v0202)
			ret = _j2534->PassThruConnect(ProtocolID, Flags, &_ChannelID);
		else
			ret = _j2534->PassThruConnect(_DeviceID, ProtocolID, Flags, BaudRate, &_ChannelID);
		if (STATUS_NOERROR != ret)
		{
#ifdef __FSSM_DEBUG__
			printErrorDescription("PassThruConnect() failed: ", ret);
#endif
			return false;
		}
		/* ----- SET CONFIGURATION ----- */
		SCONFIG_LIST Input;
		SCONFIG CfgItems[1];
		// Echo (MANDATORY):
		CfgItems[0].Parameter = LOOPBACK;   // Echo off/on
		if (protocol == AbstractDiagInterface::protocol_SSM2_ISO14230)
			CfgItems[0].Value = ON;
		else
			CfgItems[0].Value = OFF;
		Input.NumOfParams = 1;
		Input.ConfigPtr = CfgItems;
		ret = _j2534->PassThruIoctl(_ChannelID, SET_CONFIG, (void *)&Input, (void *)NULL);
		if (STATUS_NOERROR != ret)
		{
#ifdef __FSSM_DEBUG__
			printErrorDescription("PassThruIoctl() for parameter LOOPBACK failed: ", ret);
#endif
			goto err_close;
		}
		// Baudrate (MANDATORY for 02.02-API only):
		CfgItems[0].Parameter = DATA_RATE;
		CfgItems[0].Value = BaudRate;
		Input.NumOfParams = 1;
		Input.ConfigPtr = CfgItems;
		ret = _j2534->PassThruIoctl(_ChannelID, SET_CONFIG, (void *)&Input, (void *)NULL);
		if (STATUS_NOERROR != ret)
		{
#ifdef __FSSM_DEBUG__
			printErrorDescription("PassThruIoctl() for parameter DATARATE failed: ", ret);
#endif
			if (_j2534->libraryAPIversion() == J2534_API_v0202)
				goto err_close;
		}
		if (protocol == AbstractDiagInterface::protocol_SSM2_ISO14230)
		{
			/* ----- SET CONFIGURATION (ISO14230 specific) ----- */
			// P1_MAX
			CfgItems[0].Parameter = P1_MAX;     // max. ECU inter-byte time [ms]
			CfgItems[0].Value = 2;   // ISO-9141, ISO-14230 (normal timing paramter-set): 20ms
			Input.NumOfParams = 1;
			Input.ConfigPtr = CfgItems;
			ret = _j2534->PassThruIoctl(_ChannelID, SET_CONFIG, (void *)&Input, (void *)NULL);
#ifdef __FSSM_DEBUG__
			if (STATUS_NOERROR != ret)
				printErrorDescription("PassThruIoctl() for P1_MAX: ", ret);
#endif
			// P2_MAX
			/* NOTE:
			* According to ISO-9141+ISO-14230, a tester should return with a timeout error,
			* if no message is recieved within this period.
			* The ISO-9141+ISO-14230 standard value (normal timing paramter-set) is 50ms,
			* which is MUCH smaller than the value used for the SSM2 protocol (some seconds !).
			* 
			* It is nevertheless likely that testers will NOT timeout if this value is to small,
			* but respect the timeout parameter passed to PassThruReadMsgs() instead.
			* This behavior has been confirmed for the Tactrix OpenPort 2.0 interface, 
			* which doesn't support setting the P2_MAX value.
			*/
			CfgItems[0].Parameter = P2_MAX;     // max. ECU response time [ms] to a tester request or between ECU responses
			CfgItems[0].Value = 3000;  // ISO-9141, ISO-14230 (normal timing paramter-set): 50ms
			Input.NumOfParams = 1;
			Input.ConfigPtr = CfgItems;
			ret = _j2534->PassThruIoctl(_ChannelID, SET_CONFIG, (void *)&Input, (void *)NULL);
#ifdef __FSSM_DEBUG__
			if (STATUS_NOERROR != ret)
				printErrorDescription("PassThruIoctl() for P2_MAX: ", ret);
#endif
			// P3_MIN
			CfgItems[0].Parameter = P3_MIN;     // min. time [ms] between end of ECU reponse and next tester request
			CfgItems[0].Value = 10;  // ISO-9141, ISO-14230: default 55ms
			Input.NumOfParams = 1;
			Input.ConfigPtr = CfgItems;
			ret = _j2534->PassThruIoctl(_ChannelID, SET_CONFIG, (void *)&Input, (void *)NULL);
#ifdef __FSSM_DEBUG__
			if (STATUS_NOERROR != ret)
				printErrorDescription("PassThruIoctl() for P3_MIN: ", ret);
#endif
			// P4_MIN
			CfgItems[0].Parameter = P4_MIN;     // min. tester inter-byte time [ms]
			CfgItems[0].Value = 0;   // ISO-9141, ISO-14230: default 5ms
			Input.NumOfParams = 1;
			Input.ConfigPtr = CfgItems;
			ret = _j2534->PassThruIoctl(_ChannelID, SET_CONFIG, (void *)&Input, (void *)NULL);
#ifdef __FSSM_DEBUG__
			if (STATUS_NOERROR != ret)
				printErrorDescription("PassThruIoctl() for P4_MIN: ", ret);
#endif
			// NOTE: timing parameters P1_MIN, P2_MIN, P3_MAX, P4_MAX are not adjustable
			// Data bits:
			if (_j2534->libraryAPIversion() == J2534_API_v0404)
			{
				CfgItems[0].Parameter = DATA_BITS;
				CfgItems[0].Value = 8;	// should be default
				Input.NumOfParams = 1;
				Input.ConfigPtr = CfgItems;
				ret = _j2534->PassThruIoctl(_ChannelID, SET_CONFIG, (void *)&Input, (void *)NULL);
#ifdef __FSSM_DEBUG__
				if (STATUS_NOERROR != ret)
					printErrorDescription("PassThruIoctl() for parameter DATA_BITS failed: ", ret);
#endif
			}
			// Parity:
			CfgItems[0].Parameter = PARITY;
			CfgItems[0].Value = NO_PARITY;
			Input.NumOfParams = 1;	// should be default
			Input.ConfigPtr = CfgItems;
			ret = _j2534->PassThruIoctl(_ChannelID, SET_CONFIG, (void *)&Input, (void *)NULL);
#ifdef __FSSM_DEBUG__
			if (STATUS_NOERROR != ret)
				printErrorDescription("PassThruIoctl() for parameter PARITY failed: ", ret);
#endif
			/* APPLY MESSAGE FILTER (Receive all messages) */
			PASSTHRU_MSG MaskMsg;
			PASSTHRU_MSG PatternMsg;
			memset(&MaskMsg, 0, sizeof(MaskMsg));		// .Data=0-array means "do not examine any bits"
			memset(&PatternMsg, 0, sizeof(PatternMsg));	// .Data must be zero, if no Data bits are examined
			MaskMsg.DataSize = 1;
			MaskMsg.ProtocolID = ISO9141;	
			PatternMsg.DataSize = 1;
			PatternMsg.ProtocolID = ISO9141;
			if (STATUS_NOERROR != _j2534->PassThruStartMsgFilter(_ChannelID, PASS_FILTER, &MaskMsg, &PatternMsg, NULL, _FilterID))
			{
#ifdef __FSSM_DEBUG__
				printErrorDescription("PassThruStartMsgFilter() for ISO-14230 failed: ", ret);
#endif
				goto err_close;
			}
			_numFilters = 1;
		}
		else if (protocol == AbstractDiagInterface::protocol_SSM2_ISO15765)
		{
			// NOTE: also tweak values of ISO15765_BS, ISO15765_STMIN ?
			/* APPLY MESSAGE FILTERS */
			PASSTHRU_MSG MaskMsg;
			PASSTHRU_MSG PatternMsg;
			PASSTHRU_MSG FlowCtrlMsg;
			memset(&MaskMsg, 0, sizeof(MaskMsg));
			memset(&PatternMsg, 0, sizeof(PatternMsg));
			memset(&FlowCtrlMsg, 0, sizeof(FlowCtrlMsg));
			// ECU:
			MaskMsg.Data[0] = '\xFF';
			MaskMsg.Data[1] = '\xFF';
			MaskMsg.Data[2] = '\xFF';
			MaskMsg.Data[3] = '\xFF';
			MaskMsg.DataSize = 4;
			MaskMsg.ProtocolID = ISO15765;
			MaskMsg.TxFlags = ISO15765_FRAME_PAD;
			PatternMsg.Data[0] =  '\x00';
			PatternMsg.Data[1] =  '\x00';
			PatternMsg.Data[2] =  '\x07';
			PatternMsg.Data[3] =  '\xE8';
			PatternMsg.DataSize = 4;
			PatternMsg.ProtocolID = ISO15765;
			PatternMsg.TxFlags = ISO15765_FRAME_PAD;
			FlowCtrlMsg.Data[0] = '\x00';
			FlowCtrlMsg.Data[1] = '\x00';
			FlowCtrlMsg.Data[2] = '\x07';
			FlowCtrlMsg.Data[3] = '\xE0';
			FlowCtrlMsg.DataSize = 4;
			FlowCtrlMsg.ProtocolID = ISO15765;
			FlowCtrlMsg.TxFlags = ISO15765_FRAME_PAD;
			if (STATUS_NOERROR != _j2534->PassThruStartMsgFilter(_ChannelID, FLOW_CONTROL_FILTER, &MaskMsg, &PatternMsg, &FlowCtrlMsg, _FilterID + _numFilters))
			{
#ifdef __FSSM_DEBUG__
				printErrorDescription("PassThruStartMsgFilter() #1 for ISO-15765 failed: ", ret);
#endif
				goto err_close;
			}
			_numFilters = 1;
			// TCU:
			PatternMsg.Data[3] =  '\xE9';
			FlowCtrlMsg.Data[3] = '\xE1';
			if (STATUS_NOERROR != _j2534->PassThruStartMsgFilter(_ChannelID, FLOW_CONTROL_FILTER, &MaskMsg, &PatternMsg, &FlowCtrlMsg, _FilterID + _numFilters))
			{
#ifdef __FSSM_DEBUG__
				printErrorDescription("PassThruStartMsgFilter() #2 for ISO-15765 failed: ", ret);
#endif
				// Clean up configured message filter
				ret = _j2534->PassThruStopMsgFilter(_ChannelID, _FilterID[0]);
#ifdef __FSSM_DEBUG__
				if (STATUS_NOERROR != ret)
					printErrorDescription("PassThruStopMsgFilter() for ISO-15765 failed: ", ret);
#endif
				_numFilters = 0;
				goto err_close;
			}
			_numFilters = 2;
			// TODO: add support for additional addresses (requires layer/API changes)
		}
		_connected = true;
		setProtocolType( protocol );
		setProtocolBaudrate( BaudRate );
		return true;
	}
	else
		return false;

err_close:
#ifdef __FSSM_DEBUG__
	long ret = _j2534->PassThruDisconnect(_ChannelID);
	if (STATUS_NOERROR != ret)
		printErrorDescription("PassThruDisconnect() failed: ", ret);
#else
	_j2534->PassThruDisconnect(_ChannelID);
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
		// Remove filters:
		for (unsigned char k=0; k<_numFilters; k++)
		{
			ret = _j2534->PassThruStopMsgFilter(_ChannelID, _FilterID[k]);
#ifdef __FSSM_DEBUG__
			if (STATUS_NOERROR != ret)
				printErrorDescription("PassThruStopMsgFilter() failed: ", ret);
#endif
		}
		_numFilters = 0;
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
		setProtocolType( protocol_NONE );
		setProtocolBaudrate( 0 );
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
		if (protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230)
		{
			rx_msg.ProtocolID = ISO9141;
		}
		else if (protocolType() == AbstractDiagInterface::protocol_SSM2_ISO15765)
		{
			rx_msg.ProtocolID = ISO15765;
		}
		else
			return false;
		buffer->clear();
		unsigned long rxNumMsgs = 1;
		unsigned long timeout = 0;	// return immediately
		// Read message:
		do
		{
			ret = _j2534->PassThruReadMsgs(_ChannelID, &rx_msg, &rxNumMsgs, timeout);
			if ((STATUS_NOERROR == ret) && rxNumMsgs)
			{
				if (rx_msg.RxStatus & TX_MSG_TYPE)
				{
					if (rx_msg.RxStatus & TX_DONE)	// SAE J2534-1 (dec 2004): ISO-15765 only
					{
#ifdef __FSSM_DEBUG__
						std::cout << "PassThruReadMsgs(): received transmit confirmation message\n";
#endif
						continue;
					}
#ifdef __FSSM_DEBUG__
					else
						std::cout << "PassThruReadMsgs(): received loopback message\n";
#endif
				}
				else if (rx_msg.RxStatus & START_OF_MESSAGE)
				{
					/* NOTE:
					 * - incoming (multi-frame) msg transfer has commenced (ISO-15765) /
					 *   first byte of an incoming message has been received (ISO-9141 / ISO-14230)
					 * - ISO-15765: message contains CAN-ID only                                   */
#ifdef __FSSM_DEBUG__
					std::cout << "PassThruReadMsgs(): received indication message fo start of incoming message\n";
#endif
					continue;
				}
				else if ((protocolType() == protocol_SSM2_ISO15765) && (rx_msg.RxStatus & ISO15765_PADDING_ERROR))
				{
					// NOTE: ISO-15765 CAN frame was received with less than 8 data bytes
#ifdef __FSSM_DEBUG__
					std::cout << "PassThruReadMsgs(): received ISO-15765 padding error indication message.\n";
#endif
					continue;
				}
				// NOTE: all other flags do not affect the transferred data or are not defined for the ISO-protocols
				// Extract data:
				for (unsigned int k=0; k<rx_msg.DataSize; k++)
					buffer->push_back(rx_msg.Data[k]);
			}
		} while ((STATUS_NOERROR == ret) && rxNumMsgs && (protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230));
		/* NOTE: For SSM2 over ISO-14230, we read all received data;
		   Because of the "special" timings, we can't be sure that a received message corresponds to a single+complete SSM2 message.
		   Message detection/extraction is up to the upper layer */
		if ((STATUS_NOERROR == ret) || (ERR_BUFFER_EMPTY == ret))
			return true;
#ifdef __FSSM_DEBUG__
		else
		{
			printErrorDescription("PassThruReadMsgs() failed: ", ret);
		}
#endif
	}
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
		if (protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230)
		{ 
			tx_msg.ProtocolID = ISO9141;
		}
		else if (protocolType() == AbstractDiagInterface::protocol_SSM2_ISO15765)
		{
			tx_msg.ProtocolID = ISO15765;
			tx_msg.TxFlags = ISO15765_FRAME_PAD;
		}
		else
			return false;
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


bool J2534DiagInterface::clearSendBuffer()
{
	long ret = _j2534->PassThruIoctl(_ChannelID, CLEAR_TX_BUFFER, (void *)NULL, (void *)NULL);
	if (STATUS_NOERROR != ret)
	{
#ifdef __FSSM_DEBUG__
		printErrorDescription("PassThruIoctl() for parameter CLEAR_TX_BUFFER failed: ", ret);
#endif
		return false;
	}
	return true;
}


bool J2534DiagInterface::clearReceiveBuffer()
{
	long ret = _j2534->PassThruIoctl(_ChannelID, CLEAR_RX_BUFFER, (void *)NULL, (void *)NULL);
	if (STATUS_NOERROR != ret)
	{
#ifdef __FSSM_DEBUG__
		printErrorDescription("PassThruIoctl() for parameter CLEAR_RX_BUFFER failed: ", ret);
#endif
		return false;
	}
	return true;
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

