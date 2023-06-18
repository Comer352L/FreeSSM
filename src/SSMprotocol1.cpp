/*
 * SSMprotocol1.cpp - Application Layer for the old Subaru SSM protocol
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

#include "SSMprotocol1.h"

#include <QCoreApplication>
#include "libFSSM.h"


SSMprotocol1::SSMprotocol1(AbstractDiagInterface *diagInterface, QString language) : SSMprotocol(diagInterface, language)
{
	_SSMP1com = NULL;
	resetCUdata();
}


SSMprotocol1::~SSMprotocol1()
{
	resetCUdata();
}


void SSMprotocol1::resetCUdata()
{
	// RESET COMMUNICATION:
	if (_SSMP1com != NULL)
	{
		// Disconnect communication error and data signals:
		disconnect( _SSMP1com, SIGNAL( commError() ), this, SIGNAL( commError() ) );
		disconnect( _SSMP1com, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
		disconnect( _SSMP1com, SIGNAL( receivedData(const std::vector<char>&, int) ),
		            this, SLOT( processDCsRawdata(std::vector<char>, int) ) );
		disconnect( _SSMP1com, SIGNAL( receivedData(const std::vector<char>&, int) ),
		            this, SLOT( processMBSWrawData(const std::vector<char>&, int) ) );
		// Try to stop active communication processes:
		if (_SSMP1com->stopCommunication() && (_state == state_ActTesting))
		{
			bool ok = false;
			// Stop all actuator tests:
			for (unsigned int k=0; k<_allActByteAddr.size(); k++)
			{
				ok =_SSMP1com->writeAddress(_allActByteAddr.at(k), 0x00);
				if (!ok) break;
			}
			_state = state_needSetup;	// MUST BE DONE AFTER ALL CALLS OF MEMBER-FUNCTIONS AND BEFORE EMITTING SIGNALS
			if (ok)
				emit stoppedActuatorTest();
		}
		_state = state_needSetup;	// MUST BE DONE AFTER ALL CALLS OF MEMBER-FUNCTIONS AND BEFORE EMITTING SIGNALS
		delete _SSMP1com;
		_SSMP1com = NULL;
		// Emit stoppedXXX()-signals (_SSMP1com has been deleted, so we are sure they have finished):
		if (_state == state_MBSWreading)
		{
			emit stoppedMBSWreading();
		}
		else if (_state == state_DCreading)
		{
			emit stoppedDCreading();
		}
	}
	else
		_state = state_needSetup;	// MUST BE DONE AFTER ALL CALLS OF MEMBER-FUNCTIONS AND BEFORE EMITTING SIGNALS
	// Reset control unit data
	resetCommonCUdata();
}


SSMprotocol::CUsetupResult_dt SSMprotocol1::setupCUdata(enum CUtype CU)
{
	std::string LegacyDefsFile;
	SSM1_CUtype_dt SSM1_CU;
	// Reset:
	resetCUdata();
	// Update cached value of the active interface protocol:
	_ifceProtocol = _diagInterface->protocolType(); // NOTE: can't change until resetCUdata() has been called, because _SSMP1com has exclusive access to _diagInterface and doesn't change the protocol
	// Set control unit type and legacy definitions file:
	if (CU == CUtype::Engine)
	{
		SSM1_CU = SSM1_CU_Engine;
		LegacyDefsFile = QCoreApplication::applicationDirPath().toStdString() + "/definitions/SSM1defs_Engine.xml";
	}
	else if (CU == CUtype::Transmission)
	{
		SSM1_CU = SSM1_CU_Transmission;
		LegacyDefsFile = QCoreApplication::applicationDirPath().toStdString() + "/definitions/SSM1defs_Transmission.xml";
	}
	else if (CU == CUtype::CruiseControl)
	{
		SSM1_CU = SSM1_CU_CruiseCtrl;
		LegacyDefsFile = QCoreApplication::applicationDirPath().toStdString() + "/definitions/SSM1defs_CruiseControl.xml";
	}
	else if (CU == CUtype::AirCon)
	{
		SSM1_CU = SSM1_CU_AirCon;
		LegacyDefsFile = QCoreApplication::applicationDirPath().toStdString() + "/definitions/SSM1defs_AirConditioning.xml";
	}
	else if (CU == CUtype::FourWheelSteering)
	{
		SSM1_CU = SSM1_CU_FourWS;
		LegacyDefsFile = QCoreApplication::applicationDirPath().toStdString() + "/definitions/SSM1defs_FourWheelSteering.xml";
	}
	else if (CU == CUtype::ABS)
	{
		SSM1_CU = SSM1_CU_ABS;
		LegacyDefsFile = QCoreApplication::applicationDirPath().toStdString() + "/definitions/SSM1defs_ABS.xml";
	}
	else if (CU == CUtype::AirSuspension)
	{
		SSM1_CU = SSM1_CU_AirSusp;
		LegacyDefsFile = QCoreApplication::applicationDirPath().toStdString() + "/definitions/SSM1defs_AirSuspension.xml";
	}
	else if (CU == CUtype::PowerSteering)
	{
		SSM1_CU = SSM1_CU_PwrSteer;
		LegacyDefsFile = QCoreApplication::applicationDirPath().toStdString() + "/definitions/SSM1defs_PowerSteering.xml";
	}
	else
		return result_invalidCUtype;
	// Create SSMP1communication-object:
	_SSMP1com = new SSMP1communication(_diagInterface, SSM1_CU);
	// Get control unit ID:
	bool ok = _SSMP1com->getCUdata(0, _ssmCUdata);
	if (!ok && (CU == CUtype::AirCon))
	{
		_SSMP1com->selectCU(SSM1_CU_AirCon2);
		ok = _SSMP1com->getCUdata(0, _ssmCUdata);
	}
	if (!ok)
	{
		delete _SSMP1com;
		_SSMP1com = NULL;
		return result_commError;
	}
	/* NOTE: no need to check the state of the ignition switch.
	 * Power supply of these old control units is immediately cut when ignition is switched off */
	_CU = CU;
	_state = state_normal;
	// Connect communication error signals from SSMP1communication:
	connect( _SSMP1com, SIGNAL( commError() ), this, SIGNAL( commError() ) );
	connect( _SSMP1com, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
	/* Load control unit definitions */
	if (_ssmCUdata.uses_Flagbytes())
	{
		SSMFlagbyteDefinitionsInterface *FBdefsIface;
		// Request flag bytes:
		if (!_SSMP1com->getCUdata(32, _ssmCUdata))
		{
			resetCUdata();
			return result_commError;
		}
		// Read extended ID (5-byte ROM-ID):
		if (!readExtendedID(_ssmCUdata.ROM_ID))
		{
			resetCUdata();
			return result_commError;
		}
		// Check if we have definitions for this control unit:
		if (!_ssmCUdata.uses_Ax10xx_defs())
		{
			// NOTE: do not call resetCUdata(), because this will reset _state to state_needSetup which causes getSysID() + getROMID() to fail
			return result_noDefs;
		}
		/* Get definitions for this control unit */
		FBdefsIface = new SSMFlagbyteDefinitionsInterface(_language);
		if (!FBdefsIface->selectControlUnitID(_CU, _ssmCUdata))
		{
			delete FBdefsIface;
			// NOTE: do not call resetCUdata(), because this will reset _state to state_needSetup which causes getSysID() + getROMID() to fail
			return result_noDefs;
		}
		FBdefsIface->systemDescription(&_sysDescription);
		FBdefsIface->hasOBD2system(&_has_OBD2);
		FBdefsIface->hasImmobilizer(&_has_Immo);
		bool CMsup = false;
		FBdefsIface->hasClearMemory(&CMsup);
		if (CMsup)
			FBdefsIface->clearMemoryData(&_CMaddr, &_CMvalue);
		FBdefsIface->hasActuatorTests(&_has_ActTest);
		FBdefsIface->getDCblockData(&_DTCblockData);
		FBdefsIface->measuringBlocks(&_supportedMBs);
		FBdefsIface->MBdata_engineRunning(&_mb_enginespeed_data);
		FBdefsIface->switches(&_supportedSWs);
		FBdefsIface->SWdata_testModeState(&_sw_testmodestate_data);
		FBdefsIface->SWdata_DCheckState(&_sw_dcheckstate_data);
		FBdefsIface->SWdata_ignitionState(&_sw_ignitionstate_data);
		FBdefsIface->adjustments(&_adjustments);
		FBdefsIface->actuatorTests(&_actuators);
		_SSMdefsIfce = FBdefsIface;
		setupActuatorTestAddrList();
		// NOTE: all other SSM2-features (VIN, CC, CM2...) are not supported by SSM1 Control units or not required
	}
	else
	{
		/* Get definitions for this control unit */
		SSMLegacyDefinitionsInterface *LegacyDefsIface = new SSMLegacyDefinitionsInterface(_language);
		if (!LegacyDefsIface->selectDefinitionsFile(LegacyDefsFile))
		{
			delete LegacyDefsIface;
			// NOTE: do not call resetCUdata(), because this will reset _state to state_needSetup which causes getSysID() + getROMID() to fail
			return result_noOrInvalidDefsFile;
		}
		if (!LegacyDefsIface->selectID(_ssmCUdata.SYS_ID))
		{
			delete LegacyDefsIface;
			// NOTE: do not call resetCUdata(), because this will reset _state to state_needSetup which causes getSysID() + getROMID() to fail
			return result_noDefs;
		}
		std::string sysdescription;
		LegacyDefsIface->systemDescription(&sysdescription);
		_sysDescription = sysdescription;
		LegacyDefsIface->getDCblockData(&_DTCblockData);
		LegacyDefsIface->measuringBlocks(&_supportedMBs);
		LegacyDefsIface->MBdata_engineRunning(&_mb_enginespeed_data);
		LegacyDefsIface->switches(&_supportedSWs);
		LegacyDefsIface->SWdata_testModeState(&_sw_testmodestate_data);
		LegacyDefsIface->SWdata_DCheckState(&_sw_dcheckstate_data);
		LegacyDefsIface->SWdata_ignitionState(&_sw_ignitionstate_data);
		LegacyDefsIface->adjustments(&_adjustments);
		LegacyDefsIface->clearMemoryData(&_CMaddr, &_CMvalue);
		_SSMdefsIfce = LegacyDefsIface;
	}
	determineSupportedDCgroups(_DTCblockData);
	return result_success;
}


bool SSMprotocol1::startDCreading(int DCgroups)
{
	std::vector<unsigned int> DCqueryAddrList;
	bool started;

	// Check if another communication operation is in progress:
	if (_state != state_normal) return false;

	// Check argument:
	if ((DCgroups & _supportedDCgroups) != DCgroups)
		return false;

	// Setup diagnostic codes addresses list:
	for (unsigned int b = 0; b < _DTCblockData.size(); b++)
	{
		dc_block_dt dc_block = _DTCblockData.at(b);
		for (unsigned int a = 0; a < dc_block.addresses.size(); a++)
		{
			if ((dc_block.addresses.at(a).type == dc_addr_dt::Type::currentOrTempOrLatest)
			    && ((DCgroups & currentDTCs_DCgroup) || (DCgroups & temporaryDTCs_DCgroup)))
				DCqueryAddrList.push_back(dc_block.addresses.at(a).address);
			if ((dc_block.addresses.at(a).type == dc_addr_dt::Type::historicOrMemorized)
			    && ((DCgroups & historicDTCs_DCgroup) || (DCgroups & memorizedDTCs_DCgroup)))
				DCqueryAddrList.push_back(dc_block.addresses.at(a).address);
			// FIXME: futher types ?
		}
	}

	// Check if min. 1 address to read:
	if ((DCqueryAddrList.size() < 1))
		return false;

	// Add read address for test mode and D-Check status:
	if (((DCgroups & currentDTCs_DCgroup) || (DCgroups & temporaryDTCs_DCgroup))
	    && (_CU == CUtype::Engine) && _ssmCUdata.uses_Ax10xx_defs())
			DCqueryAddrList.insert(DCqueryAddrList.begin(), 0x000061); // NOTE: must be the first address !
			// FIXME: test mode and D-Check status addresses for other SSM1 control units

	// Start diagnostic code reading:
	started = _SSMP1com->readAddresses_permanent( DCqueryAddrList );
	if (started)
	{
		_state = state_DCreading;
		// Save diagnostic codes group selection (for data evaluation and restartDCreading()):
		_selectedDCgroups = DCgroups;
		// Connect signals and slots:
		connect( _SSMP1com, SIGNAL( receivedData(const std::vector<char>&, int) ),
		        this, SLOT( processDCsRawdata(std::vector<char>, int) ), Qt::BlockingQueuedConnection );
		// Emit signal:
		emit startedDCreading();
	}
	else
		resetCUdata();

	return started;
}


bool SSMprotocol1::stopDCreading()
{
	if ((_state == state_needSetup) || (_state == state_normal)) return true;
	if (_state == state_DCreading)
	{
		if (_SSMP1com->stopCommunication())
		{
			disconnect( _SSMP1com, SIGNAL( receivedData(const std::vector<char>&, int) ),
			            this, SLOT( processDCsRawdata(std::vector<char>, int) ) );
			_state = state_normal;
			emit stoppedDCreading();
			return true;
		}
		// Communication error:
		resetCUdata();
	}
	return false;
}


bool SSMprotocol1::startMBSWreading(const std::vector<MBSWmetadata_dt>& mbswmetaList)
{
	bool started = false;
	if (_state != state_normal) return false;
	// Setup list of MB/SW-addresses for SSM2Pcommunication:
	if (!setupMBSWQueryAddrList(mbswmetaList))
		return false;
	// Start MB/SW-reading:
	started = _SSMP1com->readAddresses_permanent( _selMBsSWsAddr );
	if (started)
	{
		_state = state_MBSWreading;
		// Save MB/SW-selection (necessary for evaluation of raw data):
		_MBSWmetaList = mbswmetaList;
		// Connect signals/slots:
		connect( _SSMP1com, SIGNAL( receivedData(const std::vector<char>&, int) ),
		         this, SLOT( processMBSWrawData(const std::vector<char>&, int) ) );
		// Emit signal:
		emit startedMBSWreading();
	}
	else
		resetCUdata();
	return started;
}


bool SSMprotocol1::stopMBSWreading()
{
	if ((_state == state_needSetup) || (_state == state_normal)) return true;
	if (_state == state_MBSWreading)
	{
		if (_SSMP1com->stopCommunication())
		{
			disconnect( _SSMP1com, SIGNAL( receivedData(const std::vector<char>&, int) ),
			            this, SLOT( processMBSWrawData(const std::vector<char>&, int) ) );
			_state = state_normal;
			emit stoppedMBSWreading();
			return true;
		}
		// Communication error:
		resetCUdata();
	}
	return false;
}


bool SSMprotocol1::getAdjustmentValue(unsigned char index, unsigned int *rawValue)
{
	std::vector<unsigned int> dataaddr;
	std::vector<char> data;
	if (_state != state_normal) return false;
	// Validate adjustment value selection:
	if (_adjustments.empty() || (index >= _adjustments.size())) return false;
	// Convert memory address into two byte addresses:
	dataaddr.push_back( _adjustments.at(index).addrLow );
	if (_adjustments.at(index).addrHigh > 0)
		dataaddr.push_back( _adjustments.at(index).addrHigh );
	// Read data from control unit:
	if (!_SSMP1com->readAddresses(dataaddr, &data))
	{
		resetCUdata();
		return false;
	}
	// Calculate raw value:
	*rawValue = static_cast<unsigned char>(data.at(0));
	if (_adjustments.at(index).addrHigh > 0)
		*rawValue += 256*static_cast<unsigned char>(data.at(1));
	return true;
}


bool SSMprotocol1::getAllAdjustmentValues(std::vector<unsigned int> * rawValues)
{
	std::vector<unsigned int> dataaddr;
	std::vector<char> data;
	unsigned char k = 0;
	unsigned int addrindex = 0;
	if ((_state != state_normal) || _adjustments.empty()) return false;
	// Setup address list:
	for (k=0; k<_adjustments.size(); k++)
	{
		dataaddr.push_back( _adjustments.at(k).addrLow );
		if (_adjustments.at(k).addrHigh > 0)
			dataaddr.push_back( _adjustments.at(k).addrHigh );
	}
	// Read data from control unit:
	data.resize( dataaddr.size() );
	if (!_SSMP1com->readAddresses(dataaddr, &data))
	{
		resetCUdata();
		return false;
	}
	// Calculate and return raw values:
	rawValues->clear();
	for (k=0; k<_adjustments.size(); k++)
	{
		rawValues->push_back( static_cast<unsigned char>(data.at(addrindex)) );
		addrindex++;
		if (_adjustments.at(k).addrHigh > 0)
		{
			rawValues->at(k) += 256*static_cast<unsigned char>(data.at(addrindex));
			addrindex++;
		}
	}
	return true;
}


bool SSMprotocol1::setAdjustmentValue(unsigned char index, unsigned int rawValue)
{
	std::vector<unsigned int> addresses;
	std::vector<char> data;
	if (_state != state_normal) return false;
	// Validate adjustment value selection:
	if (index >= _adjustments.size()) return false;
	if ((_adjustments.at(index).rawMin <= _adjustments.at(index).rawMax) && ((rawValue < _adjustments.at(index).rawMin) || ((rawValue > _adjustments.at(index).rawMax))))
		return false;
	if ((_adjustments.at(index).rawMin > _adjustments.at(index).rawMax) && (rawValue < _adjustments.at(index).rawMin) && (rawValue > _adjustments.at(index).rawMax))
		return false;
	if ((_adjustments.at(index).addrHigh > 0) && (rawValue > 65535))
	{
		return false;
	}
	else if (rawValue > 255)
	{
		return false;
	}
	// Setup addresses and convert raw value to 2 byte values:
	addresses.push_back( _adjustments.at(index).addrLow );
	data.push_back( static_cast<char>(rawValue & 0xff) );
	if (_adjustments.at(index).addrHigh > 0)
	{
		addresses.push_back( _adjustments.at(index).addrHigh );
		data.push_back( static_cast<char>((rawValue & 0xffff) >> 8) );
	}
	// Write value to control unit:
	if (!_SSMP1com->writeAddresses(addresses, data))
	{
		resetCUdata();
		return false;
	}
	return true;
}


bool SSMprotocol1::startActuatorTest(unsigned char actuatorTestIndex)
{
	bool ATstarted = false;
	bool ok = false;
	bool testmode = false;
	bool running = false;
	unsigned char k = 0;
	// Check if another communication operation is in progress:
	if (_state != state_normal) return false;
	// Validate selected test:
	if (!_has_ActTest || (actuatorTestIndex >= _actuators.size()))
		return false;
	// Check if control unit is in test mode:
	ok = isInTestMode(&testmode);
	if (!ok || !testmode)
		return false;
	// Check that engine is not running:
	ok = isEngineRunning(&running);
	if (!ok || running)
		return false;
	// Change state:
	_state = state_ActTesting;
	// Prepare test addresses:
	const unsigned int dataaddr = _actuators.at(actuatorTestIndex).byteAddr;
	const char databyte = static_cast<char>(1 << (_actuators.at(actuatorTestIndex).bitAddr - 1));
	// Stop all actuator tests:
	for (k=0; k<_allActByteAddr.size(); k++)
	{
		if (!_SSMP1com->writeAddress(_allActByteAddr.at(k), 0x00))
		{
			_state = state_normal; // this avoids that resetCUdata() will try to stop all actuators again
			resetCUdata();
			return false;
		}
	}
	// Start Actuator Test:
	ATstarted = _SSMP1com->writeAddress_permanent(dataaddr, databyte, 100);
	if (ATstarted)
	{
		_selectedActuatorTestIndex = actuatorTestIndex;
		emit startedActuatorTest();
	}
	else
	{
		_state = state_normal; // this avoids that resetCUdata() will try to stop all actuators again
		resetCUdata();
	}
	return ATstarted;
}


bool SSMprotocol1::stopActuatorTesting()
{
	unsigned char k = 0;
	if ((_state == state_needSetup) || (_state == state_normal)) return true;
	if (_state == state_ActTesting)
	{
		if (_SSMP1com->stopCommunication())
		{
			// Stop all actuator tests:
			for (k=0; k<_allActByteAddr.size(); k++)
			{
				if (!_SSMP1com->writeAddress(_allActByteAddr.at(k), 0x00))
				{
					resetCUdata();
					return false;
				}
			}
			_state = state_normal;
			emit stoppedActuatorTest();
			return true;
		}
		// Communication error:
		resetCUdata();
	}
	return false;
}


bool SSMprotocol1::stopAllActuators()
{
	// NOTE: This function can be called even if no actuator test has been started with SSMprotocol
	// => When switching the cars ignition on (with engine off) while test mode connector is connected,
	//    some actuator tests are started automatically
	bool ok = false;
	bool testmode = false;
	bool enginerunning = false;
	if (_state != state_normal) return false;
	// Check if actuator tests are supported:
	if (!_has_ActTest)
		return false;
	// Check if control unit is in test mode:
	ok = isInTestMode(&testmode);
	if (!ok || !testmode)
		return false;
	// Check that engine is not running:
	ok = isEngineRunning(&enginerunning);
	if (!ok || enginerunning)
		return false;
	// Stop all actuator tests:
	for (unsigned char k=0; k<_allActByteAddr.size(); k++)
	{
		if (!_SSMP1com->writeAddress(_allActByteAddr.at(k), 0x00))
		{
			resetCUdata();
			return false;
		}
	}
	return true;
}


bool SSMprotocol1::clearMemory(CMlevel_dt level, bool *success)
{
	bool CMsup = false;
	char bytewritten = 0;

	if (success != NULL)
		*success = false;
	// NOTE: _state validated by hasClearMemory()
	if (level != CMlevel_1) return false;
	if (!hasClearMemory(&CMsup) || !CMsup)
		return false;
	if (!_SSMP1com->writeAddress(_CMaddr, _CMvalue, &bytewritten))
	{
		resetCUdata();
		return false;
	}
	if (success != NULL)
		*success = (bytewritten == _CMvalue);
	return true;
}


bool SSMprotocol1::testImmobilizerCommLine(immoTestResult_dt *result)
{
	if (_state != state_normal) return false;
	if (!_ssmCUdata.uses_Ax10xx_defs())
		return false;
	if (!_has_Immo) return false;
	char checkvalue = 0;
	unsigned int readcheckaddr = 0x8B;
	// Write test-pattern:
	if (_SSMP1com->writeAddress(0xE0, '\xAA', &checkvalue))
	{
		// Read result:
		if (_SSMP1com->readAddress(readcheckaddr, &checkvalue))
		{
			/* NOTE: the actually written data is NOT 0xAA ! */
			if (checkvalue == '\x01')
			{
				*result = immoShortedToGround;
			}
			else if  (checkvalue == '\x02')
			{
				*result = immoShortedToBattery;
			}
			else
			{
				*result = immoNotShorted;
			}
			return true;
		}
	}
	// Communication error:
	resetCUdata();
	return false;
	// TODO: Immobilizer test might work different for 7xxxxx and Ax01xx controllers (if supported) !
}


bool SSMprotocol1::isEngineRunning(bool *isrunning)
{
	std::vector<unsigned int> addresses;
	std::vector<char> data;
	unsigned int raw_value = 0;
	QString scaledValueStr;
	bool ok = false;
	long int rpm = 0;

	if (_state != state_normal)
		return false;
	if (_mb_enginespeed_data.addr_low == MEMORY_ADDRESS_NONE)
		return false;
	addresses.push_back(_mb_enginespeed_data.addr_low);
	if (_mb_enginespeed_data.addr_high != MEMORY_ADDRESS_NONE)
		addresses.push_back(_mb_enginespeed_data.addr_high);
	if (!_SSMP1com->readAddresses(addresses, &data))
	{
		resetCUdata();
		return false;
	}
	raw_value = data.at(0);
	if (data.size() > 1)
		raw_value |= (data.at(1) << 8);
	if (!libFSSM::raw2scaled(raw_value, _mb_enginespeed_data.scaling_formula, 0, &scaledValueStr))
	{
		resetCUdata();
		return false;
	}
	rpm = scaledValueStr.toLong(&ok);
	if (!ok)
	{
		resetCUdata();
		return false;
	}
	if (rpm > 100) // NOTE: some MBs do not report 0 if the engine is off
		*isrunning = true;
	else
		*isrunning = false;
	return true;
}


bool SSMprotocol1::isInTestMode(bool *testmode)
{
	char byte = 0;

	if (_state != state_normal)
		return false;
	if (_sw_testmodestate_data.addr == MEMORY_ADDRESS_NONE)
		return false;
	if (!_SSMP1com->readAddress(_sw_testmodestate_data.addr, &byte))
	{
		resetCUdata();
		return false;
	}
	if ((byte & (1 << _sw_testmodestate_data.bit)) ^ _sw_testmodestate_data.inverted)
		*testmode = true;
	else
		*testmode = false;
	return true;
}


bool SSMprotocol1::waitForIgnitionOff()
{
	if (_state != state_normal)
		return false;
	unsigned int dataaddr;
	_state = state_waitingForIgnOff;
	_SSMP1com->setRetriesOnError(1);
	if (_sw_ignitionstate_data.addr != MEMORY_ADDRESS_NONE)
	{
		bool ignstate = true;
		char data = 0x00;
		do
		{
			if (!_SSMP1com->readAddress(_sw_ignitionstate_data.addr, &data))

				ignstate = false;
			else
				ignstate = ((data & (1 << _sw_ignitionstate_data.bit)) ^ _sw_ignitionstate_data.inverted);
		} while (ignstate);
	}
	else
	{
		dataaddr = 0x0000;
		QEventLoop el;
		disconnect( _SSMP1com, SIGNAL( commError() ), this, SIGNAL( commError() ) );
		disconnect( _SSMP1com, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
		if(!_SSMP1com->readAddress_permanent(dataaddr))
		{
			resetCUdata();
			return false;
		}
		connect(_SSMP1com, SIGNAL( commError() ), &el, SLOT( quit() ));
		el.exec();
		disconnect(_SSMP1com, SIGNAL( commError() ), &el, SLOT( quit() ));
	}
	_SSMP1com->setRetriesOnError(2);
	resetCUdata();
	return true;
/* NOTE: temporary solution, will become obsolete with extended SSMP1communication */
}

// PRIVATE

bool SSMprotocol1::readExtendedID(std::vector<char>& ID)
{
	std::vector<unsigned int> addresses;
	for (unsigned int addr=0x01; addr<=0x05; addr++)
		addresses.push_back(addr);
	if (!_SSMP1com->readAddresses(addresses, &ID))
		return false;
	return true;
}

