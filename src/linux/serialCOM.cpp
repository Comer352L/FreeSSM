/*
 * serialCOM.cpp - Serial port configuration and communication
 *
 * Copyright © 2008-2009 Comer352l
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
	dp = opendir ("/dev");	// open directory /dev:
	if (dp != NULL)		// if directory is open
	{
		do {
			fp = readdir (dp);     // get next file in directory
			if (fp != NULL)        // if file is available
			{
				if ((!strncmp(fp->d_name,"ttyS",4)) | (!strncmp(fp->d_name,"ttyUSB",6)))	// if filename starts with "ttyS" or "ttyUSB"
				{
					// CONSTRUCT FULL FILENAME:
					strcpy(ffn, "/dev/");		// (replaces old string)
					strcat(ffn, fp->d_name);
					testfd = -1;
					testfd = open(ffn, O_RDWR | O_NOCTTY | O_NDELAY);	// open device file
					if (!(testfd == -1))					// if device file is open
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
	else
		std::cout << "serialCOM::GetAvailablePorts():   opendir(''/dev'') failed with error " << errno << "\n";
	// SEARCH FOR USB2SERAIL-CONVERTERS:
	dp = opendir ("/dev/usb");	// open directory /dev/usb:
	if (dp != NULL)		// if directory is open
	{
		do {
			fp = readdir (dp);     // get next file in directory
			if (fp != NULL)        // if file is available
			{
				if (!strncmp(fp->d_name,"ttyUSB",6))	// if filename starts with "ttyUSB"
				{
					// CONSTRUCT FULL FILENAME:
					strcpy(ffn, "/dev/usb/");		// (replaces old string)
					strcat(ffn, fp->d_name);
					testfd = -1;
					testfd = open(ffn, O_RDWR | O_NOCTTY | O_NDELAY);	// open device file
					if (!(testfd == -1))					// if device file is open
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
	if ((portisopen == true) && (fd >= 0))
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
	short int cvTGA = -1;	// 0=OK , -1=ERROR
	short int cvIOCTL = -1;	// -1=ERROR , others=OK
	bool settingsvalid = true;
	struct termios currenttio;
	memset(&currenttio, 0, sizeof(termios));
	speed_t inputbaudrate=0, outputbaudrate=0;	// speed_t defined in termios.h
	unsigned long int cleanedbitmask=0;
	// Reset data:
	currentportsettings->baudrate = 0;
	currentportsettings->databits = 0;
	currentportsettings->parity = 0;
	currentportsettings->stopbits = 0;
	if (portisopen == false) return false;
	// Query current settings:
	cvTGA = tcgetattr(fd, &currenttio);
	if (cvTGA == 0)
	{
		// BAUDRATE:
		inputbaudrate = cfgetispeed(&currenttio);	// get input baud rate
		outputbaudrate = cfgetospeed(&currenttio);	// get output baud rate
		if (inputbaudrate == outputbaudrate)
		{
			if (outputbaudrate == B0)	// B0 is not allowed (=> Windows compatibility)
			{
				currentportsettings->baudrate = 0;
			}
			else if (outputbaudrate == B50)
			{
				currentportsettings->baudrate = 50;
			}
			else if (outputbaudrate == B75)
			{
				currentportsettings->baudrate = 75;
			}
			/*else if (outputbaudrate == B110)
			{
				currentportsettings->baudrate = 110;
			}
			else if (outputbaudrate == B134)
			{
				currentportsettings->baudrate = 134;	// 134.5 !
			}*/
			else if (outputbaudrate == B150)
			{
				currentportsettings->baudrate = 150;
			}
			else if (outputbaudrate == B200)
			{
				currentportsettings->baudrate = 200;
			}
			else if (outputbaudrate == B300)
			{
				currentportsettings->baudrate = 300;
			}
			else if (outputbaudrate == B600)
			{
				currentportsettings->baudrate = 600;
			}
			else if (outputbaudrate == B1200)
			{
				currentportsettings->baudrate = 1200;
			}
			else if (outputbaudrate == B2400)
			{
				currentportsettings->baudrate = 2400;
			}
			else if (outputbaudrate == B4800)
			{
				currentportsettings->baudrate = 4800;
			}
			else if (outputbaudrate == B9600)
			{
				currentportsettings->baudrate = 9600;
			}
			else if (outputbaudrate == B19200)
			{
				currentportsettings->baudrate = 19200;
			}
			else if (outputbaudrate == B38400)
			{
				if (serdrvaccess) // if we have access to the driver
				{
					// Get driver settings
					struct serial_struct current_serdrvinfo;
					memset(&current_serdrvinfo, 0, sizeof(serial_struct));
					cvIOCTL = ioctl(fd, TIOCGSERIAL, &current_serdrvinfo);
					if (cvIOCTL != -1)
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
								std::cout << "serialCOM::GetPortSettings():   error: custom baudrate with custom_divisor=0 detected\n";
							}
						}
					}
					else	// If driver settings are not available 
					{
						settingsvalid = false;
						std::cout << "serialCOM::GetPortSettings():   ioctl(..., TIOCGSERIAL, ...) failed with error " << errno << "\n";
					}
				}
				else
					currentportsettings->baudrate = 38400;
			}
			else if (outputbaudrate == B57600)
			{
				currentportsettings->baudrate = 57600;
			}
			else if (outputbaudrate == B115200)
			{
				currentportsettings->baudrate = 115200;
			}
			else
			{
				settingsvalid = false;
				std::cout << "serialCOM::GetPortSettings():   error: unknown baudrate\n";
			}
		}
		else	// if different baudrate settings for input and output detected
		{
			settingsvalid = false;
			std::cout << "serialCOM::GetPortSettings():   WARNING:   different baud rates for transmitting and recieving detected\n";
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
			std::cout << "serialCOM::GetPortSettings():   error: unknown number of databits\n";
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
	}
	// RETURN VALUE (SUCCESS CONTROL):
	if ((cvTGA == -1) | (settingsvalid == false))
		return false;
	else
		return true;
}


bool serialCOM::SetPortSettings(serialCOM::dt_portsettings newportsettings)
{
	short int cvTSA = -1;
	bool settingsvalid = true;
	speed_t newbaudrate=0;	// speed_t defined in termios.h
	struct termios newtio;
	memset(&newtio, 0, sizeof(termios));
	if (portisopen == false) return false;
	struct serial_struct new_serdrvinfo;
	memset(&new_serdrvinfo, 0, sizeof(serial_struct));
	if (serdrvaccess)
	{
		// Get current port settings:
		ioctl(fd, TIOCGSERIAL, &new_serdrvinfo);	// read from driver
		// Deactivate custom baudrate settings:
		new_serdrvinfo.custom_divisor = 0;
		new_serdrvinfo.flags &= ~ASYNC_SPD_CUST;
	}
	// SET CONTROL OPTIONS:
	newtio.c_cflag = (CREAD | CLOCAL);
	// BAUDRATE:
	// Set new baudrate:
	if (!(newportsettings.baudrate > 0))
	{
		settingsvalid = false;
		if (newportsettings.baudrate == 0)
			std::cout << "serialCOM::SetPortSettings:   error: illegal baudrate - 0 baud not possible\n";
		else
			std::cout << "serialCOM::SetPortSettings:   error: illegal baudrate - baud must be > 0\n";
	}
	else if (newportsettings.baudrate > 115200)
	{
		settingsvalid = false;
		std::cout << "serialCOM::SetPortSettings:   error: illegal baudrate - baudrates above 115200 are currently not supported\n";
	}
	else
	{
		unsigned long int DCBbaudconst=0;
		bool cvGSDCBC = false;
		cvGSDCBC = GetStdbaudrateDCBConst(newportsettings.baudrate, &DCBbaudconst);
		if (cvGSDCBC==true)	// if it's a standard baud rate
			newbaudrate = DCBbaudconst;
		else if (serdrvaccess)	// if it's not a standard baud rate and access to serial driver
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
			cvGSDCBC = serialCOM::GetStdbaudrateDCBConst(custombaudrate, &DCBbaudconst);
			if (cvGSDCBC==true)
				newbaudrate = DCBbaudconst;
			else
			{
				newbaudrate = B38400;
				new_serdrvinfo.flags |= ASYNC_SPD_CUST;
				new_serdrvinfo.custom_divisor = customdivisor;
			}
		}
		else
		{
			// Get nearest standard baudrate:
			double br[14] = {50, 75, /*110, B134.5*/ 150, 200, 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
			int bc[14] = {B50, B75, /*B110, B134*/ B150, B200, B300, B600, B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200};
			if (newportsettings.baudrate <= br[0])
				newbaudrate=static_cast<speed_t>(br[0]);
			else
			{
				double q2=0;
				double q1=0;
				for (int b=1; b<14; b++)
				{
					q2 = br[b] / newportsettings.baudrate; 
					if (q2 >= 1)
					{
						// br[b-1] < baudrate < br[b]:
						q1 = br[b-1] / newportsettings.baudrate; 
						// compare relative baudrate abberation, select baudrate:
						if ((q2-1) < (1-q1))
							newbaudrate=bc[b];
						else
							newbaudrate=bc[b-1];
						break;
					}
				}
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
			std::cout << "serialCOM::SetPortSettings():  error: illegal number of databits\n";
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
		std::cout << "serialCOM::SetPortSettings():  error: illegal parity option\n";
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
			std::cout << "serialCOM::SetPortSettings():   error: invalid value for parameter 'stopbits': 1.5 stopbits only allowed in combination with 5 databits\n";
		}
	}
	else if (newportsettings.stopbits == 2)
	{
		if (newportsettings.databits != 5)
			newtio.c_cflag |= CSTOPB;
		else
		{
			settingsvalid = false;
			std::cout << "serialCOM::SetPortSettings():   error: invalid value for parameter 'stopbits': 2 stopbits not allowed in combination with 5 databits\n";
		}
	}
	else
	{
		settingsvalid = false;
		std::cout << "serialCOM::SetPortSettings():   error: invalid value for parameter 'stopbits'" << '\n';
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
	if (settingsvalid == true)	// only apply new settings only if they are all valid !
	{
		ioctl(fd, TIOCSSERIAL, &new_serdrvinfo);	// write new driver settings
		cfsetispeed(&newtio, newbaudrate);
		cfsetospeed(&newtio, newbaudrate);
		cvTSA = tcsetattr(fd, TCSANOW, &newtio );
		/*	TCSANOW		Make changes now without waiting for data to complete
			TCSADRAIN	Wait until everything has been transmitted
			TCSAFLUSH	Flush input and output buffers and make the change	*/
	}
	// SUCCESS CONTROL (RETURN VALUE):
	if (cvTSA == 0)
		return true;
	else
		return false;
}


bool serialCOM::OpenPort(std::string portname)
{
	int confirm = -1;	// -1=error
	struct serial_struct new_serdrvinfo;	// new driver settings
	if (portisopen == true) return false;	// if port is already open => cancel and return "false"
	memset(&new_serdrvinfo, 0, sizeof(serial_struct));
	memset(&oldtio, 0, sizeof(termios));
	memset(&old_serdrvinfo, 0, sizeof(serial_struct));
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
		if (confirm == -1)
			std::cout << "serialCOM::OpenPort():   ioctl(..., TIOCEXCL, ...) failed with error " << errno << "\n";
		confirm = fcntl(fd, F_SETFL, FNDELAY);	// function "read" shall return "0" if no data available
		if (confirm == -1)
		{
			std::cout << "serialCOM::OpenPort():   fcntl(...) failed with error " << errno << "\n";
			if (!ClosePort())
				std::cout << "serialCOM::OpenPort():   port couldn't be closed after error during opening process" << errno << "\n";
			return false;
		}
		// SAVE SETTINGS:
		confirm = tcgetattr(fd, &oldtio);	// save current serial port settings for restore when closing port
		if (confirm == -1)
		{
			std::cout << "serialCOM::OpenPort():   tcgetattr(...) failed with error " << errno << "\n";
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
			if (confirm == -1)
				std::cout << "serialCOM::OpenPort():   ioctl(..., TIOCSSERIAL, ...) failed with error " << errno << "\n";
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
		confirm = tcflush(fd, TCIOFLUSH);
		if (confirm == -1)
		{
			std::cout << "serialCOM::OpenPort():   tcflush(..., TCIOFLUSH) failed with error " << errno << "\n";
			if (!ClosePort())
				std::cout << "serialCOM::OpenPort():   port couldn't be closed after error during opening process" << errno << "\n";
			return false;
		}
		return true;
	}
	else
	{
		std::cout << "serialCOM::OpenPort():   open(...) failed with error " << errno << "\n";
		return false;
	}
}


bool serialCOM::ClosePort()
{
	int confirm = -1;	// -1=error
	if (portisopen == false) return false;
	// CLEAR BREAK:
	confirm = ioctl(fd, TIOCCBRK, 0);    // break OFF
	if (confirm != -1)
		breakset = false;
	else
		std::cout << "serialCOM::ClosePort():   ioctl(..., TIOCCBRK, ...) failed with error " << errno << "\n";
	// CLEAR HARDWARE BUFFERS:
	confirm = tcflush(fd, TCIOFLUSH);	        // clear buffers (input and output)
	if (confirm != 0)
		std::cout << "serialCOM::ClosePort():   tcflush(...) failed with error " << errno << "\n";
	// RESTORE PORT SETTINGS:
	if (serdrvaccess)
	{
		confirm = ioctl(fd, TIOCSSERIAL, &old_serdrvinfo);	// restore old driver settings
		if (confirm == -1)
			std::cout << "serialCOM::ClosePort():   ioctl(..., TIOCSSERIAL, ...) failed with error " << errno << "\n";
	}
	if (settingssaved)
		confirm = tcsetattr(fd, TCSANOW, &oldtio);	// restore the old port settings
	if (confirm != 0)
		std::cout << "serialCOM::ClosePort():   tcsetattr(...) failed with error " << errno << "\n";
	confirm = ioctl(fd, TIOCNXCL, NULL);	// unlock device
	if (confirm == -1)
		std::cout << "serialCOM::ClosePort():   ioctl(..., TIOCNXCL, ...) failed with error " << errno << "\n";
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
		std::cout << "serialCOM::ClosePort():   close(...) failed with error " << errno << "\n";
		return false;
	}
}


bool serialCOM::Write(char *outputstr, unsigned int nrofbytestowrite)
{
	int confirm = -1;
	unsigned int nrofbyteswritten = 0;
	if (portisopen == false) return false;
	if (breakset)
	{
		confirm = ioctl(fd, TIOCCBRK, 0);	// break OFF
		if (confirm != -1)
			breakset = false;
		else
		{
			std::cout << "serialCOM::Write():   ioctl(..., TIOCCBRK, ...) failed with error " << errno << "\n";
			return false;
		}
	}
	// SEND DATA:
	nrofbyteswritten = write(fd, outputstr, nrofbytestowrite);
	confirm = tcdrain(fd); // wait for all output to be transmitted (to the buffer !?)
	// RETURN VALUE:
	if ((nrofbyteswritten == nrofbytestowrite) && (confirm==0))
		return true;
	else
	{
		if (nrofbyteswritten != nrofbytestowrite)
			std::cout << "serialCOM::Write():   write(..) failed with error " << errno << "\n";
		if (confirm != 0)
			std::cout << "serialCOM::Write():   tcdrain(..) failed with error " << errno << "\n";
		return false;
	}
}


bool serialCOM::Read(char *readdata, unsigned int *nrofbytesread)
{
	int ret;
	*nrofbytesread = 0;
	if (portisopen == false) return false;
	// READ AVAILABLE DATA:
	ret = read(fd, readdata, 512);
	// RETURN VALUE:
	if ((ret < 0) || (ret > 512))	// 512: important ! => possible if fd was not open !
	{
		*nrofbytesread = 0;
		std::cout << "serialCOM::Read():   read(..) failed with error " << errno << "\n";
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
	if (portisopen == true)
		cvTF = tcflush(fd, TCOFLUSH);
	if (cvTF == 0)
		return true;
	else
	{
		std::cout << "serialCOM::ClearSendBuffer(...):   tcflush(..., TCOFLUSH) returned error " << errno << "\n";
		return false;
	}
}


bool serialCOM::ClearRecieveBuffer()
{
	int cvTF = -1;
	if (portisopen == true)
		cvTF = tcflush(fd, TCIFLUSH);
	if (cvTF == 0)
		return true;
	else
	{
		std::cout << "serialCOM::ClearRecieveBuffer(...):   tcflush(..., TCIFLUSH) returned error " << errno << "\n";
		return false;
	}
}


bool serialCOM::SendBreak(unsigned int duration_ms)
{
	short int confirmTSB = -1;	// 0 or -1
	if ((portisopen == false) || (duration_ms < 1) || (duration_ms >= 32767))
		return false;
	breakset = true;
	confirmTSB = tcsendbreak(fd, static_cast<int>(duration_ms));
	breakset = false;
	// ALTERNATIVE:
	//ioctl(fd, TIOCSBRK, 0);	// break ON
	// msleep( (int)duration_ms );
	//ioctl(fd, TIOCCBRK, 0);    // break OFF
	if (confirmTSB == 0)
		return true;
	else
	{
		std::cout << "serialCOM::SendBreak(...):   tcsendbreak(...) returned error " << errno << "\n";
		return false;
	}
}


bool serialCOM::SetBreak()
{
	short int confirmIOCTL = -1;
	if (portisopen == false) return false;
	confirmIOCTL = ioctl(fd, TIOCSBRK, 0);	// break ON
	if (confirmIOCTL == -1)
	{
		std::cout << "serialCOM::SetBreak(...):   ioctl(..., TIOCSBRK, ...) returned error " << errno << "\n";
		return false;
	}
	breakset = true;
	return true;
}


bool serialCOM::ClearBreak()
{
	short int confirmIOCTL = -1;
	if (portisopen == false) return false;
	confirmIOCTL = ioctl(fd, TIOCCBRK, 0);    // break OFF
	if (confirmIOCTL == -1)
	{
		std::cout << "serialCOM::ClearBreak(...):   ioctl(..., TIOCCBRK, ...) returned error " << errno << "\n";
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
	if (portisopen == false) return false;
	if (ioctl(fd, FIONREAD, &bytes) != -1)
	{
		*nbytes = static_cast<unsigned int>(bytes);
		return true;
	}
	else
	{
		*nbytes = 0;
		std::cout << "serialCOM::GetNrOfBytesAvailable(...):   ioctl(..., FIONREAD, ...) returned error " << errno << "\n";
		return false;
	}
}


serialCOM::~serialCOM()
{
	if (portisopen) ClosePort();
}


// PRIVATE
bool serialCOM::GetStdbaudrateDCBConst(double baudrate, unsigned long int *DCBbaudconst)
{
	// B0 not used, because of Windows compatibility; B110, B134: divisor not unique 
	const unsigned long int stdbaudrates[] = {50,75,150,200,300,600,1200,2400,4800,9600,19200,38400,57600,115200};
	const unsigned long int stdbaudconst[] = {B50,B75,B150,B200,B300,B600,B1200,B2400,B4800,B9600,B19200,B38400,B57600,B115200};
	unsigned char k;
	for (k=0; k<14; k++)
	{
		if (stdbaudrates[k] == baudrate)
		{
			*DCBbaudconst = stdbaudconst[k];
			return true;
		}
	}
	return false;
}

