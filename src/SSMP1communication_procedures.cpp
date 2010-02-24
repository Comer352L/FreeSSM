/*
 * SSMP1communication_procedures.cpp - Communication procedures for the SSM1-protocol
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

#include "SSMP1communication_procedures.h"

#ifdef __WIN32__
    #define waitms(x) Sleep(x)
#elif defined __linux__
    #define waitms(x) usleep(1000*x)
#else
    #error "Operating system not supported !"
#endif


SSMP1communication_procedures::SSMP1communication_procedures(AbstractDiagInterface *diagInterface) : SSMP1commands(diagInterface)
{
	_currentaddr = -1;
	_lastaddr = -1;
	_addrswitch_pending = false;
	_sync = false;
}


bool SSMP1communication_procedures::setAddress(SSM1_CUtype_dt cu, unsigned int addr)
{
	if (((cu != SSM1_CU_Engine) && (cu != SSM1_CU_Transmission)) && (addr > 0xFF))
		return false;
	if (!sendReadAddressCmd(cu, addr))
		return false;
	waitms(SSMP1_T_IC_WAIT);
	_recbuffer.clear();
	_lastaddr = _currentaddr;
	_currentaddr = addr;
	_addrswitch_pending = true;
	_sync = false;
	return true;
}


bool SSMP1communication_procedures::getID(std::vector<char> * data)
{
	if (!sendQueryIdCmd()) return false;
	waitms(SSMP1_T_IC_WAIT);
	_recbuffer.clear();
	_currentaddr = -1;
	_lastaddr = -1;
	_addrswitch_pending = false;
	if (!sendQueryIdCmd()) return false;
	waitms(SSMP1_T_NEWDATA_REC_MAX);
	// Read all data from port and return the last 3 bytes
	if (_diagInterface->read(data) && (data->size() > 2))
	{
		data->erase(data->begin(), data->end()-3);
		return true;
	}
	return false;
}


bool SSMP1communication_procedures::getNextData(std::vector<char> * data, unsigned int timeout)
{
	if (_currentaddr < 0) return false;
	TimeM time;
	char hb = (_currentaddr & 0xffff) >> 8;
	char lb = _currentaddr & 0xff;
	std::vector<char> rbuf;
	bool ok = false;
	time.start();
	while (static_cast<unsigned int>(time.elapsed()) < timeout)
	{
		// Read out port buffer:
		do
		{
			ok = _diagInterface->read(&rbuf);
			if (ok && rbuf.size())
				_recbuffer.insert(_recbuffer.end(), rbuf.begin(), rbuf.end());
		} while (ok && rbuf.size());
#ifdef __FSSM_DEBUG__
		if (!ok)
			std::cout << "SSMP1communication_procedures::getNextData():   communication error\n";
#endif
		// Try to find/get dataset:
		if (ok && (_recbuffer.size() > 2))
		{
			// Synchronize with received data (if necessary):
			if (!_sync)
				syncToRecData();
			// Extract data from latest received dataset:
			if (_sync && (_recbuffer.size() > 2))
			{
				unsigned char olBytes = _recbuffer.size() % 3;
				unsigned int msgStartIndex = _recbuffer.size() - 3 - olBytes;	
				if ((_recbuffer.at(msgStartIndex) == hb) && (_recbuffer.at(msgStartIndex+1) == lb))
				{
					/* NOTE: There could have been an overflow of the drivers receive-buffer.
						 If this is the case and we nevertheless reached this point
						 => hb=lb
						 => we could extract old (but correct) data
					 */
					// Extra-check to avoid extracting old data (if a buffer-overflow occured):
					if (hb == _recbuffer.at(msgStartIndex+2)) // && _recbuffer.size > XXX
					{
						// NOTE: We could extract old (but correct) data
						if (olBytes > 0)
						{
							if (_recbuffer.at(msgStartIndex+3) != hb)
							{
								_sync = false;
							}
							else if (olBytes > 1)
							{
								if (_recbuffer.at(msgStartIndex+4) == hb)
									_sync = false;
							}
						}
					}
					/* NOTE: at this point we may have lost snychronisation (due to a buffer overflow),
						 but we are at least sure that the data byte is correct !			*/
					// Extract data, clean up buffer:
					if (_sync)
					{
						data->push_back(_recbuffer.at(msgStartIndex+2));
						_recbuffer.erase(_recbuffer.begin(), _recbuffer.begin()+msgStartIndex+3);
						return true;
					}
				}
				else	// may happen, if we got an overflow of the drivers recieve-buffer
					_sync = false;
#ifdef __FSSM_DEBUG__
				if (!_sync)
					std::cout << "SSMP1communication_procedures::getNextData():   lost synchronisation\n";
#endif
			}
		}
		// Delay before next iteration:
		waitms(10);
	}
#ifdef __FSSM_DEBUG__
	std::cout << "SSMP1communication_procedures::getNextData():   timeout\n";
#endif
	return false;
}


bool SSMP1communication_procedures::writeDatabyte(char databyte)
{
	if (_currentaddr < 0) return false;
	return sendWriteDatabyteCmd(_currentaddr, databyte);
}


bool SSMP1communication_procedures::waitForDataValue(char data, unsigned int timeout)
{
	std::vector<char> datawritten;
	TimeM time;
	bool ok = false;
	time.start();
	do
	{
		if (!getNextData(&datawritten, timeout))
		{
#ifdef __FSSM_DEBUG__
			std::cout << "SSMP1communication_procedures::waitForWriteConfirmation()   getNextData() failed !\n";
#endif
			return false;
		}
		ok = (datawritten.at(0) == data);
		if (!ok)
			datawritten.clear();
	} while(!ok && (time.elapsed() < timeout));
#ifdef __FSSM_DEBUG__
	std::cout << "SSMP1communication_procedures::waitForWriteConfirmation():   the written data was not confirmed by the CU !\n";
#endif
	return ok;
}


bool SSMP1communication_procedures::stopCUtalking(bool waitforsilence)
{
	unsigned char norec_counter = 0;
	if (!sendStopTalkingCmd())
		return false;
	if (waitforsilence)
	{
		std::vector<char> buffer;
		TimeM time;
		time.start();
		do
		{
			waitms(10);
			if (_diagInterface->read(&buffer))
			{
				if (buffer.size())
					norec_counter = 0;
				else
					norec_counter++;
			}
		} while ((time.elapsed() < SSMP1_T_NEWDATA_REC_MAX) && (norec_counter < 5));
		/* TODO:
		 * - invastigate CU-behavior, define a T_MAX_UNTIL_SILENCE (for now, we simply use T_NEWDATA_REC_MAX)
		 * - does the CU always complete sending the current message ? (=> 256-byte-blockrates ~ 1.5s !)
		 *   => make sure it will work with block-reads (in the future)
		 */
	}
	if (!waitforsilence || (norec_counter >= 5))
	{
		_recbuffer.clear();
		_currentaddr = -1;
		_lastaddr = -1;
		_addrswitch_pending = false;
	}
	return true;
}

// PRIVATE:

void SSMP1communication_procedures::syncToRecData()
{
	char hb = (_currentaddr & 0xffff) >> 8;
	char lb = _currentaddr & 0xff;
	char hb_old = (_lastaddr & 0xffff) >> 8;
	char lb_old = _lastaddr & 0xff;
#ifdef __FSSM_DEBUG__
	std::string syncerrstr;

	std::cout << "SSMP1communication_procedures::syncToRecData():   synchronization with incoming data stream ";
#endif
	for (int k=_recbuffer.size()-3; k>=0; k--)
	{
		if ((_recbuffer.at(k) == hb) && (_recbuffer.at(k+1) == lb))
		{
			_sync = true;
			// Check address byte positions in the subsequent data:
			for (unsigned int m=k+3; m<(_recbuffer.size()-1); m+=3)
			{
				if ((_recbuffer.at(m) != hb) || (_recbuffer.at(m+1) != lb))
				{
					_sync = false;
#ifdef __FSSM_DEBUG__
					syncerrstr = "Address position check failed.";
#endif
					break;
				}
			}
			/* NOTE: In case of hb==lb we may still NOT be sync ! But
				 => we are at least sure that the data byte is correct !
				 => we can set _sync to true, because when the databyte changes to a value != hb,lb
				 we will get the wrong address and therefore out of sync again !
			 */
			// Special cases that are only relevant during address switching: 
			if (_addrswitch_pending && (_lastaddr > -1) && (_currentaddr != _lastaddr))	// max 2 complete old messages
			{
				/* NOTE: Terminology:
						A1		high address byte (old dataset)
						B1		high address byte (old dataset)
						D1		databyte (old dataset) => can be different in each message 
						A2		high address byte (new dataset)
						B2		high address byte (new dataset)
						D2		databyte (new message)
						"databyte"	the byte supposed to be D2		*/
				// 1-byte-overlapping:
				if (_sync && (k<6)) 	// max 2 complete old messages
				{
					// Check for possible issues when 1+x bytes message-overlapping:
					/* NOTE: Szenario: ... (D1 | A2 B2) D2 ...     with hb==D1 and lb==A2
						  => OK, if we have received at least two bytes from the old message and the previous byte is !=B1 */
					if (k>0 && _recbuffer.at(k-1) == lb_old)
						_sync = false;
					if (_sync && (k<5))	// 2-byte (or more) overlapping possible
					{
						// Check for possible issues when 2+x bytes message-overlapping:
						/* NOTE: Szenario: ... (B1 D1 | A2) B2 D2 ...     with hb==B1 and lb==D1
							  => OK, if databyte!=A2							*/
						if ((hb == lb_old) && (_recbuffer.at(k+2) == hb))
						{
							/* Try to exclude overlapping by checking previous data (if available):
								=> OK, if the previous byte has been received and is != A1
								=> OK, if the 3. previous byte has been received and is != B1
								=> OK, if the 4. previous byte has been received and is != A1		*/
							_sync = false;
							if ((k>0) && (_recbuffer.at(k-1) != hb_old))
								_sync = true;
							if ((k>2) && (_recbuffer.at(k-3) != lb_old))
								_sync = true;
							if ((k>3) && (_recbuffer.at(k-4) != hb_old))
								_sync = true;
#ifdef __FSSM_DEBUG__
							if (!_sync)
								syncerrstr = "Overlapping checks matched special case 1.";
#endif
						}
						// NOTE: 3-byte overlapping not possible (would mean old address == new address)
						if (_sync && (k<3))	// 4-byte (or more) overlapping possible
						{
							// Check for possible issues when 4+x bytes message-overlapping:
							/* NOTE: Szenario: ... (D1 | A1 B1) D1 | A2 B2 D2 ...     with hb==D1 and lb==A1
								  => OK, if databyte!=B1							*/
							if ((hb_old == lb) && (_recbuffer.at(k+2) == lb_old))
							{
								/* Try to exclude overlapping by checking previous data (if available):
									=> OK, if the previous byte has been received and is != B1
									=> OK, if the 2. previous byte has been receeved and is != A1		*/
								_sync = false;
								if ((k>0) && (_recbuffer.at(k-1) != lb_old))
									_sync = true;
								if ((k>1) && (_recbuffer.at(k-2) != hb_old))
									_sync = true;
#ifdef __FSSM_DEBUG__
								if (!_sync)
								{
									if (syncerrstr.size())
										syncerrstr += ", 2";
									else
										syncerrstr = "Overlapping checks matched special case 2.";
								}
#endif
							}
							if (_sync && (k<2))	// 5-byte (or more) overlapping possible
							{
								// Check for possible issues when 5+x bytes message-overlapping:
								/* NOTE: Szenario: ... (B1 D1 | A1) B1 D1 | A2 B2 D2 ...     with hb==B1 and D1==lb
									  => OK, if databyte!=A1							*/
								if ((lb_old == hb) && (_recbuffer.at(k+2) == hb_old))
								{
									/* Try to exclude overlapping by checking previous data (if available):
										=> OK, if a previous byte has been received and is != A1		*/
									if (k==0 || (_recbuffer.at(k-1) == hb_old))
									{
										_sync = false;
#ifdef __FSSM_DEBUG__
										if (syncerrstr.size())
											syncerrstr += ", 3";
										else
											syncerrstr = "Overlapping checks matched special case 3.";
#endif
									}
								}
							}
							// NOTE: 6-byte overlapping not possible (would mean old address == new address)
						}
					}
				}
				// NOTE: sync=false means, that we need more datasets to be sure to have found the right message start
			}
			// Delete buffer from the beginning up to (excluding) the first byte of the message:
			if (_sync)
			{
				_recbuffer.erase( _recbuffer.begin(), _recbuffer.begin() + k);
				_addrswitch_pending = false;
				break;
			}
#ifdef __FSSM_DEBUG__
			else
				syncerrstr += " Need to wait for further incoming datasets.";
#endif
		}
	}
#ifdef __FSSM_DEBUG__
	if (!_sync)
	{
		std::cout << "failed:\n";
		if (!syncerrstr.size())
			syncerrstr = "Address-header not detetced.";
		std::cout << "    => " << syncerrstr << '\n';
	}
	else
		std::cout << "succesful.\n";
#endif
}

