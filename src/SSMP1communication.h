/*
 * SSMP1communication.h - Communication Thread for the old SSM-protocol
 *
 * Copyright (C) 2009-2012 Comer352L
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

#ifndef SSMP1COMMUNICATION_H
#define SSMP1COMMUNICATION_H


#include <QtGui>
#include <vector>
#include "AbstractDiagInterface.h"
#include "SSMP1communication_procedures.h"
#include "SSMCUdata.h"


class SSMP1communication : public QThread, private SSMP1communication_procedures
{
	Q_OBJECT

public:
	enum comOp_dt {comOp_noCom, comOp_readRomId, comOp_read, comOp_write, comOp_read_p, comOp_write_p};

	SSMP1communication(AbstractDiagInterface *diagInterface, SSM1_CUtype_dt cu, unsigned char errRetries = 2);
	~SSMP1communication();
	void selectCU(SSM1_CUtype_dt cu);
	void setRetriesOnError(unsigned char retries);
	comOp_dt getCurrentCommOperation();
	bool getCUdata(unsigned char extradatareqlen, SSMCUdata& cuData);
	bool readAddress(unsigned int addr, char * databyte);
	bool readAddresses(const std::vector<unsigned int>& addr, std::vector<char> * data);
	bool readAddress_permanent(unsigned int addr, int delay=0);
	bool readAddresses_permanent(std::vector<unsigned int> addr, int delay=0);
	bool writeAddress(unsigned int addr, char databyte, char *databytewritten = NULL);
	bool writeAddresses(std::vector<unsigned int> addr, std::vector<char> data, std::vector<char> *databyteswritten = NULL);
	bool writeAddress_permanent(unsigned int addr, char databyte, int delay=0);
	bool writeAddresses_permanent(std::vector<unsigned int> addr, std::vector<char> data, int delay=0);
	bool stopCommunication();

private:
	SSM1_CUtype_dt _cu;
	unsigned char _errRetries;
	comOp_dt _CommOperation;
	QMutex _mutex;
	QEventLoop _el;
	bool _result;
	bool _abort;
	unsigned char _reqsize;			/* size of extra data requested with the getCUdata command */
	std::vector<unsigned int> _addresses;	/* list of addresses for the read/write operation(s) */
	std::vector<char> _data;		/* processed data from read/Rom-ID operation(s)      OR
						   data to be written during the write operation(s)  */
	int _delay;

	bool doSingleCommOperation();
	void run();

signals:
	void receivedData(const std::vector<char>& rawdata, int duration_ms);
	void commError();

};


#endif
