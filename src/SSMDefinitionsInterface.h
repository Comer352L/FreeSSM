/*
 * SSMDefinitionsInterface.h - Abstract interface to the SSM definitions
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

#ifndef SSMDEFINITIONSINTERFACE_H
#define SSMDEFINITIONSINTERFACE_H


#include <vector>
#include <QString>
#include <QStringList>
#include <climits>
#include "SSMCUdata.h"


#define		MEMORY_ADDRESS_NONE		UINT_MAX


class dc_addr_dt
{
public:
	enum class Type {currentOrTempOrLatest, historicOrMemorized, CCCCsLatest, CCCCsMemorized};
	enum class Scaling {bitwise, list};
	enum class Format {simple, OBD};

	Type type;
	Scaling scaling;
	Format format;
	unsigned int address;
};


class dc_block_dt
{
public:
	std::vector<dc_addr_dt> addresses;
};


class  mb_dt
{
public:
	QString title;
	QString unit;
	QString formula;
	char precision;
};


class  sw_dt
{
public:
	QString title;
	QString unit;
};


class  mb_intl_dt : public mb_dt
{
public:
	unsigned int addrLow;
	unsigned int addrHigh;
};


class sw_intl_dt : public sw_dt
{
public:
	unsigned int  byteAddr;
	unsigned char bitAddr;
};


class mb_enginespeed_data_dt
{
public:
	unsigned int addr_low = MEMORY_ADDRESS_NONE;
	unsigned int addr_high = MEMORY_ADDRESS_NONE;
	QString scaling_formula;
};


class sw_stateindication_data_dt
{
public:
	unsigned int addr = MEMORY_ADDRESS_NONE;
	unsigned char bit = 0;
	bool inverted = false;;
};


class adjustment_dt
{
public:
	QString title;
	QString unit;
	QString formula;
	unsigned int rawMin;
	unsigned int rawMax;
	unsigned int rawDefault;
	char precision;
};


class adjustment_intl_dt : public adjustment_dt
{
public:
	unsigned int addrLow;
	unsigned int addrHigh;
};


class actuator_dt
{
public:
	QString title;
	unsigned int byteAddr;
	unsigned char bitAddr;
};


class SSMDefinitionsInterface
{
public:
	SSMDefinitionsInterface(QString language = "en");
	virtual ~SSMDefinitionsInterface();

	void setLanguage(QString language);

	virtual bool systemDescription(std::string *description) = 0;

	virtual bool getDCBlockData(std::vector<dc_block_dt> *block_data);
	virtual bool measuringBlocks(std::vector<mb_intl_dt> *mbs);
	virtual bool switches(std::vector<sw_intl_dt> *sws);
	virtual bool adjustments(std::vector<adjustment_intl_dt> *adj);
	virtual bool actuatorTests(std::vector<actuator_dt> *act);
	virtual bool clearMemoryData(unsigned int *address, char *value);
	virtual bool MBdata_engineRunning(mb_enginespeed_data_dt *mb_enginespeed_data);
	virtual bool SWdata_testModeState(sw_stateindication_data_dt *sw_testmode_data);
	virtual bool SWdata_DCheckState(sw_stateindication_data_dt *sw_dcheckactive_data);
	virtual bool SWdata_ignitionState(sw_stateindication_data_dt *sw_ignitionstate_data);

	virtual void getDCcontent(unsigned int address, char databyte, QStringList *codes, QStringList *titles) = 0;

protected:
	QString _language;
	bool _id_set;
	CUtype _CU;
	SSMCUdata _ssmCUdata;

private:
	bool SWdata_xxxState_unsupported(sw_stateindication_data_dt *sw_state_data);

};


#endif

