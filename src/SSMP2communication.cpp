/*
 * SSMP2communication.cpp - Communication Thread for the new SSM-protocol
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


#include "SSMP2communication.h"

#include <QTimer>
#include <QElapsedTimer>


SSMP2communication::SSMP2communication(AbstractDiagInterface *diagInterface, unsigned int cuaddress, unsigned char errRetries) : AbstractSSMcommunication(), SSMP2communication_core(diagInterface)
{
	_cuaddress = cuaddress;
	_CommOperation = comOp::noCom;
	_errRetries = errRetries;
	_padaddr = 0;
	_datalen = 0;
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


SSMP2communication::comOp SSMP2communication::getCurrentCommOperation()
{
	return _CommOperation;
}


bool SSMP2communication::stopCommunication()
{
	if (_CommOperation == comOp::noCom)
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
			_CommOperation = comOp::noCom;
		return stopped;
	}
}


bool SSMP2communication::getCUdata(SSMCUdata& cuData)
{
	cuData.clear();
	bool ok = false;
	if ((_CommOperation != comOp::noCom) || (_cuaddress == 0))
		return false;
	_CommOperation = comOp::readCUdata;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
		cuData.from_SSMP2(_rec_buf.data(), _rec_buf.size());
	_CommOperation = comOp::noCom;
	return ok;
}


#ifdef __SSM2_BLOCK_OPS__
bool SSMP2communication::readDataBlock(char padaddr, unsigned int dataaddr, unsigned int nrofbytes, std::vector<char> *data)
{
	bool ok = false;

	if ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230) && (nrofbytes > 254)) // ISO14230 protocol limit: length byte in header => max. 254 per reply message possible
	{
		return false;
	}
	else if ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO15765) && (nrofbytes > 256)) // ISO15765 protocol limit: data length byte in request => max. 256 possible
	{
		return false;
	}
	else
	{
		return false;
	}
	if ((_CommOperation != comOp::noCom) || (_cuaddress == 0) || isRunning())
		return false;
	_CommOperation = comOp::readBlock;
	// Prepare buffers:
	_padaddr = padaddr;
	_dataaddr.assign(1, dataaddr);
	_datalen = nrofbytes;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Assign received data:
		*data = _rec_buf;
	}
	_CommOperation = comOp::noCom;
	return ok;
}
#endif


bool SSMP2communication::readMultipleDatabytes(char padaddr, const std::vector<unsigned int> dataaddr, std::vector<char> *data)
{
	bool ok = false;
	if (data == NULL)
		return false;
	data->clear();
	if (dataaddr.size() < 1)
		return false; // limited by buffer sizes
	if ((_CommOperation != comOp::noCom) || (_cuaddress == 0) || isRunning())
		return false;
	_CommOperation = comOp::readMulti;
	// Prepare buffers:
	_padaddr = padaddr;
	_dataaddr = dataaddr;
	_datalen = dataaddr.size();
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Assign received data:
		*data = _rec_buf;
	}
	_CommOperation = comOp::noCom;
	return ok;
}


#ifdef __SSM2_BLOCK_OPS__
bool SSMP2communication::writeDataBlock(const unsigned int dataaddr, const std::vector<char> data, std::vector<char> *datawritten)
{
	bool ok = false;
	if ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230) && (data.size() > 251)) // ISO14230 protocol limit: length byte => max. 255-4 = 251 data bytes per request message possible
		return false;
	if ((_CommOperation != comOp::noCom) || (_cuaddress == 0) || isRunning())
		return false;
	_CommOperation = comOp::writeBlock;
	// Prepare buffers:
	_dataaddr.assign(1, dataaddr);
	_snd_buf = data;
	_datalen = data.size();
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Check/assign received data:
		if (datawritten == NULL)	// do not return actually written data (must be the same as send out !)
		{
			// CHECK IF ACTUALLY WRITTEN DATA IS EQUAL TO THE DATA SENT OUT:
			ok = (_rec_buf == data);
		}
		else
		{
			// EXTRACT AND RETURN WRITTEN DATA:
			*datawritten = _rec_buf;
		}
	}
	_CommOperation = comOp::noCom;
	return ok;
}
#endif


bool SSMP2communication::writeDatabyte(const unsigned int dataaddr, const char databyte, char* databytewritten)
{
	bool ok = false;
	if ((_CommOperation != comOp::noCom) || (_cuaddress == 0) || isRunning())
		return false;
	_CommOperation = comOp::writeSingle;
	// Prepare buffers:
	_dataaddr.assign(1, dataaddr);
	_snd_buf.assign(1, databyte);
	_datalen = 1;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Check/assign received data:
		if (databytewritten == NULL)	// do not return actually written data (must be the same as send out !)
		{
			// CHECK IF ACTUALLY WRITTEN DATA IS EQUAL TO THE DATA SENT OUT:
			if (_rec_buf.at(0) != databyte)
				ok = false;
		}
		else
		{
			// EXTRACT AND RETURN WRITTEN DATA:
			*databytewritten = _rec_buf.at(0);
		}
	}
	_CommOperation = comOp::noCom;
	return ok;
}


bool SSMP2communication::writeMultipleDatabytes(const std::vector<unsigned int> dataaddr, std::vector<char> data, std::vector<char> *datawritten)
{
	// NOTE: emulates multi-address/-data write using multiple signgle address/data writes
	bool ok = false;
	if (datawritten != NULL)
		datawritten->clear();
	if ((dataaddr.size() < 1) || (dataaddr.size() != data.size()))
		return false; // limited by buffer sizes
	if ((_CommOperation != comOp::noCom) || (_cuaddress == 0) || isRunning())
		return false;
	_CommOperation = comOp::writeMultiAddr_emul;
	// Prepare buffers:
	_dataaddr = dataaddr;
	_snd_buf = data;
	_datalen = dataaddr.size();
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Check/assign received data:
		if (datawritten == NULL)	// do not return actually written data (must be the same as send out !)
		{
			for (size_t b = 0; b < dataaddr.size(); b++)
			{
				// CHECK IF ACTUALLY WRITTEN DATA IS EQUAL TO THE DATA SENT OUT:
				if (_rec_buf[b] != data.at(b))
				{
					ok = false;
					break;
				}
			}
		}
		else
		{
			// EXTRACT AND RETURN WRITTEN DATA:
			*datawritten = _rec_buf;
		}
	}
	_CommOperation = comOp::noCom;
	return ok;
}


#ifdef __SSM2_BLOCK_OPS__
bool SSMP2communication::readDataBlock_permanent(char padaddr, unsigned int dataaddr, unsigned int nrofbytes, int delay)
{
	if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
	{
		if (nrofbytes > 254) // ISO14230 protocol limit: length byte in header => max. 254 per reply message possible
			return false;
	}
	else if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO15765)
	{
		if (nrofbytes > 256) // ISO15765 protocol limit: data length byte in request => max. 256 possible
			return false;
	}
	else
		return false;
	if ((_CommOperation != comOp::noCom) || (_cuaddress == 0) || isRunning())
		return false;
	_CommOperation = comOp::readBlock_p;
	// Prepare buffers:
	_padaddr = padaddr;
	_dataaddr.assign(1, dataaddr);
	_datalen = nrofbytes;
	_delay = delay;
	// Start permanent reading:
	start();
	return isRunning();
}
#endif


bool SSMP2communication::readMultipleDatabytes_permanent(const char padaddr, const std::vector<unsigned int> dataaddr, const int delay)
{
	if (dataaddr.size() < 1)
		return false; // limited by buffer sizes
	if ((_CommOperation != comOp::noCom) || (_cuaddress == 0) || isRunning())
		return false;
	_CommOperation = comOp::readMulti_p;
	// Prepare buffers:
	_padaddr = padaddr;
	_dataaddr = dataaddr;
	_datalen = dataaddr.size();
	_delay = delay;
	// Start permanent reading:
	start();
	return isRunning();
}


#ifdef __SSM2_BLOCK_OPS__
bool SSMP2communication::writeDataBlock_permanent(const unsigned int dataaddr, const std::vector<char> data, const int delay)
{
	if ((_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230) && (data.size() > 251)) // ISO14230 protocol limit: length byte => max. 255-4 = 251 data bytes per request message possible
		return false;
	if ((_CommOperation != comOp::noCom) || (_cuaddress == 0) || isRunning())
		return false;
	_CommOperation = comOp::writeBlock_p;
	// Prepare buffers:
	_dataaddr.assign(1, dataaddr);
	_snd_buf = data;
	_datalen = data.size();
	_delay = delay;
	// Start permanent writing:
	start();
	return isRunning();
}
#endif


bool SSMP2communication::writeDatabyte_permanent(const unsigned int dataaddr, const char databyte, const int delay)
{
	if ((_CommOperation != comOp::noCom) || (_cuaddress == 0) || isRunning())
		return false;
	_CommOperation = comOp::writeSingle_p;
	// Prepare buffers:
	_dataaddr.assign(1, dataaddr);
	_snd_buf.assign(1, databyte);
	_datalen = 1;
	_delay = delay;
	// Start permanent writing:
	start();
	return isRunning();
}


bool SSMP2communication::writeMultipleDatabytes_permanent(const std::vector<unsigned int> dataaddr, const std::vector<char> data, const int delay)
{
	// NOTE: emulates multi-address/-data write using multiple signgle address/data writes
	if ((dataaddr.size() < 1) || (dataaddr.size() != data.size()))
		return false; // limited by buffer sizes
	if ((_CommOperation != comOp::noCom) || (_cuaddress == 0) || isRunning())
		return false;
	_CommOperation = comOp::writeMultiAddr_emul_p;
	// Prepare buffers:
	_dataaddr = dataaddr;
	_snd_buf = data;
	_datalen = dataaddr.size();
	_delay = delay;
	// Start permanent reading:
	start();
	return isRunning();
}


// ----- Abstract communication interface -----

bool SSMP2communication::readAddress(unsigned int addr, char * databyte)
{
	std::vector<char> databytes;
	if (readMultipleDatabytes('\x00', std::vector<unsigned int>(1, addr), &databytes))
	{
		*databyte = databytes.at(0);
		return true;
	}
	return false;
}


bool SSMP2communication::readAddresses(const std::vector<unsigned int>& addr, std::vector<char> * data)
{
	return readMultipleDatabytes('\x00', addr, data);
}


bool SSMP2communication::readAddress_permanent(unsigned int addr, int delay)
{
	return readMultipleDatabytes_permanent('\x00', std::vector<unsigned int>(1, addr), delay);
}


bool SSMP2communication::readAddresses_permanent(std::vector<unsigned int> addr, int delay)
{
	return readMultipleDatabytes_permanent('\x00', addr, delay);
}


bool SSMP2communication::writeAddress(unsigned int addr, char databyte, char *databytewritten)
{
	return writeDatabyte(addr, databyte, databytewritten);
}


bool SSMP2communication::writeAddresses(std::vector<unsigned int> addr, std::vector<char> data, std::vector<char> *databyteswritten)
{
	if (databyteswritten != NULL)
		databyteswritten->clear();
	if (addr.size() != data.size())
		return false;
	if (addr.size() > 1)
		return writeMultipleDatabytes(addr, data, databyteswritten);
	else if (addr.size() == 1)
	{
		char bw;
		if (writeDatabyte(addr.at(0), data.at(0), &bw))
		{
			if (databyteswritten != NULL)
				databyteswritten->assign(1, bw);
			return true;
		}
		else
			return false;
	}
	return false;
}


bool SSMP2communication::writeAddress_permanent(unsigned int addr, char databyte, int delay)
{
	return writeDatabyte_permanent(addr, databyte, delay);
}


bool SSMP2communication::writeAddresses_permanent(std::vector<unsigned int> addr, std::vector<char> data, int delay)
{
	if (addr.size() != data.size())
		return false;
	if (addr.size() > 1)
		return writeMultipleDatabytes_permanent(addr, data, delay);
	else
		return writeDatabyte_permanent(addr.at(0), data.at(0), delay);
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
	QElapsedTimer timer;
	int duration_ms = 0;
	unsigned int op_idx = 0;
	unsigned char nrofReadAddr = 0;
	char errcount = 0;
	bool permanent = false;
	Result op_success = Result::error;
	bool abort;
	comOp operation;
	unsigned int cuaddress;
	char padaddr = '\x0';
	unsigned char datalen = 0;
	std::vector<unsigned int> dataaddr;
	std::vector<char> snd_buf;
	std::vector<char> rec_buf;
	int delay = 0;
	unsigned char errmax = 3;
	const unsigned int max_bytes_per_multiread = 33;

	// Synchronise with main-thread:
	_mutex.lock();
	operation = _CommOperation;
	cuaddress = _cuaddress;
	padaddr = _padaddr;
	datalen = _datalen;
	dataaddr = _dataaddr;
	snd_buf = _snd_buf;
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
		case comOp::noCom:
			op_str += "noCom";
			break;
		case comOp::readCUdata:
			op_str += "readCUdata";
			break;
#ifdef __SSM2_BLOCK_OPS__
		case comOp::readBlock:
			op_str += "readBlock";
			break;
#endif
		case comOp::readMulti:
			op_str += "readMulti";
			break;
#ifdef __SSM2_BLOCK_OPS__
		case comOp::readBlock_p:
			op_str += "readBlock_p";
			break;
#endif
		case comOp::readMulti_p:
			op_str += "readMulti_p";
			break;
#ifdef __SSM2_BLOCK_OPS__
		case comOp::writeBlock:
			op_str += "writeBlock";
			break;
#endif
		case comOp::writeSingle:
			op_str += "writeSingle";
			break;
		case comOp::writeMultiAddr_emul:
			op_str += "writeMulti_emulated";
			break;
#ifdef __SSM2_BLOCK_OPS__
		case comOp::writeBlock_p:
			op_str += "writeBlock_p";
			break;
#endif
		case comOp::writeSingle_p:
			op_str += "writeSingle_p";
			break;
		case comOp::writeMultiAddr_emul_p:
			op_str += "writeMulti_emulated_p";
			break;
		default:
			op_str += "INVALID/UNKNOWN: " + QString::number( static_cast<int>(operation) ).toStdString();
	}
	std::cout << op_str << '\n';
#endif
	// Preparation:
#ifdef __SSM2_BLOCK_OPS__
	if ( (operation == comOp::readBlock_p) || (operation == comOp::readMulti_p)
	     || (operation == comOp::writeBlock_p) || (operation == comOp::writeSingle_p) || (operation == comOp::writeMultiAddr_emul_p) )
#else
	if ( (operation == comOp::readMulti_p) || (operation == comOp::writeSingle_p) || (operation == comOp::writeMultiAddr_emul_p) )
#endif
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
			case comOp::readCUdata:// GetECUData(...)
				op_success = GetCUdata(cuaddress, &rec_buf);
				break;
#ifdef __SSM2_BLOCK_OPS__
			case comOp::readBlock:
			case comOp::readBlock_p:// ReadDataBlock_permanent(...)
				op_success = ReadDataBlock(cuaddress, padaddr, dataaddr.at(0), datalen, &rec_buf);
				break;
#endif
			case comOp::readMulti:
			case comOp::readMulti_p:// ReadMultipleDatabytes_permanent(...)
			{
				// CALCULATE NR OF ADDRESSES FOR NEXT READ:
				if ((max_bytes_per_multiread * (op_idx + 1)) <= datalen)
					nrofReadAddr = max_bytes_per_multiread;
				else
					nrofReadAddr = datalen % max_bytes_per_multiread;

				// READ NEXT ADDRESSES:
				std::vector<unsigned int> rd_addrs( dataaddr.begin() + (op_idx * max_bytes_per_multiread), dataaddr.begin() + (op_idx * max_bytes_per_multiread) + nrofReadAddr );
				std::vector<char> rd_buf;
				op_success = ReadMultipleDatabytes(cuaddress, padaddr, rd_addrs, &rd_buf);
				if (op_success == Result::success)
					rec_buf.insert(rec_buf.end(), rd_buf.begin(), rd_buf.end());
				break;
			}
#ifdef __SSM2_BLOCK_OPS__
			case comOp::writeBlock:
			case comOp::writeBlock_p:// WriteDataBlock_permanent(...)
				op_success = WriteDataBlock(cuaddress, dataaddr.at(0), snd_buf, &rec_buf);
				break;
#endif
			case comOp::writeSingle:
			case comOp::writeSingle_p:// WriteDatabyte_permanent(...)
			case comOp::writeMultiAddr_emul:
			case comOp::writeMultiAddr_emul_p:
				rec_buf.resize(1);
				op_success = WriteDatabyte(cuaddress, dataaddr.at(op_idx), snd_buf.at(op_idx), &rec_buf[0]);
				break;
			default:
				op_success = Result::error;
		}
		// Evaluate result; Prepare for next operation:
		if (op_success == Result::success)
		{
			// Decrease error counter:
			if (errcount > 0)
				errcount--;
			// Set query-index:
			bool report_recdata = false;
			if ( ((operation == comOp::readMulti) || (operation == comOp::readMulti_p))
			     && ((op_idx + 1) < static_cast<unsigned int>(((datalen - 1) / max_bytes_per_multiread) + 1)) )
				op_idx++;
			else if ( ((operation == comOp::writeMultiAddr_emul) || (operation == comOp::writeMultiAddr_emul_p))
				  && ((op_idx + 1) < datalen) )
				op_idx++;
			else
			{
				op_idx = 0;
				report_recdata = true;
			}
			// Send data to main thread:
			if (permanent && report_recdata)
			{
				// Get elapsed time:
				duration_ms = timer.restart();
				// Send data to main thread and clear receive buffer:
				emit receivedData(rec_buf, duration_ms);
				rec_buf.clear();
				// Wait for the desired delay time:
				if (delay > 0)
					msleep(delay);
			}
		}
		else if (op_success == Result::rejected)
		{
			errcount = errmax; // no need to retry
#ifdef __FSSM_DEBUG__
			std::cout << "SSMP2communication::run():   communication operation rejected by ECU\n";
#endif
		}
		else // Result::error
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
	} while (!abort && (errcount < errmax) && (permanent || (op_idx > 0) || (op_success != Result::success)));
	// Send error signal:
	if (permanent && !abort) // implies error
		emit commError();
	// Synchronise with main-thread, reset:
	_mutex.lock();
	if (!permanent && (op_success == Result::success))
	{
		_datalen = datalen;	// only necessary for getCUdata
		_rec_buf = rec_buf;
	}
	_result = (op_success == Result::success);
	_abort = false;
	_mutex.unlock();
	if (!permanent)
	{
		// Ensures that event-loop is started before finishing (should always be the case...)
		while (!_el.isRunning())
			msleep(10);
	}
	else
		_CommOperation = comOp::noCom;
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
 - for all write-operations (calls of SSMPcommunication_core-functions), we always read the bytes that were actually written (even if the public SSMPcommunication-functions are called with a NULL-pointer for the written data).
   Otherwise, a negative result in run() (op_success == false) could also mean that other data have been written...
 */
