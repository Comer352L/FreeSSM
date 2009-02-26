/*
 * Preferences.cpp - Adjustment of program settings
 *
 * Copyright © 2008-2009 Comer352l
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

#include "Preferences.h"



Preferences::Preferences(QMainWindow *parent, QString *portname, QString language)
{
	_newportname = *portname;
	_r_portname = portname;
	_language_current = language;
	_language_old = language;
	_confirmed = false;
	// SET UP GUI:
	setupUi(this);
	setupUiFonts();
	// PLACE WINDOW:
	// get coordinates of FreeSSM_MainWindow
	QRect FreeSSM_MW_geometry;
	int FreeSSM_MW_Xpos, FreeSSM_MW_Ypos;
	FreeSSM_MW_geometry = parent->geometry();
	FreeSSM_MW_Xpos = FreeSSM_MW_geometry.x();
	FreeSSM_MW_Ypos = FreeSSM_MW_geometry.y();
	// calculate new window coordinates
	int x, y;
	x = ( FreeSSM_MW_Xpos  + 60);
	y = ( FreeSSM_MW_Ypos  + 40);
	// move window to desired coordinates
	move ( x, y );
	// OUTPUT LANGUAGES:
	if (_language_current == "en")
	{
		_lastlangindex = 0;
	}
	else if (_language_current == "de")
	{
		_lastlangindex = 1;
	}
	else	// if currently selected language is invalid
	{
		// TRY TO GET SYSTEM LANGUAGE SETTING:
		if ((QLocale::system().language() == QLocale::English))
		{
			_language_current = "en";
			switchLanguage(0);
			_lastlangindex = 0;
		}
		else if ((QLocale::system().language() == QLocale::German))
		{
			_language_current = "de";
			switchLanguage(1);
			_lastlangindex = 1;
		}
		else	// if system language settings are not available/supported
		{
			_language_current = "en";
			switchLanguage(0);
			_lastlangindex = 0;
		}
	}
	language_comboBox->setCurrentIndex(_lastlangindex);
	// GUI-STYLES:
	_style_old = QApplication::style()->objectName();
	QStringList supStyles = QStyleFactory::keys();
	if (supStyles.size())
	{
		int styleindex = supStyles.indexOf( QRegExp( _style_old, Qt::CaseInsensitive ) );
		guistyle_comboBox->insertItems(0, supStyles);
		guistyle_comboBox->setCurrentIndex( styleindex );
	}
	else
		guistyle_comboBox->setEnabled( false );
	// GET AVAILABLE PORTS:
	std::vector<std::string> portlist;
	portlist = serialCOM::GetAvailablePorts();
	// PREPARE PORTLIST FOR OUTPUT:
	QStringList qtportlist;
	int plindex = -1; 
	for (unsigned int k=0; k<portlist.size(); k++)
	{
		qtportlist += static_cast<QString>(portlist[k].c_str());
		// CHECK IF MEMBER IS CURRENT SELECTED PORT
		if (qtportlist[k] == *portname)
		{
			plindex = k;
		}
	}
	if (plindex == -1)	// if currently selected portname is not available
	{
		if (qtportlist.size() > 0)	// if min. 1 port is available
		{
			// select first available port:
			_newportname = static_cast<QString>(qtportlist[0]);
			*_r_portname = static_cast<QString>(qtportlist[0]);
			plindex = 0;
		}
		else	// if no port is availbale
		{
			_newportname = "";	// clear port name
			*_r_portname = "";	// clear port name (return value)
			serialport_comboBox->setEnabled(false);		// deactivate port selection box
			testinterface_pushButton->setEnabled(false);	// deactivate "Test Interface"-Button
		}
	}
	// OUTPUT PORTS:
	serialport_comboBox->insertItems(0, qtportlist);
	serialport_comboBox->setCurrentIndex(plindex);
	// CONNECT SIGNALS AND SLOTS:
	connect( language_comboBox, SIGNAL( activated(int) ), this, SLOT( switchLanguage(int) ) );
	connect( guistyle_comboBox, SIGNAL( activated(QString) ), this, SLOT( switchGUIstyle(QString) ) );
	connect( serialport_comboBox, SIGNAL( activated(QString) ), this, SLOT( selectSerialPort(QString) ) );
	connect( testinterface_pushButton, SIGNAL( released() ), this, SLOT( interfacetest() ) );
	connect( ok_pushButton, SIGNAL( released() ), this, SLOT( ok() ) );
	connect( cancel_pushButton, SIGNAL( released() ), this, SLOT( cancel() ) );
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
}


Preferences::~Preferences()
{
	disconnect( language_comboBox, SIGNAL( activated(int) ), this, SLOT( switchLanguage(int) ) );
	disconnect( guistyle_comboBox, SIGNAL( activated(QString) ), this, SLOT( switchGUIstyle(QString) ) );
	disconnect( serialport_comboBox, SIGNAL( activated(QString) ), this, SLOT( selectSerialPort(QString) ) );
	disconnect( testinterface_pushButton, SIGNAL( released() ), this, SLOT( interfacetest() ) );
	disconnect( ok_pushButton, SIGNAL( released() ), this, SLOT( ok() ) );
	disconnect( cancel_pushButton, SIGNAL( released() ), this, SLOT( cancel() ) );
}


void Preferences::switchLanguage(int langindex)
{
	if (langindex == _lastlangindex) return;
	// SET UP NEW TRANSLATOR
	QTranslator *translator_new = new QTranslator;
	// LOAD LANGUAGE:
	bool langfileerror = false;
	QString AppsPath( QCoreApplication::applicationDirPath() );
	if (langindex == 0)		// english
	{
		if ( translator_new->load("FreeSSM_en.qm", AppsPath) )
			_language_current = "en";
		else
			langfileerror = true;
	}
	else if (langindex == 1)	// german
	{
		if ( translator_new->load("FreeSSM_de.qm", AppsPath) )
			_language_current = "de";
		else
			langfileerror = true;
	}
	if (langfileerror)
	{
		// DELETE NEW TRANSLATOR AND USE OLD LANGUAGE AGAIN:
		delete translator_new;	
		language_comboBox->setCurrentIndex(_lastlangindex);
		QMessageBox msg( QMessageBox::Critical, tr("Error"), tr("Error:\n- Language file missing or damaged -"), QMessageBox::Ok, this);
		QFont msgfont = msg.font();
		msgfont.setPixelSize(12); // 9pts
		msg.setFont( msgfont );
		msg.show();
		msg.exec();
		msg.close();
		return;
	}
	else	 // IF NEW LANGUAGE HAS BEEN LOADED SUCCESSFULLY:
	{
		_lastlangindex = langindex;
		// Send new language settings to FreeSSM and retranslate FreeSSM-Window:
		emit languageSelChanged(_language_current, translator_new);
		// Retranslate window content:
		language_comboBox->clear();	// NECESSARY SINCE Qt 4.4.0 (WORKAROUND)
		retranslateUi(this);
		language_comboBox->setCurrentIndex(langindex);
	}
}


void Preferences::switchGUIstyle(QString style)
{
	QStyle *qstyle = NULL;
	if (style.size())
	{
		qstyle = QStyleFactory::create( style );
		if (qstyle)
			QApplication::setStyle( qstyle );
	}
}


void Preferences::selectSerialPort(QString portname)
{
	_newportname = portname;
}


void Preferences::interfacetest()
{
	QMessageBox *msgbox;
	FSSM_WaitMsgBox *waitmsgbox = NULL;
	QFont msgboxfont;
	// PREPARE SERIAL INTERCAE:
	serialCOM *serialport = new serialCOM;
	// Open port:
	if (serialport->OpenPort(_newportname.toStdString()))
	{
		// Configure port:
		serialCOM::dt_portsettings newportsettings;
		newportsettings.baudrate = 4800;
		newportsettings.databits = 8;
		newportsettings.parity = 'N';
		newportsettings.stopbits = 1;
		if (!serialport->SetPortSettings(newportsettings))
		{
			msgbox = new QMessageBox( QMessageBox::Critical, tr("Error"), tr("Couldn't configure serial port !"), QMessageBox::Ok, this);
			msgboxfont = msgbox->font();
			msgboxfont.setPixelSize(12); // 9pts
			msgbox->setFont( msgboxfont );
			msgbox->show();
			msgbox->exec();
			msgbox->close();
			delete msgbox;
			serialport->ClosePort();
			delete serialport;
			return;
		}
	}
	else	// If port couldn't be opened
	{
		msgbox = new QMessageBox( QMessageBox::Critical, tr("Error"), tr("Couldn't open serial port !\nMaybe port is already in use by another application..."), QMessageBox::Ok, this);
		msgboxfont = msgbox->font();
		msgboxfont.setPixelSize(12); // 9pts
		msgbox->setFont( msgboxfont );
		msgbox->show();
		msgbox->exec();
		msgbox->close();
		delete msgbox;
		delete serialport;
		return;
	}
	// SETUP SSMPcommunication object:
	SSMPcommunication *SSMPcom = new SSMPcommunication(serialport, 0x10);
	// DISPLAY INFO MESSAGE:
	int choice = QMessageBox::NoButton;
	msgbox = new QMessageBox( QMessageBox::Information, tr("Interface test"), tr("Please connect diagnostic interface to the vehicles" "\n" "OBD-Connector and switch ignition on."), QMessageBox::NoButton, this);
	msgbox->addButton(tr("Start"), QMessageBox::AcceptRole);
	msgbox->addButton(tr("Cancel"), QMessageBox::RejectRole);
	msgboxfont = msgbox->font();
	msgboxfont.setPixelSize(12); // 9pts
	msgbox->setFont( msgboxfont );
	msgbox->show();
	choice = msgbox->exec();
	msgbox->close();
	delete msgbox;
	if (choice == QMessageBox::AcceptRole)
	{
		// START INTERFACE-TEST:
		bool icresult = false;
		bool retry=true;
		choice=0;
		while ((retry==true) & (icresult==false))	// TEST LOOP
		{
			// OUTPUT WAIT MESSAGE:
			waitmsgbox = new FSSM_WaitMsgBox(this, tr("Testing Interface... Please wait !     "));
			waitmsgbox->show();
			// QUERY ANY ECU ADDRESS:
			unsigned int addr = 0x61;
			char data = 0;
			icresult = SSMPcom->readMultipleDatabytes('\x0', &addr, 1, &data);
			// CLOSE WAIT MESSAGE:
			waitmsgbox->close();
			delete waitmsgbox;
			// OUTPUT TEST RESULT:
			if (icresult == false)	// IF TEST FAILED
			{
				// ERROR DIALOG:
				msgbox = new QMessageBox( QMessageBox::Critical, tr("Interface test"), tr("Interface test failed !"), QMessageBox::NoButton, this);
				msgbox->addButton(tr("Retry"), QMessageBox::AcceptRole);
				msgbox->addButton(tr("Cancel"), QMessageBox::RejectRole);
				msgboxfont = msgbox->font();
				msgboxfont.setPixelSize(12); // 9pts
				msgbox->setFont( msgboxfont );
				msgbox->show();
				choice = msgbox->exec();
				msgbox->close();
				delete msgbox;
				if (choice != QMessageBox::AcceptRole)
					retry=false;
			}
			else	// IF TEST WAS SUCCESSFUL
			{
				// REPORT SUCCESS:
				msgbox = new QMessageBox( QMessageBox::Information, tr("Interface test"), tr("Interface test successful !"), QMessageBox::Ok, this);
				msgboxfont = msgbox->font();
				msgboxfont.setPixelSize(12); // 9pts
				msgbox->setFont( msgboxfont );
				msgbox->show();
				choice = msgbox->exec();
				msgbox->close();
				delete msgbox;
			}
		}
		// COMMUNICATION AND PORT OBJECT:
		delete SSMPcom;
	}
	// CLOSE SERIAL PORT
	if (!serialport->ClosePort())
	{
		msgbox = new QMessageBox( QMessageBox::Critical, tr("Error"), tr("Error while closing serial port !"), QMessageBox::Ok, this);
		msgboxfont = msgbox->font();
		msgboxfont.setPixelSize(12); // 9pts
		msgbox->setFont( msgboxfont );
		msgbox->show();
		msgbox->exec();
		msgbox->close();
		delete msgbox;
	}
	delete serialport;
}


void Preferences::ok()
{
	// RETURN CURRENT PORTNAME:
	*_r_portname = _newportname;
	// SAVE PREFERENCES TO FILE:
	QFile prefsfile(QDir::homePath() + "/FreeSSM.prefs");
	if (prefsfile.open(QIODevice::WriteOnly | QIODevice::Text))	// try to open/built preferences file
	{
		QString stylename = QApplication::style()->objectName();
		// rewrite file completly:
		prefsfile.write(_newportname.toAscii()+"\n", _newportname.length()+1);			// save portname
		prefsfile.write(_language_current.toAscii()+"\n", _language_current.length()+1);	// save language
		prefsfile.write(stylename.toAscii()+"\n", stylename.length()+1);	// save preferred GUI-style
		prefsfile.close();	// close file
	}
	else
	{
		QMessageBox msg( QMessageBox::Warning, tr("Error"), tr("Couldn't save preferences to file !\nTo prevent this failure in the future, ensure write access\nto your home directory and file ''FreeSSM.prefs''."), QMessageBox::Ok, this);
		QFont msgfont = msg.font();
		msgfont.setPixelSize(12); // 9pts
		msg.setFont( msgfont );
		msg.show();
		msg.exec();
		msg.close();
	}
	_confirmed = true;	// IMPORTANT: prevents undo of all chnges in destructor
	close();		// close window (delete is called automaticly)
}


void Preferences::cancel()
{
	_confirmed = false;	// IMPORTANT: cause destructor to undo all changes
	close();		// close window (delete is called automaticly)
}


void Preferences::closeEvent(QCloseEvent *event)
{
	if (!_confirmed)
	{
		// Switch back to old translation:
		if (_language_old == "en")
		{
			switchLanguage(0);
		}
		else if (_language_old == "de")
		{
			switchLanguage(1);
		}
		// Switch back to old GUI-style:
		switchGUIstyle( _style_old );
	}
	event->accept();
}


void Preferences::setupUiFonts()
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
	font = ok_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	ok_pushButton->setFont(font);
	font = cancel_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	cancel_pushButton->setFont(font);
	font = testinterface_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	testinterface_pushButton->setFont(font);
	font = language_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	language_label->setFont(font);
	font = serialport_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	guistyle_label->setFont(font);
	font = serialport_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	serialport_label->setFont(font);
	font = language_comboBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	language_comboBox->setFont(font);
	font = guistyle_comboBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	guistyle_comboBox->setFont(font);
	font = serialport_comboBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	serialport_comboBox->setFont(font);
}

