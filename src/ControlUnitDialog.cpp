/*
 * ControlUnitDialog.cpp - Template for Control Unit dialogs
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

#include "ControlUnitDialog.h"


ControlUnitDialog::ControlUnitDialog(QString title, AbstractDiagInterface *diagInterface, QString language)
{
	// *** Initialize global variables:
	_language = language;
	_diagInterface = diagInterface;
	_SSMPdev = NULL;
	_infoWidget = NULL;
	_contentWidget = NULL;
	// Setup GUI:
	setupUi(this);
	// enable maximize and minimize buttons
	//   GNOME 3 at least: this also enables fast window management e.g. "View split on left" (Super-Left), "... right" (Super-Right)
	setWindowFlags( Qt::Window );
	// Set window and dialog titles:
	setWindowTitle("FreeSSM " + QApplication::applicationVersion() + " - " + title);
	title_label->setText(title);

#ifndef SMALL_RESOLUTION
	// Move window to desired coordinates:
	move( 30, 30 );
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
#else
	showFullScreen();
#endif

	// Connect signals and slots:
	connect( exit_pushButton, SIGNAL( released() ), this, SLOT( close() ) );
}


ControlUnitDialog::~ControlUnitDialog()
{
	disconnect( exit_pushButton, SIGNAL( released() ), this, SLOT( close() ) );
	if (_infoWidget)
		delete _infoWidget;
	if (_contentWidget)
		delete _contentWidget;
#ifndef SMALL_RESOLUTION
	delete _ifstatusbar;
#endif
	if (_SSMPdev)
	{
		disconnect( _SSMPdev, SIGNAL( commError() ), this, SLOT( communicationError() ) );
		delete _SSMPdev;
	}
	for (int k=0; k<_selButtons.size(); k++)
	{
		_selButtons.at(k)->disconnect();
		delete _selButtons.at(k);
	}
}


void ControlUnitDialog::setInfoWidget(QWidget *infowidget)
{
	if (_infoWidget)
		delete _infoWidget;
	infowidget->setParent(information_groupBox);
	information_groupBox->setMinimumHeight(infowidget->minimumHeight());
	_infoWidget = infowidget;
}


void ControlUnitDialog::setContentWidget(QString title, QWidget *contentwidget)
{
	if (_contentWidget)
		delete _contentWidget;
	content_groupBox->setTitle(title);
	contentwidget->setParent(content_groupBox);
	content_gridLayout->addWidget(contentwidget);
	_contentWidget = contentwidget;
}


QWidget * ControlUnitDialog::contentWidget()
{
	return _contentWidget;
}


QPushButton * ControlUnitDialog::addFunction(QString title, QIcon icon, bool checkable)
{
	QPushButton *button = new QPushButton(selection_groupBox);
	selButtons_verticalLayout->insertWidget(_selButtons.size(), button);

#ifndef SMALL_RESOLUTION
	button->setFixedWidth(160);
	button->setFixedHeight(35);
#else
	button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
#endif

	button->setCheckable(checkable);
	button->setAutoExclusive(checkable);
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
	// Save, show and return button:
	button->show();
	_selButtons.push_back(button);
	return button;
}


SSMprotocol::CUsetupResult_dt ControlUnitDialog::probeProtocol(SSMprotocol::CUtype_dt CUtype)
{
	/* NOTE:  probe SSM2-protocol first !
	   If a serial pass through (K)KL-interface is used, the interface echo could be detected as a SSM1-ROM-ID,
	   if receive buffer flushing doesn't work reliable with the used serial port driver !
	*/
	SSMprotocol::CUsetupResult_dt result = SSMprotocol::result_commError;
	if ((CUtype == SSMprotocol::CUtype_Engine) || (CUtype == SSMprotocol::CUtype_Transmission))
	{
		// Probe SSM2-protocol:
		_SSMPdev = new SSMprotocol2(_diagInterface, _language);
		if (_diagInterface->connect(AbstractDiagInterface::protocol_SSM2_ISO15765))
		{
			result = _SSMPdev->setupCUdata( CUtype );
			if ((result != SSMprotocol::result_success) && (result != SSMprotocol::result_noOrInvalidDefsFile) && (result != SSMprotocol::result_noDefs))
				_diagInterface->disconnect();
		}
		if (!_diagInterface->isConnected())
		{
			if (_diagInterface->connect(AbstractDiagInterface::protocol_SSM2_ISO14230))
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
		if (_diagInterface->connect(AbstractDiagInterface::protocol_SSM1))
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

