/*
 * CruiseControlDialog.cpp - Cruise Control Unit dialog
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

#include "CruiseControlDialog.h"
#include "CmdLine.h"
#include "CUcontent_DCs_stopCodes.h"


CruiseControlDialog::CruiseControlDialog(AbstractDiagInterface *diagInterface, QString language) : ControlUnitDialog(tr("Cruise Control Unit"), diagInterface, language)
{
	// Show information-widget:
	_infoWidget = new CUinfo_simple();
	setInfoWidget(_infoWidget);
	_infoWidget->show();
	// Setup functions:
	addContent(ContentSelection::DCsMode);
	addContent(ContentSelection::MBsSWsMode);
}


QString CruiseControlDialog::systemName()
{
	return tr("Cruise Control");
}


QString CruiseControlDialog::controlUnitName()
{
	return tr("Cruise Control Control Unit");
}


bool CruiseControlDialog::setup(ContentSelection csel, QStringList cmdline_args)
{
	Mode mode;
	QString sysdescription = "";
	std::string SYS_ID = "";
	std::string ROM_ID = "";
	QString mbssws_selfile = "";
	bool autostart = false;
	int ret;

	if (_setup_done)
		return false;
	if (!contentSupported(csel))
	{
		CmdLine::printError("the specified function is not supported by the Cruise Control Control Unit.");
		exit(ERROR_BADCMDLINEARGS);
	}
	if (!getModeForContentSelection(csel, &mode))
		return false;
	// Get command line startup parameters (if available):
	if (!getParametersFromCmdLine(&cmdline_args, &mbssws_selfile, &autostart))
		exit(ERROR_BADCMDLINEARGS);
	// ***** Create and insert the content widget *****:
	if (!prepareContentWidget(mode))
		return false;
	// ***** Connect to Control Unit *****:
	// Inform user that system needs to be switched on manually:
	QMessageBox *msgbox = new QMessageBox(QMessageBox::Information, tr("Prepare system"), QString(tr("Please switch the %1 system on.").arg(systemName())), 0, this);
	QPushButton *button = msgbox->addButton(QMessageBox::Ok);
	button->setText(tr("Continue"));
	msgbox->addButton(QMessageBox::Cancel);
	QFont msgfont = msgbox->font();
	msgfont.setPointSize(9);
	msgbox->setFont( msgfont );
	msgbox->show();
	ret = msgbox->exec();
	delete msgbox;
	if (ret != QMessageBox::Ok)
	{
		close();
		return false;
	}
	// Create Status information message box for CU initialisation/setup:
	FSSM_InitStatusMsgBox initstatusmsgbox(tr("Connecting to %1... Please wait !").arg(controlUnitName()), 0, 0, 100, this);
	initstatusmsgbox.setWindowTitle(tr("Connecting..."));
	initstatusmsgbox.setValue(5);
	initstatusmsgbox.show();
	// Try to establish CU connection:
	SSMprotocol::CUsetupResult_dt init_result = probeProtocol(SSMprotocol::CUtype_CruiseControl);
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
		// Fill info widget with information about the Control Unit:
		if (!fillInfoWidget(&initstatusmsgbox))
			goto commError;
		// Enable mode buttons:
		// NOTE: unconditionally, contents are deactivated if unsupported
		setContentSelectionButtonEnabled(ContentSelection::DCsMode, true);
		setContentSelectionButtonEnabled(ContentSelection::MBsSWsMode, true);
		// Start selected mode:
		if (!startMode(mode))
			goto commError;
		// Update and close status info:
		initstatusmsgbox.setLabelText(tr("Control Unit initialisation successful !"));
		initstatusmsgbox.setValue(100);
		QTimer::singleShot(800, &initstatusmsgbox, SLOT(accept()));
		initstatusmsgbox.exec();
		initstatusmsgbox.close();
		// Apply command line startup parameters for MB/SW mode:
		if ((csel == ContentSelection::MBsSWsMode) && (_content_MBsSWs != NULL))
		{
			if (mbssws_selfile.size())
				_content_MBsSWs->loadMBsSWs(mbssws_selfile);
			if (_content_MBsSWs->numMBsSWsSelected() && autostart)
				_content_MBsSWs->startMBSWreading();
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


CUcontent_DCs_abstract * CruiseControlDialog::allocate_DCsContentWidget()
{
	return new CUcontent_DCs_stopCodes();
}


bool CruiseControlDialog::fillInfoWidget(FSSM_InitStatusMsgBox*)
{
	std::vector<mb_dt> supportedMBs;
	std::vector<sw_dt> supportedSWs;
	// Number of supported MBs / SWs:
	if ((!_SSMPdev->getSupportedMBs(&supportedMBs)) || (!_SSMPdev->getSupportedSWs(&supportedSWs)))
		return false;	// commError
	_infoWidget->setNrOfSupportedMBsSWs(supportedMBs.size(), supportedSWs.size());
	return true;
}

