/*
 * SSMprotocol1.cpp - Application Layer for the old Subaru SSM protocol
 *
 * Copyright (C) 2009-2010 Comer352l
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


#define	SSM1_MEM_ADDR_NONE	0xFFFFFFFF


SSMprotocol1::SSMprotocol1(AbstractDiagInterface *diagInterface, QString language) : SSMprotocol(diagInterface, language)
{
	_SSMP1com = NULL;
	_CMaddr = SSM1_MEM_ADDR_NONE;
	_CMvalue = '\x00';
	resetCUdata();
}


SSMprotocol1::~SSMprotocol1()
{
	resetCUdata();
}


void SSMprotocol1::resetCUdata()
{
	// RESET COMMUNICATION:
	if (_SSMP1com != NULL)
	{
		// Disconnect communication error and data signals:
		disconnect( _SSMP1com, SIGNAL( commError() ), this, SIGNAL( commError() ) );
		disconnect( _SSMP1com, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
		disconnect( _SSMP1com, SIGNAL( recievedData(std::vector<char>, int) ),
			    this, SLOT( processDCsRawdata(std::vector<char>, int) ) );
		disconnect( _SSMP1com, SIGNAL( recievedData(std::vector<char>, int) ),
			    this, SLOT( processMBSWrawData(std::vector<char>, int) ) );
		// Try to stop active communication processes:
		_SSMP1com->stopCommunication();
		// NOTE: DO NOT CALL stopCommOperation() or any other communicating functions here because of possible recursions !
		_state = state_needSetup;	// MUST BE DONE AFTER ALL CALLS OF MEMBER-FUNCTIONS AND BEFORE EMITTING SIGNALS
		delete _SSMP1com;
		_SSMP1com = NULL;
		// Emit stoppedXXX()-signals (_SSMP1com has been deleted, so we are sure they have finished):
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
	// RESET ECU DATA:
	_ID[0] = '\x0';
	_ID[1] = '\x0';
	_ID[2] = '\x0';
	// Clear system description:
	_sysDescription.clear();
	// Clear DC data:
	_DTCdefs.clear();
	_CMaddr = SSM1_MEM_ADDR_NONE;
	_CMvalue = '\x00';
	// Reset MB/SW data:
	_supportedMBs.clear();
	_supportedSWs.clear();
	// Clear selection data:
	_selectedDCgroups = noDCs_DCgroup;
	_MBSWmetaList.clear();
	_selMBsSWsAddr.clear();
}


bool SSMprotocol1::setupCUdata(CUtype_dt CU)
{
	std::string defsFile;
	SSM1_CUtype_dt SSM1_CU;
	// Reset:
	resetCUdata();
	// Create SSMP1communication-object:
	if (CU == CUtype_Engine)
	{
		SSM1_CU = SSM1_CU_Engine;
		defsFile = QCoreApplication::applicationDirPath().toStdString() + "/SSM1defs_ECU.xml";
	}
	else if (CU == CUtype_Transmission)
	{
		SSM1_CU = SSM1_CU_Transmission;
		defsFile = QCoreApplication::applicationDirPath().toStdString() + "/SSM1defs_TCU.xml";
	}
	else if (CU == CUtype_CruiseControl)
	{
		SSM1_CU = SSM1_CU_CruiseCtrl;
	}
	else if (CU == CUtype_AirCon)
	{
		SSM1_CU = SSM1_CU_AirCon;
	}
	else if (CU == CUtype_FourWheelSteering)
	{
		SSM1_CU = SSM1_CU_FourWS;
	}
	else
		return false;
	_SSMP1com = new SSMP1communication(_diagInterface, SSM1_CU);
	// Get control unit ID:
	if (!_SSMP1com->readID(_ID))
		 return false;
	_CU = CU;
	_state = state_normal;
	// Connect communication error signals from SSMP1communication:
	connect( _SSMP1com, SIGNAL( commError() ), this, SIGNAL( commError() ) );
	connect( _SSMP1com, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
	// Load control unit definitions:
	SSM1definitionsInterface defsIface;
	if (defsIface.selectDefinitionsFile(defsFile))
	{
		defsIface.setLanguage(_language.toStdString());
		defsIface.selectID(_ID);
		// Get system description:
		defsIface.systemDescription(&_sysDescription);
		// Get definitions of the supported diagnostic codes:
		defsIface.diagnosticCodes(&_DTCdefs);
		// Get supported MBs and SWs:
		defsIface.measuringBlocks(&_supportedMBs);
		defsIface.switches(&_supportedSWs);
		// Get Clear Memory data:
		defsIface.clearMemoryData(&_CMaddr, &_CMvalue);
	}
	return true;
	
	/* TODO:
		- setup test-addresses for Immobilizer-communication-line
		- does the communication always immediately abort when ignition is switched off ?
	*/
	
}
// INCOMPLETE IMPLEMENTATION

std::string SSMprotocol1::getSysID()
{
	if (_state == state_needSetup) return "";
	return libFSSM::StrToHexstr(_ID, 3);
}


std::string SSMprotocol1::getROMID()
{
	return getSysID();
}


bool SSMprotocol1::getSystemDescription(QString *sysdescription)
{
	if (_state == state_needSetup) return false;
	if (_sysDescription.size())
	{
		*sysdescription = QString::fromStdString(_sysDescription);
		return true;
	}
	return false;
}


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

*ImmoSup = false;
return true;
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


bool SSMprotocol1::getSupportedDCgroups(int *DCgroups)
{
	int retDCgroups = 0;
	if (_state == state_needSetup) return false;
	if (_DTCdefs.size())
		retDCgroups |= currentDTCs_DCgroup | historicDTCs_DCgroup;
	*DCgroups = retDCgroups;
	return true;
}
// CHECK: really always Current and Historic DTCs supported ?

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

bool SSMprotocol1::clearMemory(CMlevel_dt level, bool *success)
{
	if (_state != state_normal) return false;
	if (level == CMlevel_2) return false;
	if (_CMaddr == SSM1_MEM_ADDR_NONE) return false;
	if (!_SSMP1com->writeAddress(_CMaddr, _CMvalue))
	{
		resetCUdata();
		return false;
	}
	return true;
}


bool SSMprotocol1::testImmobilizerCommLine(immoTestResult_dt *result)
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
		for (k=0; k<_DTCdefs.size(); k++)
			DCqueryAddrList.push_back( _DTCdefs.at(k).byteAddr_currentOrTempOrLatest );
	}
	if (DCgroups & historicDTCs_DCgroup)	// historic DTCs
	{
		for (k=0; k<_DTCdefs.size(); k++)
			DCqueryAddrList.push_back( _DTCdefs.at(k).byteAddr_historicOrMemorized );
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
		connect( _SSMP1com, SIGNAL( recievedData(std::vector<char>, int) ),
			this, SLOT( processDCsRawdata(std::vector<char>, int) ), Qt::BlockingQueuedConnection );
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
			disconnect( _SSMP1com, SIGNAL( recievedData(std::vector<char>, int) ),
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


void SSMprotocol1::processDCsRawdata(std::vector<char> DCrawdata, int duration_ms)
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
		for (DCsAddrIndex=0; DCsAddrIndex<_DTCdefs.size(); DCsAddrIndex++)
		{
			evaluateDCdataByte(_DTCdefs.at(DCsAddrIndex).byteAddr_currentOrTempOrLatest, DCrawdata.at(DCsAddrIndexOffset+DCsAddrIndex), _DTCdefs, &tmpDTCs, &tmpDTCsDescriptions);
			DCs += tmpDTCs;
			DCdescriptions += tmpDTCsDescriptions;
		}
		DCsAddrIndexOffset += _DTCdefs.size();
		emit currentOrTemporaryDTCs(DCs, DCdescriptions, false, false);
	}
	if (_selectedDCgroups & historicDTCs_DCgroup)
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
}
// INCOMPLETE; TestMode, DCheckActive always false ???;

bool SSMprotocol1::startMBSWreading(std::vector<MBSWmetadata_dt> mbswmetaList)
{
	bool started = false;
	if (_state != state_normal) return false;
	// Setup list of MB/SW-addresses for SSM2Pcommunication:
	if (!setupMBSWQueryAddrList(mbswmetaList))
		return false;
	// Start MB/SW-reading:
	started = _SSMP1com->readAddresses_permanent( _selMBsSWsAddr );
	if (started)
	{
		_state = state_MBSWreading;
		// Save MB/SW-selection (necessary for evaluation of raw data):
		_MBSWmetaList = mbswmetaList;
		// Connect signals/slots:
		connect( _SSMP1com, SIGNAL( recievedData(std::vector<char>, int) ),
			this, SLOT( processMBSWrawData(std::vector<char>, int) ) ); 
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
			disconnect( _SSMP1com, SIGNAL( recievedData(std::vector<char>, int) ),
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


bool SSMprotocol1::waitForIgnitionOff()
{
	if (_state != state_normal)
		return false;
	_state = state_waitingForIgnOff;
	_SSMP1com->setRetriesOnError(1);
	disconnect( _SSMP1com, SIGNAL( commError() ), this, SIGNAL( commError() ) );
	disconnect( _SSMP1com, SIGNAL( commError() ), this, SLOT( resetCUdata() ) );
	if (!_SSMP1com->readAddress_permanent( 0x0000 ))
	{
		resetCUdata();
		return false;
	}
	QEventLoop el;
	connect(_SSMP1com, SIGNAL( commError() ), &el, SLOT( quit() ));
	el.exec();
	disconnect(_SSMP1com, SIGNAL( commError() ), &el, SLOT( quit() ));
	_SSMP1com->setRetriesOnError(2);
	resetCUdata();
	return true;
/* NOTE: temporary solution, will become obsolete with extended SSMP1communication */
}
// USE SWITCH "ignition" (if exists) ?

