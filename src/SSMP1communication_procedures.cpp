/*
 * SSMP1communication_procedures.cpp - Communication procedures for the SSM1-protocol
 *
 * Copyright (C) 2009-2012 Comer352L
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


SSMP1communication_procedures::SSMP1communication_procedures(AbstractDiagInterface *diagInterface) : SSMP1commands(diagInterface)
{
	_currentaddr = -1;
	_lastaddr = -1;
	_addrswitch_pending = false;
	_sync = false;
}


bool SSMP1communication_procedures::setAddress(SSM1_CUtype_dt cu, unsigned int addr)
{
	if (!sendReadAddressCmd(cu, addr))
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP1communication_procedures::setAddress(...):   sendReadAddressCmd(...) failed.\n";
#endif
		return false;
	}
#ifdef __FSSM_DEBUG__
	else
		std::cout << "SSMP1communication_procedures::setAddress(...):   sent read-request for address " << std::hex << std::showbase << addr << '\n';
#endif
	waitms(SSMP1_T_IC_WAIT);
	_recbuffer.clear();
	_lastaddr = _currentaddr;
	_currentaddr = addr;
	_addrswitch_pending = true;
	_sync = false;
	return true;
}


bool SSMP1communication_procedures::getID(unsigned int addr, unsigned char extradatalen, std::vector<char> * data)
{
	const unsigned int t_max_total = (SSMP1_T_ID_RECSTART_MAX + 1000*(3+extradatalen)*8/1953) * 2;
	const unsigned char max_bytes_dropped = 15; // NOTE: depends on last request; min. 12 (seen with Ax10xx TCU) if last CU reply message length is 3 bytes
	std::vector<char> tmpbuf;
	unsigned char bytes_dropped = 0;
	bool IDvalid = false;
	bool IDconfirmed = false;
	unsigned int cu_data_len = 0;
	bool timeout;

	if (!sendQueryIdCmd(addr, 0))
		return false;
	waitms(SSMP1_T_IC_WAIT);
	_recbuffer.clear();
	_currentaddr = -1;
	_lastaddr = -1;
	_addrswitch_pending = false;
	if (!_diagInterface->clearReceiveBuffer())
		return false;
	if (!sendQueryIdCmd(addr, extradatalen))
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP1communication_procedures::getID(...):   sendQueryIdCmd() failed.\n";
#endif
		return false;
	}
	if (!_diagInterface->clearReceiveBuffer())
		return false;
	/* NOTE: Problem:
	 * - we can not rely in buffer flushing because of driver/OS/hardware latencies and bugs
	 * - control units need some time until they switch from sending data from the previous
	 *   (read address) request to sending the control unit ID (+data)
	 */
	TimeM time_total;
	TimeM time_inter_byte;
	time_total.start();
	time_inter_byte.start();
	do
	{
		waitms(20);
		// Read data, append to receive buffer
		tmpbuf.clear();
		if (!_diagInterface->read(&tmpbuf))
		{
			_recbuffer.clear();
			return false;
		}
		if (tmpbuf.size())
		{
			_recbuffer.insert(_recbuffer.end(), tmpbuf.begin(), tmpbuf.end());
			// Restart inter byte timer
			time_inter_byte.start();
			// Validate ID (the first 3 bytes)
			while (!IDvalid && _recbuffer.size() && (bytes_dropped < max_bytes_dropped))
			{
				bool drop_byte = false;
				// Check if we have a valid ID at the beginning of the buffer
				if ((_recbuffer.at(0) & 0xF0) == 0x70)
				{
					IDvalid = true;
				}
				else if ((_recbuffer.at(0) & 0xF0) == 0xA0)
				{
					if (_recbuffer.size() > 1)
					{
						if ((_recbuffer.at(1) == 0x01) || (_recbuffer.at(1) == 0x10))
							IDvalid = true;
						else
							drop_byte = true;
					}
					else
						break;
				}
				else
				{
					drop_byte = true;
				}
				// Drop first byte from receive buffer if it is not the beginning of a valid ID
				if (drop_byte)
				{
					_recbuffer.erase(_recbuffer.begin(), _recbuffer.begin() + 1);
					bytes_dropped++;
				}
			}
			// Check for repeating byte sequence(s)
			if (IDvalid && (_recbuffer.size() > 3))
			{
				unsigned int id_index = 0;
				for (unsigned int buf_index=3; buf_index<_recbuffer.size(); buf_index++)
				{
					// Compare next byte:
					if (_recbuffer.at(id_index) == _recbuffer.at(buf_index))
					{
						cu_data_len = buf_index - id_index;
						id_index++;
					}
					else
					{
						cu_data_len = buf_index + 1;
						buf_index -= id_index;
						id_index = 0;
					}
					// Check if we have reached an invalid ECU data length
					// FIXME: be more strict with CUs with Ax xx xx ID: compare cu_data_len with (3 + extradatalen)
					if ((cu_data_len > 3) && ((_recbuffer.at(0) & 0xF0) == 0x70))
					{
						// NOTE: ID must be invalid, 7x xx xx IDs are always 3 bytes long !
						IDvalid = false;
						// Drop first byte from receive buffer
						_recbuffer.erase(_recbuffer.begin(), _recbuffer.begin() + 1);
						bytes_dropped++;
						break;
					}
				}
				// Check if we have enough repetitions	// NOTE: we could do this earlier (inside the loop), but - hey - why not check ALL data we have received ?!
				if (id_index >= 3) /* NOTE: 3 bytes should be enough. The chance that we are comparing the wrong bytes and have two identical 3 byte sequences is 1/(256^3) */
					IDconfirmed = true;
			}
		}
		// Check if timeout
		timeout = (time_total.elapsed() > t_max_total) || (time_inter_byte.elapsed() > SSMP1_T_ID_RECSTART_MAX);
	} while (!IDconfirmed && !timeout && (bytes_dropped < max_bytes_dropped));
	// For control units with non-repeating data: validate ID by checking the length
	if (!IDconfirmed && IDvalid && timeout && (bytes_dropped < max_bytes_dropped) && (_recbuffer.size() >= 3))
	{
		if (((_recbuffer.at(0) & 0xF0) == 0x70) && (_recbuffer.size() == 3)) // NOTE: >3 not possible at this point
		{
			IDconfirmed = true;
			cu_data_len = 3;
		}
		else if ((_recbuffer.at(0) & 0xF0) == 0xA0)
		{
			if ((_recbuffer.at(1) == 0x01) && ((_recbuffer.size() == 3) || ((_recbuffer.size() > 3) && (_recbuffer.size() <= 3+255))))
			{
				/* FIXME: IMPROVE !
				 * how does the number of received bytes correspond to extradatalen ?
				 * Is it possible that controllers end more/less bytes than requested ? */
				IDconfirmed = true;
				cu_data_len = _recbuffer.size();
			}
			else if ((_recbuffer.at(1) == 0x10) && ((_recbuffer.size() == 3) || (_recbuffer.size() == (3 + static_cast<unsigned int>(extradatalen)))))
			{
				IDconfirmed = true;
				cu_data_len = _recbuffer.size();
			}
		}
#ifdef __FSSM_DEBUG__
		if (IDconfirmed)
			std::cout << "SSMP1communication_procedures::getID(...):   the control unit doesn't send its ID continuously, ID validated by length check.\n";
		else
			std::cout << "SSMP1communication_procedures::getID(...):   the control unit doesn't send its ID continuously, ID length check failed.\n";
#endif
	}
	// Extract data:
	if (IDconfirmed)
	{
		data->assign(_recbuffer.begin(), _recbuffer.begin() + cu_data_len);
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP1communication_procedures::getID(...):   received ID with length " << std::dec << cu_data_len << ": ";
		std::cout << libFSSM::StrToMultiLineHexstr(*data);
#endif
	}
#ifdef __FSSM_DEBUG__
	else
	{
		if (bytes_dropped >= max_bytes_dropped)
			std::cout << "SSMP1communication_procedures::getID(...):   failed to read ID from control unit: maximum number of non-ID-bytes reached.\n";
		else // timeout
			std::cout << "SSMP1communication_procedures::getID(...):   failed to read ID from control unit: timeout.\n";
	}
#endif

	_recbuffer.clear();
	return IDconfirmed;
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
			std::cout << "SSMP1communication_procedures::getNextData(...):   communication error.\n";
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
#ifdef __FSSM_DEBUG__
						std::cout << "SSMP1communication_procedures::getNextData(...):   received data: "
						          << std::hex << std::showbase << (static_cast<int>(data->back()) & 0xff) << '\n';
#endif
						return true;
					}
				}
				else	// may happen, if we got an overflow of the drivers receive-buffer
					_sync = false;
#ifdef __FSSM_DEBUG__
				if (!_sync)
					std::cout << "SSMP1communication_procedures::getNextData(...):   lost synchronisation.\n";
#endif
			}
		}
		// Delay before next iteration:
		waitms(10);
	}
#ifdef __FSSM_DEBUG__
	std::cout << "SSMP1communication_procedures::getNextData(...):   timeout.\n";
#endif
	return false;
}


bool SSMP1communication_procedures::writeDatabyte(char databyte)
{
	if (_currentaddr < 0) return false;
	bool success = sendWriteDatabyteCmd(_currentaddr, databyte);
#ifdef __FSSM_DEBUG__
	if (!success)
		std::cout << "SSMP1communication_procedures::writeDatabyte(...):   sendWriteDatabyteCmd(...) failed.\n";
	else
		std::cout << "SSMP1communication_procedures::writeDatabyte(...):   sent write-request for address "
		<< std::hex << std::showbase << _currentaddr << ": value " << (static_cast<int>(databyte) & 0xff) << '\n';
#endif
	return success;
}


char SSMP1communication_procedures::waitForDataValue(char data, unsigned int timeout)
{
	std::vector<char> datavalue;
	TimeM time;
	bool ok = false;
	time.start();
	do
	{
		datavalue.clear();
		if (!getNextData(&datavalue, timeout))
		{
#ifdef __FSSM_DEBUG__
			std::cout << "SSMP1communication_procedures::waitForDataValue(...)   getNextData(...) failed.\n";
#endif
			return false;
		}
		ok = (datavalue.at(0) == data);
	} while(!ok && (time.elapsed() < timeout));
#ifdef __FSSM_DEBUG__
	if (!ok)
		std::cout << "SSMP1communication_procedures::waitForDataValue(...):   timeout.\n";
	else
		std::cout << "SSMP1communication_procedures::waitForDataValue(...):   success.\n";
#endif
	return datavalue.at(0);
}


bool SSMP1communication_procedures::stopCUtalking(bool waitforsilence)
{
	unsigned char norec_counter = 0;
	if (!sendStopTalkingCmd())
	{
#ifdef __FSSM_DEBUG__
		std::cout << "SSMP1communication_procedures::stopCUtalking(...):   sendStopTalkingCmd() failed.\n";
#endif
		return false;
	}
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
		} while ((time.elapsed() < SSMP1_T_RECDATA_CHANGE_MAX) && (norec_counter < 5));
		/* TODO:
		 * - invastigate CU-behavior, define a T_MAX_UNTIL_SILENCE (for now, we simply use SSMP1_T_RECDATA_CHANGE_MAX)
		 * - does the CU always complete sending the current message ? (=> 256-byte-blockrates ~ 1.5s !)
		 *   => make sure it will work with block-reads (in the future)
		 */
	}
#ifdef __FSSM_DEBUG__
	if (waitforsilence)
	{
		if (norec_counter < 5)
			std::cout << "SSMP1communication_procedures::stopCUtalking(...):   timeout.\n";
		else
			std::cout << "SSMP1communication_procedures::stopCUtalking(...):   success.\n";
	}
#endif
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
							  => OK, if databyte!=A2						*/
						if ((hb == lb_old) && (_recbuffer.at(k+2) == hb))
						{
							/* Try to exclude overlapping by checking previous data (if available):
								=> OK, if the previous byte has been received and is != A1
								=> OK, if the 3. previous byte has been received and is != B1
								=> OK, if the 4. previous byte has been received and is != A1	*/
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
								  => OK, if databyte!=B1						*/
							if ((hb_old == lb) && (_recbuffer.at(k+2) == lb_old))
							{
								/* Try to exclude overlapping by checking previous data (if available):
									=> OK, if the previous byte has been received and is != B1
									=> OK, if the 2. previous byte has been receeved and is != A1	*/
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
		}
	}
#ifdef __FSSM_DEBUG__
	if (!_sync)
	{
		std::cout << "failed:\n";
		if (!syncerrstr.size())
			syncerrstr = "Address-header not detected.";
		std::cout << "    => " << syncerrstr << " Need to wait for further incoming datasets.\n";
	}
	else
		std::cout << "succesful.\n";
#endif
}

