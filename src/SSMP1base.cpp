/*
 * SSMP1base.cpp - Basic definitions and commands for the SSM1-protocol
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

#include "SSMP1base.h"


SSMP1commands::SSMP1commands(AbstractDiagInterface *diagInterface)
{
	_diagInterface = diagInterface;
}


bool SSMP1commands::sendStopTalkingCmd()
{
	char stopmsg[4] = {SSMP1_CMD_STOP_READING, '\x0', '\x0', '\x0'};
	return sendMsg(stopmsg, 4);
}


bool SSMP1commands::sendQueryIdCmd(unsigned int addr, unsigned char extradatalen)
{
	if (addr > 0xffff) return false;
	char querymsg[4] = {SSMP1_CMD_GET_ID, static_cast<char>((addr >> 8) & 0xFF), static_cast<char>(addr & 0xFF), static_cast<char>(extradatalen)};
	return sendMsg(querymsg, 4);
}


bool SSMP1commands::sendReadAddressCmd(SSM1_CUtype_dt cu, unsigned int dataaddr)
{
	if (dataaddr > 0xffff) return false;
	char msg[4] = {0,};
	unsigned char msglen = 0;
	if (cu == SSM1_CU_Engine)
	{
		msg[0] = SSMP1_CMD_READ_ENGINE;
		msglen = 4;
	}
	else if (cu == SSM1_CU_Transmission)
	{
		msg[0] = SSMP1_CMD_READ_TRANSMISSION;
		msglen = 4;
	}
	else if (cu == SSM1_CU_CruiseCtrl)
	{
		msg[0] = SSMP1_CMD_READ_CRUISECONTROL;
		msglen = 4;
	}
	else if (cu == SSM1_CU_AirCon)
	{
		msg[0] = SSMP1_CMD_READ_AIRCON;
		msglen = 3;
	}
	else if (cu == SSM1_CU_AirCon2)
	{
		msg[0] = SSMP1_CMD_READ_AIRCON2;
		msglen = 4;
	}
	else if (cu == SSM1_CU_FourWS)
	{
		msg[0] = SSMP1_CMD_READ_4WS;
		msglen = 3;
	}
	else if (cu == SSM1_CU_ABS)
	{
		msg[0] = SSMP1_CMD_READ_ABS;
		msglen = 4;
	}
	else if (cu == SSM1_CU_AirSusp)
	{
		msg[0] = SSMP1_CMD_READ_AIRSUSP;
		msglen = 4;
	}
	else if (cu == SSM1_CU_PwrSteer)
	{
		msg[0] = SSMP1_CMD_READ_POWERSTEERING;
		msglen = 4;
	}
	else
		return false;
	char highaddrbyte = (dataaddr & 0xffff) >> 8;
	char lowaddrbyte = dataaddr & 0xff;
	msg[1] = highaddrbyte;
	msg[2] = lowaddrbyte;
	msg[3] = '\x00';
	return sendMsg(msg, msglen);
	/* TODO: add block read support for newer ECUs/TCUs
		 => make nrofbytes a fcn parameter          */
}


bool SSMP1commands::sendWriteDatabyteCmd(unsigned int dataaddr, char databyte)
{
	if (dataaddr > 0xffff) return false;
	char highbyte = (dataaddr & 0xffff) >> 8;
	char lowbyte = dataaddr & 0xff;
	char writemsg[4] = {static_cast<char>(SSMP1_CMD_WRITE), highbyte, lowbyte, databyte};
	return sendMsg(writemsg, 4);
}


bool SSMP1commands::sendMsg(char msg[4], unsigned char msglen)
{
	std::vector<char> buffer(msg, msg + msglen);
	// SEND MESSAGE:
	if (!_diagInterface->write(buffer))
		return false;
	return true;
}
