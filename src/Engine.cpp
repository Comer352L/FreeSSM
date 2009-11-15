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


Engine::Engine(serialCOM *port, QString language, QString progversion)
{
	// *** Initialize global variables:
	_language = language;
	_port = port;
	_SSMPdev = NULL;
	_progversion = progversion;
	_content_DCs = NULL;
	_content_MBsSWs = NULL;
	_content_Adjustments = NULL;
	_content_SysTests = NULL;
	_lastMBSWmetaList.clear();
	_mode = DCs_mode;	// we start in Diagnostic Codes mode
	// *** Setup window/GUI:
	setAttribute(Qt::WA_DeleteOnClose, true);
	// Setup GUI:
	setupUi(this);
	setupUiFonts();
	// Move window to desired coordinates
	move( 30, 30 );
	// Set window title:
	QString wintitle = "FreeSSM " + progversion + " - " + windowTitle();
	setWindowTitle(wintitle);
	// Load/Show Diagnostic Code content:
	content_groupBox->setTitle(tr("Diagnostic Codes:"));
	DCs_pushButton->setChecked(true);
	_content_DCs = new CUcontent_DCs_engine(content_groupBox, _progversion);
	content_gridLayout->addWidget(_content_DCs);
	_content_DCs->show();
	// Make GUI visible
	this->show();
	// Connect to Control Unit, get data and setup GUI:
	setup();
}


Engine::~Engine()
{
	disconnect( DCs_pushButton, SIGNAL( released() ), this, SLOT( DCs() ) );
	disconnect( measuringblocks_pushButton, SIGNAL( released() ), this, SLOT( measuringblocks() ) );
	disconnect( adjustments_pushButton, SIGNAL( released() ), this, SLOT( adjustments() ) );
	disconnect( systemoperationtests_pushButton, SIGNAL( released() ), this, SLOT( systemoperationtests() ) );
	disconnect( clearMemory_pushButton, SIGNAL( released() ), this, SLOT( clearMemory() ) );
	disconnect( exit_pushButton, SIGNAL( released() ), this, SLOT( close() ) );
	clearContent();
	if (_SSMPdev)
	{
		disconnect( _SSMPdev, SIGNAL( commError() ), this, SLOT( communicationError() ) );
		delete _SSMPdev;
	}
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
	QColor VINcolor;
	QPalette VINlabel_palette;
	QPixmap sup_pixmap(QString::fromUtf8(":/icons/chrystal/22x22/ok.png"));
	QPixmap nsup_pixmap(QString::fromUtf8(":/icons/chrystal/22x22/editdelete.png"));
	// ***** Connect to Control Unit *****:
	// Create Status information message box for CU initialisation/setup:
	FSSM_InitStatusMsgBox initstatusmsgbox(tr("Connecting to ECU... Please wait !"), 0, 0, 100, this);
	initstatusmsgbox.setWindowTitle(tr("Connecting to ECU..."));
	initstatusmsgbox.setValue(5);
	initstatusmsgbox.show();
	// Try to establish CU connection:
	if( probeProtocol() )
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
		enginetype_label->setText(sysdescription);
		// Output ROM-ID:
		romID_label->setText( QString::fromStdString(ROM_ID) );
		// Number of supported MBs / SWs:
		if (!_SSMPdev->getSupportedMBs(&supportedMBs))
			goto commError;
		nrofdatambs_label->setText( QString::number(supportedMBs.size(), 10) );
		if (!_SSMPdev->getSupportedSWs(&supportedSWs))
			goto commError;
		nrofswitches_label->setText( QString::number(supportedSWs.size(), 10) );
		// OBD2-Support:
		if (!_SSMPdev->hasOBD2system(&supported))
			goto commError;
		if (supported)
			obd2system_label->setPixmap(sup_pixmap);
		else
			obd2system_label->setPixmap(nsup_pixmap);
		// Integrated Cruise Control:
		if (!_SSMPdev->hasIntegratedCC(&supported))
			goto commError;
		if (supported)
			ccintegrated_label->setPixmap(sup_pixmap);
		else
			ccintegrated_label->setPixmap(nsup_pixmap);
		// Immobilizer:
		if (!_SSMPdev->hasImmobilizer(&supported))
			goto commError;
		if (supported)
			immobilizer_label->setPixmap(sup_pixmap);
		else
			immobilizer_label->setPixmap(nsup_pixmap);
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
			if (VIN.size() == 0)
			{
				VIN_label->setText(tr("not programmed yet"));
				VINcolor.setRgb( 255, 170, 0, 255);	// r,g,b,alpha: orange
			}
			else
			{
				VIN_label->setText(VIN);
				VINcolor.setRgb( 0, 170, 0, 255);	// r,g,b,alpha: green
			}
		}
		else
		{
			VIN_label->setText(tr("not supported by ECU"));
			VINcolor.setRgb( 255, 0, 0, 255);	// r,g,b,alpha: red
		}
		VINlabel_palette = VIN_label->palette();
		VINlabel_palette.setColor(QPalette::Active, QPalette::WindowText, VINcolor);
		VINlabel_palette.setColor(QPalette::Inactive, QPalette::WindowText, VINcolor);
		VIN_label->setPalette(VINlabel_palette);
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
	// Connect signals/slots:
	connect( DCs_pushButton, SIGNAL( released() ), this, SLOT( DCs() ) );
	connect( measuringblocks_pushButton, SIGNAL( released() ), this, SLOT( measuringblocks() ) );
	connect( adjustments_pushButton, SIGNAL( released() ), this, SLOT( adjustments() ) );
	connect( systemoperationtests_pushButton, SIGNAL( released() ), this, SLOT( systemoperationtests() ) );
	connect( clearMemory_pushButton, SIGNAL( released() ), this, SLOT( clearMemory() ) );
	connect( exit_pushButton, SIGNAL( released() ), this, SLOT( close() ) );
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
	connect( _SSMPdev, SIGNAL( commError() ), this, SLOT( communicationError() ) );
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


bool Engine::probeProtocol()
{
	// Probe SSM1-protocol:
	if (configurePort(1953, 'E'))
	{
		_SSMPdev = new SSMprotocol1(_port, _language);
		if (_SSMPdev->setupCUdata( SSMprotocol::CUtype_Engine ))
			return true;
		delete _SSMPdev;
		// Wait 500ms:
		QEventLoop el;
		QTimer::singleShot(500, &el, SLOT(quit()));
		el.exec();
	}
	// Probe SSM2-protocol:
	if (configurePort(4800, 'N'))
	{
		_SSMPdev = new SSMprotocol2(_port, _language);
		if (_SSMPdev->setupCUdata( SSMprotocol::CUtype_Engine ))
			return true;
		delete _SSMPdev;
	}
	_SSMPdev = NULL;
	return false;
}


bool Engine::configurePort(unsigned int baud, char parity)
{
	serialCOM::dt_portsettings portsettings;
	portsettings.baudrate = static_cast<double>(baud);
	portsettings.databits = 8;
	portsettings.parity = parity;
	portsettings.stopbits = 1;
	if(!_port->SetPortSettings(portsettings))
		return false;
	if(!_port->GetPortSettings(&portsettings))
		return false;
	if ((portsettings.baudrate < (0.97*baud)) || (portsettings.baudrate > (1.03*baud)))
		return false;
	return true;
}


void Engine::DCs()
{
	bool ok = false;
	int DCgroups = 0;
	if (_mode == DCs_mode) return;
	_mode = DCs_mode;
	DCs_pushButton->setChecked(true);
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to Diagnostic Codes... Please wait !   "));
	waitmsgbox.show();
	// Remove old content:
	clearContent();
	// Set title of the content group-box:
	content_groupBox->setTitle(tr("Diagnostic Codes:"));
	// Create, setup and insert content-widget:
	_content_DCs = new CUcontent_DCs_engine(content_groupBox, _progversion);
	content_gridLayout->addWidget(_content_DCs);
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
	measuringblocks_pushButton->setChecked(true);
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to Measuring Blocks... Please wait !   "));
	waitmsgbox.show();
	// Remove old content:
	clearContent();
	// Set title of the content group-box:
	content_groupBox->setTitle(tr("Measuring Blocks:"));
	// Create, setup and insert content-widget:
	_content_MBsSWs = new CUcontent_MBsSWs(content_groupBox, _MBSWsettings);
	content_gridLayout->addWidget(_content_MBsSWs);
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
	adjustments_pushButton->setChecked(true);
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to Adjustment Values... Please wait !   "));
	waitmsgbox.show();
	// Remove old content:
	clearContent();
	// Set title of the content group-box:
	content_groupBox->setTitle(tr("Adjustments:"));
	// Create, setup and insert content-widget:
	_content_Adjustments = new CUcontent_Adjustments(content_groupBox);
	content_gridLayout->addWidget(_content_Adjustments);
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
	systemoperationtests_pushButton->setChecked(true);
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to System Tests... Please wait !   "));
	waitmsgbox.show();
	// Remove old content:
	clearContent();
	// Set title of the content group-box:
	content_groupBox->setTitle(tr("System Operation Tests:"));
	// Create, setup and insert content-widget:
	_content_SysTests = new CUcontent_sysTests(content_groupBox);
	content_gridLayout->addWidget(_content_SysTests);
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


void Engine::communicationError(QString addstr)
{
	// Show error message
	if (addstr.size() > 0) addstr.prepend('\n');
	QMessageBox msg( QMessageBox::Critical, tr("Communication Error"), tr("Communication Error:\n- No or invalid answer from ECU -") + addstr, QMessageBox::Ok, this);
	QFont msgfont = msg.font();
	msgfont.setPixelSize(12);	// 9pts
	msg.setFont( msgfont );
	msg.show();
	msg.exec();
	msg.close();
	// Close engine window (and delete all objects)
	close();
}


void Engine::closeEvent(QCloseEvent *event)
{
	if (_SSMPdev)
	{
		// Create wait message box:
		FSSM_WaitMsgBox waitmsgbox(this, tr("Stopping Communication... Please wait !   "));
		waitmsgbox.show();
		// Stop all permanent communication operations:
		_SSMPdev->stopAllPermanentOperations();
		// Reset CU data:
		_SSMPdev->resetCUdata();
		// Close wait message box:
		waitmsgbox.close();
	}
	event->accept();
}


void Engine::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS OF ui_Engine.h (made with QDesigner)
	QFont appfont = QApplication::font();
	QFont font = this->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	this->setFont(font);
	font = title_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(29);	// 22pts
	title_label->setFont(font);
	font = information_groupBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(15);	// 11-12pts
	information_groupBox->setFont(font);
	font = enginetypetitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	enginetypetitle_label->setFont(font);
	font = enginetype_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	enginetype_label->setFont(font);
	font = romIDtitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	romIDtitle_label->setFont(font);
	font = romID_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	romID_label->setFont(font);
	font = VINtitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	VINtitle_label->setFont(font);
	font = VIN_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	VIN_label->setFont(font);
	font = nrofmbsswstitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	nrofmbsswstitle_label->setFont(font);
	font = nrofdatambstitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	nrofdatambstitle_label->setFont(font);
	font = nrofdatambs_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	nrofdatambs_label->setFont(font);
	font = nrofswitchestitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	nrofswitchestitle_label->setFont(font);
	font = nrofswitches_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	nrofswitches_label->setFont(font);
	font = obd2systemTitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	obd2systemTitle_label->setFont(font);
	font = integCCtitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	integCCtitle_label->setFont(font);
	font = immosupportedtitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	immosupportedtitle_label->setFont(font);
	font = selection_groupBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(15);	// 11-12pts
	selection_groupBox->setFont(font);
	font = DCs_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	DCs_pushButton->setFont(font);
	font = measuringblocks_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	measuringblocks_pushButton->setFont(font);
	font = adjustments_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	adjustments_pushButton->setFont(font);
	font = systemoperationtests_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	systemoperationtests_pushButton->setFont(font);
	font = clearMemory_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	clearMemory_pushButton->setFont(font);
	font = exit_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	exit_pushButton->setFont(font);
	font = content_groupBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(15);	// 11-12pts
	content_groupBox->setFont(font);
}

