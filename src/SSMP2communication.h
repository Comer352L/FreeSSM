/*
 * SSMP2communication.h - Communication Thread for the new SSM-protocol
 *
 * Copyright (C) 2008-2010 Comer352l
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



class SSMP2communication : public QThread, private SSMP2communication_core
{
	Q_OBJECT

public:
	enum comOp_dt {comOp_noCom, comOp_readCUdata, comOp_readBlock, comOp_readMulti, comOp_writeBlock, comOp_writeSingle, comOp_readBlock_p, comOp_readMulti_p, comOp_writeBlock_p, comOp_writeSingle_p};

	SSMP2communication(AbstractDiagInterface *diagInterface, char cuaddress = '\x0', unsigned char errRetries = 2);
	~SSMP2communication();
	void setCUaddress(char cuaddress);
	void setRetriesOnError(unsigned char retries);

	bool getCUdata(char *SYS_ID, char *ROM_ID, char *flagbytes, unsigned char *nrofflagbytes);
	bool readDataBlock(char padadr, unsigned int dataadr, unsigned int nrofbytes, char *data);
	bool readMultipleDatabytes(char padadr, unsigned int dataadr[256], unsigned int datalen, char *data);
	bool writeDataBlock(unsigned int dataadr, char *data, unsigned int datalen, char *datawritten=NULL);
	bool writeDatabyte(unsigned int dataadr, char databyte, char *databytewritten=NULL);

	bool readDataBlock_permanent(char padadr, unsigned int dataadr, unsigned int nrofbytes, int delay=0);
	bool readMultipleDatabytes_permanent(char padadr, unsigned int dataadr[256], unsigned int datalen, int delay=0);
	bool writeDataBlock_permanent(unsigned int dataadr, char *data, unsigned int datalen, int delay=0);
	bool writeDatabyte_permanent(unsigned int dataadr, char databyte, int delay=0);

	comOp_dt getCurrentCommOperation();

	bool stopCommunication();

private:
	char _cuaddress;
	comOp_dt _CommOperation;
	QMutex _mutex;
	QEventLoop _el;
	bool _result;
	bool _abort;
	unsigned char _errRetries;
	// Buffers for sending/recieving data:
	char _padadr;
	unsigned int _dataadr[256];
	unsigned char _datalen;
	char _snd_buf[256];
	char _rec_buf[256];
	int _delay;

	bool doSingleCommOperation();
	void run();

signals:
	void recievedData(QByteArray rawdata, int duration_ms);

	void commError();

};


#endif
