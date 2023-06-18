/*
 * AbstractSSMcommunication.cpp - Abstract Communication class for the SSM protocol communication
 *
 * Copyright (C) 2023 Comer352L
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


#include "AbstractSSMcommunication.h"


AbstractSSMcommunication::AbstractSSMcommunication()
{
	_errRetries = 2;
	_result = false;
	_abort = false;
	_delay = 0;
}


AbstractSSMcommunication::~AbstractSSMcommunication()
{
}


void AbstractSSMcommunication::setRetriesOnError(unsigned char retries)
{
	_errRetries = retries;
}

