/*
 * SSMFlagbyteDefinitionsInterface.h - Interface to the SSM flagbyte definitions
 *
 * Copyright (C) 2008-2023 Comer352L
 * Copyright (C) 2019 madanadam (Turkish language support)
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

#ifndef SSMFLAGBYTEDEFINITIONSINTERFACE_H
#define SSMFLAGBYTEDEFINITIONSINTERFACE_H



#include <QString>
#include <QStringList>
#include "SSMCUdata.h"
#include "SSMDefinitionsInterface.h"
#include "SSMFlagbyteDefinitions_SysDescriptions.h"
#include "SSMFlagbyteDefinitions_en.h"
#include "SSMFlagbyteDefinitions_de.h"
#include "SSMFlagbyteDefinitions_tr.h"



class SSMFlagbyteDefinitionsInterface: public SSMDefinitionsInterface, public SSMFlagbyteDefinitions_SysDescriptions
{

public:
	SSMFlagbyteDefinitionsInterface(QString language = "en");
	~SSMFlagbyteDefinitionsInterface();

	bool selectControlUnitID(CUtype cu, const SSMCUdata& ssmCUdata);

	bool systemDescription(std::string *description);

	bool hasOBD2system(bool *OBD2);
	bool hasVINsupport(bool *VINsup);
	bool hasImmobilizer(bool *ImmoSup);
	bool hasImmobilizerTest(bool *ImmoTestSup);
	bool hasIntegratedCC(bool *CCsup);
	bool hasClearMemory(bool *CMsup);
	bool hasClearMemory2(bool *CM2sup);
	bool hasActuatorTests(bool *ATsup);

	bool getDCblockData(std::vector<dc_block_dt> *block_data);
	bool measuringBlocks(std::vector<mb_intl_dt> *mbs);
	bool switches(std::vector<sw_intl_dt> *sws);
	bool adjustments(std::vector<adjustment_intl_dt> *adj);
	bool actuatorTests(std::vector<actuator_dt> *act);
	bool clearMemoryData(unsigned int *address, char *value);
	bool clearMemory2Data(unsigned int *address, char *value);
	bool MBdata_engineRunning(mb_enginespeed_data_dt *mb_enginespeed_data);
	bool SWdata_testModeState(sw_stateindication_data_dt *sw_testmode_data);
	bool SWdata_DCheckState(sw_stateindication_data_dt *sw_dcheckactive_data);
	bool SWdata_ignitionState(sw_stateindication_data_dt *sw_ignitionstate_data);

	void getDCcontent(unsigned int address, char databyte, QStringList *codes, QStringList *titles);	// virtual function implementation

private:
	class dc_defs_dt
	{
	public:
		unsigned int byteAddr_currentOrTempOrLatest;
		unsigned int byteAddr_historicOrMemorized;
		QString code[8];
		QString title[8];
	};

	std::vector<dc_defs_dt> *_dc_defs;
	std::vector<dc_defs_dt> *_cccc_defs;
	bool _memCCs_supported;

	void setupDiagnosticCodes();
	void setupCruiseControlCancelCodes();
	void addDCdefs(unsigned int currOrTempOrLatestDCsAddr, unsigned int histOrMemDCsAddr, QStringList rawDefs, std::vector<dc_defs_dt> * defs);
	bool getBitDCcontent(std::vector<dc_defs_dt> *defs, unsigned int addr, char databyte, QStringList *codes, QStringList *titles);
	bool hasTestMode(bool *TMsup);
	bool hasMBengineSpeed(bool *EngSpeedMBsup);

};


#endif
