/*
 * SSMP2communication.h - Communication Thread for the new SSM-protocol
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

#ifndef SSMP2COMMUNICATION_H
#define SSMP2COMMUNICATION_H


#include <QtGui>
#include "AbstractDiagInterface.h"
#include "SSMP2communication_core.h"
#include "SSMCUdata.h"


#define SSMP2COM_BUFFER_SIZE	256  // buffer size => max. nr. of bytes/addresses for requests; MIN 104 NEEDED !



class SSMP2communication : public QThread, private SSMP2communication_core
{
	Q_OBJECT

public:
	enum comOp_dt {comOp_noCom, comOp_readCUdata, comOp_readBlock, comOp_readMulti, comOp_writeBlock, comOp_writeSingle, comOp_readBlock_p, comOp_readMulti_p, comOp_writeBlock_p, comOp_writeSingle_p};

	SSMP2communication(AbstractDiagInterface *diagInterface, unsigned int cuaddress = 0x0, unsigned char errRetries = 2);
	~SSMP2communication();
	void setCUaddress(unsigned int cuaddress);
	void setRetriesOnError(unsigned char retries);

	bool getCUdata(SSMCUdata& cuData);
	bool readDataBlock(char padaddr, unsigned int dataaddr, unsigned int nrofbytes, char *data);
	bool readMultipleDatabytes(char padaddr, unsigned int dataaddr[SSMP2COM_BUFFER_SIZE], unsigned int datalen, char *data);
	bool writeDataBlock(unsigned int dataaddr, char *data, unsigned int datalen, char *datawritten=NULL);
	bool writeDatabyte(unsigned int dataaddr, char databyte, char *databytewritten=NULL);

	bool readDataBlock_permanent(char padaddr, unsigned int dataaddr, unsigned int nrofbytes, int delay=0);
	bool readMultipleDatabytes_permanent(char padaddr, unsigned int dataaddr[SSMP2COM_BUFFER_SIZE], unsigned int datalen, int delay=0);
	bool writeDataBlock_permanent(unsigned int dataaddr, char *data, unsigned int datalen, int delay=0);
	bool writeDatabyte_permanent(unsigned int dataaddr, char databyte, int delay=0);

	comOp_dt getCurrentCommOperation();

	bool stopCommunication();

private:
	unsigned int _cuaddress;
	comOp_dt _CommOperation;
	QMutex _mutex;
	QEventLoop _el;
	bool _result;
	bool _abort;
	unsigned char _errRetries;
	// Buffers for sending/recieving data:
	char _padaddr;
	unsigned int _dataaddr[SSMP2COM_BUFFER_SIZE];
	unsigned int _datalen;
	char _snd_buf[SSMP2COM_BUFFER_SIZE];
	char _rec_buf[SSMP2COM_BUFFER_SIZE];
	int _delay;

	bool doSingleCommOperation();
	void run();

signals:
	void receivedData(const std::vector<char>& rawdata, int duration_ms);

	void commError();

};


#endif
