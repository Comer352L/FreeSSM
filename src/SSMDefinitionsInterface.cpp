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


bool SSMDefinitionsInterface::getDCBlockData(std::vector<dc_block_dt> *block_data)
{
	if (block_data == NULL)
		return false;
	block_data->clear();
	if (!_id_set)
		return false;
	return true;
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


bool SSMDefinitionsInterface::MBdata_engineRunning(mb_enginespeed_data_dt *mb_enginespeed_data)
{
	if (mb_enginespeed_data == NULL)
		return false;
	mb_enginespeed_data->addr_low = MEMORY_ADDRESS_NONE;
	mb_enginespeed_data->addr_high = MEMORY_ADDRESS_NONE;
	mb_enginespeed_data->scaling_formula = "";
	if (!_id_set)
		return false;
	return true;
}


bool SSMDefinitionsInterface::SWdata_testModeState(sw_stateindication_data_dt *sw_testmode_data)
{
	return SWdata_xxxState_unsupported(sw_testmode_data);
}


bool SSMDefinitionsInterface::SWdata_DCheckState(sw_stateindication_data_dt *sw_testmode_data)
{
	return SWdata_xxxState_unsupported(sw_testmode_data);
}


bool SSMDefinitionsInterface::SWdata_ignitionState(sw_stateindication_data_dt *sw_ignitionstate_data)
{
	return SWdata_xxxState_unsupported(sw_ignitionstate_data);
}


// PRIVATE


bool SSMDefinitionsInterface::SWdata_xxxState_unsupported(sw_stateindication_data_dt *sw_state_data)
{
	if (sw_state_data == NULL)
		return false;
	sw_state_data->addr = MEMORY_ADDRESS_NONE;
	sw_state_data->bit = 0;
	sw_state_data->inverted = false;
	if (!_id_set)
		return false;
	return true;
}

