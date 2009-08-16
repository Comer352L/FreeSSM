/*
 * SSMprotocol2.cpp - Abstract application layer for the Subaru SSM protocols
 *
 * Copyright (C) 2009 Comer352l
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



SSMprotocol::SSMprotocol(serialCOM *port, QString language)
{
	_port = port;
	_language = language;
	_state = state_needSetup;
}


SSMprotocol::~SSMprotocol()
{
}


bool SSMprotocol::CUtype(SSMprotocol::CUtype_dt *CU)
{
	if (_state == state_needSetup) return false;
	*CU = _CU;
	return true;
}


SSMprotocol::state_dt SSMprotocol::state()
{
	return _state;
}


bool SSMprotocol::getLastDCgroupsSelection(int *DCgroups)
{
	if (_state == state_needSetup) return false;
	if (_selectedDCgroups != noDCs_DCgroup)
	{
		*DCgroups = _selectedDCgroups;
		return true;
	}
	return false;
}


bool SSMprotocol::getSupportedMBs(std::vector<mb_dt> *supportedMBs)
{
	if (_state == state_needSetup) return false;
	supportedMBs->clear();
	for (unsigned int k=0; k<_supportedMBs.size(); k++)
		supportedMBs->push_back( _supportedMBs.at(k) );
	return true;
}


bool SSMprotocol::getSupportedSWs(std::vector<sw_dt> *supportedSWs)
{
	if (_state == state_needSetup) return false;
	supportedSWs->clear();
	for (unsigned int k=0; k<_supportedSWs.size(); k++)
		supportedSWs->push_back( _supportedSWs.at(k) );
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


bool SSMprotocol::getSupportedActuatorTests(QStringList *actuatorTestTitles)
{
	return false;
}


bool SSMprotocol::getLastActuatorTestSelection(unsigned char *actuatorTestIndex)
{
	return false;
}


bool SSMprotocol::getAdjustmentValue(unsigned char index, unsigned int *rawValue)
{
	return false;
}


bool SSMprotocol::getAllAdjustmentValues(std::vector<unsigned int> *rawValues)
{
	return false;
}


bool SSMprotocol::setAdjustmentValue(unsigned char index, unsigned int rawValue)
{
	return false;
}


bool SSMprotocol::getVIN(QString *VIN)
{
	return false;
}


bool SSMprotocol::isEngineRunning(bool *isrunning)
{
	return false;
}


bool SSMprotocol::isInTestMode(bool *testmode)
{
	return false;
}


bool SSMprotocol::restartDCreading()
{
	return startDCreading(_selectedDCgroups);
}


void SSMprotocol::evaluateDCdataByte(unsigned int DCbyteadr, char DCrawdata, std::vector<dc_defs_dt> DCdefs, QStringList *DC, QStringList *DCdescription)
{
	bool DCsAssigned[8] = {false,};
	unsigned char setbits[8] = {0,};
	unsigned char setbitslen = 0;
	unsigned int k = 0;
	unsigned char setbitsindex = 0;
	QString ukdctitle;

	DC->clear();
	DCdescription->clear();
	if (DCrawdata == 0) return;
	// Search definitions:
	dc_defs_dt def;
	for (k=0; k<DCdefs.size(); k++)
	{
		if ((DCdefs.at(k).byteAddr_currentOrTempOrLatest == DCbyteadr) || (DCdefs.at(k).byteAddr_historicOrMemorized == DCbyteadr))
		{
			def = DCdefs.at(k);
			break;
		}
	}
	// Assign codes and descriptions:
	for (unsigned char flagbit=0; flagbit<8; flagbit++)
	{
		if (DCrawdata & static_cast<char>(pow(2, flagbit)))
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


bool SSMprotocol::restartMBSWreading()
{
	return startMBSWreading(_MBSWmetaList);
}


bool SSMprotocol::setupMBSWQueryAddrList(std::vector<MBSWmetadata_dt> MBSWmetaList)
{
	// ***** SETUP (BYTE-) ADDRESS LIST FOR QUERYS *****
	unsigned int k = 0, m = 0;
	bool newadr = true;
	if (MBSWmetaList.size() == 0) return false;
	for (k=0; k<MBSWmetaList.size(); k++)
	{
		newadr = true;
		// CHECK IF ADDRESS IS ALREADY ON THE QUERY-LIST:
		if (_selMBsSWsAddr.size())
		{
			// CHECK IF ADDRESS IS ALREADY ON THE LIST:
			for (m=0; (m<_selMBsSWsAddr.size()); m++)
			{
				if (MBSWmetaList.at(k).blockType == 0)
				{
					// CHECK IF CURRENT MB IS VALID/EXISTS:
					if (MBSWmetaList.at(k).nativeIndex > _supportedMBs.size()) return false;
					// COMPARE ADDRESS:
					if (_selMBsSWsAddr.at(m) == (_supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).adr_low))
					{
						newadr = false;
						break;
					}
				}
				else
				{
					// CHECK IF CURRENT SW IS VALID/EXISTS:
					if (MBSWmetaList.at(k).nativeIndex > _supportedSWs.size()) return false;
					// COMPARE ADDRESS:
					if (_selMBsSWsAddr.at(m) == (_supportedSWs.at( MBSWmetaList.at(k).nativeIndex ).byteadr))
					{
						newadr = false;
						break;
					}
				}
			}
		}
		// ADD ADDRESS TO QUERY-LIST IF IT IS NEW:
		if (newadr)
		{
			if (MBSWmetaList.at(k).blockType == 0)
			{
				// ADD ADDRESS(ES) OF CURRENT MB TO LIST:
				_selMBsSWsAddr.push_back( _supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).adr_low );
				if (_supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).adr_high > 0)
					_selMBsSWsAddr.push_back( _supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).adr_high );
			}
			else
				// ADD ADDRESS OF CURRENT SW TO LIST:
				_selMBsSWsAddr.push_back( _supportedSWs.at( MBSWmetaList.at(k).nativeIndex ).byteadr );
		}
	}
	return true;
}


void SSMprotocol::processMBSWrawData(QByteArray MBSWrawdata, int duration_ms)
{
	std::vector<unsigned int> rawValues;
	QStringList valueStrList;
	QStringList unitStrList;
	assignMBSWRawData( MBSWrawdata, &rawValues );
	emit newMBSWrawValues(rawValues, duration_ms);
}


void SSMprotocol::assignMBSWRawData(QByteArray rawdata, std::vector<unsigned int> * mbswrawvalues)
{
	// ***** ASSIGN RAW DATA *****:
	unsigned int k = 0, m = 0;
	mbswrawvalues->clear();
	mbswrawvalues->resize(_MBSWmetaList.size(), 0);
	for (m=0; m<_selMBsSWsAddr.size(); m++)	// ADDRESS LOOP
	{
		for (k=0; k<_MBSWmetaList.size(); k++)	// MB/SW LOOP
		{
			if (_MBSWmetaList.at(k).blockType == 0)
			{
				// COMPARE ADDRESSES:
				if (_selMBsSWsAddr.at(m) == _supportedMBs.at( _MBSWmetaList.at(k).nativeIndex ).adr_low)
				{
					// ADDRESS/RAW BYTE CORRESPONDS WITH LOW BYTE ADDRESS OF MB
					mbswrawvalues->at(k) += static_cast<unsigned char>(rawdata.at(m));
				}
				else if (_selMBsSWsAddr.at(m) == _supportedMBs.at( _MBSWmetaList.at(k).nativeIndex ).adr_high)
				{
					// ADDRESS/RAW BYTE CORRESPONDS WITH HIGH BYTE ADDRESS OF MB
					mbswrawvalues->at(k) += static_cast<unsigned char>(rawdata.at(m)) * 256;
				}
			}
			else
			{
				if (_selMBsSWsAddr.at(m) == _supportedSWs.at( _MBSWmetaList.at(k).nativeIndex ).byteadr)
				{
					// ADDRESS/RAW BYTE CORRESPONS WITH BYTE ADDRESS OF SW
					if ( rawdata.at(m) & static_cast<char>(pow(2, (_supportedSWs.at( _MBSWmetaList.at(k).nativeIndex ).bitadr -1) ) ) )	// IF ADDRESS BIT IS SET
						mbswrawvalues->at(k) = 1;
					else	// IF ADDRESS BIT IS NOT SET
						mbswrawvalues->at(k) = 0;
				}
			}
		}
	}
}


bool SSMprotocol::startActuatorTest(unsigned char actuatorTestIndex)
{
	return false;
}


bool SSMprotocol::restartActuatorTest()
{
	return false;
}


bool SSMprotocol::stopActuatorTesting()
{
	if ((_state == state_needSetup) || (_state == state_normal)) return true;
	return false;
}


bool SSMprotocol::stopAllActuators()
{
	return false;
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



