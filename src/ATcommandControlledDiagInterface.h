/*
 * ATcommandControlledDiagInterface.h - AT-command controlled diagnostic interface
 *
 * Copyright (C) 2012 Comer352L
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

#ifndef ATCOMMANDCONTROLLEDDIAGINTERFACE_H
#define ATCOMMANDCONTROLLEDDIAGINTERFACE_H


#include <string>
#include <vector>
#include <QThread>
#include <QMutex>
#include "AbstractDiagInterface.h"
#include "libFSSM.h"
#ifdef __WIN32__
	#include "windows\serialCOM.h"
	#include "windows\TimeM.h"
	#define waitms(x) Sleep(x)
#elif defined __linux__
	#include <unistd.h>
	#include "linux/serialCOM.h"
	#include "linux/TimeM.h"
	#define waitms(x) usleep(1000*x)
#else
	#error "Operating system not supported !"
#endif
#ifdef __FSSM_DEBUG__
	#include <iostream>
#endif



#define CUSTOM_BAUDRATE		57600	/* Custom baud rate (increased) for ELM327/ELM329 interfaces */
/* NOTE: min. value = 4000000/255=15687 baud; max. value = 500000 baud */
/* NOTE: maximum baud rate depends on the interface circuit. The datasheet says that baud rates
 *       up to 57600 should be safe, and baud rates bigger than ~120000 often do not work reliable */

#define BRC_HS_TIMEOUT		75	/* Handshaking timeout for baud rate changing procedure [ms] */
/* NOTE: in 5 ms steps; max. 255*5ms=1275ms; chip default value = 75ms */



//#define __ENABLE_SSM2_ISO14230_EXPERIMENTAL_SUPPORT__



class ATcommandControlledDiagInterface : public AbstractDiagInterface, private QThread
{

public:
	ATcommandControlledDiagInterface();
	~ATcommandControlledDiagInterface();
	interface_type interfaceType();
	bool open( std::string name );
	bool isOpen();
	bool close();
	bool connect(AbstractDiagInterface::protocol_type protocol);
	bool isConnected();
	bool disconnect();
	bool read(std::vector<char> *buffer);
	bool write(std::vector<char> buffer);
	bool clearSendBuffer();
	bool clearReceiveBuffer();

private:
	enum if_model_dt {if_none, if_unsupported, if_model_ELM327, if_model_ELM329, if_model_AGV, if_model_AGV4000B};

	serialCOM *_port;
	bool _connected;
	float _baudrate;
	bool _custom_baudrate;
	if_model_dt _if_model;
	bool _try_echo_detection;
	bool _linefeed_enabled;
	bool _headers_enabled;
	unsigned int _source_addr;
	unsigned int _target_addr;
	QMutex _mutex;
	std::vector< std::string > _RxQueue;
	std::string _lastcmd;
	bool _flush_local_Rx_buffer;
	bool _ready;
	bool _exit;

	void run();
	std::string writeRead(std::string command, unsigned int timeout = 5000);
	bool transmitMessage(std::vector<char> data, bool flush_rec);
	if_model_dt probeInterface();
	bool configureDevice();
	std::vector<char> processRecData(std::string datamsg);
	bool changeDeviceAddresses(unsigned int source_addr, unsigned int target_addr, AbstractDiagInterface::protocol_type protocol);
	bool changeInterfaceBaudRate(unsigned int baudrate);
	std::string dataToHexStr(const std::vector<char> data);
	std::vector<char> hexStrToData(std::string hexstr);

};


#endif
