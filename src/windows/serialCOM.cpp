/*
 * serialCOM.cpp - Serial port configuration and communication
 *
 * Copyright � 2008-2009 Comer352l
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
	breakset=false;
	currentportname = "";
	memset(&olddcb, 0, sizeof(DCB));
	olddcb.DCBlength = sizeof(DCB);
	settingssaved = false;
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
	// OPEN REGISTRY-KEY AND BROWSE ENTRYS:
	cv = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey);
	if (cv == ERROR_SUCCESS) // ERROR_SUCCESS is of type long
	{
		while ((RegEnumValueA(hKey, index, ValueName, &szValueName, NULL, NULL,Data,&szData)) == ERROR_SUCCESS)
		{
			if (!strncmp((char*)Data,"COM",3))	// compares the first 3 characters
				portlist.push_back((char*)Data);
			szValueName = 256;		// because RegEnumValue has changed value 
			szData = 256;			// because RegEnumValue has changed value 
			index++;
		}
		std::sort(portlist.begin(), portlist.end());	// quicksort from <algorithm> (automaticly included, if <vector> is included)
		cv = RegCloseKey(hKey);
		if (cv != ERROR_SUCCESS)
			std::cout << "serialCOM::GetAvailablePorts():   RegCloseKey(...) failed with error " << cv << "\n";
	}
	else
		std::cout << "serialCOM::GetAvailablePorts():   RegOpenKeyEx(...) failed with error " << cv << "\n";
	// RETURN VALUE:
	return portlist;
}


bool serialCOM::IsOpen()
{
	if ((portisopen==true) & (hCom != INVALID_HANDLE_VALUE) & (hCom != NULL))
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
	bool confirmGCS=false, cvGMbr = false;
	bool settingsvalid = true;
	double maxbaudrate = 0;
	unsigned long int divisor = 0;
	DCB currentdcb;
	memset(&currentdcb, 0, sizeof(DCB));
	currentdcb.DCBlength = sizeof(DCB);
	// RESET DATA:
	currentportsettings->baudrate = 0;
	currentportsettings->databits = 0;
	currentportsettings->parity = 0;
	currentportsettings->stopbits = 0;
	if (portisopen == false) return false;
	// REQUEST PORT SETTINGS FROM SYSTEM:
	confirmGCS = GetCommState(hCom, &currentdcb);
	if (confirmGCS==true)
	{
		// BAUDRATE SETTINGS:
		if (currentdcb.BaudRate != 0)
		{
			cvGMbr = serialCOM::GetMaxbaudrate(&maxbaudrate);
			if (cvGMbr == true)
			{
				divisor = static_cast<unsigned long int>(round(maxbaudrate / currentdcb.BaudRate));
				currentportsettings->baudrate = maxbaudrate / divisor;
			}
			else
			{
				currentportsettings->baudrate = currentdcb.BaudRate;
				std::cout << "serialCOM::SetPortSettings:   unable to detect baudbase\n";
				std::cout << "                              WARNING: reported baud rate may differ from real baudrate !\n";
			}
		}
		else
		{
			settingsvalid = false;
			std::cout << "serialCOM::GetPortSettings():   System reports baudrate=0\n";
		}
		// DATABITS SETTINGS:
		currentportsettings->databits = currentdcb.ByteSize;
		// PARITY SETTINGS:
		if (currentdcb.Parity == NOPARITY)
		{
			currentportsettings->parity = 'N';             // no parity bit
		}
		else if (currentdcb.Parity == ODDPARITY)
		{
			currentportsettings->parity = 'O';             // Odd
		}
		else if (currentdcb.Parity == EVENPARITY)
		{
			currentportsettings->parity = 'E';             // Even
		}
		else if (currentdcb.Parity == MARKPARITY)
		{
			currentportsettings->parity = 'M';             // Mark
		}
		else if (currentdcb.Parity == SPACEPARITY)
		{
			currentportsettings->parity = 'S';             // Space
		}
		else
		{
			settingsvalid = false;
			std::cout << "serialCOM::GetPortSettings():   unknown parity settings\n";
		}
		// STOPBITS SETTINGS:
		if (currentdcb.StopBits == ONESTOPBIT)
		{
			currentportsettings->stopbits = 1;
		}
		else if (currentdcb.StopBits == ONE5STOPBITS)
		{
			currentportsettings->stopbits = 1.5;
		}
		else if (currentdcb.StopBits == TWOSTOPBITS)
		{
			currentportsettings->stopbits = 2;
		}
		else
		{
			settingsvalid = false;
			std::cout << "serialCOM::GetPortSettings():   unknown stopbits settings\n";
		}
	}
	else
	{
		settingsvalid = false;
		std::cout << "serialCOM::GetPortSettings():   GetCommState(...) failed\n";
	}
	// RETURN SUCCESS:
	if ((confirmGCS == false) | (settingsvalid == false))
		return false;
	else
		return true;
}


bool serialCOM::SetPortSettings(serialCOM::dt_portsettings newportsettings)
{
	bool confirmGCS=false, confirmSCS=false, nsvalid=true, cvGMbr = false;
	double maxbaudrate = 0, exactbaudrate = 0;
	unsigned int bauddivisor = 0;
	DCB newdcb;
	memset(&newdcb, 0, sizeof(DCB));
	newdcb.DCBlength = sizeof(DCB);
	if (portisopen == false) return false;
	confirmGCS = GetCommState(hCom, &newdcb);
	if (confirmGCS=true)
	{
		// SET NEW PORT SETTINGS (not all will be changed):
		// BAUDRATE:
		if (!(newportsettings.baudrate > 0))
		{
			nsvalid = false;
			if (newportsettings.baudrate == 0)
				std::cout << "serialCOM::SetPortSettings:   illegal baudrate - 0 baud not possible\n";
			else
				std::cout << "serialCOM::SetPortSettings:   illegal baudrate - baud must be > 0\n";
		}
		else if (newportsettings.baudrate > 115200)
		{
			nsvalid = false;
			std::cout << "serialCOM::SetPortSettings:   illegal baudrate - baudrates above 115200 are currently not supported\n";
		}
		else
		{
			cvGMbr = serialCOM::GetMaxbaudrate(&maxbaudrate);	// get max. available baudrate
			if (cvGMbr == false)
			{
			newdcb.BaudRate = static_cast<DWORD>(round(newportsettings.baudrate));	// set baud rate directly
			std::cout << "serialCOM::SetPortSettings:   unable to detect baudbase\n";
			std::cout << "                              WARNING: reported baud rate may differ from real baudrate !\n";
			}
			else
			{
				if (newportsettings.baudrate > maxbaudrate)
				{
				nsvalid = false;
				std::cout << "serialCOM::SetPortSettings:   baudrate exceeds capabilities of interface/driver\n";
				}
				else
				{
					// Calculate "exact" baudrate:
					bauddivisor = static_cast<unsigned int>(round(maxbaudrate / newportsettings.baudrate));
					if (bauddivisor < 1) bauddivisor = 1;	// zur Sicherheit, ist eigentlich schon ausgeschlossen !
					if (bauddivisor > 65535) bauddivisor = 65535;
					exactbaudrate = (maxbaudrate / bauddivisor);	// Datentyp DWORD schraenkt moegliche extrem niedrige Baudraten ein !
					/* NOTE: DO NOT ROUND HERE !
					 * setCommState() doesn't round, it simply cuts the decimals => we get problems at e.g. 10400 baud !
					 */
					newdcb.BaudRate = static_cast<DWORD>(exactbaudrate);	// standard baudrates: CBR_9600, CBR_4800, ...
				}
			}
		}
		// DATABITS SETTINGS:
		if ((newportsettings.databits >= 5) & (newportsettings.databits <= 8))
			newdcb.ByteSize = newportsettings.databits;
		else 
		{
			nsvalid=false;
			std::cout << "serialCOM::SetPortSettings():   invalid value for parameter 'databits'\n";
		}
		// PARITY SETTINGS:
		if (newportsettings.parity == 'N')
		{
			newdcb.Parity = NOPARITY;                // no parity bit
		}
		else if (newportsettings.parity == 'O')
		{
			newdcb.Parity = ODDPARITY;               // Odd
		}
		else if (newportsettings.parity == 'E')
		{
			newdcb.Parity = EVENPARITY;              // Even
		}
		else if (newportsettings.parity == 'M')
		{
			newdcb.Parity = MARKPARITY;              // Mark
		}
		else if (newportsettings.parity == 'S')
		{
			newdcb.Parity = SPACEPARITY;             // Space
		}
		else
		{
			nsvalid=false;
			std::cout << "serialCOM::SetPortSettings():   invalid value for parameter 'parity'\n";
		}
		// STOPBIT SETTINGS:      (=> The use of 5 data bits with 2 stop bits is an invalid combination, as is 6, 7, or 8 data bits with 1.5 stop bits)
		if (newportsettings.stopbits == 1)
		{
			newdcb.StopBits = ONESTOPBIT;		// 1 stop bit
		}
		else if (newportsettings.stopbits == 1.5)
		{
			if (newportsettings.databits == 5)
				newdcb.StopBits = ONE5STOPBITS;	// 1.5 stop bits
			else
			{
				nsvalid=false;
				std::cout << "serialCOM::SetPortSettings():   invalid value for parameter 'stopbits': 1.5 stopbits only allowed in combination with 5 databits\n";
			}
		}
		else if (newportsettings.stopbits == 2)
		{
			if (!(newportsettings.databits == 5))
				newdcb.StopBits = TWOSTOPBITS;	// 2 stop bits
			else
			{
				nsvalid=false;
				std::cout << "serialCOM::SetPortSettings():   invalid value for parameter 'stopbits': 2 stopbits not allowed in combination with 5 databits\n";
			}
		}
		else
		{
			nsvalid=false;
			std::cout << "serialCOM::SetPortSettings():   invalid value for parameter 'stopbits'\n";
		}
		// MORE OPTIONS:
		newdcb.fBinary = true;	// binary mode (false is not supported by Windows)
		newdcb.fParity = false;	// no parity checking
		newdcb.fNull = false;	// VERY IMPORTANT: don't discard recieved zero characters
		// FLOW CONTROL:
		newdcb.fOutxCtsFlow = false;	// CTS disabled
		newdcb.fOutxDsrFlow = false;	// DSR disabled
		newdcb.fDtrControl = DTR_CONTROL_DISABLE;	// DTR disabled
		newdcb.fRtsControl = RTS_CONTROL_DISABLE;	// RTS disabled
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
		if (nsvalid==true)
			confirmSCS = SetCommState(hCom, &newdcb);
		if (confirmSCS == false)
			std::cout << "serialCOM::SetPortSettings():   SetCommState(...) failed\n";
	}
	else
		std::cout << "serialCOM::SetPortSettings():   GetCommState(...) failed\n";
	// RETURN SUCCESS:
	if ((confirmGCS == true) & (confirmSCS == true))
		return true;
	else
		return false;
}


bool serialCOM::OpenPort(std::string portname)
{
	bool confirm;
	DWORD error;
	if (portisopen == true) return false;
	// OPEN PORT:
	hCom = CreateFileA(portname.c_str(),			// name of port
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
		error = GetLastError();
		std::cout << "serialCOM::OpenPort():   CreateFileA(...) failed with error " << error << "\n";
		return false;
	}
	else	// port successfully opened
	{
		portisopen = true;
		currentportname = portname;
		// SAVE PORT SETTINGS:
		memset(&olddcb, 0, sizeof(DCB));
		olddcb.DCBlength = sizeof(DCB);
		confirm = GetCommState(hCom, &olddcb);
		if (confirm == false)
		{
			error = GetLastError();
			std::cout << "serialCOM::OpenPort():   GetCommState(...) failed with error " << error << "\n";
		}
		settingssaved = confirm;
		// SET TIMEOUTS (=> Timeouts disabled; read operation immediatly returns recieved characters):
		COMMTIMEOUTS timeouts = {0};
		timeouts.ReadIntervalTimeout = MAXDWORD;	// Max. time between two arriving characters
		timeouts.ReadTotalTimeoutMultiplier = 0;	// Multiplied with nr. of characters to read 
		timeouts.ReadTotalTimeoutConstant = 0;		// Total max. time for read operations = TotalTimeoutMultiplier * nrOfBytes + TimeoutConstant
		timeouts.WriteTotalTimeoutConstant = 0;		// Total max. time for write operations = TotalTimeoutMultiplier * nrOfBytes + TimeoutConstant
		timeouts.WriteTotalTimeoutMultiplier = 0;	// Multiplied with nr. of characters to write 
		// all values in [ms]; if one of these max. times is exceeded, Read / Write Operation is stoped and nr. of Read/WrittenBytes is given back
		confirm = SetCommTimeouts(hCom, &timeouts);	// Apply new timeout settings
		if (confirm == false)
		{
			error = GetLastError();
			std::cout << "serialCOM::OpenPort():   SetCommTimeouts(...) failed with error " << error << "\n";
			if (!ClosePort())
				std::cout << "serialCOM::OpenPort():   port couldn't be closed after error during opening process\n";
			return false;
		}
/*
		// CLEAR BREAK (should not be necessary, because break is cancelled automatically after closing the device):
		confirm = ClearCommBreak(hCom);
		if (!confirm)
		{
			error = GetLastError();
			std::cout << "serialCOM::OpenPort():   ClearCommBreak(...) failed with error " << error << "\n";	// debug-output
		}
		else
*/
		breakset = false;
		// CLEAR HARDWARE BUFFERS:
		confirm = PurgeComm(hCom, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR);   // clears output buffer (if the device driver has one)
		if (!confirm)
		{
			error = GetLastError();
			std::cout << "serialCOM::OpenPort():   PurgeComm(...) failed with error " << error << "\n";
			if (!ClosePort())
				std::cout << "serialCOM::OpenPort():   port couldn't be closed after error during opening process\n";
			return false;
		}
		return true;
	}
}


bool serialCOM::ClosePort()
{
	bool confirm=false;
	DWORD error;
	if (portisopen == false) return false;
	// CLEAR BREAK:
	confirm = ClearCommBreak(hCom);
	if (confirm)
		breakset=false;
	else
	{
		error = GetLastError();
		std::cout << "serialCOM::ClosePort():   ClearCommBreak(...) failed with error " << error << "\n";
	}
	// CLEAR HARDWARE BUFFERS:
	confirm = PurgeComm(hCom, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR);   // clears output buffer (if the device driver has one)
	if (!confirm)
	{
		error = GetLastError();
		std::cout << "serialCOM::ClosePort():   PurgeComm(...) failed with error " << error << "\n";
	}
	// RESTORE OLD PORT SETTINGS:
	confirm = SetCommState(hCom, &olddcb);
	if (confirm==false)
	{
		error = GetLastError();
		std::cout << "serialCOM::ClosePort():   SetCommState(...) failed with error " << error << "\n";
	}
	// CLOSE PORT:
	confirm = CloseHandle(hCom);
	if (confirm==false)
	{
		error = GetLastError();
		std::cout << "serialCOM::ClosePort():   CloseHandle(...) failed with error " << error << "\n";
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


bool serialCOM::Write(char *outputstr, unsigned int nrofbytestowrite)
{
	bool confirmWF=false, confirmCCB=false;
	DWORD BytesWritten=0;
	if (portisopen == false) return false;
	if (breakset)
	{
		confirmCCB = ClearCommBreak(hCom);
		if (confirmCCB)
			breakset = false;
		else
		{
			std::cout << "serialCOM::Write():   ClearCommBreak(...) failed\n";
			return false;
		}
	}
	// SEND DATA:
	confirmWF = WriteFile (hCom,			// Port handle
			       outputstr,		// Pointer to the data to write 
			       nrofbytestowrite,	// Number of bytes to write
			       &BytesWritten,		// Pointer to the number of bytes written
			       NULL			// Pointer to an OVERLAPPED structure; Must be NULL if not supported
			      );
	// RETURN VALUE:
	if ((confirmWF == true) & (BytesWritten == static_cast<DWORD>(nrofbytestowrite)))
		return true;
	else
	{
		if (confirmWF == false)
			std::cout << "serialCOM::Write():   WriteFile(...) failed\n";
		if (BytesWritten != static_cast<DWORD>(nrofbytestowrite))
			std::cout << "serialCOM::Write():   some bytes could not be written\n";
		return false;
	}
}


bool serialCOM::Read(char *readdata, unsigned int *nrofbytesread)
{
	bool confirmRF = false;
	DWORD nbr=0;
	*nrofbytesread = 0;
	if (portisopen == false) return false;
	// READ RECIEVED DATA:
	confirmRF = ReadFile (hCom,		// Port handle
			      readdata,		// Pointer to data to read
			      512,		// Number of bytes to read
			      &nbr,		// Pointer to number of bytes read
			      NULL		// Pointer to an OVERLAPPED structure; Must be NULL if not supported
			     );
	if (confirmRF == false)
		std::cout << "serialCOM::Read():   ReadFile(...) failed\n";
	else
		*nrofbytesread = static_cast<unsigned int>(nbr);
	// RETURN SUCCESS:
	return confirmRF;
}


bool serialCOM::ClearSendBuffer()
{
	bool confirmPC = false;
	DWORD error = 0;
	if (portisopen == false) return false;
	// confirmPC = PurgeComm(hCom, PURGE_TXABORT);   // terminates all outstanding overlapped write operations and returns immediately, even if the write operations have not been completed
	confirmPC = PurgeComm(hCom, PURGE_TXCLEAR);   // clears output buffer (if the device driver has one)
	if (!confirmPC)
	{
		error = GetLastError();
		std::cout << "serialCOM::ClearSendBuffer():   PurgeComm(...) failed with error " << error << "\n";
	}
	return confirmPC;
}


bool serialCOM::ClearRecieveBuffer()
{
	bool confirmPC = false;
	DWORD error = 0;
	if (portisopen == false) return false;
	// confirmPC = PurgeComm(hCom, PURGE_RXABORT);   // terminates all outstanding overlapped read operations and returns immediately, even if the read operations have not been completed
	confirmPC = PurgeComm(hCom, PURGE_RXCLEAR);   // clears input buffer (if the device driver has one)
	if (!confirmPC)
	{
		error = GetLastError();
		std::cout << "serialCOM::ClearRecieveBuffer():   PurgeComm(...) failed with error " << error << "\n";
	}
	return confirmPC;
}


bool serialCOM::SendBreak(unsigned int duration_ms)
{
	bool cvSCB=false, cvCCB=false;
	DWORD error = 0;
	if ((portisopen == false) | (duration_ms < 1) | (duration_ms >= 32767))
		return false;
	cvSCB = SetCommBreak(hCom);
	if (!cvSCB)
	{
		error = GetLastError();
		std::cout << "serialCOM::SendBreak():   SetCommBreak(...) failed with error " << error << "\n";
	}
	else
		breakset = true;
	Sleep((DWORD)duration_ms);	// TO BE REPLACED BY A HP-TIMER ! (very imprecise !)
	cvCCB = ClearCommBreak(hCom);
	if (!cvCCB)
	{
		error = GetLastError();
		std::cout << "serialCOM::SendBreak():   ClearCommBreak(...) failed with error " << error << "\n";
	}
	else
		breakset = false;
	if ((cvSCB==true) & (cvCCB==true))
		return true;
	else
		return false;
}


bool serialCOM::SetBreak()
{
	bool cvSCB=false;
	DWORD error = 0;
	if (portisopen == false) return false;
	cvSCB = SetCommBreak(hCom);
	breakset = cvSCB;
	if (cvSCB==true)
		return true;
	else
	{
		error = GetLastError();
		std::cout << "serialCOM::SetBreak():   SetCommBreak(...) failed with error " << error << "\n";
		return false;
	}
}


bool serialCOM::ClearBreak()
{
	bool cvCCB=false;
	DWORD error = 0;
	if (portisopen == false) return false;
	cvCCB = ClearCommBreak(hCom);
	if (cvCCB==true)
	{
		breakset = false;
		return true;
	}
	else
	{
		error = GetLastError();
		std::cout << "serialCOM::ClearBreak():   ClearCommBreak(...) failed with error " << error << "\n";
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
	if (portisopen == false) return false;
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


serialCOM::~serialCOM()
{
	if (portisopen) ClosePort();
}



// PRIVATE
bool serialCOM::GetMaxbaudrate(double *maxbaudrate)
{
	bool confirmGCP = false, baudbasedet=true;
	COMMPROP portproperties;
	memset(&portproperties, 0, sizeof(COMMPROP));
	*maxbaudrate = 0;
	confirmGCP = GetCommProperties(hCom, &portproperties);
	if (confirmGCP == false) return false;
	if ((portproperties.dwSettableBaud & ~0x10000000) > 0x7ffff)
	{
		// unable to determine baudbase (seems to be larger than 115200)
		baudbasedet = false;
	}
	else if ((portproperties.dwSettableBaud & ~0x10000) != portproperties.dwSettableBaud)
	{
		*maxbaudrate = 128000;	// ???
		std::cout << "serialCOM::GetMaxbaudrate:   WARNING: unusual baudbase of 128kbps detected\n";
	}
	else if ((portproperties.dwSettableBaud & ~0x20000) != portproperties.dwSettableBaud)
	{
		*maxbaudrate = 115200;	// UART 16550(A)
	}
	else if ((portproperties.dwSettableBaud & ~0x40000) != portproperties.dwSettableBaud)
	{
		*maxbaudrate = 57600;
	}
	/*else if ((portproperties.dwSettableBaud & ~0x8000) != portproperties.dwSettableBaud)
	{
		*maxbaudrate = 56000;	//  in reality, it's 57600 too !
	}*/
	else if ((portproperties.dwSettableBaud & ~0x4000) != portproperties.dwSettableBaud)
	{
		*maxbaudrate = 38400;	// UART 16540
	}
	else if ((portproperties.dwSettableBaud & ~0x2000) != portproperties.dwSettableBaud)
	{
		*maxbaudrate = 19200;
	}
	else if ((portproperties.dwSettableBaud & ~0x1000) != portproperties.dwSettableBaud)
	{
		*maxbaudrate = 14400;
	}
	else if ((portproperties.dwSettableBaud & ~0x800) != portproperties.dwSettableBaud)
	{
		*maxbaudrate = 9600;		// UART 8250
	}
	else
	{
		// unable to determine baudbase
		baudbasedet = false;
	}
	return baudbasedet;
	/*NOTE: instead of checking dwSettableBaud we could also use dwMaxBaud, which should (!) contain
	 * the max. baudrate as a single bit set.
	 */
}

