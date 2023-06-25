/*
 * SSMprotocol.cpp - Abstract application layer for the Subaru SSM protocols
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

#include "SSMprotocol.h"



SSMprotocol::SSMprotocol(AbstractDiagInterface *diagInterface, QString language)
{
	_diagInterface = diagInterface;
	_ifceProtocol = diagInterface->protocolType(); // NOTE: may change later
	_language = language;
	_CU = CUtype::Engine;
	_state = state_needSetup;
	_SSMPcom = NULL;
	_SSMdefsIfce = NULL;
	resetCUdata();
	qRegisterMetaType< std::vector<char> >("std::vector<char>");
}


SSMprotocol::~SSMprotocol()
{
}


void SSMprotocol::resetCUdata()
{
	// RESET COMMUNICATION:
	if (_SSMPcom != NULL)
	{
		// Disconnect communication error and data signals:
		disconnect( _SSMPcom, SIGNAL( commError() ), this, SIGNAL( commError() ) );
		disconnect( _SSMPcom, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
		disconnect( _SSMPcom, SIGNAL( receivedData(const std::vector<char>&, int) ),
		            this, SLOT( processDCsRawdata(std::vector<char>, int) ) );
		disconnect( _SSMPcom, SIGNAL( receivedData(const std::vector<char>&, int) ),
		            this, SLOT( processMBSWrawData(const std::vector<char>&, int) ) );
		// Try to stop active communication processes:
		if (_SSMPcom->stopCommunication() && (_state == state_ActTesting))
		{
			bool ok = false;
			// Stop all actuator tests:
			for (unsigned int k = 0; k < _allActByteAddr.size(); k++)
			{
				ok =_SSMPcom->writeAddress(_allActByteAddr.at(k), 0x00);
				if (!ok)
					break;
			}
			_state = state_needSetup;	// MUST BE DONE AFTER ALL CALLS OF MEMBER-FUNCTIONS AND BEFORE EMITTING SIGNALS
			if (ok)
				emit stoppedActuatorTest();
		}
		_state = state_needSetup;	// MUST BE DONE AFTER ALL CALLS OF MEMBER-FUNCTIONS AND BEFORE EMITTING SIGNALS
		delete _SSMPcom;
		_SSMPcom = NULL;
		// Emit stoppedXXX()-signals (_SSMP1com has been deleted, so we are sure they have finished):
		if (_state == state_MBSWreading)
			emit stoppedMBSWreading();
		else if (_state == state_DCreading)
			emit stoppedDCreading();
	}
	else
		_state = state_needSetup;	// MUST BE DONE AFTER ALL CALLS OF MEMBER-FUNCTIONS AND BEFORE EMITTING SIGNALS
	// RESET ECU DATA:
	_ssmCUdata.clear();
	// Clear system description:
	_sysDescription.clear();
	_has_OBD2 = false;
	_has_Immo = false;
	_has_ImmoTest = false;
	_has_ActTest = false;
	_has_SW_ignition = false;
	// Clear DC data:
	_DTCblockData.clear();
	_supportedDCgroups = noDCs_DCgroup;
	// Clear Clear Memory data:
	_CMaddr = MEMORY_ADDRESS_NONE;
	_CMvalue = '\x00';
	_CM2addr = MEMORY_ADDRESS_NONE;
	_CM2value = '\x00';
	// Clear MB/SW data:
	_supportedMBs.clear();
	_supportedSWs.clear();
	_mb_enginespeed_data = mb_enginespeed_data_dt();
	_sw_testmodestate_data = sw_stateindication_data_dt();
	_sw_dcheckstate_data = sw_stateindication_data_dt();
	_sw_ignitionstate_data = sw_stateindication_data_dt();
	// Clear adjustment values data:
	_adjustments.clear();
	// Clear actuator tests data:
	_actuators.clear();
	_allActByteAddr.clear();
	// Clear selection data:
	_selectedDCgroups = noDCs_DCgroup;
	_MBSWmetaList.clear();
	_selMBsSWsAddr.clear();
	_selectedActuatorTestIndex = 255; // index ! => 0=first actuator !
	// Destruct definitions interface:
	if (_SSMdefsIfce != NULL)
	{
		delete _SSMdefsIfce;
		_SSMdefsIfce = NULL;
	}
}


bool SSMprotocol::CUtype(enum CUtype *CU)
{
	if (_state == state_needSetup)
		return false;
	*CU = _CU;
	return true;
}


SSMprotocol::state_dt SSMprotocol::state()
{
	return _state;
}


AbstractDiagInterface::protocol_type SSMprotocol::ifceProtocolType()
{
	if (_state == state_needSetup)
		_ifceProtocol = _diagInterface->protocolType();
	// else: NOTE: we must not access _diagInterface, because SSMP[1/2]communication has exclusive access, which also means the protocol can't change
	return _ifceProtocol;
}


std::string SSMprotocol::getSysID() const
{
	if (_state == state_needSetup)
		return "";
	return libFSSM::StrToHexstr(_ssmCUdata.SYS_ID);
}


std::string SSMprotocol::getROMID() const
{
	if (_state == state_needSetup)
		return "";
	if (_ssmCUdata.uses_Flagbytes())
		return libFSSM::StrToHexstr(_ssmCUdata.ROM_ID);
	else
		return getSysID();
}


bool SSMprotocol::getSystemDescription(QString *sysdescription)
{
	if (_state == state_needSetup)
		return false;
	if (_sysDescription.size())
	{
		*sysdescription = QString::fromStdString(_sysDescription);
		return true;
	}
	return false;
}


bool SSMprotocol::hasOBD2system(bool *OBD2)
{
	if (_state == state_needSetup)
		return false;
	*OBD2 = _has_OBD2;
	return true;
}


bool SSMprotocol::hasVINsupport(bool *VINsup)
{
	if (_state == state_needSetup)
		return false;
	*VINsup = false;
	return true;
}


bool SSMprotocol::hasImmobilizer(bool *ImmoSup)
{
	if (_state == state_needSetup)
		return false;
	*ImmoSup = _has_Immo;
	return true;
}


bool SSMprotocol::hasImmobilizerTest(bool *ImmoTestSup)
{
	if (_state == state_needSetup)
		return false;
	*ImmoTestSup = _has_ImmoTest;
	return true;
}


bool SSMprotocol::hasIntegratedCC(bool *CCsup)
{
	if (_state == state_needSetup)
		return false;
	*CCsup = false;
	return true;
}


bool SSMprotocol::hasClearMemory(bool *CMsup)
{
	if (_state == state_needSetup)
		return false;
	*CMsup = (_CMaddr != MEMORY_ADDRESS_NONE);
	return true;
}


bool SSMprotocol::hasClearMemory2(bool *CM2sup)
{
	if (_state == state_needSetup)
		return false;
	*CM2sup = false;
	return true;
}


bool SSMprotocol::hasMBengineSpeed(bool *MBsup)
{
	if (_state == state_needSetup)
		return false;
	*MBsup = (_mb_enginespeed_data.addr_low != MEMORY_ADDRESS_NONE);
	return true;
}


bool SSMprotocol::hasTestMode(bool *TMsup)
{
	if (_state == state_needSetup)
		return false;
	*TMsup = (_sw_testmodestate_data.addr != MEMORY_ADDRESS_NONE);
	return true;
}


bool SSMprotocol::hasActuatorTests(bool *ATsup)
{
	if (_state == state_needSetup)
		return false;
	*ATsup = _has_ActTest;
	return true;
}


bool SSMprotocol::getSupportedDCgroups(int *DCgroups)
{
	if (_state == state_needSetup)
		return false;
	*DCgroups = _supportedDCgroups;
	return true;
}


bool SSMprotocol::getLastDCgroupsSelection(int *DCgroups)
{
	if (_state == state_needSetup)
		return false;
	*DCgroups = _selectedDCgroups;
	return true;
}


bool SSMprotocol::getSupportedMBs(std::vector<mb_dt> *supportedMBs)
{
	if (_state == state_needSetup)
		return false;
	supportedMBs->assign(begin(_supportedMBs), end(_supportedMBs));
	return true;
}


bool SSMprotocol::getSupportedSWs(std::vector<sw_dt> *supportedSWs)
{
	if (_state == state_needSetup)
		return false;
	supportedSWs->assign(begin(_supportedSWs), end(_supportedSWs));
	return true;
}


bool SSMprotocol::getLastMBSWselection(std::vector<MBSWmetadata_dt> *MBSWmetaList)
{
	if (_state == state_needSetup)
		return false;
	if (!_MBSWmetaList.empty())
	{
		*MBSWmetaList = _MBSWmetaList;
		return true;
	}
	return false;
}


bool SSMprotocol::getSupportedAdjustments(std::vector<adjustment_dt> *supportedAdjustments)
{
	if (_state == state_needSetup)
		return false;
	supportedAdjustments->assign(begin(_adjustments), end(_adjustments));
	return true;
}


bool SSMprotocol::getSupportedActuatorTests(QStringList *actuatorTestTitles)
{
	if (_has_ActTest)
	{
		actuatorTestTitles->clear();
		std::for_each(begin(_actuators), end(_actuators), [actuatorTestTitles](actuator_dt a) { actuatorTestTitles->push_back(a.title); } );
		return true;
	}
	return false;
}


bool SSMprotocol::getLastActuatorTestSelection(unsigned char *actuatorTestIndex)
{
	if (_has_ActTest && (_selectedActuatorTestIndex != 255))
	{
		*actuatorTestIndex = _selectedActuatorTestIndex;
		return true;
	}
	return false;
}


bool SSMprotocol::getVIN(QString *VIN)
{
	(void)*VIN;
	return false;
}


bool SSMprotocol::startDCreading(int DCgroups)
{
	std::vector<unsigned int> DCqueryAddrList;
	bool started;

	// Check if another communication operation is in progress:
	if (_state != state_normal)
		return false;

	// Check argument:
	if ((DCgroups & _supportedDCgroups) != DCgroups)
		return false;

	// Setup diagnostic codes addresses list:
	for (unsigned int b = 0; b < _DTCblockData.size(); b++)
	{
		dc_block_dt dc_block = _DTCblockData.at(b);
		for (unsigned int a = 0; a < dc_block.addresses.size(); a++)
		{
			if (dc_block.addresses.at(a).type == dc_addr_dt::Type::currentOrTempOrLatest)
			{
				if ((DCgroups & currentDTCs_DCgroup) || (DCgroups & temporaryDTCs_DCgroup))
					DCqueryAddrList.push_back(dc_block.addresses.at(a).address);
			}
			if (dc_block.addresses.at(a).type == dc_addr_dt::Type::historicOrMemorized)
			{
				if ((DCgroups & historicDTCs_DCgroup) || (DCgroups & memorizedDTCs_DCgroup))
					DCqueryAddrList.push_back(dc_block.addresses.at(a).address);
			}
			if (dc_block.addresses.at(a).type == dc_addr_dt::Type::CCCCsLatest)
			{
				if (DCgroups & CClatestCCs_DCgroup)
					DCqueryAddrList.push_back(dc_block.addresses.at(a).address);
			}
			if (dc_block.addresses.at(a).type == dc_addr_dt::Type::CCCCsMemorized)
			{
				if (DCgroups & CCmemorizedCCs_DCgroup)
					DCqueryAddrList.push_back(dc_block.addresses.at(a).address);
			}
			// else: can not happen
		}
	}

	// Check if min. 1 address to read:
	if ((DCqueryAddrList.size() < 1))
		return false;

	// Add read address for test mode and D-Check status:
	if ((DCgroups & currentDTCs_DCgroup) || (DCgroups & temporaryDTCs_DCgroup))
	{
		if (_sw_testmodestate_data.addr != MEMORY_ADDRESS_NONE)
			DCqueryAddrList.insert(DCqueryAddrList.begin(), _sw_testmodestate_data.addr);   // NOTE: must be the first address !
		if ((_sw_dcheckstate_data.addr != MEMORY_ADDRESS_NONE) && (_sw_dcheckstate_data.addr != _sw_testmodestate_data.addr))
			DCqueryAddrList.insert(DCqueryAddrList.begin(), _sw_dcheckstate_data.addr);     // NOTE: must be the second address !
	}

	// Start diagostic codes reading:
	started = _SSMPcom->readAddresses_permanent(DCqueryAddrList);
	if (started)
	{
		_state = state_DCreading;
		// Save diagnostic codes group selection (for data evaluation and restartDCreading()):
		_selectedDCgroups = DCgroups;
		// Connect signals and slots:
		connect( _SSMPcom, SIGNAL( receivedData(const std::vector<char>&, int) ),
			this, SLOT( processDCsRawdata(const std::vector<char>&, int) ), Qt::BlockingQueuedConnection );
		// Emit signal:
		emit startedDCreading();
	}
	else
		resetCUdata();

	return started;
}


bool SSMprotocol::restartDCreading()
{
	return startDCreading(_selectedDCgroups);
}


bool SSMprotocol::stopDCreading()
{
	if ((_state == state_needSetup) || (_state == state_normal))
		return true;
	if (_state == state_DCreading)
	{
		if (_SSMPcom->stopCommunication())
		{
			disconnect( _SSMPcom, SIGNAL( receivedData(const std::vector<char>&, int) ),
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


bool SSMprotocol::startMBSWreading(const std::vector<MBSWmetadata_dt>& mbswmetaList)
{
	bool started = false;
	if (_state != state_normal)
		return false;
	// Setup list of MB/SW-addresses for communication:
	if (!setupMBSWQueryAddrList(mbswmetaList))
		return false;
	// Start MB/SW-reading:
	started = _SSMPcom->readAddresses_permanent(_selMBsSWsAddr);
	if (started)
	{
		_state = state_MBSWreading;
		// Save MB/SW-selection (necessary for evaluation of raw data):
		_MBSWmetaList = mbswmetaList;
		// Connect signals/slots:
		connect( _SSMPcom, SIGNAL( receivedData(const std::vector<char>&, int) ),
		         this, SLOT( processMBSWrawData(const std::vector<char>&, int) ) );
		// Emit signal:
		emit startedMBSWreading();
	}
	else
		resetCUdata();
	return started;
}


bool SSMprotocol::restartMBSWreading()
{
	return startMBSWreading(_MBSWmetaList);
}


bool SSMprotocol::stopMBSWreading()
{
	if ((_state == state_needSetup) || (_state == state_normal))
		return true;
	if (_state == state_MBSWreading)
	{
		if (_SSMPcom->stopCommunication())
		{
			disconnect( _SSMPcom, SIGNAL( receivedData(const std::vector<char>&, int) ),
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


bool SSMprotocol::getAdjustmentValue(unsigned char index, unsigned int *rawValue)
{
	std::vector<unsigned int> dataaddr;
	std::vector<char> data;
	if (_state != state_normal)
		return false;
	// Validate adjustment value selection:
	if (_adjustments.empty() || (index >= _adjustments.size()))
		return false;
	// Convert memory address into two byte addresses:
	dataaddr.push_back( _adjustments.at(index).addrLow );
	if (_adjustments.at(index).addrHigh > 0)
		dataaddr.push_back( _adjustments.at(index).addrHigh );
	// Read data from control unit:
	if (!_SSMPcom->readAddresses(dataaddr, &data))
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


bool SSMprotocol::getAllAdjustmentValues(std::vector<unsigned int> * rawValues)
{
	std::vector<unsigned int> dataaddr;
	std::vector<char> data;
	unsigned char k = 0;
	unsigned int addrindex = 0;
	if ((_state != state_normal) || _adjustments.empty())
		return false;
	// Setup address list:
	for (k = 0; k < _adjustments.size(); k++)
	{
		dataaddr.push_back( _adjustments.at(k).addrLow );
		if (_adjustments.at(k).addrHigh > 0)
			dataaddr.push_back( _adjustments.at(k).addrHigh );
	}
	// Read data from control unit:
	data.resize( dataaddr.size() );
	if (!_SSMPcom->readAddresses(dataaddr, &data))
	{
		resetCUdata();
		return false;
	}
	// Calculate and return raw values:
	rawValues->clear();
	for (k = 0; k < _adjustments.size(); k++)
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


bool SSMprotocol::setAdjustmentValue(unsigned char index, unsigned int rawValue)
{
	std::vector<unsigned int> addresses;
	std::vector<char> data;
	if (_state != state_normal)
		return false;
	// Validate adjustment value selection:
	if (index >= _adjustments.size())
		return false;
	if ((_adjustments.at(index).rawMin <= _adjustments.at(index).rawMax) && ((rawValue < _adjustments.at(index).rawMin) || ((rawValue > _adjustments.at(index).rawMax))))
		return false;
	if ((_adjustments.at(index).rawMin > _adjustments.at(index).rawMax) && (rawValue < _adjustments.at(index).rawMin) && (rawValue > _adjustments.at(index).rawMax))
		return false;
	if ((_adjustments.at(index).addrHigh > 0) && (rawValue > 65535))
		return false;
	else if (rawValue > 255)
		return false;
	// Setup addresses and convert raw value to 2 byte values:
	addresses.push_back( _adjustments.at(index).addrLow );
	data.push_back( static_cast<char>(rawValue & 0xff) );
	if (_adjustments.at(index).addrHigh > 0)
	{
		addresses.push_back( _adjustments.at(index).addrHigh );
		data.push_back( static_cast<char>((rawValue & 0xffff) >> 8) );
	}
	// Write value to control unit:
	if (!_SSMPcom->writeAddresses(addresses, data))
	{
		resetCUdata();
		return false;
	}
	return true;
}


bool SSMprotocol::startActuatorTest(unsigned char actuatorTestIndex)
{
	bool ATstarted = false;
	bool ok = false;
	bool testmode = false;
	bool running = false;
	// Check if another communication operation is in progress:
	if (_state != state_normal)
		return false;
	// Validate selected test:
	if (!_has_ActTest || (actuatorTestIndex >= _actuators.size()))
		return false;
	// Check if control unit is in test mode:
	if (_sw_testmodestate_data.addr != MEMORY_ADDRESS_NONE)
	{
		ok = isInTestMode(&testmode);
		if (!ok || !testmode)
			return false;
	} // else: assume it is not relevant (otherwise the ECU would provide it)
	// Check that engine is not running:
	if (_mb_enginespeed_data.addr_low != MEMORY_ADDRESS_NONE)
	{
		ok = isEngineRunning(&running);
		if (!ok || running)
			return false;
	} // else: assume it is not relevant (otherwise the ECU would provide it)
	// Change state:
	_state = state_ActTesting;
	// Prepare test addresses:
	const unsigned int dataaddr = _actuators.at(actuatorTestIndex).byteAddr;
	const char databyte = static_cast<char>(1 << (_actuators.at(actuatorTestIndex).bitAddr - 1));
	// Stop all actuator tests:
	for (size_t k = 0; k < _allActByteAddr.size(); k++)
	{
		if (!_SSMPcom->writeAddress(_allActByteAddr.at(k), 0x00))
		{
			_state = state_normal; // this avoids that resetCUdata() will try to stop all actuators again
			resetCUdata();
			return false;
		}
	}
	// Start Actuator Test:
	ATstarted = _SSMPcom->writeAddress_permanent(dataaddr, databyte, 100);
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


bool SSMprotocol::restartActuatorTest()
{
	return startActuatorTest(_selectedActuatorTestIndex);
}


bool SSMprotocol::stopActuatorTesting()
{
	if ((_state == state_needSetup) || (_state == state_normal))
		return true;
	if (_state == state_ActTesting)
	{
		if (_SSMPcom->stopCommunication())
		{
			// Stop all actuator tests:
			for (size_t k = 0; k < _allActByteAddr.size(); k++)
			{
				if (!_SSMPcom->writeAddress(_allActByteAddr.at(k), 0x00))
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


bool SSMprotocol::stopAllActuators()
{
	// NOTE: This function can be called even if no actuator test has been started with SSMprotocol
	// => When switching the cars ignition on (with engine off) while test mode connector is connected,
	//    some actuator tests are started automatically
	bool ok = false;
	bool testmode = false;
	bool enginerunning = false;
	if (_state != state_normal)
		return false;
	// Check if actuator tests are supported:
	if (!_has_ActTest)
		return false;
	// Check if control unit is in test mode:
	if (_sw_testmodestate_data.addr != MEMORY_ADDRESS_NONE)
	{
		ok = isInTestMode(&testmode);
		if (!ok || !testmode)
			return false;
	} // else: assume it is not relevant (otherwise the ECU would provide it)
	// Check that engine is not running:
	if (_mb_enginespeed_data.addr_low != MEMORY_ADDRESS_NONE)
	{
		ok = isEngineRunning(&enginerunning);
		if (!ok || enginerunning)
			return false;
	} // else: assume it is not relevant (otherwise the ECU would provide it)
	// Stop all actuator tests:
	for (size_t k = 0; k < _allActByteAddr.size(); k++)
	{
		if (!_SSMPcom->writeAddress(_allActByteAddr.at(k), 0x00))
		{
			resetCUdata();
			return false;
		}
	}
	return true;
}


bool SSMprotocol::clearMemory(CMlevel_dt level, bool *success)
{
	bool CMsup = false;
	unsigned int addr = MEMORY_ADDRESS_NONE;
	char val = 0;
	char bytewritten = '\x00';

	if (success != NULL)
		*success = false;
	// NOTE: _state validated by hasClearMemory()
	if (level == CMlevel_1)
	{
		if (hasClearMemory(&CMsup) && CMsup)
		{
			addr = _CMaddr;
			val = _CMvalue;
		}
		else
			return false;
	}
	else if (level == CMlevel_2)
	{
		if (hasClearMemory2(&CMsup) && CMsup)
		{
			addr = _CM2addr;
			val = _CM2value;
		}
		else
			return false;
	}
	else
		return false;
	if (!_SSMPcom->writeAddress(addr, val, &bytewritten))
	{
		resetCUdata();
		return false;
	}
	if (success != NULL)
		*success = (bytewritten == val);
	return true;
}


bool SSMprotocol::testImmobilizerCommLine(immoTestResult_dt *result)
{
	char checkvalue = 0;
	unsigned int readcheckaddr = 0x8B;

	if (_state != state_normal)
		return false;
	if (!_ssmCUdata.uses_Ax10xx_defs())
		return false;
		// NOTE/TODO: immobilizer test might work different for 7xxxxx and Ax01xx controllers (if supported) !
	if (!_has_ImmoTest)
		return false;
	// Write test-pattern:
	if (_SSMPcom->writeAddress(0xE0, '\xAA', &checkvalue))
	{
		// Read result:
		if (_SSMPcom->readAddress(readcheckaddr, &checkvalue))
		{
			/* NOTE: the actually written data is NOT 0xAA ! */
			if (checkvalue == '\x01')
				*result = immoShortedToGround;
			else if  (checkvalue == '\x02')
				*result = immoShortedToBattery;
			else
				*result = immoNotShorted;
			return true;
		}
	}
	// Communication error:
	resetCUdata();
	return false;
}


bool SSMprotocol::isEngineRunning(bool *isrunning)
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
	if (!_SSMPcom->readAddresses(addresses, &data))
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


bool SSMprotocol::isInTestMode(bool *testmode)
{
	char byte = 0;

	if (_state != state_normal)
		return false;
	if (_sw_testmodestate_data.addr == MEMORY_ADDRESS_NONE)
		return false;
	if (!_SSMPcom->readAddress(_sw_testmodestate_data.addr, &byte))
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


bool SSMprotocol::stopAllPermanentOperations()
{
	bool result = false;
	if ((_state == state_needSetup) || (_state == state_normal))
		result = true;
	else if (_state == state_DCreading)
		result = stopDCreading();
	else if (_state == state_MBSWreading)
		result = stopMBSWreading();
	else if (_state == state_ActTesting)
		result = stopActuatorTesting();
	return result;
}


bool SSMprotocol::waitForIgnitionOff()
{
	unsigned int dataaddr;

	if (_state != state_normal)
		return false;
	_state = state_waitingForIgnOff;
	_SSMPcom->setRetriesOnError(1);
	if (_sw_ignitionstate_data.addr != MEMORY_ADDRESS_NONE)
	{
		bool ignstate = true;
		char data = 0x00;
		do
		{
			if (!_SSMPcom->readAddress(_sw_ignitionstate_data.addr, &data))

				ignstate = false;
			else
				ignstate = ((data & (1 << _sw_ignitionstate_data.bit)) ^ _sw_ignitionstate_data.inverted);
		} while (ignstate);
	}
	else
	{
		dataaddr = 0x0000;
		QEventLoop el;
		disconnect( _SSMPcom, SIGNAL( commError() ), this, SIGNAL( commError() ) );
		disconnect( _SSMPcom, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
		if(!_SSMPcom->readAddress_permanent(dataaddr))
		{
			resetCUdata();
			return false;
		}
		connect(_SSMPcom, SIGNAL( commError() ), &el, SLOT( quit() ));
		el.exec();
		disconnect(_SSMPcom, SIGNAL( commError() ), &el, SLOT( quit() ));
	}
	_SSMPcom->setRetriesOnError(2);
	resetCUdata();
	return true;
}

// PROTECTED / PRIVATE:

void SSMprotocol::processDCsRawdata(std::vector<char> DCrawdata, int duration_ms)
{
	(void)duration_ms; // to avoid compiler error
	bool TestMode = false;
	bool DCheckActive = false;
	QStringList DCs_currentOrTemporary;
	QStringList DCdescriptions_currentOrTemporary;
	QStringList DCs_historicOrMemorized;
	QStringList DCdescriptions_historicOrMemorized;
	QStringList CCCCs_latest;
	QStringList CCCCdescriptions_latest;
	QStringList CCCCs_memorized;
	QStringList CCCCdescriptions_memorized;
	unsigned int rd_index = 0;

	if ((_selectedDCgroups & currentDTCs_DCgroup) || (_selectedDCgroups & temporaryDTCs_DCgroup))
	{
		if (_sw_testmodestate_data.addr != MEMORY_ADDRESS_NONE)
		{
			TestMode = ((DCrawdata.at(0) & (1 << _sw_testmodestate_data.bit)) ^ _sw_testmodestate_data.inverted);
			rd_index++;
		}
		if (_sw_dcheckstate_data.addr != MEMORY_ADDRESS_NONE)
		{
			if ((rd_index > 0) && (_sw_dcheckstate_data.addr != _sw_testmodestate_data.addr))
			{
				DCheckActive = ((DCrawdata.at(rd_index) & (1 << _sw_dcheckstate_data.bit)) ^ _sw_dcheckstate_data.inverted);
				rd_index++;
			}
			else
				DCheckActive = ((DCrawdata.at(0) & (1 << _sw_dcheckstate_data.bit)) ^ _sw_dcheckstate_data.inverted);
		}
	}

	for (size_t b = 0; b < _DTCblockData.size(); b++)
	{
		dc_block_dt block = _DTCblockData.at(b);
		for (size_t a = 0; a < block.addresses.size(); a++)
		{
			QString tmpCode;
			QString tmpDescr;
			QStringList tmpCodes;
			QStringList tmpDescriptions;
			dc_addr_dt addr = block.addresses.at(a);
			unsigned int address = addr.address;
			char databyte = DCrawdata.at(rd_index);

			_SSMdefsIfce->getDCcontent(address, databyte, &tmpCodes, &tmpDescriptions);
			if (addr.type == dc_addr_dt::Type::currentOrTempOrLatest)
			{
				DCs_currentOrTemporary.append(tmpCodes);
				DCdescriptions_currentOrTemporary.append(tmpDescriptions);
			}
			else if (addr.type == dc_addr_dt::Type::historicOrMemorized)
			{
				DCs_historicOrMemorized.append(tmpCodes);
				DCdescriptions_historicOrMemorized.append(tmpDescriptions);
			}
			else if (addr.type == dc_addr_dt::Type::CCCCsLatest)
			{
				CCCCs_latest.append(tmpCodes);
				CCCCdescriptions_latest.append(tmpDescriptions);
			}
			else if (addr.type == dc_addr_dt::Type::CCCCsMemorized)
			{
				CCCCs_memorized.append(tmpCodes);
				CCCCdescriptions_memorized.append(tmpDescriptions);
			}
			// else: cannot happen

			rd_index++;
		}
	}

	emit currentOrTemporaryDTCs(DCs_currentOrTemporary, DCdescriptions_currentOrTemporary, TestMode, DCheckActive);
	emit historicOrMemorizedDTCs(DCs_historicOrMemorized, DCdescriptions_historicOrMemorized);
	emit latestCCCCs(CCCCs_latest, CCCCdescriptions_latest);
	emit memorizedCCCCs(CCCCs_memorized, CCCCdescriptions_memorized);
}


bool SSMprotocol::setupMBSWQueryAddrList(std::vector<MBSWmetadata_dt> MBSWmetaList)
{
	// ***** SETUP (BYTE-) ADDRESS LIST FOR QUERYS *****
	bool newaddr = true;
	_selMBsSWsAddr.clear();
	if (MBSWmetaList.size() == 0)
		return false;
	for (size_t k = 0; k < MBSWmetaList.size(); k++)
	{
		newaddr = true;
		// CHECK IF ADDRESS IS ALREADY ON THE QUERY-LIST:
		if (_selMBsSWsAddr.size())
		{
			// CHECK IF ADDRESS IS ALREADY ON THE LIST:
			for (size_t m = 0; m < _selMBsSWsAddr.size(); m++)
			{
				if (MBSWmetaList.at(k).blockType == BlockType::MB)
				{
					// CHECK IF CURRENT MB IS VALID/EXISTS:
					if (MBSWmetaList.at(k).nativeIndex > _supportedMBs.size())
						return false;
					// COMPARE ADDRESS:
					if (_selMBsSWsAddr.at(m) == _supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).addrLow)
					{
						newaddr = false;
						break;
					}
				}
				else
				{
					// CHECK IF CURRENT SW IS VALID/EXISTS:
					if (MBSWmetaList.at(k).nativeIndex > _supportedSWs.size())
						return false;
					// COMPARE ADDRESS:
					if (_selMBsSWsAddr.at(m) == _supportedSWs.at( MBSWmetaList.at(k).nativeIndex ).byteAddr)
					{
						newaddr = false;
						break;
					}
				}
			}
		}
		// ADD ADDRESS TO QUERY-LIST IF IT IS NEW:
		if (newaddr)
		{
			if (MBSWmetaList.at(k).blockType == BlockType::MB)
			{
				// ADD ADDRESS(ES) OF CURRENT MB TO LIST:
				_selMBsSWsAddr.push_back( _supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).addrLow );
				if (_supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).addrHigh != MEMORY_ADDRESS_NONE)
					_selMBsSWsAddr.push_back( _supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).addrHigh );
			}
			else
				// ADD ADDRESS OF CURRENT SW TO LIST:
				_selMBsSWsAddr.push_back( _supportedSWs.at( MBSWmetaList.at(k).nativeIndex ).byteAddr );
		}
	}
	return true;
}


void SSMprotocol::processMBSWrawData(const std::vector<char>& MBSWrawdata, int duration_ms)
{
	std::vector<unsigned int> rawValues = assignMBSWRawData(MBSWrawdata);
	emit newMBSWrawValues(rawValues, duration_ms);
}


std::vector<unsigned int> SSMprotocol::assignMBSWRawData(const std::vector<char>& rawdata)
{
	std::vector<unsigned int> mbswrawvalues(_MBSWmetaList.size());

	for (size_t m = 0; m < _selMBsSWsAddr.size(); m++)
	{
		const unsigned int address = _selMBsSWsAddr.at(m);
		const unsigned char rawbyte = static_cast<unsigned char>(rawdata.at(m));

		for (size_t k = 0; k < _MBSWmetaList.size(); k++)
		{
			const MBSWmetadata_dt& metadata = _MBSWmetaList.at(k);
			unsigned int& rawvalue = mbswrawvalues.at(k);

			if (metadata.blockType == BlockType::MB)
			{
				mb_intl_dt& mb = _supportedMBs.at(metadata.nativeIndex);
				// COMPARE ADDRESSES:
				if (address == mb.addrLow)
				{
					// ADDRESS/RAW BYTE CORRESPONDS WITH LOW BYTE ADDRESS OF MB
					rawvalue += rawbyte;
				}
				else if (address == mb.addrHigh)
				{
					// ADDRESS/RAW BYTE CORRESPONDS WITH HIGH BYTE ADDRESS OF MB
					rawvalue += rawbyte << 8;
				}
			}
			else
			{
				sw_intl_dt& sw = _supportedSWs.at(metadata.nativeIndex);
				if (address == sw.byteAddr)
				{
					// ADDRESS/RAW BYTE CORRESPONS WITH BYTE ADDRESS OF SW
					const unsigned char bitmask = 1 << (sw.bitAddr - 1);
					rawvalue = (rawbyte & bitmask) ? 1 : 0;
				}
			}
		}
	}
	return mbswrawvalues;
}


void SSMprotocol::setupActuatorTestAddrList()
{
	bool aol = false;

	_allActByteAddr.clear();
	for (size_t n = 0; n < _actuators.size(); n++)
	{
		// Check if byte address is already on the list:
		for (size_t m = 0; m < _allActByteAddr.size(); m++)
		{
			if (_allActByteAddr.at(m) == _actuators.at(n).byteAddr)
			{
				aol = true;
				break;
			}
		}
		// Put address on addresses list (if not duplicate):
		if (!aol)
			_allActByteAddr.push_back( _actuators.at(n).byteAddr );
		else
			aol = false;
	}
}


void SSMprotocol::determineSupportedDCgroups(std::vector<dc_block_dt> DCblockData)
{
	_supportedDCgroups = noDCs_DCgroup;
	for (size_t b = 0; b < DCblockData.size(); b++)
	{
		dc_block_dt block_data = DCblockData.at(b);
		for (size_t a = 0; a < block_data.addresses.size(); a++)
		{
			dc_addr_dt addr_data = block_data.addresses.at(a);
			if (addr_data.type == dc_addr_dt::Type::currentOrTempOrLatest)
			{
				if (addr_data.format == dc_addr_dt::Format::OBD)
					_supportedDCgroups |= temporaryDTCs_DCgroup;
				else // dc_addr_dt::Format::simple
					_supportedDCgroups |= currentDTCs_DCgroup;
			}
			if (addr_data.type == dc_addr_dt::Type::historicOrMemorized)
			{
				if (addr_data.format == dc_addr_dt::Format::OBD)
					_supportedDCgroups |= memorizedDTCs_DCgroup;
				else // dc_addr_dt::Format::simple
					_supportedDCgroups |= historicDTCs_DCgroup;
			}
			if (addr_data.type == dc_addr_dt::Type::CCCCsLatest)
				_supportedDCgroups |= CClatestCCs_DCgroup;
			if (addr_data.type == dc_addr_dt::Type::CCCCsMemorized)
				_supportedDCgroups |= CCmemorizedCCs_DCgroup;
		}
	}
}

