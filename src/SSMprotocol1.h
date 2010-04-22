/*
 * SSMprotocol1.h - Application Layer for the old Subaru SSM protocol
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

#ifndef SSMPROTOCOL1_H
#define SSMPROTOCOL1_H



#include <string>
#include <vector>
#include <math.h>
#include "AbstractDiagInterface.h"
#include "SSMprotocol.h"
#include "SSMP1communication.h"
#include "SSM1definitionsInterface.h"
#include "libFSSM.h"



class SSMprotocol1 : public SSMprotocol
{
	Q_OBJECT

public:
	SSMprotocol1(AbstractDiagInterface *diagInterface, QString language="en");
	~SSMprotocol1();
	// NON-COMMUNICATION-FUNCTIONS:
	bool setupCUdata(CUtype_dt CU);									// INCOMPLETE IMPLEMENTATION
	protocol_dt protocolType() { return SSM1; };
	std::string getSysID();
	std::string getROMID();
	bool getSystemDescription(QString *sysdescription);
	bool hasOBD2system(bool *OBD2);
	bool hasVINsupport(bool *VINsup);
	bool hasImmobilizer(bool *ImmoSup);								// IMPLEMENTATION MISSING
	bool hasIntegratedCC(bool *CCsup);
	bool hasClearMemory(bool *CMsup);
	bool hasClearMemory2(bool *CM2sup);
	bool hasTestMode(bool *TMsup);
	bool hasActuatorTests(bool *ATsup);
	bool getSupportedDCgroups(int *DCgroups);
	bool getSupportedAdjustments(std::vector<adjustment_dt> *supportedAdjustments);
	// COMMUNICATION BASED FUNCTIONS:
	bool isEngineRunning(bool *isrunning);								// IMPLEMENTATION MISSING; DO WE NEED IT ?
	bool clearMemory(CMlevel_dt level, bool *success);
	bool testImmobilizerCommLine(immoTestResult_dt *result);					// IMPLEMENTATION MISSING
	bool startDCreading(int DCgroups);
	bool stopDCreading();
	bool startMBSWreading(std::vector<MBSWmetadata_dt> mbswmetaList);
	bool stopMBSWreading();
	bool waitForIgnitionOff();									// IMPROVE ?

private:
	SSMP1communication *_SSMP1com;
	// *** CONTROL UNIT RAW DATA ***:
	char _ID[3];
	std::string _sysDescription;
	unsigned int _CMaddr;
	char _CMvalue;

private slots:
	void processDCsRawdata(std::vector<char> dcrawdata, int duration_ms);				// INCOMPLETE IMPLEMENTATION

public slots:
	void resetCUdata();

};



#endif

