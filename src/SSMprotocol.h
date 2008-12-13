#ifndef SSMPROTOCOL_H
#define SSMPROTOCOL_H



#ifdef __WIN32__
    #include "windows\serialCOM.h"
#elif defined __linux__
    #include "linux/serialCOM.h"
#else
    #error "Operating system not supported !"
#endif
#include "math.h"
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
#define SSMP_MAX_ACTTESTS		30				/* currently 21		<256 ! */
#define SSMP_MAX_ACTBYTEADDR		10				/* currently 3		<256 ! */




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
	bool getSupportedMBs(unsigned int *nrofsupportedMBs, mbsw_dt *supportedMBs = NULL);
	bool getLastMBSWselection(MBSWmetadata_dt MBSWmetaList[SSMP_MAX_MBSW], unsigned int *MBSWmetaList_len);
	bool getSupportedSWs(unsigned int *nrofsupportedSWs, mbsw_dt *supportedSWs = NULL);
	bool getSupportedAdjustments(adjustment_dt *supportedAdjustments, unsigned char *nrofsupportedAdjustments);
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
	bool startMBSWreading(MBSWmetadata_dt mbswmetaList[SSMP_MAX_MBSW], unsigned int mbswmetaList_len);
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
	unsigned int _temporaryDTCsAddr[SSMP_MAX_DTCADDR];
	unsigned int _memorizedDTCsAddr[SSMP_MAX_DTCADDR];
	unsigned char _nrofDTCsAddr;
	QStringList _DC_rawDefs;
	// Cruise Control Cancel Codes:
	unsigned int _latestCCCCsAddr[SSMP_MAX_CCCCADDR];
	unsigned char _nrofLatestCCCCsAddr;
	unsigned int _memorizedCCCCsAddr[SSMP_MAX_CCCCADDR];
	unsigned char _nrofMemorizedCCCCsAddr;
	QStringList _CC_rawDefs;
	// Measuring Blocks and Switches:
	mb_intl_dt _supportedMBs[SSMP_MAX_MB];
	sw_intl_dt _supportedSWs[SSMP_MAX_SW];
	unsigned int _nrofsupportedMBs;
	unsigned int _nrofsupportedSWs;
	// Adjustment Values:
	adjustment_intl_dt _adjustments[SSMP_MAX_ADJUSTMENTS];
	unsigned char _nrofAdjustments;
	// Actuator Tests:
	actuator_dt _actuators[SSMP_MAX_ACTTESTS];
	unsigned char _nrofActuators;
	unsigned int _allActByteAddr[SSMP_MAX_ACTBYTEADDR];
	unsigned char _nrofAllActByteAddr;
	// *** Selection data ***:
	int _selectedDCgroups;
	bool _ignoreDCheckStateOnDCreading;
	MBSWmetadata_dt _MBSWmetaList[SSMP_MAX_MBSW];
	unsigned int _MBSWmetaList_len;
	unsigned char _selectedActuatorTestIndex;
	// *** Temporary (operation related) data ***:
	unsigned int _selMBsSWaAddr[SSMP_MAX_MBSW];
	unsigned int _selMBsSWsAddrLen;

	// CU-FEATURES SETUP FUNCTIONS:
	void setupDTCaddresses(char flagbytes[96], unsigned char nrofflagbytes,
				unsigned int *temporaryDTCsAddr, unsigned int *memorizedDTCsAddr,
				unsigned char *nrofDTCsAddr);
	void setupCCCCaddresses(unsigned int *latestCCCCsAddr, unsigned char *nrofLatestCCCCsAddr,
				unsigned int *memorizedCCCCsAddr, unsigned char *nrofMemorizedCCCCsAddr);
	void setupSupportedMBs(CUtype_dt CU, QString language, char flagbytes[96], unsigned char nrofflagbytes,
				mb_intl_dt *supportedMBs, unsigned int *nrofsupportedMBs);
	void setupSupportedSWs(CUtype_dt CU, QString language, char flagbytes[96], unsigned char nrofflagbytes,
				sw_intl_dt *supportedSWs, unsigned int *nrofsupportedSWs);
	void setupAdjustmentsData(CUtype_dt CU, QString language, char flagbytes[96], unsigned char nrofflagbytes,
				adjustment_intl_dt *adjustments, unsigned char *nrofAdjustments);
	void setupActuatorTestData(QString language, actuator_dt *actuators, unsigned char *nrofActuators,
				   unsigned int *allActByteAddr, unsigned char *nrofAllActByteAddr);
	// PREPARATION AND EVALUATION FUNCTIONS:
	void evaluateDTCDataByte(unsigned int DTCbyteadr, char DTCrawdata,
				 QStringList *DTC, QStringList *DTCdescription);
	void evaluateCCCCDataByte(unsigned int CCbyteadr, char CCrawdata,
				  QStringList *CC, QStringList *CCdescription);
	bool setupMBSWQueryAddrList(MBSWmetadata_dt MBSWmetaList[SSMP_MAX_MBSW], unsigned int MBSWmetaList_len, 
				    mb_intl_dt supportedMBs[SSMP_MAX_MB], unsigned int nrofsupportedMBs,
				    sw_intl_dt supportedSWs[SSMP_MAX_SW], unsigned int nrofsupportedSWs,
				    unsigned int *mbswaddr, unsigned int *mbswaddrlen);
	void assignMBSWRawData(QByteArray rawdata, unsigned int mbswaddr[SSMP_MAX_MBSW], unsigned char mbswaddrlen,
				MBSWmetadata_dt MBSWmetaList[SSMP_MAX_MBSW], unsigned int MBSWmetaList_len,
				mb_intl_dt supportedMBs[SSMP_MAX_MB], sw_intl_dt supportedSWs[SSMP_MAX_SW],
				unsigned int * mbswrawvalues);
	void processMBSWRawValues(unsigned int mbswrawvalues[SSMP_MAX_MBSW],
				  MBSWmetadata_dt MBSWmetaList[SSMP_MAX_MBSW], unsigned int MBSWmetaList_len,
				  mb_intl_dt supportedMBs[SSMP_MAX_MB], sw_intl_dt supportedSWs[SSMP_MAX_SW],
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

