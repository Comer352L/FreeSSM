/*
 * SSMP2communication_core.cpp - Core functions (services) of the new SSM-protocol
 *
 * Copyright (C) 2008-2019 Comer352L
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


#include "SSMP2communication_core.h"


SSMP2communication_core::SSMP2communication_core(AbstractDiagInterface *diagInterface)
{
	_diagInterface = diagInterface;
}


bool SSMP2communication_core::ReadDataBlock(const unsigned int ecuaddr, const char padaddr, const unsigned int dataaddr, const unsigned int nrofbytes, char *data)
{
	if ((dataaddr > 0xffffff) || (nrofbytes == 0))
		return false;
	if ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230) && (nrofbytes > 254)) // ISO14230 protocol limit: length byte in header => max. 254 per reply message possible
	{
		return false;
	}
	else if ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO15765) && (nrofbytes > 256)) // ISO15765 protocol limit: data length byte in request => max. 256 possible
	{
		return false;
	}
	else
	{
		return false;
	}

	char indata[255] = {0,};
	unsigned char indatalen = 0;
	char querymsg[6] = {0,};
	// SETUP MESSAGE (without Header+Checksum):
	querymsg[0] = '\xA0';
	querymsg[1] = padaddr;
	libFSSM::setUInt24BigEndian(querymsg + 2, dataaddr);
	querymsg[5] = nrofbytes - 1;
	// SEND MESSAGE + RECEIVE ANSWER:
	if (SndRcvMessage(ecuaddr, querymsg, 6, indata, &indatalen))
	{
		// CHECK DATA:
		if (indatalen == (nrofbytes+1))
		{
			if (indata[0] == '\xE0')
			{
				// EXTRACT DATA:
				std::copy(indata + 1, indata + 1 + nrofbytes, data);
				return true;
			}
		}
	}
	return false;
}


bool SSMP2communication_core::ReadMultipleDatabytes(const unsigned int ecuaddr, const char padaddr, const unsigned int dataaddr[], const unsigned int datalen, char* data)
{
	if (datalen == 0)
		return false;
	if ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230) && (datalen > 84)) // ISO14230 protocol limit: length byte in header => max. (255-2)/3 = 84 addresses per request message possible
		return false;
	// NOTE: Control unit have (different) limits which are lower than the theoretical max. number of addresses per request !
	char indata[255] = {0,};
	unsigned char indatalen = 0;
	char querymsg[255] = {0,};
	// SETUP MESSAGE:
	querymsg[0] = '\xA8';
	querymsg[1] = padaddr;
	for (unsigned int k=0; k<datalen; k++) {
		libFSSM::setUInt24BigEndian(querymsg + 2 + k*3, dataaddr[k]);
	}
	// SEND MESSAGE + RECEIVE ANSWER:
	if (SndRcvMessage(ecuaddr, querymsg, (2+3*datalen), indata, &indatalen))
	{
		// CHECK DATA:
		if (indatalen == datalen+1 && indata[0] == '\xE8') {
			// EXTRACT DATA:
			std::copy(indata + 1, indata + 1 + datalen, data);
			return true;
		}
	}
	return false;
}


bool SSMP2communication_core::WriteDataBlock(const unsigned int ecuaddr, const unsigned int dataaddr, const char *data, const unsigned int datalen, char* datawritten)
{
	if ((dataaddr > 0xffffff) || (datalen == 0))
		return false;
	if ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230) && (datalen > 251)) // ISO14230 protocol limit: length byte => max. 255-4 = 251 data bytes per request message possible
		return false;
	char indata[252] = {0,};
	unsigned char indatalen = 0;
	char writemsg[255] = {0,};
	// SETUP MESSAGE:
	writemsg[0] = '\xB0';
	libFSSM::setUInt24BigEndian(writemsg + 1, dataaddr);
	std::copy(data, data + datalen, writemsg + 4);
	// SEND MESSAGE + RECEIVE ANSWER:
	if (SndRcvMessage(ecuaddr, writemsg, (4+datalen), indata, &indatalen))
	{
		// CHECK DATA:
		if (indatalen == (datalen+1))
		{
			if (indata[0] == '\xF0')
			{
				if (datawritten == NULL)
				{
					// CHECK IF ACTUALLY WRITTEN DATA IS EQUAL TO THE DATA SENT OUT:
					return !memcmp(data, indata + 1, datalen);
				}
				else
				{
					// EXTRACT AND RETURN WRITTEN DATA:
					std::copy(indata + 1, indata + 1 + datalen, datawritten);
					return true;
					// NOTE: NECESSARY FOR SOME OPERATIONS, WHERE THE ACTUALLY WRITTEN DATA IS DIFFERENT TO THE DATA SENT OUT
				}
			}
		}
	}
	return false;
}


bool SSMP2communication_core::WriteDatabyte(const unsigned int ecuaddr, const unsigned int dataaddr, const char databyte, char *databytewritten)
{
	if (dataaddr > 0xffffff) return false;
	char indata[2] = {0,};
	unsigned char indatalen = 0;
	char writemsg[5] = {0,};
	// SETUP MESSAGE (without Header+Checksum):
	writemsg[0] = '\xB8';
	libFSSM::setUInt24BigEndian(writemsg + 1, dataaddr);
	writemsg[4] = databyte;
	// SEND MESSAGE + RECEIVE ANSWER:
	if (SndRcvMessage(ecuaddr, writemsg, 5, indata, &indatalen))
	{
		// CHECK DATA:
		if (indatalen == 2)
		{
			if (indata[0] == '\xF8')
			{
				if (databytewritten == NULL)
				{
					// CHECK IF ACTUALLY WRITTEN DATA IS EQAUL TO THE DATA SENT OUT:
					if (indata[1] == databyte)
						return true;
					else
						return false;
				}
				else
				{
					// EXTRACT AND RETURN WRITTEN DATA:
					*databytewritten = indata[1];
					return true;
					// NOTE: NECESSARY FOR SOME OPERATIONS, WHERE THE ACTUALLY WRITTEN DATA IS DIFFERENT TO THE DATA SENT OUT
				}
			}
		}
	}
	return false;
}


bool SSMP2communication_core::GetCUdata(const unsigned int ecuaddr, char *cuData, unsigned char *cuDataSize)
{
	*cuDataSize = 0;
	char reqmsg = 0;
	// Request command byte
	switch(_diagInterface->protocolType()) {
	case AbstractDiagInterface::protocol_type::SSM2_ISO14230: reqmsg = '\xBF'; break;
	case AbstractDiagInterface::protocol_type::SSM2_ISO15765:	reqmsg = '\xAA'; break;
	default: return false;
	}

	// SEND MESSAGE + RECEIVE ANSWER:
	char indata[255] = {0,};
	unsigned char indatalen = 0;
	if (SndRcvMessage(ecuaddr, &reqmsg, 1, indata, &indatalen))
	{
		// CHECK MESSAGE LENGTH:
		// usual flagbytes sizes: 32, 48, 96
		// i.e. ResponseCommand[1] + SYSID[3] + ROMID[5] + flagbytes[96] = 105 bytes total
		if ((indatalen == 41) || (indatalen == 57) || (indatalen == 105))
		{
			// CHECK DATA:
			if (((_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230) && (indata[0] == '\xFF'))
				|| ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO15765) && (indata[0] == '\xEA')))
			{
				std::copy(indata + 1, indata + indatalen, cuData);
				*cuDataSize = indatalen - 1;
				return true;
			}
		}
	}
	return false;
}


bool SSMP2communication_core::SndRcvMessage(const unsigned int ecuaddr, const char *outdata, const unsigned char outdatalen, char *indata, unsigned char *indatalen)
{
	if (_diagInterface == NULL) return false;
	if (outdatalen < 1) return false;
	std::vector<char> msg_buffer;
	msg_buffer.reserve(outdatalen + 5);
	// SETUP COMPLETE MESSAGE:
	// Protocol-header
	switch(_diagInterface->protocolType()) {
	case AbstractDiagInterface::protocol_type::SSM2_ISO14230:
		if (ecuaddr > 0xff) return false;
		// header, 4 bytes
		msg_buffer.push_back('\x80');
		msg_buffer.push_back(ecuaddr);
		msg_buffer.push_back('\xF0');
		msg_buffer.push_back(static_cast<char>(outdatalen));
		break;
	case AbstractDiagInterface::protocol_type::SSM2_ISO15765:
		// CAN-ID, 4 bytes
		libFSSM::push_back_UInt32BigEndian(msg_buffer, ecuaddr);
		break;
	default:
		return false;
	}
	// Message:
	msg_buffer.insert(msg_buffer.end(), outdata, outdata + outdatalen);
	// Checksum (SSM2 over ISO-14230 only):
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
		msg_buffer.push_back( libFSSM::calcchecksum(msg_buffer.data(), msg_buffer.size()) );
#ifdef __FSSM_DEBUG__
	// DEBUG-OUTPUT:
	std::cout << "SSMP2communication_core::SndRcvMessage(...):   sending message:\n";
	std::cout << libFSSM::StrToMultiLineHexstr(msg_buffer, 16, "   ");
#endif
	// CLEAR INTERFACE BUFFERS:
	// NOTE: buffers can contain incomplete messages e.g. due to temporary "ignition off"
#ifdef __FSSM_DEBUG__
	if (!_diagInterface->clearSendBuffer())
		std::cout << "SSMP2communication_core::SndRcvMessage(...):   error: failed to clear Tx buffer !\n";
	if (!_diagInterface->clearReceiveBuffer())
		std::cout << "SSMP2communication_core::SndRcvMessage(...):   error: failed to clear Rx buffer !\n";
#else
	_diagInterface->clearSendBuffer();
	_diagInterface->clearReceiveBuffer();
#endif
	// SEND MESSAGE:
	if (!_diagInterface->write(msg_buffer))
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::SndRcvMessage(...):   error: write failed !\n";
#endif
		return false;
	}
	msg_buffer.clear();
	/* RECEIVE REPLY MESSAGE: */
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
	{
		if (!receiveReplyISO14230(ecuaddr, 4+outdatalen+1, &msg_buffer))
			return false;
	}
	else if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO15765)
	{
		if (!receiveReplyISO15765(ecuaddr, &msg_buffer))
			return false;
	}
#ifdef __FSSM_DEBUG__
	// DEBUG-OUTPUT:
	std::cout << "SSMP2communication_core::SndRcvMessage(...):   received message:\n";
	std::cout << libFSSM::StrToMultiLineHexstr(msg_buffer, 16, "   ");
#endif
	// MESSAGE LENGTH:
	switch(_diagInterface->protocolType()) {
	case AbstractDiagInterface::protocol_type::SSM2_ISO14230:
		// ignore SSM2_header[4] and checksum[1]
		std::copy(msg_buffer.begin() + 4, msg_buffer.end() - 1, indata);
		*indatalen = msg_buffer.size() - 4 - 1;
		break;
	case AbstractDiagInterface::protocol_type::SSM2_ISO15765:
		// ignore CAN-ID[4]
		std::copy(msg_buffer.begin() + 4, msg_buffer.end(), indata);
		*indatalen = msg_buffer.size() - 4;
		break;
	default:
		*indatalen = 0;
		return false;
	}
	return true;
}


bool SSMP2communication_core::receiveReplyISO14230(const unsigned int ecuaddr, const unsigned int outmsg_len, std::vector<char> *msg_buffer)
{
	std::vector<char> read_buffer;
	int min_bytes_to_read = 0; // NOTE: must be signed
	unsigned short int msglen = 0;
	bool echo = false;
	TimeM timer;
	unsigned long int time_elapsed = 0;
	unsigned int time_remaining = SSM2_READ_TIMEOUT;

	if ((ecuaddr > 0xff) || (outmsg_len > 260))  // BUG !
		return false;
	timer.start();
	// WAIT FOR HEADER OF ANSWER OR ECHO:
	if (!readFromInterface(4, time_remaining, &read_buffer, false))
		return false;
	// CHECK HEADER OF THE MESSAGE AND DETECT ECHO
	if ((read_buffer.at(0) == '\x80') &&
	    (read_buffer.at(1) == static_cast<char>(ecuaddr)) &&
	    (read_buffer.at(2) == '\xF0') &&
	    (read_buffer.at(3) == static_cast<char>(outmsg_len - 4 - 1)))
		echo = true;
	else if ((read_buffer.at(0) == '\x80') &&
		 (read_buffer.at(1) == '\xF0') &&
		 (read_buffer.at(2) == static_cast<char>(ecuaddr)))
		echo = false;
	else
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: invalid protocol header (#1) !\n";
#endif
		return false;
	}
	// READ MINIMUM REMAINING BYTES:
	msglen = 4 + static_cast<unsigned char>(read_buffer.at(3)) + 1;
	min_bytes_to_read = msglen - read_buffer.size() + echo*(4 + 1 + 1);
	if (min_bytes_to_read > 0)
	{
		time_elapsed = timer.elapsed();
		if (time_elapsed < SSM2_READ_TIMEOUT)
			time_remaining = SSM2_READ_TIMEOUT - time_elapsed;
		else
			time_remaining = 0;
		if (!readFromInterface(min_bytes_to_read, time_remaining, &read_buffer, true))
			return false;
	}
	// CHECK IF REPLY MESSAGE IS TOO LONG:
	if (!echo && (static_cast<int>(read_buffer.size()) != msglen))	// NOTE: if echo, read_buffer.size() = msglen + 4 + (1 + X) + 1
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: received reply message is too long (#1) !\n";
#endif
		return false;
	}
	// CHECK MESSAGE CHECKSUM:
	if (read_buffer.at(msglen-1) != libFSSM::calcchecksum(&read_buffer.at(0), msglen - 1))
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: wrong checksum (#1) !\n";
#endif
		return false;
	}
	// IF THERE IS NO ECHO, WE'RE DONE !
	if (!echo)
		return true;
	// ELIMINATE ECHO:
	read_buffer.erase(read_buffer.begin(), read_buffer.begin() + outmsg_len);
	// CHECK IF PROTOCOL HEADER OF REPLY IS CORRECT:
	if ((read_buffer.at(0) != '\x80') || (read_buffer.at(1) != '\xF0') || (read_buffer.at(2) != static_cast<char>(ecuaddr)))
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: reply message has invalid protocol header !\n";
#endif
		return false;
	}
	// READ REST OF THE MESSAGE:
	msglen = 4 + static_cast<unsigned char>(read_buffer.at(3)) + 1;
	min_bytes_to_read = msglen - read_buffer.size();
	if (min_bytes_to_read > 0)
	{
		time_elapsed = timer.elapsed();
		if (time_elapsed < SSM2_READ_TIMEOUT)
			time_remaining = SSM2_READ_TIMEOUT - time_elapsed;
		else
			time_remaining = 0;
		if (!readFromInterface(min_bytes_to_read, time_remaining, &read_buffer, true))
			return false;
	}
	// CHECK IF REPLY MESSAGE IS TOO LONG
	if (read_buffer.size() != msglen)
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: received reply message is too long (#2) !\n";
#endif
		return false;
	}
	// CHECK CHECKSUM:
	if (read_buffer.back() != libFSSM::calcchecksum(read_buffer.data(), read_buffer.size() - 1))
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: wrong checksum (#2) !\n";
#endif
		return false;
	}
	msg_buffer->assign(read_buffer.begin(), read_buffer.end());
	return true;
}


bool SSMP2communication_core::receiveReplyISO15765(const unsigned int ecuaddr, std::vector<char> *msg_buffer)
{
	msg_buffer->clear();
	// READ MESSAGE
	if (!readFromInterface(5, SSM2_READ_TIMEOUT, msg_buffer, false)) // NOTE: we always get complete messages from the interfaces (as long as minbytes is > 0) !
		return false;

	// CHECK CAN-IDENTIFIER (IF POSSIBLE)
	if (ecuaddr == (ecuaddr & 0x7EF)) // ISO15765-4 11 bit CAN IDs for physical addressing
	{
		unsigned int msgaddr = libFSSM::parseUInt32BigEndian(&msg_buffer->at(0));
		if (msgaddr != (ecuaddr + 8))
			return false;
	}
	return true;
}


bool SSMP2communication_core::readFromInterface(const unsigned int minbytes, const unsigned int timeout, std::vector<char> *buffer, bool append)
{
	std::vector<char> temp_buffer;
	std::vector<char> recv_buffer;
	TimeM time;
	time.start();
	do
	{
		waitms(10);
		temp_buffer.clear();
		if (!_diagInterface->read(&temp_buffer))
		{
#ifdef __FSSM_DEBUG__
			std::cout << "SSMP2communication_core::readFromInterface():   error: failed to read from interface !\n";
#endif
			// NOTE: fail silent
		}
		else if (temp_buffer.size())
		{
			recv_buffer.insert(recv_buffer.end(), temp_buffer.begin(), temp_buffer.end());
		}
	} while ((recv_buffer.size() < minbytes) && (time.elapsed() < timeout));
	if (recv_buffer.size() < minbytes)
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::readFromInterface():   error: timeout while reading from interface !\n";
#endif
		return false;
	}
	if (append)
		buffer->insert(buffer->end(), recv_buffer.begin(), recv_buffer.end());
	else
		buffer->assign(recv_buffer.begin(), recv_buffer.end());
	return true;
}
