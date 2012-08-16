/*
 * AirConDialog.cpp - Air Conditioning Control Unit dialog
 * 
 * Copyright (C) 2012 L1800Turbo, 2008-2012 Comer352L
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

#include "AirConDialog.h"


AirConDialog::AirConDialog(AbstractDiagInterface *diagInterface, QString language) : ControlUnitDialog(tr("Auto Air Conditioning Control Unit"), diagInterface, language)
{
	// *** Initialize global variables:
	_content_DCs = NULL;
	_content_MBsSWs = NULL;
	_mode = DCs_mode;	// we start in Diagnostic Codes mode
	// Show information-widget:
	_infoWidget = new CUinfo_simple("Air Conditioning");
	setInfoWidget(_infoWidget);
	_infoWidget->show();
	// Setup functions:
	QPushButton *pushButton = addFunction(tr("&Diagnostic Codes"), QIcon(QString::fromUtf8(":/icons/chrystal/22x22/messagebox_warning.png")), true);
	pushButton->setChecked(true);
	connect( pushButton, SIGNAL( clicked() ), this, SLOT( DTCs() ) );
	pushButton = addFunction(tr("&Measuring Blocks"), QIcon(QString::fromUtf8(":/icons/oxygen/22x22/applications-utilities.png")), true);
	connect( pushButton, SIGNAL( clicked() ), this, SLOT( measuringblocks() ) );
	_clearMemory_pushButton = addFunction(tr("Clear Memory"), QIcon(QString::fromUtf8(":/icons/chrystal/22x22/eraser.png")), false);
	connect( _clearMemory_pushButton, SIGNAL( clicked() ), this, SLOT( clearMemory() ) );
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
	// Load/Show Diagnostic Code content:
	_content_DCs = new CUcontent_DCs_twoMemories();
	setContentWidget(tr("Diagnostic Codes:"), _content_DCs);
	_content_DCs->show();
	// Make GUI visible
	this->show();
	// Connect to Control Unit, get data and setup GUI:
	setup();
}


void AirConDialog::setup()
{
	// *** Local variables:
	QString sysdescription = "";
	std::string ROM_ID = "";
	bool supported = false;
	std::vector<mb_dt> supportedMBs;
	std::vector<sw_dt> supportedSWs;
	int supDCgroups = 0;
	// ***** Connect to Control Unit *****:
	// Create Status information message box for CU initialisation/setup:
	FSSM_InitStatusMsgBox initstatusmsgbox(tr("Connecting to Auto Air Conditioning Control Unit... Please wait !"), 0, 0, 100, this);
	initstatusmsgbox.setWindowTitle(tr("Connecting..."));
	initstatusmsgbox.setValue(5);
	initstatusmsgbox.show();
	// Try to establish CU connection:
	SSMprotocol::CUsetupResult_dt init_result = probeProtocol(SSMprotocol::CUtype_AirCon);
	if ((init_result == SSMprotocol::result_success) || (init_result == SSMprotocol::result_noOrInvalidDefsFile) || (init_result == SSMprotocol::result_noDefs))
	{
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
			std::string SYS_ID = _SSMPdev->getSysID();
			if (!SYS_ID.length())
				goto commError;
			sysdescription = tr("unknown");
			if (SYS_ID != ROM_ID)
				sysdescription += " (" + QString::fromStdString(SYS_ID) + ")";
		}
		// Output system description:
		_infoWidget->setSystemTypeText(sysdescription);
		// Output ROM-ID:
		_infoWidget->setRomIDText( QString::fromStdString(ROM_ID) );
		if (init_result == SSMprotocol::result_success)
		{
			// Number of supported MBs / SWs:
			if ((!_SSMPdev->getSupportedMBs(&supportedMBs)) || (!_SSMPdev->getSupportedSWs(&supportedSWs)))
				goto commError;
			_infoWidget->setNrOfSupportedMBsSWs(supportedMBs.size(), supportedSWs.size());
			// "Clear Memory"-support:
			if (!_SSMPdev->hasClearMemory(&supported))
				goto commError;
			_clearMemory_pushButton->setEnabled(supported);
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
			initstatusmsgbox.setLabelText(tr("Control Unit initialisation successful !"));
			initstatusmsgbox.setValue(100);
			QTimer::singleShot(800, &initstatusmsgbox, SLOT(accept()));
			initstatusmsgbox.exec();
			initstatusmsgbox.close();
		}
		else
		{
			// "Clear Memory"-support:
			_clearMemory_pushButton->setEnabled(false);
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
				errtext = tr("Error:\nThis control unit is not yet supported by FreeSSM.\nFreeSSM can communiate with the control unit, but it doesn't have the necessary data to provide diagnostic operations.\nIf you want to contribute to the the project (help adding defintions), feel free to contact the authors.");
			}
			QMessageBox msg( QMessageBox::Critical, tr("Error"), errtext, QMessageBox::Ok, this);
			QFont msgfont = msg.font();
			msgfont.setPixelSize(12);	// 9pts
			msg.setFont( msgfont );
			msg.show();
			msg.exec();
			msg.close();
			// Exit CU dialog:
			close();
		}
	}
	else // All other errors
		goto commError;
	return;

commError:
	initstatusmsgbox.close();
	communicationError();
}


void AirConDialog::DTCs()
{
	bool ok = false;
	int DCgroups = 0;
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
	ok = _content_DCs->setup(_SSMPdev);
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
	// Save new mode:
	_mode = DCs_mode;
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (!ok)
		communicationError();
}


void AirConDialog::measuringblocks()
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
	ok = _content_MBsSWs->setup(_SSMPdev);
	if (ok)
		ok = _content_MBsSWs->setMBSWselection(_lastMBSWmetaList);
	// Get notification, if internal error occures:
	if (ok)
		connect(_content_MBsSWs, SIGNAL( error() ), this, SLOT( close() ) );
	// Save new mode:
	_mode = MBsSWs_mode;
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (!ok)
		communicationError();
}


void AirConDialog::clearMemory()
{
	runClearMemory(SSMprotocol::CMlevel_1);
}


void AirConDialog::runClearMemory(SSMprotocol::CMlevel_dt level)
{
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
	if (result == ClearMemoryDlg::CMresult_communicationError)
	{
		communicationError();
	}
	else if ((result == ClearMemoryDlg::CMresult_reconnectAborted) || (result == ClearMemoryDlg::CMresult_reconnectFailed))
	{
		close(); // exit engine control unit dialog
	}
}


void AirConDialog::saveContentSettings()
{
	if (_mode == MBsSWs_mode)
	{
		_content_MBsSWs->getMBSWselection(&_lastMBSWmetaList);
		_content_MBsSWs->getSettings(&_MBSWsettings);
	}
}

