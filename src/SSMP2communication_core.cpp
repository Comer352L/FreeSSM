/*
 * SSMP2communication_core.cpp - Core functions (services) of the new SSM-protocol
 *
 * Copyright (C) 2008-2012 Comer352L
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

#ifdef __WIN32__
    #include <windows.h>
    #define waitms(x) Sleep(x)
#elif defined __linux__
    #define waitms(x) usleep(1000*x)
#else
    #error "Operating system not supported !"
#endif


SSMP2communication_core::SSMP2communication_core(AbstractDiagInterface *diagInterface)
{
	_diagInterface = diagInterface;
}



bool SSMP2communication_core::ReadDataBlock(unsigned int ecuaddr, char padaddr, unsigned int dataaddr, unsigned char nrofbytes, char *data)
{
	if ((dataaddr > 0xffffff) || (nrofbytes == 0))
		return false;
	if ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230) && (nrofbytes > 254)) // ISO14230 protocol limit: length byte in header => max. 254 per reply message possible
	{
		return false;
	}
	else if ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO15765) && (nrofbytes > 256)) // ISO15765 protocol limit: data length byte in request => max. 256 possible
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
	unsigned int k;
	// SETUP MESSAGE (without Header+Checksum):
	querymsg[0] = '\xA0';
	querymsg[1] = padaddr;
	querymsg[2] = (dataaddr & 0xffffff) >> 16;
	querymsg[3] = (dataaddr & 0xffff) >> 8;
	querymsg[4] = dataaddr & 0xff;
	querymsg[5] = nrofbytes - 1;
	// SEND MESSAGE + RECIEVE ANSWER:
	if (SndRcvMessage(ecuaddr, querymsg, 6, indata, &indatalen))
	{
		// CHECK DATA:
		if (indatalen == (nrofbytes+1))
		{
			if (indata[0] == '\xE0')
			{
				// EXTRACT DATA:
				for (k=0; k<nrofbytes; k++)
					data[k] = indata[1+k];
				return true;
			}
		}
	}
	return false;
}



bool SSMP2communication_core::ReadMultipleDatabytes(unsigned int ecuaddr, char padaddr, unsigned int dataaddr[256], unsigned char datalen, char *data)
{
	if (datalen == 0)
		return false;
	if ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230) && (datalen > 84)) // ISO14230 protocol limit: length byte in header => max. (255-2)/3 = 84 addresses per request message possible
		return false;
	// NOTE: Control unit have (different) limits which are lower than the theoretical max. number of addresses per request !
	char indata[255] = {0,};
	unsigned char indatalen = 0;
	char querymsg[255] = {0,};
	unsigned int k = 0;
	// SETUP MESSAGE:
	querymsg[0] = '\xA8';
	querymsg[1] = padaddr;
	for (k=0; k<datalen; k++)
	{
		querymsg[2+k*3] = (dataaddr[k] & 0xffffff) >> 16;
		querymsg[3+k*3] = (dataaddr[k] & 0xffff) >> 8;
		querymsg[4+k*3] = dataaddr[k] & 0xff;
	}
	// SEND MESSAGE + RECIEVE ANSWER:
	if (SndRcvMessage(ecuaddr, querymsg, (2+3*datalen), indata, &indatalen))
	{
		// CHECK DATA:
		if (indatalen == (datalen+1))
		{
			if (indata[0] == '\xE8')
			{
				// EXTRACT DATA:
				for (k=0; k<datalen; k++)
					data[k] = indata[1+k];
				return true;
			}
		}
	}
	return false;
}



bool SSMP2communication_core::WriteDataBlock(unsigned int ecuaddr, unsigned int dataaddr, char *data, unsigned char datalen, char *datawritten)
{
	if ((dataaddr > 0xffffff) || (datalen == 0))
		return false;
	if ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230) && (datalen > 251)) // ISO14230 protocol limit: length byte => max. 255-4 = 251 data bytes per request message possible
		return false;
	char indata[252] = {0,};
	unsigned char indatalen = 0;
	char writemsg[255] = {0,};
	unsigned int k = 0;
	// SETUP MESSAGE:
	writemsg[0] = '\xB0';
	writemsg[1] = (dataaddr & 0xffffff) >> 16;
	writemsg[2] = (dataaddr & 0xffff) >> 8;
	writemsg[3] = dataaddr & 0xff;
	for (k=0; k<datalen; k++)
		writemsg[4+k] = data[k];
	// SEND MESSAGE + RECIEVE ANSWER:
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
					for (k=0; k<datalen; k++)
					{
						if (data[k] != indata[1+k])
						return false;
					}
					return true;
				}
				else
				{
					// EXTRACT AND RETURN WRITTEN DATA:
					for (k=0; k<datalen; k++)
						datawritten[k] = indata[1+k];
					return true;
					// NOTE: NECESSARY FOR SOME OPERATIONS, WHERE THE ACTUALLY WRITTEN DATA IS DIFFERENT TO THE DATA SENT OUT
				}
			}
		}
	}
	return false;
}



bool SSMP2communication_core::WriteDatabyte(unsigned int ecuaddr, unsigned int dataaddr, char databyte, char *databytewritten)
{
	if (dataaddr > 0xffffff) return false;
	char indata[2] = {0,};
	unsigned char indatalen = 0;
	char writemsg[5] = {0,};
	// SETUP MESSAGE (without Header+Checksum):
	writemsg[0] = '\xB8';
	writemsg[1] = (dataaddr & 0xffffff) >> 16;
	writemsg[2] = (dataaddr & 0xffff) >> 8;
	writemsg[3] = dataaddr & 0xff;
	writemsg[4] = databyte;
	// SEND MESSAGE + RECIEVE ANSWER:
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



bool SSMP2communication_core::GetCUdata(unsigned int ecuaddr, char *SYS_ID, char *ROM_ID, char *flagbytes, unsigned char *nrofflagbytes)
{
	*nrofflagbytes=0;
	char indata[255] = {0,};
	unsigned char indatalen = 0;
	char reqmsg = '\x0';
	unsigned char k = 0;
	// Request command byte
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230)
	{
		reqmsg = '\xBF';
	}
	else if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO15765)
	{
		reqmsg = '\xAA';
	}
	else
		return false;
	// SEND MESSAGE + RECIEVE ANSWER:
	if (SndRcvMessage(ecuaddr, &reqmsg, 1, indata, &indatalen))
	{
		// CHECK MESSAGE LENGTH:
		if ((indatalen == 41) || (indatalen == 57) || (indatalen == 105))
		{
			// CHECK DATA:
			if (((_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230) && (indata[0] == '\xFF'))
			    || ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO15765) && (indata[0] == '\xEA')))
			{
				// EXTRACT CU DATA:
				SYS_ID[0] = indata[1];
				SYS_ID[1] = indata[2];
				SYS_ID[2] = indata[3];
				ROM_ID[0] = indata[4];
				ROM_ID[1] = indata[5];
				ROM_ID[2] = indata[6];
				ROM_ID[3] = indata[7];
				ROM_ID[4] = indata[8];
				*nrofflagbytes = indatalen-9;
				for (k=0; k<*nrofflagbytes; k++)
					flagbytes[k] = indata[9+k];
				return true;
			}
		}
	}
	return false;
}



bool SSMP2communication_core::SndRcvMessage(unsigned int ecuaddr, char *outdata, unsigned char outdatalen, char *indata, unsigned char *indatalen)
{
	if (_diagInterface == NULL) return false;
	if (outdatalen < 1) return false;
	std::vector<char> msg_buffer;
	unsigned int k = 0;
	// SETUP COMPLETE MESSAGE:
	// Protocol-header
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230)
	{
	  	if (ecuaddr > 0xff) return false;
		msg_buffer.push_back('\x80');
		msg_buffer.push_back(ecuaddr);
		msg_buffer.push_back('\xF0');
		msg_buffer.push_back(static_cast<char>(outdatalen));
	}
	else if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO15765)
	{
		msg_buffer.push_back((ecuaddr & 0xffffffff) >> 24);
		msg_buffer.push_back((ecuaddr & 0xffffff) >> 16);
		msg_buffer.push_back((ecuaddr & 0xffff) >> 8);
		msg_buffer.push_back(ecuaddr & 0xff);
	}
	else
		return false;
	// Message:
	for (k=0; k<outdatalen; k++)
		msg_buffer.push_back(outdata[k]);
	// Checksum (SSM2 over ISO-14230 only):
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230)
		msg_buffer.push_back( calcchecksum(&msg_buffer.at(0), 4 + outdatalen) );
#ifdef __FSSM_DEBUG__
	// DEBUG-OUTPUT:
	std::cout << "SSMP2communication_core::SndRcvMessage(...):   sending message:\n";
	for (k=0; k<=(msg_buffer.size()/16); k++)
	{
		if (16*(k+1) <= msg_buffer.size())
			std::cout << "   " << libFSSM::StrToHexstr(&msg_buffer.at(k*16), 16) << '\n';
		else if (msg_buffer.size()%16)
			std::cout << "   " << libFSSM::StrToHexstr(&msg_buffer.at(k*16), (msg_buffer.size()%16)) << '\n';
	}
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
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230)
	{
		if (!receiveReplyISO14230(ecuaddr, 4+outdatalen+1, &msg_buffer))
			return false;
	}
	else if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO15765)
	{
		if (!receiveReplyISO15765(ecuaddr, &msg_buffer))
			return false;
	}
#ifdef __FSSM_DEBUG__
	// DEBUG-OUTPUT:
	std::cout << "SSMP2communication_core::SndRcvMessage(...):   received message:\n";
	for (k=0; k<=(msg_buffer.size()/16); k++)
	{
		if (16*(k+1) <= msg_buffer.size())
			std::cout << "   " << libFSSM::StrToHexstr(&msg_buffer.at(k*16), 16) << '\n';
		else if (msg_buffer.size()%16)
			std::cout << "   " << libFSSM::StrToHexstr(&msg_buffer.at(k*16), (msg_buffer.size()%16)) << '\n';
	}
#endif
	// MESSAGE LENGTH:
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230)
		*indatalen = msg_buffer.size() - 4 - 1;
	else
		*indatalen = msg_buffer.size() - 4;
	// EXTRACT AND RETURN DATA:
	for (k=0; k<*indatalen; k++)
		indata[k] = msg_buffer.at(4+k);
	return true;
}



bool SSMP2communication_core::receiveReplyISO14230(unsigned int ecuaddr, unsigned int outmsg_len, std::vector<char> *msg_buffer)
{
	/* NOTE: SERIAL-PASS-THROUGH-INTERFACES DO NOT RETURN COMPLETE MESSAGES, SO WE HAVE TO READ DATA IN STEPS (FOR A GOOD PERFORMANCE) */

	std::vector<char> read_buffer;
	int min_bytes_to_read = 0; // NOTE: must be signed !
	unsigned short int msglen = 0;
	bool echo = false;
	TimeM timer;

	msg_buffer->clear();
	timer.start();
	// WAIT FOR HEADER OF ANSWER OR ECHO:
	if (!readFromInterface(4, SSM2_READ_TIMEOUT, msg_buffer))
		return false;
	// CHECK HEADER OF THE MESSAGE AND DETECT ECHO
	if ((msg_buffer->at(0) == '\x80') && (static_cast<unsigned char>(msg_buffer->at(1)) == ecuaddr) && (msg_buffer->at(2) == '\xF0') && (static_cast<unsigned char>(msg_buffer->at(3)) == (outmsg_len - 4 - 1)))
	{
		// ECHO HEADER RECEIVED
		echo = true;
	}
	else if ((msg_buffer->at(0) == '\x80') && (msg_buffer->at(1) == '\xF0') && (static_cast<unsigned char>(msg_buffer->at(2)) == ecuaddr))
	{
		// HEADER OF INCOMING MESSAGE RECEIVED
		echo = false;
	}
	else
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: invalid protocol header (#1) !\n";
#endif
		return false;
	}
	// READ MINIMUM REMAINING BYTES
	msglen = (4 + static_cast<unsigned char>(msg_buffer->at(3)) + 1);
	min_bytes_to_read = msglen - msg_buffer->size() + echo*4; // NOTE: can be < 0 !
	if (min_bytes_to_read > 0)
	{
		if (!readFromInterface(min_bytes_to_read, SSM2_READ_TIMEOUT - timer.elapsed(), &read_buffer))
			return false;
		msg_buffer->insert(msg_buffer->end(), read_buffer.begin(), read_buffer.end());
	}
	else if (!echo && (static_cast<int>(msg_buffer->size()) != msglen))	// CHECK IF REPLY IS TOO LONG
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: received reply message is too long (#1) !\n";
#endif
		return false;
	}
	// CHECK MESSAGE CHECKSUM:
	if (msg_buffer->at(msglen-1) != calcchecksum(&msg_buffer->at(0), msglen - 1))
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
	msg_buffer->erase(msg_buffer->begin(), msg_buffer->begin() + outmsg_len);
	// CHECK IF PROTOCOL HEADER OF REPLY IS CORRECT:
	if ((msg_buffer->at(0) != '\x80') || (msg_buffer->at(1) != '\xF0') || (static_cast<unsigned char>(msg_buffer->at(2)) != ecuaddr))
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: invalid protocol header (#2)\n";
#endif
		return false;
	}
	// CALCULATE LENGTH OF COMPLETE ANSWER MESSAGE (using length byte of header):
	msglen = 4 + static_cast<unsigned char>(msg_buffer->at(3)) + 1;
	// READ REST OF THE MESSAGE:
	if (msg_buffer->size() < msglen)	// IF ANSWER IS INCOMPLETE
	{
		// WAIT FOR REST OF THE INCOMING MESSAGE:
		min_bytes_to_read = msglen - msg_buffer->size();
		if (!readFromInterface(min_bytes_to_read, SSM2_READ_TIMEOUT - timer.elapsed(), &read_buffer)) // Timeout: 260-4=256Bytes=533.3ms => 540ms
			return false;
		msg_buffer->insert(msg_buffer->end(), read_buffer.begin(), read_buffer.end());
	}
	else if (msg_buffer->size() != msglen)	// CHECK IF REPLY IS TOO LONG
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: received reply message is too long (#2) !\n";
#endif
		return false;
	}
	// CHECK CHECKSUM:
	if (msg_buffer->back() != calcchecksum(&msg_buffer->at(0), (msg_buffer->size() - 1)))
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::receiveReplyISO14230():   error: wrong checksum (#2) !\n";
#endif
		return false;
	}
	return true;
}


bool SSMP2communication_core::receiveReplyISO15765(unsigned int ecuaddr, std::vector<char> *msg_buffer)
{
	msg_buffer->clear();
	// READ MESSAGE
	if (!readFromInterface(5, SSM2_READ_TIMEOUT, msg_buffer)) // NOTE: we always get complete messages from the interfaces (as long as minbytes is > 0) !
		return false;
	// CHECK CAN-IDENTIFIER (IF POSSIBLE)
	if (ecuaddr == (ecuaddr & 0x7EF)) // ISO15765-4 11 bit CAN IDs for physical addressing
	{
		unsigned int msgaddr =  ((static_cast<unsigned char>(msg_buffer->at(0)) << 24)
				       + (static_cast<unsigned char>(msg_buffer->at(1)) << 16)
				       + (static_cast<unsigned char>(msg_buffer->at(2)) << 8)
				       +  static_cast<unsigned char>(msg_buffer->at(3)));
		if (msgaddr != (ecuaddr + 8))
			return false;
	}
	return true;
}



bool SSMP2communication_core::readFromInterface(unsigned int minbytes, unsigned int timeout, std::vector<char> *buffer)
{
	std::vector<char> read_buffer;
	TimeM time;
	time.start();
	buffer->clear();
	do
	{
		waitms(10);
		if (!_diagInterface->read(&read_buffer))
		{
#ifdef __FSSM_DEBUG__
			std::cout << "SSMP2communication_core::readFromInterface():   error: failed to read from interface !\n";
#endif
			// NOTE: fail silent
		}	
		else if (read_buffer.size())
		{
			buffer->insert(buffer->end(), read_buffer.begin(), read_buffer.end());
		}
	} while ((buffer->size() < minbytes) && (time.elapsed() < timeout));
	if (buffer->size() < minbytes)
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP2communication_core::readFromInterface():   error: timeout while reading from interface !\n";
#endif
		return false;
	}
	return true;
}



char SSMP2communication_core::calcchecksum(char *message, unsigned int nrofbytes)
{
	unsigned short int cs = 0;
	unsigned int k;
	for (k=0; k<nrofbytes; k++)
		cs = (cs + message[k]) & 0xff;
	return static_cast<char>(cs);
}



bool SSMP2communication_core::charcmp(char *chararray_a, char *chararray_b, unsigned int len)
{
	unsigned int k;
	for (k=0; k<len; k++)
	{
		if (chararray_a[k] != chararray_b[k])
		return false;
	}
	return true;
}


