/*
 * SSMprotocol.h - Application Layer for the new Subaru SSM protocol
 *
 * Copyright © 2008 Comer352l
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

#ifndef SSMPROTOCOL_H
#define SSMPROTOCOL_H



#ifdef __WIN32__
    #include "windows\serialCOM.h"
#elif defined __linux__
    #include "linux/serialCOM.h"
#else
    #error "Operating system not supported !"
#endif
#include <vector>
#include <math.h>
#include "SSMPcommunication.h"
#include "SSMprotocol_ID.h"
#include "SSMprotocol_def_en.h"
#include "SSMprotocol_def_de.h"




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



class SSMprotocol : public QObject, private SSMprotocol_ID
{
	Q_OBJECT

public:
	enum CUtype_dt {ECU=1, TCU=2};
	enum state_dt {state_needSetup=0, state_normal=1, state_DCreading=2, state_MBSWreading=3, state_ActTesting=4, state_waitingForIgnOff=5};
	enum DCgroups_dt {noDCs_DCgroup=0, temporaryDTCs_DCgroup=1, memorizedDTCs_DCgroup=2, CClatestCCs_DCgroup=4, CCmemorizedCCs_DCgroup=8, allDCs_DCgroup=15};
	enum CMlevel_dt {CMlevel_1=1, CMlevel_2=2};
	enum immoTestResult_dt {immoNotShorted, immoShortedToGround, immoShortedToBattery};

	SSMprotocol(serialCOM *port, CUtype_dt CU, QString language="en");
	~SSMprotocol();
	// NON-COMMUNICATION-FUNCTIONS:
	SSMprotocol::CUtype_dt CUtype();
	SSMprotocol::state_dt state();
	bool setupCUdata();
	bool getSysID(QString *SYS_ID);
	bool getROMID(QString *ROM_ID);
	bool getSystemDescription(QString *sysdescription);
	bool hasOBD2(bool *OBD2);
	bool hasVINsupport(bool *VINsup);
	bool hasImmobilizer(bool *ImmoSup);
	bool hasIntegratedCC(bool *CCsup);
	bool hasIntegratedCCmemorizedCodes(bool *CCmemCCsup);
	bool hasClearMemory2(bool *CM2sup);
	bool hasTestMode(bool *TMsup);
	bool hasActuatorTests(bool *ATsup);
	// COMMUNICATION BASED FUNCTIONS:
	bool getSupportedDCgroups(int *DCgroups);
	bool getLastDCgroupsSelection(int *DCgroups);
	bool getSupportedMBs(std::vector<mbsw_dt> *supportedMBs);
	bool getSupportedSWs(std::vector<mbsw_dt> *supportedSWs);
	bool getLastMBSWselection(std::vector<MBSWmetadata_dt> *MBSWmetaList);
	bool getSupportedAdjustments(std::vector<adjustment_dt> *supportedAdjustments);
	bool getSupportedActuatorTests(QStringList *actuatorTestTitles);
	bool getLastActuatorTestSelection(unsigned char *actuatorTestIndex);
	bool isEngineRunning(bool *isrunning);
	bool isInTestMode(bool *testmode);
	bool getVIN(QString *VIN);
	bool getAllAdjustmentValues(unsigned int * rawValues);
	bool getAdjustmentValue(unsigned char index, unsigned int *rawValue);
	bool setAdjustmentValue(unsigned char index, unsigned int rawValue);
	bool ClearMemory(CMlevel_dt level, bool *success);
	bool TestImmobilizerCommLine(immoTestResult_dt *result);
	bool stopAllActuators();
	bool startDCreading(int DCgroups, bool ignoreDCheckState = false);
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
	SSMPcommunication *_SSMPcom;
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
	std::vector<unsigned int> _temporaryDTCsAddr;
	std::vector<unsigned int> _memorizedDTCsAddr;
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
	bool _ignoreDCheckStateOnDCreading;
	std::vector<MBSWmetadata_dt> _MBSWmetaList;
	unsigned char _selectedActuatorTestIndex;
	// *** Temporary (operation related) data ***:
	unsigned int _selMBsSWaAddr[SSMP_MAX_MBSW];
	unsigned int _selMBsSWsAddrLen;

	// CU-FEATURES SETUP FUNCTIONS:
	void setupDTCaddresses(char flagbytes[96], unsigned char nrofflagbytes,
				std::vector<unsigned int> *temporaryDTCsAddr, std::vector<unsigned int> *memorizedDTCsAddr);
	void setupCCCCaddresses(std::vector<unsigned int> *latestCCCCsAddr, std::vector<unsigned int> *memorizedCCCCsAddr);
	void setupSupportedMBs(CUtype_dt CU, QString language, char flagbytes[96], unsigned char nrofflagbytes,
				std::vector<mb_intl_dt> *supportedMBs);
	void setupSupportedSWs(CUtype_dt CU, QString language, char flagbytes[96], unsigned char nrofflagbytes,
				std::vector<sw_intl_dt> *supportedSWs);
	void setupAdjustmentsData(CUtype_dt CU, QString language, char flagbytes[96], unsigned char nrofflagbytes,
				  std::vector<adjustment_intl_dt> *adjustments);
	void setupActuatorTestData(QString language, std::vector<actuator_dt> *actuators, std::vector<unsigned int> *allActByteAddr);
	// PREPARATION AND EVALUATION FUNCTIONS:
	void evaluateDCdataByte(unsigned int DCbyteadr, char DCrawdata, QStringList DC_rawDefs,
				 QStringList *DC, QStringList *DCdescription);
	bool setupMBSWQueryAddrList(std::vector<MBSWmetadata_dt> MBSWmetaList, 
				    std::vector<mb_intl_dt> supportedMBs, std::vector<sw_intl_dt> supportedSWs, 
				    unsigned int *mbswaddr, unsigned int *mbswaddrlen);
	void assignMBSWRawData(QByteArray rawdata, unsigned int mbswaddr[SSMP_MAX_MBSW], unsigned char mbswaddrlen,
				std::vector<MBSWmetadata_dt> MBSWmetaList, std::vector<mb_intl_dt> supportedMBs, 
				std::vector<sw_intl_dt> supportedSWs, unsigned int * mbswrawvalues);
	void processMBSWRawValues(unsigned int mbswrawvalues[SSMP_MAX_MBSW], std::vector<MBSWmetadata_dt> MBSWmetaList, 
				  std::vector<mb_intl_dt> supportedMBs, std::vector<sw_intl_dt> supportedSWs, 
				  QStringList *valueStrList, QStringList *unitStrList);
	bool scaleMB(unsigned int rawvalue, QString scaleformula, double *scaledvalue);
	bool validateVIN(char VIN[17]);
	void StrToHexstr(char *inputstr, unsigned int nrbytes, QString *hexstr);

signals:
	void temporaryDTCs(QStringList currentDTCs, QStringList currentDTCsDescriptions);
	void memorizedDTCs(QStringList historicDTCs, QStringList historicDTCsDescriptions);
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

