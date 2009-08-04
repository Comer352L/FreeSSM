/*
 * SSMprotocol1.cpp - Application Layer for the old Subaru SSM protocol
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

#include "SSMprotocol1.h"



SSMprotocol1::SSMprotocol1(serialCOM *port, CUtype_dt CU, QString language) : SSMprotocol(port, CU, language)
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

	// TODO !
}
// IMPLEMENTATION MISSING

bool SSMprotocol1::setupCUdata(bool ignoreIgnitionOFF)
{

	// TODO !
	
return false;
}
// IMPLEMENTATION MISSING

std::string SSMprotocol1::getROMID()
{
	if (_state == state_needSetup) return "";
	return libFSSM::StrToHexstr(_ROM_ID, 3);
}


bool SSMprotocol1::getSystemDescription(QString *sysdescription)
{
	if (_state == state_needSetup) return false;

	// TODO !
	// => Get system description from definitions and copy to sysdescription

	return true;
}
// IMPLEMENTATION MISSING

bool SSMprotocol1::hasOBD2system(bool *OBD2)
{
	if (_state == state_needSetup) return false;
	*OBD2 = false;
	return true;
}


bool SSMprotocol1::hasVINsupport(bool *VINsup)
{
	if (_state == state_needSetup) return false;
	*VINsup = false;
	return true;
}


bool SSMprotocol1::hasImmobilizer(bool *ImmoSup)
{
	if (_state == state_needSetup) return false;

	// TODO !

return false;
}
// IMPLEMENTATION MISSING

bool SSMprotocol1::hasIntegratedCC(bool *CCsup)
{
	if (_state == state_needSetup) return false;
	*CCsup = false;
	return true;
}


bool SSMprotocol1::hasClearMemory2(bool *CM2sup)
{
	if (_state == state_needSetup) return false;
	*CM2sup = false;
	return true;
}


bool SSMprotocol1::hasTestMode(bool *TMsup)
{
	if (_state == state_needSetup) return false;
	*TMsup = false;
	return true;
}


bool SSMprotocol1::hasActuatorTests(bool *ATsup)
{
	if (_state == state_needSetup) return false;
	*ATsup = false;
	return true;
}


void SSMprotocol1::setupDTCaddresses()
{

	// TODO !
	// => Get supported DTC addresses from definitions and save to _currOrTempDTCsAddr, _histOrMemDTCsAddr

}
// IMPLEMENTATION MISSING

void SSMprotocol1::setupSupportedMBs()
{

	// TODO !
	// => Get supported MBs from definitions and save to _supportedMBs

}
// IMPLEMENTATION MISSING

void SSMprotocol1::setupSupportedSWs()
{

	// TODO !
	// => Get supported SWs from definitions and save to _supportedSWs

}
// IMPLEMENTATION MISSING

bool SSMprotocol1::getSupportedDCgroups(int *DCgroups)
{
	int retDCgroups = 0;
	bool supported = false;
	if (_state == state_needSetup) return false;
	if (!_currOrTempDTCsAddr.empty())
		retDCgroups += currentDTCs_DCgroup;
	if (!_histOrMemDTCsAddr.empty())
		retDCgroups += historicDTCs_DCgroup;
	*DCgroups = retDCgroups;
	return true;
}


bool SSMprotocol1::getSupportedAdjustments(std::vector<adjustment_dt> *supportedAdjustments)
{
	if (_state == state_needSetup) return false;
	supportedAdjustments->clear();
	return true;
}


bool SSMprotocol1::isEngineRunning(bool *isrunning)
{
	if (_state != state_normal) return false;

	// TODO !
	
return false;
}
// IMPLEMENTATION MISSING; DO WE NEED IT ?

bool SSMprotocol1::ClearMemory(CMlevel_dt level, bool *success)
{
	if (_state != state_normal) return false;
	if (level == CMlevel_2) return false;

	// TODO !
	
return false;
}
// IMPLEMENTATION MISSING

bool SSMprotocol1::TestImmobilizerCommLine(immoTestResult_dt *result)
{

	// TODO !

return false;
}
// IMPLEMENTATION MISSING

bool SSMprotocol1::startDCreading(int DCgroups)
{
	std::vector<unsigned int> DCqueryAddrList;
	unsigned char k = 0;
	bool started;
	// Check if another communication operation is in progress:
	if (_state != state_normal) return false;
	// Check argument:
	if (!((DCgroups == 1) || (DCgroups == 4) || (DCgroups == 5))) return false;
	// Setup diagnostic codes addresses list:
	if (DCgroups & currentDTCs_DCgroup)	// current DTCs
	{
		for (k=0; k<_currOrTempDTCsAddr.size(); k++)
			DCqueryAddrList.push_back( _currOrTempDTCsAddr.at(k) );
	}
	if (DCgroups & historicDTCs_DCgroup)	// historic DTCs
	{
		for (k=0; k<_histOrMemDTCsAddr.size(); k++)
			DCqueryAddrList.push_back( _histOrMemDTCsAddr.at(k) );
	}
	// Check if min. 1 Address to read:
	if (!DCqueryAddrList.size())
		return false;
	// Start diagostic code reading:
	started = _SSMP1com->readAddresses_permanent( DCqueryAddrList );
	if (started)
	{
		_state = state_DCreading;
		// Save diagnostic codes group selection (for data evaluation and restartDCreading()):
		_selectedDCgroups = DCgroups;
		// Connect signals and slots:
		connect( _SSMP1com, SIGNAL( recievedData(QByteArray, int) ),
			this, SLOT( processDCsRawdata(QByteArray, int) ), Qt::BlockingQueuedConnection );
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
			disconnect( _SSMP1com, SIGNAL( recievedData(QByteArray, int) ),
				    this, SLOT( processDCsRawdata(QByteArray, int) ) );
			_state = state_normal;
			emit stoppedDCreading();
			return true;
		}
		// Communication error:
		resetCUdata();
	}
	return false;
}


void SSMprotocol1::processDCsRawdata(QByteArray DCrawdata, int duration_ms)
{
	QStringList DCs;
	QStringList DCdescriptions;
	QStringList tmpDTCs;
	QStringList tmpDTCsDescriptions;
	unsigned int DCsAddrIndex = 0;
	unsigned int DCsAddrIndexOffset = 0;
	duration_ms = 0; // to avoid compiler error
	if (_selectedDCgroups & currentDTCs_DCgroup)
	{
		DCs.clear();
		DCdescriptions.clear();
		// Evaluate current/latest data trouble codes:
		for (DCsAddrIndex=0; DCsAddrIndex<_currOrTempDTCsAddr.size(); DCsAddrIndex++)
		{
//			evaluateDCdataByte(_currOrTempDTCsAddr.at(DCsAddrIndex), DCrawdata.at(DCsAddrIndexOffset+DCsAddrIndex), _DTC_rawDefs, &tmpDTCs, &tmpDTCsDescriptions);
			DCs += tmpDTCs;
			DCdescriptions += tmpDTCsDescriptions;
		}
		DCsAddrIndexOffset += _currOrTempDTCsAddr.size();
		emit currentOrTemporaryDTCs(DCs, DCdescriptions, false, false);
	}
	if (_selectedDCgroups & historicDTCs_DCgroup)
	{
		DCs.clear();
		DCdescriptions.clear();
		// Evaluate historic/memorized data trouble codes:
		for (DCsAddrIndex=0; DCsAddrIndex<_histOrMemDTCsAddr.size(); DCsAddrIndex++)
		{
//			evaluateDCdataByte(_histOrMemDTCsAddr.at(DCsAddrIndex), DCrawdata.at(DCsAddrIndexOffset+DCsAddrIndex), _DTC_rawDefs, &tmpDTCs, &tmpDTCsDescriptions);
			DCs += tmpDTCs;
			DCdescriptions += tmpDTCsDescriptions;
		}
		DCsAddrIndexOffset += _histOrMemDTCsAddr.size();
		emit historicOrMemorizedDTCs(DCs, DCdescriptions);
	}
}
// INCOMPLETE; evaluateDCdataByte(); TestMode, DCheckActive always false ???;

bool SSMprotocol1::startMBSWreading(std::vector<MBSWmetadata_dt> mbswmetaList)
{
	bool started = false;
	if (_state != state_normal) return false;
	// Setup list of MB/SW-addresses for SSM2Pcommunication:
	if (!setupMBSWQueryAddrList(mbswmetaList))
		return false;
	// Start MB/SW-reading:
	started = _SSMP1com->readAddresses_permanent( std::vector<unsigned int>(_selMBsSWsAddr, _selMBsSWsAddr + _selMBsSWsAddrLen) );
	if (started)
	{
		_state = state_MBSWreading;
		// Save MB/SW-selection (necessary for evaluation of raw data):
		_MBSWmetaList = mbswmetaList;
		// Connect signals/slots:
		connect( _SSMP1com, SIGNAL( recievedData(QByteArray, int) ),
			this, SLOT( processMBSWrawData(QByteArray, int) ) ); 
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
			disconnect( _SSMP1com, SIGNAL( recievedData(QByteArray, int) ),
				    this, SLOT( processMBSWrawData(QByteArray, int) ) );
			_state = state_normal;
			emit stoppedMBSWreading();
			return true;
		}
		// Communication error:
		resetCUdata();
	}
	return false;
}


bool SSMprotocol1::waitForIgnitionOff()
{

	// TODO !

return true;
/* NOTE: temporary solution, will become obsolete with extended SSMP1communication */
}
// IMPLEMENTATION MISSING

