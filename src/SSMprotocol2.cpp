/*
 * SSMprotocol2.cpp - Application Layer for the new Subaru SSM protocol
 *
 * Copyright (C) 2008-2010 Comer352l
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
	_CCCCdefs.clear();
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
}


SSMprotocol::CUsetupResult_dt SSMprotocol2::setupCUdata(CUtype_dt CU)
{
	return setupCUdata(CU, false);
}


SSMprotocol::CUsetupResult_dt SSMprotocol2::setupCUdata(CUtype_dt CU, bool ignoreIgnitionOFF)
{
	char CUaddress = 0;
	bool ATsup = false;
	bool CCsup = false;
	// Reset:
	resetCUdata();
	// Create SSMP2communication-object:
	if (CU == CUtype_Engine)
	{
		CUaddress = '\x10';
	}
	else if (CU == CUtype_Transmission)
	{
		CUaddress = '\x18';
	}
	else
		return result_invalidCUtype;
	_SSMP2com = new SSMP2communication(_diagInterface, CUaddress);
	// Get control unit data:
	if (!_SSMP2com->getCUdata(_SYS_ID, _ROM_ID, _flagbytes, &_nrofflagbytes))
		 return result_commError;
	// Ensure that ignition switch is ON:
	if ((_flagbytes[12] & 0x08) && !ignoreIgnitionOFF)
	{
		unsigned int dataaddr = 0x62;
		char data = 0x00;
		if (!_SSMP2com->readMultipleDatabytes('\x0', &dataaddr, 1, &data))
			return result_commError;
		if (!(data & 0x08))
			return result_commError;
	}
	_CU = CU;
	_state = state_normal;
	// Connect communication error signals from SSM2Pcommunication:
	connect( _SSMP2com, SIGNAL( commError() ), this, SIGNAL( commError() ) );
	connect( _SSMP2com, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
	// Get definitions of the supported diagnostic codes:
	setupDTCdata();
	hasIntegratedCC(&CCsup);
	if ( CCsup )	// not necessary, because function checks this, too...
		setupCCCCdata();
	// Get supported MBs and SWs:
	setupSupportedMBs();
	setupSupportedSWs();
	// Get supported Adaptions:
	setupAdjustmentsData();
	// Get actuator test data:
	hasActuatorTests(&ATsup);
	if (ATsup)
		setupActuatorTestData();
	return result_success;
}


bool SSMprotocol2::getSystemDescription(QString *sysdescription)
{
	if (_state == state_needSetup) return false;
	if (_CU == CUtype_Engine)
	{
		return getSysDescriptionBySysID( SSMprotocol2_ID::ECU_sysID, _SYS_ID, sysdescription );
	}
	else if (_CU == CUtype_Transmission)
	{
		return getSysDescriptionBySysID( SSMprotocol2_ID::TCU_sysID, _SYS_ID, sysdescription );
	}
	return false;
}


bool SSMprotocol2::hasOBD2system(bool *OBD2)
{
	if (_state == state_needSetup) return false;
	if (!(_flagbytes[29] & 0x80) && !(_flagbytes[28] & 0x02))
		*OBD2 = true;
	else
		*OBD2 = false;
	return true;
}


bool SSMprotocol2::hasVINsupport(bool *VINsup)
{
	if (_state == state_needSetup) return false;
	if ((_CU==CUtype_Engine) && (_nrofflagbytes > 32))
	{
		if (_flagbytes[36] & 0x01)
			*VINsup = true;
		else
			*VINsup = false;
	}
	else
		*VINsup = false;
	return true;
}


bool SSMprotocol2::hasImmobilizer(bool *ImmoSup)
{
	if (_state == state_needSetup) return false;
	if (_CU==CUtype_Engine && (_flagbytes[11] & 0x20))
	{
		if (_flagbytes[28] & 0x10)
			*ImmoSup = true;
		else
			*ImmoSup = false;
	}
	else
		*ImmoSup = false;

	return true;
}


bool SSMprotocol2::hasIntegratedCC(bool *CCsup)
{
	if (_state == state_needSetup) return false;
	if ((_CU==CUtype_Engine) && (_nrofflagbytes > 32))
	{
		if (_flagbytes[39] & 0x01)
			*CCsup = true;
		else
			*CCsup = false;
	}
	else
		*CCsup = false;
	return true;
}


bool SSMprotocol2::hasClearMemory(bool *CMsup)
{
	if (_state == state_needSetup) return false;
	*CMsup = true;
	return true;
}


bool SSMprotocol2::hasClearMemory2(bool *CM2sup)
{
	if (_state == state_needSetup) return false;
	if ((_CU==CUtype_Transmission) && (_nrofflagbytes > 32))
	{
		if (_flagbytes[39] & 0x02)
			*CM2sup = true;
		else
			*CM2sup = false;
	}
	else
		*CM2sup = false;
	return true;
}


bool SSMprotocol2::hasTestMode(bool *TMsup)
{
	if (_state == state_needSetup) return false;
	if ((_CU==CUtype_Engine) && (_flagbytes[11] & 0x20))
		*TMsup = true;
	else 
		*TMsup = false;
	return true;
}


bool SSMprotocol2::hasActuatorTests(bool *ATsup)
{
	bool TMsup = false;
	if (!hasTestMode(&TMsup))	// includes check of _status
		return false;
	*ATsup = false;
	if ((_CU==CUtype_Engine) && TMsup)
	{
		if (_flagbytes[28] & 0x40)
		{
			if (_flagbytes[0] & 0x01)
				*ATsup = true;
		}
	}
	return true;
}


void SSMprotocol2::setupDTCdata()
{
	unsigned int addr = 0;
	// Get raw DTC-definitions:
	QStringList rawDefs;	
	bool obdDTCs = !(_flagbytes[29] & 0x80);
	if (_language == "de")
	{
		SSMprotocol2_def_de rawdefs_de;
		if (obdDTCs)
			rawDefs = rawdefs_de.OBDDTCrawDefs();
		else
			rawDefs = rawdefs_de.SUBDTCrawDefs();
	}
	else
	{
		SSMprotocol2_def_en rawdefs_en;
		if (obdDTCs)
			rawDefs = rawdefs_en.OBDDTCrawDefs();
		else
			rawDefs = rawdefs_en.SUBDTCrawDefs();
	}
	// Setup data of the supported DTCs:
	_DTCdefs.clear();
	if (_flagbytes[29] & 0x80)
	{
		for (addr=0x8E; addr<=0x98; addr++)
			addDCdefs(addr, addr+22, rawDefs, &_DTCdefs);
		return;
	}
	else if ((_flagbytes[29] & 0x10) || (_flagbytes[29] & 0x40))
	{
		for (addr=0x8E; addr<=0xAD; addr++)
			addDCdefs(addr, addr+32, rawDefs, &_DTCdefs);
	}
	if (_flagbytes[28] & 0x01)
	{
		for (addr=0xF0; addr<=0xF3; addr++)
			addDCdefs(addr, addr+4, rawDefs, &_DTCdefs);
	}
	if (_nrofflagbytes > 32)
	{
		if (_flagbytes[39] & 0x80)
		{
			for (addr=0x123; addr<=0x12A; addr++)
				addDCdefs(addr, addr+8, rawDefs, &_DTCdefs);
		}
		if (_flagbytes[39] & 0x40)
		{
			for (addr=0x150; addr<=0x154; addr++)
				addDCdefs(addr, addr+5, rawDefs, &_DTCdefs);
		}
		if (_flagbytes[39] & 0x20)
		{
			for (addr=0x160; addr<=0x164; addr++)
				addDCdefs(addr, addr+5, rawDefs, &_DTCdefs);
		}
		if (_flagbytes[39] & 0x10)
		{
			for (addr=0x174; addr<=0x17A; addr++)
				addDCdefs(addr, addr+7, rawDefs, &_DTCdefs);
		}
		if (_nrofflagbytes > 48)
		{
			if (_flagbytes[50] & 0x40)
			{
				for (addr=0x1C1; addr<=0x1C6; addr++)
					addDCdefs(addr, addr+6, rawDefs, &_DTCdefs);
				for (addr=0x20A; addr<=0x20D; addr++)
					addDCdefs(addr, addr+4, rawDefs, &_DTCdefs);
			}
			if (_flagbytes[50] & 0x20)
			{
				for (addr=0x263; addr<=0x267; addr++)
					addDCdefs(addr, addr+5, rawDefs, &_DTCdefs);
			}
		}
	}
}


void SSMprotocol2::setupCCCCdata()
{
	bool supported = false;
	unsigned int addr = 0;
	// Check if CU is equipped with CC:
	hasIntegratedCC(&supported);
	if (!supported) return;
	// Get raw CCCC-definitions:
	QStringList CCrawDefs;
	if (_language == "de")
	{
		SSMprotocol2_def_de rawdefs_de;
		CCrawDefs = rawdefs_de.CCCCrawDefs();
	}
	else
	{
		SSMprotocol2_def_en rawdefs_en;
		CCrawDefs = rawdefs_en.CCCCrawDefs();
	}
	// Setup data of the supported CCCCs:
	_CCCCdefs.clear();
	for (addr=0x133; addr<=0x136; addr++)
		addDCdefs(addr, addr+4, CCrawDefs,  &_CCCCdefs);
}


void SSMprotocol2::setupSupportedMBs()
{
	QString mbdefline;
	QString tmpstr;
	int tmpbytenr = 0;
	int tmpbitnr = 0;
	bool tmpCUsupported = false;
	int tmpprecision = 0;
	mb_intl_dt tmpMB;
	bool ok = false;
	int k = 0;

	_supportedMBs.clear();
	// Select definitions depending on language:
	QStringList mbrawdata;
	if (_language == "de")
	{
		SSMprotocol2_def_de rawdefs_de;
		mbrawdata = rawdefs_de.MBrawDefs();
	}
	else
	{
		SSMprotocol2_def_en rawdefs_en;
		mbrawdata = rawdefs_en.MBrawDefs();
	}
	// Assort list of supported MBs:
	for (k=0; k<mbrawdata.size(); k++)
	{
		// Get flagbyte address definition:
		mbdefline = mbrawdata.at(k);
		tmpstr = mbdefline.section(';', 0, 0);
		tmpbytenr = tmpstr.toInt(&ok, 10);
		// Check if flagbyte address is defined for our CU:
		if (ok && (tmpbytenr <= _nrofflagbytes))
		{
			// Get flagbit definition:
			tmpstr = mbdefline.section(';', 1, 1);
			tmpbitnr = tmpstr.toInt();
			// Check if CU supports this MB (if flagbit is set):
			if ((tmpbitnr < 1) || (tmpbitnr > 8))
				ok = false;
			if ((ok) && (_flagbytes[tmpbytenr-1] & static_cast<unsigned char>(pow(2, (tmpbitnr-1)))))
			{
				// Check if MB is intended for this CU type:
				tmpCUsupported = 0;
				tmpstr = mbdefline.section(';', 2, 2);
				tmpCUsupported = tmpstr.toInt();
				if (_CU == CUtype_Engine)
				{
					tmpCUsupported = tmpstr.toUInt() & 0x01;
				}
				else if (_CU == CUtype_Transmission)
				{
					tmpCUsupported = tmpstr.toUInt() & 0x02;
				}
				if (tmpCUsupported == 1)
				{
					// Get memory address (low) definition:
					tmpstr = mbdefline.section(';', 3, 3);
					tmpMB.addr_low = tmpstr.toUInt(&ok, 16);
					// Check if memory address (low) is valid:
					if (ok && (tmpMB.addr_low > 0))
					{
						// Get memory address (high) definition:
						tmpstr = mbdefline.section(';', 4, 4);
						if (tmpstr.isEmpty())
							tmpMB.addr_high = MEMORY_ADDRESS_NONE;
						else
							tmpMB.addr_high = tmpstr.toUInt(&ok, 16);
						if (ok)
						{
							// Get title definition:
							tmpMB.title = mbdefline.section(';', 5, 5);
							// Check if title is available:
							if (!tmpMB.title.isEmpty())
							{
								// Get scaling formula definition:
								tmpMB.scaleformula = mbdefline.section(';', 7, 7);
								// Check if scaling formula is available:
								if (!tmpMB.scaleformula.isEmpty())
								{
									// Get precision and correct if necessary:
									tmpstr = mbdefline.section(';', 8, 8);
									tmpprecision = tmpstr.toInt(&ok, 10);
									if ((ok) && (tmpprecision < 4) && (tmpprecision > -4))
										tmpMB.precision = tmpprecision;
									else
										tmpMB.precision = 1;
									// Get unit:
									tmpMB.unit = mbdefline.section(';', 6, 6);
									// ***** MB IS SUPPORTED BY CU AND DEFINITION IS VALID *****
									// Put MB data on the list:
									_supportedMBs.push_back(tmpMB);
								}
							}
						}
					}
				}
			}
		}
	}
}


void SSMprotocol2::setupSupportedSWs()
{
	QString swdefline;
	QString tmpstr;
	int tmpbytenr = 0;
	int tmpbitnr = 0;
	bool tmpCUsupported = false;
	sw_intl_dt tmpSW;
	bool ok = false;
	int k = 0;

	_supportedSWs.clear();
	// Select definitions depending on language:
	QStringList swrawdata;
	if (_language == "de")
	{
		SSMprotocol2_def_de rawdefs_de;
		swrawdata = rawdefs_de.SWrawDefs();
	}
	else
	{
		SSMprotocol2_def_en rawdefs_en;
		swrawdata = rawdefs_en.SWrawDefs();
	}
	// Assort list of supported switches:
	for (k=0; k<swrawdata.size(); k++)
	{
		// Get flagbyte address definition:
		swdefline = swrawdata.at(k);
		tmpstr = swdefline.section(';', 0, 0);
		tmpbytenr = tmpstr.toInt(&ok, 10);
		// Check if flagbyte address is defined for our CU:
		if (ok && (tmpbytenr <= _nrofflagbytes))
		{
			// Get flagbit definition:
			tmpstr = swdefline.section(';', 1, 1);
			tmpbitnr = tmpstr.toInt();
			// Check if CU supports this switch (if flagbit is set):
			if ((tmpbitnr < 1) || (tmpbitnr > 8))
				ok = false;
			if ((ok) && (_flagbytes[tmpbytenr-1] & static_cast<unsigned char>(pow(2, (tmpbitnr-1)) )))
			{
				// Check if switch is intended for this CU type:
				tmpCUsupported = 0;
				tmpstr = swdefline.section(';', 2, 2);
				tmpCUsupported = tmpstr.toInt();
				if (_CU == CUtype_Engine)
				{
					tmpCUsupported = tmpstr.toUInt() & 0x01;
				}
				else if (_CU == CUtype_Transmission)
				{
					tmpCUsupported = tmpstr.toUInt() & 0x02;
				}
				if (tmpCUsupported == 1)
				{
					// Get memory address definition:
					tmpstr = swdefline.section(';', 3, 3);
					tmpSW.byteAddr = tmpstr.toInt(&ok, 16);
					// Check if memory address is valid:
					if (ok && (tmpSW.byteAddr > 0))
					{
						// Get title definition:
						tmpSW.title = swdefline.section(';', 4, 4);
						// Check if title is available:
						if (!tmpSW.title.isEmpty())
						{
							// Get unit definition:
							tmpSW.unit = swdefline.section(';', 5, 5);
							if (!tmpSW.unit.isEmpty())
							{
								// ***** SWITCH IS SUPPORTED BY CU AND DEFINITION IS VALID *****
								// Put switch data on the list:
								tmpSW.bitAddr = static_cast<unsigned char>(tmpbitnr);
								_supportedSWs.push_back(tmpSW);
							}
						}
					}
				}
			}
		}
	}
}


void SSMprotocol2::setupAdjustmentsData()
{
	QString defline = "";
	QString tmphelpstr = "";
	unsigned int tmpflagbyte = 0;
	unsigned int tmpflagbit = 0;
	unsigned int tmpsidbval = 0;
	unsigned int tmpCU = 0;
	adjustment_intl_dt tmpadjustment;
	int k = 0;
	bool ok = false;
	bool supported = false;
	QStringList adjustmentsrawdata;

	_adjustments.clear();
	if (_language == "de")
	{
		SSMprotocol2_def_de rawdefs_de;
		adjustmentsrawdata = rawdefs_de.AdjustmentRawDefs();
	}
	else
	{
		SSMprotocol2_def_en rawdefs_en;
		adjustmentsrawdata = rawdefs_en.AdjustmentRawDefs();
	}
	for (k=0; k<adjustmentsrawdata.size(); k++)
	{
		defline = adjustmentsrawdata.at(k);
		tmphelpstr = defline.section(';', 0, 0);
		supported = false;
		if (tmphelpstr.count('-') == 1)
		{
			tmpflagbyte = tmphelpstr.section('-', 0, 0).toUInt(&ok, 10);
			if (ok && (tmpflagbyte > 0) && (tmpflagbyte < _nrofflagbytes))
			{
				tmpflagbit = tmphelpstr.section('-', 1, 1).toUInt(&ok, 10);
				if (ok && (tmpflagbit > 0) && (tmpflagbit < 9))
				{
					if (_flagbytes[tmpflagbyte-1] & static_cast<unsigned char>(pow(2,(tmpflagbit-1))))
						supported = true;
				}
			}
		}
		else if (tmphelpstr.size() == 6)
		{
			tmpsidbval = tmphelpstr.mid(0, 2).toUInt(&ok, 16);
			if (ok && (tmpsidbval == static_cast<unsigned char>(_SYS_ID[0])))
			{
				tmpsidbval = tmphelpstr.mid(2, 2).toUInt(&ok, 16);
				if (ok && (tmpsidbval == static_cast<unsigned char>(_SYS_ID[1])))
				{
					tmpsidbval = tmphelpstr.mid(4, 2).toUInt(&ok, 16);
					if (ok && (tmpsidbval == static_cast<unsigned char>(_SYS_ID[2])))
						supported = true;
				}
			}
		}
		if (supported)
		{
			tmpCU = defline.section(';', 1, 1).toUInt(&ok, 10);
			if (ok)
			{
				if ( ((_CU == CUtype_Engine) && (tmpCU == 0)) || ((_CU == CUtype_Transmission) && (tmpCU == 1)) )
				{
					tmpadjustment.AddrLow = defline.section(';', 2, 2).toUInt(&ok, 16);
					if (ok && (tmpadjustment.AddrLow > 0))
					{
						tmpadjustment.AddrHigh = defline.section(';', 3, 3).toUInt(&ok, 16);
						if (!ok || (tmpadjustment.AddrHigh < 1))
							tmpadjustment.AddrHigh = 0;
						tmpadjustment.title = defline.section(';', 4, 4);
						if (tmpadjustment.title.length() > 0)
						{
							tmpadjustment.unit = defline.section(';', 5, 5);	// may be empty
							tmphelpstr = defline.section(';', 6, 6);
							tmpadjustment.rawMin = tmphelpstr.toUInt(&ok, 10);
							if (ok)
							{
								tmphelpstr = defline.section(';', 7, 7);
								tmpadjustment.rawMax = tmphelpstr.toUInt(&ok, 10);
								if (ok)
								{
									tmphelpstr = defline.section(';', 8, 8);
									tmpadjustment.rawDefault = tmphelpstr.toUInt(&ok, 10);
									if (ok)
									{
										tmpadjustment.formula = defline.section(';', 9, 9);
										if (tmpadjustment.formula.length() > 0)
										{
											tmphelpstr = defline.section(';', 10, 10);
											if (tmphelpstr.length() == 0)
												tmphelpstr = "0";
											tmpadjustment.precision = tmphelpstr.toInt(&ok, 10);
											if (ok && (tmpadjustment.precision<=10) && (tmpadjustment.precision>=-10))
												_adjustments.push_back(tmpadjustment);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}


void SSMprotocol2::setupActuatorTestData()
{
	QString tmpstr = "";
	unsigned int tmpflagbyte = 0;
	unsigned int tmpflagbit = 0;
	actuator_dt tmpactuator;
	unsigned char tmpbitadr = 0;
	bool fbvalid = true;
	bool ok = false;
	bool aol = false;
	int k = 0;
	unsigned int m = 0;
	QStringList actuatorsrawdata;

	_actuators.clear();
	_allActByteAddr.clear();
	if (_language == "de")
	{
		SSMprotocol2_def_de rawdefs_de;
		actuatorsrawdata = rawdefs_de.ActuatorRawDefs();
	}
	else
	{
		SSMprotocol2_def_en rawdefs_en;
		actuatorsrawdata = rawdefs_en.ActuatorRawDefs();
	}
	for (k=0; k<actuatorsrawdata.size(); k++)
	{
		fbvalid = true;
		tmpstr = actuatorsrawdata.at(k).section(';', 0, 0);
		if (tmpstr.size() > 0)
		{
			tmpflagbyte = tmpstr.toUInt();
			if ((tmpflagbyte < 1) || (tmpflagbyte > _nrofflagbytes))
				fbvalid = false;
		}
		if (fbvalid)
		{
			tmpstr = actuatorsrawdata.at(k).section(';', 1, 1);
			if ((tmpstr.size() > 0) == (tmpflagbyte == 0))//Flagbit definition existiert UND zulässige Flagbytedefinition existiert
			{
				fbvalid = false;
			}
			else if (tmpstr.size() > 0)
			{
				tmpflagbit = tmpstr.toUInt();
				if ((tmpflagbit < 1) || (tmpflagbit > 8))
					fbvalid = false;
				else
				{
					// Check if flagbyte is set:
					if (!(_flagbytes[tmpflagbyte-1] & static_cast<unsigned char>(pow(2, tmpflagbit-1))))
						fbvalid = false;
				}
			}
		}
		if (fbvalid)
		{
			tmpactuator.byteadr = actuatorsrawdata.at(k).section(';', 2, 2).toUInt(&ok, 16);
			if (ok && (tmpactuator.byteadr > 0))
			{
				tmpbitadr = actuatorsrawdata.at(k).section(';', 3, 3).toUInt();
				if ((tmpbitadr > 0) && (tmpbitadr < 9))
				{
					tmpactuator.title = actuatorsrawdata.at(k).section(';', 4, 4);
					if (tmpactuator.title.length() > 0)
					{
						tmpactuator.bitadr = tmpbitadr;
						_actuators.push_back( tmpactuator );
						// Check if byte address is already on the list:
						for (m=0; m<_allActByteAddr.size(); m++)
						{
							if (_allActByteAddr.at(m) == tmpactuator.byteadr)
							{
								aol = true;
								break;
							}
						}
						// Put address on addresses list (if not duplicate):
						if (!aol)
							_allActByteAddr.push_back( tmpactuator.byteadr );
						else
							aol = false;
					}
				}
			}
		}
	}
}


void SSMprotocol2::addDCdefs(unsigned int currOrTempOrLatestDCsAddr, unsigned int histOrMemDCsAddr, QStringList rawDefs, std::vector<dc_defs_dt> * defs)
{
	dc_defs_dt tmpdef;
	QStringList tmpdefparts;
	unsigned int tmpdtcaddr_1 = 0;
	unsigned int tmpdtcaddr_2 = 0;
	unsigned int tmpbitaddr = 0;
	bool ok = false;

	tmpdef.byteAddr_currentOrTempOrLatest = currOrTempOrLatestDCsAddr;
	tmpdef.byteAddr_historicOrMemorized = histOrMemDCsAddr;
	for (unsigned char k=0; k<8; k++)
	{
		tmpdef.code[k] = "???";
		if (_language == "de")
			tmpdef.title[k] = "UNBEKANNT (Adresse 0x";
		else
			tmpdef.title[k] = "UNKNOWN (Address 0x";
		tmpdef.title[k] += QString::number(currOrTempOrLatestDCsAddr,16).toUpper() + "/0x" + QString::number(histOrMemDCsAddr,16).toUpper() + " Bit " + QString::number(k+1) + ")";
	}
	for (int m=0; m<rawDefs.size(); m++)
	{
		tmpdefparts = rawDefs.at(m).split(';');
		if (tmpdefparts.size() == 5)
		{
			tmpdtcaddr_1 = tmpdefparts.at(0).toUInt(&ok, 16);  // current/temporary/latest/D-Check DCs memory address
			tmpdtcaddr_2 = tmpdefparts.at(1).toUInt(&ok, 16);  // historic/memorized DCs memory address
			if ((ok) && (tmpdtcaddr_1 == currOrTempOrLatestDCsAddr) && (tmpdtcaddr_2 == histOrMemDCsAddr))
			{
				tmpbitaddr = tmpdefparts.at(2).toUInt(); // flagbit
				tmpdef.code[ tmpbitaddr-1 ] = tmpdefparts.at(3);
				tmpdef.title[ tmpbitaddr-1 ] = tmpdefparts.at(4);
			}
		}
	}
	defs->push_back(tmpdef);
	/* NOTE:	- DCs with missing definitions are displayed as UNKNOWN
			- DCs with existing definition and empty code- and title-fields are ignored */
}


bool SSMprotocol2::getSupportedDCgroups(int *DCgroups)
{
	int retDCgroups = 0;
	bool supported = false;
	if (_state == state_needSetup) return false;
	if (_flagbytes[29] & 0x80)
	{
		if (_DTCdefs.size())
			retDCgroups |= currentDTCs_DCgroup | historicDTCs_DCgroup;
	}
	else
	{
		if (_DTCdefs.size())
			retDCgroups |= temporaryDTCs_DCgroup | memorizedDTCs_DCgroup;
	}
	if (!hasIntegratedCC(&supported))
		return false;
	if (supported)
	{
		retDCgroups += CClatestCCs_DCgroup;
		if (_flagbytes[41] & 0x04)
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
	if (hasActuatorTests(&ATsup))
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
	if (hasActuatorTests(&ATsup))
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
	if (!hasVINsupport(&VINsup))
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
	if (!hasTestMode(&TMsup) || !TMsup) return false;
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
		if (!hasClearMemory2(&CM2sup) || CM2sup==false) return false;
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
	if (!hasImmobilizer(&ImmoSup) || ImmoSup==false) return false;
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
	bool CCmemSup = false;
	bool started;
	// Check if another communication operation is in progress:
	if (_state != state_normal) return false;
	// Try to determine the supported Cruise Control Cancel Code groups:
	if (hasIntegratedCC(&CCsup))
		CCmemSup = (_flagbytes[41] & 0x04);
	else
		return false;
	// Check argument:
	if (DCgroups < 1 || DCgroups > 63) return false;
	if (((DCgroups & currentDTCs_DCgroup) || (DCgroups & historicDTCs_DCgroup)) && ((DCgroups & temporaryDTCs_DCgroup) || (DCgroups & memorizedDTCs_DCgroup)))
		return false;
	if (DCgroups > 15)
	{
		if (!CCsup)
			return false;
		if ( (DCgroups > 31) && (!CCmemSup) )
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
		if ((DCgroups & CCmemorizedCCs_DCgroup) && CCmemSup)	// CC memorized cancel codes
		{
			for (k=0; k<_CCCCdefs.size(); k++)
				DCqueryAddrList.push_back( _CCCCdefs.at(k).byteAddr_historicOrMemorized );
		}
	}
	// Check if min. 1 Address to read:
	if ((DCqueryAddrList.size() < 1) || ((DCqueryAddrList.at(0) == 0x000061) && (DCqueryAddrList.size() < 2)))
		return false;
	// Start diagostic code reading:
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
	QStringList DCs;
	QStringList DCdescriptions;
	QStringList tmpDTCs;
	QStringList tmpDTCsDescriptions;
	bool TestMode = false;
	bool DCheckActive = false;
	unsigned int DCsAddrIndex = 0;
	unsigned int DCsAddrIndexOffset = 0;
	duration_ms = 0; // to avoid compiler error
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
	ok = hasActuatorTests(&ATsup);
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
	ok = hasActuatorTests(&ATsup);
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

