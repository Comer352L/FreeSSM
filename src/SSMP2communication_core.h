/*
 * SSMP2communication_core.h - Core functions (services) of the new SSM-protocol
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

#ifndef SSMP2COMMUNICATIONCORE_H
#define SSMP2COMMUNICATIONCORE_H


#include "AbstractDiagInterface.h"
#include "libFSSM.h"
#ifdef __WIN32__
	#include "windows\TimeM.h"
	#define waitms(x) Sleep(x)
#elif defined __linux__
	#include <unistd.h>
	#include "linux/TimeM.h"
	#define waitms(x) usleep(1000*x)
#else
	#error "Operating system not supported !"
#endif
#ifdef __FSSM_DEBUG__
	#include <iostream>
#endif



#define SSM2_READ_TIMEOUT	2000 // [ms]



class SSMP2communication_core
{

public:
	SSMP2communication_core(AbstractDiagInterface *diagInterface);

	bool ReadDataBlock(const unsigned int ecuaddr, const char padaddr, const unsigned int dataaddr, const unsigned int nrofbytes, char *data);
	bool ReadMultipleDatabytes(const unsigned int ecuaddr, const char padaddr, const unsigned int dataaddr[], const unsigned int datalen, char* data);
	bool WriteDataBlock(const unsigned int ecuaddr, const unsigned int dataaddr, const char *data, const unsigned int datalen, char *datawritten = NULL);
	bool WriteDatabyte(const unsigned int ecuaddr, const unsigned int dataaddr, const char databyte, char *databytewritten = NULL);
	bool GetCUdata(const unsigned int ecuaddr, char *cuData, unsigned char *cuDataSize);

protected:
	AbstractDiagInterface *_diagInterface;

private:
	bool SndRcvMessage(const unsigned int ecuaddr, const char *outdata, const unsigned char outdatalen, char *indata, unsigned char *indatalen);
	bool receiveReplyISO14230(const unsigned int ecuaddr, const unsigned int outmsg_len, std::vector<char> *msg_buffer);
	bool receiveReplyISO15765(const unsigned int ecuaddr, std::vector<char> *msg_buffer);
	bool readFromInterface(const unsigned int minbytes, const unsigned int timeout, std::vector<char> *buffer, bool append = false);
};



#endif
