/*
 * SSMP2communication_core.h - Core functions (services) of the new SSM-protocol
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

#ifndef SSMP2COMMUNICATIONCORE_H
#define SSMP2COMMUNICATIONCORE_H


#include "AbstractDiagInterface.h"
#ifdef __WIN32__
    #include "windows\TimeM.h"
#elif defined __linux__
    #include "linux/TimeM.h"
#else
    #error "Operating system not supported !"
#endif
#ifdef __FSSM_DEBUG__
    #include <iostream>
    #include "libFSSM.h"
#endif



#define SSM2_READ_TIMEOUT	2000 // [ms]



class SSMP2communication_core
{

public:
	SSMP2communication_core(AbstractDiagInterface *diagInterface);

	bool ReadDataBlock(unsigned int ecuaddr, char padaddr, unsigned int dataaddr, unsigned int nrofbytes, char *data);
	bool ReadMultipleDatabytes(unsigned int ecuaddr, char padaddr, unsigned int dataaddr[256], unsigned int datalen, char *data);
	bool WriteDataBlock(unsigned int ecuaddr, unsigned int dataaddr, char *data, unsigned int datalen, char *datawritten = NULL);
	bool WriteDatabyte(unsigned int ecuaddr, unsigned int dataaddr, char databyte, char *databytewritten = NULL);
	bool GetCUdata(unsigned int ecuaddr, char *SYS_ID, char *ROM_ID, char *flagbytes, unsigned char *nrofflagbytes);

private:
	AbstractDiagInterface *_diagInterface;

	bool SndRcvMessage(unsigned int ecuaddr, char *outdata, unsigned char outdatalen, char *indata, unsigned char *indatalen);
	bool receiveReplyISO14230(unsigned int ecuaddr, unsigned int outmsg_len, std::vector<char> *msg_buffer);
	bool receiveReplyISO15765(unsigned int ecuaddr, std::vector<char> *msg_buffer);
	bool readFromInterface(unsigned int minbytes, unsigned int timeout, std::vector<char> *buffer);
	char calcchecksum(char *message, unsigned int nrofbytes);
	bool charcmp(char *chararray_a, char *chararray_b, unsigned int len);

};



#endif
