/*
 * SSMP1communication.cpp - Communication Thread for the old SSM-protocol
 *
 * Copyright (C) 2009-2023 Comer352L
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

#include "SSMP1communication.h"

#include <QTimer>
#include <QElapsedTimer>

SSMP1communication::SSMP1communication(AbstractDiagInterface *diagInterface, SSM1_CUtype_dt cu, unsigned char errRetries) : AbstractSSMcommunication(), SSMP1communication_procedures(diagInterface)
{
	_cu = cu;
	_errRetries = errRetries;
	_CommOperation = comOp::noCom;
}


SSMP1communication::~SSMP1communication()
{
	stopCommunication();
}


void SSMP1communication::selectCU(SSM1_CUtype_dt cu)
{
	_cu = cu;
}


SSMP1communication::comOp SSMP1communication::getCurrentCommOperation()
{
	return _CommOperation;
}


bool SSMP1communication::stopCommunication()
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


bool SSMP1communication::getCUdata(unsigned char extradatareqlen, SSMCUdata& cuData)
{
	bool ok = false;
	if ((_CommOperation != comOp::noCom) || isRunning())
		return false;
	_CommOperation = comOp::readRomId;
	_reqsize = extradatareqlen;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		cuData.from_SSMP1(&_data.at(0), _data.size());
	}
	_CommOperation = comOp::noCom;
	return ok;
}


bool SSMP1communication::readAddress(unsigned int addr, char * databyte)
{
	bool ok = false;
	std::vector<char> data;
	ok = readAddresses(std::vector<unsigned int>(1, addr), &data);
	if (ok)
		*databyte = data.at(0);
	return ok;
}


bool SSMP1communication::readAddresses(const std::vector<unsigned int>& addr, std::vector<char> * data)
{
	bool ok = false;
	if ((_CommOperation != comOp::noCom) || isRunning() || (addr.size() == 0))
		return false;
	_CommOperation = comOp::read;
	// Prepare buffers:
	_addresses = addr;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
		*data = _data;
	_CommOperation = comOp::noCom;
	return ok;
}


bool SSMP1communication::writeAddress(unsigned int addr, char databyte, char *databytewritten)
{
	if (databytewritten == NULL)
		return writeAddresses(std::vector<unsigned int>(1, addr), std::vector<char>(1, databyte));
	else
	{
		std::vector<char> databyteswritten;
		bool ok = writeAddresses(std::vector<unsigned int>(1, addr), std::vector<char>(1, databyte), &databyteswritten);
		*databytewritten = databyteswritten.at(0);
		return ok;
	}
}


bool SSMP1communication::writeAddresses(std::vector<unsigned int> addr, std::vector<char> data, std::vector<char> *databyteswritten)
{
	bool ok = false;
	if ((_CommOperation != comOp::noCom) || isRunning()
	    || (addr.size() == 0) || (data.size() == 0) || (addr.size() != data.size()))
		return false;
	_CommOperation = comOp::write;
	// Prepare buffers:
	_addresses = addr;
	_data = data;
	// Communication-operation:
	ok = doSingleCommOperation();
	// Actually written data:
	if (databyteswritten == NULL)
	{
		// Check written data:
		unsigned int k = 0;
		while (ok && (k < _data.size()))
		{
			ok = (_data.at(k) == data.at(k));
			k++;
		}
	}
	else	// Pass written data:
		*databyteswritten = _data;
	_CommOperation = comOp::noCom;
	return ok;
}


bool SSMP1communication::readAddress_permanent(unsigned int addr, int delay)
{
	return readAddresses_permanent(std::vector<unsigned int>(1,addr), delay);
}


bool SSMP1communication::readAddresses_permanent(std::vector<unsigned int> addr, int delay)
{
	if ((_CommOperation != comOp::noCom) || isRunning() || (addr.size() == 0))
		return false;
	_CommOperation = comOp::read_p;
	// Prepare buffers:
	_addresses = addr;
	_delay = delay;
	// Start permanent reading:
	start();
	return isRunning();
}


bool SSMP1communication::writeAddress_permanent(unsigned int addr, char databyte, int delay)
{
	return writeAddresses_permanent(std::vector<unsigned int>(1, addr), std::vector<char>(1, databyte), delay);
}


bool SSMP1communication::writeAddresses_permanent(std::vector<unsigned int> addr, std::vector<char> data, int delay)
{
	if ((_CommOperation != comOp::noCom) || isRunning()
	    || (addr.size() == 0) || (data.size() == 0) || (addr.size() != data.size()))
		return false;
	_CommOperation = comOp::write_p;
	// Prepare buffers:
	_addresses = addr;
	_data = data;
	_delay = delay;
	// Start permanent writing:
	start();
	return isRunning();
}


// PRIVATE

bool SSMP1communication::doSingleCommOperation()
{
	connect( this, SIGNAL( finished() ), &_el, SLOT( quit() ) );
	start();
	_el.exec();
	disconnect( this, SIGNAL( finished() ), &_el, SLOT( quit() ) );
	return _result;
}


void SSMP1communication::run()
{
	QElapsedTimer timer;
	int duration_ms = 0;
	bool permanent = false;
	bool op_success = false;
	bool abort;
	comOp operation;
	SSM1_CUtype_dt cu;
	unsigned char reqsize = 0;
	std::vector<unsigned int> addresses;
	std::vector<char> data;
	std::vector<char> wcdata;
	unsigned int k = 0;
	bool setAddr = true;
	unsigned char errmax = 3;
	char errcount = 0;
	int delay = 0;

	// Synchronise with main-thread:
	_mutex.lock();
	operation = _CommOperation;
	cu = _cu;
	reqsize = _reqsize;
	addresses = _addresses;
	data = _data;
	errmax = _errRetries + 1;
	delay = _delay;
	_result = false;
	_abort = false;
	_mutex.unlock();
#ifdef __FSSM_DEBUG__
	// Debug-output:
	std::string op_str = "SSMP1communication::run():   operation: ";
	switch (operation)
	{
		case comOp::noCom:
			op_str += "noCom";
			break;
		case comOp::readRomId:
			op_str += "readRomId";
			break;
		case comOp::read:
			op_str += "read";
			break;
		case comOp::read_p:
			op_str += "read_p";
			break;
		case comOp::write:
			op_str += "write";
			break;
		case comOp::write_p:
			op_str += "write_p";
			break;
		default:
			op_str += "INVALID/UNKNOWN: " + QString::number( static_cast<int>(operation) ).toStdString();
	}
	std::cout << op_str << '\n';
#endif
	// Preparation:
	if ( operation == comOp::read_p || operation == comOp::write_p )
	{
		permanent = true;
		timer.start();
	}
	if ((operation == comOp::write) || (operation == comOp::write_p))
		wcdata = data;
	if (operation == comOp::readRomId)
	{
		addresses.clear();
		if (cu == SSM1_CU_FourWS)
			addresses.push_back(0xffff);
		else
			addresses.push_back(0x8000);
			// NOTE: for old ECUs 0x0000 has been used, but apparently they accept at least 0x8000, too (address is likely ignored)
	}
	// COMMUNICATION:
	do
	{
		// Set next address:
		if (setAddr)
		{
			op_success = setAddress(cu, addresses.at(k));
			setAddr = !op_success;
#ifdef __FSSM_DEBUG__
			if (!op_success)
				std::cout << "SSMP1communication::run():   setAddress(...) failed for address 0x" << std::hex << addresses.at(k) << " !\n";
#endif
		}
		if (!setAddr)	// only if there was no error during address-setting
		{
			// Do communication operation for the current address:
			if ((operation == comOp::read) || (operation == comOp::read_p))
			{
				// Read next data:
				if (k == 0)
					data.clear();
				op_success = getNextData(&data);
#ifdef __FSSM_DEBUG__
				if (!op_success)
					std::cout << "SSMP1communication::run():   getNextData(...) failed !\n";
#endif
			}
			else if ((operation == comOp::write) || (operation == comOp::write_p))
			{
				// Write next data:
				if (writeDatabyte(data.at(k)))
				{
					wcdata.at(k) = waitForDataValue(data.at(k));
					op_success = true;
				}
				else
				{
					op_success = false;
#ifdef __FSSM_DEBUG__
					std::cout << "SSMP1communication::run():   writeDatabyte(...) failed !\n";
#endif
				}
			}
			else if (operation == comOp::readRomId)
			{
				// Get ROM-ID:
				op_success = getID(addresses.at(0), reqsize, &data);
			}
		}
		// Evaluate result; Prepare for next operation:
		if (op_success)
		{
			// Decrease error counter:
			if (errcount > 0)
				errcount--;
			// Set next address index:
			if (addresses.size() > 1)
			{
				if (k < (addresses.size() - 1))
					k++;
				else
					k = 0;
				setAddr = true;
			}
			// Send data to main thread:
			if (permanent && (k == 0))
			{
				// GET ELAPSED TIME:
				duration_ms = timer.restart();
				// SEND DATA TO MAIN THREAD:
				emit receivedData(data, duration_ms);
				// Wait for the desired delay time:
				if (delay > 0)
					msleep(delay);
			}
		}
		else
		{
#ifdef __FSSM_DEBUG__
			std::cout << "SSMP1communication::run():   communication operation error counter=" << std::dec << (int)(errcount) << '\n';
#endif
			errcount++;
			setAddr = true;	// repeat the complete procedure
			if (errcount < errmax)
				msleep(100);
		}
		// GET ABORT STATUS::
		_mutex.lock();
		abort = _abort;
		_mutex.unlock();
	} while (!abort && (errcount < errmax) && (permanent || (k > 0) || !op_success));
	// Try to stop control unit from permanent data sending:
#ifndef __FSSM_DEBUG__
	stopCUtalking(true);
#else
	if (!stopCUtalking(true))
		std::cout << "SSMP1communication::run():   stopCUtalking failed !\n";
#endif
	// Send error signal:
	if (permanent && !abort && !op_success)
		emit commError();
	// Synchronise with main-thread, reset:
	_mutex.lock();
	if (!permanent && op_success)
	{
		if (operation == comOp::write)
			_data = wcdata;
		else
			_data = data;
		_result = op_success;
	}
	_abort = false;
	_mutex.unlock();
	if (!permanent)
	{
		// Ensure that event-loop is already started (should always be the case...)
		while (!_el.isRunning())
			msleep(10);
	}
	else
		_CommOperation = comOp::noCom;
#ifdef __FSSM_DEBUG__
	std::cout << "SSMP1communication::run():   communication operation finished.\n";
#endif
}


