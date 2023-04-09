/*
 * SSMDefinitionsInterface.cpp - Interface to the SSM definitions
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

#include "SSMDefinitionsInterface.h"


SSMDefinitionsInterface::SSMDefinitionsInterface(QString language)
{
	_language = language;
	_id_set = false;
	_CU = CUtype::Engine;
}


SSMDefinitionsInterface::~SSMDefinitionsInterface()
{
}


void SSMDefinitionsInterface::setLanguage(QString language)
{
	_language = language;
}


bool SSMDefinitionsInterface::measuringBlocks(std::vector<mb_intl_dt> *mbs)
{
	if (mbs == NULL)
		return false;
	mbs->clear();
	if (!_id_set)
		return false;
	return true;
}


bool SSMDefinitionsInterface::switches(std::vector<sw_intl_dt> *sws)
{
	if (sws == NULL)
		return false;
	sws->clear();
	if (!_id_set)
		return false;
	return true;
}


bool SSMDefinitionsInterface::adjustments(std::vector<adjustment_intl_dt> *adj)
{
	if (adj == NULL)
		return false;
	adj->clear();
	if (!_id_set)
		return false;
	return true;
}


bool SSMDefinitionsInterface::actuatorTests(std::vector<actuator_dt> *act)
{
	if (act == NULL)
		return false;
	act->clear();
	if (!_id_set)
		return false;
	return true;
}


bool SSMDefinitionsInterface::clearMemoryData(unsigned int *address, char *value)
{
	*address = MEMORY_ADDRESS_NONE;
	*value = '\x00';
	if (!_id_set)
		return false;
	return true;
}

