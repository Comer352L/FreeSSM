/*
 * ControlUnitDialog.cpp - Template for Control Unit dialogs
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

#include "ControlUnitDialog.h"


ControlUnitDialog::ControlUnitDialog(QString title, serialCOM *port, QString language)
{
	// *** Initialize global variables:
	_language = language;
	_port = port;
	_SSMPdev = NULL;
	_infoWidget = NULL;
	_contentWidget = NULL;
	// *** Setup window/GUI:
	setAttribute(Qt::WA_DeleteOnClose, true);
	// Setup GUI:
	setupUi(this);
	setupUiFonts();
	// Move window to desired coordinates
	move( 30, 30 );
	// Set window and dialog titles:
	setWindowTitle("FreeSSM " + QApplication::applicationVersion() + " - " + title);
	title_label->setText(title);
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
	_infoWidget = infowidget;
}


void ControlUnitDialog::setContentWidget(QString title, QWidget *contentwidget)
{
	if (_contentWidget)
		delete _contentWidget;
	content_groupBox->setTitle(title);
	contentwidget->setParent(content_groupBox);
	content_groupBox->layout()->addWidget(contentwidget);
	_contentWidget = contentwidget;
}


QPushButton * ControlUnitDialog::addFunction(QString title, QIcon icon, bool checkable)
{
	QPushButton *button = new QPushButton(selection_groupBox);
	button->setGeometry(20, 23 + 41 * _selButtons.size(), 152, 35);
	button->setCheckable(checkable);
	button->setAutoExclusive(checkable);
	// Icon:
	button->setIconSize(QSize(22, 22));
	button->setIcon(icon);
	// Font:
	QFont font = button->font();
	font.setFamily(QApplication::font().family());
	font.setBold(false);
	font.setPixelSize(13);	// 10pts
	button->setFont(font);
	// Text: prepend/append spaces to achieve proper icon positions:
	QFontMetrics fm(font);
	title = title.trimmed();
	int targetstrsize = button->size().width() - button->iconSize().width() - 18;
	int barestrsize = fm.size(Qt::TextShowMnemonic, title).width();
	if (barestrsize < targetstrsize)
	{
		double spacesize = fm.size(Qt::TextShowMnemonic, " ").width();
		int nspaces = ((targetstrsize - barestrsize) / spacesize + 0.5);
		title.prepend( QString( nspaces/2, ' ' ) );
		title.append( QString( nspaces - nspaces/2, ' ' ) );
	}
	button->setText(title);
	// Save, show and return button:
	button->show();
	_selButtons.push_back(button);
	return button;
}


bool ControlUnitDialog::probeProtocol(SSMprotocol::CUtype_dt CUtype)
{
	// Probe SSM1-protocol:
	if (configurePort(1953, 'E'))
	{
		_SSMPdev = new SSMprotocol1(_port, _language);
		if (_SSMPdev->setupCUdata( CUtype ))
		{
			connect( _SSMPdev, SIGNAL( commError() ), this, SLOT( communicationError() ) );
			return true;
		}
		delete _SSMPdev;
		// Wait 500ms:
		QEventLoop el;
		QTimer::singleShot(500, &el, SLOT(quit()));
		el.exec();
	}
	// Probe SSM2-protocol:
	if ((CUtype == SSMprotocol::CUtype_Engine) || (CUtype == SSMprotocol::CUtype_Transmission))
	{
		if (configurePort(4800, 'N'))
		{
			_SSMPdev = new SSMprotocol2(_port, _language);
			if (_SSMPdev->setupCUdata( CUtype ))
			{
				connect( _SSMPdev, SIGNAL( commError() ), this, SLOT( communicationError() ) );
				return true;
			}
			delete _SSMPdev;
		}
	}
	_SSMPdev = NULL;
	return false;
}


bool ControlUnitDialog::configurePort(unsigned int baud, char parity)
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


void ControlUnitDialog::communicationError(QString addstr)
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


void ControlUnitDialog::closeEvent(QCloseEvent *event)
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


void ControlUnitDialog::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS OF ui_ControlUnit.h (made with QDesigner)
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
	font = selection_groupBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(15);	// 11-12pts
	selection_groupBox->setFont(font);
	font = exit_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	exit_pushButton->setFont(font);
	font = content_groupBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(15);	// 11-12pts
	content_groupBox->setFont(font);
}

