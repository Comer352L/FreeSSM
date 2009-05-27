/*
 * SSMprotocol2.h - Application Layer for the new Subaru SSM protocol
 *
 * Copyright (C) 2008-2009 Comer352l
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
#include "SSMP2communication.h"
#include "SSMprotocol2_ID.h"
#include "SSMprotocol2_def_en.h"
#include "SSMprotocol2_def_de.h"
#include "libFSSM.h"




#define SSMP_MAX_MB			300				/* currently 156 */
#define SSMP_MAX_SW			300				/* currently 154 */
#define SSMP_MAX_MBSW			(SSMP_MAX_MB + SSMP_MAX_SW)
#define SSMP_MAX_DTCADDR		100				/* currently 61/61	<256 ! */
#define SSMP_MAX_CCCCADDR		10				/* currently 4/4	<256 ! */
#define SSMP_MAX_ADJUSTMENTS		30				/* currently 24		<256 ! */




class  mbsw_dt
{
public:
	QString title;
	QString unit;
};


class  mb_intl_dt: public mbsw_dt
{
public:
	unsigned int adr_low;
	unsigned int adr_high;
	QString scaleformula;
	char precision;
};


class sw_intl_dt : public mbsw_dt
{
public:
	unsigned int  byteadr;
	unsigned char bitadr;
};


class MBSWmetadata_dt
{
public:
	bool blockType;
	unsigned int nativeIndex;
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
	unsigned int AddrLow;
	unsigned int AddrHigh;
};


class actuator_dt
{
public:
	QString title;
	unsigned int byteadr;
	unsigned char bitadr;
};



class SSMprotocol2 : public QObject, private SSMprotocol2_ID
{
	Q_OBJECT

public:
	enum CUtype_dt {ECU=1, TCU=2};
	enum state_dt {state_needSetup=0, state_normal=1, state_DCreading=2, state_MBSWreading=3, state_ActTesting=4, state_waitingForIgnOff=5};
	enum DCgroups_dt {noDCs_DCgroup=0, currentDTCs_DCgroup=1, temporaryDTCs_DCgroup=2, historicDTCs_DCgroup=4, memorizedDTCs_DCgroup=8,
			  CClatestCCs_DCgroup=16, CCmemorizedCCs_DCgroup=32, allDCs_DCgroup=63};
	enum CMlevel_dt {CMlevel_1=1, CMlevel_2=2};
	enum immoTestResult_dt {immoNotShorted, immoShortedToGround, immoShortedToBattery};

	SSMprotocol2(serialCOM *port, CUtype_dt CU, QString language="en");
	~SSMprotocol2();
	// NON-COMMUNICATION-FUNCTIONS:
	SSMprotocol2::CUtype_dt CUtype();
	SSMprotocol2::state_dt state();
	bool setupCUdata(bool ignoreIgnitionOFF=false);
	std::string getSysID();
	std::string getROMID();
	bool getSystemDescription(QString *sysdescription);
	bool hasOBD2system(bool *OBD2);
	bool hasVINsupport(bool *VINsup);
	bool hasImmobilizer(bool *ImmoSup);
	bool hasIntegratedCC(bool *CCsup);
	bool hasClearMemory2(bool *CM2sup);
	bool hasTestMode(bool *TMsup);
	bool hasActuatorTests(bool *ATsup);
	bool getSupportedDCgroups(int *DCgroups);
	bool getLastDCgroupsSelection(int *DCgroups);
	bool getSupportedMBs(std::vector<mbsw_dt> *supportedMBs);
	bool getSupportedSWs(std::vector<mbsw_dt> *supportedSWs);
	bool getLastMBSWselection(std::vector<MBSWmetadata_dt> *MBSWmetaList);
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
	bool ClearMemory(CMlevel_dt level, bool *success);
	bool TestImmobilizerCommLine(immoTestResult_dt *result);
	bool stopAllActuators();
	bool startDCreading(int DCgroups);
	bool restartDCreading();
	bool stopDCreading();
	bool startMBSWreading(std::vector<MBSWmetadata_dt> mbswmetaList);
	bool restartMBSWreading();
	bool stopMBSWreading();
	bool startActuatorTest(unsigned char actuatorTestIndex);
	bool restartActuatorTest();
	bool stopActuatorTesting();
	bool stopAllPermanentOperations();
	bool waitForIgnitionOff();

private:
	serialCOM *_port;
	SSMP2communication *_SSMP2com;
	CUtype_dt _CU;
	QString _language;
	state_dt _state;
	// *** CONTROL UNIT RAW DATA ***:
	char _SYS_ID[3];
	char _ROM_ID[5];
	char _flagbytes[96];
	unsigned char _nrofflagbytes;
	// *** CONTROL UNIT BASIC DATA (SUPPORTED FEATURES) ***:
	// Diagnostic Trouble Codes:
	std::vector<unsigned int> _currOrTempDTCsAddr;
	std::vector<unsigned int> _histOrMemDTCsAddr;
	QStringList _DTC_rawDefs;
	// Cruise Control Cancel Codes:
	std::vector<unsigned int> _latestCCCCsAddr;
	std::vector<unsigned int> _memorizedCCCCsAddr;
	QStringList _CC_rawDefs;
	// Measuring Blocks and Switches:
	std::vector<mb_intl_dt> _supportedMBs;
	std::vector<sw_intl_dt> _supportedSWs;
	// Adjustment Values:
	std::vector<adjustment_intl_dt> _adjustments;
	// Actuator Tests:
	std::vector<actuator_dt> _actuators;
	std::vector<unsigned int> _allActByteAddr;
	// *** Selection data ***:
	int _selectedDCgroups;
	std::vector<MBSWmetadata_dt> _MBSWmetaList;
	unsigned char _selectedActuatorTestIndex;
	// *** Temporary (operation related) data ***:
	unsigned int _selMBsSWaAddr[SSMP_MAX_MBSW];
	unsigned int _selMBsSWsAddrLen;

	// CU-FEATURES SETUP FUNCTIONS:
	void setupDTCaddresses();
	void setupCCCCaddresses();
	void setupSupportedMBs();
	void setupSupportedSWs();
	void setupAdjustmentsData();
	void setupActuatorTestData();
	// PREPARATION AND EVALUATION FUNCTIONS:
	void evaluateDCdataByte(unsigned int DCbyteadr, char DCrawdata, QStringList DC_rawDefs,
				QStringList *DC, QStringList *DCdescription);
	bool setupMBSWQueryAddrList(std::vector<MBSWmetadata_dt> MBSWmetaList);
	void assignMBSWRawData(QByteArray rawdata, unsigned int * mbswrawvalues);
	void processMBSWRawValues(unsigned int mbswrawvalues[SSMP_MAX_MBSW], QStringList *valueStrList, QStringList *unitStrList);
	bool validateVIN(char VIN[17]);

signals:
	void currentOrTemporaryDTCs(QStringList currentDTCs, QStringList currentDTCsDescriptions, bool testMode, bool DCheckActive);
	void historicOrMemorizedDTCs(QStringList historicDTCs, QStringList historicDTCsDescriptions);
	void latestCCCCs(QStringList currentCCCCs, QStringList currentCCCCsDescriptions);
	void memorizedCCCCs(QStringList historicCCCCs, QStringList historicCCCCsDescriptions);
	void newMBSWvalues(QStringList valueStrList, QStringList unitStrList, int duration_ms);
	void startedMBSWreading();
	void startedDCreading();
	void startedActuatorTest();
	void stoppedMBSWreading();
	void stoppedDCreading();
	void stoppedActuatorTest();
	void commError();

private slots:
	void processDCsRawdata(QByteArray dcrawdata, int duration_ms);
	void processMBSWrawData(QByteArray MBSWrawdata, int duration_ms);

public slots:
	void resetCUdata();

};



#endif

