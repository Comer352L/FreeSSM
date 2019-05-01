/*
 * Engine.cpp - Engine Control Unit dialog
 *
 * Copyright (C) 2008-2019 Comer352L
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

#include "EngineDialog.h"
#include "CmdLine.h"
#include "CUcontent_DCs_engine.h"


EngineDialog::EngineDialog(AbstractDiagInterface *diagInterface, QString language) : ControlUnitDialog(controlUnitName(), diagInterface, language)
{
	// Show information-widget:
	_infoWidget = new CUinfo_Engine();
	setInfoWidget(_infoWidget);
	_infoWidget->show();
	// Setup functions:
	addContent(ContentSelection::DCsMode);
	addContent(ContentSelection::MBsSWsMode);
	addContent(ContentSelection::AdjustmentsMode);
	addContent(ContentSelection::SysTestsMode);
	addContent(ContentSelection::ClearMemoryFcn);
}


QString EngineDialog::systemName()
{
	return tr("Engine");
}


QString EngineDialog::controlUnitName()
{
	return tr("Engine Control Unit");
}


bool EngineDialog::setup(ContentSelection csel, QStringList cmdline_args)
{
	Mode mode;
	QString sysdescription = "";
	std::string SYS_ID = "";
	std::string ROM_ID = "";
	QString mbssws_selfile = "";
	bool autostart = false;

	if (_setup_done)
		return false;
	if (!contentSupported(csel))
	{
		CmdLine::printError("the specified function is not supported by the Engine Control Unit.");
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
	// Create Status information message box for CU initialisation/setup:
	FSSM_InitStatusMsgBox initstatusmsgbox(tr("Connecting to %1... Please wait !").arg(controlUnitName()), 0, 0, 100, this);
	initstatusmsgbox.setWindowTitle(tr("Connecting..."));
	initstatusmsgbox.setValue(5);
	initstatusmsgbox.show();
	// Try to establish CU connection:
	SSMprotocol::CUsetupResult_dt init_result = probeProtocol(SSMprotocol::CUtype_Engine);
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
	_infoWidget->setEngineTypeText(sysdescription);
	// Output ROM-ID:
	_infoWidget->setRomIDText( QString::fromStdString(ROM_ID) );
	if (init_result == SSMprotocol::result_success)
	{
		QString VIN = "";
		bool supported = false;
		bool testmode = false;
		bool enginerunning = false;
		// Fill info widget with information about the Control Unit:
		if (!fillInfoWidget(&initstatusmsgbox))
			goto commError;
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
			if (testmode)
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
		// "Clear Memory"-support:
		if (!_SSMPdev->hasClearMemory(&supported))
			goto commError;
		setContentSelectionButtonEnabled(ContentSelection::ClearMemoryFcn, supported);
		// Enable mode buttons:
		// NOTE: unconditionally, contents are deactivated if unsupported
		setContentSelectionButtonEnabled(ContentSelection::DCsMode, true);
		setContentSelectionButtonEnabled(ContentSelection::MBsSWsMode, true);
		setContentSelectionButtonEnabled(ContentSelection::AdjustmentsMode, true);
		setContentSelectionButtonEnabled(ContentSelection::SysTestsMode, true);
		// Start selected mode:
		if (!startMode(mode))
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
		else if ((csel == ContentSelection::MBsSWsMode) && (_content_MBsSWs != NULL))
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


CUcontent_DCs_abstract * EngineDialog::allocate_DCsContentWidget()
{
	return new CUcontent_DCs_engine();
}


bool EngineDialog::fillInfoWidget(FSSM_InitStatusMsgBox *initstatusmsgbox)
{
	QString VIN = "";
	bool supported = false;
	std::vector<mb_dt> supportedMBs;
	std::vector<sw_dt> supportedSWs;
	// Number of supported MBs / SWs:
	if ((!_SSMPdev->getSupportedMBs(&supportedMBs)) || (!_SSMPdev->getSupportedSWs(&supportedSWs)))
		return false;	// commError
	_infoWidget->setNrOfSupportedMBsSWs(supportedMBs.size(), supportedSWs.size());
	// OBD2-Support:
	if (!_SSMPdev->hasOBD2system(&supported))
		return false;	// commError
	_infoWidget->setOBD2Supported(supported);
	// Integrated Cruise Control:
	if (!_SSMPdev->hasIntegratedCC(&supported))
		return false;	// commError
	_infoWidget->setIntegratedCCSupported(supported);
	// Immobilizer:
	if (!_SSMPdev->hasImmobilizer(&supported))
		return false;	// commError
	_infoWidget->setImmobilizerSupported(supported);
	// Update status info message box:
	if (initstatusmsgbox != NULL)
	{
		initstatusmsgbox->setLabelText(tr("Reading Vehicle Ident. Number... Please wait !"));
		initstatusmsgbox->setValue(55);
	}
	// Query and output VIN, if supported:
	if (!_SSMPdev->hasVINsupport(&supported))
		return false;	// commError
	if (supported)
	{
		if (!_SSMPdev->getVIN(&VIN))
			return false;	// commError
	}
	_infoWidget->setVINinfo(supported, VIN);
	return true;
}

