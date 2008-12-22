/*
 * Transmission.cpp - Transmission Control Unit dialog
 *
 * Copyright © 2008 Comer352l
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

#include "Transmission.h"


Transmission::Transmission(SSMprotocol *ssmpdev, QString progversion)
{
	// *** Initialize global variables:
	_SSMPdev = ssmpdev;
	_progversion = progversion;
	_content_DCs = NULL;
	_content_MBsSWs = NULL;
	_content_Adjustments = NULL;
	for (int k=0; k<SSMP_MAX_MBSW; k++)
	{
		_lastMBSWmetaList[k].blockType = 0;
		_lastMBSWmetaList[k].nativeIndex = 0;
	}
	_lastMBSWmetaList_len = 0;
	_MBSWtimemode = 0;	// refresh duration [ms]
	_mode = DCs_mode;	// we start in Diagnostic Codes mode
	// *** Setup window/GUI:
	setAttribute (Qt::WA_DeleteOnClose, true);
	// Setup GUI:
	setupUi(this);
	setupUiFonts();
	// Move window to desired coordinates
	move ( 30, 30 );
	// Set window title:
	QString wintitle = "FreeSSM " + progversion + " - " + windowTitle();
	setWindowTitle(wintitle);
	// Load/Show Diagnostic Code content:
	content_groupBox->setTitle(tr("Diagnostic Codes:"));
	DTCs_pushButton->setChecked(true);
	_content_DCs = new CUcontent_DCs_transmission(content_groupBox, _SSMPdev, _progversion);
	content_gridLayout->addWidget(_content_DCs);
	_content_DCs->show();
	// Make GUI visible
	this->show();
	// Connect to Control Unit, get data and setup GUI:
	setup();
}



Transmission::~Transmission()
{
	disconnect(_SSMPdev, SIGNAL( commError() ), this, SLOT( communicationError() ));
	disconnect( DTCs_pushButton, SIGNAL( pressed() ), this, SLOT( DTCs() ) );
	disconnect( measuringblocks_pushButton, SIGNAL( pressed() ), this, SLOT( measuringblocks() ) );
	disconnect( adjustments_pushButton, SIGNAL( pressed() ), this, SLOT( adjustments() ) );
	disconnect( clearMemory_pushButton, SIGNAL( pressed() ), this, SLOT( clearMemory() ) );
	disconnect( clearMemory2_pushButton, SIGNAL( pressed() ), this, SLOT( clearMemory2() ) );
	disconnect( exit_pushButton, SIGNAL( pressed() ), this, SLOT( close() ) );
	clearContent();
}



void Transmission::setup()
{
	// *** Local variables:
	QString sysdescription = "";
	QString ROM_ID = "";
	bool supported = false;
	std::vector<mbsw_dt> supportedMBsSWs;
	int supDCgroups = 0;
	QPixmap sup_pixmap(QString::fromUtf8(":/icons/chrystal/22x22/ok.png"));
	QPixmap nsup_pixmap(QString::fromUtf8(":/icons/chrystal/22x22/editdelete.png"));
	// ***** Connect to Control Unit *****:
	// Create Status information message box for CU initialisation/setup:
	FSSM_InitStatusMsgBox initstatusmsgbox(tr("Connecting to TCU... Please wait !"), 0, 0, 100, this);
	initstatusmsgbox.setWindowTitle(tr("Connecting to TCU..."));
	initstatusmsgbox.setValue(5);
	initstatusmsgbox.show();
	// Try to establish CU connection:
	if( _SSMPdev->setupCUdata() )
	{
		// Update status info message box:
		initstatusmsgbox.setLabelText(tr("Processing TCU data... Please wait !"));
		initstatusmsgbox.setValue(40);
		// Query system description:
		if (!_SSMPdev->getSystemDescription(&sysdescription))
		{
			QString SYS_ID = "";
			if (!_SSMPdev->getSysID(&SYS_ID))
				goto commError;
			sysdescription = tr("unknown (") + SYS_ID + ")";
		}
		// Output system description:
		transmissiontype_label->setText(sysdescription);
		// Query ROM-ID:
		if (!_SSMPdev->getROMID(&ROM_ID))
			goto commError;
		// Output ROM-ID:
		romID_label->setText(ROM_ID);
		// Number of supported MBs / SWs:
		if (!_SSMPdev->getSupportedMBs(&supportedMBsSWs))
			goto commError;
		nrofdatambs_label->setText( QString::number(supportedMBsSWs.size(), 10) );
		if (!_SSMPdev->getSupportedSWs(&supportedMBsSWs))
			goto commError;
		nrofswitches_label->setText( QString::number(supportedMBsSWs.size(), 10) );
		// OBD2-Support:
		if (!_SSMPdev->hasOBD2(&supported))
			goto commError;
		if (supported)
			obd2compliant_label->setPixmap(sup_pixmap);
		else
			obd2compliant_label->setPixmap(nsup_pixmap);
		// "Clear Memory 2"-support:
		if (!_SSMPdev->hasClearMemory2(&supported))
			goto commError;
		if (supported)
			clearMemory2_pushButton->setEnabled(true);
		else
			clearMemory2_pushButton->setEnabled(false);
	}
	else // CU-connection could not be established
		goto commError;
	// Connect signals/slots:
	connect( DTCs_pushButton, SIGNAL( pressed() ), this, SLOT( DTCs() ) );
	connect( measuringblocks_pushButton, SIGNAL( pressed() ), this, SLOT( measuringblocks() ) );
	connect( adjustments_pushButton, SIGNAL( pressed() ), this, SLOT( adjustments() ) );
	connect( clearMemory_pushButton, SIGNAL( pressed() ), this, SLOT( clearMemory() ) );
	connect( clearMemory2_pushButton, SIGNAL( pressed() ), this, SLOT( clearMemory2() ) );
	connect( exit_pushButton, SIGNAL( pressed() ), this, SLOT( close() ) );
	connect(_SSMPdev, SIGNAL( commError() ), this, SLOT( communicationError() ));
	// Start Diagnostic Codes reading:
	if (!_content_DCs->setup())
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
	initstatusmsgbox.setLabelText(tr("TCU-Initialisation successful !"));
	initstatusmsgbox.setValue(100);
	QTimer::singleShot(800, &initstatusmsgbox, SLOT(accept()));
	initstatusmsgbox.exec();
	initstatusmsgbox.close();
	return;

commError:
	initstatusmsgbox.close();
	communicationError();
}



void Transmission::DTCs()
{
	bool ok = false;
	int DCgroups = 0;
	if (_mode == DCs_mode) return;
	_mode = DCs_mode;
	DTCs_pushButton->setChecked(true);
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to Diagnostic Codes... Please wait !   "));
	waitmsgbox.show();
	// Remove old content:
	clearContent();
	// Set title of the content group-box:
	content_groupBox->setTitle(tr("Diagnostic Codes:"));
	// Create, setup and insert content-widget:
	_content_DCs = new CUcontent_DCs_transmission(content_groupBox, _SSMPdev, _progversion);
	content_gridLayout->addWidget(_content_DCs);
	_content_DCs->show();
	ok = _content_DCs->setup();
	// Start DC-reading:
	if (ok)
	{
		ok = _SSMPdev->getSupportedDCgroups(&DCgroups);
		if (ok && DCgroups != SSMprotocol::noDCs_DCgroup)
			ok = _content_DCs->startDCreading();
	}
	// Get notification, if internal error occures:
	if (ok)
		connect(_content_DCs, SIGNAL( error() ), this, SLOT( close() ) );
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (!ok)
		communicationError();
}



void Transmission::measuringblocks()
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
	_content_MBsSWs = new CUcontent_MBsSWs(content_groupBox, _SSMPdev, _MBSWtimemode);
	content_gridLayout->addWidget(_content_MBsSWs);
	_content_MBsSWs->show();
	ok = _content_MBsSWs->setup();
	if (ok)
		ok = _content_MBsSWs->setMBSWselection(_lastMBSWmetaList, _lastMBSWmetaList_len);
	// Get notification, if internal error occures:
	if (ok)
		connect(_content_MBsSWs, SIGNAL( error() ), this, SLOT( close() ) );
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (!ok)
		communicationError();
}



void Transmission::adjustments()
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
	_content_Adjustments = new CUcontent_Adjustments(content_groupBox, _SSMPdev);
	content_gridLayout->addWidget(_content_Adjustments);
	_content_Adjustments->show();
	ok = _content_Adjustments->setup();
	if (ok)
		connect(_content_Adjustments, SIGNAL( communicationError() ), this, SLOT( close() ) );
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (!ok)
		communicationError();
}



void Transmission::clearMemory()
{
	runClearMemory(SSMprotocol::CMlevel_1);
}



void Transmission::clearMemory2()
{
	runClearMemory(SSMprotocol::CMlevel_2);
}



void Transmission::runClearMemory(SSMprotocol::CMlevel_dt level)
{
	bool ok = false;
	ClearMemoryDlg::CMresult_dt result;
	// Create "Clear Memory"-dialog:
	ClearMemoryDlg cmdlg(this, _SSMPdev, level);
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
		ok = _content_Adjustments->setup(); // refresh adjustment values
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



void Transmission::clearContent()
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
		_content_MBsSWs->getCurrentMBSWselection(_lastMBSWmetaList, &_lastMBSWmetaList_len);
		_content_MBsSWs->getCurrentTimeMode(&_MBSWtimemode);
		delete _content_MBsSWs;
		_content_MBsSWs = NULL;
	}
	if (_content_Adjustments != NULL)
	{
		disconnect(_content_Adjustments, SIGNAL( communicationError() ), this, SLOT( close() ) );
		delete _content_Adjustments;
		_content_Adjustments = NULL;
	}
}



void Transmission::communicationError(QString addstr)
{
	// Show error message
	if (addstr.size() > 0) addstr.prepend('\n');
	QMessageBox msg( QMessageBox::Critical, tr("Communication Error"), tr("Communication Error:\n- No or invalid answer from TCU -") + addstr, QMessageBox::Ok, this);
	QFont msgfont = msg.font();
	msgfont.setPixelSize(12);	// 9pts
	msg.setFont( msgfont );
	msg.show();
	msg.exec();
	msg.close();
	// Close transmission window (and delete all objects)
	close();
}



void Transmission::closeEvent(QCloseEvent *event)
{
	// Create wait message box:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Stopping Communication... Please wait !   "));
	waitmsgbox.show();
	// Stop all permanent communication operations:
	_SSMPdev->stopAllPermanentOperations();
	// Reset CU data:
	_SSMPdev->resetCUdata();
	// NOTE: we got _SSMPdev already initialized, so we do NOT delete it here !
	event->accept();
	// Close wait message box:
	waitmsgbox.close();
	// Window will now be closed and delete is called...
}



void Transmission::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS OF ui_FreeSSM.h (made with QDesigner)
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
	font = transmissiontypetitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	transmissiontypetitle_label->setFont(font);
	font = transmissiontype_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	transmissiontype_label->setFont(font);
	font = romIDtitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	romIDtitle_label->setFont(font);
	font = romID_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	romID_label->setFont(font);
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
	font = OBDsupportedtitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	OBDsupportedtitle_label->setFont(font);
	font = selection_groupBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(15);	// 11-12pts
	selection_groupBox->setFont(font);
	font = DTCs_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	DTCs_pushButton->setFont(font);
	font = measuringblocks_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	measuringblocks_pushButton->setFont(font);
	font = adjustments_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	adjustments_pushButton->setFont(font);
	font = clearMemory_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	clearMemory_pushButton->setFont(font);
	font = clearMemory2_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	clearMemory2_pushButton->setFont(font);
	font = exit_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	exit_pushButton->setFont(font);
	font = content_groupBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(15);	// 11-12pts
	content_groupBox->setFont(font);
}


