/*
 * serialCOM.h - Serial port configuration and communication
 *
 * Copyright (C) 2008-2009 Comer352l
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

#ifndef SERIALCOM_H
#define SERIALCOM_H


//#define __SERIALCOM_DEBUG__


#include <cstring>		// memset(), strcpy(), ...
#include <cmath>		// round()
extern "C"
{
    #include <windows.h>
}
#include <string>
#include <vector>
#ifdef __SERIALCOM_DEBUG__
    #include <iostream>
#endif



class serialCOM
{

private:
	HANDLE hCom;		// handle to port
	bool portisopen;
	bool breakset;
	std::string currentportname; 
	DCB olddcb;		// backup of old port settings
	bool settingssaved;
	
	bool GetMaxbaudrate(double *maxbaudrate);

public:
	struct dt_portsettings
	{
		double baudrate;
		unsigned short databits;
		char parity;
		float stopbits;
	};

	serialCOM();
	~serialCOM();
	static std::vector<std::string> GetAvailablePorts();
	bool IsOpen();
	std::string GetPortname();
	bool GetPortSettings(serialCOM::dt_portsettings *currentportsettings);
	bool SetPortSettings(dt_portsettings newportsettings);
	bool OpenPort(std::string portname);	// returns success of operation, NOT PORT STATUS (open/closed)
	bool ClosePort();			// returns success of operation, NOT PORT STATUS (open/closed)
	bool Write(char *outputstr, unsigned int nrofbytestowrite);
	bool Read(char *readdata, unsigned int *nrofbytesread);
	bool ClearSendBuffer();
	bool ClearRecieveBuffer();
	bool SendBreak(unsigned int duration_ms);
	bool SetBreak();
	bool ClearBreak();
	bool BreakIsSet();
	bool GetNrOfBytesAvailable(unsigned int *nbytes);

};


#endif
