/*
 * SSMP2communication.cpp - Communication Thread for the new SSM-protocol
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


#include "SSMP2communication.h"


SSMP2communication::SSMP2communication(AbstractDiagInterface *diagInterface, unsigned int cuaddress, unsigned char errRetries) : QThread(), SSMP2communication_core(diagInterface)
{
	_cuaddress = cuaddress;
	_CommOperation = comOp_noCom;
	_result = false;
	_abort = false;
	_errRetries = errRetries;
	_padaddr = 0;
	_datalen = 0;
	_delay = 0;
	std::fill_n(_dataaddr, SSMP2COM_BUFFER_SIZE, 0);
	std::fill_n(_rec_buf, SSMP2COM_BUFFER_SIZE, 0);
	std::fill_n(_snd_buf, SSMP2COM_BUFFER_SIZE, 0);
}



SSMP2communication::~SSMP2communication()
{
	stopCommunication();
	disconnect( this, SIGNAL( finished() ), 0, 0 );
}



void SSMP2communication::setCUaddress(unsigned int cuaddress)
{
	_cuaddress = cuaddress;
}



void SSMP2communication::setRetriesOnError(unsigned char retries)
{
	_errRetries = retries;
}



bool SSMP2communication::getCUdata(SSMCUdata& cuData)
{
	cuData.clear();
	bool ok = false;
	if ((_CommOperation != comOp_noCom) || (_cuaddress == 0)) return false;
	_CommOperation = comOp_readCUdata;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
		cuData.from_SSMP2(_rec_buf, _datalen);
	_CommOperation = comOp_noCom;
	return ok;
}



bool SSMP2communication::readDataBlock(char padaddr, unsigned int dataaddr, unsigned int nrofbytes, char *data)
{
	bool ok = false;

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
	if ((_CommOperation != comOp_noCom) || (_cuaddress == 0) || isRunning())
		return false;
	_CommOperation = comOp_readBlock;
	// Prepare buffers:
	_padaddr = padaddr;
	_dataaddr[0] = dataaddr;
	_datalen = nrofbytes;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Assign received data:
		std::copy(_rec_buf, _rec_buf + nrofbytes, data);
	}
	_CommOperation = comOp_noCom;
	return ok;
}



bool SSMP2communication::readMultipleDatabytes(char padaddr, const unsigned int dataaddr[SSMP2COM_BUFFER_SIZE], unsigned int datalen, char *data)
{
	bool ok = false;
	if (datalen > SSMP2COM_BUFFER_SIZE) return false; // limited by buffer sizes
	if ((_CommOperation != comOp_noCom) || (_cuaddress == 0) || isRunning()) return false;
	_CommOperation = comOp_readMulti;
	// Prepare buffers:
	_padaddr = padaddr;
	std::copy(dataaddr, dataaddr + datalen, _dataaddr);
	_datalen = datalen;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Assign received data:
		std::copy(_rec_buf, _rec_buf + datalen, data);
	}
	_CommOperation = comOp_noCom;
	return ok;
}



bool SSMP2communication::writeDataBlock(const unsigned int dataaddr, const char* data, const unsigned int datalen, char* datawritten)
{
	bool ok = false;
	if ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230) && (datalen > 251)) // ISO14230 protocol limit: length byte => max. 255-4 = 251 data bytes per request message possible
	{
		return false;
	}
	else if (datalen > SSMP2COM_BUFFER_SIZE) // limited by buffer sizes
	{
		return false;
	}
	if ((_CommOperation != comOp_noCom) || (_cuaddress == 0) || isRunning())
		return false;
	_CommOperation = comOp_writeBlock;
	// Prepare buffers:
	_dataaddr[0] = dataaddr;
	std::copy(data, data + datalen, _snd_buf);
	_datalen = datalen;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Check/assign received data:
		if (datawritten == NULL)	// do not return actually written data (must be the same as send out !)
		{
			// CHECK IF ACTUALLY WRITTEN DATA IS EQUAL TO THE DATA SENT OUT:
			ok = !memcmp(_rec_buf, data, datalen);
		}
		else
		{
			// EXTRACT AND RETURN WRITTEN DATA:
			std::copy(_rec_buf, _rec_buf + _datalen, datawritten);
		}
	}
	_CommOperation = comOp_noCom;
	return ok;
}



bool SSMP2communication::writeDatabyte(const unsigned int dataaddr, const char databyte, char* databytewritten)
{
	bool ok = false;
	if ((_CommOperation != comOp_noCom) || (_cuaddress == 0) || isRunning()) return false;
	_CommOperation = comOp_writeSingle;
	// Prepare buffers:
	_dataaddr[0] = dataaddr;
	_snd_buf[0] = databyte;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Check/assign received data:
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
	_CommOperation = comOp_noCom;
	return ok;
}



bool SSMP2communication::readDataBlock_permanent(char padaddr, unsigned int dataaddr, unsigned int nrofbytes, int delay)
{
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
	if ((_CommOperation != comOp_noCom) || (_cuaddress == 0) || isRunning())
		return false;
	_CommOperation = comOp_readBlock_p;
	// Prepare buffers:
	_padaddr = padaddr;
	_dataaddr[0] = dataaddr;
	_datalen = nrofbytes;
	_delay = delay;
	// Start permanent reading:
	start();
	return isRunning();
}



bool SSMP2communication::readMultipleDatabytes_permanent(const char padaddr, const unsigned int dataaddr[SSMP2COM_BUFFER_SIZE], const unsigned int datalen, const int delay)
{
	if (datalen > SSMP2COM_BUFFER_SIZE) return false; // limited by buffer sizes
	if ((_CommOperation != comOp_noCom) || (_cuaddress == 0) || isRunning()) return false;
	_CommOperation = comOp_readMulti_p;
	// Prepare buffers:
	_padaddr = padaddr;
	std::copy(dataaddr, dataaddr + datalen, _dataaddr);
	_datalen = datalen;
	_delay = delay;
	// Start permanent reading:
	start();
	return isRunning();
}



bool SSMP2communication::writeDataBlock_permanent(const unsigned int dataaddr, const char* data, const unsigned int datalen, const int delay)
{
	if ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230) && (datalen > 251)) // ISO14230 protocol limit: length byte => max. 255-4 = 251 data bytes per request message possible
	{
		return false;
	}
	else if (datalen > SSMP2COM_BUFFER_SIZE) // limited by buffer sizes
	{
		return false;
	}
	if ((_CommOperation != comOp_noCom) || (_cuaddress == 0) || isRunning())
		return false;
	_CommOperation = comOp_writeBlock_p;
	// Prepare buffers:
	_dataaddr[0] = dataaddr;
	std::copy(data, data + datalen, _snd_buf);
	_datalen = datalen;
	_delay = delay;
	// Start permanent writing:
	start();
	return isRunning();
}



bool SSMP2communication::writeDatabyte_permanent(const unsigned int dataaddr, const char databyte, const int delay)
{
	if ((_CommOperation != comOp_noCom) || (_cuaddress == 0) || isRunning()) return false;
	_CommOperation = comOp_writeSingle_p;
	// Prepare buffers:
	_dataaddr[0] = dataaddr;
	_snd_buf[0] = databyte;
	_datalen = 1;
	_delay = delay;
	// Start permanent writing:
	start();
	return isRunning();
}



SSMP2communication::comOp_dt SSMP2communication::getCurrentCommOperation()
{
	return _CommOperation;
}



bool SSMP2communication::stopCommunication()
{
	if (_CommOperation == comOp_noCom)
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
			_CommOperation = comOp_noCom;
		return stopped;
	}
}

// PRIVATE:

bool SSMP2communication::doSingleCommOperation()
{
	connect( this, SIGNAL( finished() ), &_el, SLOT( quit() ) );
	start();
	_el.exec();
	disconnect( this, SIGNAL( finished() ), &_el, SLOT( quit() ) );
	return _result;
}


void SSMP2communication::run()
{
	std::vector<char> rawdata;
	QElapsedTimer timer;
	int duration_ms = 0;
	unsigned int rindex = 1;
	unsigned char nrofReadAddr = 0;
	char errcount = 0;
	bool permanent = false;
	bool op_success = false;
	bool abort;
	comOp_dt operation;
	unsigned int cuaddress;
	char padaddr = '\x0';
	unsigned char datalen = 0;
	unsigned int dataaddr[SSMP2COM_BUFFER_SIZE] = {0};
	char snd_buf[SSMP2COM_BUFFER_SIZE] = {'\x0'};
	char rec_buf[SSMP2COM_BUFFER_SIZE] = {'\x0'};
	int delay = 0;
	unsigned char errmax = 3;
	const unsigned int max_bytes_per_multiread = 33;

	// Synchronise with main-thread:
	_mutex.lock();
	operation = _CommOperation;
	cuaddress = _cuaddress;
	padaddr = _padaddr;
	datalen = _datalen;
	std::copy(_dataaddr, _dataaddr + datalen, dataaddr);
	std::copy(_snd_buf, _snd_buf + datalen, snd_buf);
	delay = _delay;
	errmax = _errRetries + 1;
	_result = false;
	_abort = false;
	_mutex.unlock();
#ifdef __FSSM_DEBUG__
	// Debug-output:
	std::string op_str = "SSMP2communication::run():   operation: ";
	switch (operation)
	{
		case comOp_noCom:
			op_str += "noCom";
			break;
		case comOp_readCUdata:
			op_str += "readCUdata";
			break;
		case comOp_readBlock:
			op_str += "readBlock";
			break;
		case comOp_readMulti:
			op_str += "readMulti";
			break;
		case comOp_readBlock_p:
			op_str += "readBlock_p";
			break;
		case comOp_readMulti_p:
			op_str += "readMulti_p";
			break;
		case comOp_writeBlock:
			op_str += "writeBlock";
			break;
		case comOp_writeSingle:
			op_str += "writeSingle";
			break;
		case comOp_writeBlock_p:
			op_str += "writeBlock_p";
			break;
		case comOp_writeSingle_p:
			op_str += "writeSingle_p";
			break;
		default:
			op_str += "INVALID/UNKNOWN: " + QString::number(operation).toStdString();
	}
	std::cout << op_str << '\n';
#endif
	// Preparation:
	if ( operation==comOp_readBlock_p || operation==comOp_readMulti_p || operation==comOp_writeBlock_p || operation==comOp_writeSingle_p )
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
			case comOp_readCUdata:// GetECUData(...)
				op_success = GetCUdata(cuaddress, rec_buf, &datalen);
				break;
			case comOp_readBlock:
			case comOp_readBlock_p:// ReadDataBlock_permanent(...)
				op_success = ReadDataBlock(cuaddress, padaddr, dataaddr[0], datalen, rec_buf);
				break;
			case comOp_readMulti:
			case comOp_readMulti_p:// ReadMultipleDatabytes_permanent(...)
				// CALCULATE NR OF ADDRESSES FOR NEXT READ:
				if (max_bytes_per_multiread*(rindex) <= datalen)
					nrofReadAddr = max_bytes_per_multiread;
				else
					nrofReadAddr = datalen % max_bytes_per_multiread;
				// READ NEXT ADDRESSES:
				op_success = ReadMultipleDatabytes(cuaddress, padaddr, dataaddr+((rindex-1)*max_bytes_per_multiread), nrofReadAddr, rec_buf+((rindex-1)*max_bytes_per_multiread));
				break;
			case comOp_writeBlock:
			case comOp_writeBlock_p:// WriteDataBlock_permanent(...)
				op_success = WriteDataBlock(cuaddress, dataaddr[0], snd_buf, datalen, rec_buf);
				break;
			case comOp_writeSingle:
			case comOp_writeSingle_p:// WriteDatabyte_permanent(...)
				op_success = WriteDatabyte(cuaddress, dataaddr[0], snd_buf[0], rec_buf);
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
			if ( ((operation == comOp_readMulti) || (operation == comOp_readMulti_p)) && (rindex < static_cast<unsigned int>(((datalen-1)/max_bytes_per_multiread)+1)) )
				rindex++;
			else
				rindex=1;
			// Send data to main thread:
			if (permanent && (rindex == 1))
			{
				// CONVERT/PREPARE DATA FOR RETURNING
				rawdata = std::vector<char>(rec_buf, rec_buf+datalen);
				// GET ELAPSED TIME:
				duration_ms = timer.restart();
				// SEND DATA TO MAIN THREAD:
				emit receivedData(rawdata, duration_ms);
				// Wait for the desired delay time:
				if (delay > 0) msleep(delay);
			}
		}
		else
		{
			errcount++;
#ifdef __FSSM_DEBUG__
			std::cout << "SSMP2communication::run():   communication operation error counter=" << (int)(errcount) << '\n';
#endif
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
		std::copy(rec_buf, rec_buf + datalen, _rec_buf);
		_result = op_success;
	}
	_abort = false;
	_mutex.unlock();
	if (!permanent)
	{
		// Ensures that event-loop is started before finishing (should always be the case...)
		while (!_el.isRunning())
			msleep(10);
	}
	else
		_CommOperation = comOp_noCom;
#ifdef __FSSM_DEBUG__
	std::cout << "SSMP2communication::run():   communication operation finished." << '\n';
#endif
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
