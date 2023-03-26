/*
 * SSMFlagbyteDefinitionsInterface.h - Interface to the SSM flagbyte definitions
 *
 * Copyright (C) 2008-2019 Comer352L
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
#include "SSMprotocol.h"
#include "SSMFlagbyteDefinitions_SysDescriptions.h"
#include "SSMFlagbyteDefinitions_en.h"
#include "SSMFlagbyteDefinitions_de.h"
#include "SSMFlagbyteDefinitions_tr.h"



class SSMFlagbyteDefinitionsInterface: public SSMFlagbyteDefinitions_SysDescriptions
{

public:
	SSMFlagbyteDefinitionsInterface(QString language = "en");
	~SSMFlagbyteDefinitionsInterface();

	void setLanguage(QString lang);
	bool selectControlUnitID(CUtype cu, const SSMCUdata& ssmCUdata);

	bool systemDescription(QString *description);

	bool hasOBD2system(bool *OBD2);
	bool hasVINsupport(bool *VINsup);
	bool hasImmobilizer(bool *ImmoSup);
	bool hasImmobilizerTest(bool *ImmoTestSup);
	bool hasIntegratedCC(bool *CCsup);
	bool hasClearMemory(bool *CMsup);
	bool hasClearMemory2(bool *CM2sup);
	bool hasTestMode(bool *TMsup);
	bool hasActuatorTests(bool *ATsup);
	bool hasMBengineSpeed(bool *EngSpeedMBsup);
	bool hasSWignition(bool *IgnSWsup);

	bool diagnosticCodes(std::vector<dc_defs_dt> *diagnosticCodes, bool *fmt_OBD2);
	bool cruiseControlCancelCodes(std::vector<dc_defs_dt> *cancelCodes, bool *memCC_supported);
	bool measuringBlocks(std::vector<mb_intl_dt> *mbs);
	bool switches(std::vector<sw_intl_dt> *sws);
	bool adjustments(std::vector<adjustment_intl_dt> *adj);
	bool actuatorTests(std::vector<actuator_dt> *act);

private:
	QString _language;
	bool _id_set;
	CUtype _CU;
	SSMCUdata _ssmCUdata;

	void addDCdefs(unsigned int currOrTempOrLatestDCsAddr, unsigned int histOrMemDCsAddr, QStringList rawDefs, std::vector<dc_defs_dt> * defs);

};


#endif
