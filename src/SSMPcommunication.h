/*
 * SSMPcommunication.h - Communication Thread for the new SSM-protocol
 *
 * Copyright © 2008 Comer352l
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

#ifndef SSMPCOMMUNICATION_H
#define SSMPCOMMUNICATION_H


#ifdef __WIN32__
    #include "windows\serialCOM.h"
#elif defined __linux__
    #include "linux/serialCOM.h"
#else
    #error "Operating system not supported !"
#endif
#include <QtGui>
#include "SSMPcommunication_core.h"



class SSMPcommunication : protected QThread, private SSMPcommunication_core
{
	Q_OBJECT

public:
	enum com_op_dt {noCom, readCUdata, readBlock, readMulti, writeBlock, writeSingle, readBlock_p, readMulti_p, writeBlock_p, writeSingle_p};

	SSMPcommunication(serialCOM *port, char cuaddress = '\x0', unsigned char errRetries = 2);
	~SSMPcommunication();
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

	com_op_dt getCurrentCommOperation();

	bool stopCommunication();

private:
	char _cuaddress;
	com_op_dt _CommOperation;
	QMutex _mutex;
	QEventLoop _el;
	bool _result;
	bool _abort;
	unsigned char _errRetries;
	// Buffers for sending/recieving data data:
	char _padadr;
	unsigned int _dataadr[256];
	unsigned char _datalen;
	char _snd_buf[256];
	char _rec_buf[256];
	int _delay;

	void run();
	bool doSingleCommOperation();

signals:
	void recievedData(QByteArray rawdata, int duration_ms);

	void commError();

};


#endif
