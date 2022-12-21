/*
 * SSMP1communication_procedures.h - Communication procedures for the SSM1-protocol
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

#ifndef SSMP1COMMUNICATION_PROCEDURES_H
#define SSMP1COMMUNICATION_PROCEDURES_H


#ifdef __WIN32__
	#include "windows\TimeM.h"
	#define waitms(x) Sleep(x)
#elif defined __linux__
	#include <unistd.h>
	#include "linux/TimeM.h"
	#define waitms(x) usleep(1000*x)
#else
	#error "Operating system not supported !"
#endif
#ifdef __FSSM_DEBUG__
	#include "libFSSM.h"
	#include <iostream>
#endif
#include <vector>
#include "AbstractDiagInterface.h"
#include "SSMP1base.h"


class SSMP1communication_procedures : private SSMP1commands
{

public:
	SSMP1communication_procedures(AbstractDiagInterface *diagInterface);
	bool setAddress(SSM1_CUtype_dt cu, unsigned int addr);
	bool getID(unsigned int addr, unsigned char extradatalen, std::vector<char> * data);
	bool writeDatabyte(char databyte);
	bool getNextData(std::vector<char> * data, unsigned int timeout = SSMP1_T_RW_REC_MAX);	/* read and process received data */
	char waitForDataValue(char data, unsigned int timeout = SSMP1_T_RECDATA_CHANGE_MAX);
	bool stopCUtalking(bool waitforsilence = false);

private:
	std::vector<char> _recbuffer;	/* incoming (unprocessed) data from serial port => used by getNextData() */
	int _currentaddr;		/* currently used read/write address */
	int _lastaddr;			/* last read/write address */
	bool _addrswitch_pending;	/* lately switched the read/write address ? => Needed for synchronisation ! */
	bool _sync;			/* synchronisation status for incoming data processing => used by getNextData() */

	void syncToRecData();

};


#endif

