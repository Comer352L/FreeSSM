/*
 * ControlUnitDialog.cpp - Template for Control Unit dialogs
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

#include "ControlUnitDialog.h"
#include "CmdLine.h"
#include "SSMprotocol1.h"
#include "SSMprotocol2.h"
#include "ClearMemoryDlg.h"


ControlUnitDialog::ControlUnitDialog(QString title, AbstractDiagInterface *diagInterface, QString language, bool preferSSM2protocolVariantISO14230)
{
	// *** Initialize global variables:
	_language = language;
	_diagInterface = diagInterface;
	_preferSSM2protocolVariantISO14230 = preferSSM2protocolVariantISO14230;
	_SSMPdev = NULL;
	_infoWidget = NULL;
	_contentWidget = NULL;
	_setup_done = false;
	_mode = Mode::None;
	_content_DCs = NULL;
	_content_MBsSWs = NULL;
	_content_Adjustments = NULL;
	_content_SysTests = NULL;
	// Setup GUI:
	setupUi(this);
	// enable maximize and minimize buttons
	//   GNOME 3 at least: this also enables fast window management e.g. "View split on left" (Super-Left), "... right" (Super-Right)
	setWindowFlags( Qt::Window );
	// Set window and dialog titles:
	setWindowTitle("FreeSSM " + QApplication::applicationVersion() + " - " + title);
	title_label->setText(title);
#ifndef SMALL_RESOLUTION
	// Apply quirk for GTK+-Layout:
	if (QApplication::style()->objectName().toLower() == "gtk+")
	{
		int left = 0;
		int top = 0;
		int right = 0;
		int bottom = 0;
		selection_horizontalLayout->getContentsMargins(&left, &top, &right, &bottom);
		selection_horizontalLayout->setContentsMargins(left, top+8, right, bottom);
		content_gridLayout->getContentsMargins(&left, &top, &right, &bottom);
		content_gridLayout->setContentsMargins(left, top+8, right, bottom);
	}
	// Add status bar for the diagnostic interface
	main_verticalLayout->setContentsMargins(-1, -1, -1, 4);
	_ifstatusbar = new DiagInterfaceStatusBar(this);
	main_verticalLayout->addWidget(_ifstatusbar);
	_ifstatusbar->show();
	_ifstatusbar->setInterfaceName( QString::fromStdString( diagInterface->name() ), Qt::blue );
	_ifstatusbar->setInterfaceVersion( QString::fromStdString( diagInterface->version() ), Qt::blue );
	_ifstatusbar->setProtocolName("---");
	_ifstatusbar->setBaudRate("---");
#endif
	// Connect signals and slots:
	connect( exit_pushButton, SIGNAL( released() ), this, SLOT( close() ) );
}


ControlUnitDialog::~ControlUnitDialog()
{
	disconnect( exit_pushButton, SIGNAL( released() ), this, SLOT( close() ) );
	if (_infoWidget)
		delete _infoWidget;
	deleteContentWidgets();
#ifndef SMALL_RESOLUTION
	delete _ifstatusbar;
#endif
	if (_SSMPdev)
	{
		disconnect( _SSMPdev, SIGNAL( commError() ), this, SLOT( communicationError() ) );
		delete _SSMPdev;
	}
	// Delete selection buttons:
	QList<QPushButton*> buttons = _contentSelectionButtons.values();
	_contentSelectionButtons.clear();
	for (int k=0; k<buttons.size(); k++)
	{
		buttons.at(k)->disconnect();
		delete buttons.at(k);
	}
}


void ControlUnitDialog::addContent(ContentSelection csel)
{
	QString title;
	QIcon icon;
	bool checkable;
	if (csel == ContentSelection::DCsMode)
	{
		title = tr("&Diagnostic Codes");
		icon = QIcon(QString::fromUtf8(":/icons/chrystal/22x22/messagebox_warning.png"));
		checkable = true;
	}
	else if (csel == ContentSelection::MBsSWsMode)
	{
		title = tr("&Measuring Blocks");
		icon = QIcon(QString::fromUtf8(":/icons/oxygen/22x22/applications-utilities.png"));
		checkable = true;
	}
	else if (csel == ContentSelection::AdjustmentsMode)
	{
		title = tr("&Adjustments");
		icon = QIcon(QString::fromUtf8(":/icons/chrystal/22x22/configure.png"));
		checkable = true;
	}
	else if (csel == ContentSelection::SysTestsMode)
	{
		title = tr("System &Tests");
		icon = QIcon(QString::fromUtf8(":/icons/chrystal/22x22/klaptop.png"));
		checkable = true;
	}
	else if (csel == ContentSelection::ClearMemoryFcn)
	{
		title = tr("Clear Memory");
		icon = QIcon(QString::fromUtf8(":/icons/chrystal/22x22/eraser.png"));
		checkable = false;
	}
	else if (csel == ContentSelection::ClearMemory2Fcn)
	{
		title = tr("Clear Memory 2");
		icon = QIcon(QString::fromUtf8(":/icons/chrystal/22x22/eraser.png"));
		checkable = false;
	}
	else // BUG
		return;
	// Create button:
	QPushButton *button = new QPushButton(selection_groupBox);
	selButtons_verticalLayout->insertWidget(_contentSelectionButtons.size(), button);
#ifndef SMALL_RESOLUTION
	button->setFixedWidth(160);
	button->setFixedHeight(35);
#else
	button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
#endif
	button->setCheckable(checkable);
	button->setAutoExclusive(checkable);
	button->setDisabled(true);
	// Icon:
	button->setIconSize(QSize(22, 22));
	button->setIcon(icon);
	// Font:
	QFont font = button->font();
	font.setFamily(QApplication::font().family());
	font.setBold(false);
	font.setPointSize(10);
	button->setFont(font);
	// Text: prepend/append spaces to achieve proper icon positions:
	QFontMetrics fm(font);
	title = title.trimmed();
	int targetstrsize = button->size().width() - button->iconSize().width() - 18;
	int barestrsize = fm.size(Qt::TextShowMnemonic, title).width();
	if (barestrsize < targetstrsize)
	{
		double spacesize = fm.size(Qt::TextShowMnemonic, " ").width();
		int nspaces = static_cast<int>((targetstrsize - barestrsize) / spacesize + 0.5);
		title.prepend( QString( nspaces/2, ' ' ) );
		title.append( QString( nspaces - nspaces/2, ' ' ) );
	}
	button->setText(title);
	// Connect buttons with slots:
	if (csel == ContentSelection::DCsMode)
		connect( button, SIGNAL( clicked() ), this, SLOT( switchToDCsMode() ) );
	else if (csel == ContentSelection::MBsSWsMode)
		connect( button, SIGNAL( clicked() ), this, SLOT( switchToMBsSWsMode() ) );
	else if (csel == ContentSelection::AdjustmentsMode)
		connect( button, SIGNAL( clicked() ), this, SLOT( switchToAdjustmentsMode() ) );
	else if (csel == ContentSelection::SysTestsMode)
		connect( button, SIGNAL( clicked() ), this, SLOT( switchToSystemOperationTestsMode() ) );
	else if (csel == ContentSelection::ClearMemoryFcn)
		connect( button, SIGNAL( clicked() ), this, SLOT( clearMemory() ) );
	else if (csel == ContentSelection::ClearMemory2Fcn)
		connect( button, SIGNAL( clicked() ), this, SLOT( clearMemory2() ) );
	//else: BUG
	// Save, show and return button:
	button->show();
	_contentSelectionButtons.insert(csel, button);
}


bool ControlUnitDialog::contentSupported(ContentSelection csel)
{
	return _contentSelectionButtons.contains(csel);
}


bool ControlUnitDialog::setup(ContentSelection csel, QStringList cmdline_args)
{
	Mode mode;
	bool supported = false;
	QString mbssws_selfile = "";
	bool autostart = false;

	if (_setup_done)
		return false;
	if (!contentSupported(csel))
	{
		CmdLine::printError("function not supported by the selected Control Unit.");
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
	if (systemRequiresManualON())
	{
		// Inform user that system needs to be switched on manually:
		QMessageBox *msgbox = new QMessageBox(QMessageBox::Information, tr("Prepare system"), QString(tr("Please switch the %1 system on.").arg(systemName())), QMessageBox::NoButton, this);
		QPushButton *button = msgbox->addButton(QMessageBox::Ok);
		button->setText(tr("Continue"));
		msgbox->addButton(QMessageBox::Cancel);
		QFont msgfont = msgbox->font();
		msgfont.setPointSize(9);
		msgbox->setFont( msgfont );
		msgbox->show();
		int ret = msgbox->exec();
		delete msgbox;
		if (ret != QMessageBox::Ok)
		{
			close();
			return false;
		}
	}
	// Create Status information message box for CU initialisation/setup:
	FSSM_ProgressDialog initstatusmsgbox(tr("Connecting to %1... Please wait !").arg(controlUnitName()), 0, 0, 100, this);
	initstatusmsgbox.setWindowTitle(tr("Connecting..."));
	initstatusmsgbox.setValue(5);
	initstatusmsgbox.show();
	// Try to establish CU connection:
	SSMprotocol::CUsetupResult_dt init_result = probeProtocol( controlUnitType() );
	if ((init_result != SSMprotocol::result_success) && (init_result != SSMprotocol::result_noOrInvalidDefsFile) && (init_result != SSMprotocol::result_noDefs))
		goto commError;
	// Update status info message box:
	initstatusmsgbox.setLabelText(tr("Processing Control Unit data... Please wait !"));
	initstatusmsgbox.setValue(40);
	// Display system description and ID:
	if (!displaySystemDescriptionAndID(_SSMPdev, _infoWidget))
		goto commError;
	// Check if we have valid definitions for this Control Unit:
	if (init_result != SSMprotocol::result_success)
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
			errtext = tr("Error:\nThis control unit is not yet supported by FreeSSM.\n"\
			             "FreeSSM can communicate with the control unit, but it doesn't have the necessary data to provide diagnostic operations.\n"\
			             "If you want to contribute to the project (help adding defintions), feel free to contact the authors.");
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
	// Display extended Control Unit information:
	if (!displayExtendedCUinfo(_SSMPdev, _infoWidget, &initstatusmsgbox))
		goto commError;
	// ***** Prepare the Control Unit *****:
	// Check if we need to stop the automatic actuator test:
	if (!_SSMPdev->hasActuatorTests(&supported))
		goto commError;
	if (supported)
	{
		if (!handleActuatorTests(&initstatusmsgbox))
			goto commError;
	}
	// ***** Enable content selection buttons *****:
	if (contentSupported(ContentSelection::ClearMemoryFcn))
	{
		// "Clear Memory"-support:
		if (!_SSMPdev->hasClearMemory(&supported))
			goto commError;
		setContentSelectionButtonEnabled(ContentSelection::ClearMemoryFcn, supported);
	}
	if (contentSupported(ContentSelection::ClearMemory2Fcn))
	{
		// "Clear Memory 2"-support:
		if (!_SSMPdev->hasClearMemory2(&supported))
			goto commError;
		setContentSelectionButtonEnabled(ContentSelection::ClearMemory2Fcn, supported);
	}
	// NOTE: enable modes unconditionally, UI contents are deactivated if unsupported by the CU
	setContentSelectionButtonEnabled(ContentSelection::DCsMode, true);
	setContentSelectionButtonEnabled(ContentSelection::MBsSWsMode, true);
	setContentSelectionButtonEnabled(ContentSelection::AdjustmentsMode, true);
	setContentSelectionButtonEnabled(ContentSelection::SysTestsMode, true);
	// ***** Start selected Control Unit content/functionality *****:
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
	{
		clearMemory();
	}
	else if (csel == ContentSelection::ClearMemory2Fcn)
	{
		clearMemory2();
	}
	// Apply command line startup parameters for MB/SW mode:
	else if ((csel == ContentSelection::MBsSWsMode) && (_content_MBsSWs != NULL))
	{
		if (mbssws_selfile.size())
			_content_MBsSWs->loadMBsSWs(mbssws_selfile);
		if (_content_MBsSWs->numMBsSWsSelected() && autostart)
			_content_MBsSWs->startMBSWreading();
	}

	_setup_done = true;
	return true;

commError:
	initstatusmsgbox.close();
	communicationError();
	return false;
}


void ControlUnitDialog::setInfoWidget(CUinfo_abstract *infowidget)
{
	if (_infoWidget)
		delete _infoWidget;
	infowidget->setParent(information_groupBox);
	information_groupBox->setMinimumHeight(infowidget->minimumHeight());
	infowidget->show();
	_infoWidget = infowidget;
}


bool ControlUnitDialog::displaySystemDescriptionAndID(SSMprotocol *SSMPdev, CUinfo_abstract *abstractInfoWidget)
{
	QString sysdescription = "";
	std::string SYS_ID = "";
	std::string ROM_ID = "";
	if (SSMPdev == NULL)
		return false;
	if (abstractInfoWidget == NULL)
		return true; // NOTE: no communication error
	// Query ROM-ID:
	ROM_ID = SSMPdev->getROMID();
	if (!ROM_ID.length())
		return false;
	// Query system description:
	if (!SSMPdev->getSystemDescription(&sysdescription))
	{
		SYS_ID = SSMPdev->getSysID();
		if (!SYS_ID.length())
			return false;
		sysdescription = tr("unknown");
		if (SYS_ID != ROM_ID)
			sysdescription += " (" + QString::fromStdString(SYS_ID) + ")";
	}
	// Display system description and ID:
	abstractInfoWidget->setSystemTypeText(sysdescription);
	abstractInfoWidget->setRomIDText(QString::fromStdString(ROM_ID));

	return true;
}


bool ControlUnitDialog::prepareContentWidget(Mode mode)
{
	// ***** Create, setup and insert the content widget *****:
	if (mode == Mode::DCs)
	{
		setContentSelectionButtonChecked(ContentSelection::DCsMode, true);
		_content_DCs = allocate_DCsContentWidget();
		setContentWidget(tr("Diagnostic Codes:"), _content_DCs);
		_content_DCs->show();
	}
	else if (mode == Mode::MBsSWs)
	{
		setContentSelectionButtonChecked(ContentSelection::MBsSWsMode, true);
		_content_MBsSWs = new CUcontent_MBsSWs(_MBSWsettings);
		setContentWidget(tr("Measuring Blocks:"), _content_MBsSWs);
		_content_MBsSWs->show();
	}
	else if (mode == Mode::Adjustments)
	{
		setContentSelectionButtonChecked(ContentSelection::AdjustmentsMode, true);
		_content_Adjustments = new CUcontent_Adjustments();
		setContentWidget(tr("Adjustments:"), _content_Adjustments);
		_content_Adjustments->show();
	}
	else if (mode == Mode::SysTests)
	{
		setContentSelectionButtonChecked(ContentSelection::SysTestsMode, true);
		_content_SysTests = new CUcontent_sysTests();
		setContentWidget(tr("System Operation Tests:"), _content_SysTests);
		_content_SysTests->show();
	}
	else // BUG
		return false;
	return true;
}


void ControlUnitDialog::setContentWidget(QString title, QWidget *contentwidget)
{
	content_groupBox->setTitle(title);
	contentwidget->setParent(content_groupBox);
	content_gridLayout->addWidget(contentwidget);
	_contentWidget = contentwidget;
}


void ControlUnitDialog::deleteContentWidgets()
{
	if (_content_DCs != NULL)
	{
		delete(_content_DCs);
		_content_DCs = NULL;
	}
	if (_content_MBsSWs != NULL)
	{
		delete(_content_MBsSWs);
		_content_MBsSWs = NULL;
	}
	if (_content_Adjustments != NULL)
	{
		delete(_content_Adjustments);
		_content_Adjustments = NULL;
	}
	if (_content_SysTests != NULL)
	{
		delete(_content_SysTests);
		_content_SysTests = NULL;
	}
	_contentWidget = NULL;
}


QWidget * ControlUnitDialog::contentWidget()
{
	return _contentWidget;
}


bool ControlUnitDialog::getParametersFromCmdLine(QStringList *cmdline_args, QString *selection_file, bool *autostart)
{
	QStringList cargs;
	QStringList parameters;
	QString selfile = "";
	bool astart = false;

	if (cmdline_args == NULL)
		return false;
	cargs = *cmdline_args;
	// Check if command line option -p/--parameters is specified and extract corresponding value(s):
	if (!CmdLine::parseForOption(&cargs, "-p", "--parameters", &parameters))
		return true; // no error, option unused
	// Validate number of specified parameters:
	if (parameters.size() < 1)
	{
		CmdLine::printError("no parameter(s) specified with option -p/--parameters");
		return false;
	}
	// Validate parameter strings:
	for (int p=0; p<parameters.size(); p++)
	{
		if (parameters.at(p).startsWith("selectionfile="))
		{
			if (selfile.size())
			{
				CmdLine::printError("parameter \"selectionfile=<FILE>\" specified multiple times with different values");
				// NOTE: values must be different, because otherwise the whole parameter would have been eliminated as duplicate
				return false;
			}
			selfile = parameters.at(p);
			selfile.remove(0, selfile.indexOf('=') + 1);
			if ((selfile.size()) && !QFile::exists(selfile))
			{
				CmdLine::printError("specified MBs/SWs selection file does not exist");
				return false;
			}
		}
		else if (parameters.at(p) == "autostart")
		{
			astart = true;
		}
		else
		{
			CmdLine::printError("unknown parameter specified with option -p/--parameters: " + parameters.at(p));
			return false;
		}
	}
	// Check if a selection file has been specified if autostart is specified:
	if (astart && !selfile.size())
	{
		CmdLine::printError("option -p/--parameters: parameter \"autostart\" specified without parameter \"selectionfile=<FILE>\"");
		return false;
	}
	// Assign return values:
	*cmdline_args = cargs;
	if (selfile.size() && (selection_file != NULL))
		*selection_file = selfile;
	if (astart && (autostart != NULL))
		*autostart = true;
	return true;
}


bool ControlUnitDialog::getModeForContentSelection(ContentSelection csel, Mode *mode)
{
	if ((csel == ContentSelection::DCsMode) || (csel == ContentSelection::ClearMemoryFcn) || (csel == ContentSelection::ClearMemory2Fcn))
		*mode = Mode::DCs;
	else if (csel == ContentSelection::MBsSWsMode)
		*mode = Mode::MBsSWs;
	else if (csel == ContentSelection::AdjustmentsMode)
		*mode = Mode::Adjustments;
	else if (csel == ContentSelection::SysTestsMode)
		*mode = Mode::SysTests;
	else // BUG
		return false;
	return true;
}


void ControlUnitDialog::setContentSelectionButtonEnabled(ContentSelection csel, bool enabled)
{
	if (_contentSelectionButtons.contains(csel))
		_contentSelectionButtons.value(csel)->setEnabled(enabled);
}


void ControlUnitDialog::setContentSelectionButtonChecked(ContentSelection csel, bool checked)
{
	if (_contentSelectionButtons.contains(csel))
		_contentSelectionButtons.value(csel)->setChecked(checked);
}


SSMprotocol::CUsetupResult_dt ControlUnitDialog::probeProtocol(CUtype CUtype)
{
	/* NOTE:  probe SSM2-protocol first !
	   If a serial pass through (K)KL-interface is used, the interface echo could be detected as a SSM1-ROM-ID,
	   if receive buffer flushing doesn't work reliable with the used serial port driver !
	*/
	SSMprotocol::CUsetupResult_dt result = SSMprotocol::result_commError;
	if ((CUtype == CUtype::Engine) || (CUtype == CUtype::Transmission))
	{
		// Probe SSM2-protocol:
		AbstractDiagInterface::protocol_type first_protocol = AbstractDiagInterface::protocol_type::SSM2_ISO15765;
		AbstractDiagInterface::protocol_type second_protocol = AbstractDiagInterface::protocol_type::SSM2_ISO14230;
		if (_preferSSM2protocolVariantISO14230)
		{
			first_protocol = AbstractDiagInterface::protocol_type::SSM2_ISO14230;
			second_protocol = AbstractDiagInterface::protocol_type::SSM2_ISO15765;
		}
		_SSMPdev = new SSMprotocol2(_diagInterface, _language);
		if (_diagInterface->connect(first_protocol))
		{
			result = _SSMPdev->setupCUdata( CUtype );
			if ((result != SSMprotocol::result_success) && (result != SSMprotocol::result_noOrInvalidDefsFile) && (result != SSMprotocol::result_noDefs))
				_diagInterface->disconnect();
		}
		if (!_diagInterface->isConnected())
		{
			if (_diagInterface->connect(second_protocol))
			{
				result = _SSMPdev->setupCUdata( CUtype );
				if ((result != SSMprotocol::result_success) && (result != SSMprotocol::result_noOrInvalidDefsFile) && (result != SSMprotocol::result_noDefs))
					_diagInterface->disconnect();
			}
		}
		if (_diagInterface->isConnected())
			connect( _SSMPdev, SIGNAL( commError() ), this, SLOT( communicationError() ) );
		else
		{
			delete _SSMPdev;
			_SSMPdev = NULL;
		}
	}
	if (_SSMPdev == NULL)
	{
		// Probe SSM1-protocol:
		if (_diagInterface->connect(AbstractDiagInterface::protocol_type::SSM1))
		{
			_SSMPdev = new SSMprotocol1(_diagInterface, _language);
			result = _SSMPdev->setupCUdata( CUtype );
			if ((result == SSMprotocol::result_success) || (result == SSMprotocol::result_noOrInvalidDefsFile) || (result == SSMprotocol::result_noDefs))
				connect( _SSMPdev, SIGNAL( commError() ), this, SLOT( communicationError() ) );
			else
			{
				delete _SSMPdev;
				_SSMPdev = NULL;
				_diagInterface->disconnect();
			}
		}
	}
#ifndef SMALL_RESOLUTION
	// Update diagnostic interface status bar:
	if (_SSMPdev != NULL)
	{
		// Display protocol name:
		_ifstatusbar->setProtocolName( QString::fromStdString( _diagInterface->protocolDescription() ), Qt::darkGreen );
		// Display protocol baud rate:
		unsigned int br = _diagInterface->protocolBaudRate();
		QString bstr;
		if (br <100000)
		{
			bstr = QString::number(br) + " baud";
		}
		else if (br < 100000000)
		{
			bstr = QString::number(br/1000.0, 'g', 5) + " Kbaud";
		}
		else
		{
			bstr = QString::number(br/1000000.0, 'g', 5) + " Mbaud";
		}
		_ifstatusbar->setBaudRate(bstr, Qt::darkGreen);
	}
#endif
	return result;
}


bool ControlUnitDialog::handleActuatorTests(FSSM_ProgressDialog *statusmsgbox)
{
	CUtype ecu_type;
	if (!_SSMPdev->CUtype(&ecu_type))
		return false;
	if ((ecu_type == CUtype::Engine) && (_SSMPdev->ifceProtocolType() != AbstractDiagInterface::protocol_type::SSM2_ISO15765))
	// NOTE: we restrict actuator test stopping to ECUs for now. Might be required / make sense for other ECUs, too...
	// FIXME: enable ISO-15765 again, when SSMprotocol has been fixed
	{
		bool supported = false;

		// Update status info message box:
		statusmsgbox->setLabelText(tr("Checking system status... Please wait !"));
		statusmsgbox->setValue(70);

		// Check test mode state (if provided by ECU):
		if (!_SSMPdev->hasTestMode(&supported))
			return false;
		if (supported)
		{
			// Query test mode connector status:
			bool testmode = false;
			if (!_SSMPdev->isInTestMode(&testmode))
				return false;
			if (!testmode)
				return true;
		} // else: assume it is not relevant (otherwise the ECU would provide it)

		// Check engine speed (if provided by ECU):
		if (!_SSMPdev->hasMBengineSpeed(&supported))
			return false;
		if (supported)
		{
			bool enginerunning = false;
			// Check that engine is not running:
			if (!_SSMPdev->isEngineRunning(&enginerunning))
				return false;
			if (enginerunning)
				return true;
		} // else: assume it is not relevant (otherwise the ECU would provide it)

		// Update status info message box:
		statusmsgbox->setLabelText(tr("Stopping actuators... Please wait !"));
		statusmsgbox->setValue(85);

		// Stop all actuator tests:
		if (!_SSMPdev->stopAllActuators())
			return false;

	} // else: FIXME/TODO: maybe we should/must stop actuator tests with other ECU types, too...

	return true;
}


void ControlUnitDialog::switchToDCsMode()
{
	bool com_err = false;
	if (_mode == Mode::DCs) return;
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to Diagnostic Codes... Please wait !"));
	waitmsgbox.show();
	// Save content settings:
	saveContentSettings();
	// Delete current content widget:
	deleteContentWidgets();
	// Create and insert new content widget:
	if (prepareContentWidget(Mode::DCs))
		// Start DCs mode:
		com_err = !startDCsMode();
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (com_err)
		communicationError();
}


void ControlUnitDialog::switchToMBsSWsMode()
{
	bool com_err = false;
	if (_mode == Mode::MBsSWs) return;
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to Measuring Blocks... Please wait !"));
	waitmsgbox.show();
	// Save content settings:
	saveContentSettings();
	// Delete current content widget:
	deleteContentWidgets();
	// Create and insert new content widget:
	if (prepareContentWidget(Mode::MBsSWs))
		// Start MB/SW mode:
		com_err = !startMBsSWsMode();
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (com_err)
		communicationError();
}


void ControlUnitDialog::switchToAdjustmentsMode()
{
	bool com_err = false;
	if (_mode == Mode::Adjustments) return;
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to Adjustment Values... Please wait !"));
	waitmsgbox.show();
	// Save content settings:
	saveContentSettings();
	// Delete current content widget:
	deleteContentWidgets();
	// Create and insert new content widget:
	if (prepareContentWidget(Mode::Adjustments))
		// Start Adjustments mode:
		com_err = !startAdjustmentsMode();
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (com_err)
		communicationError();
}


void ControlUnitDialog::switchToSystemOperationTestsMode()
{
	bool com_err = false;
	if (_mode == Mode::SysTests) return;
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Switching to System Tests... Please wait !"));
	waitmsgbox.show();
	// Save content settings:
	saveContentSettings();
	// Delete current content widget:
	deleteContentWidgets();
	// Create and insert new content widget:
	if (prepareContentWidget(Mode::SysTests))
		// Start System Operation Tests mode:
		com_err = !startSystemOperationTestsMode();
	// Close wait-message:
	waitmsgbox.close();
	// Check for communication error:
	if (com_err)
		communicationError();
}


bool ControlUnitDialog::startMode(Mode mode)
{
	bool ok = true;
	if (mode == Mode::DCs)
		ok = startDCsMode();
	else if (mode == Mode::MBsSWs)
		ok = startMBsSWsMode();
	else if (mode == Mode::Adjustments)
		ok = startAdjustmentsMode();
	else if (mode == Mode::SysTests)
		ok = startSystemOperationTestsMode();
	else // BUG
		ok = false;
	return ok;
}


bool ControlUnitDialog::startDCsMode()
{
	int DCgroups = 0;
	if (_content_DCs == NULL)
		return false;
	if (!_content_DCs->setup(_SSMPdev))
		return false;
	if (!_SSMPdev->getSupportedDCgroups(&DCgroups))
		return false;
	if (DCgroups != SSMprotocol::noDCs_DCgroup)
	{
		if (!_content_DCs->startDCreading())
			return false;
		connect(_content_DCs, SIGNAL( error() ), this, SLOT( close() ) );
	}
	_mode = Mode::DCs;
	return true;
}


bool ControlUnitDialog::startMBsSWsMode()
{
	if (_content_MBsSWs == NULL)
		return false;
	if (!_content_MBsSWs->setup(_SSMPdev))
		return false;
	if (!_content_MBsSWs->setMBSWselection(_lastMBSWmetaList))
		return false;
	connect(_content_MBsSWs, SIGNAL( error() ), this, SLOT( close() ) );
	_mode = Mode::MBsSWs;
	return true;
}


bool ControlUnitDialog::startAdjustmentsMode()
{
	if (_content_Adjustments == NULL)
		return false;
	if (!_content_Adjustments->setup(_SSMPdev))
		return false;
	connect(_content_Adjustments, SIGNAL( communicationError() ), this, SLOT( close() ) );
	_mode = Mode::Adjustments;
	return true;
}


bool ControlUnitDialog::startSystemOperationTestsMode()
{
	if (_content_SysTests == NULL)
		return false;
	if (!_content_SysTests->setup(_SSMPdev))
		return false;
	connect(_content_SysTests, SIGNAL( error() ), this, SLOT( close() ) );
	_mode = Mode::SysTests;
	return true;
}


void ControlUnitDialog::clearMemory()
{
	runClearMemory(SSMprotocol::CMlevel_1);
}


void ControlUnitDialog::clearMemory2()
{
	runClearMemory(SSMprotocol::CMlevel_2);
}


void ControlUnitDialog::runClearMemory(SSMprotocol::CMlevel_dt level)
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
	if ((result == ClearMemoryDlg::CMresult_success) && (_mode == Mode::Adjustments))
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


void ControlUnitDialog::saveContentSettings()
{
	if ((_mode == Mode::MBsSWs) && (_content_MBsSWs != NULL))
	{
		_lastMBSWmetaList = _content_MBsSWs->getMBSWselection();
		_MBSWsettings = _content_MBsSWs->getSettings();
	}
}


void ControlUnitDialog::communicationError(QString addstr)
{
	// Show error message
	if (addstr.size() > 0) addstr.prepend('\n');
	QMessageBox msg( QMessageBox::Critical, tr("Communication Error"), tr("Communication Error:\n- No or invalid answer from Control Unit -") + addstr, QMessageBox::Ok, this);
	QFont msgfont = msg.font();
	msgfont.setPointSize(9);
	msg.setFont( msgfont );
	msg.show();
	msg.exec();
	msg.close();
	// Close dialog (and delete all objects)
	close();
}


void ControlUnitDialog::closeEvent(QCloseEvent *event)
{
	if (_SSMPdev)
	{
		// Create wait message box:
		FSSM_WaitMsgBox waitmsgbox(this, tr("Stopping Communication... Please wait !"));
		waitmsgbox.show();
		// Reset CU data:
		_SSMPdev->resetCUdata();
		// Close wait message box:
		waitmsgbox.close();
	}
	event->accept();
}
