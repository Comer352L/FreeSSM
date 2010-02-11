/*
 * SSMP1base.h - Basic definitions and commands for the SSM1-protocol
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

#ifndef SSMP1COMMANDS_H
#define SSMP1COMMANDS_H


#include "AbstractDiagInterface.h"


/* SSM1-protocol-commands */
#define		SSMP1_CMD_GET_ID			0x00
#define		SSMP1_CMD_STOP_READING			0x12
#define		SSMP1_CMD_READ_ENGINE			0x78
#define		SSMP1_CMD_READ_TRANSMISSION		0x45
#define		SSMP1_CMD_READ_CRUISECONTROL		0xAB
#define		SSMP1_CMD_READ_AIRCON			0x89
#define		SSMP1_CMD_READ_4WS			0x92
#define		SSMP1_CMD_WRITE				0xAA

/* Timings */
#define		SSMP1_T_IC_WAIT			40	/* delay between two commands [ms] */
#define		SSMP1_T_NEWDATA_REC_MAX		100	/* max. time until a new dataset/ROM-ID is recieved [ms] */

/* Timeout */
#define		SSMP1_T_RW_REC_MAX		1000	/* timeout for reply-message to read-/write-request [ms] */



enum SSM1_CUtype_dt {SSM1_CU_Engine, SSM1_CU_Transmission, SSM1_CU_CruiseCtrl, SSM1_CU_AirCon, SSM1_CU_FourWS};


class SSMP1commands
{

public:
	SSMP1commands(AbstractDiagInterface *diagInterface);
	bool sendStopTalkingCmd();
	bool sendQueryIdCmd();
	bool sendReadAddressCmd(SSM1_CUtype_dt cu, unsigned int dataaddr);
	bool sendWriteDatabyteCmd(unsigned int dataaddr, char databyte);

protected:
	AbstractDiagInterface *_diagInterface;

private:
	bool sendMsg(char msg[4], unsigned char msglen);

};


#endif

