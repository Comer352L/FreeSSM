/*
 * SSMP2communication.h - Communication Thread for the new SSM-protocol
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

#ifndef SSMP2COMMUNICATION_H
#define SSMP2COMMUNICATION_H


#include "AbstractSSMcommunication.h"
#include "AbstractDiagInterface.h"
#include "SSMP2communication_core.h"
#include "SSMCUdata.h"



//#define		__SSM2_BLOCK_OPS__		// NOTE: currently unused



class SSMP2communication : public AbstractSSMcommunication, private SSMP2communication_core
{

public:
	enum class comOp {
		noCom,
		readCUdata,
#ifdef __SSM2_BLOCK_OPS__
		readBlock,
#endif
		readMulti,
#ifdef __SSM2_BLOCK_OPS__
		writeBlock,
#endif
		writeSingle,
		writeMultiAddr_emul,
#ifdef __SSM2_BLOCK_OPS__
		readBlock_p,
#endif
		readMulti_p,
#ifdef __SSM2_BLOCK_OPS__
		writeBlock_p,
#endif
		writeSingle_p,
		writeMultiAddr_emul_p
	};

	SSMP2communication(AbstractDiagInterface *diagInterface, unsigned int cuaddress = 0x0, unsigned char errRetries = 2);
	~SSMP2communication();
	void setCUaddress(unsigned int cuaddress);
	comOp getCurrentCommOperation();
	bool stopCommunication();

	// Native communication functions:
	bool getCUdata(SSMCUdata& cuData);
#ifdef __SSM2_BLOCK_OPS__
	bool readDataBlock(char padaddr, unsigned int dataaddr, unsigned int nrofbytes, std::vector<char> *data);
	bool readDataBlock_permanent(const char padaddr, const unsigned int dataaddr, const unsigned int nrofbytes, const int delay=0);
#endif
	bool readMultipleDatabytes(char padaddr, const std::vector<unsigned int> dataaddr, std::vector<char> *data);
	bool readMultipleDatabytes_permanent(const char padaddr, const std::vector<unsigned int> dataaddr, const int delay = 0);
#ifdef __SSM2_BLOCK_OPS__
	bool writeDataBlock(const unsigned int dataaddr, const std::vector<char> data, std::vector<char> *datawritten = NULL);
	bool writeDataBlock_permanent(const unsigned int dataaddr, const std::vector<char> data, const int delay = 0);
#endif
	bool writeDatabyte(const unsigned int dataaddr, const char databyte, char *databytewritten = NULL);
	bool writeDatabyte_permanent(const unsigned int dataaddr, const char databyte, const int delay = 0);

	// Emulated communication functions
	bool writeMultipleDatabytes(const std::vector<unsigned int> dataaddr, std::vector<char> data, std::vector<char> *datawritten);
	bool writeMultipleDatabytes_permanent(const std::vector<unsigned int> dataaddr, const std::vector<char> data, const int delay = 0);

	// Abstract communication interface:
	bool readAddress(unsigned int addr, char * databyte);
	bool readAddresses(const std::vector<unsigned int>& addr, std::vector<char> * data);
	bool readAddress_permanent(unsigned int addr, int delay = 0);
	bool readAddresses_permanent(std::vector<unsigned int> addr, int delay = 0);
	bool writeAddress(unsigned int addr, char databyte, char *databytewritten = NULL);
	bool writeAddresses(std::vector<unsigned int> addr, std::vector<char> data, std::vector<char> *databyteswritten = NULL);
	bool writeAddress_permanent(unsigned int addr, char databyte, int delay = 0);
	bool writeAddresses_permanent(std::vector<unsigned int> addr, std::vector<char> data, int delay = 0);

private:
	unsigned int _cuaddress;
	comOp _CommOperation;
	// Buffers for sending/recieving data:
	char _padaddr;
	std::vector<unsigned int> _dataaddr;
	unsigned int _datalen;
	std::vector<char> _snd_buf;
	std::vector<char> _rec_buf;

	bool doSingleCommOperation();
	void run();

};


#endif
