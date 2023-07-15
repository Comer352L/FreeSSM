/*
 * SSMP2communication_core.cpp - Core functions (services) of the new SSM-protocol
 *
 * Copyright (C) 2008-2023 Comer352L
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


SSMP2communication_core::Result SSMP2communication_core::GetCUdata(const unsigned int ecuaddr, std::vector<char> *cuData)
{
	std::vector<char> req;
	std::vector<char> resp;
	Result res;

	if (cuData == NULL)
		return Result::error;
	cuData->clear();
	// SETUP MESSAGE:
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
		req.push_back('\xBF');
	else if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO15765)
		req.push_back('\xAA');
	else
		return Result::error;
	// SEND MESSAGE + RECEIVE ANSWER:
	res = SndRcvMessage(ecuaddr, req, &resp);
	if (res == Result::success)
	{
		// CHECK MESSAGE LENGTH:
		// usual flagbytes sizes: 32, 48, 96
		// i.e. ResponseCommand[1] + SYSID[3] + ROMID[5] + flagbytes[96] = 105 bytes total
		if ((resp.size() == 41) || (resp.size() == 57) || (resp.size() == 105))
		{
			// CHECK DATA:
			if (   ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230) && (resp.at(0) == '\xFF'))
			    || ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO15765) && (resp.at(0) == '\xEA')))
			{
				cuData->assign(resp.begin() + 1, resp.end());
				return Result::success;
			}
		}
		else
			return Result::error;
	}
	return res;
}


SSMP2communication_core::Result SSMP2communication_core::ReadDataBlock(const unsigned int ecuaddr, const char padaddr, const unsigned int dataaddr, const unsigned int nrofbytes, std::vector<char> *data)
{
	std::vector<char> req;
	std::vector<char> resp;
	Result res;

	if (data == NULL)
		return Result::error;
	data->clear();
	if ((dataaddr > 0xffffff) || (nrofbytes == 0))
		return Result::error;
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
	{
		if (nrofbytes > 254) // ISO14230 protocol limit: data length byte in response => max. 254 possible
			return Result::error;
	}
	else if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO15765)
	{
		if (nrofbytes > 256) // ISO15765 protocol limit: data length byte in request => max. 256 possible
			return Result::error;
	}
	else
		return Result::error;
	// SETUP MESSAGE:
	req.push_back('\xA0');
	req.push_back(padaddr);
	libFSSM::push_backUInt24BigEndian(req, dataaddr);
	req.push_back(nrofbytes - 1);
	// SEND MESSAGE + RECEIVE ANSWER:
	res = SndRcvMessage(ecuaddr, req, &resp);
	if (res == Result::success)
	{
		// CHECK DATA:
		if ((resp.size() == (nrofbytes + 1)) && (resp.at(0) == '\xE0'))
		{
			// EXTRACT DATA:
			data->assign(resp.begin() + 1, resp.end());
			return Result::success;
		}
		else
			return Result::error;
	}
	return res;
}


SSMP2communication_core::Result SSMP2communication_core::ReadMultipleDatabytes(const unsigned int ecuaddr, const char padaddr, const std::vector<unsigned int> dataaddr, std::vector<char> *data)
{
	std::vector<char> req;
	std::vector<char> resp;
	Result res;

	if (data == NULL)
		return Result::error;
	data->clear();
	if (dataaddr.size() == 0)
		return Result::error;
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
	{
		if (dataaddr.size() > 84) // ISO14230 protocol limit: length byte in header => max. (255-2)/3 = 84 addresses per request message possible
			return Result::error;
	}
	else if (_diagInterface->protocolType() != AbstractDiagInterface::protocol_type::SSM2_ISO15765)
		return Result::error;
	// NOTE: Control unit have (different) limits which are lower than the theoretical max. number of addresses per request !
	// SETUP MESSAGE:
	req.push_back('\xA8');
	req.push_back(padaddr);
	for (size_t k = 0; k < dataaddr.size(); k++)
	{
		if (dataaddr.at(k) > 0xffffff)
			return Result::error;
		libFSSM::push_backUInt24BigEndian(req, dataaddr.at(k));
	}
	// SEND MESSAGE + RECEIVE ANSWER:
	res = SndRcvMessage(ecuaddr, req, &resp);
	if (res == Result::success)
	{
		// CHECK DATA:
		if ((resp.size() == (dataaddr.size() + 1)) && (resp.at(0) == '\xE8'))
		{
			// EXTRACT DATA:
			data->assign(resp.begin() + 1, resp.end());
			return Result::success;
		}
		else
			return Result::error;
	}
	return res;
}


SSMP2communication_core::Result SSMP2communication_core::WriteDataBlock(const unsigned int ecuaddr, const unsigned int dataaddr, const std::vector<char> data, std::vector<char> *datawritten)
{
	std::vector<char> req = {'\xB0'};
	std::vector<char> resp;
	Result res;

	if (datawritten != NULL)
		datawritten->clear();
	if ((dataaddr > 0xffffff) || (data.size() == 0))
		return Result::error;
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
	{
		if (data.size() > 251) // ISO14230 protocol limit: length byte => max. 255-4 = 251 data bytes per request message possible
			return Result::error;
	}
	else if (_diagInterface->protocolType() != AbstractDiagInterface::protocol_type::SSM2_ISO15765)
		return Result::error;
	// SETUP MESSAGE:
	libFSSM::push_backUInt24BigEndian(req, dataaddr);
	req.insert(req.end(), data.begin(), data.end());
	// SEND MESSAGE + RECEIVE ANSWER:
	res = SndRcvMessage(ecuaddr, req, &resp);
	if (res == Result::success)
	{
		// CHECK DATA:
		if ((resp.size() == (data.size() + 1)) && (resp.at(0) == '\xF0'))
		{
			if (datawritten == NULL)
			{
				// CHECK IF ACTUALLY WRITTEN DATA IS EQUAL TO THE DATA SENT OUT:
				if (!memcmp(data.data(), resp.data() + 1, data.size()))
					return Result::success;
				else
					return Result::error;
			}
			else
			{
				// EXTRACT AND RETURN WRITTEN DATA:
				datawritten->assign(resp.begin() + 1, resp.end());
				return Result::success;
				// NOTE: NECESSARY FOR SOME OPERATIONS, WHERE THE ACTUALLY WRITTEN DATA IS DIFFERENT TO THE DATA SENT OUT
			}
		}
		else
			return Result::error;
	}
	return res;
}


SSMP2communication_core::Result SSMP2communication_core::WriteDatabyte(const unsigned int ecuaddr, const unsigned int dataaddr, const char databyte, char *databytewritten)
{
	std::vector<char> req;
	std::vector<char> resp;
	Result res;

	if (dataaddr > 0xffffff)
		return Result::error;
	if (   (_diagInterface->protocolType() != AbstractDiagInterface::protocol_type::SSM2_ISO14230)
	    && (_diagInterface->protocolType() != AbstractDiagInterface::protocol_type::SSM2_ISO15765))
		return Result::error;
	// SETUP MESSAGE:
	req.push_back('\xB8');
	libFSSM::push_backUInt24BigEndian(req, dataaddr);
	req.push_back(databyte);
	// SEND MESSAGE + RECEIVE ANSWER:
	res = (SndRcvMessage(ecuaddr, req, &resp));
	if (res == Result::success)
	{
		// CHECK DATA:
		if ((resp.size() == 2) && (resp.at(0) == '\xF8'))
		{
			if (databytewritten == NULL)
			{
				// CHECK IF ACTUALLY WRITTEN DATA IS EQAUL TO THE DATA SENT OUT:
				if (resp.at(1) == databyte)
					return Result::success;
				else
					return Result::error;
			}
			else
			{
				// EXTRACT AND RETURN WRITTEN DATA:
				*databytewritten = resp.at(1);
				return Result::success;
				// NOTE: NECESSARY FOR SOME OPERATIONS, WHERE THE ACTUALLY WRITTEN DATA IS DIFFERENT TO THE DATA SENT OUT
			}
		}
		else
			return Result::error;
	}
	return res;
}


SSMP2communication_core::Result SSMP2communication_core::SndRcvMessage(const unsigned int ecuaddr, const std::vector<char> request, std::vector<char> *response)
{
	response->clear();
	if (_diagInterface == NULL)
		return Result::error;
	if (request.size() < 1)
		return Result::error;
	std::vector<char> msg_buffer;
	// SETUP COMPLETE MESSAGE:
	// Protocol-header
	switch(_diagInterface->protocolType())
	{
		case AbstractDiagInterface::protocol_type::SSM2_ISO14230:
			if ((ecuaddr > 0xff) || (request.size() > 255))
				return Result::error;
			// header, 4 bytes
			msg_buffer.push_back('\x80');
			msg_buffer.push_back(ecuaddr);
			msg_buffer.push_back('\xF0');
			msg_buffer.push_back(static_cast<char>(request.size()));
			break;
		case AbstractDiagInterface::protocol_type::SSM2_ISO15765:
			// CAN-ID, 4 bytes
			libFSSM::push_back_UInt32BigEndian(msg_buffer, ecuaddr);
			break;
		default:
			return Result::error;
	}
	// Message:
	msg_buffer.insert(msg_buffer.end(), request.begin(), request.end());
	// Checksum (SSM2 over ISO-14230 only):
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
		msg_buffer.push_back( libFSSM::calcchecksum(msg_buffer.data(), msg_buffer.size()) );
#ifdef __FSSM_DEBUG__
	// DEBUG-OUTPUT:
	std::cout << "SSMP2communication_core::SndRcvMessage(...):   sending message:" << std::endl;
	std::cout << libFSSM::StrToMultiLineHexstr(msg_buffer, 16, "   ");
#endif
	// CLEAR INTERFACE BUFFERS:
	// NOTE: buffers can contain incomplete messages e.g. due to temporary "ignition off"
#ifdef __FSSM_DEBUG__
	if (!_diagInterface->clearSendBuffer())
		std::cout << "SSMP2communication_core::SndRcvMessage(...):   error: failed to clear Tx buffer !" << std::endl;
	if (!_diagInterface->clearReceiveBuffer())
		std::cout << "SSMP2communication_core::SndRcvMessage(...):   error: failed to clear Rx buffer !" << std::endl;
#else
	_diagInterface->clearSendBuffer();
	_diagInterface->clearReceiveBuffer();
#endif
	// SEND MESSAGE:
	if (!_diagInterface->write(msg_buffer))
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::SndRcvMessage(...):   error: write failed !" << std::endl;
#endif
		return Result::error;
	}
	msg_buffer.clear();
	/* RECEIVE REPLY MESSAGE: */
	Result res;
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
	{
		res = receiveReplyISO14230(ecuaddr, 4 + request.size() + 1, &msg_buffer);
		if (res != Result::success)
			return res;
	}
	else if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO15765)
	{
		res = receiveReplyISO15765(ecuaddr, &msg_buffer);
		if (res != Result::success)
			return res;
	}
#ifdef __FSSM_DEBUG__
	// DEBUG-OUTPUT:
	std::cout << "SSMP2communication_core::SndRcvMessage(...):   received message:" << std::endl;
	std::cout << libFSSM::StrToMultiLineHexstr(msg_buffer, 16, "   ");
#endif
	// MESSAGE LENGTH:
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
	{
		// ignore SSM2_header[4] and checksum[1]
		response->assign(msg_buffer.begin() + 4, msg_buffer.end() - 1);
	}
	else if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO15765)
	{
		// ignore CAN-ID[4]
		response->assign(msg_buffer.begin() + 4, msg_buffer.end());
	}
	return Result::success;
}


SSMP2communication_core::Result SSMP2communication_core::receiveReplyISO14230(const unsigned int ecuaddr, const unsigned int outmsg_len, std::vector<char> *msg_buffer)
{
	std::vector<char> read_buffer;
	int min_bytes_to_read = 0; // NOTE: must be signed
	unsigned short int msglen = 0;
	bool echo = false;
	TimeM timer;
	unsigned long int time_elapsed = 0;
	unsigned int time_remaining = SSM2_READ_TIMEOUT;

	if ((ecuaddr > 0xff) || (outmsg_len > 260))  // BUG !
		return Result::error;
	timer.start();
	// WAIT FOR HEADER OF ANSWER OR ECHO:
	if (!readFromInterface(4, time_remaining, &read_buffer, false))
		return Result::error;
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
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: invalid protocol header (#1) !" << std::endl;
#endif
		return Result::error;
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
			return Result::error;
	}
	// CHECK IF REPLY MESSAGE IS TOO LONG:
	if (!echo && (static_cast<int>(read_buffer.size()) != msglen))	// NOTE: if echo, read_buffer.size() = msglen + 4 + (1 + X) + 1
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: received reply message is too long (#1) !" << std::endl;
#endif
		return Result::error;
	}
	// CHECK MESSAGE CHECKSUM:
	if (read_buffer.at(msglen - 1) != libFSSM::calcchecksum(&read_buffer.at(0), msglen - 1))
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: wrong checksum (#1) !" << std::endl;
#endif
		return Result::error;
	}
	// IF THERE IS NO ECHO, WE'RE DONE !
	if (!echo)
		return Result::success;
	// ELIMINATE ECHO:
	read_buffer.erase(read_buffer.begin(), read_buffer.begin() + outmsg_len);
	// CHECK IF PROTOCOL HEADER OF REPLY IS CORRECT:
	if ((read_buffer.at(0) != '\x80') || (read_buffer.at(1) != '\xF0') || (read_buffer.at(2) != static_cast<char>(ecuaddr)))
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: reply message has invalid protocol header !" << std::endl;
#endif
		return Result::error;
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
			return Result::error;
	}
	// CHECK IF REPLY MESSAGE IS TOO LONG
	if (read_buffer.size() != msglen)
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: received reply message is too long (#2) !" << std::endl;
#endif
		return Result::error;
	}
	// CHECK CHECKSUM:
	if (read_buffer.back() != libFSSM::calcchecksum(read_buffer.data(), read_buffer.size() - 1))
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: wrong checksum (#2) !" << std::endl;
#endif
		return Result::error;
	}
	msg_buffer->assign(read_buffer.begin(), read_buffer.end());
	return Result::success;
}


SSMP2communication_core::Result SSMP2communication_core::receiveReplyISO15765(const unsigned int ecuaddr, std::vector<char> *msg_buffer)
{
	msg_buffer->clear();
	// READ MESSAGE
	if (!readFromInterface(5, SSM2_READ_TIMEOUT, msg_buffer, false)) // NOTE: we always get complete messages from the interfaces (as long as minbytes is > 0) !
		return Result::error;

	// CHECK CAN-IDENTIFIER (IF POSSIBLE)
	if (ecuaddr == (ecuaddr & 0x7EF)) // ISO15765-4 11 bit CAN IDs for physical addressing
	{
		unsigned int msgaddr = libFSSM::parseUInt32BigEndian(&msg_buffer->at(0));
		if (msgaddr != (ecuaddr + 8))
			return Result::error;
		if ((msg_buffer->size() == 7) && (msg_buffer->at(4) == '\x7F'))
		{
#ifdef __FSSM_DEBUG__
			std::cout << "SSMP2communication_core::receiveReplyISO15765():   request rejected with response code 0x" << libFSSM::StrToHexstr( &(msg_buffer->at(6)), 1 ) << std::endl;
#endif
			return Result::rejected;
		}
	}
	return Result::success;
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
			std::cout << "SSMP2communication_core::readFromInterface():   error: failed to read from interface !" << std::endl;
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
		std::cout << "SSMP2communication_core::readFromInterface():   error: timeout while reading from interface !" << std::endl;
#endif
		return false;
	}
	if (append)
		buffer->insert(buffer->end(), recv_buffer.begin(), recv_buffer.end());
	else
		buffer->assign(recv_buffer.begin(), recv_buffer.end());
	return true;
}
