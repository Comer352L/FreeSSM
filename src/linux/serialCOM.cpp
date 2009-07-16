/*
 * serialCOM.cpp - Serial port configuration and communication
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

#include "serialCOM.h"


serialCOM::serialCOM()
{
	fd = -1;
	portisopen = false;
	breakset = false;
	currentportname = "";
	memset(&oldtio, 0, sizeof(oldtio));
	memset(&old_serdrvinfo, 0, sizeof(old_serdrvinfo));
	settingssaved = false;
	serdrvaccess = false;
}


std::vector<std::string> serialCOM::GetAvailablePorts()
{
	std::vector<std::string> portlist(0);
	int testfd = -1;				// file descriptor for tested device files
	char ffn[256] = "";				// full filename incl. path
	DIR *dp = NULL;
	struct dirent *fp = NULL;
	// CHECK DEVICE FILES:
	dp = opendir ("/dev");
	if (dp != NULL)
	{
		do {
			fp = readdir (dp);	// get next file in directory
			if (fp != NULL)
			{
				if ((!strncmp(fp->d_name,"ttyS",4)) | (!strncmp(fp->d_name,"ttyUSB",6))) // if filename starts with "ttyS" or "ttyUSB"
				{
					// CONSTRUCT FULL FILENAME:
					strcpy(ffn, "/dev/");		// (replaces old string)
					strcat(ffn, fp->d_name);
					testfd = -1;
					testfd = open(ffn, O_RDWR | O_NOCTTY | O_NDELAY);
					if (!(testfd == -1))
					{
						// FIND OUT, IF FILE IS A TTY:
						if (isatty(testfd))
							portlist.push_back(ffn);
						close(testfd);
					}
				}
			}
		} while (fp != NULL);
		closedir (dp);
	}
#ifdef __SERIALCOM_DEBUG__
	else
		std::cout << "serialCOM::GetAvailablePorts():   opendir(''/dev'') failed with error " << errno << " " << strerror(errno) << "\n";
#endif
	// SEARCH FOR USB-SERIAL-CONVERTERS:
	dp = opendir ("/dev/usb");
	if (dp != NULL)
	{
		do {
			fp = readdir (dp);     // get next file in directory
			if (fp != NULL)
			{
				if (!strncmp(fp->d_name,"ttyUSB",6))	// if filename starts with "ttyUSB"
				{
					// CONSTRUCT FULL FILENAME:
					strcpy(ffn, "/dev/usb/");	// (replaces old string)
					strcat(ffn, fp->d_name);
					testfd = -1;
					testfd = open(ffn, O_RDWR | O_NOCTTY | O_NDELAY);
					if (!(testfd == -1))
					{
						// FIND OUT, IF FILE IS A TTY:
						if (isatty(testfd))
							portlist.push_back(ffn);
						close(testfd);
					}
				}
			}
		} while (fp != NULL);
		closedir (dp);
	}
	return portlist;
}


bool serialCOM::IsOpen()
{
	if (portisopen && (fd >= 0))
		return true;
	else
		return false;
}


std::string serialCOM::GetPortname()
{
	return currentportname;
}


bool serialCOM::GetPortSettings(serialCOM::dt_portsettings *currentportsettings)
{
	int cvIOCTL_SD = -1;	// -1=ERROR , others=OK
	bool settingsvalid = true;
	struct termios2 currenttio;
	memset(&currenttio, 0, sizeof(currenttio));
	speed_t baudrate=0;
	unsigned int cleanedbitmask=0;
	// Reset data:
	currentportsettings->baudrate = 0;
	currentportsettings->databits = 0;
	currentportsettings->parity = 0;
	currentportsettings->stopbits = 0;
	if (!portisopen) return false;
	// Query current settings:
	if (ioctl(fd, TCGETS2, &currenttio) == -1)
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::GetPortSettings():   ioctl(..., TCGETS2, ...) failed with error " << errno << " " << strerror(errno)<< "\n";
#endif
		return false;
	}
	// BAUDRATE:
	baudrate = currenttio.c_cflag & CBAUD;	// get baud rate
	/* NOTE:
	   - c_ispeed/c_ospeed is ignored by the system when setting a
	     new termios2 with c_cflag not set to BOTHER (set to another Bxxxxx)
	   - when getting the termios2 struct from the system, the
	     c_ispeed/c_ospeed field SEEM to contain always the
	     baudrate value regardless of the baud settings in c_cflag
	     CAN WE BE SURE THAT ALL DRIVERS BEHAVE LIKE THIS ???
	     => For now, we only trust c_ispeed/c_ospeed if c_cflag
	     contains BOTHER
	*/
#ifdef __SERIALCOM_DEBUG__
	std::cout << "serialCOM::GetPortSettings(): baudrates in struct termios2:\n";
	std::cout << "   c_cflag & CBAUD: " << (currenttio.c_cflag & CBAUD) << '\n';
	std::cout << "   c_ispeed: " << currenttio.c_ispeed << '\n';
	std::cout << "   c_ospeed: " << currenttio.c_ospeed << '\n';
#endif
	if (baudrate == BOTHER)
	{
		if (currenttio.c_ispeed == currenttio.c_ispeed)
		{
			currentportsettings->baudrate = currenttio.c_ispeed;
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::GetPortSettings():   WARNING:   baudrate is encoded with the BOTHER-method !\n => The reported baudrate may differ from the ''real'' baudrate depending on the driver !\n";
#endif
		}
		else
		{
			settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::GetPortSettings():   ERROR:   different baud rates for transmitting and recieving detected\n";
#endif
		}
	}
	else if (baudrate == B0)	// B0 is not allowed (=> Windows compatibility)
	{
		currentportsettings->baudrate = 0;
	}
	else if (baudrate == B50)
	{
		currentportsettings->baudrate = 50;
	}
	else if (baudrate == B75)
	{
		currentportsettings->baudrate = 75;
	}
	else if (baudrate == B110)
	{
		currentportsettings->baudrate = 110;
	}
	else if (baudrate == B134)
	{
		currentportsettings->baudrate = 134.5;
	}
	else if (baudrate == B150)
	{
		currentportsettings->baudrate = 150;
	}
	else if (baudrate == B200)
	{
		currentportsettings->baudrate = 200;
	}
	else if (baudrate == B300)
	{
		currentportsettings->baudrate = 300;
	}
	else if (baudrate == B600)
	{
		currentportsettings->baudrate = 600;
	}
	else if (baudrate == B1200)
	{
		currentportsettings->baudrate = 1200;
	}
	else if (baudrate == B1800)
	{
		currentportsettings->baudrate = 1800;
	}
	else if (baudrate == B2400)
	{
		currentportsettings->baudrate = 2400;
	}
	else if (baudrate == B4800)
	{
		currentportsettings->baudrate = 4800;
	}
	else if (baudrate == B9600)
	{
		currentportsettings->baudrate = 9600;
	}
	else if (baudrate == B19200)
	{
		currentportsettings->baudrate = 19200;
	}
	else if (baudrate == B38400)
	{
		if (serdrvaccess) // if we have access to the driver
		{
			// Get driver settings
			struct serial_struct current_serdrvinfo;
			memset(&current_serdrvinfo, 0, sizeof(current_serdrvinfo));
			cvIOCTL_SD = ioctl(fd, TIOCGSERIAL, &current_serdrvinfo);
			if (cvIOCTL_SD != -1)
			{
				// Check if it is a non-standard baudrate:
				if (current_serdrvinfo.flags != (current_serdrvinfo.flags | ASYNC_SPD_CUST))
					currentportsettings->baudrate = 38400;
				else
				{
					if (current_serdrvinfo.custom_divisor != 0)
						currentportsettings->baudrate = ( static_cast<double>(current_serdrvinfo.baud_base) / current_serdrvinfo.custom_divisor); // Calculate custom baudrate
					else
					{
						settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
						std::cout << "serialCOM::GetPortSettings():   error: custom baudrate with custom_divisor=0 detected\n";
#endif
					}
				}
			}
			else	// If driver settings are not available 
			{
				settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
				std::cout << "serialCOM::GetPortSettings():   ioctl(..., TIOCGSERIAL, ...) failed with error " << errno << " " << strerror(errno)<< "\n";
#endif
			}
		}
		else
			currentportsettings->baudrate = 38400;
	}
	else if (baudrate == B57600)
	{
		currentportsettings->baudrate = 57600;
	}
	else if (baudrate == B115200)
	{
		currentportsettings->baudrate = 115200;
	}
	else
	{
		settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::GetPortSettings():   error: unknown baudrate\n";
#endif
	}
	// DATABITS:
	cleanedbitmask = (currenttio.c_cflag & ~CSIZE);
	if (currenttio.c_cflag == (cleanedbitmask | CS8))
	{
		currentportsettings->databits = 8;
	}
	else if (currenttio.c_cflag == (cleanedbitmask | CS7))
	{
		currentportsettings->databits = 7;
	}
	else if (currenttio.c_cflag == (cleanedbitmask | CS6))
	{
		currentportsettings->databits = 6;
	}
	else if (currenttio.c_cflag == (cleanedbitmask | CS5))
	{
		currentportsettings->databits = 5;
	}
	else
	{
		settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::GetPortSettings():   error: unknown number of databits\n";
#endif
	}
	// PARITY (NOTE:   CMSPAR NOT AVAILABLE ON ALL SYSTEMS !!!):
	if (currenttio.c_cflag != (currenttio.c_cflag | PARENB)) // if parity-flag is not set
		currentportsettings->parity='N';
	else    	// if parity-flag is set
	{
		if (currenttio.c_cflag != (currenttio.c_cflag | PARODD)) // if Odd-(Mark-) parity-flag is not set
		{
			if (currenttio.c_cflag != (currenttio.c_cflag | CMSPAR)) // if Space/Mark-parity-flag is not set
				currentportsettings->parity='E';
			else
				currentportsettings->parity='S';
		}
		else	// if Odd-(Mark-) parity-flag is set
		{
			if (currenttio.c_cflag != (currenttio.c_cflag | CMSPAR)) // if Space/Mark-parity-flag is not set
				currentportsettings->parity='O';
			else
				currentportsettings->parity='M';
		}
	}
	// STOPBITS:
	if (currenttio.c_cflag != (currenttio.c_cflag | CSTOPB))
		currentportsettings->stopbits = 1;
	else
	{
		if (currentportsettings->databits == 5)
		{
			currentportsettings->stopbits = 1.5;
		}
		else if (currentportsettings->databits > 5)
		{
			currentportsettings->stopbits = 2;
		}
		else
		{
			settingsvalid = false;
			currentportsettings->stopbits = 0;
		}
	}
	// RETURN SUCCESS:
	return settingsvalid;
}


bool serialCOM::SetPortSettings(serialCOM::dt_portsettings newportsettings)
{
	int cIOCTL = -1;
	int cIOCTL_SD = -1;
	bool isStdBaud = true;
	bool settingsvalid = true;
	speed_t newbaudrate = 0;
	struct termios2 newtio;
	memset(&newtio, 0, sizeof(newtio));
	if (!portisopen) return false;
	struct serial_struct new_serdrvinfo;
	memset(&new_serdrvinfo, 0, sizeof(new_serdrvinfo));
	if (serdrvaccess)
	{
		// Get current port settings:
		cIOCTL_SD = ioctl(fd, TIOCGSERIAL, &new_serdrvinfo);	// read from driver
#ifdef __SERIALCOM_DEBUG__
		if (cIOCTL_SD == -1)
			std::cout << "serialCOM::SetPortSettings():   ioctl(..., TIOCSSERIAL, ...) #1 failed with error " << errno << " " << strerror(errno)<< "\n";
#endif
		// Deactivate custom baudrate settings:
		new_serdrvinfo.custom_divisor = 0;
		new_serdrvinfo.flags &= ~ASYNC_SPD_CUST;
		new_serdrvinfo.flags |= ASYNC_LOW_LATENCY;
	}
	// SET CONTROL OPTIONS:
	newtio.c_cflag = (CREAD | CLOCAL);
	// BAUDRATE:
	// Set new baudrate:
	if (!(newportsettings.baudrate > 0))
	{
		settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
		if (newportsettings.baudrate == 0)
			std::cout << "serialCOM::SetPortSettings:   error: illegal baudrate - 0 baud not possible\n";
		else
			std::cout << "serialCOM::SetPortSettings:   error: illegal baudrate - baudrate must be > 0\n";
#endif
	}
	else if (newportsettings.baudrate > 115200)
	{
		settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::SetPortSettings:   error: illegal baudrate - baudrates above 115200 are currently not supported\n";
#endif
	}
	else
	{
		isStdBaud = GetStdbaudrateDCBConst(newportsettings.baudrate, &newbaudrate);
		if (!isStdBaud)
		{
			/* NOTE: The "old" method for setting non-standrad baudrates is prefered,
			*	 because we konw the supported baudrates exactly and can select them
			*	 according to our own startegy (=> min. relative deviation)
			*/
			if (serdrvaccess && (cIOCTL_SD != -1))	// if we have access to serial driver
			{
				int customdivisor = 0;
				double custombaudrate = 0;
				customdivisor = static_cast<int>(round(new_serdrvinfo.baud_base / newportsettings.baudrate));
				if (customdivisor < 1)
					customdivisor = 1;	// ...to be sure
				if (customdivisor > 65535)
					customdivisor = 65535;
				custombaudrate = static_cast<double>(new_serdrvinfo.baud_base / customdivisor);
				// Check if it is a standard baud rate now:
				if (!GetStdbaudrateDCBConst(newportsettings.baudrate, &newbaudrate))
				{
					newbaudrate = B38400;
					new_serdrvinfo.flags |= ASYNC_SPD_CUST;
					new_serdrvinfo.custom_divisor = customdivisor;
				}
			}
			else
			{
				newbaudrate = BOTHER;
				newtio.c_ispeed = round(newportsettings.baudrate);
				newtio.c_ospeed = round(newportsettings.baudrate);
				/* TODO:
				* - DOES ioctl(..., TCSETS2, ...) ALWAYS SET THE NEAREST POSSIBLE BAUD RATE ? HOW IS "NEAREST" DEFINED (ABSOLUTE/RELATIVE ?) ?
				* - IS THERE A MAXIMUM DEVIATION, WHICH LETS THE ioctl() FAIL IF IT IS EXCEEDED ?
				* - IS THIS BEHAVIOR UNIFIED/GUARANTEED FOR ALL SERIAL PORT DRIVERS ?
				*/
				/* NOTE: if the ioctl() fails (later in this function), we will
				*       retry with the nearest possible standard baudrate
				*/
			}
		}
	}
	// DATABITS:
	switch (newportsettings.databits)
	{
		case 8:
			newtio.c_cflag |= CS8;
			break;
		case 7:
			newtio.c_cflag |= CS7;
			break;
		case 6:
			newtio.c_cflag |= CS6;
			break;
		case 5:
			newtio.c_cflag |= CS5;
			break;
		default:
			settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::SetPortSettings():  error: illegal number of databits\n";
#endif
			break;
	}
	// PARITY:
	if (newportsettings.parity == 'N')
	{
		newtio.c_cflag &= ~PARENB;	// deactivate parity (not really necessary, because c_cflag is clean)
	}
	else if (newportsettings.parity == 'E')
	{
		newtio.c_cflag |= PARENB;	// activate parity
		newtio.c_cflag &= ~CMSPAR;	// activate mark/space mode (not really necessary, because c_cflag is clean)
		newtio.c_cflag &= ~PARODD;	// deactivate odd parity (not really necessary, because c_cflag is clean)
	}
	else if (newportsettings.parity == 'O')
	{
		newtio.c_cflag |= PARENB;	// activate parity
		newtio.c_cflag &= ~CMSPAR;	// deactivate mark/space mode (not really necessary, because c_cflag is clean)
		newtio.c_cflag |= PARODD;	// activate odd parity
	}
	else if (newportsettings.parity == 'S')
	{
		newtio.c_cflag |= PARENB;	// activate parity
		newtio.c_cflag |= CMSPAR;	// activate mark/space mode
		newtio.c_cflag &= ~PARODD;	// deactivate mark parity (not really necessary, because c_cflag is clean)
	}
	else if (newportsettings.parity == 'M')
	{
		newtio.c_cflag |= PARENB;	// activate parity
		newtio.c_cflag |= CMSPAR;	// activate mark/space mode
		newtio.c_cflag |= PARODD;	// activate mark parity
	}
	else
	{
		settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::SetPortSettings():  error: illegal parity option\n";
#endif
	}
	// STOPBITS:
	if (newportsettings.stopbits == 1)
	{
		newtio.c_cflag &= ~CSTOPB;	//  (not really necessary, because c_cflag is clean)
	}
	else if (newportsettings.stopbits == 1.5)
	{
		if (newportsettings.databits == 5)
			newtio.c_cflag |= CSTOPB;
		else
		{
			settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::SetPortSettings():   error: invalid value for parameter 'stopbits': 1.5 stopbits only allowed in combination with 5 databits\n";
#endif
		}
	}
	else if (newportsettings.stopbits == 2)
	{
		if (newportsettings.databits != 5)
			newtio.c_cflag |= CSTOPB;
		else
		{
			settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::SetPortSettings():   error: invalid value for parameter 'stopbits': 2 stopbits not allowed in combination with 5 databits\n";
#endif
		}
	}
	else
	{
		settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::SetPortSettings():   error: invalid value for parameter 'stopbits'" << '\n';
#endif
	}
   /*	CBAUD	Bit mask for baud rate
	B0	0 baud (drop DTR)
	B50	50 baud
	B75	75 baud
	B110	110 baud
	B134	134.5 baud
	B150	150 baud
	B200	200 baud
	B300	300 baud
	B600	600 baud
	B1200	1200 baud
	B1800	1800 baud
	B2400	2400 baud
	B4800	4800 baud
	B9600	9600 baud
	B19200	19200 baud
	B38400	38400 baud
	B57600	57,600 baud
	B76800	76,800 baud
	B115200	115,200 baud
	EXTA	External rate clock
	EXTB	External rate clock
	CSIZE	Bit mask for data bits
	CS5	5 data bits
	CS6	6 data bits
	CS7	7 data bits
	CS8	8 data bits
	CSTOPB	2 stop bits (1 otherwise)
	CREAD	Enable receiver
	PARENB	Enable parity bit
	PARODD	Use odd parity instead of even
	CMSPAR	Use space/mark parity instead of even/odd		// not POSIX-defined ! (undocumented !)
	HUPCL	Hangup (drop DTR) on last close
	CLOCAL	Local line - do not change "owner" of port
	LOBLK	Block job control output
	CRTSCTS	Enable hardware flow control (not supported on all platforms)
	CNEW_RTSCTS		// not available under Linux				*/
// LINE OPTIONS (LOCAL OPTIONS) - controls how input characters are managed:
	newtio.c_lflag = NOFLSH;
   /*	ISIG	Enable SIGINTR, SIGSUSP, SIGDSUSP, and SIGQUIT signals
	ICANON	Enable canonical input (else raw)
	XCASE	Map uppercase \lowercase (obsolete)
	ECHO	Enable echoing of input characters
	ECHOE	Echo erase character as BS-SP-BS
	ECHOK	Echo NL after kill character
	ECHONL	Echo NL
	NOFLSH	Disable flushing of input buffers after interrupt or quit characters
	IEXTEN	Enable extended functions (this flag, as well as ICANON must be enabled for the
                special characters EOL2, LNEXT, REPRINT, WERASE to be interpreted, and for the IUCLC
                flag to be effective)
	ECHOCTL	Echo control characters as ^char and delete as ~?
	ECHOPRT	Echo erased character as character erased
	ECHOKE	BS-SP-BS entire line on line kill
	FLUSHO	Output being flushed
	PENDIN	Retype pending input at next read or input char
	TOSTOP	Send SIGTTOU for background output				*/
// INPUT OPTIONS:
	newtio.c_iflag = IGNPAR | IGNBRK;
   /*	INPCK	Enable parity check
	IGNPAR	Ignore parity errors
	PARMRK	Mark parity errors
	ISTRIP	Strip parity bits
	IXON	Enable software flow control (outgoing)
	IXOFF	Enable software flow control (incoming)
	IXANY	Allow any character to start flow again
	IGNBRK	Ignore break condition
	BRKINT	Send a SIGINT when a break condition is detected
	INLCR	Map NL to CR
	IGNCR	Ignore CR
	ICRNL	Map CR to NL
	IUCLC	Map uppercase to lowercase
	IMAXBEL	Echo BEL on input line too long
	IUTF8				*/
// OUTPUT OPTIONS:
	newtio.c_oflag &= ~OPOST;  // => raw output - all other option bits will be ignored
   /*	OPOST	Postprocess output (not set = raw output)
	OLCUC	Map lowercase to uppercase (not POSIX ???)
	ONLCR	Map NL to CR-NL
	OCRNL	Map CR to NL
	NOCR	No CR output at column 0
	ONOCR
	ONLRET	NL performs CR function
	OFILL	Use fill characters for delay
	OFDEL	Fill character is DEL
	NLDLY	Mask for delay time needed between lines
	NL0	No delay for NLs
	NL1	Delay further output after newline for 100 milliseconds
	CRDLY	Mask for delay time needed to return carriage to left column
	CR0	No delay for CRs
	CR1	Delay after CRs depending on current column position
	CR2	Delay 100 milliseconds after sending CRs
	CR3	Delay 150 milliseconds after sending CRs
	TABDLY	Mask for delay time needed after TABs
	TAB0	No delay for TABs
	TAB1	Delay after TABs depending on current column position
	TAB2	Delay 100 milliseconds after sending TABs
	TAB3	Expand TAB characters to spaces
	BSDLY	Mask for delay time needed after BSs
	BS0	No delay for BSs
	BS1	Delay 50 milliseconds after sending BSs
	VTDLY	Mask for delay time needed after VTs
	VT0	No delay for VTs
	VT1	Delay 2 seconds after sending VTs
	FFDLY	Mask for delay time needed after FFs
	FF0	No delay for FFs
	FF1	Delay 2 seconds after sending FFs	
// CONTROL CHARACTERS AND TIMOUT OPTIONS:		*/
	// Timeout settings (for input):
	newtio.c_cc[VMIN] = 0;
	newtio.c_cc[VTIME] = 0;
   /*	VINTR		Interrupt				CTRL-C
	VQUIT		Quit					CTRL-Z
	VERASE		Erase					Backspace (BS)
	VKILL		Kill-line				CTRL-U
	VEOF		End-of-file				CTRL-D
	VEOL		End-of-line				Carriage return (CR)
	VEOL2		Second end-of-line			Line feed (LF)
	VMIN		Minimum number of characters to read	-
	VSTART		Start flow				CTRL-Q (XON)
	VSTOP		Stop flow				CTRL-S (XOFF)
	VTIME		Time to wait for data (1/10 seconds)	-
	VSWTC							\0
	VSUSP							CTRL-z
	VREPRINT						CTRL-r
	VDISCARD						CTRL-u
	VWERASE							CTRL-w
	VLNEXT							CTRL-v
	...									*/
	// SET NEW PORT SETTING:
	if (settingsvalid == true)	// apply new settings only if they are all valid !
	{
		if (serdrvaccess)
		{
			cIOCTL_SD = ioctl(fd, TIOCSSERIAL, &new_serdrvinfo);	// write new driver settings	// -1 on error
			/* NOTE: always do this ioctl to deactivate the ASYNC_SPD_CUST if we have a standard-baudrate ! */
#ifdef __SERIALCOM_DEBUG__
			if (cIOCTL_SD == -1)
				std::cout << "serialCOM::SetPortSettings():   ioctl(..., TIOCSSERIAL, ...) #2 failed with error " << errno << " " << strerror(errno)<< "\n";
#endif
		}
		newtio.c_cflag &= ~CBAUD;
		newtio.c_cflag |= newbaudrate;
		cIOCTL = ioctl(fd, TCSETS2, &newtio);	// 0 on success, check errno
#ifdef __SERIALCOM_DEBUG__
		if (cIOCTL == -1)
			std::cout << "serialCOM::SetPortSettings():   ioctl(..., TCSETS2, ...) failed with error " << errno << " " << strerror(errno)<< "\n";
#endif
		/* NOTE: The following code-block guarantees maximum compatibility:
		 * In case of non-standard baudrates set with the "BOTHER"-method,
		 * some serial port drivers might not set the baudrate to the nearest
		 * supported value IN ANY CASE: 
		 * The ioctl() could fail, if a maximum deviation is exceeded !
		 * => We try to select the nearest supported baudrate manually in this case
		 */
		if ((cIOCTL == -1) && (!isStdBaud))
		{
			// Set baudrate to the nearest standard value:
			newbaudrate = GetNearestStdBaudrate(newportsettings.baudrate);
			newtio.c_cflag &= ~CBAUD;
			newtio.c_cflag |= newbaudrate;
			cIOCTL = ioctl(fd, TCSETS2, &newtio);
#ifdef __SERIALCOM_DEBUG__
			if (cIOCTL == -1)
				std::cout << "serialCOM::SetPortSettings():   ioctl(..., TCSETS2, ...) #2 failed with error " << errno << " " << strerror(errno)<< "\n";
#endif
		}
	}
	// SUCCESS CONTROL (RETURN VALUE):
	return ((cIOCTL != -1) && (!serdrvaccess || (serdrvaccess && (cIOCTL_SD != -1)) || (isStdBaud && (newbaudrate!=B38400))));
	/* NOTE: we can tolerate a failing TIOCSSERIAL-ioctl() if the baudrate is not set to B38400,
	 *       because the ASYNC_SPD_CUST-flag and the custom divisor are always ignored if B38400 is not set !
	 */
}


bool serialCOM::OpenPort(std::string portname)
{
	int confirm = -1;	// -1=error
	struct serial_struct new_serdrvinfo;	// new driver settings
	if (portisopen) return false;	// if port is already open => cancel and return "false"
	memset(&new_serdrvinfo, 0, sizeof(new_serdrvinfo));
	memset(&oldtio, 0, sizeof(oldtio));
	memset(&old_serdrvinfo, 0, sizeof(old_serdrvinfo));
	// OPEN PORT:
	fd = open(portname.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
	/* 		O_RDWR	 = read/write access;
	*		O_NOCTTY = ignore control characters
	*		O_NDELAY = ignore DCD-line		*/
	if (!(fd < 0))	// if port is now open
	{
		portisopen = true;
		currentportname = portname;
		confirm = ioctl(fd, TIOCEXCL, NULL);	// LOCK DEVICE
#ifdef __SERIALCOM_DEBUG__
		if (confirm == -1)
			std::cout << "serialCOM::OpenPort():   ioctl(..., TIOCEXCL, ...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
		confirm = fcntl(fd, F_SETFL, FNDELAY);	// function "read" shall return "0" if no data available
		if (confirm == -1)
		{
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::OpenPort():   fcntl(...) failed with error " << errno << "\n";
#endif
			confirm = ClosePort();
#ifdef __SERIALCOM_DEBUG__
			if (!confirm)
				std::cout << "serialCOM::OpenPort():   port couldn't be closed after error during opening process\n";
#endif
			return false;
		}
		// SAVE SETTINGS:
		confirm = ioctl(fd, TCGETS2, &oldtio);
		if (confirm == -1)
		{
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::OpenPort():   ioctl(..., TCGETS2, ...) failed with error " << errno << "\n";
#endif
			settingssaved = false;
		}
		else
			settingssaved = true;
		confirm = ioctl(fd, TIOCGSERIAL, &old_serdrvinfo);	// driver settings
		if (confirm != -1)
			serdrvaccess = true;
		else
			serdrvaccess = false;
		if (serdrvaccess)
		{
			// CHANGE DRIVER SETTINGS:
			new_serdrvinfo = old_serdrvinfo;
			new_serdrvinfo.flags |= ASYNC_LOW_LATENCY;		// request low latency behaviour
			confirm = ioctl(fd, TIOCSSERIAL, &new_serdrvinfo);	// write driver settings
#ifdef __SERIALCOM_DEBUG__
			if (confirm == -1)
				std::cout << "serialCOM::OpenPort():   ioctl(..., TIOCSSERIAL, ...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
		}
/*
		// CLEAR BREAK (should not be necessary, because break is cancelled automatically after closing the device):
		confirm = ioctl(fd, TIOCCBRK, 0);    // break OFF
		if (confirm == -1)
			std::cout << "serialCOM::OpenPort():   ioctl(..., TIOCCBRK, ) failed with error " << errno << "\n";
		else
*/
		breakset = false;
		// CLEAR HARDWARE BUFFERS:
		confirm = ioctl(fd, TCFLSH, TCIOFLUSH);
		if (confirm == -1)
		{
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::OpenPort():   ioctl(..., TCFLSH, TCIOFLUSH) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
			confirm = ClosePort();
#ifdef __SERIALCOM_DEBUG__
			if (!confirm)
				std::cout << "serialCOM::OpenPort():   port couldn't be closed after error during opening process\n";
#endif
			return false;
		}
		// CONFIGURE COMMUNICATION, SET STANDARD PORT-SETTINGS
		if (!SetPortSettings( dt_portsettings(9600,8,'N',1) ))
		{
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::OpenPort():   Couldn't set standard port settings with SetPortSettings() !\n";
#endif
			confirm = ClosePort();
#ifdef __SERIALCOM_DEBUG__
			if (!confirm)
				std::cout << "serialCOM::OpenPort():   port couldn't be closed after error during opening process\n";
#endif
			return false;
			/* NOTE: SetPortSettings not only changes the 4 communication parameters.
			         It configures additional parameters (like control characters, timeouts, ...) which are 
				 are important to ensure proper communication behavior !
			 */
		}
		return true;
	}
	else
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::OpenPort():   open(...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
		return false;
	}
}


bool serialCOM::ClosePort()
{
	int confirm = -1;	// -1=error
	if (!portisopen) return false;
	// CLEAR BREAK:
	confirm = ioctl(fd, TIOCCBRK, 0);    // break OFF
	if (confirm != -1)
		breakset = false;
#ifdef __SERIALCOM_DEBUG__
	else
		std::cout << "serialCOM::ClosePort():   ioctl(..., TIOCCBRK, ...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
	// CLEAR HARDWARE BUFFERS:
	confirm = ioctl(fd, TCFLSH, TCIOFLUSH);	// clear buffers (input and output)
#ifdef __SERIALCOM_DEBUG__
	if (confirm == -1)
		std::cout << "serialCOM::ClosePort():   ioctl(..., TCFLSH, TCIOFLUSH) failed with error " << errno << "\n";
#endif
	// RESTORE PORT SETTINGS:
	if (serdrvaccess)
	{
		confirm = ioctl(fd, TIOCSSERIAL, &old_serdrvinfo);	// restore old driver settings
#ifdef __SERIALCOM_DEBUG__
		if (confirm == -1)
			std::cout << "serialCOM::ClosePort():   ioctl(..., TIOCSSERIAL, ...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
	}
	if (settingssaved)
	{
		confirm = ioctl(fd, TCSETS2, &oldtio);	// restore the old port settings
#ifdef __SERIALCOM_DEBUG__
	if (confirm == -1)
		std::cout << "serialCOM::ClosePort():   ioctl(..., TCSETS2, ...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
	}
	confirm = ioctl(fd, TIOCNXCL, NULL);	// unlock device
#ifdef __SERIALCOM_DEBUG__
	if (confirm == -1)
		std::cout << "serialCOM::ClosePort():   ioctl(..., TIOCNXCL, ...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
	// CLOSE PORT:
	confirm = close(fd);
	// CLEAN UP, RETURN VALUE:
	if (confirm==0)
	{
		fd = -1;
		portisopen = false;
		breakset = false;
		currentportname = "";
		serdrvaccess = false;
		return true;
	}
	else
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::ClosePort():   close(...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
		return false;
	}
}


bool serialCOM::Write(std::vector<char> data)
{
	return Write(&data.at(0), data.size());
}


bool serialCOM::Write(char *data, unsigned int datalen)
{
	int confirm = -1;
	unsigned int nrofbyteswritten = 0;
	if (!portisopen) return false;
	if (breakset)
	{
		confirm = ioctl(fd, TIOCCBRK, 0);	// break OFF
		if (confirm != -1)
			breakset = false;
		else
		{
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::Write():   ioctl(..., TIOCCBRK, ...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
			return false;
		}
	}
	// SEND DATA:
	nrofbyteswritten = write(fd, data, datalen);
	// WAIT UNTIL ALL BYTES ARE TRANSMITTED (to the buffer !?):
	confirm = ioctl(fd, TCSBRK, 1);	// => linux-implementation of POSIX-fcn tcdrain(fd)
	// NOTE: 0 = send 250ms break; >0 = wait until all data is sent
	// RETURN VALUE:
	if ((nrofbyteswritten == datalen) && (confirm != -1))
		return true;
	else
	{
#ifdef __SERIALCOM_DEBUG__
		if (nrofbyteswritten != datalen)
			std::cout << "serialCOM::Write():   write(...) failed with error " << errno << " " << strerror(errno) << "\n";
		if (confirm != 0)
			std::cout << "serialCOM::Write():   ioctl(......, TCSBRK, 0) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
		return false;
	}
}


bool serialCOM::Read(unsigned int maxbytes, std::vector<char> *data)
{
	if (maxbytes > INT_MAX) return false;	// real limit: MAXDWORD
	unsigned int rdatalen = 0;
	char *rdata = (char*) malloc(maxbytes);
	if (rdata == NULL) return false;
	bool ok = Read(maxbytes, rdata, &rdatalen);
	if (ok)	data->assign(rdata, rdata+rdatalen);
	free(rdata);
	return ok;
}


bool serialCOM::Read(unsigned int maxbytes, char *data, unsigned int *nrofbytesread)
{
	int ret;
	*nrofbytesread = 0;
	if (portisopen == false) return false;
	if (maxbytes > INT_MAX) return false;	// real limit: SSIZE_MAX
	// READ AVAILABLE DATA:
	ret = read(fd, data, maxbytes);
	// RETURN VALUE:
	if ((ret < 0) || (ret > static_cast<int>(maxbytes)))	// >maxbytes: important ! => possible if fd was not open !
	{
		*nrofbytesread = 0;
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::Read():   read(..) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
		return false;
	}
	else
	{
		*nrofbytesread = static_cast<unsigned int>(ret);
		return true;
	}
}


bool serialCOM::ClearSendBuffer()
{
	int cvTF = -1;
	if (portisopen)
		cvTF = ioctl(fd, TCFLSH, TCOFLUSH);
	if (cvTF != -1)
		return true;
	else
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::ClearSendBuffer(...):   ioctl(..., TCFLSH, TCOFLUSH) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
		return false;
	}
}


bool serialCOM::ClearRecieveBuffer()
{
	int cvTF = -1;
	if (portisopen)
		cvTF = ioctl(fd, TCFLSH, TCIFLUSH);
	if (cvTF != -1)
		return true;
	else
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::ClearRecieveBuffer(...):   ioctl(..., TCFLSH, TCIFLUSH) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
		return false;
	}
}


bool serialCOM::SendBreak(unsigned int duration_ms)
{
	int confirmSB = -1;	// 0 or -1
	if ((!portisopen) || (duration_ms < 1) || (duration_ms >= 32767))
		return false;
	breakset = true;
	if (duration_ms == 250) 
	{
		confirmSB = ioctl(fd, TCSBRK, 0);
#ifdef __SERIALCOM_DEBUG__
		if (confirmSB == -1)
			std::cout << "serialCOM::SendBreak(...):   ioctl(..., TCSBRK, ...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
		/* NOTE: the Linux TCSBRK-ioctl is different from other implementations !
		 *	 - for arg == 0, a break of 250ms is send
		 *	 - for arg > 0, it waits until the Tx-buffer is empty/all data is send
		 *	   => this is how POSIX-fcn tcdrain(...) is implemented !
		 */
	}
	else if ((duration_ms / 100)*100 == duration_ms)
	{
		confirmSB = ioctl(fd, TCSBRKP, duration_ms/100);
#ifdef __SERIALCOM_DEBUG__
		if (confirmSB == -1)
			std::cout << "serialCOM::SendBreak(...):   ioctl(..., TCSBRKP, ...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
		/* NOTE: the Linux TCSBRKP-icotl is defined for compatibility. 
			 It works like the TCSBRK-ioctl on other systems and can be used to send breaks of selectable duration.
			 => on Linux, the argument is interpreted as multiplier of 100ms (other systems behave different !)
		 */
	}
	else
	{
		// We have to do the timing on our own
		confirmSB = ioctl(fd, TIOCSBRK, 0);	// break ON
		if (confirmSB != -1)
		{
			usleep(1000*duration_ms);		// GLIBC uses select() here... Would that be more accurate ?
			confirmSB = ioctl(fd, TIOCCBRK, 0);	// break OFF
			if (confirmSB == -1)
			{
#ifdef __SERIALCOM_DEBUG__
				std::cout << "serialCOM::SendBreak(...):   ioctl(..., TIOCCBRK, ...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
				return false;	// WITH breakset STILL TRUE !
			}
		}
#ifdef __SERIALCOM_DEBUG__
		else
			std::cout << "serialCOM::SendBreak(...):   ioctl(..., TIOCSBRK, ...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
	}
	breakset = false;
	if (confirmSB == -1)
		return false;
	return true;
}


bool serialCOM::SetBreak()
{
	int confirmIOCTL = -1;
	if (!portisopen) return false;
	confirmIOCTL = ioctl(fd, TIOCSBRK, 0);	// break ON
	if (confirmIOCTL == -1)
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::SetBreak(...):   ioctl(..., TIOCSBRK, ...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
		return false;
	}
	breakset = true;
	return true;
}


bool serialCOM::ClearBreak()
{
	int confirmIOCTL = -1;
	if (!portisopen) return false;
	confirmIOCTL = ioctl(fd, TIOCCBRK, 0);    // break OFF
	if (confirmIOCTL == -1)
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::ClearBreak(...):   ioctl(..., TIOCCBRK, ...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
		return false;
	}
	breakset = false;
	return true;
}


bool serialCOM::BreakIsSet()
{
	return breakset;
}


bool serialCOM::GetNrOfBytesAvailable(unsigned int *nbytes)
{
	int bytes = 0;
	if (!portisopen) return false;
	if (ioctl(fd, FIONREAD, &bytes) != -1)
	{
		*nbytes = static_cast<unsigned int>(bytes);
		return true;
	}
	else
	{
		*nbytes = 0;
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::GetNrOfBytesAvailable(...):   ioctl(..., FIONREAD, ...) failed with error " << errno << " " << strerror(errno) << "\n";
#endif
		return false;
	}
}


serialCOM::~serialCOM()
{
	if (portisopen) ClosePort();
}

// PRIVATE

bool serialCOM::GetStdbaudrateDCBConst(double baudrate, speed_t *DCBbaudconst)
{
	// B0 not used, because of Windows compatibility; B110, B134: divisor not unique 
	const double stdbaudrates[] = {50,75,110,134.5,150,200,300,600,1200,1800,2400,4800,9600,19200,38400,57600,115200};
	const speed_t stdbaudconst[] = {B50,B75,B110,B134,B150,B200,B300,B600,B1200,B1800,B2400,B4800,B9600,B19200,B38400,B57600,B115200};
	unsigned char k;
	for (k=0; k<17; k++)
	{
		if (stdbaudrates[k] == baudrate)
		{
			*DCBbaudconst = stdbaudconst[k];
			return true;
		}
	}
	return false;
}


speed_t serialCOM::GetNearestStdBaudrate(double selBaudrate)
{
	// Get nearest standard baudrate:
	speed_t nearestBaudrate = 0;
	const double br[17] = {50, 75, 110, 134.5, 150, 200, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
	const speed_t bc[17] = {B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800, B2400, B4800, B9600, B19200, B38400, B57600, B115200};
	if (selBaudrate <= br[0])
	{
		nearestBaudrate=bc[0];
	}
	else if (selBaudrate >= br[16])
	{
		nearestBaudrate=bc[16];
	}
	else
	{
		double q2=0;
		double q1=0;
		for (int b=1; b<17; b++)
		{
			q2 = br[b] / selBaudrate; 
			if (q2 >= 1)
			{
				// br[b-1] < baudrate < br[b]:
				q1 = br[b-1] / selBaudrate; 
				// compare relative baudrate deviation, select baudrate:
				if ((q2-1) < (1-q1))
					nearestBaudrate=bc[b];
				else
					nearestBaudrate=bc[b-1];
				break;
			}
		}
	}
	return nearestBaudrate;
}
