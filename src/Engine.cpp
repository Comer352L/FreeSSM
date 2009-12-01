/*
 * Engine.cpp - Engine Control Unit dialog
 *
 * Copyright (C) 2008-2009 Comer352l
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

#include "Engine.h"


Engine::Engine(serialCOM *port, QString language, QString progversion) : ControlUnitDialog(tr("Engine Control Unit"), port, language, progversion)
{
	// *** Initialize global variables:
	_content_DCs = NULL;
	_content_MBsSWs = NULL;
	_content_Adjustments = NULL;
	_content_SysTests = NULL;
	_lastMBSWmetaList.clear();
	_mode = DCs_mode;	// we start in Diagnostic Codes mode
	// Show information-widget:
	_infoWidget = new CUinfo_Engine(infoGroupBox());
	_infoWidget->show();
	// Setup functions:
	_DCs_pushButton = addFunction(tr(" &Diagnostic Codes  "), QIcon(QString::fromUtf8(":/icons/chrystal/22x22/messagebox_warning.png")), true);
	_DCs_pushButton->setChecked(true);
	connect( _DCs_pushButton, SIGNAL( released() ), this, SLOT( DCs() ) );
	_measuringblocks_pushButton = addFunction(tr(" &Measuring Blocks "), QIcon(QString::fromUtf8(":/icons/oxygen/22x22/applications-utilities.png")), true);
	connect( _measuringblocks_pushButton, SIGNAL( released() ), this, SLOT( measuringblocks() ) );
	_adjustments_pushButton = addFunction(tr("     &Adjustments     "), QIcon(QString::fromUtf8(":/icons/chrystal/22x22/configure.png")), true);
	connect( _adjustments_pushButton, SIGNAL( released() ), this, SLOT( adjustments() ) );
	_systemoperationtests_pushButton = addFunction(tr("    System &Tests    "), QIcon(QString::fromUtf8(":/icons/chrystal/22x22/klaptop.png")), true);
	connect( _systemoperationtests_pushButton, SIGNAL( released() ), this, SLOT( systemoperationtests() ) );
	QPushButton *cmbutton = addFunction(tr("    Clear Memory    "), QIcon(QString::fromUtf8(":/icons/chrystal/22x22/eraser.png")), false);
	connect( cmbutton, SIGNAL( released() ), this, SLOT( clearMemory() ) );
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
	// Load/Show Diagnostic Code content:
	contentGroupBox()->setTitle(tr("Diagnostic Codes:"));
	_content_DCs = new CUcontent_DCs_engine(contentGroupBox(), _progversion);
	contentGroupBox()->layout()->addWidget(_content_DCs);
	_content_DCs->show();
	// Make GUI visible
	this->show();
	// Connect to Control Unit, get data and setup GUI:
	setup();
}


Engine::~Engine()
{
	clearContent();
	delete _infoWidget;
}


void Engine::setup()
{
	QString sysdescription = "";
	std::string ROM_ID = "";
	QString VIN = "";
	bool supported = false;
	bool testmode = false;
	bool enginerunning = false;
	std::vector<mb_dt> supportedMBs;
	std::vector<sw_dt> supportedSWs;
	int supDCgroups = 0;
	// ***** Connect to Control Unit *****:
	// Create Status information message box for CU initialisation/setup:
	FSSM_InitStatusMsgBox initstatusmsgbox(tr("Connecting to ECU... Please wait !"), 0, 0, 100, this);
	initstatusmsgbox.setWindowTitle(tr("Connecting to ECU..."));
	initstatusmsgbox.setValue(5);
	initstatusmsgbox.show();
	// Try to establish CU connection:
	if( probeProtocol(SSMprotocol::CUtype_Engine) )
	{
		// Update status info message box:
		initstatusmsgbox.setLabelText(tr("Processing ECU data... Please wait !"));
		initstatusmsgbox.setValue(40);
		// Query ROM-ID:
		ROM_ID = _SSMPdev->getROMID();
		if (!ROM_ID.length())
			goto commError;
		// Query system description:
		if (!_SSMPdev->getSystemDescription(&sysdescription))
		{
			std::string SYS_ID = _SSMPdev->getSysID();
			if (!SYS_ID.length())
				goto commError;
			sysdescription = tr("unknown");
			if (SYS_ID != ROM_ID)
				sysdescription += " (" + QString::fromStdString(SYS_ID) + ")";
		}
		// Output system description:
		_infoWidget->setEngineTypeText(sysdescription);
		// Output ROM-ID:
		_infoWidget->setRomIDText( QString::fromStdString(ROM_ID) );
		// Number of supported MBs / SWs:
		if ((!_SSMPdev->getSupportedMBs(&supportedMBs)) || (!_SSMPdev->getSupportedSWs(&supportedSWs)))
			goto commError;
		_infoWidget->setNrOfSupportedMBsSWs(supportedMBs.size(), supportedSWs.size());
		// OBD2-Support:
		if (!_SSMPdev->hasOBD2system(&supported))
			goto commError;
		_infoWidget->setOBD2Supported(supported);
		// Integrated Cruise Control:
		if (!_SSMPdev->hasIntegratedCC(&supported))
			goto commError;
		_infoWidget->setIntegratedCCSupported(supported);
		// Immobilizer:
		if (!_SSMPdev->hasImmobilizer(&supported))
			goto commError;
		_infoWidget->setImmobilizerSupported(supported);
		// // Update status info message box:
		initstatusmsgbox.setLabelText(tr("Reading Vehicle Ident. Number... Please wait !"));
		initstatusmsgbox.setValue(55);
		// Query and output VIN, if supported:
		if (!_SSMPdev->hasVINsupport(&supported))
			goto commError;
		if (supported)
		{
			if (!_SSMPdev->getVIN(&VIN))
				goto commError;
		}
		_infoWidget->setVINinfo(supported, VIN);
		// Check if we need to stop the automatic actuator test:
		if (!_SSMPdev->hasActuatorTests(&supported))
			goto commError;
		if (supported)
		{
			// Update status info message box:
			initstatusmsgbox.setLabelText(tr("Checking system status... Please wait !"));
			initstatusmsgbox.setValue(70);
			// Query test mode connector status:
			if (!_SSMPdev->isInTestMode(&testmode)) // if actuator tests are available, test mode is available, too...
				goto commError;
			if (testmode)	// wenn ECU im Testmodus
			{
				// Check that engine is not running:
				if (!_SSMPdev->isEngineRunning(&enginerunning)) // if actuator tests are available, MB "engine speed" is available, too...
					goto commError;
				if (!enginerunning)
				{
					// Update status info message box:
					initstatusmsgbox.setLabelText(tr("Stopping actuators... Please wait !"));
					initstatusmsgbox.setValue(85);
					// Stop all actuator tests:
					if (!_SSMPdev->stopAllActuators())
						goto commError;
				}
			}
		}
	}
	else // CU-connection could not be established
		goto commError;
	// Start Diagnostic Codes reading:
	if (!_content_DCs->setup(_SSMPdev))
		goto commError;
	if (!_SSMPdev->getSupportedDCgroups(&supDCgroups))
		goto commError;
	if (supDCgroups != SSMprotocol::noDCs_DCgroup)
	{
		if (!_content_DCs->startDCreading())
			goto commError;
	}
	connect(_content_DCs, SIGNAL( error() ), this, SLOT( close() ) );
	// Update and close status info:
	initstatusmsgbox.setLabelText(tr("ECU-initialisation successful !"));
	initstatusmsgbox.setValue(100);
	QTimer::singleShot(800, &initstatusmsgbox, SLOT(accept()));
	initstatusmsgbox.exec();
	initstatusmsgbox.close();
	return;

commError:
	initstatusmsgbox.close();
	communicationError();
}


void Engine::DCs()
{
	bool ok = false;
	int DCgroups = 0;
	if (_mode == DCs_mode) return;
	_mode = DCs_mode;
	_DCs_pushButton->setChecked(true);
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to Diagnostic Codes... Please wait !   "));
	waitmsgbox.show();
	// Remove old content:
	clearContent();
	// Set title of the content group-box:
	contentGroupBox()->setTitle(tr("Diagnostic Codes:"));
	// Create, setup and insert content-widget:
	_content_DCs = new CUcontent_DCs_engine(contentGroupBox(), _progversion);
	contentGroupBox()->layout()->addWidget(_content_DCs);
	_content_DCs->show();
	ok = _content_DCs->setup(_SSMPdev);
	// Start DC-reading:
	if (ok)
		ok = _SSMPdev->getSupportedDCgroups(&DCgroups);
		if (ok && DCgroups != SSMprotocol::noDCs_DCgroup)
			ok = _content_DCs->startDCreading();
	// Get notification, if internal error occures:
	if (ok)
		connect(_content_DCs, SIGNAL( error() ), this, SLOT( close() ) );
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (!ok)
		communicationError();
}


void Engine::measuringblocks()
{
	bool ok = false;
	if (_mode == MBsSWs_mode) return;
	_mode = MBsSWs_mode;
	_measuringblocks_pushButton->setChecked(true);
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to Measuring Blocks... Please wait !   "));
	waitmsgbox.show();
	// Remove old content:
	clearContent();
	// Set title of the content group-box:
	contentGroupBox()->setTitle(tr("Measuring Blocks:"));
	// Create, setup and insert content-widget:
	_content_MBsSWs = new CUcontent_MBsSWs(contentGroupBox(), _MBSWsettings);
	contentGroupBox()->layout()->addWidget(_content_MBsSWs);
	_content_MBsSWs->show();
	ok = _content_MBsSWs->setup(_SSMPdev);
	if (ok)
		ok = _content_MBsSWs->setMBSWselection(_lastMBSWmetaList);
	// Get notification, if internal error occures:
	if (ok)
		connect(_content_MBsSWs, SIGNAL( error() ), this, SLOT( close() ) );
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (!ok)
		communicationError();
}


void Engine::adjustments()
{
	bool ok = false;
	if (_mode == Adaptions_mode) return;
	_mode = Adaptions_mode;
	_adjustments_pushButton->setChecked(true);
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to Adjustment Values... Please wait !   "));
	waitmsgbox.show();
	// Remove old content:
	clearContent();
	// Set title of the content group-box:
	contentGroupBox()->setTitle(tr("Adjustments:"));
	// Create, setup and insert content-widget:
	_content_Adjustments = new CUcontent_Adjustments(contentGroupBox());
	contentGroupBox()->layout()->addWidget(_content_Adjustments);
	_content_Adjustments->show();
	ok = _content_Adjustments->setup(_SSMPdev);
	if (ok)
		connect(_content_Adjustments, SIGNAL( communicationError() ), this, SLOT( close() ) );
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (!ok)
		communicationError();
}


void Engine::systemoperationtests()
{
	bool ok = false;
	if (_mode == SysTests_mode) return;
	_mode = SysTests_mode;
	_systemoperationtests_pushButton->setChecked(true);
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to System Tests... Please wait !   "));
	waitmsgbox.show();
	// Remove old content:
	clearContent();
	// Set title of the content group-box:
	contentGroupBox()->setTitle(tr("System Operation Tests:"));
	// Create, setup and insert content-widget:
	_content_SysTests = new CUcontent_sysTests(contentGroupBox());
	contentGroupBox()->layout()->addWidget(_content_SysTests);
	_content_SysTests->show();
	ok = _content_SysTests->setup(_SSMPdev);
	// Get notification, if internal error occures:
	if (ok)
		connect(_content_SysTests, SIGNAL( error() ), this, SLOT( close() ) );
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (!ok)
		communicationError();
}


void Engine::clearMemory()
{
	bool ok = false;
	ClearMemoryDlg::CMresult_dt result;
	// Create "Clear Memory"-dialog:
	ClearMemoryDlg cmdlg(this, _SSMPdev, SSMprotocol::CMlevel_1);
	// Temporary disconnect from "communication error"-signal:
	disconnect(_SSMPdev, SIGNAL( commError() ), this, SLOT( communicationError() ));
	// Run "Clear Memory"-procedure:
	result = cmdlg.run();
	// Reconnect to "communication error"-signal:
	connect(_SSMPdev, SIGNAL( commError() ), this, SLOT( communicationError() ));
	// Check result:
	if ((result == ClearMemoryDlg::CMresult_success) && (_mode == Adaptions_mode))
	{
		FSSM_WaitMsgBox waitmsgbox(this, tr("Reading Adjustment Values... Please wait !   "));
		waitmsgbox.show();
		ok = _content_Adjustments->setup(_SSMPdev); // refresh adjustment values
		waitmsgbox.close();
		if (!ok)
			communicationError();
	}
	else if (result == ClearMemoryDlg::CMresult_communicationError)
	{
		communicationError();
	}
	else if ((result == ClearMemoryDlg::CMresult_reconnectAborted) || (result == ClearMemoryDlg::CMresult_reconnectFailed))
	{
		close(); // exit engine control unit dialog
	}
}


void Engine::clearContent()
{
	// Delete content widget(s):
	if (_content_DCs != NULL)
	{
		disconnect(_content_DCs, SIGNAL( error() ), this, SLOT( close() ) );
		delete _content_DCs;
		_content_DCs = NULL;
	}
	if (_content_MBsSWs != NULL)
	{
		disconnect(_content_MBsSWs, SIGNAL( error() ), this, SLOT( close() ) );
		_content_MBsSWs->getMBSWselection(&_lastMBSWmetaList);
		_content_MBsSWs->getSettings(&_MBSWsettings);
		delete _content_MBsSWs;
		_content_MBsSWs = NULL;
	}
	if (_content_Adjustments != NULL)
	{
		disconnect(_content_Adjustments, SIGNAL( communicationError() ), this, SLOT( close() ) );
		delete _content_Adjustments;
		_content_Adjustments = NULL;
	}
	if (_content_SysTests != NULL)
	{
		disconnect(_content_SysTests, SIGNAL( error() ), this, SLOT( close() ) );
		delete _content_SysTests;
		_content_SysTests = NULL;
	}
}

