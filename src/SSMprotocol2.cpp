/*
 * SSMprotocol2.cpp - Application Layer for the new Subaru SSM protocol
 *
 * Copyright (C) 2008-2012 Comer352L
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

#include "SSMprotocol2.h"



SSMprotocol2::SSMprotocol2(AbstractDiagInterface *diagInterface, QString language) : SSMprotocol(diagInterface, language)
{
	_SSMP2com = NULL;
	_SSM2defsIface = NULL;
	resetCUdata();
}


SSMprotocol2::~SSMprotocol2()
{
	resetCUdata();
}


void SSMprotocol2::resetCUdata()
{
	unsigned int k = 0;
	bool ok = false;
	unsigned int dataadr = 0x61;
	char currentdatabyte = '\x0';
	// RESET COMMUNICATION:
	if (_SSMP2com != NULL)
	{
		// Disconnect communication error and data signals:
		disconnect( _SSMP2com, SIGNAL( commError() ), this, SIGNAL( commError() ) );
		disconnect( _SSMP2com, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
		disconnect( _SSMP2com, SIGNAL( recievedData(std::vector<char>, int) ),
			    this, SLOT( processDCsRawdata(std::vector<char>, int) ) );
		disconnect( _SSMP2com, SIGNAL( recievedData(std::vector<char>, int) ),
			    this, SLOT( processMBSWrawData(std::vector<char>, int) ) );
		// Try to stop active communication processes:
		ok = _SSMP2com->stopCommunication();
		if (ok && (_state == state_ActTesting))
		{
			ok = _SSMP2com->readMultipleDatabytes(0x0, &dataadr, 1, &currentdatabyte);
			if (ok)
			{
				// Check if test mode is active:
				if (currentdatabyte & 0x20)
				{
					// Stop all actuator tests:
					for (k=0; k<_allActByteAddr.size(); k++)
					{
						ok =_SSMP2com->writeDatabyte(_allActByteAddr.at(k), 0x00);
						if (!ok) break;
					}
					_state = state_needSetup;	// MUST BE DONE AFTER ALL CALLS OF MEMBER-FUNCTIONS AND BEFORE EMITTING SIGNALS
					if (ok)
						emit stoppedActuatorTest();
				}
			}
		}
		// NOTE: DO NOT CALL stopCommOperation() or any other communicating functions here because of possible recursions !
		_state = state_needSetup;	// MUST BE DONE AFTER ALL CALLS OF MEMBER-FUNCTIONS AND BEFORE EMITTING SIGNALS
		delete _SSMP2com;
		_SSMP2com = NULL;
		// Emit stoppedXXX()-signals (_SSMP2com has been deleted, so we are sure they have finished):
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
	// RESET ECU RAW DATA:
	_SYS_ID[0] = 0;
	_SYS_ID[1] = 0;
	_SYS_ID[2] = 0;
	_ROM_ID[0] = 0;
	_ROM_ID[1] = 0;
	_ROM_ID[2] = 0;
	_ROM_ID[3] = 0;
	_ROM_ID[4] = 0;
	memset(_flagbytes, 0, 96);
	_nrofflagbytes = 0;
	// *** RESET BASIC DATA ***:
	// Reset DC-data:
	_DTCdefs.clear();
	_DTC_fmt_OBD2 = false;
	_CCCCdefs.clear();
	_memCCs_supported = false;
	// Reset MB/SW-data:
	_supportedMBs.clear();
	_supportedSWs.clear();
	// Reset adjustments-data:
	_adjustments.clear();
	// Reset actuator-test-data:
	_actuators.clear();
	_allActByteAddr.clear();
	// *** Reset selection data ***:
	_selectedDCgroups = noDCs_DCgroup;
	_MBSWmetaList.clear();
	_selMBsSWsAddr.clear();
	_selectedActuatorTestIndex = 255; // index ! => 0=first actuator !
	delete _SSM2defsIface;
	_SSM2defsIface = NULL;
}


SSMprotocol::CUsetupResult_dt SSMprotocol2::setupCUdata(CUtype_dt CU)
{
	return setupCUdata(CU, false);
}


SSMprotocol::CUsetupResult_dt SSMprotocol2::setupCUdata(CUtype_dt CU, bool ignoreIgnitionOFF)
{
	unsigned int CUaddress = 0x0;
	// Reset:
	resetCUdata();
	// Create SSMP2communication-object:
	if (CU == CUtype_Engine)
	{
		if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230)
		{
			CUaddress = 0x10;
		}
		else if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO15765)
		{
			CUaddress = 0x7E0;
		}
		else
			return result_invalidInterfaceConfig;
	}
	else if (CU == CUtype_Transmission)
	{
		if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230)
		{
			CUaddress = 0x18;
		}
		else if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO15765)
		{
			CUaddress = 0x7E1;
		}
		else
			return result_invalidInterfaceConfig;
	}
	else
		return result_invalidCUtype;
	_SSMP2com = new SSMP2communication(_diagInterface, CUaddress, 1);
	// Get control unit data:
	if (!_SSMP2com->getCUdata(_SYS_ID, _ROM_ID, _flagbytes, &_nrofflagbytes))
	{
		if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_SSM2_ISO14230)
		{
			_SSMP2com->setCUaddress(0x01);
			if (!_SSMP2com->getCUdata(_SYS_ID, _ROM_ID, _flagbytes, &_nrofflagbytes))
			{
				if (CU == CUtype_Engine)
				{
					_SSMP2com->setCUaddress(0x02);
					if (!_SSMP2com->getCUdata(_SYS_ID, _ROM_ID, _flagbytes, &_nrofflagbytes))
						goto commError;
				}
				else
					goto commError;
			}
		}
		else
			goto commError;
	}
	_SSMP2com->setRetriesOnError(2);
	// Ensure that ignition switch is ON:
	if ((_flagbytes[12] & 0x08) && !ignoreIgnitionOFF)
	{
		unsigned int dataaddr = 0x62;
		char data = 0x00;
		if (!_SSMP2com->readMultipleDatabytes('\x0', &dataaddr, 1, &data) || !(data & 0x08))
			goto commError;
	}
	_CU = CU;
	_state = state_normal;
	// Connect communication error signals from SSM2Pcommunication:
	connect( _SSMP2com, SIGNAL( commError() ), this, SIGNAL( commError() ) );
	connect( _SSMP2com, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
	/* Get definitions for this control unit */
	_SSM2defsIface = new SSM2definitionsInterface(_language);
	_SSM2defsIface->selectControlUnitID(_CU, _SYS_ID, _ROM_ID, _flagbytes, _nrofflagbytes);
	// Get definitions of the supported diagnostic codes:
	_SSM2defsIface->diagnosticCodes(&_DTCdefs, &_DTC_fmt_OBD2);
	_SSM2defsIface->cruiseControlCancelCodes(&_CCCCdefs, &_memCCs_supported);
	// Get supported MBs and SWs:
	_SSM2defsIface->measuringBlocks(&_supportedMBs);
	_SSM2defsIface->switches(&_supportedSWs);
	// Get supported Adaptions:
	_SSM2defsIface->adjustments(&_adjustments);
	// Get actuator test data:
	setupActuatorTestData();
	return result_success;
	
commError:
	delete _SSMP2com;
	_SSMP2com = NULL;
	return result_commError;
}


bool SSMprotocol2::getSystemDescription(QString *sysdescription)
{
	if (_state == state_needSetup) return false;
	return _SSM2defsIface->systemDescription(sysdescription);
}


bool SSMprotocol2::hasOBD2system(bool *OBD2)
{
	if (_state == state_needSetup) return false;
	return _SSM2defsIface->hasOBD2system(OBD2);
}


bool SSMprotocol2::hasVINsupport(bool *VINsup)
{
	if (_state == state_needSetup) return false;
	return _SSM2defsIface->hasVINsupport(VINsup);
}


bool SSMprotocol2::hasImmobilizer(bool *ImmoSup)
{
	if (_state == state_needSetup) return false;
	return _SSM2defsIface->hasImmobilizer(ImmoSup);
}


bool SSMprotocol2::hasIntegratedCC(bool *CCsup)
{
	if (_state == state_needSetup) return false;
	return _SSM2defsIface->hasIntegratedCC(CCsup);
}


bool SSMprotocol2::hasClearMemory(bool *CMsup)
{
	if (_state == state_needSetup) return false;
	return _SSM2defsIface->hasClearMemory(CMsup);
}


bool SSMprotocol2::hasClearMemory2(bool *CM2sup)
{
	if (_state == state_needSetup) return false;
	return _SSM2defsIface->hasClearMemory2(CM2sup);
}


bool SSMprotocol2::hasTestMode(bool *TMsup)
{
	if (_state == state_needSetup) return false;
	return _SSM2defsIface->hasTestMode(TMsup);
}


bool SSMprotocol2::hasActuatorTests(bool *ATsup)
{
	if (_state == state_needSetup) return false;
	return _SSM2defsIface->hasActuatorTests(ATsup);
}


void SSMprotocol2::setupActuatorTestData()
{
	bool aol = false;
	if (!_SSM2defsIface->actuatorTests(&_actuators))
		return;
	for (unsigned int n=0; n<_actuators.size(); n++)
	{
		// Check if byte address is already on the list:
		for (unsigned int m=0; m<_allActByteAddr.size(); m++)
		{
			if (_allActByteAddr.at(m) == _actuators.at(n).byteadr)
			{
				aol = true;
				break;
			}
		}
		// Put address on addresses list (if not duplicate):
		if (!aol)
			_allActByteAddr.push_back( _actuators.at(n).byteadr );
		else
			aol = false;
	}
}


bool SSMprotocol2::getSupportedDCgroups(int *DCgroups)
{
	int retDCgroups = 0;
	bool supported = false;
	if (_state == state_needSetup) return false;
	if (!_DTC_fmt_OBD2)
	{
		if (_DTCdefs.size())
			retDCgroups |= currentDTCs_DCgroup | historicDTCs_DCgroup;
	}
	else
	{
		if (_DTCdefs.size())
			retDCgroups |= temporaryDTCs_DCgroup | memorizedDTCs_DCgroup;
	}
	if (!_SSM2defsIface->hasIntegratedCC(&supported))
		return false;
	if (supported)
	{
		retDCgroups += CClatestCCs_DCgroup;
		if (_memCCs_supported)
			retDCgroups |= CCmemorizedCCs_DCgroup;
	}
	*DCgroups = retDCgroups;
	return true;
}


bool SSMprotocol2::getSupportedAdjustments(std::vector<adjustment_dt> *supportedAdjustments)
{
	if (_state == state_needSetup) return false;
	supportedAdjustments->clear();
	for (unsigned int k=0; k<_adjustments.size(); k++)
		supportedAdjustments->push_back( _adjustments.at(k) );
	return true;
}


bool SSMprotocol2::getSupportedActuatorTests(QStringList *actuatorTestTitles)
{
	unsigned char k = 0;
	bool ATsup = false;
	if (_SSM2defsIface->hasActuatorTests(&ATsup))
	{
		if (ATsup)
		{
			actuatorTestTitles->clear();
			for (k=0; k<_actuators.size(); k++)
				actuatorTestTitles->append(_actuators.at(k).title);
			return true;
		}
	}
	return false;
}


bool SSMprotocol2::getLastActuatorTestSelection(unsigned char *actuatorTestIndex)
{
	bool ATsup = false;
	if (_SSM2defsIface->hasActuatorTests(&ATsup))
	{
		if (ATsup && (_selectedActuatorTestIndex != 255))
		{
			*actuatorTestIndex = _selectedActuatorTestIndex;
			return true;
		}
	}
	return false;
}


bool SSMprotocol2::getAdjustmentValue(unsigned char index, unsigned int *rawValue)
{
	unsigned int dataaddr[2] = {0,0};
	unsigned int datalen = 0;
	char data[2] = {0,0};
	if (_state != state_normal) return false;
	// Validate adjustment value selection:
	if (_adjustments.empty() || (index >= _adjustments.size())) return false;
	// Convert memory address into two byte addresses:
	dataaddr[0] = _adjustments.at(index).AddrLow;
	datalen = 1;
	if (_adjustments.at(index).AddrHigh > 0)
	{
		dataaddr[1] = _adjustments.at(index).AddrLow;
		datalen++;
	}
	// Read data from control unit:
	if (!_SSMP2com->readMultipleDatabytes('\x0', dataaddr, datalen, data))
	{
		resetCUdata();
		return false;
	}
	// Calculate raw value:
	*rawValue = static_cast<unsigned char>(data[0]);
	if (_adjustments.at(index).AddrHigh > 0)
		*rawValue += 256*static_cast<unsigned char>(data[1]);
	return true;
}


bool SSMprotocol2::getAllAdjustmentValues(std::vector<unsigned int> * rawValues)
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
	if (!_SSMP2com->readMultipleDatabytes('\x0', &dataaddr.at(0), dataaddr.size(), &data.at(0)))
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


bool SSMprotocol2::setAdjustmentValue(unsigned char index, unsigned int rawValue)
{
	char lowdatabyte = 0;
	char highdatabyte = 0;
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
	// Convert raw value to 2 byte values:
	lowdatabyte = static_cast<char>(rawValue & 0xff);
	if (_adjustments.at(index).AddrHigh > 0)
		highdatabyte = static_cast<char>((rawValue & 0xffff) >> 8);
	// Write value to control unit:
	if (_adjustments.at(index).AddrHigh > 0)
	{
		if (!_SSMP2com->writeDatabyte(_adjustments.at(index).AddrHigh, highdatabyte))
		{
			resetCUdata();
			return false;
		}
	}
	if (!_SSMP2com->writeDatabyte(_adjustments.at(index).AddrLow, lowdatabyte))
	{
		resetCUdata();
		return false;
	}
	return true;
}


bool SSMprotocol2::getVIN(QString *VIN)
{
	bool VINsup = false;
	char vin[18] = {0,};
	char vinaddrdata[4] = {0,};
	unsigned int dataaddr[3];
	dataaddr[0] = 0xDA;
	dataaddr[1] = 0xDB;
	dataaddr[2] = 0xDC;
	unsigned int vinstartaddr = 0;
	unsigned int vinaddr[17] = {0,};
	unsigned int k = 0;
	if (!_SSM2defsIface->hasVINsupport(&VINsup))
		return false;
	if (!VINsup) return false;
	VIN->clear();
	if (_SSMP2com->readMultipleDatabytes(0x0, dataaddr, 3, vinaddrdata))
	{
		vinstartaddr = (65536 * static_cast<unsigned char>(vinaddrdata[0]))
				+ (256 * static_cast<unsigned char>(vinaddrdata[1]))
				+ (static_cast<unsigned char>(vinaddrdata[2]));
		for (k=1; k<=17; k++)
			vinaddr[k-1] = vinstartaddr+k-1;
		if (_SSMP2com->readMultipleDatabytes(0x0, vinaddr, 17, vin))
		{
			if (validateVIN(vin))
			{
				vin[17]='\x0';
				*VIN = static_cast<QString>(vin);
			}
			return true;
		}
	}
	// Communication error:
	resetCUdata();
	return false;
}


bool SSMprotocol2::isEngineRunning(bool *isrunning)
{
	unsigned int dataadr = 0x0e;
	char currentdatabyte = 0;
	if (_state != state_normal) return false;
	if (!(_flagbytes[0] & 0x01)) return false;
	if (!_SSMP2com->readMultipleDatabytes(0x0, &dataadr, 1, &currentdatabyte))
	{
		resetCUdata();
		return false;
	}
	if (currentdatabyte > 3)
		*isrunning = true;
	else
		*isrunning = false;
	return true;
}


bool SSMprotocol2::isInTestMode(bool *testmode)
{
	bool TMsup = false;
	unsigned int dataadr = 0x61;
	char currentdatabyte = 0;
	if (_state != state_normal) return false;
	if (!_SSM2defsIface->hasTestMode(&TMsup) || !TMsup) return false;
	if (!_SSMP2com->readMultipleDatabytes(0x0, &dataadr, 1, &currentdatabyte))
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


bool SSMprotocol2::clearMemory(CMlevel_dt level, bool *success)
{
	*success = false;
	char val = 0;
	char bytewritten = 0;
	bool CM2sup = false;
	if (_state != state_normal) return false;
	if (level == CMlevel_1)
	{
		val = 0x40;
	}
	else if (level == CMlevel_2)
	{
		if (!_SSM2defsIface->hasClearMemory2(&CM2sup) || CM2sup==false) return false;
		val = 0x20;
	}
	else
	{
		return false;
	}
	if (!_SSMP2com->writeDatabyte(0x000060, val, &bytewritten))
	{
		resetCUdata();
		return false;
	}
	*success = (bytewritten == val);
	return true;
}


bool SSMprotocol2::testImmobilizerCommLine(immoTestResult_dt *result)
{
	bool ImmoSup = false;
	if (_state != state_normal) return false;
	if (!_SSM2defsIface->hasImmobilizer(&ImmoSup) || ImmoSup==false) return false;
	char checkvalue = 0;
	unsigned int readcheckadr = 0x8B;
	// Write test-pattern:
	if (_SSMP2com->writeDatabyte(0xE0, '\xAA', &checkvalue))
	{
		// Read result:
		if (_SSMP2com->readMultipleDatabytes('\x0', &readcheckadr, 1, &checkvalue))
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
}


bool SSMprotocol2::startDCreading(int DCgroups)
{
	std::vector <unsigned int> DCqueryAddrList;
	unsigned char k = 0;
	bool CCsup = false;
	bool started;
	// Check if another communication operation is in progress:
	if (_state != state_normal) return false;
	// Try to determine the supported Cruise Control Cancel Code groups:
	if (!_SSM2defsIface->hasIntegratedCC(&CCsup))
		return false;
	// Check argument:
	if (DCgroups < 1 || DCgroups > 63) return false;
	if (((DCgroups & currentDTCs_DCgroup) || (DCgroups & historicDTCs_DCgroup)) && ((DCgroups & temporaryDTCs_DCgroup) || (DCgroups & memorizedDTCs_DCgroup)))
		return false;
	if (DCgroups > 15)
	{
		if (!CCsup)
			return false;
		if ( (DCgroups > 31) && (!_memCCs_supported) )
			return false;
	}
	// Setup diagnostic codes addresses list:
	if ((DCgroups & currentDTCs_DCgroup) || (DCgroups & temporaryDTCs_DCgroup))	// current/temporary DTCs
	{
		if (_CU == CUtype_Engine)
			DCqueryAddrList.push_back( 0x000061 );
		for (k=0; k<_DTCdefs.size(); k++)
			DCqueryAddrList.push_back( _DTCdefs.at(k).byteAddr_currentOrTempOrLatest );
	}	
	if ((DCgroups & historicDTCs_DCgroup) || (DCgroups & memorizedDTCs_DCgroup))	// historic/memorized DTCs
	{
		for (k=0; k<_DTCdefs.size(); k++)
			DCqueryAddrList.push_back( _DTCdefs.at(k).byteAddr_historicOrMemorized );
	}
	if (CCsup)
	{
		if (DCgroups & CClatestCCs_DCgroup)	// CC latest cancel codes
		{
			for (k=0; k<_CCCCdefs.size(); k++)
				DCqueryAddrList.push_back( _CCCCdefs.at(k).byteAddr_currentOrTempOrLatest );
		}
		if ((DCgroups & CCmemorizedCCs_DCgroup) && _memCCs_supported)	// CC memorized cancel codes
		{
			for (k=0; k<_CCCCdefs.size(); k++)
				DCqueryAddrList.push_back( _CCCCdefs.at(k).byteAddr_historicOrMemorized );
		}
	}
	// Check if min. 1 address to read:
	if ((DCqueryAddrList.size() < 1) || ((DCqueryAddrList.at(0) == 0x000061) && (DCqueryAddrList.size() < 2)))
		return false;
	// Start diagostic codes reading:
	started = _SSMP2com->readMultipleDatabytes_permanent('\x0', &DCqueryAddrList.at(0), DCqueryAddrList.size());
	if (started)
	{
		_state = state_DCreading;
		// Save diagnostic codes group selection (for data evaluation and restartDCreading()):
		_selectedDCgroups = DCgroups;
		// Connect signals and slots:
		connect( _SSMP2com, SIGNAL( recievedData(std::vector<char>, int) ),
			this, SLOT( processDCsRawdata(std::vector<char>, int) ), Qt::BlockingQueuedConnection );
		// Emit signal:
		emit startedDCreading();
	}
	else
		resetCUdata();
	return started;
}


bool SSMprotocol2::stopDCreading()
{
	if ((_state == state_needSetup) || (_state == state_normal)) return true;
	if (_state == state_DCreading)
	{
		if (_SSMP2com->stopCommunication())
		{
			disconnect( _SSMP2com, SIGNAL( recievedData(std::vector<char>, int) ),
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


void SSMprotocol2::processDCsRawdata(std::vector<char> DCrawdata, int duration_ms)
{
	(void)duration_ms; // to avoid compiler error
	QStringList DCs;
	QStringList DCdescriptions;
	QStringList tmpDTCs;
	QStringList tmpDTCsDescriptions;
	bool TestMode = false;
	bool DCheckActive = false;
	unsigned int DCsAddrIndex = 0;
	unsigned int DCsAddrIndexOffset = 0;
	if ((_selectedDCgroups & currentDTCs_DCgroup) || (_selectedDCgroups & temporaryDTCs_DCgroup))
	{
		if (_CU == CUtype_Engine)
		{
			DCsAddrIndexOffset = 1;
			if (_flagbytes[11] & 0x20)	// Test mode supported
			{
				if (DCrawdata.at(0) & 0x20)
					TestMode = true;
			}
			if (DCrawdata.at(0) & 0x80)
				DCheckActive = true;
		}
		else
			DCheckActive = false;
		DCs.clear();
		DCdescriptions.clear();
		// Evaluate current/latest data trouble codes:
		for (DCsAddrIndex=0; DCsAddrIndex<_DTCdefs.size(); DCsAddrIndex++)
		{
			evaluateDCdataByte(_DTCdefs.at(DCsAddrIndex).byteAddr_currentOrTempOrLatest, DCrawdata.at(DCsAddrIndexOffset+DCsAddrIndex), _DTCdefs, &tmpDTCs, &tmpDTCsDescriptions);
			DCs += tmpDTCs;
			DCdescriptions += tmpDTCsDescriptions;
		}
		DCsAddrIndexOffset += _DTCdefs.size();
		emit currentOrTemporaryDTCs(DCs, DCdescriptions, TestMode, DCheckActive);
	}
	if ((_selectedDCgroups & historicDTCs_DCgroup) || (_selectedDCgroups & memorizedDTCs_DCgroup))
	{
		DCs.clear();
		DCdescriptions.clear();
		// Evaluate historic/memorized data trouble codes:
		for (DCsAddrIndex=0; DCsAddrIndex<_DTCdefs.size(); DCsAddrIndex++)
		{
			evaluateDCdataByte(_DTCdefs.at(DCsAddrIndex).byteAddr_historicOrMemorized, DCrawdata.at(DCsAddrIndexOffset+DCsAddrIndex), _DTCdefs, &tmpDTCs, &tmpDTCsDescriptions);
			DCs += tmpDTCs;
			DCdescriptions += tmpDTCsDescriptions;
		}
		DCsAddrIndexOffset += _DTCdefs.size();
		emit historicOrMemorizedDTCs(DCs, DCdescriptions);
	}
	if (_selectedDCgroups == (_selectedDCgroups | CClatestCCs_DCgroup))
	{
		DCs.clear();
		DCdescriptions.clear();
		// Evaluate latest CC cancel codes:
		for (DCsAddrIndex=0; DCsAddrIndex<_CCCCdefs.size(); DCsAddrIndex++)
		{
			evaluateDCdataByte(_CCCCdefs.at(DCsAddrIndex).byteAddr_currentOrTempOrLatest, DCrawdata.at(DCsAddrIndexOffset+DCsAddrIndex), _CCCCdefs, &tmpDTCs, &tmpDTCsDescriptions);
			DCs += tmpDTCs;
			DCdescriptions += tmpDTCsDescriptions;
		}
		DCsAddrIndexOffset += _CCCCdefs.size();
		emit latestCCCCs(DCs, DCdescriptions);
	}
	if (_selectedDCgroups == (_selectedDCgroups | CCmemorizedCCs_DCgroup))
	{
		DCs.clear();
		DCdescriptions.clear();
		// Evaluate memorized CC cancel codes:
		for (DCsAddrIndex=0; DCsAddrIndex<_CCCCdefs.size(); DCsAddrIndex++)
		{
			evaluateDCdataByte(_CCCCdefs.at(DCsAddrIndex).byteAddr_historicOrMemorized, DCrawdata.at(DCsAddrIndexOffset+DCsAddrIndex), _CCCCdefs, &tmpDTCs, &tmpDTCsDescriptions);
			DCs += tmpDTCs;
			DCdescriptions += tmpDTCsDescriptions;
		}
		emit memorizedCCCCs(DCs, DCdescriptions);
	}
}


bool SSMprotocol2::startMBSWreading(std::vector<MBSWmetadata_dt> mbswmetaList)
{
	bool started = false;
	if (_state != state_normal) return false;
	// Setup list of MB/SW-addresses for SSM2Pcommunication:
	if (!setupMBSWQueryAddrList(mbswmetaList))
		return false;
 	// Start MB/SW-reading:
	started = _SSMP2com->readMultipleDatabytes_permanent('\x0', &_selMBsSWsAddr.at(0), _selMBsSWsAddr.size());
	if (started)
	{
		_state = state_MBSWreading;
		// Save MB/SW-selection (necessary for evaluation of raw data):
		_MBSWmetaList = mbswmetaList;
		// Connect signals/slots:
		connect( _SSMP2com, SIGNAL( recievedData(std::vector<char>, int) ),
			this, SLOT( processMBSWrawData(std::vector<char>, int) ) ); 
		// Emit signal:
		emit startedMBSWreading();
	}
	else
		resetCUdata();
	return started;
}


bool SSMprotocol2::stopMBSWreading()
{
	if ((_state == state_needSetup) || (_state == state_normal)) return true;
	if (_state == state_MBSWreading)
	{
		if (_SSMP2com->stopCommunication())
		{
			disconnect( _SSMP2com, SIGNAL( recievedData(std::vector<char>, int) ),
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


bool SSMprotocol2::startActuatorTest(unsigned char actuatorTestIndex)
{
	bool ATsup = false;
	bool ATstarted = false;
	bool ok = false;
	bool testmode = false;
	bool running = false;
	unsigned char k = 0;
	// Check if another communication operation is in progress:
	if (_state != state_normal) return false;
	// Validate selected test:
	ok = _SSM2defsIface->hasActuatorTests(&ATsup);
	if (!ok || (ATsup == false) || (actuatorTestIndex >= _actuators.size()))
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
		if (!_SSMP2com->writeDatabyte(_allActByteAddr.at(k), 0x00))
		{
			_state = state_normal; // this avoids that resetCUdata() will try to stop all actuators again
			resetCUdata();
			return false;
		}
	}
	// Start Actuator Test:
	ATstarted = _SSMP2com->writeDatabyte_permanent(dataaddr, databyte, 100);
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


bool SSMprotocol2::restartActuatorTest()
{
	return startActuatorTest(_selectedActuatorTestIndex);
}


bool SSMprotocol2::stopActuatorTesting()
{
	unsigned char k = 0;
	if ((_state == state_needSetup) || (_state == state_normal)) return true;
	if (_state == state_ActTesting)
	{
		if (_SSMP2com->stopCommunication())
		{
			// Stop all actuator tests:
			for (k=0; k<_allActByteAddr.size(); k++)
			{
				if (!_SSMP2com->writeDatabyte(_allActByteAddr.at(k), 0x00))
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


bool SSMprotocol2::stopAllActuators()
{
	// NOTE: This function can be called even if no actuator test has been started with SSMprotocol
	// => When switching the cars ignition on (with engine off) while test mode connector is connected,
	//    some actuator tests are started automaticly
	unsigned char k = 0;
	bool ok = false;
	bool ATsup = false;
	bool testmode = false;
	bool enginerunning = false;
	if (_state != state_normal) return false;
	// Check if actuator tests are supported:
	ok = _SSM2defsIface->hasActuatorTests(&ATsup);
	if (!ok || !ATsup)
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
	for (k=0; k<_allActByteAddr.size(); k++)
	{
		if (!_SSMP2com->writeDatabyte(_allActByteAddr.at(k), 0x00))
		{
			resetCUdata();
			return false;
		}
	}
	return true;
}


bool SSMprotocol2::waitForIgnitionOff()
{
	if (_state != state_normal)
		return false;
	unsigned int dataaddr = 0x62;
	_state = state_waitingForIgnOff;
	_SSMP2com->setRetriesOnError(1);
	if (_flagbytes[12] & 0x08)	// MB "ignition switch"
	{
		bool ignstate = true;
		char data = 0x00;
		do
		{
			if (!_SSMP2com->readMultipleDatabytes('\x0', &dataaddr, 1, &data))
				ignstate = false;
			else
				ignstate = (data & 0x08);
		} while (ignstate);
	}
	else
	{
		QEventLoop el;
		disconnect( _SSMP2com, SIGNAL( commError() ), this, SIGNAL( commError() ) );
		disconnect( _SSMP2com, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
		if(!_SSMP2com->readMultipleDatabytes_permanent('\x0', &dataaddr, 1))
		{
			resetCUdata();
			return false;
		}
		connect(_SSMP2com, SIGNAL( commError() ), &el, SLOT( quit() ));
		el.exec();
		disconnect(_SSMP2com, SIGNAL( commError() ), &el, SLOT( quit() ));
	}
	_SSMP2com->setRetriesOnError(2);
	resetCUdata();
	return true;
/* NOTE: temporary solution, will become obsolete with new SSMP2communication */
}


bool SSMprotocol2::validateVIN(char VIN[17])
{
	unsigned char k = 0;
	for (k=0; k<17; k++)
	{
		if ((VIN[k] < '\x30') || (VIN[k] > '\x39'))	// 0-9
		{
			if ((k > 10) || (VIN[k] < '\x41') || (VIN[k] > '\x5A')) // A-Z; NOTE: I,O,Q are not allowed
				return false;
		}
	}
	return true;
}

