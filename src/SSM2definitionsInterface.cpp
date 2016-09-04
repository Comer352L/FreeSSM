/*
 * SSM2definitionsInterface.cpp - Interface to the SSM2-definitions
 *
 * Copyright (C) 2008-2016 Comer352L
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

#include "SSM2definitionsInterface.h"



SSM2definitionsInterface::SSM2definitionsInterface(QString language)
{
	_language = language;
	_id_set = false;
	_CU = SSMprotocol::CUtype_Engine;
}


SSM2definitionsInterface::~SSM2definitionsInterface()
{
}


void SSM2definitionsInterface::setLanguage(QString lang)
{
	_language = lang;
}


void SSM2definitionsInterface::selectControlUnitID(SSMprotocol::CUtype_dt cu, const SSMCUdata& ssmCUdata)
{
	_CU = cu;
	_ssmCUdata = ssmCUdata;
	_id_set = true;
}


bool SSM2definitionsInterface::systemDescription(QString *description)
{
	if (!_id_set)
		return false;
	if (_CU == SSMprotocol::CUtype_Engine)
	{
		return getSysDescriptionBySysID( SSMprotocol2_ID::ECU_sysID, _ssmCUdata.SYS_ID, description );
	}
	else if (_CU == SSMprotocol::CUtype_Transmission)
	{
		return getSysDescriptionBySysID( SSMprotocol2_ID::TCU_sysID, _ssmCUdata.SYS_ID, description );
	}
	else
		return false;
}


bool SSM2definitionsInterface::diagnosticCodes(std::vector<dc_defs_dt> *diagnosticCodes, bool *fmt_OBD2)
{
	unsigned int addr = 0;
	QStringList rawDefs;

	if (!_id_set)
		return false;
	if ((_CU != SSMprotocol::CUtype_Engine) && (_CU != SSMprotocol::CUtype_Transmission))
		return false;
	*fmt_OBD2 = !_ssmCUdata.flagbytebit(29, 7);
	if (_language == "de")
	{
		SSMprotocol2_def_de rawdefs_de;
		if (*fmt_OBD2)
			rawDefs = rawdefs_de.OBDDTCrawDefs();
		else
			rawDefs = rawdefs_de.SUBDTCrawDefs();
	}
	else
	{
		SSMprotocol2_def_en rawdefs_en;
		if (*fmt_OBD2)
			rawDefs = rawdefs_en.OBDDTCrawDefs();
		else
			rawDefs = rawdefs_en.SUBDTCrawDefs();
	}
	// Setup data of the supported DTCs:
	diagnosticCodes->clear();
	if (!*fmt_OBD2)
	{
		for (addr=0x8E; addr<=0x98; addr++)
			addDCdefs(addr, addr+22, rawDefs, diagnosticCodes);
		return true;
	}
	else if (_ssmCUdata.flagbytebit(29, 4) || _ssmCUdata.flagbytebit(29, 6))
	{
		for (addr=0x8E; addr<=0xAD; addr++)
			addDCdefs(addr, addr+32, rawDefs, diagnosticCodes);
	}
	if (_ssmCUdata.flagbytebit(28, 0))
	{
		for (addr=0xF0; addr<=0xF3; addr++)
			addDCdefs(addr, addr+4, rawDefs, diagnosticCodes);
	}
	if (_ssmCUdata.flagbytescount() > 32)
	{
		if (_ssmCUdata.flagbytebit(39, 7))
		{
			for (addr=0x123; addr<=0x12A; addr++)
				addDCdefs(addr, addr+8, rawDefs, diagnosticCodes);
		}
		if (_ssmCUdata.flagbytebit(39, 6))
		{
			for (addr=0x150; addr<=0x154; addr++)
				addDCdefs(addr, addr+5, rawDefs, diagnosticCodes);
		}
		if (_ssmCUdata.flagbytebit(39, 5))
		{
			for (addr=0x160; addr<=0x164; addr++)
				addDCdefs(addr, addr+5, rawDefs, diagnosticCodes);
		}
		if (_ssmCUdata.flagbytebit(39, 4))
		{
			for (addr=0x174; addr<=0x17A; addr++)
				addDCdefs(addr, addr+7, rawDefs, diagnosticCodes);
		}
		if (_ssmCUdata.flagbytescount() > 48)
		{
			if (_ssmCUdata.flagbytebit(50, 6))
			{
				for (addr=0x1C1; addr<=0x1C6; addr++)
					addDCdefs(addr, addr+6, rawDefs, diagnosticCodes);
				for (addr=0x20A; addr<=0x20D; addr++)
					addDCdefs(addr, addr+4, rawDefs, diagnosticCodes);
			}
			if (_ssmCUdata.flagbytebit(50, 5))
			{
				for (addr=0x263; addr<=0x267; addr++)
					addDCdefs(addr, addr+5, rawDefs, diagnosticCodes);
			}
		}
	}
	return true;
}


bool SSM2definitionsInterface::cruiseControlCancelCodes(std::vector<dc_defs_dt> *cancelCodes, bool *memCC_supported)
{
	unsigned int addr = 0;
	bool CCsup = false;
	// Check if CU is equipped with CC:
	if (!hasIntegratedCC( &CCsup ))
		return false;
	cancelCodes->clear();
	if (!CCsup)
		return true;
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
	for (addr=0x133; addr<=0x136; addr++)
		addDCdefs(addr, addr+4, CCrawDefs, cancelCodes);
	*memCC_supported = _ssmCUdata.flagbytebit(41, 2);
	return true;
}


bool SSM2definitionsInterface::measuringBlocks(std::vector<mb_intl_dt> *measuringblocks)
{
	QString mbdefline;
	QString tmpstr;
	int tmpprecision = 0;
	mb_intl_dt tmpMB;
	bool ok = false;
	int k = 0;

	if (!_id_set)
		return false;
	if ((_CU != SSMprotocol::CUtype_Engine) && (_CU != SSMprotocol::CUtype_Transmission))
		return false;
	measuringblocks->clear();
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
		unsigned int tmpbytenr = tmpstr.toUInt(&ok, 10);
		// Check if flagbyte is supported by our CU:
		if (!ok || (tmpbytenr > _ssmCUdata.flagbytescount()))
			continue;
		// Get flagbit definition:
		tmpstr = mbdefline.section(';', 1, 1);
		unsigned int tmpbitnr = tmpstr.toUInt();
		// Check if CU supports this MB (if flagbit is set):
		if ((tmpbitnr < 1) || (tmpbitnr > 8))
			continue;
		if (!_ssmCUdata.flagbytebit(tmpbytenr-1, tmpbitnr-1))
			continue;
		// Check if MB is intended for this CU type:
		tmpstr = mbdefline.section(';', 2, 2);
		if (!((_CU == SSMprotocol::CUtype_Engine) && (tmpstr.toUInt() & 0x01)) &&
		    !((_CU == SSMprotocol::CUtype_Transmission) && (tmpstr.toUInt() & 0x02)))
			continue;
		// Get memory address (low) definition:
		tmpstr = mbdefline.section(';', 3, 3);
		tmpMB.addr_low = tmpstr.toUInt(&ok, 16);
		// Check if memory address (low) is valid:
		if (!ok || (tmpMB.addr_low == 0))
			continue;
		// Get memory address (high) definition:
		tmpstr = mbdefline.section(';', 4, 4);
		if (tmpstr.isEmpty())
			tmpMB.addr_high = MEMORY_ADDRESS_NONE;
		else
			tmpMB.addr_high = tmpstr.toUInt(&ok, 16);
		if (!ok)
			continue;
		// Get title definition:
		tmpMB.title = mbdefline.section(';', 5, 5);
		// Check if title is available:
		if (tmpMB.title.isEmpty())
			continue;
		// Get scaling formula definition:
		tmpMB.scaleformula = mbdefline.section(';', 7, 7);
		// Check if scaling formula is available:
		if (tmpMB.scaleformula.isEmpty())
			continue;
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
		measuringblocks->push_back(tmpMB);
	}
	return true;
}


bool SSM2definitionsInterface::switches(std::vector<sw_intl_dt> *switches)
{
	QString swdefline;
	QString tmpstr;
	sw_intl_dt tmpSW;
	bool ok = false;
	int k = 0;

	if (!_id_set)
		return false;
	if ((_CU != SSMprotocol::CUtype_Engine) && (_CU != SSMprotocol::CUtype_Transmission))
		return false;
	switches->clear();
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
		unsigned int tmpbytenr = tmpstr.toUInt(&ok, 10);
		// Check if flagbyte is supported by our CU:
		if (!ok || (tmpbytenr > _ssmCUdata.flagbytescount()))
			continue;
		// Get flagbit definition:
		tmpstr = swdefline.section(';', 1, 1);
		unsigned int tmpbitnr = tmpstr.toUInt();
		// Check if CU supports this switch (if flagbit is set):
		if ((tmpbitnr < 1) || (tmpbitnr > 8))
			continue;
		if (!_ssmCUdata.flagbytebit(tmpbytenr-1, tmpbitnr-1))
			continue;
		// Check if switch is intended for this CU type:
		tmpstr = swdefline.section(';', 2, 2);
		if (!((_CU == SSMprotocol::CUtype_Engine) && (tmpstr.toUInt() & 0x01)) &&
		    !((_CU == SSMprotocol::CUtype_Transmission) && (tmpstr.toUInt() & 0x02)))
			continue;
		// Get memory address definition:
		tmpstr = swdefline.section(';', 3, 3);
		tmpSW.byteAddr = tmpstr.toUInt(&ok, 16);
		// Check if memory address is valid:
		if (!ok || (tmpSW.byteAddr == 0))
			continue;
		// Get title definition:
		tmpSW.title = swdefline.section(';', 4, 4);
		// Check if title is available:
		if (tmpSW.title.isEmpty())
			continue;
		// Get unit definition:
		tmpSW.unit = swdefline.section(';', 5, 5);
		if (tmpSW.unit.isEmpty())
			continue;
		// ***** SWITCH IS SUPPORTED BY CU AND DEFINITION IS VALID *****
		// Put switch data on the list:
		tmpSW.bitAddr = static_cast<unsigned char>(tmpbitnr);
		switches->push_back(tmpSW);
	}
	return true;
}


bool SSM2definitionsInterface::adjustments(std::vector<adjustment_intl_dt> *adjustments)
{
	QString defline = "";
	QString tmphelpstr = "";
	unsigned int tmpflagbyte = 0;
	unsigned int tmpflagbit = 0;
	unsigned int tmpCU = 0;
	adjustment_intl_dt tmpadjustment;
	int k = 0;
	bool ok = false;
	QStringList adjustmentsrawdata;

	if (!_id_set)
		return false;
	if ((_CU != SSMprotocol::CUtype_Engine) && (_CU != SSMprotocol::CUtype_Transmission))
		return false;
	adjustments->clear();
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
		if (tmphelpstr.count('-') == 1)
		{
			tmpflagbyte = tmphelpstr.section('-', 0, 0).toUInt(&ok, 10);
			if (!ok || (tmpflagbyte == 0) || (tmpflagbyte > _ssmCUdata.flagbytescount()))
				continue;
			tmpflagbit = tmphelpstr.section('-', 1, 1).toUInt(&ok, 10);
			if (!ok || (tmpflagbit < 1) || (tmpflagbit > 8))
				continue;
			if (!_ssmCUdata.flagbytebit(tmpflagbyte-1, tmpflagbit-1))
				continue;
		}
		else if (tmphelpstr.size() == 6)
		{
			unsigned int tmpsidbval = tmphelpstr.mid(0, 2).toUInt(&ok, 16);
			if (!ok || (tmpsidbval != static_cast<unsigned char>(_ssmCUdata.SYS_ID.at(0))))
				continue;
			tmpsidbval = tmphelpstr.mid(2, 2).toUInt(&ok, 16);
			if (!ok || (tmpsidbval != static_cast<unsigned char>(_ssmCUdata.SYS_ID.at(1))))
				continue;
			tmpsidbval = tmphelpstr.mid(4, 2).toUInt(&ok, 16);
			if (!ok || (tmpsidbval != static_cast<unsigned char>(_ssmCUdata.SYS_ID.at(2))))
				continue;
		}
		else
			continue;
		tmpCU = defline.section(';', 1, 1).toUInt(&ok, 10);
		if (!ok)
			continue;
		if ( !((_CU == SSMprotocol::CUtype_Engine) && (tmpCU == 0)) && !((_CU == SSMprotocol::CUtype_Transmission) && (tmpCU == 1)) )
			continue;
		tmpadjustment.AddrLow = defline.section(';', 2, 2).toUInt(&ok, 16);
		if (!ok || (tmpadjustment.AddrLow == 0))
			continue;
		tmpadjustment.AddrHigh = defline.section(';', 3, 3).toUInt(&ok, 16);
		if (!ok || (tmpadjustment.AddrHigh < 1))
			tmpadjustment.AddrHigh = 0;
		tmpadjustment.title = defline.section(';', 4, 4);
		if (!tmpadjustment.title.length())
			continue;
		tmpadjustment.unit = defline.section(';', 5, 5);	// may be empty
		tmphelpstr = defline.section(';', 6, 6);
		tmpadjustment.rawMin = tmphelpstr.toUInt(&ok, 10);
		if (!ok)
			continue;
		tmphelpstr = defline.section(';', 7, 7);
		tmpadjustment.rawMax = tmphelpstr.toUInt(&ok, 10);
		if (!ok)
			continue;
		tmphelpstr = defline.section(';', 8, 8);
		tmpadjustment.rawDefault = tmphelpstr.toUInt(&ok, 10);
		if (!ok)
			continue;
		tmpadjustment.formula = defline.section(';', 9, 9);
		if (!tmpadjustment.formula.length())
			continue;
		tmphelpstr = defline.section(';', 10, 10);
		if (tmphelpstr.length() == 0)
			tmphelpstr = "0";
		tmpadjustment.precision = tmphelpstr.toInt(&ok, 10);
		if (ok && (tmpadjustment.precision<=10) && (tmpadjustment.precision>=-10))
			adjustments->push_back(tmpadjustment);
	}
	return true;
}


bool SSM2definitionsInterface::actuatorTests(std::vector<actuator_dt> *actuators)
{
	bool ATsup = false;
	QString tmpstr = "";
	unsigned int tmpflagbyte = 0;
	unsigned int tmpflagbit = 0;
	actuator_dt tmpactuator;
	unsigned char tmpbitadr = 0;
	bool ok = false;
	int k = 0;
	QStringList actuatorsrawdata;

	if (!hasActuatorTests( &ATsup ))
		return false;
	actuators->clear();
	if (!ATsup)
		return true;
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
		tmpstr = actuatorsrawdata.at(k).section(';', 0, 0);
		if (!tmpstr.size())
			continue;
		tmpflagbyte = tmpstr.toUInt();
		if ((tmpflagbyte < 1) || (tmpflagbyte > _ssmCUdata.flagbytescount()))
			continue;
		tmpstr = actuatorsrawdata.at(k).section(';', 1, 1);
		if (!tmpstr.size())
			continue;
		tmpflagbit = tmpstr.toUInt();
		if ((tmpflagbit < 1) || (tmpflagbit > 8))
			continue;
		if (!_ssmCUdata.flagbytebit(tmpflagbyte-1, tmpflagbit-1))
			continue;
		tmpactuator.byteadr = actuatorsrawdata.at(k).section(';', 2, 2).toUInt(&ok, 16);
		if (!ok || (tmpactuator.byteadr == 0))
			continue;
		tmpbitadr = actuatorsrawdata.at(k).section(';', 3, 3).toUInt();
		if ((tmpbitadr < 1) || (tmpbitadr > 8))
			continue;
		tmpactuator.title = actuatorsrawdata.at(k).section(';', 4, 4);
		if (!tmpactuator.title.length())
			continue;
		tmpactuator.bitadr = tmpbitadr;
		actuators->push_back( tmpactuator );
	}
	return true;
}


bool SSM2definitionsInterface::hasOBD2system(bool *OBD2)
{
	if (!_id_set)
		return false;
	if ((_CU != SSMprotocol::CUtype_Engine) && (_CU != SSMprotocol::CUtype_Transmission))
		return false;
	*OBD2 = !_ssmCUdata.flagbytebit(29, 7) && !_ssmCUdata.flagbytebit(28, 1);
	return true;
}


bool SSM2definitionsInterface::hasVINsupport(bool *VINsup)
{
	if (!_id_set)
		return false;
	if ((_CU != SSMprotocol::CUtype_Engine) && (_CU != SSMprotocol::CUtype_Transmission))
		return false;
	*VINsup = _CU == SSMprotocol::CUtype_Engine && _ssmCUdata.flagbytebit(36, 0);
	return true;
}


bool SSM2definitionsInterface::hasImmobilizer(bool *ImmoSup)
{
	if (!_id_set)
		return false;
	if ((_CU != SSMprotocol::CUtype_Engine) && (_CU != SSMprotocol::CUtype_Transmission))
		return false;
	*ImmoSup = _CU == SSMprotocol::CUtype_Engine
		&& _ssmCUdata.flagbytebit(28, 4);
	return true;
}


bool SSM2definitionsInterface::hasImmobilizerTest(bool *ImmoTestSup)
{
	if (!_id_set)
		return false;
	if ((_CU != SSMprotocol::CUtype_Engine) && (_CU != SSMprotocol::CUtype_Transmission))
		return false;
	*ImmoTestSup = _CU == SSMprotocol::CUtype_Engine
		&& _ssmCUdata.flagbytebit(11, 5)
		&& _ssmCUdata.flagbytebit(28, 4);
	return true;
}


bool SSM2definitionsInterface::hasIntegratedCC(bool *CCsup)
{
	if (!_id_set)
		return false;
	if ((_CU != SSMprotocol::CUtype_Engine) && (_CU != SSMprotocol::CUtype_Transmission))
		return false;
	*CCsup = _CU == SSMprotocol::CUtype_Engine && _ssmCUdata.flagbytebit(39, 0);
	return true;
}


bool SSM2definitionsInterface::hasClearMemory(bool *CMsup)
{
	if (!_id_set)
		return false;
	if ((_CU != SSMprotocol::CUtype_Engine) && (_CU != SSMprotocol::CUtype_Transmission))
		return false;
	*CMsup = true;
	return true;
}


bool SSM2definitionsInterface::hasClearMemory2(bool *CM2sup)
{
	if (!_id_set)
		return false;
	if ((_CU != SSMprotocol::CUtype_Engine) && (_CU != SSMprotocol::CUtype_Transmission))
		return false;
	*CM2sup = _CU == SSMprotocol::CUtype_Transmission && _ssmCUdata.flagbytebit(39, 1);
	return true;
}


bool SSM2definitionsInterface::hasTestMode(bool *TMsup)
{
	if (!_id_set)
		return false;
	if ((_CU != SSMprotocol::CUtype_Engine) && (_CU != SSMprotocol::CUtype_Transmission))
		return false;
	*TMsup = _CU == SSMprotocol::CUtype_Engine && _ssmCUdata.flagbytebit(11, 5);
	return true;
}


bool SSM2definitionsInterface::hasActuatorTests(bool *ATsup)
{
	bool TMsup = false;
	if (!hasTestMode(&TMsup))	// includes check of _status
		return false;
	*ATsup = _CU == SSMprotocol::CUtype_Engine
		&& TMsup
		&& _ssmCUdata.flagbytebit(28, 6)
		&& _ssmCUdata.flagbytebit(0, 0);
	return true;
}


bool SSM2definitionsInterface::hasMBengineSpeed(bool *EngSpeedMBsup)
{
	if (!_id_set)
		return false;
	if ((_CU != SSMprotocol::CUtype_Engine) && (_CU != SSMprotocol::CUtype_Transmission))
		return false;
	*EngSpeedMBsup = _ssmCUdata.flagbytebit(0, 0);
	return true;
}


bool SSM2definitionsInterface::hasSWignition(bool *IgnSWsup)
{
	if (!_id_set)
		return false;
	if ((_CU != SSMprotocol::CUtype_Engine) && (_CU != SSMprotocol::CUtype_Transmission))
		return false;
	*IgnSWsup = _ssmCUdata.flagbytebit(12, 3);
	return true;
}


// PRIVATE:

void SSM2definitionsInterface::addDCdefs(unsigned int currOrTempOrLatestDCsAddr, unsigned int histOrMemDCsAddr, QStringList rawDefs, std::vector<dc_defs_dt> * defs)
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
		tmpdef.title[k] = "     ???     (0x" + QString::number(currOrTempOrLatestDCsAddr,16).toUpper() + "/0x" + QString::number(histOrMemDCsAddr,16).toUpper() + " Bit " + QString::number(k+1) + ")";
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
	/* NOTE:	- DCs with missing definitions are displayed with address byte + bit in the title field
			- DCs with existing definition and empty code- AND title-fields are ignored */
}

