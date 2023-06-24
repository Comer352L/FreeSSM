/*
 * SSMP2communication_core.h - Core functions (services) of the new SSM-protocol
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
	enum class Result {success, error, rejected};

	SSMP2communication_core(AbstractDiagInterface *diagInterface);

	Result GetCUdata(const unsigned int ecuaddr, std::vector<char> *cuData);
	Result ReadDataBlock(const unsigned int ecuaddr, const char padaddr, const unsigned int dataaddr, const unsigned int nrofbytes, std::vector<char> *data);
	Result ReadMultipleDatabytes(const unsigned int ecuaddr, const char padaddr, const std::vector<unsigned int> dataaddr, std::vector<char> *data);
	Result WriteDataBlock(const unsigned int ecuaddr, const unsigned int dataaddr, const std::vector<char> data, std::vector<char> *datawritten = NULL);
	Result WriteDatabyte(const unsigned int ecuaddr, const unsigned int dataaddr, const char databyte, char *databytewritten = NULL);

protected:
	AbstractDiagInterface *_diagInterface;

private:
	Result SndRcvMessage(const unsigned int ecuaddr, const std::vector<char> request, std::vector<char> *response);
	Result receiveReplyISO14230(const unsigned int ecuaddr, const unsigned int outmsg_len, std::vector<char> *msg_buffer);
	Result receiveReplyISO15765(const unsigned int ecuaddr, std::vector<char> *msg_buffer);
	bool readFromInterface(const unsigned int minbytes, const unsigned int timeout, std::vector<char> *buffer, bool append = false);
};



#endif
