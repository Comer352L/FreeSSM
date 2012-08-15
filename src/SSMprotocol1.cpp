/*
 * SSMprotocol1.cpp - Application Layer for the old Subaru SSM protocol
 *
 * Copyright (C) 2009-2012 Comer352L
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
		disconnect( _SSMP1com, SIGNAL( recievedData(std::vector<char>, int) ),
			    this, SLOT( processDTCsRawdata(std::vector<char>, int) ) );
		disconnect( _SSMP1com, SIGNAL( recievedData(std::vector<char>, int) ),
			    this, SLOT( processMBSWrawData(std::vector<char>, int) ) );
		// Try to stop active communication processes:
		_SSMP1com->stopCommunication();
		// NOTE: DO NOT CALL stopCommOperation() or any other communicating functions here because of possible recursions !
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
	_uses_SSM2defs = false;
	_CMaddr = MEMORY_ADDRESS_NONE;
	_CMvalue = '\x00';
}


SSMprotocol::CUsetupResult_dt SSMprotocol1::setupCUdata(CUtype_dt CU)
{
	std::string SSM1defsFile;
	SSM1_CUtype_dt SSM1_CU;
	char flagbytes[96];
	unsigned char nrofflagbytes;
	// Reset:
	resetCUdata();
	// Create SSMP1communication-object:
	if (CU == CUtype_Engine)
	{
		SSM1_CU = SSM1_CU_Engine;
		SSM1defsFile = QCoreApplication::applicationDirPath().toStdString() + "/SSM1defs_Engine.xml";
	}
	else if (CU == CUtype_Transmission)
	{
		SSM1_CU = SSM1_CU_Transmission;
		SSM1defsFile = QCoreApplication::applicationDirPath().toStdString() + "/SSM1defs_Transmission.xml";
	}
	else if (CU == CUtype_CruiseControl)
	{
		SSM1_CU = SSM1_CU_CruiseCtrl;
		SSM1defsFile = QCoreApplication::applicationDirPath().toStdString() + "/SSM1defs_CruiseControl.xml";
	}
	else if (CU == CUtype_AirCon)
	{
		SSM1_CU = SSM1_CU_AirCon;
		SSM1defsFile = QCoreApplication::applicationDirPath().toStdString() + "/SSM1defs_AirConditioning.xml";
	}
	else if (CU == CUtype_FourWheelSteering)
	{
		SSM1_CU = SSM1_CU_FourWS;
		SSM1defsFile = QCoreApplication::applicationDirPath().toStdString() + "/SSM1defs_FourWheelSteering.xml";
	}
	else if (CU == CUtype_ABS)
	{
		SSM1_CU = SSM1_CU_ABS;
		SSM1defsFile = QCoreApplication::applicationDirPath().toStdString() + "/SSM1defs_ABS.xml";
	}
	else if (CU == CUtype_AirSuspension)
	{
		SSM1_CU = SSM1_CU_AirSusp;
		SSM1defsFile = QCoreApplication::applicationDirPath().toStdString() + "/SSM1defs_AirSuspension.xml";
	}
	else if (CU == CUtype_PowerSteering)
	{
		SSM1_CU = SSM1_CU_PwrSteer;
		SSM1defsFile = QCoreApplication::applicationDirPath().toStdString() + "/SSM1defs_PowerSteering.xml";
	}
	else
		return result_invalidCUtype;
	_SSMP1com = new SSMP1communication(_diagInterface, SSM1_CU);
	// Get control unit ID:
	bool ok = _SSMP1com->getCUdata(_SYS_ID, flagbytes, &nrofflagbytes);
	if (!ok && (CU == CUtype_AirCon))
	{
		_SSMP1com->selectCU(SSM1_CU_AirCon2);
		ok = _SSMP1com->getCUdata(_SYS_ID, flagbytes, &nrofflagbytes);
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
	_uses_SSM2defs = ((_SYS_ID[0] & '\xF0') == '\xA0') && (_SYS_ID[1] == '\x10');
	// Connect communication error signals from SSMP1communication:
	connect( _SSMP1com, SIGNAL( commError() ), this, SIGNAL( commError() ) );
	connect( _SSMP1com, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
	/* Load control unit definitions */
	if (_uses_SSM2defs)
	{
		if (nrofflagbytes > 32)
			nrofflagbytes = 32;
		// NOTE: Some Ax 10 xx controllers send more than 32 bytes of data, but only the first 32 bytes are flag bytes.
		// FIXME: Remove this when SSMP1communication::getCUdata(...) has been modified and handles the extra data correctly
 		// Read extended ID (5-byte ROM-ID):
		if (!readExtendedID(_ROM_ID))
			return result_commError;
		// Setup definitions interface:
		SSM2definitionsInterface SSM2defsIface(_language);
		SSM2defsIface.selectControlUnitID(_CU, _SYS_ID, _ROM_ID, flagbytes, nrofflagbytes);
		// Get system description:
		SSM2defsIface.systemDescription(&_sysDescription);
		// Get supported features;
		SSM2defsIface.hasOBD2system(&_has_OBD2);
		SSM2defsIface.hasImmobilizer(&_has_Immo);
		bool CMsup = false;
		SSM2defsIface.hasClearMemory(&CMsup);
		if (CMsup)
		{
			_CMaddr = 0x60;
			_CMvalue = 0x40;
		}
		SSM2defsIface.hasTestMode(&_has_TestMode);
		SSM2defsIface.hasActuatorTests(&_has_ActTest);
		SSM2defsIface.hasMBengineSpeed(&_has_MB_engineSpeed);
		SSM2defsIface.hasSWignition(&_has_SW_ignition);
		// Get definitions of the supported diagnostic codes:
		SSM2defsIface.diagnosticCodes(&_DTCdefs, &_DTC_fmt_OBD2);
		// Get supported MBs and SWs:
		SSM2defsIface.measuringBlocks(&_supportedMBs);
		SSM2defsIface.switches(&_supportedSWs);
		// Get supported adjustment values;
		SSM2defsIface.adjustments(&_adjustments);
		// Get supported actuator tests:
		SSM2defsIface.actuatorTests(&_actuators);
		setupActuatorTestAddrList();
		// NOTE: all other SSM2-features (VIN, CC, CM2...) are not be supported by SSM1 Control units
	}
	else
	{
		// Setup definitions interface:
		SSM1definitionsInterface SSM1defsIface;
		if (!SSM1defsIface.selectDefinitionsFile(SSM1defsFile))
			return result_noOrInvalidDefsFile;
		SSM1defsIface.setLanguage(_language.toStdString());
		if (!SSM1defsIface.selectID(_SYS_ID)) // TODO: Ax 01 xx IDs
			return result_noDefs;
		// Get system description:
		std::string sysdescription;
		SSM1defsIface.systemDescription(&sysdescription);
		_sysDescription = QString::fromStdString(sysdescription);
		// Get definitions of the supported diagnostic codes:
		SSM1defsIface.diagnosticCodes(&_DTCdefs);
		// Get supported MBs and SWs:
		SSM1defsIface.measuringBlocks(&_supportedMBs);
		SSM1defsIface.switches(&_supportedSWs);
		// Get Clear Memory data:
		SSM1defsIface.clearMemoryData(&_CMaddr, &_CMvalue);
	}
	return result_success;
}


bool SSMprotocol1::hasClearMemory(bool *CMsup)
{
	if (_state == state_needSetup) return false;
	*CMsup = (_CMaddr != MEMORY_ADDRESS_NONE);
	return true;
}


bool SSMprotocol1::getSupportedDCgroups(int *DCgroups)
{
	int retDCgroups = 0;
	if (_state == state_needSetup) return false;
	if (_DTCdefs.size())
	{
		if (_DTC_fmt_OBD2)
			retDCgroups |= temporaryDTCs_DCgroup;
		else
			retDCgroups |= currentDTCs_DCgroup;
		for (unsigned int k=0; k<_DTCdefs.size(); k++)
		{
			if (_DTCdefs.at(k).byteAddr_historicOrMemorized != MEMORY_ADDRESS_NONE)
			{
				if (_DTC_fmt_OBD2)
					retDCgroups |= memorizedDTCs_DCgroup;
				else
					retDCgroups |= historicDTCs_DCgroup;
				break;
			}
		}
	}
	*DCgroups = retDCgroups;
	return true;
}


bool SSMprotocol1::startDCreading(int DCgroups)
{
	std::vector<unsigned int> DCqueryAddrList;
	bool started;
	// Check if another communication operation is in progress:
	if (_state != state_normal) return false;
	// Check argument:
	if (DCgroups < 1 || DCgroups > 15)
		return false;
	if (((DCgroups & currentDTCs_DCgroup) || (DCgroups & historicDTCs_DCgroup)) && ((DCgroups & temporaryDTCs_DCgroup) || (DCgroups & memorizedDTCs_DCgroup)))
		return false;
	// Setup diagnostic codes addresses list:
	if ((DCgroups & currentDTCs_DCgroup) || (DCgroups & temporaryDTCs_DCgroup))	// current/temporary DTCs
	{
		if ((_CU == CUtype_Engine) && _uses_SSM2defs)
			DCqueryAddrList.push_back( 0x000061 );
		// FIXME: test mode and D-Check status addresses for other SSM1 control units
		for (unsigned int k=0; k<_DTCdefs.size(); k++)
		{
			if (_DTCdefs.at(k).byteAddr_currentOrTempOrLatest != MEMORY_ADDRESS_NONE)
				DCqueryAddrList.push_back( _DTCdefs.at(k).byteAddr_currentOrTempOrLatest );
		}
	}	
	if ((DCgroups & historicDTCs_DCgroup) || (DCgroups & memorizedDTCs_DCgroup))	// historic/memorized DTCs
	{
		for (unsigned int k=0; k<_DTCdefs.size(); k++)
		{
			if (_DTCdefs.at(k).byteAddr_historicOrMemorized != MEMORY_ADDRESS_NONE)
				DCqueryAddrList.push_back( _DTCdefs.at(k).byteAddr_historicOrMemorized );
		}
	}
	// Check if min. 1 address to read:
	if ((DCqueryAddrList.size() < 1) || ((DCqueryAddrList.size() < 2) && _uses_SSM2defs && (DCqueryAddrList.at(0) == 0x000061)))
		return false;
	// Start diagostic code reading:
	started = _SSMP1com->readAddresses_permanent( DCqueryAddrList );
	if (started)
	{
		_state = state_DCreading;
		// Save diagnostic codes group selection (for data evaluation and restartDCreading()):
		_selectedDCgroups = DCgroups;
		// Connect signals and slots:
		connect( _SSMP1com, SIGNAL( recievedData(std::vector<char>, int) ),
			this, SLOT( processDTCsRawdata(std::vector<char>, int) ), Qt::BlockingQueuedConnection );
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
			disconnect( _SSMP1com, SIGNAL( recievedData(std::vector<char>, int) ),
				    this, SLOT( processDTCsRawdata(std::vector<char>, int) ) );
			_state = state_normal;
			emit stoppedDCreading();
			return true;
		}
		// Communication error:
		resetCUdata();
	}
	return false;
}


bool SSMprotocol1::startMBSWreading(std::vector<MBSWmetadata_dt> mbswmetaList)
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
		connect( _SSMP1com, SIGNAL( recievedData(std::vector<char>, int) ),
			this, SLOT( processMBSWrawData(std::vector<char>, int) ) ); 
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
			disconnect( _SSMP1com, SIGNAL( recievedData(std::vector<char>, int) ),
				    this, SLOT( processMBSWrawData(std::vector<char>, int) ) );
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
	dataaddr.push_back( _adjustments.at(index).AddrLow );
	if (_adjustments.at(index).AddrHigh > 0)
		dataaddr.push_back( _adjustments.at(index).AddrHigh );
	// Read data from control unit:
	if (!_SSMP1com->readAddresses(dataaddr, &data))
	{
		resetCUdata();
		return false;
	}
	// Calculate raw value:
	*rawValue = static_cast<unsigned char>(data.at(0));
	if (_adjustments.at(index).AddrHigh > 0)
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
		dataaddr.push_back( _adjustments.at(k).AddrLow );
		if (_adjustments.at(k).AddrHigh > 0)
			dataaddr.push_back( _adjustments.at(k).AddrHigh );
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
		if (_adjustments.at(k).AddrHigh > 0)
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
	if ((_adjustments.at(index).AddrHigh > 0) && (rawValue > 65535))
	{
		return false;
	}
	else if (rawValue > 255)
	{
		return false;
	}
	// Setup addresses and convert raw value to 2 byte values:
	addresses.push_back( _adjustments.at(index).AddrLow );
	data.push_back( static_cast<char>(rawValue & 0xff) );
	if (_adjustments.at(index).AddrHigh > 0)
	{
		addresses.push_back( _adjustments.at(index).AddrHigh );
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
	unsigned int dataaddr = _actuators.at(actuatorTestIndex).byteadr;
	char databyte = static_cast<char>(pow(2, _actuators.at(actuatorTestIndex).bitadr - 1));
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
	*success = false;
	if (_state != state_normal) return false;
	if (level != CMlevel_1) return false;
	if (_CMaddr == MEMORY_ADDRESS_NONE) return false;
	char bytewritten = 0;
	if (!_SSMP1com->writeAddress(_CMaddr, _CMvalue, &bytewritten))
	{
		resetCUdata();
		return false;
	}
	*success = (bytewritten == _CMvalue);
	return true;
}


bool SSMprotocol1::testImmobilizerCommLine(immoTestResult_dt *result)
{
	if (_state != state_normal) return false;
	if (!_uses_SSM2defs)
		return false;
	if (!_has_Immo) return false;
	char checkvalue = 0;
	unsigned int readcheckadr = 0x8B;
	// Write test-pattern:
	if (_SSMP1com->writeAddress(0xE0, '\xAA', &checkvalue))
	{
		// Read result:
		if (_SSMP1com->readAddress(readcheckadr, &checkvalue))
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
	char currentdatabyte = 0;
	if (_state != state_normal) return false;
	if ((_CU != CUtype_Engine) && (_CU != CUtype_Transmission))
		return false;
	if (!_uses_SSM2defs)	// FIXME: other defintion types
		return false;
	if (!_has_MB_engineSpeed) return false;
	
	if (!_SSMP1com->readAddress(0x0e, &currentdatabyte))
	{
		resetCUdata();
		return false;
	}
	if (currentdatabyte > 3)
		*isrunning = true;
	else
		*isrunning = false;
	return true;
	// FIXME: this is a dirty hack !
}


bool SSMprotocol1::isInTestMode(bool *testmode)
{
	unsigned int dataadr = 0x61;
	char currentdatabyte = 0;
	if (_state != state_normal) return false;
	if (_uses_SSM2defs)	// FIXME: other defintion types
		return false;
	if (!_has_TestMode) return false;
	if (!_SSMP1com->readAddress(dataadr, &currentdatabyte))
	{
		resetCUdata();
		return false;
	}
	if (currentdatabyte & 0x20)
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
	if (_has_SW_ignition && _uses_SSM2defs)	// FIXME: other defintion types
	{
		bool ignstate = true;
		char data = 0x00;
		dataaddr = 0x62;
		do
		{
			if (!_SSMP1com->readAddress(dataaddr, &data))
				ignstate = false;
			else
				ignstate = (data & 0x08);
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

bool SSMprotocol1::readExtendedID(char ID[5])
{
	std::vector<unsigned int> addresses;
	for (unsigned int addr=0x01; addr<=0x05; addr++)
		addresses.push_back(addr);
	std::vector<char> data;
	if (!_SSMP1com->readAddresses(addresses, &data))
		return false;
	for (unsigned char i=0; i<5; i++)
		ID[i] = data.at(i);
	return true;
}


