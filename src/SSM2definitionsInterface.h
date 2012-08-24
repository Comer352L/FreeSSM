/*
 * SSM2definitionsInterface.h - Interface to the SSM1-definitions
 *
 * Copyright (C) 2008-2012 Comer352L
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

#ifndef SSM2DEFINITIONSINTERFACE_H
#define SSM2DEFINITIONSINTERFACE_H



#include <QString>
#include <QStringList>
#include "SSMprotocol.h"
#include "SSMprotocol2_ID.h"
#include "SSMprotocol2_def_en.h"
#include "SSMprotocol2_def_de.h"



class SSM2definitionsInterface: public SSMprotocol2_ID
{

public:
	SSM2definitionsInterface(QString language = "en");
	~SSM2definitionsInterface();

	void setLanguage(QString lang);
	void selectControlUnitID(SSMprotocol::CUtype_dt cu, char id1[3], char id2[5], char flagbytes[96], unsigned char nrofflagbytes);

	bool systemDescription(QString *description);

	bool hasOBD2system(bool *OBD2);
	bool hasVINsupport(bool *VINsup);
	bool hasImmobilizer(bool *ImmoSup);
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
	SSMprotocol::CUtype_dt _CU;
	char _ID1[3];
	char _ID2[5];
	char _flagbytes[96];
	unsigned char _nrofflagbytes;

	void addDCdefs(unsigned int currOrTempOrLatestDCsAddr, unsigned int histOrMemDCsAddr, QStringList rawDefs, std::vector<dc_defs_dt> * defs);

};


#endif
