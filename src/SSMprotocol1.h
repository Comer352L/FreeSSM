/*
 * SSMprotocol1.h - Application Layer for the old Subaru SSM protocol
 *
 * Copyright (C) 2009 Comer352l
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



#ifdef __WIN32__
    #include "windows\serialCOM.h"
#elif defined __linux__
    #include "linux/serialCOM.h"
#else
    #error "Operating system not supported !"
#endif
#include <string>
#include <vector>
#include <math.h>
#include "SSMprotocol.h"
#include "SSMP1communication.h"
#include "libFSSM.h"



class SSMprotocol1 : public SSMprotocol
{
	Q_OBJECT

public:
	SSMprotocol1(serialCOM *port, CUtype_dt CU, QString language="en");
	~SSMprotocol1();
	// NON-COMMUNICATION-FUNCTIONS:
	bool setupCUdata(bool ignoreIgnitionOFF=false);							// IMPLEMENTATION MISSING
	std::string getROMID();
	bool getSystemDescription(QString *sysdescription);						// IMPLEMENTATION MISSING
	bool hasOBD2system(bool *OBD2);
	bool hasVINsupport(bool *VINsup);
	bool hasImmobilizer(bool *ImmoSup);								// IMPLEMENTATION MISSING
	bool hasIntegratedCC(bool *CCsup);
	bool hasClearMemory2(bool *CM2sup);
	bool hasTestMode(bool *TMsup);
	bool hasActuatorTests(bool *ATsup);
	bool getSupportedDCgroups(int *DCgroups);
	bool getSupportedAdjustments(std::vector<adjustment_dt> *supportedAdjustments);
	// COMMUNICATION BASED FUNCTIONS:
	bool isEngineRunning(bool *isrunning);								// IMPLEMENTATION MISSING; DO WE NEED IT ?
	bool clearMemory(CMlevel_dt level, bool *success);						// IMPLEMENTATION MISSING
	bool testImmobilizerCommLine(immoTestResult_dt *result);					// IMPLEMENTATION MISSING
	bool startDCreading(int DCgroups);
	bool stopDCreading();
	bool startMBSWreading(std::vector<MBSWmetadata_dt> mbswmetaList);
	bool stopMBSWreading();
	bool waitForIgnitionOff();									// IMPLEMENTATION MISSING

private:
	SSMP1communication *_SSMP1com;
	// *** CONTROL UNIT RAW DATA ***:
	char _ROM_ID[3];

	// CU-FEATURES SETUP FUNCTIONS:
	void setupDTCaddresses();
	void setupSupportedMBs();
	void setupSupportedSWs();
	// EVALUATION FUNCTIONS:
/*	void evaluateDCdataByte(unsigned int DCbyteadr, char DCrawdata, QStringList DC_rawDefs,
				QStringList *DC, QStringList *DCdescription);
	=> adapt from SSMprotocol2 !								*/

private slots:
	void processDCsRawdata(QByteArray dcrawdata, int duration_ms);					// INCOMPLETE IMPLEMENTATION

public slots:
	void resetCUdata();										// IMPLEMENTATION MISSING

};



#endif

