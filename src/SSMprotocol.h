/*
 * SSMprotocol.h - Abstract application layer for the Subaru SSM protocols
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

#ifndef SSMPROTOCOL_H
#define SSMPROTOCOL_H



#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QObject>
#include <QEventLoop>
#include <string>
#include <vector>
#include "AbstractDiagInterface.h"
#include "libFSSM.h"
#include "SSMCUdata.h"
#include "AbstractSSMcommunication.h"
#include "SSMDefinitionsInterface.h"



enum class BlockType { MB, SW };


class MBSWmetadata_dt
{
public:
	BlockType blockType;
	unsigned int nativeIndex;

	bool operator == (const MBSWmetadata_dt &a) const
	{
		return ( (a.blockType==blockType) && (a.nativeIndex==nativeIndex) );
	}
};


class SSMprotocol : public QObject
{
	Q_OBJECT

public:
	enum protocol_dt {SSM1, SSM2};
	enum CUsetupResult_dt {result_success, result_invalidCUtype, result_invalidInterfaceConfig, result_commError, result_noOrInvalidDefsFile, result_noDefs};
	enum state_dt {state_needSetup, state_normal, state_DCreading, state_MBSWreading, state_ActTesting, state_waitingForIgnOff};
	enum DCgroups_dt {noDCs_DCgroup=0, currentDTCs_DCgroup=1, temporaryDTCs_DCgroup=2, historicDTCs_DCgroup=4, memorizedDTCs_DCgroup=8,
	                  CClatestCCs_DCgroup=16, CCmemorizedCCs_DCgroup=32};
	enum CMlevel_dt {CMlevel_1=1, CMlevel_2=2};
	enum immoTestResult_dt {immoNotShorted, immoShortedToGround, immoShortedToBattery};

	SSMprotocol(AbstractDiagInterface *diagInterface, QString language="en");
	virtual ~SSMprotocol();
	// NON-COMMUNICATION-FUNCTIONS:
	bool CUtype(CUtype *CU);
	state_dt state();
	virtual CUsetupResult_dt setupCUdata(enum CUtype CU) = 0;
	virtual protocol_dt protocolType() = 0;
	AbstractDiagInterface::protocol_type ifceProtocolType();
	std::string getSysID() const;
	std::string getROMID() const;
	bool getSystemDescription(QString *sysdescription);
	bool hasOBD2system(bool *OBD2);
	virtual bool hasVINsupport(bool *VINsup);
	bool hasImmobilizer(bool *ImmoSup);
	bool hasImmobilizerTest(bool *ImmoTestSup);
	virtual bool hasIntegratedCC(bool *CCsup);
	virtual bool hasClearMemory(bool *CMsup);
	virtual bool hasClearMemory2(bool *CM2sup);
	bool hasTestMode(bool *TMsup);
	bool hasActuatorTests(bool *ATsup);
	bool getSupportedDCgroups(int *DCgroups);
	bool getLastDCgroupsSelection(int *DCgroups);
	bool getSupportedMBs(std::vector<mb_dt> *supportedMBs);
	bool getSupportedSWs(std::vector<sw_dt> *supportedSWs);
	bool getLastMBSWselection(std::vector<MBSWmetadata_dt> *MBSWmetaList);
	bool getSupportedAdjustments(std::vector<adjustment_dt> *supportedAdjustments);
	bool getSupportedActuatorTests(QStringList *actuatorTestTitles);
	bool getLastActuatorTestSelection(unsigned char *actuatorTestIndex);
	// COMMUNICATION BASED FUNCTIONS:
	virtual bool getVIN(QString *VIN);
	bool startDCreading(int DCgroups);
	bool restartDCreading();
	virtual bool stopDCreading() = 0;
	virtual bool startMBSWreading(const std::vector<MBSWmetadata_dt>& mbswmetaList) = 0;
	bool restartMBSWreading();
	virtual bool stopMBSWreading() = 0;
	virtual bool getAdjustmentValue(unsigned char index, unsigned int *rawValue) = 0;
	virtual bool getAllAdjustmentValues(std::vector<unsigned int> *rawValues) = 0;
	virtual bool setAdjustmentValue(unsigned char index, unsigned int rawValue) = 0;
	virtual bool startActuatorTest(unsigned char actuatorTestIndex) = 0;
	bool restartActuatorTest();
	virtual bool stopActuatorTesting() = 0;
	virtual bool stopAllActuators() = 0;
	virtual bool clearMemory(CMlevel_dt level, bool *success) = 0;
	virtual bool testImmobilizerCommLine(immoTestResult_dt *result) = 0;
	virtual bool isEngineRunning(bool *isrunning) = 0;
	virtual bool isInTestMode(bool *testmode) = 0;
	bool stopAllPermanentOperations();
	virtual bool waitForIgnitionOff() = 0;

protected:
	AbstractDiagInterface *_diagInterface;
	AbstractDiagInterface::protocol_type _ifceProtocol;
	enum CUtype _CU;
	state_dt _state;
	QString _language;
	// *** CONTROL UNIT RAW DATA ***:
	SSMCUdata _ssmCUdata;
	std::string _sysDescription;
	// *** COMMUNICATION OBJECT ***
	AbstractSSMcommunication *_SSMPcom;
	// *** CONTROL UNIT DEFINITION INTERFACE ***:
	SSMDefinitionsInterface *_SSMdefsIfce;
	// *** CONTROL UNIT BASIC DATA (SUPPORTED FEATURES) ***:
	bool _has_OBD2;
	bool _has_Immo;
	bool _has_ImmoTest;
	bool _has_ActTest;
	bool _has_SW_ignition;
	// Diagnostic Trouble Codes:
	std::vector<dc_block_dt> _DTCblockData;
	int _supportedDCgroups;
	// Clear Memory:
	unsigned int _CMaddr = MEMORY_ADDRESS_NONE;
	char _CMvalue;
	// Measuring Blocks and Switches:
	std::vector<mb_intl_dt> _supportedMBs;
	std::vector<sw_intl_dt> _supportedSWs;
	mb_enginespeed_data_dt _mb_enginespeed_data;
	sw_stateindication_data_dt _sw_testmodestate_data;
	sw_stateindication_data_dt _sw_dcheckstate_data;
	sw_stateindication_data_dt _sw_ignitionstate_data;
	// Adjustment Values:
	std::vector<adjustment_intl_dt> _adjustments;
	// Actuator Tests:
	std::vector<actuator_dt> _actuators;
	std::vector<unsigned int> _allActByteAddr;
	// *** Selection data (temporary) ***:
	int _selectedDCgroups;
	std::vector<MBSWmetadata_dt> _MBSWmetaList;
	std::vector<unsigned int> _selMBsSWsAddr;
	unsigned char _selectedActuatorTestIndex;

	bool setupMBSWQueryAddrList(std::vector<MBSWmetadata_dt> MBSWmetaList);
	std::vector<unsigned int> assignMBSWRawData(const std::vector<char>& rawdata);
	void setupActuatorTestAddrList();
	void determineSupportedDCgroups(std::vector<dc_block_dt> DCblockData);
	void resetCommonCUdata();

signals:
	void currentOrTemporaryDTCs(QStringList currentDTCs, QStringList currentDTCsDescriptions, bool testMode, bool DCheckActive);
	void historicOrMemorizedDTCs(QStringList historicDTCs, QStringList historicDTCsDescriptions);
	void latestCCCCs(QStringList currentCCCCs, QStringList currentCCCCsDescriptions);
	void memorizedCCCCs(QStringList historicCCCCs, QStringList historicCCCCsDescriptions);
	void newMBSWrawValues(const std::vector<unsigned int>& rawValues, int duration_ms);
	void startedDCreading();
	void startedMBSWreading();
	void startedActuatorTest();
	void stoppedDCreading();
	void stoppedMBSWreading();
	void stoppedActuatorTest();
	void commError();

protected slots:
	void processMBSWrawData(const std::vector<char>& MBSWrawdata, int duration_ms);
	void processDCsRawdata(std::vector<char> dcrawdata, int duration_ms);

public slots:
	virtual void resetCUdata() = 0;

};



#endif

