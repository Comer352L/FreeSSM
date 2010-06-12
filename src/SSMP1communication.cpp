/*
 * SSMP1communication.cpp - Communication Thread for the old SSM-protocol
 *
 * Copyright (C) 2009-2010 Comer352l
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


SSMP1communication::SSMP1communication(AbstractDiagInterface *diagInterface, SSM1_CUtype_dt cu, unsigned char errRetries) : QThread(), SSMP1communication_procedures(diagInterface)
{
	_cu = cu;
	_errRetries = errRetries;
	_CommOperation = comOp_noCom;
	_result = false;
	_abort = false;
}


SSMP1communication::~SSMP1communication()
{
	stopCommunication();
}


void SSMP1communication::selectCU(SSM1_CUtype_dt cu)
{
	_cu = cu;
}


void SSMP1communication::setRetriesOnError(unsigned char retries)
{
	_errRetries = retries;
}


SSMP1communication::comOp_dt SSMP1communication::getCurrentCommOperation()
{
	return _CommOperation;
}


bool SSMP1communication::readID(char *ID)
{
	bool ok = false;
	if ((_CommOperation != comOp_noCom) || isRunning()) return false;
	_CommOperation = comOp_readRomId;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
	{
		// Return ROM-ID
		ID[0] = _data.at(0);
		ID[1] = _data.at(1);
		ID[2] = _data.at(2);
	}
	_CommOperation = comOp_noCom;
	return ok;
}


bool SSMP1communication::readAddress(unsigned int addr, char * databyte)
{
	bool ok = false;
	std::vector<char> data;
	ok = readAddresses(std::vector<unsigned int>(1,addr), &data);
	if (ok)
		*databyte = data.at(0);
	return ok;
}


bool SSMP1communication::readAddresses(std::vector<unsigned int> addr, std::vector<char> * data)
{
	bool ok = false;
	if ((_CommOperation != comOp_noCom) || isRunning() || (addr.size()==0)) return false;
	_CommOperation = comOp_read;
	// Prepare buffers:
	_addresses = addr;
	// Communication-operation:
	ok = doSingleCommOperation();
	if (ok)
		*data = _data;
	_CommOperation = comOp_noCom;
	return ok;
}


bool SSMP1communication::writeAddress(unsigned int addr, char databyte)
{
	return writeAddresses(std::vector<unsigned int>(1,addr), std::vector<char>(1,databyte));
}


bool SSMP1communication::writeAddresses(std::vector<unsigned int> addr, std::vector<char> data)
{
	bool ok = false;
	if ((_CommOperation != comOp_noCom) || isRunning() || (addr.size()==0) || (data.size()==0) || (addr.size()!=data.size())) return false;
	_CommOperation = comOp_write;
	// Prepare buffers:
	_addresses = addr;
	_data = data;
	// Communication-operation:
	ok = doSingleCommOperation();
	_CommOperation = comOp_noCom;
	return ok;
}


bool SSMP1communication::readAddress_permanent(unsigned int addr)
{
	return readAddresses_permanent(std::vector<unsigned int>(1,addr));
}


bool SSMP1communication::readAddresses_permanent(std::vector<unsigned int> addr)
{
	if ((_CommOperation != comOp_noCom) || isRunning() || (addr.size()==0)) return false;
	_CommOperation = comOp_read_p;
	// Prepare buffers:
	_addresses = addr;
	// Start permanent reading:
	start();
	return isRunning();
}


bool SSMP1communication::writeAddress_permanent(unsigned int addr, char databyte)
{
	return writeAddresses_permanent(std::vector<unsigned int>(1,addr), std::vector<char>(1,databyte));
}


bool SSMP1communication::writeAddresses_permanent(std::vector<unsigned int> addr, std::vector<char> data)
{
	if ((_CommOperation != comOp_noCom) || isRunning() || (addr.size()==0) || (data.size()==0) || (addr.size()!=data.size())) return false;
	_CommOperation = comOp_write_p;
	// Prepare buffers:
	_addresses = addr;
	_data = data;
	// Start permanent writing:
	start();
	return isRunning();
}


bool SSMP1communication::stopCommunication()
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
	QTime timer;
	int duration_ms = 0;
	bool permanent = false;
	bool op_success = false;
	bool abort;
	comOp_dt operation;
	SSM1_CUtype_dt cu;
	std::vector<unsigned int> addresses;
	std::vector<char> data;
	unsigned int k = 0;
	bool setAddr = true;
	unsigned char errmax = 3;
	char errcount = 0;

	// Synchronise with main-thread:
	_mutex.lock();
	operation = _CommOperation;
	cu = _cu;
	addresses = _addresses;
	data = _data;
	errmax = _errRetries + 1;
	_result = false;
	_abort = false;
	_mutex.unlock();
#ifdef __FSSM_DEBUG__
	// Debug-output:
	std::string op_str = "SSMP1communication::run():   operation: ";
	switch (operation)
	{
		case comOp_noCom:
			op_str += "noCom";
			break;
		case comOp_readRomId:
			op_str += "readRomId";
			break;
		case comOp_read:
			op_str += "read";
			break;
		case comOp_read_p:
			op_str += "read_p";
			break;
		case comOp_write:
			op_str += "write";
			break;
		case comOp_write_p:
			op_str += "write_p";
			break;
		default:
			op_str += "INVALID/UNKNOWN: " + QString::number(operation).toStdString();
	}
	std::cout << op_str << '\n';
#endif
	// Preparation:
	if ( operation==comOp_read_p || operation==comOp_write_p )
	{
		permanent = true;
		timer.start();
	}
	if (operation == comOp_readRomId)
		addresses.push_back(0x0000);
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
			if ((operation == comOp_read) || (operation == comOp_read_p))
			{
				// Read next data:
				if (k==0) data.clear();
				op_success = getNextData(&data);
#ifdef __FSSM_DEBUG__
				if (!op_success)
					std::cout << "SSMP1communication::run():   getNextData(...) failed !\n";
#endif
			}
			else if ((operation == comOp_write) || (operation == comOp_write_p))
			{
				// Write next data:
				if (writeDatabyte(data.at(k)))
					op_success = waitForDataValue(data.at(k));
				else
					op_success = false;
#ifdef __FSSM_DEBUG__
				if (!op_success)
					std::cout << "SSMP1communication::run():   writeDatabyte(...) failed !\n";
#endif
			}
			else if (operation == comOp_readRomId)
			{
				// Get ROM-ID:
				op_success = getID(&data);
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
				if (k < (addresses.size()-1))
					k++;
				else
					k=0;
				setAddr = true;
			}
			// Send data to main thread:
			if (permanent && (k==0))
			{
				// GET ELAPSED TIME:
				duration_ms = timer.restart();
				// SEND DATA TO MAIN THREAD:
				emit recievedData(data, duration_ms);
			}
		}
		else
		{
#ifdef __FSSM_DEBUG__
			std::cout << "SSMP1communication::run():   communication operation error counter=" << (int)(errcount) << '\n';
#endif
			errcount++;
			setAddr = true;	// repeat the complete procedure
			if (errcount < errmax) msleep(100);
		}
		// GET ABORT STATUS::
		_mutex.lock();
		abort = _abort;
		_mutex.unlock();
	} while (!abort && (errcount < errmax) && (permanent || k>0 || !op_success));
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
#ifdef __FSSM_DEBUG__
	std::cout << "SSMP1communication::run():   communication operation finished.\n";
#endif
}


