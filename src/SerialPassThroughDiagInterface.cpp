/*
 * SerialPassThroughDiagInterface.cpp - Serial port pass-through diagnostic interface
 *
 * Copyright (C) 2010-2019 Comer352L
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

#include "SerialPassThroughDiagInterface.h"


SerialPassThroughDiagInterface::SerialPassThroughDiagInterface()
{
	std::vector<protocol_type> supportedProtocols;

	_port = NULL;
	_connected = false;
	setName("Serial Port Pass-Through");
	setVersion("1.0");
	supportedProtocols.push_back(protocol_type::SSM1);
	supportedProtocols.push_back(protocol_type::SSM2_ISO14230);
	setSupportedProtocols(supportedProtocols);
	/* NOTE: due to the interfaces construction, we can not know which protocol it actually supports !
	 * One possibility would be to check for an data echo:
	 * - SSM1-interfaces must not have an echo
	 * - SSM2-interfaces must have an echo
	 * Problem:
	 * - echo test makes sense only if the interface is powered
	 * - we still don't know if the protocol is really supported
	 */
	setProtocolBaudrate( 0 );
}


SerialPassThroughDiagInterface::~SerialPassThroughDiagInterface()
{
	disconnect();
	close();
}


AbstractDiagInterface::interface_type SerialPassThroughDiagInterface::interfaceType()
{
	return interface_type::serialPassThrough;
}


bool SerialPassThroughDiagInterface::open( std::string name )
{
	if (_port)
		return false;
	_port = new serialCOM;
	if (_port->OpenPort( name ))
	{
		_port->SetControlLines(true, false);
		_port->ClearSendBuffer();
		_port->ClearReceiveBuffer();
		// FIXME: examine interface to find out which protocol it supports (e.g. test for echo). See comment in constructor.
		// NOTE: we can not assume that the interface is powered (ignition switched on) !
		return true;
	}
	delete _port;
	_port = NULL;
	return false;
}


bool SerialPassThroughDiagInterface::isOpen()
{
	return _port;
}


bool SerialPassThroughDiagInterface::close()
{
	if (_port)
	{
		if (_connected)
			disconnect();
		if (_port->ClosePort())
		{
			delete _port;
			_port = NULL;
			return true;
		}
	}
	return false;
}


bool SerialPassThroughDiagInterface::connect(protocol_type protocol)
{
	if (_port && !_connected)
	{
		if (protocol == AbstractDiagInterface::protocol_type::SSM1)
		{
			if (_port->SetPortSettings(1953, 8, 'E', 1))
			{
				double baudrate = 0;
				if(!_port->GetPortSettings(&baudrate))
					return false;
				if ((baudrate < (0.97*1953)) || (baudrate > (1.03*1953)))
				{
#ifdef __FSSM_DEBUG__
					std::cout << "SerialPassThroughDiagInterface::connect(...):   Selected baudrate not supported (devergence > 3%)\n";
#endif
					return false;
				}
				setProtocolType( protocol );
				setProtocolBaudrate( 1953 );
				_connected = true;
				return true;
			}
		}
		else if (protocol == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
		{
			if (_port->SetPortSettings(4800, 8, 'N', 1))
			{
				setProtocolType( protocol );
				setProtocolBaudrate( 4800 );
				_connected = true;
				return true;
			}
		}
	}
	return false;
}


bool SerialPassThroughDiagInterface::isConnected()
{
	return (_port && _connected);
}


bool SerialPassThroughDiagInterface::disconnect()
{
	if (_port)
	{
		_connected = false;
		setProtocolType( protocol_type::NONE );
		setProtocolBaudrate( 0 );
		return true;
	}
	return false;
}


bool SerialPassThroughDiagInterface::read(std::vector<char> *buffer)
{
	if (_port && _connected)
	{
		unsigned int nbytes = 0;
		if (_port->GetNrOfBytesAvailable(&nbytes))
		{
			std::vector<char> readbuffer;
			if (nbytes)
			{
				if (!_port->Read( 0, nbytes, 0, &readbuffer ))
					return false;
			}
			buffer->assign(readbuffer.begin(), readbuffer.end());
			return true;
		}
	}
	return false;
}


bool SerialPassThroughDiagInterface::write(std::vector<char> buffer)
{
	if (_port && _connected)
	{
		TimeM time;
		unsigned int t_el = 0;
		unsigned int T_Tx_min = 0;
		if (protocolType() == AbstractDiagInterface::protocol_type::SSM1)
		{
			T_Tx_min = static_cast<unsigned int>(1000 * buffer.size() * 11 / 1953.0);
		}
		else if (protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
		{
			T_Tx_min = static_cast<unsigned int>(1000 * buffer.size() * 10 / 4800.0);
		}
		time.start();
		if (!_port->Write( buffer ))
			return false;
		if (T_Tx_min)
		{
			t_el = time.elapsed();
			if (t_el < T_Tx_min)
				waitms(T_Tx_min - t_el);
		}
		return true;
	}
	return false;
}


bool SerialPassThroughDiagInterface::clearSendBuffer()
{
	if (_port == NULL)
		return false;
	return _port->ClearSendBuffer();
}


bool SerialPassThroughDiagInterface::clearReceiveBuffer()
{
	if (_port == NULL)
		return false;
	return _port->ClearReceiveBuffer();
}

