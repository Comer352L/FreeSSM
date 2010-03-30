/*
 * Preferences.cpp - Adjustment of program settings
 *
 * Copyright (C) 2008-2010 Comer352l
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



Preferences::Preferences(QMainWindow *parent, AbstractDiagInterface::interface_type *ifacetype, QString *ifacefilename, QString language) : QDialog(parent)
{
	_newinterfacetype = *ifacetype;
	_r_interfacetype = ifacetype;
	_newinterfacefilename = *ifacefilename;
	_r_interfacefilename = ifacefilename;
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
	x = (FreeSSM_MW_Xpos + 60);
	y = (FreeSSM_MW_Ypos + 40);
	// move window to desired coordinates
	move(x, y);
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
		if (QLocale::system().language() == QLocale::German)
		{
			_language_current = "de";
			switchLanguage(1);
			_lastlangindex = 1;
		}
		else	// default
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
		int styleindex = supStyles.indexOf( QRegExp( _style_old, Qt::CaseInsensitive, QRegExp::FixedString ) );
		// NOTE: default pattern syntax QRegExp::RegExp(2) doesn't work for "gtk+"/"GTK+" !
		guistyle_comboBox->insertItems(0, supStyles);
		guistyle_comboBox->setCurrentIndex( styleindex );
	}
	else
		guistyle_comboBox->setEnabled( false );
	// INTERFACES:
	int if_name_index = -1;
	if (_newinterfacetype == AbstractDiagInterface::interface_J2534) // J2534-Pass-Through
	{
		interfaceType_comboBox->setCurrentIndex(1);
		selectInterfaceType(1); // NOTE: fills _J2534libraryPaths, changes _newinterfacefilename
		if_name_index = _J2534libraryPaths.indexOf(*_r_interfacefilename);
	}
	else	// Serial Pass-Through
	{
		interfaceType_comboBox->setCurrentIndex(0);
		selectInterfaceType(0); // NOTE: fills _J2534libraryPaths, changes _newinterfacefilename
		if_name_index = interfaceName_comboBox->findText(*_r_interfacefilename);
	}
	if (if_name_index >= 0)
	{
		interfaceName_comboBox->setCurrentIndex(if_name_index);
		selectInterfaceName(if_name_index);
	}
	else
	{
		if (interfaceName_comboBox->count() > 0)	// if min. 1 device available
		{
			interfaceName_comboBox->setCurrentIndex(0);
			selectInterfaceName(0);
			*_r_interfacefilename = _newinterfacefilename;
		}
		else
		{
			_newinterfacefilename = "";
			*_r_interfacefilename = "";
		}
	}
	// CONNECT SIGNALS AND SLOTS:
	connect( language_comboBox, SIGNAL( activated(int) ), this, SLOT( switchLanguage(int) ) );
	connect( guistyle_comboBox, SIGNAL( activated(QString) ), this, SLOT( switchGUIstyle(QString) ) );
	connect( interfaceType_comboBox, SIGNAL( activated(int) ), this, SLOT( selectInterfaceType(int) ) );
	connect( interfaceName_comboBox, SIGNAL( activated(int) ), this, SLOT( selectInterfaceName(int) ) );
	connect( testinterface_pushButton, SIGNAL( released() ), this, SLOT( interfacetest() ) );
	connect( ok_pushButton, SIGNAL( released() ), this, SLOT( ok() ) );
	connect( cancel_pushButton, SIGNAL( released() ), this, SLOT( cancel() ) );
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
}


Preferences::~Preferences()
{
	disconnect( language_comboBox, SIGNAL( activated(int) ), this, SLOT( switchLanguage(int) ) );
	disconnect( guistyle_comboBox, SIGNAL( activated(QString) ), this, SLOT( switchGUIstyle(QString) ) );
	disconnect( interfaceType_comboBox, SIGNAL( activated(int) ), this, SLOT( selectInterfaceType(int) ) );
	disconnect( interfaceName_comboBox, SIGNAL( activated(int) ), this, SLOT( selectInterfaceName(int) ) );
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


void Preferences::selectInterfaceType(int index)
{
	interfaceName_comboBox->clear();
	_J2534libraryPaths.clear();
	_newinterfacefilename = "";
	QStringList deviceNames;
	if (index == 0)		// Serial Pass-Through
	{
		_newinterfacetype = AbstractDiagInterface::interface_serialPassThrough;
		interfaceName_label->setText(tr("Serial Port:"));
		std::vector<std::string> portlist = serialCOM::GetAvailablePorts();
		for (unsigned int k=0; k<portlist.size(); k++)
			deviceNames.push_back(QString::fromStdString(portlist.at(k)));
	}
	else if (index == 1)	// J2534-Pass-Through
	{
		_newinterfacetype = AbstractDiagInterface::interface_J2534;
		interfaceName_label->setText(tr("Interface-Name:"));
		std::vector<J2534Library> J2534libs = J2534_API::getAvailableJ2534Libs();
		for (unsigned int k=0; k<J2534libs.size(); k++)
		{
			deviceNames.push_back(QString::fromStdString(J2534libs.at(k).name));
			_J2534libraryPaths.push_back(QString::fromStdString(J2534libs.at(k).path));
		}
	}
	if (deviceNames.size())
	{
		interfaceName_comboBox->addItems(deviceNames);
		interfaceName_comboBox->setCurrentIndex(0);
		selectInterfaceName(0);
	}
	interfaceName_comboBox->setEnabled(deviceNames.size());
	testinterface_pushButton->setEnabled(deviceNames.size());
	// NOTE: Never put anything else than a device-file into the interfaceName_comboBox !
}


void Preferences::selectInterfaceName(int index)
{
	if ((index < 0) || (index >= interfaceName_comboBox->count()))
		_newinterfacefilename = "";
	else
	{
		if (_newinterfacetype == AbstractDiagInterface::interface_J2534)
		{
			if (index < _J2534libraryPaths.size())
				_newinterfacefilename = _J2534libraryPaths.at(index);
			else	// should never happen	
				_newinterfacefilename = "";
		}
		else
			_newinterfacefilename = interfaceName_comboBox->currentText();
	}
}


void Preferences::interfacetest()
{
	QMessageBox *msgbox;
	FSSM_WaitMsgBox *waitmsgbox = NULL;
	QFont msgboxfont;
	// PREPARE INTERFACE:
	AbstractDiagInterface *diagInterface = NULL;
	if (_newinterfacetype == AbstractDiagInterface::interface_serialPassThrough)
	{
		diagInterface = new SerialPassThroughDiagInterface;
	}
	else if (_newinterfacetype == AbstractDiagInterface::interface_J2534)
	{
		diagInterface = new J2534DiagInterface;
	}
	else
	{
		displayErrorMsg(tr("The selected interface is not supported !"));
		displayErrorMsg(tr("Internal error:\nThe interface test for the selected interface is not yet implemented.\n=> Please report this as a bug."));
		return;
	}
	// OPEN INTERFACE:
	if (diagInterface->open(_newinterfacefilename.toStdString()))
	{
		if (!diagInterface->connect(AbstractDiagInterface::protocol_SSM2))
		{
			displayErrorMsg(tr("Couldn't configure the diagnostic interface !"));
			diagInterface->close();
			delete diagInterface;
			return;
		}
	}
	else
	{
		displayErrorMsg(tr("Couldn't open the diagnostic interface !\nMaybe the device is already in use by another application..."));
		delete diagInterface;
		return;
	}
	// SETUP SSMPcommunication object:
	SSMP2communication *SSMP2com = new SSMP2communication(diagInterface, 0x10);
	// DISPLAY INFO MESSAGE:
	int choice = QMessageBox::NoButton;
	msgbox = new QMessageBox( QMessageBox::Information, tr("Interface test"), tr("Please connect diagnostic interface to the vehicles\nOBD-connector and switch ignition on."), QMessageBox::NoButton, this);
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
			waitmsgbox = new FSSM_WaitMsgBox(this, tr("Testing interface... Please wait !     "));
			waitmsgbox->show();
			// QUERY ANY ECU ADDRESS:
			unsigned int addr = 0x61;
			char data = 0;
			icresult = SSMP2com->readMultipleDatabytes('\x0', &addr, 1, &data);
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
		delete SSMP2com;
	}
	// CLOSE INTERFACE:
	if (!diagInterface->close())
		displayErrorMsg(tr("Couldn't close the diagnostic interface !"));
	delete diagInterface;
}


void Preferences::ok()
{
	// RETURN CURRENT INTERFACE-TYPE AND -NAME:
	*_r_interfacetype = _newinterfacetype;
	*_r_interfacefilename = _newinterfacefilename;
	// SAVE PREFERENCES TO FILE:
	QFile prefsfile(QDir::homePath() + "/FreeSSM.prefs");
	if (prefsfile.open(QIODevice::WriteOnly | QIODevice::Text))	// try to open/built preferences file
	{
		QString stylename = QApplication::style()->objectName();
		// rewrite file completely:
		prefsfile.write(_newinterfacefilename.toAscii()+"\n", _newinterfacefilename.length()+1);	// save interface name
		prefsfile.write(_language_current.toAscii()+"\n", _language_current.length()+1);	// save language
		prefsfile.write(stylename.toAscii()+"\n", stylename.length()+1);			// save preferred GUI-style
		prefsfile.write(QString::number(_newinterfacetype).toAscii()+"\n", QString::number(_newinterfacetype).size()+1); // save interface type
		prefsfile.close();
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


void Preferences::displayErrorMsg(QString errormsg)
{
	QMessageBox *msgbox;
	QFont msgboxfont;
	msgbox = new QMessageBox( QMessageBox::Critical, tr("Error"), errormsg, QMessageBox::Ok, this);
	msgboxfont = msgbox->font();
	msgboxfont.setPixelSize(12); // 9pts
	msgbox->setFont( msgboxfont );
	msgbox->show();
	msgbox->exec();
	msgbox->close();
	delete msgbox;
}


void Preferences::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS OF ui_Preferences.h (made with QDesigner)
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
	font = guistyle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	guistyle_label->setFont(font);
	font = interfaceType_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	interfaceType_label->setFont(font);
	font = interfaceName_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	interfaceName_label->setFont(font);
	font = language_comboBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	language_comboBox->setFont(font);
	font = guistyle_comboBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	guistyle_comboBox->setFont(font);
	font = interfaceType_comboBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	interfaceType_comboBox->setFont(font);
	font = interfaceName_comboBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	interfaceName_comboBox->setFont(font);
}

