/*
 * ABSdialog.cpp - ABS/VDC Control Unit dialog
 *
 * Copyright (C) 2012 L1800Turbo, 2008-2019 Comer352L
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

 /*
  * TODO:
  *  FreezeFrame
  */

#include "ABSdialog.h"
#include "CmdLine.h"
#include "CUcontent_DCs_twoMemories.h"


ABSdialog::ABSdialog(AbstractDiagInterface *diagInterface, QString language) : ControlUnitDialog(tr("ABS/VDC Control Unit"), diagInterface, language)
{
	// Show information-widget:
	_infoWidget = new CUinfo_simple();
	setInfoWidget(_infoWidget);
	_infoWidget->show();
	// Setup functions:
	QPushButton *pushButton = addContent(ContentSelection::DCsMode);
	connect( pushButton, SIGNAL( clicked() ), this, SLOT( switchToDCsMode() ) );
	pushButton = addContent(ContentSelection::MBsSWsMode);
	connect( pushButton, SIGNAL( clicked() ), this, SLOT( switchToMBsSWsMode() ) );
	pushButton = addContent(ContentSelection::ClearMemoryFcn);
	connect( pushButton, SIGNAL( clicked() ), this, SLOT( clearMemory() ) );
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
}


bool ABSdialog::setup(ContentSelection csel, QStringList cmdline_args)
{
	// *** Local variables:
	QString sysdescription = "";
	std::string SYS_ID = "";
	std::string ROM_ID = "";
	QString mbssws_selfile = "";
	bool autostart = false;

	if (_setup_done)
		return false;
	// Get command line startup parameters (if available):
	if (!getParametersFromCmdLine(&cmdline_args, &mbssws_selfile, &autostart))
		exit(ERROR_BADCMDLINEARGS);
	// ***** Create, setup and insert the content-widget *****:
	if ((csel == ContentSelection::DCsMode) || (csel == ContentSelection::ClearMemoryFcn))
	{
		setContentSelectionButtonChecked(ContentSelection::DCsMode, true);
		_content_DCs = new CUcontent_DCs_twoMemories();
		setContentWidget(tr("Diagnostic Codes:"), _content_DCs);
		_content_DCs->show();
	}
	else if (csel == ContentSelection::MBsSWsMode)
	{
		setContentSelectionButtonChecked(ContentSelection::MBsSWsMode, true);
		_content_MBsSWs = new CUcontent_MBsSWs(_MBSWsettings);
		setContentWidget(tr("Measuring Blocks:"), _content_MBsSWs);
		_content_MBsSWs->show();
	}
	else	// NOTE: currently only possible due to wrong command line parameters
	{
		CmdLine::printError("the specified function is not supported by the ABS/VDC Control Unit.");
		exit(ERROR_BADCMDLINEARGS);
	}
	// ***** Connect to Control Unit *****:
	// Create Status information message box for CU initialisation/setup:
	FSSM_InitStatusMsgBox initstatusmsgbox(tr("Connecting to ABS/VDC Control Unit... Please wait !"), 0, 0, 100, this);
	initstatusmsgbox.setWindowTitle(tr("Connecting..."));
	initstatusmsgbox.setValue(5);
	initstatusmsgbox.show();
	// Try to establish CU connection:
	SSMprotocol::CUsetupResult_dt init_result = probeProtocol(SSMprotocol::CUtype_ABS);
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
	_infoWidget->setSystemTypeText(sysdescription);
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
		// "Clear Memory"-support:
		if (!_SSMPdev->hasClearMemory(&supported))
			goto commError;
		setContentSelectionButtonEnabled(ContentSelection::ClearMemoryFcn, supported);
		// Enable mode buttons:
		// NOTE: unconditionally, contents are deactivated if unsupported
		setContentSelectionButtonEnabled(ContentSelection::DCsMode, true);
		setContentSelectionButtonEnabled(ContentSelection::MBsSWsMode, true);
		// Start selected mode:
		bool ok = false;
		if ((csel == ContentSelection::DCsMode) || (csel == ContentSelection::ClearMemoryFcn))
		{
			ok = startDCsMode();
		}
		else if (csel == ContentSelection::MBsSWsMode)
		{
			ok = startMBsSWsMode();
		}
		// else BUG
		if (!ok)
			goto commError;
		// Update and close status info:
		initstatusmsgbox.setLabelText(tr("Control Unit initialisation successful !"));
		initstatusmsgbox.setValue(100);
		QTimer::singleShot(800, &initstatusmsgbox, SLOT(accept()));
		initstatusmsgbox.exec();
		initstatusmsgbox.close();
		// Run Clear Memory procedure if requested:
		if (csel == ContentSelection::ClearMemoryFcn)
			clearMemory();
		// Apply command line startup parameters for MB/SW mode:
		else if (csel == ContentSelection::MBsSWsMode)
		{
			if (((supportedMBs.size() + supportedSWs.size()) > 0) && (_content_MBsSWs != NULL))
			{
				if (mbssws_selfile.size())
					_content_MBsSWs->loadMBsSWs(mbssws_selfile);
				if (_content_MBsSWs->numMBsSWsSelected() && autostart)
					_content_MBsSWs->startMBSWreading();
			}
		}
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
			errtext = tr("Error:\nThis control unit is not yet supported by FreeSSM.\nFreeSSM can communicate with the control unit, but it doesn't have the necessary data to provide diagnostic operations.\nIf you want to contribute to the project (help adding defintions), feel free to contact the authors.");
		}
		QMessageBox msg( QMessageBox::Critical, tr("Error"), errtext, QMessageBox::Ok, this);
		QFont msgfont = msg.font();
		msgfont.setPointSize(9);
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


void ABSdialog::switchToDCsMode()
{
	bool ok = false;
	if (_mode == Mode::DCs) return;
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


void ABSdialog::switchToMBsSWsMode()
{
	bool ok = false;
	if (_mode == Mode::MBsSWs) return;
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

