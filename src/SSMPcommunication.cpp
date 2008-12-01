/*
 * SSMPcommunication.cpp - Communication Thread for the new SSM-protocol
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


#include "SSMPcommunication.h"


SSMPcommunication::SSMPcommunication(serialCOM *port, char cuaddress, unsigned char errRetries) : QThread(), SSMPcommunication_core(port)
{
	_cuaddress = cuaddress;

	unsigned int k = 0;
	_CommOperation = noCom;
	_result = false;
	_abort = false;
	_errRetries = errRetries;
	_padadr = 0;
	for (k=0; k<256; k++) _dataadr[k] = 0;
	_datalen = 0;
	for (k=0; k<256; k++) _rec_buf[k] = 0;
	for (k=0; k<256; k++) _snd_buf[k] = 0;
	_delay = 0;
}



SSMPcommunication::~SSMPcommunication()
{
	stopCommunication();
	disconnect( this, SIGNAL( finished() ), 0, 0 );
}



void SSMPcommunication::setCUaddress(char cuaddress)
{
	_cuaddress = cuaddress;
}



void SSMPcommunication::setRetriesOnError(unsigned char retries)
{
	_errRetries = retries;
}



void SSMPcommunication::run()
{
	QByteArray rawdata;
	QTime timer;
	int duration_ms = 0;
	unsigned int k = 1;
	unsigned int rindex = 1;
	unsigned char nrofReadAddr = 0;
	char errcount = 0;
	bool permanent = false;
	bool op_success = false;
	bool abort;
	com_op_dt operation;
	char cuaddress;
	char padadr = '\x0';
	unsigned char datalen = 0;
	unsigned int dataadr[256] = {0};
	char snd_buf[256] = {'\x0'};
	char rec_buf[256] = {'\x0'};
	int delay = 0;
	unsigned char errmax = 3;

	// Synchronise with main-thread:
	_mutex.lock();
	operation = _CommOperation;
	cuaddress = _cuaddress;
	padadr = _padadr;
	datalen = _datalen;
	for (k=0; k<datalen; k++) dataadr[k] = _dataadr[k];
	for (k=0; k<datalen; k++) snd_buf[k] = _snd_buf[k];
	delay = _delay;
	errmax = _errRetries + 1;
	_result = false;
	_abort = false;
	_mutex.unlock();
	// Preparation:
	if ( operation==readBlock_p || operation==readMulti_p || operation==writeBlock_p || operation==writeSingle_p )
	{
		permanent = true;
		timer.start();
	}
	// COMMUNICATION:
	do
	{
		// Call SSMP-core-function:
		switch (operation)
		{
			case readCUdata:// GetECUData(...)
				op_success = GetCUdata(cuaddress, rec_buf, rec_buf+3, rec_buf+8, &datalen);
				if (op_success) datalen += 8;
				break;
			case readBlock:
			case readBlock_p:// ReadDataBlock_permanent(...)
				op_success = ReadDataBlock(cuaddress, padadr, dataadr[0], datalen, rec_buf);
				break;
			case readMulti:
			case readMulti_p:// ReadMultipleDatabytes_permanent(...)
				// CALCULATE NR OF ADDRESSES FOR NEXT READ:
				if (33*(rindex) <= datalen)
					nrofReadAddr = 33;
				else
					nrofReadAddr = datalen%33;
				// READ NEXT ADDRESSES:
				op_success = ReadMultipleDatabytes(cuaddress, padadr, dataadr+((rindex-1)*33), nrofReadAddr, rec_buf+((rindex-1)*33));
				break;
			case writeBlock:
			case writeBlock_p:// WriteDataBlock_permanent(...)
				op_success = WriteDataBlock(cuaddress, dataadr[0], snd_buf, datalen, rec_buf);
				break;
			case writeSingle:
			case writeSingle_p:// WriteDatabyte_permanent(...)
				op_success = WriteDatabyte(cuaddress, dataadr[0], snd_buf[0], rec_buf);
				break;
			default:
				op_success = false;
		}
		// Evaluate result; Prepare for next operation:
		if (op_success)
		{
			// Decrease error counter:
			if (errcount > 0)
				errcount--;
			// Set query-index:
			if ((operation == readMulti_p) && (rindex < static_cast<unsigned int>((datalen/33)+1)) )
				rindex++;
			else
				rindex=1;
			// Send data to main thread:
			if (permanent && (rindex == 1))
			{
				// CONVERT/PREPARE DATA FOR RETURNING
				rawdata = QByteArray::fromRawData ( rec_buf, datalen );
				// GET ELAPSED TIME:
				duration_ms = timer.restart();
				// SEND DATA TO MAIN THREAD:
				emit recievedData(rawdata, duration_ms);
				// Wait for the desired delay time:
				if (delay > 0) msleep(delay);
			}
		}
		else
		{
			errcount++;
			std::cout << "SSMPcommunication::run(...): communication operation error counter=" << (int)(errcount) << '\n';
		}
		// GET ABORT STATUS::
		_mutex.lock();
		abort = _abort;
		_mutex.unlock();
	} while (!abort && (errcount < errmax) && (permanent || (rindex > 1) || !op_success));
	// Send error signal:
	if (permanent && !abort && !op_success)
		emit commError();
	// Synchronise with main-thread, reset:
	_mutex.lock();
	if (!permanent && op_success)
	{
		_datalen = datalen;	// only necessary for getCUdata
		for (k=0; k<datalen; k++) _rec_buf[k] = rec_buf[k];
		_result = op_success;
	}
	_abort = false;
	_mutex.unlock();
	// Ensures that event-loop is started started (should always be the case...)
	while (!_el.isRunning())
	{
		msleep(10);
	}
}


bool SSMPcommunication::getCUdata(char *SYS_ID, char *ROM_ID, char *flagbytes, unsigned char *nrofflagbytes)
{
	bool ok = false;
	int k = 0;
	if ((_CommOperation != noCom) || (_cuaddress == 0)) return false;
	_CommOperation = readCUdata;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Return System-ID:
		SYS_ID[0] = _rec_buf[0];
		SYS_ID[1] = _rec_buf[1];
		SYS_ID[2] = _rec_buf[2];
		// Return ROM-ID
		for (k=0; k<5; k++)
			ROM_ID[k] = _rec_buf[3+k];
		// Return flagbytes:
		for (k=0; k<(_datalen - 8); k++)
			flagbytes[k] = _rec_buf[8+k];
		*nrofflagbytes = _datalen - 8;
	}
	_CommOperation = noCom;
	return ok;
}



bool SSMPcommunication::readDataBlock(char padadr, unsigned int dataadr, unsigned int nrofbytes, char *data)
{
	bool ok = false;
	unsigned int k = 0;
	if (nrofbytes > 254) return false;
	if ((_CommOperation != noCom) || (_cuaddress == 0) || isRunning()) return false;
	_CommOperation = readBlock;
	// Prepare buffers:
	_padadr = padadr;
	_dataadr[0] = dataadr;
	_datalen = nrofbytes;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Assign recieved data:
		for (k=0; k<nrofbytes; k++)
			data[k] = _rec_buf[k];
	}
	_CommOperation = noCom;
	return ok;
}



bool SSMPcommunication::readMultipleDatabytes(char padadr, unsigned int dataadr[256], unsigned int datalen, char *data)
{
	bool ok = false;
	unsigned int k = 0;
	if (datalen > 256) return false;
	if ((_CommOperation != noCom) || (_cuaddress == 0) || isRunning()) return false;
	_CommOperation = readMulti;
	// Prepare buffers:
	_padadr = padadr;
	for (k=0; k<datalen; k++) _dataadr[k] = dataadr[k];
	_datalen = datalen;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Assign recieved data:
		for (k=0; k<datalen; k++)
			data[k] = _rec_buf[k];
	}
	_CommOperation = noCom;
	return ok;
}



bool SSMPcommunication::writeDataBlock(unsigned int dataadr, char *data, unsigned int datalen, char *datawritten)
{
	bool ok = false;
	unsigned int k = 0;
	if (datalen > 251) return false;
	if ((_CommOperation != noCom) || (_cuaddress == 0) || isRunning()) return false;
	_CommOperation = writeBlock;
	// Prepare buffers:
	_dataadr[0] = dataadr;
	for (k=0; k<datalen; k++) _snd_buf[k] = data[k];
	_datalen = datalen;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Check/assign recieved data:
		if (datawritten == NULL)	// do not return actually written data (must be the same as send out !)
		{
			// CHECK IF ACTUALLY WRITTEN DATA IS EQUAL TO THE DATA SENT OUT:
			for (k=0; k<_datalen; k++)
			{
				if (_snd_buf[k] != _rec_buf[k])
				{
					ok = false;
					break;
				}
			}
		}
		else
		{
			// EXTRACT AND RETURN WRITTEN DATA:
			for (k=0; k<_datalen; k++)
				datawritten[k] = _rec_buf[k];
		}
	}
	_CommOperation = noCom;
	return ok;
}



bool SSMPcommunication::writeDatabyte(unsigned int dataadr, char databyte, char *databytewritten)
{
	bool ok = false;
	if ((_CommOperation != noCom) || (_cuaddress == 0) || isRunning()) return false;
	_CommOperation = writeSingle;
	// Prepare buffers:
	_dataadr[0] = dataadr;
	_snd_buf[0] = databyte;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Check/assign recieved data:
		if (databytewritten == NULL)	// do not return actually written data (must be the same as send out !)
		{
			// CHECK IF ACTUALLY WRITTEN DATA IS EQUAL TO THE DATA SENT OUT:
			if (_rec_buf[0] != databyte)
				ok = false;
		}
		else
		{
			// EXTRACT AND RETURN WRITTEN DATA:
			databytewritten[0] = _rec_buf[0];
		}
	}
	_CommOperation = noCom;
	return ok;
}



bool SSMPcommunication::doSingleCommOperation()
{
	connect( this, SIGNAL( finished() ), &_el, SLOT( quit() ) );
	start();
	_el.exec();
	disconnect( this, SIGNAL( finished() ), &_el, SLOT( quit() ) );
	return _result;
}



bool SSMPcommunication::readDataBlock_permanent(char padadr, unsigned int dataadr, unsigned int nrofbytes, int delay)
{
	if (nrofbytes > 254) return false;
	if ((_CommOperation != noCom) || (_cuaddress == 0) || isRunning()) return false;
	_CommOperation = readBlock_p;
	// Prepare buffers:
	_padadr = padadr;
	_dataadr[0] = dataadr;
	_datalen = nrofbytes;
	_delay = delay;
	// Start permanent reading:
	start();
	return isRunning();
}



bool SSMPcommunication::readMultipleDatabytes_permanent(char padadr, unsigned int dataadr[256], unsigned int datalen, int delay)
{
	unsigned int k = 0;
	if (datalen > 256) return false;
	if ((_CommOperation != noCom) || (_cuaddress == 0) || isRunning()) return false;
	_CommOperation = readMulti_p;
	// Prepare buffers:
	_padadr = padadr;
	for (k=0; k<datalen; k++) _dataadr[k] = dataadr[k];
	_datalen = datalen;
	_delay = delay;
	// Start permanent reading:
	start();
	return isRunning();
}



bool SSMPcommunication::writeDataBlock_permanent(unsigned int dataadr, char *data, unsigned int datalen, int delay)
{
	unsigned int k = 0;
	if (datalen > 251) return false;	// currently max. 251 bytes per write possible
	if ((_CommOperation != noCom) || (_cuaddress == 0) || isRunning()) return false;
	_CommOperation = writeBlock_p;
	// Prepare buffers:
	_dataadr[0] = dataadr;
	for (k=0; k<datalen; k++) _snd_buf[k] = data[k];
	_datalen = datalen;
	_delay = delay;
	// Start permanent writing:
	start();
	return isRunning();
}



bool SSMPcommunication::writeDatabyte_permanent(unsigned int dataadr, char databyte, int delay)
{
	if ((_CommOperation != noCom) || (_cuaddress == 0) || isRunning()) return false;
	_CommOperation = writeSingle_p;
	// Prepare buffers:
	_dataadr[0] = dataadr;
	_snd_buf[0] = databyte;
	_datalen = 1;
	_delay = delay;
	// Start permanent writing:
	start();
	return isRunning();
}



SSMPcommunication::com_op_dt SSMPcommunication::getCurrentCommOperation()
{
	return _CommOperation;
}



bool SSMPcommunication::stopCommunication()
{
	if (_CommOperation == noCom)
		return true;
	else
	{
		bool stopped = false;
		QTimer timer;
		connect( this, SIGNAL( finished() ), &_el, SLOT( quit() ) );
		connect( &timer, SIGNAL( timeout() ), &_el, SLOT( quit() ) );
		_mutex.lock();
		_abort = true;
		_mutex.unlock();
		timer.start(5000);
		_el.exec();
		disconnect( &timer, SIGNAL( timeout() ), &_el, SLOT( quit() ) );
		disconnect( this, SIGNAL( finished() ), &_el, SLOT( quit() ) );
		stopped = !isRunning();
		if (!stopped)
		{
			terminate();
			stopped = wait(5000);
		}
		if (stopped)
			_CommOperation = noCom;
		return stopped;
	}
}






/*
TODO:
 THIS CLASS IS SCHEDULED FOR A MAJOR REWRITE:
 => latest SSM-protocol features need complex combined read/write operations
 => "idle"-communication will be implemented for a better detection of control unit disconnects
*/

/*
NOTE: 
 - signal commError() is only emmited for permanent communication operations (success of single operation can be checked with the boolean return value)
 - for all write-operations (calls of SSMPcommunication_core-functions), we always read the bytes that were actually written (even if the public SSMPcommunication-functions are called with a NULL-pointer for the written data). Otherwise, a negative result in run() (op_success == false) could also mean that other data have been written...
 */
