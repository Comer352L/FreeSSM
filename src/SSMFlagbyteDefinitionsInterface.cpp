/*
 * SSMFlagbyteDefinitionsInterface.cpp - Interface to the SSM flagbyte definitions
 *
 * Copyright (C) 2008-2023 Comer352L
 * Copyright (C) 2019 madanadam (Turkish language support)
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

#include "SSMFlagbyteDefinitionsInterface.h"


SSMFlagbyteDefinitionsInterface::SSMFlagbyteDefinitionsInterface(QString language) : SSMDefinitionsInterface(language)
{
	_dc_defs = NULL;
	_cccc_defs = NULL;
	_memCCs_supported = false;
}


SSMFlagbyteDefinitionsInterface::~SSMFlagbyteDefinitionsInterface()
{
	if (_dc_defs != NULL)
		delete _dc_defs;
	if (_cccc_defs != NULL)
		delete _cccc_defs;
}


bool SSMFlagbyteDefinitionsInterface::selectControlUnitID(CUtype cu, const SSMCUdata& ssmCUdata)
{
	if ((cu != CUtype::Engine) && (cu != CUtype::Transmission))
		return false;
	_CU = cu;
	_ssmCUdata = ssmCUdata;
	setupDiagnosticCodes();
	setupCruiseControlCancelCodes();
	_id_set = true;
	return true;
}


bool SSMFlagbyteDefinitionsInterface::systemDescription(std::string *description)
{
	QString descr;
	bool ok = false;
	if (description == NULL)
		return false;
	if (!_id_set)
		return false;
	if (_CU == CUtype::Engine)
		ok = getSysDescriptionBySysID( SystemType::ECU, _ssmCUdata.SYS_ID, &descr );
	else // CUtype::Transmission
		ok = getSysDescriptionBySysID( SystemType::TCU, _ssmCUdata.SYS_ID, &descr );
	if (ok)
		*description = descr.toStdString();
	return ok;
}


bool SSMFlagbyteDefinitionsInterface::getDCblockData(std::vector<dc_block_dt> *block_data)
{
	if (block_data == NULL)
		return false;
	block_data->clear();
	if (!_id_set)
		return false;

	for (unsigned int i = 0; i < _dc_defs->size(); i++)
	{
		dc_block_dt new_block;
		dc_addr_dt new_addr;

		new_addr.scaling = dc_addr_dt::Scaling::bitwise;

		new_addr.address = _dc_defs->at(i).byteAddr_currentOrTempOrLatest;
		new_addr.type = dc_addr_dt::Type::currentOrTempOrLatest;
		new_block.addresses.push_back(new_addr);

		new_addr.address = _dc_defs->at(i).byteAddr_historicOrMemorized;
		new_addr.type = dc_addr_dt::Type::historicOrMemorized;
		new_block.addresses.push_back(new_addr);

		block_data->push_back(new_block);
	}

	for (unsigned int i = 0; i < _cccc_defs->size(); i++)
	{
		dc_block_dt new_block;
		dc_addr_dt new_addr;

		new_addr.scaling = dc_addr_dt::Scaling::bitwise;

		new_addr.address = _cccc_defs->at(i).byteAddr_currentOrTempOrLatest;
		new_addr.type = dc_addr_dt::Type::currentOrTempOrLatest;
		new_block.addresses.push_back(new_addr);

		if (_memCCs_supported)
		{
				new_addr.address = _cccc_defs->at(i).byteAddr_historicOrMemorized;
				new_addr.type = dc_addr_dt::Type::historicOrMemorized;
				new_block.addresses.push_back(new_addr);
		}

		block_data->push_back(new_block);
	}

	return true;
}


bool SSMFlagbyteDefinitionsInterface::measuringBlocks(std::vector<mb_intl_dt> *measuringblocks)
{
	QString mbdefline;
	QString tmpstr;
	int tmpprecision = 0;
	mb_intl_dt tmpMB;
	bool ok = false;
	int k = 0;

	if (!_id_set)
		return false;
	measuringblocks->clear();
	// Select definitions depending on language:
	QStringList mbrawdata;
	if (_language == "de")
	{
		SSMFlagbyteDefinitions_de rawdefs_de;
		mbrawdata = rawdefs_de.MBrawDefs();
	}
	else if (_language == "tr")	// Turkish
	{
		SSMFlagbyteDefinitions_tr rawdefs_tr;
		mbrawdata = rawdefs_tr.MBrawDefs();
	}
	else
	{
		SSMFlagbyteDefinitions_en rawdefs_en;
		mbrawdata = rawdefs_en.MBrawDefs();
	}
	// Assort list of supported MBs:
	for (k=0; k<mbrawdata.size(); k++)
	{
		// Get flagbyte address definition:
#if QT_VERSION < 0x050000
		mbdefline = QString::fromUtf8( mbrawdata.at(k).toLatin1() );
#else
		mbdefline = mbrawdata.at(k);
#endif
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
		if (!((_CU == CUtype::Engine) && (tmpstr.toUInt() & 0x01)) &&
		    !((_CU == CUtype::Transmission) && (tmpstr.toUInt() & 0x02)))
			continue;
		// Get memory address (low) definition:
		tmpstr = mbdefline.section(';', 3, 3);
		tmpMB.addrLow = tmpstr.toUInt(&ok, 16);
		// Check if memory address (low) is valid:
		if (!ok || (tmpMB.addrLow == 0))
			continue;
		// Get memory address (high) definition:
		tmpstr = mbdefline.section(';', 4, 4);
		if (tmpstr.isEmpty())
			tmpMB.addrHigh = MEMORY_ADDRESS_NONE;
		else
			tmpMB.addrHigh = tmpstr.toUInt(&ok, 16);
		if (!ok)
			continue;
		// Get title definition:
		tmpMB.title = mbdefline.section(';', 5, 5);
		// Check if title is available:
		if (tmpMB.title.isEmpty())
			continue;
		// Get scaling formula definition:
		tmpMB.formula = mbdefline.section(';', 7, 7);
		// Check if scaling formula is available:
		if (tmpMB.formula.isEmpty())
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


bool SSMFlagbyteDefinitionsInterface::switches(std::vector<sw_intl_dt> *switches)
{
	QString swdefline;
	QString tmpstr;
	sw_intl_dt tmpSW;
	bool ok = false;
	int k = 0;

	if (!_id_set)
		return false;
	switches->clear();
	// Select definitions depending on language:
	QStringList swrawdata;
	if (_language == "de")
	{
		SSMFlagbyteDefinitions_de rawdefs_de;
		swrawdata = rawdefs_de.SWrawDefs();
	}
	else if (_language == "tr")	// Turkish
	{
		SSMFlagbyteDefinitions_tr rawdefs_tr;
		swrawdata = rawdefs_tr.SWrawDefs();
	}
	else
	{
		SSMFlagbyteDefinitions_en rawdefs_en;
		swrawdata = rawdefs_en.SWrawDefs();
	}
	// Assort list of supported switches:
	for (k=0; k<swrawdata.size(); k++)
	{
		// Get flagbyte address definition:
#if QT_VERSION < 0x050000
		swdefline = QString::fromUtf8( swrawdata.at(k).toLatin1() );
#else
		swdefline = swrawdata.at(k);
#endif
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
		if (!((_CU == CUtype::Engine) && (tmpstr.toUInt() & 0x01)) &&
		    !((_CU == CUtype::Transmission) && (tmpstr.toUInt() & 0x02)))
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


bool SSMFlagbyteDefinitionsInterface::adjustments(std::vector<adjustment_intl_dt> *adjustments)
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
	adjustments->clear();
	if (_language == "de")
	{
		SSMFlagbyteDefinitions_de rawdefs_de;
		adjustmentsrawdata = rawdefs_de.AdjustmentRawDefs();
	}
	else if (_language == "tr")	// Turkish
	{
		SSMFlagbyteDefinitions_tr rawdefs_tr;
		adjustmentsrawdata = rawdefs_tr.AdjustmentRawDefs();
	}
	else
	{
		SSMFlagbyteDefinitions_en rawdefs_en;
		adjustmentsrawdata = rawdefs_en.AdjustmentRawDefs();
	}
	for (k=0; k<adjustmentsrawdata.size(); k++)
	{
#if QT_VERSION < 0x050000
		defline = QString::fromUtf8( adjustmentsrawdata.at(k).toLatin1() );
#else
		defline = adjustmentsrawdata.at(k);
#endif
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
		if ( !((_CU == CUtype::Engine) && (tmpCU == 0)) && !((_CU == CUtype::Transmission) && (tmpCU == 1)) )
			continue;
		tmpadjustment.addrLow = defline.section(';', 2, 2).toUInt(&ok, 16);
		if (!ok || (tmpadjustment.addrLow == 0))
			continue;
		tmpadjustment.addrHigh = defline.section(';', 3, 3).toUInt(&ok, 16);
		if (!ok || (tmpadjustment.addrHigh < 1))
			tmpadjustment.addrHigh = 0;
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


bool SSMFlagbyteDefinitionsInterface::actuatorTests(std::vector<actuator_dt> *actuators)
{
	bool ATsup = false;
	QString tmpstr = "";
	unsigned int tmpflagbyte = 0;
	unsigned int tmpflagbit = 0;
	actuator_dt tmpactuator;
	unsigned char tmpbitaddr = 0;
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
		SSMFlagbyteDefinitions_de rawdefs_de;
		actuatorsrawdata = rawdefs_de.ActuatorRawDefs();
	}
	else if (_language == "tr")	// Turkish
	{
		SSMFlagbyteDefinitions_tr rawdefs_tr;
		actuatorsrawdata = rawdefs_tr.ActuatorRawDefs();
	}
	else
	{
		SSMFlagbyteDefinitions_en rawdefs_en;
		actuatorsrawdata = rawdefs_en.ActuatorRawDefs();
	}
	for (k=0; k<actuatorsrawdata.size(); k++)
	{
#if QT_VERSION < 0x050000
		tmpstr = QString::fromUtf8( actuatorsrawdata.at(k).section(';', 0, 0).toLatin1() );
#else
		tmpstr = actuatorsrawdata.at(k).section(';', 0, 0);
#endif
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
		tmpactuator.byteAddr = actuatorsrawdata.at(k).section(';', 2, 2).toUInt(&ok, 16);
		if (!ok || (tmpactuator.byteAddr == 0))
			continue;
		tmpbitaddr = actuatorsrawdata.at(k).section(';', 3, 3).toUInt();
		if ((tmpbitaddr < 1) || (tmpbitaddr > 8))
			continue;
		tmpactuator.title = actuatorsrawdata.at(k).section(';', 4, 4);
		if (!tmpactuator.title.length())
			continue;
		tmpactuator.bitAddr = tmpbitaddr;
		actuators->push_back( tmpactuator );
	}
	return true;
}


bool SSMFlagbyteDefinitionsInterface::clearMemoryData(unsigned int *address, char *value)
{
	bool cm_sup = false;

	*address = MEMORY_ADDRESS_NONE;
	*value = 0x00;
	if (!hasClearMemory(&cm_sup)) // also validates _id_set
		return false;
	if (cm_sup)
	{
		*address = 0x60;
		*value = 0x40;
	}

	return true;
}


bool SSMFlagbyteDefinitionsInterface::clearMemory2Data(unsigned int *address, char *value)
{
	bool cm2_sup = false;

	*address = MEMORY_ADDRESS_NONE;
	*value = 0x00;
	if (!hasClearMemory2(&cm2_sup)) // also validates _id_set
		return false;
	if (cm2_sup)
	{
		*address = 0x60;
		*value = 0x40;
	}

	return true;
}


bool SSMFlagbyteDefinitionsInterface::MBdata_engineRunning(mb_enginespeed_data_dt *mb_enginespeed_data)
{
	bool EngSpeedMBsup = false;

	if (mb_enginespeed_data == NULL)
		return false;
	if (!hasMBengineSpeed(&EngSpeedMBsup))
		return false;
	if (EngSpeedMBsup)
	{
		mb_enginespeed_data->addr_low = 0x0F;
		mb_enginespeed_data->addr_high = 0x0E;
		mb_enginespeed_data->scaling_formula = "/4";
	}
	else
	{
		mb_enginespeed_data->addr_low = MEMORY_ADDRESS_NONE;
		mb_enginespeed_data->addr_high = MEMORY_ADDRESS_NONE;
		mb_enginespeed_data->scaling_formula = "";
	}
	return true;
}


bool SSMFlagbyteDefinitionsInterface::SWdata_testModeState(sw_stateindication_data_dt *sw_testmode_data)
{
	bool TMsup = false;

	if (sw_testmode_data == NULL)
		return false;
	if (!hasTestMode(&TMsup))
		return false;
	if (TMsup)
	{
		sw_testmode_data->addr = 0x61;
		sw_testmode_data->bit = 5;
		sw_testmode_data->inverted = false;
	}
	else
	{
		sw_testmode_data->addr = MEMORY_ADDRESS_NONE;
		sw_testmode_data->bit = 0;
		sw_testmode_data->inverted = false;
	}
	return true;
}


bool SSMFlagbyteDefinitionsInterface::SWdata_DCheckState(sw_stateindication_data_dt *sw_dcheckactive_data)
{
	if (sw_dcheckactive_data == NULL)
		return false;
	if (!_id_set)
		return false;
	if ((_CU == CUtype::Engine) && _ssmCUdata.flagbytebit(11, 7))
	{
		sw_dcheckactive_data->addr = 0x61;
		sw_dcheckactive_data->bit = 7;
		sw_dcheckactive_data->inverted = false;
	}
	else
	{
		sw_dcheckactive_data->addr = MEMORY_ADDRESS_NONE;
		sw_dcheckactive_data->bit = 0;
		sw_dcheckactive_data->inverted = false;
	}
	return true;
}


bool SSMFlagbyteDefinitionsInterface::SWdata_ignitionState(sw_stateindication_data_dt *sw_ignitionstate_data)
{
	if (sw_ignitionstate_data == NULL)
		return false;
	if (!_id_set)
		return false;
	if (_ssmCUdata.flagbytebit(12, 3))
	{
		sw_ignitionstate_data->addr = 0x62;
		sw_ignitionstate_data->bit = 3;
		sw_ignitionstate_data->inverted = false;
	}
	else
	{
		sw_ignitionstate_data->addr = MEMORY_ADDRESS_NONE;
		sw_ignitionstate_data->bit = 0;
		sw_ignitionstate_data->inverted = false;
	}
	return true;
}


void SSMFlagbyteDefinitionsInterface::getDCcontent(unsigned int address, char databyte, QStringList *codes, QStringList *titles)
{
	// NOTE: codes OR titles may be NULL !
	if ((codes == NULL) && (titles == NULL))
		return;
	if (codes != NULL)
		codes->clear();
	if (titles != NULL)
		titles->clear();
	if (!_id_set)
		return;
	if (getBitDCcontent(_dc_defs, address, databyte, codes, titles))
		return;
	getBitDCcontent(_cccc_defs, address, databyte, codes, titles);
	/* NOTE: We do not expect this function to be called with an address for which no valid definitions exist,
	 *       because we didn't report such an address with getDCblockData().
	 *       We can't assume the value at an invalid address to correspond to (a) DC(s),
	 *       so we do not report any DCs with generic/substitude code+title string.
	 */
}


bool SSMFlagbyteDefinitionsInterface::hasOBD2system(bool *OBD2)
{
	if (!_id_set)
		return false;
	*OBD2 = (!_ssmCUdata.flagbytebit(29, 7) && !_ssmCUdata.flagbytebit(28, 1));
	return true;
}


bool SSMFlagbyteDefinitionsInterface::hasVINsupport(bool *VINsup)
{
	if (!_id_set)
		return false;
	*VINsup = ((_CU == CUtype::Engine) && _ssmCUdata.flagbytebit(36, 0));
	return true;
}


bool SSMFlagbyteDefinitionsInterface::hasImmobilizer(bool *ImmoSup)
{
	if (!_id_set)
		return false;
	*ImmoSup = ((_CU == CUtype::Engine) && _ssmCUdata.flagbytebit(28, 4));
	return true;
}


bool SSMFlagbyteDefinitionsInterface::hasImmobilizerTest(bool *ImmoTestSup)
{
	bool ImmoSup = false;
	bool TMsup = false;
	if (!hasImmobilizer(&ImmoSup) || !hasTestMode(&TMsup))
		return false;
	*ImmoTestSup = (TMsup && ImmoSup);
	return true;
}


bool SSMFlagbyteDefinitionsInterface::hasIntegratedCC(bool *CCsup)
{
	if (!_id_set)
		return false;
	*CCsup = ((_CU == CUtype::Engine) && _ssmCUdata.flagbytebit(39, 0));
	return true;
}


bool SSMFlagbyteDefinitionsInterface::hasClearMemory(bool *CMsup)
{
	if (!_id_set)
		return false;
	*CMsup = true;
	return true;
}


bool SSMFlagbyteDefinitionsInterface::hasClearMemory2(bool *CM2sup)
{
	if (!_id_set)
		return false;
	*CM2sup = ((_CU == CUtype::Transmission) && _ssmCUdata.flagbytebit(39, 1));
	return true;
}


bool SSMFlagbyteDefinitionsInterface::hasActuatorTests(bool *ATsup)
{
	bool TMsup = false;
	bool EngSpeedMBsup = false;
	if (!hasTestMode(&TMsup) || !hasMBengineSpeed(&EngSpeedMBsup))
		return false;
	*ATsup = ((_CU == CUtype::Engine)
		  && TMsup
		  && EngSpeedMBsup
		  && _ssmCUdata.flagbytebit(28, 6));
	return true;
}


// PRIVATE:

void SSMFlagbyteDefinitionsInterface::setupDiagnosticCodes()
{
	unsigned int addr = 0;
	QStringList rawDefs;
	bool fmt_OBD2 = false;

	if (_dc_defs == NULL)
		_dc_defs = new std::vector<dc_defs_dt>();
	else
		_dc_defs->clear();
	fmt_OBD2 = !_ssmCUdata.flagbytebit(29, 7);
	if (_language == "de")
	{
		SSMFlagbyteDefinitions_de rawdefs_de;
		if (fmt_OBD2)
			rawDefs = rawdefs_de.OBDDTCrawDefs();
		else
			rawDefs = rawdefs_de.SUBDTCrawDefs();
	}
	else if (_language == "tr")	// Turkish
	{
		SSMFlagbyteDefinitions_tr rawdefs_tr;
		if (fmt_OBD2)
			rawDefs = rawdefs_tr.OBDDTCrawDefs();
		else
			rawDefs = rawdefs_tr.SUBDTCrawDefs();
	}
	else
	{
		SSMFlagbyteDefinitions_en rawdefs_en;
		if (fmt_OBD2)
			rawDefs = rawdefs_en.OBDDTCrawDefs();
		else
			rawDefs = rawdefs_en.SUBDTCrawDefs();
	}
	// Setup data of the supported DCs:
	if (!fmt_OBD2)
	{
		for (addr=0x8E; addr<=0x98; addr++)
			addDCdefs(addr, addr+22, rawDefs, _dc_defs);
		return;
	}
	else if (_ssmCUdata.flagbytebit(29, 4) || _ssmCUdata.flagbytebit(29, 6))
	{
		for (addr=0x8E; addr<=0xAD; addr++)
			addDCdefs(addr, addr+32, rawDefs, _dc_defs);
	}
	if (_ssmCUdata.flagbytebit(28, 0))
	{
		for (addr=0xF0; addr<=0xF3; addr++)
			addDCdefs(addr, addr+4, rawDefs, _dc_defs);
	}
	if (_ssmCUdata.flagbytescount() > 32)
	{
		if (_ssmCUdata.flagbytebit(39, 7))
		{
			for (addr=0x123; addr<=0x12A; addr++)
				addDCdefs(addr, addr+8, rawDefs, _dc_defs);
		}
		if (_ssmCUdata.flagbytebit(39, 6))
		{
			for (addr=0x150; addr<=0x154; addr++)
				addDCdefs(addr, addr+5, rawDefs, _dc_defs);
		}
		if (_ssmCUdata.flagbytebit(39, 5))
		{
			for (addr=0x160; addr<=0x164; addr++)
				addDCdefs(addr, addr+5, rawDefs, _dc_defs);
		}
		if (_ssmCUdata.flagbytebit(39, 4))
		{
			for (addr=0x174; addr<=0x17A; addr++)
				addDCdefs(addr, addr+7, rawDefs, _dc_defs);
		}
		if (_ssmCUdata.flagbytescount() > 48)
		{
			if (_ssmCUdata.flagbytebit(50, 6))
			{
				for (addr=0x1C1; addr<=0x1C6; addr++)
					addDCdefs(addr, addr+6, rawDefs, _dc_defs);
				for (addr=0x20A; addr<=0x20D; addr++)
					addDCdefs(addr, addr+4, rawDefs, _dc_defs);
			}
			if (_ssmCUdata.flagbytebit(50, 5))
			{
				for (addr=0x263; addr<=0x267; addr++)
					addDCdefs(addr, addr+5, rawDefs, _dc_defs);
			}
		}
	}
}


void SSMFlagbyteDefinitionsInterface::setupCruiseControlCancelCodes()
{
	unsigned int addr = 0;
	bool CCsup = false;
	QStringList CCrawDefs;

	if (_cccc_defs == NULL)
		_cccc_defs = new std::vector<dc_defs_dt>();
	else
		_cccc_defs->clear();
	// Check if CU is equipped with CC:
	if (!hasIntegratedCC( &CCsup ))
		return;
	if (!CCsup)
		return;
	// Get raw CCCC-definitions:
	if (_language == "de")
	{
		SSMFlagbyteDefinitions_de rawdefs_de;
		CCrawDefs = rawdefs_de.CCCCrawDefs();
	}
	else if (_language == "tr")	// Turkish
	{
		SSMFlagbyteDefinitions_tr rawdefs_tr;
		CCrawDefs = rawdefs_tr.CCCCrawDefs();
	}
	else
	{
		SSMFlagbyteDefinitions_en rawdefs_en;
		CCrawDefs = rawdefs_en.CCCCrawDefs();
	}
	// Setup data of the supported CCCCs:
	for (addr=0x133; addr<=0x136; addr++)
		addDCdefs(addr, addr+4, CCrawDefs, _cccc_defs);
	_memCCs_supported = _ssmCUdata.flagbytebit(41, 2);
}


void SSMFlagbyteDefinitionsInterface::addDCdefs(unsigned int currOrTempOrLatestDCsAddr, unsigned int histOrMemDCsAddr, QStringList rawDefs, std::vector<dc_defs_dt> * defs)
{
	dc_defs_dt tmpdef;
	QStringList tmpdefparts;
	unsigned int tmpdcaddr_1 = 0;
	unsigned int tmpdcaddr_2 = 0;
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
			tmpdcaddr_1 = tmpdefparts.at(0).toUInt(&ok, 16);  // current/temporary/latest/D-Check DCs memory address
			tmpdcaddr_2 = tmpdefparts.at(1).toUInt(&ok, 16);  // historic/memorized DCs memory address
			if ((ok) && (tmpdcaddr_1 == currOrTempOrLatestDCsAddr) && (tmpdcaddr_2 == histOrMemDCsAddr))
			{
				tmpbitaddr = tmpdefparts.at(2).toUInt(); // flagbit
				tmpdef.code[ tmpbitaddr-1 ] = tmpdefparts.at(3);
				tmpdef.title[ tmpbitaddr-1 ] = tmpdefparts.at(4);
			}
		}
	}
	defs->push_back(tmpdef);
	/* NOTE: - DCs with missing definitions are displayed with address byte + bit in the title field
	         - DCs with existing definitions and empty code- AND title elements are ignored
	*/
}


bool SSMFlagbyteDefinitionsInterface::getBitDCcontent(std::vector<dc_defs_dt> *defs, unsigned int addr, char databyte, QStringList *codes, QStringList *titles)
{
	for (unsigned int i = 0; i < defs->size(); i++)
	{
		dc_defs_dt dc_defs = defs->at(i);
		if ((dc_defs.byteAddr_currentOrTempOrLatest == addr) || (dc_defs.byteAddr_historicOrMemorized == addr))
		{
			for (unsigned char bit = 0; bit < 8; bit++)
			{
				if (databyte & static_cast<char>(1 << bit))
				{
					// NOTE: DCs with existing definition and empty code and title strings shall be ignored
					if (!(dc_defs.code[bit].isEmpty() && (dc_defs.title[bit].isEmpty())))
					{
						if (codes != NULL)
							codes->push_back(dc_defs.code[bit]);
						if (titles != NULL)
							titles->push_back(dc_defs.title[bit]);
					}
				}
			}
			return true;
		}
	}
	return false;
}


bool SSMFlagbyteDefinitionsInterface::hasTestMode(bool *TMsup)
{
	if (!_id_set)
		return false;
	*TMsup = ((_CU == CUtype::Engine) && _ssmCUdata.flagbytebit(11, 5));
	return true;
}


bool SSMFlagbyteDefinitionsInterface::hasMBengineSpeed(bool *EngSpeedMBsup)
{
	if (!_id_set)
		return false;
	*EngSpeedMBsup = _ssmCUdata.flagbytebit(0, 0);
	return true;
}

