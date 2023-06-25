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
	resetCUdata();
}


SSMprotocol1::~SSMprotocol1()
{
	resetCUdata();
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
	SSMP1communication *SSMP1com = new SSMP1communication(_diagInterface, SSM1_CU);
	// Get control unit ID:
	bool ok = SSMP1com->getCUdata(0, _ssmCUdata);
	if (!ok && (CU == CUtype::AirCon))
	{
		SSMP1com->selectCU(SSM1_CU_AirCon2);
		ok = SSMP1com->getCUdata(0, _ssmCUdata);
	}
	if (!ok)
	{
		delete SSMP1com;
		SSMP1com = NULL;
		return result_commError;
	}
	/* NOTE: no need to check the state of the ignition switch.
	 * Power supply of these old control units is immediately cut when ignition is switched off */
	_SSMPcom = SSMP1com;
	_CU = CU;
	_state = state_normal;
	// Connect communication error signals from SSMP1communication:
	connect( _SSMPcom, SIGNAL( commError() ), this, SIGNAL( commError() ) );
	connect( _SSMPcom, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
	/* Load control unit definitions */
	if (_ssmCUdata.uses_Flagbytes())
	{
		SSMFlagbyteDefinitionsInterface *FBdefsIface;
		// Request flag bytes:
		if (!SSMP1com->getCUdata(32, _ssmCUdata))
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
		FBdefsIface->hasImmobilizerTest(&_has_ImmoTest);
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
		LegacyDefsIface->actuatorTests(&_actuators);
		_has_ActTest = _actuators.size() > 0;
		LegacyDefsIface->clearMemoryData(&_CMaddr, &_CMvalue);
		_SSMdefsIfce = LegacyDefsIface;
	}
	determineSupportedDCgroups(_DTCblockData);
	return result_success;
}


// PRIVATE

bool SSMprotocol1::readExtendedID(std::vector<char>& ID)
{
	std::vector<unsigned int> addresses;
	for (unsigned int addr=0x01; addr<=0x05; addr++)
		addresses.push_back(addr);
	if (!_SSMPcom->readAddresses(addresses, &ID))
		return false;
	return true;
}

