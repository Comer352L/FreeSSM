/*
 * SSMprotocol.cpp - Abstract application layer for the Subaru SSM protocols
 *
 * Copyright (C) 2009-2016 Comer352L
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
	_language = language;
	_CU = CUtype::Engine;
	_state = state_needSetup;
	resetCommonCUdata();
	qRegisterMetaType< std::vector<char> >("std::vector<char>");
}


SSMprotocol::~SSMprotocol()
{
}


bool SSMprotocol::CUtype(enum CUtype *CU)
{
	if (_state == state_needSetup) return false;
	*CU = _CU;
	return true;
}


SSMprotocol::state_dt SSMprotocol::state()
{
	return _state;
}


std::string SSMprotocol::getSysID() const
{
	if (_state == state_needSetup) return "";
	return libFSSM::StrToHexstr(_ssmCUdata.SYS_ID);
}


std::string SSMprotocol::getROMID() const
{
	if (_state == state_needSetup) return "";
	if (_ssmCUdata.uses_Flagbytes())
		return libFSSM::StrToHexstr(_ssmCUdata.ROM_ID);
	else
		return getSysID();
}


bool SSMprotocol::getSystemDescription(QString *sysdescription)
{
	if (_state == state_needSetup) return false;
	if (_sysDescription.size())
	{
		*sysdescription = QString::fromStdString(_sysDescription);
		return true;
	}
	return false;
}


bool SSMprotocol::hasOBD2system(bool *OBD2)
{
	if (_state == state_needSetup) return false;
	*OBD2 = _has_OBD2;
	return true;
}


bool SSMprotocol::hasVINsupport(bool *VINsup)
{
	if (_state == state_needSetup) return false;
	*VINsup = false;
	return true;
}


bool SSMprotocol::hasImmobilizer(bool *ImmoSup)
{
	if (_state == state_needSetup) return false;
	*ImmoSup = _has_Immo;
	return true;
}


bool SSMprotocol::hasImmobilizerTest(bool *ImmoTestSup)
{
	if (_state == state_needSetup) return false;
	*ImmoTestSup = _has_ImmoTest;
	return true;
}


bool SSMprotocol::hasIntegratedCC(bool *CCsup)
{
	if (_state == state_needSetup) return false;
	*CCsup = false;
	return true;
}


bool SSMprotocol::hasClearMemory(bool *CMsup)
{
	if (_state == state_needSetup) return false;
	*CMsup = (_CMaddr != MEMORY_ADDRESS_NONE);
	return true;
}


bool SSMprotocol::hasClearMemory2(bool *CM2sup)
{
	if (_state == state_needSetup) return false;
	*CM2sup = false;
	return true;
}


bool SSMprotocol::hasTestMode(bool *TMsup)
{
	if (_state == state_needSetup) return false;
	*TMsup = _has_TestMode;
	return true;
}


bool SSMprotocol::hasActuatorTests(bool *ATsup)
{
	if (_state == state_needSetup) return false;
	*ATsup = _has_ActTest;
	return true;
}


bool SSMprotocol::getLastDCgroupsSelection(int *DCgroups)
{
	if (_state == state_needSetup) return false;
	*DCgroups = _selectedDCgroups;
	return true;
}


bool SSMprotocol::getSupportedMBs(std::vector<mb_dt> *supportedMBs)
{
	if (_state == state_needSetup) return false;
	supportedMBs->assign(begin(_supportedMBs), end(_supportedMBs));
	return true;
}


bool SSMprotocol::getSupportedSWs(std::vector<sw_dt> *supportedSWs)
{
	if (_state == state_needSetup) return false;
	supportedSWs->assign(begin(_supportedSWs), end(_supportedSWs));
	return true;
}


bool SSMprotocol::getLastMBSWselection(std::vector<MBSWmetadata_dt> *MBSWmetaList)
{
	if (_state == state_needSetup) return false;
	if (!_MBSWmetaList.empty())
	{
		*MBSWmetaList = _MBSWmetaList;
		return true;
	}
	return false;
}


bool SSMprotocol::getSupportedAdjustments(std::vector<adjustment_dt> *supportedAdjustments)
{
	if (_state == state_needSetup) return false;
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


bool SSMprotocol::restartDCreading()
{
	return startDCreading(_selectedDCgroups);
}


bool SSMprotocol::restartMBSWreading()
{
	return startMBSWreading(_MBSWmetaList);
}


bool SSMprotocol::restartActuatorTest()
{
	return startActuatorTest(_selectedActuatorTestIndex);
}


bool SSMprotocol::stopAllPermanentOperations()
{
	bool result = false;
	if ((_state == state_needSetup) || (_state == state_normal))
	{
		result = true;
	}
	else if (_state == state_DCreading)
	{
		result = stopDCreading();
	}
	else if (_state == state_MBSWreading)
	{
		result = stopMBSWreading();
	}
	else if (_state == state_ActTesting)
	{
		result = stopActuatorTesting();
	}
	return result;
}

// PROTECTED / PRIVATE:

unsigned int SSMprotocol::processDTCsRawdata(std::vector<char> DCrawdata, int duration_ms)
{
	(void)duration_ms; // to avoid compiler error
	QStringList DCs;
	QStringList DCdescriptions;
	QStringList tmpDTCs;
	QStringList tmpDTCsDescriptions;
	bool TestMode = false;
	bool DCheckActive = false;
	unsigned int rawDataIndex = 0;
	if ((_selectedDCgroups & currentDTCs_DCgroup) || (_selectedDCgroups & temporaryDTCs_DCgroup))
	{
		if (_CU == CUtype::Engine && _ssmCUdata.uses_Ax10xx_defs())
		{
			rawDataIndex = 1;
			if (_has_TestMode)
				TestMode = DCrawdata.at(0) & 0x20;
			DCheckActive = DCrawdata.at(0) & 0x80;
		}
		// FIXME: support test mode and D-Check status for other SSM1 control units, too
		DCs.clear();
		DCdescriptions.clear();
		// Evaluate current/latest data trouble codes:
		for (unsigned int DTCdefs_index=0; DTCdefs_index<_DTCdefs.size(); DTCdefs_index++)
		{
			unsigned int address = _DTCdefs.at(DTCdefs_index).byteAddr_currentOrTempOrLatest;
			if (address == MEMORY_ADDRESS_NONE)
				continue;
			evaluateDCdataByte(address, DCrawdata.at(rawDataIndex), _DTCdefs, &tmpDTCs, &tmpDTCsDescriptions);
			DCs += tmpDTCs;
			DCdescriptions += tmpDTCsDescriptions;
			rawDataIndex++;
		}
		emit currentOrTemporaryDTCs(DCs, DCdescriptions, TestMode, DCheckActive);
	}
	if ((_selectedDCgroups & historicDTCs_DCgroup) || (_selectedDCgroups & memorizedDTCs_DCgroup))
	{
		DCs.clear();
		DCdescriptions.clear();
		// Evaluate historic/memorized data trouble codes:
		for (unsigned int DTCdefs_index=0; DTCdefs_index<_DTCdefs.size(); DTCdefs_index++)
		{
			unsigned int address = _DTCdefs.at(DTCdefs_index).byteAddr_historicOrMemorized;
			if (address == MEMORY_ADDRESS_NONE)
				continue;
			evaluateDCdataByte(address, DCrawdata.at(rawDataIndex), _DTCdefs, &tmpDTCs, &tmpDTCsDescriptions);
			DCs += tmpDTCs;
			DCdescriptions += tmpDTCsDescriptions;
			rawDataIndex++;
		}
		emit historicOrMemorizedDTCs(DCs, DCdescriptions);
	}
	return rawDataIndex;
}


void SSMprotocol::evaluateDCdataByte(unsigned int DCbyteaddr, char DCrawdata, std::vector<dc_defs_dt> DCdefs, QStringList *DC, QStringList *DCdescription)
{
	DC->clear();
	DCdescription->clear();
	if (DCrawdata == 0) return;
	// Search definitions:
	dc_defs_dt def;
	for (size_t k=0; k<DCdefs.size(); k++)
	{
		if ((DCdefs.at(k).byteAddr_currentOrTempOrLatest == DCbyteaddr) || (DCdefs.at(k).byteAddr_historicOrMemorized == DCbyteaddr))
		{
			def = DCdefs.at(k);
			break;
		}
	}
	// Assign codes and descriptions:
	for (unsigned char flagbit=0; flagbit<8; flagbit++)
	{
		if (DCrawdata & static_cast<char>(1 << flagbit))
		{
			// Check if DC is to be ignored:
			// NOTE: DCs with existing definition and empty code- and title-fields must be ignored !
			if (!(def.code[flagbit].isEmpty() && def.title[flagbit].isEmpty()))
			{
				DC->push_back( def.code[flagbit] );		// DC
				DCdescription->push_back( def.title[flagbit] );	// DC description
			}
		}
	}
}


bool SSMprotocol::setupMBSWQueryAddrList(std::vector<MBSWmetadata_dt> MBSWmetaList)
{
	// ***** SETUP (BYTE-) ADDRESS LIST FOR QUERYS *****
	unsigned int k = 0, m = 0;
	bool newaddr = true;
	_selMBsSWsAddr.clear();
	if (MBSWmetaList.size() == 0) return false;
	for (k=0; k<MBSWmetaList.size(); k++)
	{
		newaddr = true;
		// CHECK IF ADDRESS IS ALREADY ON THE QUERY-LIST:
		if (_selMBsSWsAddr.size())
		{
			// CHECK IF ADDRESS IS ALREADY ON THE LIST:
			for (m=0; m<_selMBsSWsAddr.size(); m++)
			{
				if (MBSWmetaList.at(k).blockType == BlockType::MB)
				{
					// CHECK IF CURRENT MB IS VALID/EXISTS:
					if (MBSWmetaList.at(k).nativeIndex > _supportedMBs.size()) return false;
					// COMPARE ADDRESS:
					if (_selMBsSWsAddr.at(m) == _supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).addr_low)
					{
						newaddr = false;
						break;
					}
				}
				else
				{
					// CHECK IF CURRENT SW IS VALID/EXISTS:
					if (MBSWmetaList.at(k).nativeIndex > _supportedSWs.size()) return false;
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
				_selMBsSWsAddr.push_back( _supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).addr_low );
				if (_supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).addr_high != MEMORY_ADDRESS_NONE)
					_selMBsSWsAddr.push_back( _supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).addr_high );
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

	for (size_t m=0; m<_selMBsSWsAddr.size(); m++)	// ADDRESS LOOP
	{
		const unsigned int address = _selMBsSWsAddr.at(m);
		const unsigned char rawbyte = static_cast<unsigned char>(rawdata.at(m));

		for (size_t k=0; k<_MBSWmetaList.size(); k++)	// MB/SW LOOP
		{
			const MBSWmetadata_dt& metadata = _MBSWmetaList.at(k);
			unsigned int& rawvalue = mbswrawvalues.at(k);

			if (metadata.blockType == BlockType::MB)
			{
				mb_intl_dt& mb = _supportedMBs.at(metadata.nativeIndex);
				// COMPARE ADDRESSES:
				if (address == mb.addr_low)
				{
					// ADDRESS/RAW BYTE CORRESPONDS WITH LOW BYTE ADDRESS OF MB
					rawvalue += rawbyte;
				}
				else if (address == mb.addr_high)
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
	for (unsigned int n=0; n<_actuators.size(); n++)
	{
		// Check if byte address is already on the list:
		for (unsigned int m=0; m<_allActByteAddr.size(); m++)
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


void SSMprotocol::resetCommonCUdata()
{
	// RESET ECU DATA:
	_ssmCUdata.clear();
	// Clear system description:
	_sysDescription.clear();
	_has_OBD2 = false;
	_has_Immo = false;
	_has_ImmoTest = false;
	_has_TestMode = false;
	_has_ActTest = false;
	_has_MB_engineSpeed = false;
	_has_SW_ignition = false;
	// Clear DC data:
	_DTCdefs.clear();
	_DTC_fmt_OBD2 = false;
	// Clear Clear Memory data:
	_CMaddr = MEMORY_ADDRESS_NONE;
	_CMvalue = '\x00';
	// Clear MB/SW data:
	_supportedMBs.clear();
	_supportedSWs.clear();
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
}

