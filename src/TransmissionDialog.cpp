/*
 * Transmission.cpp - Transmission Control Unit dialog
 *
 * Copyright (C) 2008-2018 Comer352L
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

#include "TransmissionDialog.h"


TransmissionDialog::TransmissionDialog(AbstractDiagInterface *diagInterface, QString language) : ControlUnitDialog(tr("Transmission Control Unit"), diagInterface, language)
{
	// *** Initialize global variables:
	_content_DCs = NULL;
	_content_MBsSWs = NULL;
	_content_Adjustments = NULL;
	// Show information-widget:
	_infoWidget = new CUinfo_Transmission();
	setInfoWidget(_infoWidget);
	_infoWidget->show();
	// Setup functions:
	QPushButton *pushButton = addFunction(tr("&Diagnostic Codes"), QIcon(QString::fromUtf8(":/icons/chrystal/22x22/messagebox_warning.png")), true);
	connect( pushButton, SIGNAL( clicked() ), this, SLOT( switchToDCsMode() ) );
	pushButton = addFunction(tr("&Measuring Blocks"), QIcon(QString::fromUtf8(":/icons/oxygen/22x22/applications-utilities.png")), true);
	connect( pushButton, SIGNAL( clicked() ), this, SLOT( switchToMBsSWsMode() ) );
	pushButton = addFunction(tr("&Adjustments"), QIcon(QString::fromUtf8(":/icons/chrystal/22x22/configure.png")), true);
	connect( pushButton, SIGNAL( clicked() ), this, SLOT( switchToAdjustmentsMode() ) );
	_clearMemory_pushButton = addFunction(tr("Clear Memory"), QIcon(QString::fromUtf8(":/icons/chrystal/22x22/eraser.png")), false);
	connect( _clearMemory_pushButton, SIGNAL( clicked() ), this, SLOT( clearMemory() ) );
	_clearMemory2_pushButton = addFunction(tr("Clear Memory 2"), QIcon(QString::fromUtf8(":/icons/chrystal/22x22/eraser.png")), false);
	connect( _clearMemory2_pushButton, SIGNAL( clicked() ), this, SLOT( clearMemory2() ) );
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
}


bool TransmissionDialog::setup(enum mode_dt mode)
{
	// *** Local variables:
	QString sysdescription = "";
	std::string SYS_ID = "";
	std::string ROM_ID = "";

	if (_setup_done)
		return true;
	// ***** Create, setup and insert the content-widget *****:
	if (mode == DCs_mode)
	{
		_selButtons.at(0)->setChecked(true);
		_content_DCs = new CUcontent_DCs_twoMemories();
		setContentWidget(tr("Diagnostic Codes:"), _content_DCs);
		_content_DCs->show();
	}
	else if (mode == MBsSWs_mode)
	{
		_selButtons.at(1)->setChecked(true);
		_content_MBsSWs = new CUcontent_MBsSWs(_MBSWsettings);
		setContentWidget(tr("Measuring Blocks:"), _content_MBsSWs);
		_content_MBsSWs->show();
	}
	else if (mode == Adjustments_mode)
	{
		_selButtons.at(2)->setChecked(true);
		_content_Adjustments = new CUcontent_Adjustments();
		setContentWidget(tr("Adjustments:"), _content_Adjustments);
		_content_Adjustments->show();
	}
	else
		return false;
	// ***** Connect to Control Unit *****:
	// Create Status information message box for CU initialisation/setup:
	FSSM_InitStatusMsgBox initstatusmsgbox(tr("Connecting to Transmission Control Unit... Please wait !"), 0, 0, 100, this);
	initstatusmsgbox.setWindowTitle(tr("Connecting..."));
	initstatusmsgbox.setValue(5);
	initstatusmsgbox.show();
	// Try to establish CU connection:
	SSMprotocol::CUsetupResult_dt init_result = probeProtocol(SSMprotocol::CUtype_Transmission);
	if ((init_result != SSMprotocol::result_success) && (init_result != SSMprotocol::result_noOrInvalidDefsFile) && (init_result != SSMprotocol::result_noDefs))
		goto commError;
	// Update status info message box:
	initstatusmsgbox.setLabelText(tr("Processing Control Unit data... Please wait !"));
	initstatusmsgbox.setValue(40);
	// Query ROM-ID:
	ROM_ID = _SSMPdev->getROMID();
	if (!ROM_ID.length())
		goto commError;
	// Query system description:
	if (!_SSMPdev->getSystemDescription(&sysdescription))
	{
		SYS_ID = _SSMPdev->getSysID();
		if (!SYS_ID.length())
			goto commError;
		sysdescription = tr("unknown");
		if (SYS_ID != ROM_ID)
			sysdescription += " (" + QString::fromStdString(SYS_ID) + ")";
	}
	// Output system description:
	_infoWidget->setTransmissionTypeText(sysdescription);
	// Output ROM-ID:
	_infoWidget->setRomIDText( QString::fromStdString(ROM_ID) );
	if (init_result == SSMprotocol::result_success)
	{
		bool supported = false;
		std::vector<mb_dt> supportedMBs;
		std::vector<sw_dt> supportedSWs;
		// Number of supported MBs / SWs:
		if ((!_SSMPdev->getSupportedMBs(&supportedMBs)) || (!_SSMPdev->getSupportedSWs(&supportedSWs)))
			goto commError;
		_infoWidget->setNrOfSupportedMBsSWs(supportedMBs.size(), supportedSWs.size());
		// OBD2-Support:
		if (!_SSMPdev->hasOBD2system(&supported))
			goto commError;
		_infoWidget->setOBD2Supported(supported);
		// "Clear Memory"-support:
		if (!_SSMPdev->hasClearMemory(&supported))
			goto commError;
		_clearMemory_pushButton->setEnabled(supported);
		// "Clear Memory 2"-support:
		if (!_SSMPdev->hasClearMemory2(&supported))
			goto commError;
		_clearMemory2_pushButton->setEnabled(supported);
		// Enable mode buttons:
		// NOTE: unconditionally, contents are deactivated if unsupported
		_selButtons.at(0)->setEnabled(true);
		_selButtons.at(1)->setEnabled(true);
		_selButtons.at(2)->setEnabled(true);
		// Start selected mode:
		bool ok = false;
		if (mode == DCs_mode)
		{
			ok = startDCsMode();
		}
		else if (mode == MBsSWs_mode)
		{
			ok = startMBsSWsMode();
		}
		else if (mode == Adjustments_mode)
		{
			ok = startAdjustmentsMode();
		}
		// else: BUG
		if (!ok)
			goto commError;
		// Update and close status info:
		initstatusmsgbox.setLabelText(tr("Control Unit initialisation successful !"));
		initstatusmsgbox.setValue(100);
		QTimer::singleShot(800, &initstatusmsgbox, SLOT(accept()));
		initstatusmsgbox.exec();
		initstatusmsgbox.close();
	}
	else
	{
		// Close progress dialog:
		initstatusmsgbox.close();
		// Show error message:
		QString errtext;
		if (init_result == SSMprotocol::result_noOrInvalidDefsFile)
		{
			errtext = tr("Error:\nNo valid definitions file found.\nPlease make sure that FreeSSM is installed properly.");
		}
		else if (init_result == SSMprotocol::result_noDefs)
		{
			errtext = tr("Error:\nThis control unit is not yet supported by FreeSSM.\nFreeSSM can communicate with the control unit, but it doesn't have the necessary data to provide diagnostic operations.\nIf you want to contribute to the the project (help adding defintions), feel free to contact the authors.");
		}
		QMessageBox msg( QMessageBox::Critical, tr("Error"), errtext, QMessageBox::Ok, this);
		QFont msgfont = msg.font();
		msgfont.setPointSize(9);	// 9pts
		msg.setFont( msgfont );
		msg.show();
		msg.exec();
		msg.close();
		// Exit CU dialog:
		close();
		return false;
	}
	_setup_done = true;
	return true;

commError:
	initstatusmsgbox.close();
	communicationError();
	return false;
}


void TransmissionDialog::switchToDCsMode()
{
	bool ok = false;
	if (_mode == DCs_mode) return;
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to Diagnostic Codes... Please wait !"));
	waitmsgbox.show();
	// Save content settings:
	saveContentSettings();
	// Create, setup and insert new content-widget:
	_content_DCs = new CUcontent_DCs_twoMemories();
	setContentWidget(tr("Diagnostic Codes:"), _content_DCs);
	_content_DCs->show();
	// Start DCs mode:
	ok = startDCsMode();
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (!ok)
		communicationError();
}


void TransmissionDialog::switchToMBsSWsMode()
{
	bool ok = false;
	if (_mode == MBsSWs_mode) return;
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to Measuring Blocks... Please wait !"));
	waitmsgbox.show();
	// Save content settings:
	saveContentSettings();
	// Create, setup and insert new content-widget:
	_content_MBsSWs = new CUcontent_MBsSWs(_MBSWsettings);
	setContentWidget(tr("Measuring Blocks:"), _content_MBsSWs);
	_content_MBsSWs->show();
	// Start MB/SW mode:
	ok = startMBsSWsMode();
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (!ok)
		communicationError();
}


void TransmissionDialog::switchToAdjustmentsMode()
{
	bool ok = false;
	if (_mode == Adjustments_mode) return;
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to Adjustment Values... Please wait !"));
	waitmsgbox.show();
	// Save content settings:
	saveContentSettings();
	// Create, setup and insert new content-widget:
	_content_Adjustments = new CUcontent_Adjustments();
	setContentWidget(tr("Adjustments:"), _content_Adjustments);
	_content_Adjustments->show();
	// Start Adjustments mode:
	ok = startAdjustmentsMode();
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (!ok)
		communicationError();
}


void TransmissionDialog::clearMemory()
{
	runClearMemory(SSMprotocol::CMlevel_1);
}


void TransmissionDialog::clearMemory2()
{
	runClearMemory(SSMprotocol::CMlevel_2);
}


bool TransmissionDialog::startDCsMode()
{
	int DCgroups = 0;
	if (_content_DCs == NULL)
		return false;
	if (!_content_DCs->setup(_SSMPdev))
		return false;
	if (!_SSMPdev->getSupportedDCgroups(&DCgroups))
		return false;
	if (DCgroups == SSMprotocol::noDCs_DCgroup)
		return false;
	if (!_content_DCs->startDCreading())
		return false;
	connect(_content_DCs, SIGNAL( error() ), this, SLOT( close() ) );
	_mode = DCs_mode;
	return true;
}


bool TransmissionDialog::startMBsSWsMode()
{
	if (_content_MBsSWs == NULL)
		return false;
	if (!_content_MBsSWs->setup(_SSMPdev))
		return false;
	if (!_content_MBsSWs->setMBSWselection(_lastMBSWmetaList))
		return false;
	connect(_content_MBsSWs, SIGNAL( error() ), this, SLOT( close() ) );
	_mode = MBsSWs_mode;
	return true;
}


bool TransmissionDialog::startAdjustmentsMode()
{
	if (_content_Adjustments == NULL)
		return false;
	if (!_content_Adjustments->setup(_SSMPdev))
		return false;
	connect(_content_Adjustments, SIGNAL( communicationError() ), this, SLOT( close() ) );
	_mode = Adjustments_mode;
	return true;
}


void TransmissionDialog::runClearMemory(SSMprotocol::CMlevel_dt level)
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
	if ((result == ClearMemoryDlg::CMresult_success) && (_mode == Adjustments_mode))
	{
		FSSM_WaitMsgBox waitmsgbox(this, tr("Reading Adjustment Values... Please wait !"));
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
		close(); // exit control unit dialog
	}
}


void TransmissionDialog::saveContentSettings()
{
	if (_mode == MBsSWs_mode)
	{
		_lastMBSWmetaList = _content_MBsSWs->getMBSWselection();
		_MBSWsettings = _content_MBsSWs->getSettings();
	}
}

