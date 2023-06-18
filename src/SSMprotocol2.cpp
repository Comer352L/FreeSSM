/*
 * SSMprotocol2.cpp - Application Layer for the new Subaru SSM protocol
 *
 * Copyright (C) 2008-2023 Comer352L
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

#include "libFSSM.h"


SSMprotocol2::SSMprotocol2(AbstractDiagInterface *diagInterface, QString language) : SSMprotocol(diagInterface, language)
{
	_SSMP2com = NULL;
	resetCUdata();
}


SSMprotocol2::~SSMprotocol2()
{
	resetCUdata();
}


void SSMprotocol2::resetCUdata()
{
	// RESET COMMUNICATION:
	if (_SSMP2com != NULL)
	{
		// Disconnect communication error and data signals:
		disconnect( _SSMP2com, SIGNAL( commError() ), this, SIGNAL( commError() ) );
		disconnect( _SSMP2com, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
		disconnect( _SSMP2com, SIGNAL( receivedData(const std::vector<char>&, int) ),
				this, SLOT( processDCsRawdata(const std::vector<char>&, int) ) );
		disconnect( _SSMP2com, SIGNAL( receivedData(const std::vector<char>&, int) ),
				this, SLOT( processMBSWrawData(const std::vector<char>&, int) ) );
		// Try to stop active communication processes:
		if (_SSMP2com->stopCommunication() && (_state == state_ActTesting))
		{
			bool ok = false;
			// Stop all actuator tests:
			for (unsigned int k=0; k<_allActByteAddr.size(); k++)
			{
				ok =_SSMP2com->writeDatabyte(_allActByteAddr.at(k), 0x00);
				if (!ok) break;
			}
			_state = state_needSetup;	// MUST BE DONE AFTER ALL CALLS OF MEMBER-FUNCTIONS AND BEFORE EMITTING SIGNALS
			if (ok)
				emit stoppedActuatorTest();
		}
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
	// Reset control unit data
	resetCommonCUdata();
	_has_VINsupport = false;
	_has_integratedCC = false;
}


SSMprotocol::CUsetupResult_dt SSMprotocol2::setupCUdata(enum CUtype CU)
{
	return setupCUdata(CU, false);
}


SSMprotocol::CUsetupResult_dt SSMprotocol2::setupCUdata(enum CUtype CU, bool ignoreIgnitionOFF)
{
	unsigned int CUaddress = 0x0;
	SSMFlagbyteDefinitionsInterface *FBdefsIface;
	bool supported = false;

	// Reset:
	resetCUdata();
	// Update cached value of the active interface protocol:
	_ifceProtocol = _diagInterface->protocolType(); // NOTE: can't change until resetCUdata() has been called, because _SSMP2com has exclusive access to _diagInterface and doesn't change the protocol
	// Create SSMP2communication-object:
	if (CU == CUtype::Engine)
	{
		if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
		{
			CUaddress = 0x10;
		}
		else if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO15765)
		{
			CUaddress = 0x7E0;
		}
		else
			return result_invalidInterfaceConfig;
	}
	else if (CU == CUtype::Transmission)
	{
		if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
		{
			CUaddress = 0x18;
		}
		else if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO15765)
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
	if (!_SSMP2com->getCUdata(_ssmCUdata))
	{
		if (_diagInterface->protocolType() == AbstractDiagInterface::protocol_type::SSM2_ISO14230)
		{
			_SSMP2com->setCUaddress(0x01);
			if (!_SSMP2com->getCUdata(_ssmCUdata))
			{
				if (CU == CUtype::Engine)
				{
					_SSMP2com->setCUaddress(0x02);
					if (!_SSMP2com->getCUdata(_ssmCUdata))
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
	_SSMPcom = _SSMP2com;
	_CU = CU;
	_state = state_normal;
	// Connect communication error signals from SSM2Pcommunication:
	connect( _SSMP2com, SIGNAL( commError() ), this, SIGNAL( commError() ) );
	connect( _SSMP2com, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
	/* Get definitions for this control unit */
	FBdefsIface = new SSMFlagbyteDefinitionsInterface(_language);
	if (!FBdefsIface->selectControlUnitID(_CU, _ssmCUdata))
	{
		// NOTE: do not call resetCUdata(), because this will reset _state to state_needSetup which causes getSysID() + getROMID() to fail
		return result_noDefs;
	}
	FBdefsIface->systemDescription(&_sysDescription);
	FBdefsIface->hasOBD2system(&_has_OBD2);
	FBdefsIface->hasImmobilizer(&_has_Immo);
	FBdefsIface->hasImmobilizerTest(&_has_ImmoTest);
	FBdefsIface->hasActuatorTests(&_has_ActTest);
	if (FBdefsIface->hasClearMemory(&supported) && supported)
		FBdefsIface->clearMemoryData(&_CMaddr, &_CMvalue);
	supported = false;
	if (FBdefsIface->hasClearMemory2(&supported) && supported)
		FBdefsIface->clearMemory2Data(&_CM2addr, &_CM2value);
	FBdefsIface->hasVINsupport(&_has_VINsupport);
	FBdefsIface->hasIntegratedCC(&_has_integratedCC);
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
	// Ensure that ignition switch is ON:
	if ((_sw_ignitionstate_data.addr != MEMORY_ADDRESS_NONE) && !ignoreIgnitionOFF)
	{
		char data = 0x00;
		if (!_SSMP2com->readMultipleDatabytes('\x0', &_sw_ignitionstate_data.addr, 1, &data))
			goto commError;
		if (!((data & (1 << _sw_ignitionstate_data.bit)) ^ _sw_ignitionstate_data.inverted))
			goto commError;
	}
	// Prepare some internal data:
	determineSupportedDCgroups(_DTCblockData);
	setupActuatorTestAddrList();
	return result_success;

commError:
	resetCUdata();
	return result_commError;
}


bool SSMprotocol2::hasVINsupport(bool *VINsup)
{
	if (_state == state_needSetup) return false;
	*VINsup = _has_VINsupport;
	return true;
}


bool SSMprotocol2::hasIntegratedCC(bool *CCsup)
{
	if (_state == state_needSetup) return false;
	*CCsup = _has_integratedCC;
	return true;
}


bool SSMprotocol2::hasClearMemory2(bool *CM2sup)
{
	if (_state == state_needSetup) return false;
	*CM2sup = (_CM2addr != MEMORY_ADDRESS_NONE);
	return true;
}


bool SSMprotocol2::getVIN(QString *VIN)
{
	const int VINlength = 17;
	static const unsigned int dataaddr[3] = {0xDA, 0xDB, 0xDC};
	char vin[VINlength + 1] = {0,};
	char vinaddrdata[4] = {0,};
	unsigned int vinaddr[VINlength] = {0,};
	if (!_has_VINsupport)
		return false;
	VIN->clear();
	if (_SSMP2com->readMultipleDatabytes(0x0, dataaddr, 3, vinaddrdata))
	{
		vinaddr[0] = libFSSM::parseUInt24BigEndian(vinaddrdata);
		for (unsigned int k=1; k<VINlength; k++)
			vinaddr[k] = vinaddr[0] + k;
		if (_SSMP2com->readMultipleDatabytes(0x0, vinaddr, VINlength, vin))
		{
			if (validateVIN(vin))
			{
				vin[VINlength]='\x0';
				*VIN = static_cast<QString>(vin);
			}
			return true;
		}
	}
	// Communication error:
	resetCUdata();
	return false;
}

// PRIVATE

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

