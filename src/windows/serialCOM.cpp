/*
 * serialCOM.cpp - Serial port configuration and communication
 *
 * Copyright (C) 2008-2014 Comer352L
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
	hCom = 0;
	portisopen = false;
	breakset = false;
	DTRset = true;
	RTSset = true;
	currentportname = "";
	read_timeout_set = false;
	last_read_timeout = 0;
	memset(&olddcb, 0, sizeof(DCB));
	olddcb.DCBlength = sizeof(DCB);
	settingssaved = false;
}


serialCOM::~serialCOM()
{
	if (portisopen) ClosePort();
}


std::vector<std::string> serialCOM::GetAvailablePorts()
{
	std::vector<std::string> portlist(0);
	HKEY hKey;				// handle to registry key
	DWORD index = 0;			// index registry-key         unsigned int (32bit)
	char ValueName[256] = "";
	unsigned long szValueName = 256;	// variable that specifies the size (in characters, including the terminating null char) of the buffer pointed to by the "ValueName" parameter.
	unsigned char Data[256] = "";		// buffer that receives the data for the value entry. This parameter can be NULL if the data is not required
	unsigned long szData = 256;		// variable that specifies the size, in bytes, of the buffer pointed to by the lpData parameter.
	long cv;
	HANDLE hCom_t = NULL;
	// OPEN REGISTRY-KEY AND BROWSE ENTRYS:
	cv = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey);
	if (cv == ERROR_SUCCESS) // ERROR_SUCCESS is of type long
	{
		while ((RegEnumValueA(hKey, index, ValueName, &szValueName, NULL, NULL,Data,&szData)) == ERROR_SUCCESS)
		{
			if (!strncmp((char*)Data,"COM",3))	// compares the first 3 characters
			{
				// CHECK IF PORT IS AVAILABLE (not in use):
				char NTdevName[20] = "\\\\.\\";	// => "\\.\"
				strcpy(NTdevName+4, (char*)Data);
				/* NOTE: MS-DOS device names ("COMx") are not working reliable if x is > 9 !!!
					=> device can not be opened (error 2 "The system cannot find the file specified.")
					Using NT device names instead ("\\.\COMx")
				*/
				hCom_t = CreateFileA(NTdevName,				// device name of the port
							 GENERIC_READ | GENERIC_WRITE,	// read/write access
							 0,					// must be opened with exclusive-access
							 NULL,				// default security attributes
							 OPEN_EXISTING,			// must use OPEN_EXISTING
							 0,					// not overlapped I/O
							 NULL				// must be NULL for comm devices
							);
				if (hCom_t != INVALID_HANDLE_VALUE)
				{
#ifndef __SERIALCOM_DEBUG__
					CloseHandle(hCom_t);
#else
					if (!CloseHandle(hCom_t))
						std::cout << "serialCOM::GetAvailablePorts():   CloseHandle(...) failed with error " << GetLastError() << "\n";
					std::cout << "serialCOM::GetAvailablePorts():   registered port " << (char*)Data << " is available\n";
#endif
					portlist.push_back((char*)Data);
				}
#ifdef __SERIALCOM_DEBUG__
				else
					std::cout << "serialCOM::GetAvailablePorts():   registered port " << (char*)Data << " is not available:   error " << GetLastError() << "\n";
#endif
			}
			szValueName = 256;		// because RegEnumValue has changed value
			szData = 256;			// because RegEnumValue has changed value
			index++;
		}
		std::sort(portlist.begin(), portlist.end());	// quicksort from <algorithm>
		cv = RegCloseKey(hKey);
#ifdef __SERIALCOM_DEBUG__
		if (cv != ERROR_SUCCESS)
			std::cout << "serialCOM::GetAvailablePorts():   RegCloseKey(...) failed with error " << cv << "\n";
#endif
	}
#ifdef __SERIALCOM_DEBUG__
	else
		std::cout << "serialCOM::GetAvailablePorts():   RegOpenKeyEx(...) failed with error " << cv << "\n";
#endif
	// RETURN VALUE:
	return portlist;
}


bool serialCOM::IsOpen()
{
	return (portisopen && (hCom != INVALID_HANDLE_VALUE) && (hCom != NULL));
}


std::string serialCOM::GetPortname()
{
	return currentportname;
}


bool serialCOM::GetPortSettings(double *baudrate, unsigned short *databits, char *parity, float *stopbits)
{
	/* NOTE: NULL-pointer arguments allowed ! */
	bool cvGMbr = false;
	bool settingsvalid = true;
	double maxbaudrate = 0;
	unsigned long int divisor = 0;
	DCB currentdcb;
	memset(&currentdcb, 0, sizeof(DCB));
	currentdcb.DCBlength = sizeof(DCB);
	if (!portisopen) return false;
	// REQUEST PORT SETTINGS FROM SYSTEM:
	if (!GetCommState(hCom, &currentdcb))
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::GetPortSettings():   GetCommState(...) failed\n";
#endif
		return false;
	}
	// BAUDRATE SETTINGS:
	if (baudrate)
	{
		if (currentdcb.BaudRate != 0)
		{
			cvGMbr = serialCOM::GetMaxbaudrate(&maxbaudrate);
			if (cvGMbr)
			{
				divisor = static_cast<unsigned long int>(round(maxbaudrate / currentdcb.BaudRate));
				*baudrate = maxbaudrate / divisor;
			}
			else
			{
				*baudrate = currentdcb.BaudRate;
#ifdef __SERIALCOM_DEBUG__
				std::cout << "serialCOM::SetPortSettings:   unable to detect baudbase\n";
				std::cout << "                              WARNING: reported baud rate may differ from real baudrate !\n";
#endif
			}
		}
		else
		{
			*baudrate = 0;
			settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::GetPortSettings():   System reports baudrate=0\n";
#endif
		}
	}
	// DATABITS SETTINGS:
	if (databits)
		*databits = currentdcb.ByteSize;
	// PARITY SETTINGS:
	if (parity)
	{
		if (currentdcb.Parity == NOPARITY)
		{
			*parity = 'N';             // no parity bit
		}
		else if (currentdcb.Parity == ODDPARITY)
		{
			*parity = 'O';             // Odd
		}
		else if (currentdcb.Parity == EVENPARITY)
		{
			*parity = 'E';             // Even
		}
		else if (currentdcb.Parity == MARKPARITY)
		{
			*parity = 'M';             // Mark
		}
		else if (currentdcb.Parity == SPACEPARITY)
		{
			*parity = 'S';             // Space
		}
		else
		{
			settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::GetPortSettings():   unknown parity settings\n";
#endif
		}
	}
	// STOPBITS SETTINGS:
	if (stopbits)
	{
		if (currentdcb.StopBits == ONESTOPBIT)
		{
			*stopbits = 1;
		}
		else if (currentdcb.StopBits == ONE5STOPBITS)
		{
			*stopbits = 1.5;
		}
		else if (currentdcb.StopBits == TWOSTOPBITS)
		{
			*stopbits = 2;
		}
		else
		{
			settingsvalid = false;
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::GetPortSettings():   unknown stopbits settings\n";
#endif
		}
	}
	// RETURN SUCCESS:
	return settingsvalid;
}


bool serialCOM::SetPortSettings(double baudrate, unsigned short databits, char parity, float stopbits)
{
	bool confirmSCS=false, nsvalid=true, cvGMbr=false;
	double maxbaudrate = 0, exactbaudrate = 0;
	unsigned int bauddivisor = 0;
	DCB newdcb;
	memset(&newdcb, 0, sizeof(DCB));
	newdcb.DCBlength = sizeof(DCB);
	if (!portisopen) return false;
	if (!GetCommState(hCom, &newdcb))
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::SetPortSettings():   GetCommState(...) failed\n";
#endif
		return false;
	}
	// SET NEW PORT SETTINGS (not all will be changed):
	// BAUDRATE:
	if (baudrate <= 0)
	{
		nsvalid = false;
#ifdef __SERIALCOM_DEBUG__
		if (baudrate == 0)
			std::cout << "serialCOM::SetPortSettings:   illegal baudrate - 0 baud not possible\n";
		else
			std::cout << "serialCOM::SetPortSettings:   illegal baudrate - baud must be > 0\n";
#endif
	}
	else
	{
		cvGMbr = serialCOM::GetMaxbaudrate(&maxbaudrate);	// get max. available baudrate
		if (!cvGMbr)
		{
			newdcb.BaudRate = static_cast<DWORD>(round(baudrate));	// set baud rate directly
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::SetPortSettings:   unable to detect baudbase\n";
			std::cout << "                              WARNING: reported baud rate may differ from real baudrate !\n";
#endif
		}
		else
		{
			if (baudrate > maxbaudrate)
			{
				nsvalid = false;
#ifdef __SERIALCOM_DEBUG__
				std::cout << "serialCOM::SetPortSettings:   error: illegal baudrate - the maximum supported baud rate of this device is " << std::dec << maxbaudrate << " baud\n";
#endif
			}
			else
			{
				// Calculate "exact" baudrate:
				bauddivisor = static_cast<unsigned int>(round(maxbaudrate / baudrate));
				if (bauddivisor < 1) bauddivisor = 1;
				if (bauddivisor > 65535) bauddivisor = 65535;
				exactbaudrate = (maxbaudrate / bauddivisor);	// data type DWORD restricts possible extremely low baud rates !
				/* NOTE: DO NOT ROUND HERE !
				 * setCommState() doesn't round, it simply cuts the decimals => we get problems at e.g. 10400 baud !
				 */
				newdcb.BaudRate = static_cast<DWORD>(exactbaudrate);	// standard baudrates: CBR_9600, CBR_4800, ...
			}
		}
	}
	// DATABITS SETTINGS:
	if ((databits >= 5) && (databits <= 8))
		newdcb.ByteSize = databits;
	else
	{
		nsvalid = false;
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::SetPortSettings():   invalid value for parameter 'databits'\n";
#endif
	}
	// PARITY SETTINGS:
	if (parity == 'N')
	{
		newdcb.Parity = NOPARITY;                // no parity bit
	}
	else if (parity == 'O')
	{
		newdcb.Parity = ODDPARITY;               // Odd
	}
	else if (parity == 'E')
	{
		newdcb.Parity = EVENPARITY;              // Even
	}
	else if (parity == 'M')
	{
		newdcb.Parity = MARKPARITY;              // Mark
	}
	else if (parity == 'S')
	{
		newdcb.Parity = SPACEPARITY;             // Space
	}
	else
	{
		nsvalid = false;
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::SetPortSettings():   invalid value for parameter 'parity'\n";
#endif
	}
	// STOPBIT SETTINGS:      (=> The use of 5 data bits with 2 stop bits is an invalid combination, as is 6, 7, or 8 data bits with 1.5 stop bits)
	if (stopbits == 1)
	{
		newdcb.StopBits = ONESTOPBIT;		// 1 stop bit
	}
	else if (stopbits == 1.5)
	{
		if (databits == 5)
			newdcb.StopBits = ONE5STOPBITS;	// 1.5 stop bits
		else
		{
			nsvalid = false;
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::SetPortSettings():   invalid value for parameter 'stopbits': 1.5 stopbits only allowed in combination with 5 databits\n";
#endif
		}
	}
	else if (stopbits == 2)
	{
		if (databits != 5)
			newdcb.StopBits = TWOSTOPBITS;	// 2 stop bits
		else
		{
			nsvalid = false;
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::SetPortSettings():   invalid value for parameter 'stopbits': 2 stopbits not allowed in combination with 5 databits\n";
#endif
		}
	}
	else
	{
		nsvalid = false;
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::SetPortSettings():   invalid value for parameter 'stopbits'\n";
#endif
	}
	// MORE OPTIONS:
	newdcb.fBinary = true;	// binary mode (false is not supported by Windows)
	newdcb.fParity = false;	// no parity checking
	newdcb.fNull = false;	// VERY IMPORTANT: don't discard received zero characters
	// FLOW CONTROL:
	newdcb.fOutxCtsFlow = false;	// CTS disabled
	newdcb.fOutxDsrFlow = false;	// DSR disabled
	if (DTRset)
		newdcb.fDtrControl = DTR_CONTROL_ENABLE;	// DTR enabled = "ready"
	else
		newdcb.fDtrControl = DTR_CONTROL_DISABLE;	// DTR enabled = "ready"
	if (RTSset)
		newdcb.fRtsControl = RTS_CONTROL_ENABLE;	// RTS enabled = "request"
	else
		newdcb.fRtsControl = RTS_CONTROL_DISABLE;	// RTS enabled = "request"
	// NOTE: Important: set to XXX_CONTROL_DISABLE or XXX_CONTROL_ENABLE to ensure that flow control is disabled !
	newdcb.fDsrSensitivity = false;
	newdcb.fOutX = false;		// XON/XOFF (for transmission) diabled
	newdcb.fInX = false;		// XON/XOFF (for reception) diabled
	// FLOW CONTROL DETAILS:
	// newdcb.XonLim = 0;
	// newdcb.XoffLim = 0;
	newdcb.fTXContinueOnXoff = true;
	// ERROR CONTROL:
	newdcb.fErrorChar = false;	// character replacment on parity errors disbaled (only used, if fParity = true)
	newdcb.fAbortOnError = false;
	// CHARACTERS FOR FLOW/ERROR CONTROL:
	// newdcb.XonChar;
	// newdcb.XoffChar;
	// newdcb.ErrorChar;
	// newdcb.EofChar;
	// newdcb.EvtChar;
	// APPLY NEW PORT SETTINGS:
	if (nsvalid)
		confirmSCS = SetCommState(hCom, &newdcb);
#ifdef __SERIALCOM_DEBUG__
	if (!confirmSCS)
		std::cout << "serialCOM::SetPortSettings():   SetCommState(...) failed\n";
#endif
	// RETURN SUCCESS:
	return confirmSCS;
}


bool serialCOM::OpenPort(std::string portname)
{
	bool confirm;
	if (portisopen) return false;
	// OPEN PORT:
	std::string NTdevName = "\\\\.\\";	// => "\\.\"
	NTdevName += portname;
	/* NOTE: MS-DOS device names ("COMx") are not working reliable if x is > 9 !!!
		=> device can not be opened (error 2 "The system cannot find the file specified.")
		Using NT device names instead ("\\.\COMx")
	*/
	hCom = CreateFileA(NTdevName.c_str(),			// name of port
			   GENERIC_READ | GENERIC_WRITE,	// read/write access
			   0,					// must be opened with exclusive-access
			   NULL,				// default security attributes
			   OPEN_EXISTING,			// must use OPEN_EXISTING
			   0,					// not overlapped I/O
			   NULL					// must be NULL for comm devices
			  );
	// CHECK IF OPENING PORT WAS SUCCESSFUL:
	if ((hCom == INVALID_HANDLE_VALUE) | (hCom == NULL))	      // if error while opening port
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::OpenPort():   CreateFileA(...) failed with error " << GetLastError() << "\n";
#endif
		return false;
	}
	portisopen = true;
	currentportname = portname;
	// SAVE PORT SETTINGS:
	memset(&olddcb, 0, sizeof(DCB));
	olddcb.DCBlength = sizeof(DCB);
	confirm = GetCommState(hCom, &olddcb);
#ifdef __SERIALCOM_DEBUG__
	if (!confirm)
		std::cout << "serialCOM::OpenPort():   GetCommState(...) failed with error " << GetLastError() << "\n";
#endif
	settingssaved = confirm;
	// SET TIMEOUTS (=> Timeouts disabled; read operation immediatly returns received characters):
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = MAXDWORD;	// Max. time between two arriving characters
	timeouts.ReadTotalTimeoutMultiplier = 0;	// Multiplied with nr. of characters to read
	timeouts.ReadTotalTimeoutConstant = 0;		// Total max. time for read operations = TotalTimeoutMultiplier * nrOfBytes + TimeoutConstant
	timeouts.WriteTotalTimeoutConstant = 0;		// Total max. time for write operations = TotalTimeoutMultiplier * nrOfBytes + TimeoutConstant
	timeouts.WriteTotalTimeoutMultiplier = 0;	// Multiplied with nr. of characters to write
	// all values in [ms]; if one of these max. times is exceeded, Read / Write Operation is stoped and nr. of Read/WrittenBytes is given back
	confirm = SetCommTimeouts(hCom, &timeouts);	// Apply new timeout settings
	if (!confirm)
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::OpenPort():   SetCommTimeouts(...) failed with error " << GetLastError() << "\n";
#endif
		confirm = ClosePort();
#ifdef __SERIALCOM_DEBUG__
		if (!confirm)
			std::cout << "serialCOM::OpenPort():   Port couldn't be closed after error during opening process\n";
#endif
		return false;
	}
	read_timeout_set = false;
/*
	// CLEAR BREAK (should not be necessary, because break is cancelled automatically after closing the device):
	confirm = ClearCommBreak(hCom);
	if (!confirm)
		std::cout << "serialCOM::OpenPort():   ClearCommBreak(...) failed with error " << GetLastError() << "\n";	// debug-output
	else
*/
	breakset = false;
	// CLEAR HARDWARE BUFFERS:
	confirm = PurgeComm(hCom, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR);   // clears output buffer (if the device driver has one)
	if (!confirm)
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::OpenPort():   PurgeComm(...) failed with error " << GetLastError() << "\n";
#endif
		confirm = ClosePort();
#ifdef __SERIALCOM_DEBUG__
		if (!confirm)
			std::cout << "serialCOM::OpenPort():   Port couldn't be closed after error during opening process\n";
#endif
		return false;
	}
	// CONFIGURE COMMUNICATION, SET STANDARD PORT-SETTINGS
	if (!SetPortSettings(9600, 8, 'N', 1))
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::OpenPort():   Couldn't set standard port settings with SetPortSettings()\n";
#endif
		confirm = ClosePort();
#ifdef __SERIALCOM_DEBUG__
		if (!confirm)
			std::cout << "serialCOM::OpenPort():   Port couldn't be closed after error during opening process\n";
#endif
		return false;
		/* NOTE: SetPortSettings not only changes the 4 communication parameters.
				 It configures additional parameters which are
			 are important to ensure proper communication behavior !
		 */
	}
	/* NOTE: DTR+RTS control lines are set in SetPortSettings() */
	return true;
}


bool serialCOM::ClosePort()
{
	bool confirm = false;
	if (!portisopen) return false;
	// CLEAR BREAK:
	confirm = ClearCommBreak(hCom);
	if (confirm)
		breakset = false;
#ifdef __SERIALCOM_DEBUG__
	else
	{
		std::cout << "serialCOM::ClosePort():   ClearCommBreak(...) failed with error " << GetLastError() << "\n";
	}
#endif
	// CLEAR HARDWARE BUFFERS:
	confirm = PurgeComm(hCom, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR);   // clears output buffer (if the device driver has one)
#ifdef __SERIALCOM_DEBUG__
	if (!confirm)
	{
		std::cout << "serialCOM::ClosePort():   PurgeComm(...) failed with error " << GetLastError() << "\n";
	}
#endif
	// RESTORE OLD PORT SETTINGS:
	confirm = SetCommState(hCom, &olddcb);
#ifdef __SERIALCOM_DEBUG__
	if (!confirm)
	{
		std::cout << "serialCOM::ClosePort():   SetCommState(...) failed with error " << GetLastError() << "\n";
	}
#endif
	// CLOSE PORT:
	confirm = CloseHandle(hCom);
	if (!confirm)
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::ClosePort():   CloseHandle(...) failed with error " << GetLastError() << "\n";
#endif
		return false;
	}
	else
	{
		hCom = NULL;
		portisopen = false;
		breakset = false;
		currentportname="";
		return true;
	}
}


bool serialCOM::Write(std::vector<char> data)
{
	return Write(&data.at(0), data.size());
}


bool serialCOM::Write(char *data, unsigned int datalen)
{
	bool confirmWF=false, confirmCCB=false;
	DWORD BytesWritten = 0;
	if (!portisopen) return false;
	if (breakset)
	{
		confirmCCB = ClearCommBreak(hCom);
		if (confirmCCB)
			breakset = false;
		else
		{
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::Write():   ClearCommBreak(...) failed\n";
#endif
			return false;
		}
	}
	// SEND DATA:
	confirmWF = WriteFile (hCom,			// Port handle
				   data,			// Pointer to the data to write
				   datalen,			// Number of bytes to write
				   &BytesWritten,		// Pointer to the number of bytes written
				   NULL			// Pointer to an OVERLAPPED structure; Must be NULL if not supported
				  );
	// RETURN VALUE:
	if (confirmWF && (BytesWritten == static_cast<DWORD>(datalen)))
		return true;
	else
	{
#ifdef __SERIALCOM_DEBUG__
		if (!confirmWF)
			std::cout << "serialCOM::Write():   WriteFile(...) failed\n";
		if (BytesWritten != static_cast<DWORD>(datalen))
			std::cout << "serialCOM::Write():   some bytes could not be written\n";
#endif
		return false;
	}
}


bool serialCOM::Read(unsigned int minbytes, unsigned int maxbytes, unsigned int timeout, std::vector<char> *data)
{
	if (!portisopen || (minbytes > maxbytes) || (maxbytes > INT_MAX)) // NOTE: real limit: MAXDWORD
		return false;
	unsigned int rdatalen = 0;
	char *rdata = (char*) malloc(maxbytes);
	if (rdata == NULL) return false;
	bool ok = Read(minbytes, maxbytes, timeout, rdata, &rdatalen);
	if (ok)	data->assign(rdata, rdata+rdatalen);
	free(rdata);
	return ok;
}


bool serialCOM::Read(unsigned int minbytes, unsigned int maxbytes, unsigned int timeout, char *data, unsigned int *nrofbytesread)
{
	*nrofbytesread = 0;
	if (!portisopen || (minbytes > maxbytes) || (maxbytes > INT_MAX)) // NOTE: real limit: MAXDWORD
		return false;
	bool confirmRF = false;
	DWORD nbr = 0;
	unsigned int rb_total = 0;
	COMMTIMEOUTS timeouts;

	*nrofbytesread = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	// --- READ MINIMUM NUMBER OF REQUESTED BYTES ---
	if (minbytes)
	{
		if (!read_timeout_set || !last_read_timeout || (timeout != last_read_timeout))
		{
			// Enable timeout for reading:
			timeouts.ReadIntervalTimeout = 0;
			timeouts.ReadTotalTimeoutMultiplier = 0;
			if (timeout)
				timeouts.ReadTotalTimeoutConstant = timeout;
			else  // wait indefinitely
				timeouts.ReadTotalTimeoutConstant = MAXDWORD;
			if (!SetCommTimeouts(hCom, &timeouts))
			{
#ifdef __SERIALCOM_DEBUG__
				std::cout << "serialCOM::Read():   SetCommTimeouts(...) failed with error " << GetLastError() << "\n";
#endif
				return false;
			}
			last_read_timeout = timeouts.ReadTotalTimeoutConstant;
			read_timeout_set = true;
		}
		// READ DATA:
		confirmRF = ReadFile (hCom,		// Port handle
					  data,		// Pointer to data to read
					  minbytes,		// Max. number of bytes to read
					  &nbr,		// Pointer to number of bytes read
					  NULL		// Pointer to an OVERLAPPED structure; Must be NULL if not supported
					 );
		/* NOTE: - on timeout, ReadFile() succeeds with less bytes than requested (no error)
				 - ReadFile() always waits the full timeout to get the number of bytes requested	*/
		if (confirmRF)
		{
			rb_total += nbr;
#ifdef __SERIALCOM_DEBUG__
			if (rb_total < minbytes)
				std::cout << "serialCOM::Read():   TIMEOUT\n";
#endif
		}
		else
		{
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::Read():   ReadFile(...) failed with error " << GetLastError() << "\n";
#endif
			return false;
		}
	}
	// --- READ REMAINING DATA ---
	if ((maxbytes - rb_total > 0) && (rb_total >= minbytes))
	{
		if (minbytes || read_timeout_set)
		{
			// Disable timeout for reading (let ReadFile return immediately):
			timeouts.ReadIntervalTimeout = MAXDWORD;	// Max. time between two arriving characters
			timeouts.ReadTotalTimeoutMultiplier = 0;	// Multiplied with nr. of characters to read
			timeouts.ReadTotalTimeoutConstant = 0;		// Total max. time for read operations = TotalTimeoutMultiplier * nrOfBytes + TimeoutConstant
			if (!SetCommTimeouts(hCom, &timeouts))
			{
#ifdef __SERIALCOM_DEBUG__
				std::cout << "serialCOM::Read():   SetCommTimeouts(...) failed with error " << GetLastError() << "\n";
#endif
				return false;
			}
			read_timeout_set = false;
		}
		// READ REMAINING DATA:
		confirmRF = ReadFile (hCom,			// Port handle
					  data + rb_total,		// Pointer to data to read
					  maxbytes - rb_total,	// Max. number of bytes to read
					  &nbr,			// Pointer to number of bytes read
					  NULL			// Pointer to an OVERLAPPED structure; Must be NULL if not supported
					 );
		// NOTE: ReadFile() returns success even if less than the number of requested bytes is read
		if (confirmRF)
			rb_total += nbr;
		else
		{
#ifdef __SERIALCOM_DEBUG__
			std::cout << "serialCOM::Read():   ReadFile(...) failed with error " << GetLastError() << "\n";
#endif
			return false;
		}
	}
	// Returned data:
	*nrofbytesread = static_cast<unsigned int>(rb_total);
	return true;
	/* NOTE: - we always return the received bytes even if we received less than minbytes (timeout)
			 - return value indicates error but not a timeout (can be checked by comparing minbytes and nrofbytesread) */
}


bool serialCOM::ClearSendBuffer()
{
	bool confirmPC = false;
	if (!portisopen) return false;
	// confirmPC = PurgeComm(hCom, PURGE_TXABORT);   // terminates all outstanding overlapped write operations and returns immediately, even if the write operations have not been completed
	confirmPC = PurgeComm(hCom, PURGE_TXCLEAR);   // clears output buffer (if the device driver has one)
#ifdef __SERIALCOM_DEBUG__
	if (!confirmPC)
		std::cout << "serialCOM::ClearSendBuffer():   PurgeComm(...) failed with error " << GetLastError() << "\n";
#endif
	return confirmPC;
}


bool serialCOM::ClearReceiveBuffer()
{
	bool confirmPC = false;
	if (!portisopen) return false;
	// confirmPC = PurgeComm(hCom, PURGE_RXABORT);   // terminates all outstanding overlapped read operations and returns immediately, even if the read operations have not been completed
	confirmPC = PurgeComm(hCom, PURGE_RXCLEAR);   // clears input buffer (if the device driver has one)
#ifdef __SERIALCOM_DEBUG__
	if (!confirmPC)
		std::cout << "serialCOM::ClearReceiveBuffer():   PurgeComm(...) failed with error " << GetLastError() << "\n";
#endif
	return confirmPC;
}


bool serialCOM::SendBreak(unsigned int duration_ms)
{
	bool cvSCB=false, cvCCB=false;
	if ((!portisopen) || (duration_ms < 1) || (duration_ms >= 32767))
		return false;
	cvSCB = SetCommBreak(hCom);
	if (cvSCB)
		breakset = true;
#ifdef __SERIALCOM_DEBUG__
	else
		std::cout << "serialCOM::SendBreak():   SetCommBreak(...) failed with error " << GetLastError() << "\n";
#endif
	Sleep((DWORD)duration_ms);	// TO BE REPLACED BY A HP-TIMER ! (very imprecise !)
	cvCCB = ClearCommBreak(hCom);
	if (cvCCB)
		breakset = false;
#ifdef __SERIALCOM_DEBUG__
	else
		std::cout << "serialCOM::SendBreak():   ClearCommBreak(...) failed with error " << GetLastError() << "\n";
#endif
	return (cvSCB && cvCCB);
}


bool serialCOM::SetBreak()
{
	bool cvSCB = false;
	if (!portisopen) return false;
	cvSCB = SetCommBreak(hCom);
	breakset = cvSCB;
	if (cvSCB==true)
		return true;
	else
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::SetBreak():   SetCommBreak(...) failed with error " << GetLastError() << "\n";
#endif
		return false;
	}
}


bool serialCOM::ClearBreak()
{
	bool cvCCB = false;
	if (!portisopen) return false;
	cvCCB = ClearCommBreak(hCom);
	if (cvCCB==true)
	{
		breakset = false;
		return true;
	}
	else
	{
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::ClearBreak():   ClearCommBreak(...) failed with error " << GetLastError() << "\n";
#endif
		return false;
	}
}


bool serialCOM::BreakIsSet()
{
	return breakset;
}


bool serialCOM::GetNrOfBytesAvailable(unsigned int *nbytes)
{
	COMSTAT comstat;
	memset(&comstat, 0, sizeof(COMSTAT));
	DWORD Errors = 0;
	if (!portisopen) return false;
	if (ClearCommError(hCom, &Errors, &comstat) != 0)
	{
		*nbytes = static_cast<unsigned int>(comstat.cbInQue);
		return true;
	}
	else
	{
		*nbytes = 0;
		return false;
	}
}


bool serialCOM::SetControlLines(bool DTR, bool RTS)
{
	bool ok = false;
	if (!portisopen) return false;
	if (DTR)
		ok = EscapeCommFunction(hCom, SETDTR);	// "Ready"
	else
		ok = EscapeCommFunction(hCom, CLRDTR);	// "NOT Ready"
	if (RTS)
		ok = EscapeCommFunction(hCom, SETRTS);	// "Request"
	else
		ok = EscapeCommFunction(hCom, CLRRTS);	// "NO Request"
	/* NOTE: lines are inverted. Set flag means line=0/low/"space" */
	if (ok)
	{
		DTRset = DTR;
		RTSset = RTS;
	}
#ifdef __SERIALCOM_DEBUG__
	else
		std::cout << "serialCOM::SetControlLines():   EscapeCommFunction(...) failed with error " << GetLastError() << "\n";
#endif
	return ok;
}

// PRIVATE

bool serialCOM::GetMaxbaudrate(double *maxbaudrate)
{
	bool confirmGCP = false, baudbasedet=true;
	COMMPROP portproperties;
	memset(&portproperties, 0, sizeof(COMMPROP));
	*maxbaudrate = 0;
	confirmGCP = GetCommProperties(hCom, &portproperties);
	if (confirmGCP == false)
		return false;
	if (portproperties.dwSettableBaud > 0x7ffff) // NOTE: 0x7ffff = all values (except BOTHER) ORed
	{
		// unable to determine baudbase (seems to be larger than 115200)
		baudbasedet = false;
	}
	else if ((portproperties.dwSettableBaud | BAUD_128K) == portproperties.dwSettableBaud) // BAUD_128K = 0x10000
	{
		*maxbaudrate = 128000;	// ???
#ifdef __SERIALCOM_DEBUG__
		std::cout << "serialCOM::GetMaxbaudrate:   WARNING: unusual baudbase of 128kbps detected\n";
#endif
	}
	else if ((portproperties.dwSettableBaud | BAUD_115200) == portproperties.dwSettableBaud) // BAUD_115200 = 0x20000
	{
		*maxbaudrate = 115200;	// UART 16550(A)
	}
	else if ((portproperties.dwSettableBaud | BAUD_57600) == portproperties.dwSettableBaud) // BAUD_57600 = 0x40000
	{
		*maxbaudrate = 57600;
	}
	/*else if ((portproperties.dwSettableBaud | BAUD_56K) == portproperties.dwSettableBaud) // BAUD_56K = 0x8000
	{
		*maxbaudrate = 56000;	//  in reality, it's 57600 too !
	}*/
	else if ((portproperties.dwSettableBaud | BAUD_38400) == portproperties.dwSettableBaud) // BAUD_38400 = 0x4000
	{
		*maxbaudrate = 38400;	// UART 16540
	}
	else if ((portproperties.dwSettableBaud | BAUD_19200) == portproperties.dwSettableBaud) // BAUD_19200 = 0x2000
	{
		*maxbaudrate = 19200;
	}
	else if ((portproperties.dwSettableBaud | BAUD_14400) == portproperties.dwSettableBaud) // BAUD_14400 = 0x1000
	{
		*maxbaudrate = 14400;
	}
	else if ((portproperties.dwSettableBaud | BAUD_9600) == portproperties.dwSettableBaud) // BAUD_9600 = 0x800
	{
		*maxbaudrate = 9600;		// UART 8250
	}
	// NOTE: there are even smaller values defined, but even the UART 8250 supports 9600 baud...
	else
	{
		// unable to determine baudbase
		baudbasedet = false;
	}
	return baudbasedet;
	/*NOTE: dwMaxBaud should contain the maximum baudrate only (no bit-field),
	 * but it seems to be set to BAUD_USER always, if the device supports non-standrad-baudrates !
	 * For example: UART16550A (max. baudrate 115200), generic driver:
	 * dwMaxBaud is BAUD_USER instead of BAUD_115200
	 * => we have to use the dwSettableBaud bit-field
	 */
}


