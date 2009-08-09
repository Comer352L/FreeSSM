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


std::string SSMprotocol::getSysID()
{
	return "";
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


bool SSMprotocol::getSupportedMBs(std::vector<mbsw_dt> *supportedMBs)
{
	if (_state == state_needSetup) return false;
	supportedMBs->clear();
	for (unsigned int k=0; k<_supportedMBs.size(); k++)
		supportedMBs->push_back( _supportedMBs.at(k) );
	return true;
}


bool SSMprotocol::getSupportedSWs(std::vector<mbsw_dt> *supportedSWs)
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
	// Create list of set flagbits:
	unsigned char flagbit = 0;
	for (flagbit=1; flagbit<9; flagbit++)
	{
		if (DCrawdata & static_cast<char>(pow(2, (flagbit-1))))
		{
			setbits[setbitslen] = flagbit;
			setbitslen++;
		}
	}
	// *** Search for matching DC definition ***:
	for (k=0; k<DCdefs.size(); k++)
	{
		/* NOTE:	- unknown/reserved DCs have a definition with description "UNKNOWN ..."
				- DCs with missing definitions are ignored				*/
		if ((DCdefs.at(k).byteAddr_currentOrTempOrLatest == DCbyteadr) || (DCdefs.at(k).byteAddr_historicOrMemorized == DCbyteadr))
		{
			for (setbitsindex=0; setbitsindex<setbitslen; setbitsindex++)
			{
				// Check if DC is to be ignored:
				if (!(DCdefs.at(k).code[ setbits[setbitsindex]-1 ].isEmpty() && DCdefs.at(k).title[ setbits[setbitsindex]-1 ].isEmpty()))
				{
					DC->push_back( DCdefs.at(k).code[ setbits[setbitsindex]-1 ] );		// DC
					DCdescription->push_back( DCdefs.at(k).title[ setbits[setbitsindex]-1 ] );	// DC description
				}
				DCsAssigned[setbitsindex] = true;
			}
		}
	}
	// *** Add DCs without matching definition:
	for (k=0; k<setbitslen; k++)
	{
		if (!DCsAssigned[k])
		{
			if (_language == "de")
				ukdctitle = "UNBEKANNT (Adresse 0x";
			else
				ukdctitle = "UNKNOWN (Address 0x";
			ukdctitle += QString::number(DCbyteadr,16).toUpper() + " Bit " + QString::number(setbits[k]) + ")";
			DC->push_back("???");			// DC
			DCdescription->push_back(ukdctitle);	// DC description
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
	_selMBsSWsAddrLen = 0;
	unsigned int k = 0, m = 0;
	bool newadr = true;
	if (MBSWmetaList.size() == 0) return false;
	for (k=0; k<MBSWmetaList.size(); k++)
	{
		newadr = true;
		// CHECK IF ADDRESS IS ALREADY ON THE QUERY-LIST:
		if (_selMBsSWsAddrLen > 0)
		{
			// CHECK IF ADDRESS IS ALREADY ON THE LIST:
			for (m=0; (m<_selMBsSWsAddrLen); m++)
			{
				if (MBSWmetaList.at(k).blockType == 0)
				{
					// CHECK IF CURRENT MB IS VALID/EXISTS:
					if (MBSWmetaList.at(k).nativeIndex > _supportedMBs.size()) return false;
					// COMPARE ADDRESS:
					if (_selMBsSWsAddr[m] == (_supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).adr_low))
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
					if (_selMBsSWsAddr[m] == (_supportedSWs.at( MBSWmetaList.at(k).nativeIndex ).byteadr))
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
				_selMBsSWsAddr[_selMBsSWsAddrLen] = _supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).adr_low;
				_selMBsSWsAddrLen++;
				if (_supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).adr_high > 0)
				{
					_selMBsSWsAddr[_selMBsSWsAddrLen] = _supportedMBs.at( MBSWmetaList.at(k).nativeIndex ).adr_high;
					_selMBsSWsAddrLen++;
				}
			}
			else
			{
				// ADD ADDRESS OF CURRENT SW TO LIST:
				_selMBsSWsAddr[_selMBsSWsAddrLen] = _supportedSWs.at( MBSWmetaList.at(k).nativeIndex ).byteadr;
				_selMBsSWsAddrLen++;
			}
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
	for (m=0; m<_selMBsSWsAddrLen; m++)	// ADDRESS LOOP
	{
		for (k=0; k<_MBSWmetaList.size(); k++)	// MB/SW LOOP
		{
			if (_MBSWmetaList.at(k).blockType == 0)
			{
				// COMPARE ADDRESSES:
				if (_selMBsSWsAddr[m] == _supportedMBs.at( _MBSWmetaList.at(k).nativeIndex ).adr_low)
				{
					// ADDRESS/RAW BYTE CORRESPONDS WITH LOW BYTE ADDRESS OF MB
					mbswrawvalues->at(k) += static_cast<unsigned char>(rawdata.at(m));
				}
				else if (_selMBsSWsAddr[m] == _supportedMBs.at( _MBSWmetaList.at(k).nativeIndex ).adr_high)
				{
					// ADDRESS/RAW BYTE CORRESPONDS WITH HIGH BYTE ADDRESS OF MB
					mbswrawvalues->at(k) += static_cast<unsigned char>(rawdata.at(m)) * 256;
				}
			}
			else
			{
				if (_selMBsSWsAddr[m] == _supportedSWs.at( _MBSWmetaList.at(k).nativeIndex ).byteadr)
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



