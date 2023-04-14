/*
 * SSMprotocol1.h - Application Layer for the old Subaru SSM protocol
 *
 * Copyright (C) 2009-2023 Comer352L
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



#include <QString>
#include <vector>
#include "AbstractDiagInterface.h"
#include "SSMCUdata.h"
#include "SSMprotocol.h"
#include "SSMP1communication.h"
#include "SSMLegacyDefinitionsInterface.h"
#include "SSMFlagbyteDefinitionsInterface.h"



class SSMprotocol1 : public SSMprotocol
{
	Q_OBJECT

public:
	SSMprotocol1(AbstractDiagInterface *diagInterface, QString language="en");
	~SSMprotocol1();
	// NON-COMMUNICATION-FUNCTIONS:
	CUsetupResult_dt setupCUdata(enum CUtype CU);
	protocol_dt protocolType() { return SSM1; }
	// COMMUNICATION BASED FUNCTIONS:
	bool startDCreading(int DCgroups);
	bool stopDCreading();
	bool startMBSWreading(const std::vector<MBSWmetadata_dt>& mbswmetaList);
	bool stopMBSWreading();
	bool getAdjustmentValue(unsigned char index, unsigned int *rawValue);
	bool getAllAdjustmentValues(std::vector<unsigned int> *rawValues);
	bool setAdjustmentValue(unsigned char index, unsigned int rawValue);
	bool startActuatorTest(unsigned char actuatorTestIndex);
	bool stopActuatorTesting();
	bool stopAllActuators();
	bool clearMemory(CMlevel_dt level, bool *success);
	bool testImmobilizerCommLine(immoTestResult_dt *result);
	bool isEngineRunning(bool *isrunning);
	bool isInTestMode(bool *testmode);
	bool waitForIgnitionOff();

private:
	SSMP1communication *_SSMP1com;

	bool readExtendedID(std::vector<char>& ID);

public slots:
	void resetCUdata();

};



#endif

