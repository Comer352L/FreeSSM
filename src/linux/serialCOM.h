/*
 * serialCOM.h - Serial port configuration and communication
 *
 * Copyright (C) 2008-2013 Comer352L
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
#include <cstdlib>		// malloc()/free()
extern "C"
{
	#include <asm/termbits.h>	// terminal control definitions (linux)
	#include <fcntl.h>		// File control definitions => fcntl() in fcn OpenPort()
	#include <dirent.h>		// definition of dirent-structure for access to directories => used in fcn GetAvailablePort()
	#include <sys/ioctl.h>	// ioctl => for access to serial port driver
	#include <sys/select.h>	// select()
	#include <linux/serial.h>	// serial port driver
	#include <limits.h>
	#include <unistd.h>		// usleep(), isatty(), close()
	#ifdef __SERIALCOM_DEBUG__
		#include <errno.h>
	#endif
}
#include <string>
#include <vector>
#include <ctime>
#ifdef __SERIALCOM_DEBUG__
	#include <iostream>
#endif



class serialCOM
{

public:
	serialCOM();
	~serialCOM();
	static std::vector<std::string> GetAvailablePorts();
	bool IsOpen();
	std::string GetPortname();
	bool GetPortSettings(double *baudrate, unsigned short *databits = NULL, char *parity = NULL, float *stopbits = NULL);
	bool SetPortSettings(double baudrate, unsigned short databits, char parity, float stopbits);
	bool OpenPort(std::string portname);	// returns success of operation, NOT PORT STATUS (open/closed)
	bool ClosePort();			// returns success of operation, NOT PORT STATUS (open/closed)
	bool Write(std::vector<char> data);
	bool Write(char *data, unsigned int datalen);
	bool Read(unsigned int minbytes, unsigned int maxbytes, unsigned int timeout, std::vector<char> *data);
	bool Read(unsigned int minbytes, unsigned int maxbytes, unsigned int timeout, char *data, unsigned int *nrofbytesread);
	bool ClearSendBuffer();
	bool ClearReceiveBuffer();
	bool SendBreak(unsigned int duration_ms);
	bool SetBreak();
	bool ClearBreak();
	bool BreakIsSet();
	bool GetNrOfBytesAvailable(unsigned int *nbytes);
	bool SetControlLines(bool DTR, bool RTS);

private:
	struct std_baudrate {
		double value;
		speed_t constant;
	};

	bool GetStdBaudRateDCBConst(double baudrate, speed_t *DCBbaudconst);
	speed_t GetNearestStdBaudrate(double selBaudrate);

	int fd;					// file descriptor for the port
	bool portisopen;
	bool breakset;
	std::string currentportname;
	struct termios2 oldtio;			// backup of port settings
	struct serial_struct old_serdrvinfo;	// backup of serial driver settings
	bool settingssaved;
	bool flag_async_low_latency_supported;
	bool ioctl_tiocgserial_supported;
	bool ioctl_tiocsserial_supported;
	static struct std_baudrate std_baudrates[];

};


#endif
