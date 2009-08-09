/*
 * SSMprotocol.h - Abstract application layer for the Subaru SSM protocols
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

#ifndef SSMPROTOCOL_H
#define SSMPROTOCOL_H



#ifdef __WIN32__
    #include "windows\serialCOM.h"
#elif defined __linux__
    #include "linux/serialCOM.h"
#else
    #error "Operating system not supported !"
#endif
#include <QString>
#include <QStringList>
#include <QObject>
#include <QEventLoop>
#include <string>
#include <vector>




#define SSMP_MAX_MB			300				/* currently 156 */
#define SSMP_MAX_SW			300				/* currently 154 */
#define SSMP_MAX_MBSW			(SSMP_MAX_MB + SSMP_MAX_SW)
#define SSMP_MAX_DTCADDR		100				/* currently 61/61	<256 ! */
#define SSMP_MAX_CCCCADDR		10				/* currently 4/4	<256 ! */
#define SSMP_MAX_ADJUSTMENTS		30				/* currently 24		<256 ! */



class dc_defs_dt
{
public:
	unsigned int byteAddr_currentOrTempOrLatest;
	unsigned int byteAddr_historicOrMemorized;
	QString code[8];
	QString title[8];
};
  
  
class  mbsw_dt
{
public:
	QString title;
	QString unit;
	QString scaleformula;
	char precision;
};


class  mb_intl_dt: public mbsw_dt
{
public:
	unsigned int adr_low;
	unsigned int adr_high;
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



class SSMprotocol : public QObject
{
	Q_OBJECT

public:
	enum CUtype_dt {CUtype_Engine, CUtype_Transmission, CUtype_CruiseControl, CUtype_AirCon, CUtype_FourWheelSteering};
	enum state_dt {state_needSetup=0, state_normal=1, state_DCreading=2, state_MBSWreading=3, state_ActTesting=4, state_waitingForIgnOff=5};
	enum DCgroups_dt {noDCs_DCgroup=0, currentDTCs_DCgroup=1, temporaryDTCs_DCgroup=2, historicDTCs_DCgroup=4, memorizedDTCs_DCgroup=8,
			  CClatestCCs_DCgroup=16, CCmemorizedCCs_DCgroup=32, allDCs_DCgroup=63};
	enum CMlevel_dt {CMlevel_1=1, CMlevel_2=2};
	enum immoTestResult_dt {immoNotShorted, immoShortedToGround, immoShortedToBattery};

	SSMprotocol(serialCOM *port, QString language="en");
	~SSMprotocol();
	// NON-COMMUNICATION-FUNCTIONS:
	bool CUtype(SSMprotocol::CUtype_dt *CU);
	SSMprotocol::state_dt state();
	virtual bool setupCUdata(CUtype_dt CU, bool ignoreIgnitionOFF=false) = 0;
	virtual std::string getSysID();
	virtual std::string getROMID() = 0;
	virtual bool getSystemDescription(QString *sysdescription) = 0;
	virtual bool hasOBD2system(bool *OBD2) = 0;
	virtual bool hasVINsupport(bool *VINsup) = 0;
	virtual bool hasImmobilizer(bool *ImmoSup) = 0;
	virtual bool hasIntegratedCC(bool *CCsup) = 0;
	virtual bool hasClearMemory2(bool *CM2sup) = 0;
	virtual bool hasTestMode(bool *TMsup) = 0;
	virtual bool hasActuatorTests(bool *ATsup) = 0;
	virtual bool getSupportedDCgroups(int *DCgroups) = 0;
	bool getLastDCgroupsSelection(int *DCgroups);
	bool getSupportedMBs(std::vector<mbsw_dt> *supportedMBs);
	bool getSupportedSWs(std::vector<mbsw_dt> *supportedSWs);
	bool getLastMBSWselection(std::vector<MBSWmetadata_dt> *MBSWmetaList);
	virtual bool getSupportedAdjustments(std::vector<adjustment_dt> *supportedAdjustments) = 0;
	virtual bool getSupportedActuatorTests(QStringList *actuatorTestTitles);
	virtual bool getLastActuatorTestSelection(unsigned char *actuatorTestIndex);
	// COMMUNICATION BASED FUNCTIONS:
	virtual bool isEngineRunning(bool *isrunning) = 0;
	virtual bool isInTestMode(bool *testmode);
	virtual bool getVIN(QString *VIN);
	virtual bool getAllAdjustmentValues(std::vector<unsigned int> *rawValues);
	virtual bool getAdjustmentValue(unsigned char index, unsigned int *rawValue);
	virtual bool setAdjustmentValue(unsigned char index, unsigned int rawValue);
	virtual bool clearMemory(CMlevel_dt level, bool *success) = 0;
	virtual bool testImmobilizerCommLine(immoTestResult_dt *result) = 0;
	virtual bool stopAllActuators();
	virtual bool startDCreading(int DCgroups) = 0;
	bool restartDCreading();
	virtual bool stopDCreading() = 0;
	virtual bool startMBSWreading(std::vector<MBSWmetadata_dt> mbswmetaList) = 0;
	bool restartMBSWreading();
	virtual bool stopMBSWreading() = 0;
	virtual bool startActuatorTest(unsigned char actuatorTestIndex);
	virtual bool restartActuatorTest();
	virtual bool stopActuatorTesting();
	bool stopAllPermanentOperations();
	virtual bool waitForIgnitionOff() = 0;

protected:
	serialCOM *_port;
	CUtype_dt _CU;
	state_dt _state;
	QString _language;
	// *** CONTROL UNIT BASIC DATA (SUPPORTED FEATURES) ***:
	// Diagnostic Trouble Codes:
	std::vector<dc_defs_dt> _DTCdefs;
	// Measuring Blocks and Switches:
	std::vector<mb_intl_dt> _supportedMBs;
	std::vector<sw_intl_dt> _supportedSWs;
	// *** Temporary (operation related) data ***:
	unsigned int _selMBsSWsAddr[SSMP_MAX_MBSW];
	unsigned int _selMBsSWsAddrLen;
	// *** Selection data ***:
	int _selectedDCgroups;
	std::vector<MBSWmetadata_dt> _MBSWmetaList;

	void evaluateDCdataByte(unsigned int DCbyteadr, char DCrawdata, std::vector<dc_defs_dt> DCdefs,
				QStringList *DC, QStringList *DCdescription);
	bool setupMBSWQueryAddrList(std::vector<MBSWmetadata_dt> MBSWmetaList);
	void assignMBSWRawData(QByteArray rawdata, std::vector<unsigned int> * mbswrawvalues);

signals:
	void currentOrTemporaryDTCs(QStringList currentDTCs, QStringList currentDTCsDescriptions, bool testMode, bool DCheckActive);
	void historicOrMemorizedDTCs(QStringList historicDTCs, QStringList historicDTCsDescriptions);
	void latestCCCCs(QStringList currentCCCCs, QStringList currentCCCCsDescriptions);
	void memorizedCCCCs(QStringList historicCCCCs, QStringList historicCCCCsDescriptions);
	void newMBSWrawValues(std::vector<unsigned int> rawValues, int duration_ms);
	void startedDCreading();
	void startedMBSWreading();
	void startedActuatorTest();
	void stoppedDCreading();
	void stoppedMBSWreading();
	void stoppedActuatorTest();
	void commError();

protected slots:
	void processMBSWrawData(QByteArray MBSWrawdata, int duration_ms);

public slots:
	virtual void resetCUdata() = 0;

};



#endif

