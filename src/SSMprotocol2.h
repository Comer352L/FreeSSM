/*
 * SSMprotocol2.h - Application Layer for the new Subaru SSM protocol
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

#ifndef SSMPROTOCOL2_H
#define SSMPROTOCOL2_H



#include <string>
#include <vector>
#include <math.h>
#include "AbstractDiagInterface.h"
#include "SSMprotocol.h"
#include "SSMP2communication.h"
#include "SSM2definitionsInterface.h"



class SSMprotocol2 : public SSMprotocol, private SSMprotocol2_ID
{
	Q_OBJECT

public:
	SSMprotocol2(AbstractDiagInterface *diagInterface, QString language="en");
	~SSMprotocol2();
	// NON-COMMUNICATION-FUNCTIONS:
	CUsetupResult_dt setupCUdata(CUtype_dt CU);
	CUsetupResult_dt setupCUdata(CUtype_dt CU, bool ignoreIgnitionOFF=false);
	protocol_dt protocolType() { return SSM2; };
	bool getSystemDescription(QString *sysdescription);
	bool hasOBD2system(bool *OBD2);
	bool hasVINsupport(bool *VINsup);
	bool hasImmobilizer(bool *ImmoSup);
	bool hasIntegratedCC(bool *CCsup);
	bool hasClearMemory(bool *CMsup);
	bool hasClearMemory2(bool *CM2sup);
	bool hasTestMode(bool *TMsup);
	bool hasActuatorTests(bool *ATsup);
	bool getSupportedDCgroups(int *DCgroups);
	bool getSupportedAdjustments(std::vector<adjustment_dt> *supportedAdjustments);
	bool getSupportedActuatorTests(QStringList *actuatorTestTitles);
	bool getLastActuatorTestSelection(unsigned char *actuatorTestIndex);
	// COMMUNICATION BASED FUNCTIONS:
	bool isEngineRunning(bool *isrunning);
	bool isInTestMode(bool *testmode);
	bool getVIN(QString *VIN);
	bool getAllAdjustmentValues(std::vector<unsigned int> * rawValues);
	bool getAdjustmentValue(unsigned char index, unsigned int *rawValue);
	bool setAdjustmentValue(unsigned char index, unsigned int rawValue);
	bool clearMemory(CMlevel_dt level, bool *success);
	bool testImmobilizerCommLine(immoTestResult_dt *result);
	bool stopAllActuators();
	bool startDCreading(int DCgroups);
	bool stopDCreading();
	bool startMBSWreading(std::vector<MBSWmetadata_dt> mbswmetaList);
	bool stopMBSWreading();
	bool startActuatorTest(unsigned char actuatorTestIndex);
	bool restartActuatorTest();
	bool stopActuatorTesting();
	bool waitForIgnitionOff();

private:
	SSMP2communication *_SSMP2com;
	SSM2definitionsInterface *_SSM2defsIface;
	// *** CONTROL UNIT BASIC DATA (SUPPORTED FEATURES) ***:
	bool _DTC_fmt_OBD2;
	// Cruise Control Cancel Codes:
	std::vector<dc_defs_dt> _CCCCdefs;
	// Adjustment Values:
	std::vector<adjustment_intl_dt> _adjustments;
	// Actuator Tests:
	std::vector<actuator_dt> _actuators;
	std::vector<unsigned int> _allActByteAddr;
	// *** Selection data ***:
	unsigned char _selectedActuatorTestIndex;

	void setupActuatorTestData();
	bool validateVIN(char VIN[17]);

private slots:
	void processDCsRawdata(std::vector<char> dcrawdata, int duration_ms);

public slots:
	void resetCUdata();

};



#endif
