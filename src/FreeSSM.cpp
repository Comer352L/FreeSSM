/*
 * FreeSSM.cpp - Program main window
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

#include "FreeSSM.h"



FreeSSM::FreeSSM(QApplication *app)
{
	_qt_translator = NULL;
	_translator = NULL;
	_iface_type = AbstractDiagInterface::interface_serialPassThrough;
	_iface_filename = "";
	_language = "en";	// default language
	_dumping = false;
	QString appsPath( QCoreApplication::applicationDirPath() );
	// SETUP GUI:
	setupUi(this);
	setWindowFlags( windowFlags() & ~Qt::WindowMaximizeButtonHint );	// only necessary for MS Windows
#ifndef SMALL_RESOLUTION
	// LOAD BACKGROUND PICTURE:
	background_label->setPixmap(appsPath + "/background.png");
	// SHOW PROGRAM TITEL + VERSION:
	QFont titlefont = this->font();
	titlefont.setPointSize(20);
	titlefont.setBold(true);
	_progtitle_label = new QLabel(this);
	_progtitle_label->setGeometry(20, 17, 315, 34);
	_progtitle_label->setFont( titlefont );
	_progtitle_label->setText("FreeSSM " + QApplication::applicationVersion());
	this->setWindowTitle("FreeSSM " + QApplication::applicationVersion());
	// PLACE WINDOW IN THE CENTER OF THE SCREEN:
	QDesktopWidget desktop;
	int x = (desktop.width() - size().width()) / 2;
	int y = (desktop.height() - size().height()) / 2 - 50;
	this->move ( x, y );
#else
	showFullScreen();
#endif
	// LOAD PREFERENCES FROM FILE:
	QString savedinterfacefilename = "";
	QString savedlanguage = "";
	QString savedGUIstyle = "";
	QString savedinterfacetype = "";
	QFile prefsfile(QDir::homePath() + "/FreeSSM.prefs");
	if (prefsfile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QByteArray line;
		if (!prefsfile.atEnd())
		{
			// Load interface type settings:
			line = prefsfile.readLine();
			line.truncate(line.length()-1);	// truncate newline-character
			savedinterfacefilename = static_cast<QString>(line);
		}
		if (!prefsfile.atEnd())
		{
			// Load language settings:
			line = prefsfile.readLine();
			line.truncate(line.length()-1);
			savedlanguage = static_cast<QString>(line);
		}
		if (!prefsfile.atEnd())
		{
			// Load GUI-style settings:
			line = prefsfile.readLine();
			line.truncate(line.length()-1);
			savedGUIstyle = static_cast<QString>(line);
		}
		if (!prefsfile.atEnd())
		{
			// Load interface file name settings:
			line = prefsfile.readLine();
			line.truncate(line.length()-1);
			savedinterfacetype = static_cast<QString>(line);
		}
		prefsfile.close();
	}
	// SET PREFERRED GUI-Style:
	if (savedGUIstyle.size())
	{
		QStyle *qstyle = QStyleFactory::create( savedGUIstyle );
		if (qstyle)
			QApplication::setStyle( qstyle );
	}
	// CHECK SAVED LANGUAGE:
	bool sl_valid = false;
	QLocale loc = QLocale(savedlanguage);
	if ((loc != QLocale::C) && (__supportedLocales.indexOf( loc ) > -1))
	{
		_language = savedlanguage;
		sl_valid = true;
	}
	// TRY TO SELECT SYSTEM LANGUAGE, IF SAVED LANGUAGE IS INVALID:
	if (!sl_valid)
	{
		if (__supportedLocales.indexOf( QLocale::system() ) > -1)
			_language = QLocale::system().name().section('_', 0, 0);
	}
	// SET TRANSLATOR AND RETRANSLATE:
	_translator = new QTranslator;
	bool langfileerror = false;
	langfileerror = !_translator->load("FreeSSM_" + _language + ".qm", appsPath);
	if (langfileerror && (_language != "en"))
	{
		// Fallback to English
		_language = "en";
		langfileerror = !_translator->load("FreeSSM_en.qm", appsPath);
		// Display error message about missing language file:
		QMessageBox msg( QMessageBox::Critical, tr("Error"), tr("Error:\n- Language file missing or damaged -"), QMessageBox::Ok, this);
		QFont msgfont = msg.font();
		msgfont.setPointSize(9);
		msg.setFont( msgfont );
		msg.show();
		msg.exec();
		msg.close();
	}
	if (!langfileerror)
	{
		app->installTranslator(_translator);
		retranslateUi(this);
	}
	else
	{
		delete _translator;
		_translator = NULL;
	}
	// SET Qt-TRANSLATOR (if necessary and available):
	if (_language != "en")
	{
		QString qt_ts_path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
		if (qt_ts_path.isEmpty())
			qt_ts_path = QCoreApplication::applicationDirPath();
		_qt_translator = new QTranslator;
		if (_qt_translator->load("qt_" + _language, qt_ts_path))
			app->installTranslator(_qt_translator);
		else
		{
			delete _qt_translator;
			_qt_translator = NULL;
		}
	}
	// CHECK THE SAVED INTERFACE SETTINGS AND CORRECT IF NECESSARY:
	if (savedinterfacetype == QString::number(AbstractDiagInterface::interface_J2534))	// J2534-Pass-Through
	{
		_iface_type = AbstractDiagInterface::interface_J2534;
		const auto J2534libs = J2534_API::getAvailableJ2534Libs();
		if (J2534libs.size())
		{
			if (!savedinterfacefilename.isEmpty())
			{
				for (const auto& lib : J2534libs)
				{
					if (savedinterfacefilename == QString::fromStdString(lib.path))
					{
						_iface_filename = savedinterfacefilename;
						break;
					}
				}
			}
			if (_iface_filename.isEmpty())
				_iface_filename = QString::fromStdString(J2534libs.at(0).path);
		}
		// NOTE: otherwise _iface_filename remains empty
	}
	else	// Serial Pass-Through, AT-comand controlled (e.g. ELM, AGV, Diamex) or invalid
	{
		if (savedinterfacetype == QString::number(AbstractDiagInterface::interface_ATcommandControlled))
			_iface_type = AbstractDiagInterface::interface_ATcommandControlled;
		else
			_iface_type = AbstractDiagInterface::interface_serialPassThrough;
		std::vector<std::string> portlist;
		portlist = serialCOM::GetAvailablePorts();
		if (portlist.size())
		{
			if (!savedinterfacetype.isEmpty() && !savedinterfacefilename.isEmpty())
			{
				for (unsigned int k=0; k<portlist.size(); k++)
				{
					if (savedinterfacefilename == QString::fromStdString(portlist.at(k)))
					{
						_iface_filename = savedinterfacefilename;
						break;
					}
				}
			}
			if (_iface_filename.isEmpty())
				_iface_filename = QString::fromStdString(portlist.at(0));
		}
		// NOTE: otherwise _iface_filename remains empty
	}
	// CREATE ACTION FOR DUMPING CONTROL UNIT ID-DATA TO FILE:
	_dump_action = new QAction(this);
	_dump_action->setShortcut( QKeySequence("Ctrl+Alt+Return") );
	this->addAction(_dump_action);
	// CONNECT SIGNALS/SLOTS:
	connect( engine_pushButton, SIGNAL( released() ), this, SLOT( engine() ) );
	connect( transmission_pushButton, SIGNAL( released() ), this, SLOT( transmission() ) );
	connect( absvdc_pushButton, SIGNAL( released() ), this, SLOT( abs() ) );
	connect( cruisecontrol_pushButton, SIGNAL( released() ), this, SLOT( cruisecontrol() ) );
	connect( aircon_pushButton, SIGNAL( released() ), this, SLOT( aircon() ) );
	connect( preferences_pushButton, SIGNAL( released() ), this, SLOT( preferences() ) );
	connect( help_pushButton, SIGNAL( released() ), this, SLOT( help() ) );
	connect( about_pushButton, SIGNAL( released() ), this, SLOT( about() ) );
	connect( exit_pushButton, SIGNAL( released() ), this, SLOT( close() ) );
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
	connect( _dump_action, SIGNAL(triggered()), this, SLOT(dumpCUdata()) );
}


FreeSSM::~FreeSSM()
{
	disconnect( _dump_action, SIGNAL( triggered() ), this, SLOT( dumpCUdata() ) );
	disconnect( engine_pushButton, SIGNAL( released() ), this, SLOT( engine() ) );
	disconnect( transmission_pushButton, SIGNAL( released() ), this, SLOT( transmission() ) );
	disconnect( absvdc_pushButton, SIGNAL( released() ), this, SLOT( abs() ) );
	disconnect( cruisecontrol_pushButton, SIGNAL( released() ), this, SLOT( cruisecontrol() ) );
	disconnect( aircon_pushButton, SIGNAL( released() ), this, SLOT( aircon() ) );
	disconnect( preferences_pushButton, SIGNAL( released() ), this, SLOT( preferences() ) );
	disconnect( help_pushButton, SIGNAL( released() ), this, SLOT( help() ) );
	disconnect( about_pushButton, SIGNAL( released() ), this, SLOT( about() ) );
	disconnect( exit_pushButton, SIGNAL( released() ), this, SLOT( close() ) );
	delete _dump_action;
#ifndef SMALL_RESOLUTION
	delete _progtitle_label;
#endif
	if (_translator != NULL)
	{
		QApplication::removeTranslator(_translator);
		delete _translator;
	}
	if (_qt_translator != NULL)
	{
		QApplication::removeTranslator(_qt_translator);
		delete _qt_translator;
	}
}


void FreeSSM::engine()
{
	if (_dumping) return;
	AbstractDiagInterface *diagInterface = initInterface();
	if (diagInterface)
	{
		EngineDialog *enginedialog = new EngineDialog(diagInterface, _language);
		if (!enginedialog->isHidden())
			enginedialog->exec();
		delete enginedialog;
		delete diagInterface;
	}
}


void FreeSSM::transmission()
{
	if (_dumping) return;
	AbstractDiagInterface *diagInterface = initInterface();
	if (diagInterface)
	{
		TransmissionDialog *transmissiondialog = new TransmissionDialog(diagInterface, _language);
		if (!transmissiondialog->isHidden())
			transmissiondialog->exec();
		delete transmissiondialog;
		delete diagInterface;
	}
}

void FreeSSM::abs()
{
	if (_dumping) return;
	AbstractDiagInterface *diagInterface = initInterface();
	if (diagInterface)
	{
		ABSdialog *absdialog = new ABSdialog(diagInterface, _language);
		if (!absdialog->isHidden())
			absdialog->exec();
		delete absdialog;
		delete diagInterface;
	}
}

void FreeSSM::cruisecontrol()
{
	if (_dumping) return;
	AbstractDiagInterface *diagInterface = initInterface();
	if (diagInterface)
	{
		CruiseControlDialog *cruisecontroldialog = new CruiseControlDialog(diagInterface, _language);
		if (!cruisecontroldialog->isHidden())
			cruisecontroldialog->exec();
		delete cruisecontroldialog;
		delete diagInterface;
	}
}

void FreeSSM::aircon()
{
	if (_dumping) return;
	AbstractDiagInterface *diagInterface = initInterface();
	if (diagInterface)
	{
		AirConDialog *aircondialog = new AirConDialog(diagInterface, _language);
		if (!aircondialog->isHidden())
			aircondialog->exec();
		delete aircondialog;
		delete diagInterface;
	}
}



void FreeSSM::preferences()
{
	if (_dumping) return;
	Preferences *preferencesdlg = new Preferences(this, &_iface_type, &_iface_filename, _language);
	connect(preferencesdlg, SIGNAL( languageSelChanged(QString, QTranslator*) ),
					this, SLOT( retranslate(QString, QTranslator*) ));
	preferencesdlg->exec();
	delete preferencesdlg;
}


void FreeSSM::help()
{
	QString helpurlstr( QCoreApplication::applicationDirPath() );
	helpurlstr += "/doc/help_" + _language + ".html";
	QDesktopServices::openUrl( QUrl(helpurlstr) );
}


void FreeSSM::about()
{
	About *aboutdlg = new About(this, _language);
	aboutdlg->show();
	aboutdlg->exec();
	delete aboutdlg;
}


AbstractDiagInterface * FreeSSM::initInterface()
{
	// Check if an interface is selected:
	if (_iface_filename == "")
	{
		displayErrorMsg(tr("No interface selected !\n=> Please select a dignostic interface in the preferences."));
		return NULL;
	}
	// Open interface:
	AbstractDiagInterface *diagInterface = NULL;
	if (_iface_type == AbstractDiagInterface::interface_serialPassThrough)
	{
		diagInterface = new SerialPassThroughDiagInterface;
	}
	else if (_iface_type == AbstractDiagInterface::interface_J2534)
	{
		diagInterface = new J2534DiagInterface;
	}
	else if (_iface_type == AbstractDiagInterface::interface_ATcommandControlled)
	{
		diagInterface = new ATcommandControlledDiagInterface;
	}
	else
	{
		displayErrorMsg(tr("Internal error:\nThe selected interface type cannot be initialized !\n=> Please report this as a bug."));
		return NULL;
	}
	if (diagInterface->open(_iface_filename.toStdString()))
		return diagInterface;
	// Return error:
	displayErrorMsg(tr("Couldn't open the diagnostic interface !\nMaybe the device is already in use by another application..."));
	delete diagInterface;
	return NULL;
}


void FreeSSM::retranslate(QString newlanguage, QTranslator *newtranslator)
{
	// Uninstall and delete current translator:
	if (_translator != NULL)
	{
		QApplication::removeTranslator(_translator);
		delete _translator;
		_translator = NULL;
	}
	// Save new language settings:
	_language = newlanguage;
	_translator = newtranslator;
	// Install new translator and retranslate:
	QApplication::installTranslator(_translator);
	retranslateUi(this);
	// Change translator for Qt-dialogs:
	if (_qt_translator != NULL)
	{
		QApplication::removeTranslator(_qt_translator);
		delete _qt_translator;
		_qt_translator = NULL;
	}
	if (newlanguage != "en")
	{
		QString qt_ts_path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
		if (qt_ts_path.isEmpty())
			qt_ts_path = QCoreApplication::applicationDirPath();
		_qt_translator = new QTranslator;
		if (_qt_translator->load("qt_" + newlanguage, qt_ts_path))
			QApplication::installTranslator(_qt_translator);
		else
		{
			delete _qt_translator;
			_qt_translator = NULL;
		}
	}
}


void FreeSSM::dumpCUdata()
{
	const size_t VINlength = 17;

	QFile dumpfile;
	SSMCUdata ssmCUdata;
	std::string hexstr = "";

	unsigned int dataaddr[VINlength] = {0};
	char data[VINlength] = {0};
	int ssm1_cu_index = SSM1_CU_Engine;
	bool cu_resp = false;

	if (_dumping) return;
	// Initialize and configure serial port:
	AbstractDiagInterface *diagInterface = initInterface();
	if (!diagInterface)
		return;
	_dumping = true;
	// Set filename:
	QString filename(QDir::homePath() + "/dump");
	if (QFile::exists(filename + ".dat"))
	{
		for (unsigned int f=2; f<65535; f++)
		{
			if (!QFile::exists(filename + QString::number(f) + ".dat"))
			{
				filename.append(QString::number(f));
				break;
			}
		}
	}
	filename.append(".dat");
	dumpfile.setFileName(filename);
	if (!dumpfile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		delete diagInterface;	// will be closed in destructor
		_dumping = false;
		return;
	}
	// Create SSMP1-Communication-object:
	SSMP1communication SSMP1com(diagInterface, SSM1_CU_Engine, 0);
	// Create SSMP2-Communication-object:
	SSMP2communication SSMP2com(diagInterface, 0x10, 0);
	// ######## SSM2-Control-Units ########
	// **************** ECU ***************
	// Read ECU data:
	if (diagInterface->connect(AbstractDiagInterface::protocol_SSM2_ISO14230))
	{
		cu_resp = true;
		if (!SSMP2com.getCUdata(ssmCUdata))
		{
			SSMP2com.setCUaddress(0x01);
			if (!SSMP2com.getCUdata(ssmCUdata))
			{
				SSMP2com.setCUaddress(0x02);
				cu_resp = SSMP2com.getCUdata(ssmCUdata);
			}
		}
	}
	if (!cu_resp)
	{
		diagInterface->disconnect();
		if (diagInterface->connect(AbstractDiagInterface::protocol_SSM2_ISO15765))
		{
			SSMP2com.setCUaddress(0x7E0);
			cu_resp = SSMP2com.getCUdata(ssmCUdata);
		}
	}
	if (cu_resp)
	{
		dumpfile.write(libFSSM::StrToMultiLineHexstr(ssmCUdata.SYS_ID).c_str());
		dumpfile.write(libFSSM::StrToMultiLineHexstr(ssmCUdata.ROM_ID).c_str());
		dumpfile.write(libFSSM::StrToMultiLineHexstr(ssmCUdata.flagbytes).c_str());
		dumpfile.write("\n");
		// VIN:
		if (ssmCUdata.flagbytebit(36, 0))
		{
			dataaddr[0] = 0xDA;
			dataaddr[1] = 0xDB;
			dataaddr[2] = 0xDC;
			if (SSMP2com.readMultipleDatabytes(0x0, dataaddr, 3, data))
			{
				hexstr = libFSSM::StrToHexstr(data, 3);
				hexstr.push_back('\n');
				dumpfile.write(hexstr.data(), hexstr.length());

				dataaddr[0] = libFSSM::parseUInt24BigEndian(data);
				for (unsigned char k=1; k<VINlength; k++)
					dataaddr[k] = dataaddr[0]+k;
				if (SSMP2com.readMultipleDatabytes(0x0, dataaddr, VINlength, data))
				{
					hexstr = libFSSM::StrToMultiLineHexstr(data, VINlength, VINlength);
					hexstr.append(data, VINlength);
					hexstr += "\n\n";
					dumpfile.write(hexstr.data(), hexstr.size());
				}
			}
		}
	}
	else
		dumpfile.write("\n---\n", 5);
	// **************** TCU ***************
	// Read TCU data:
	cu_resp = false;
	diagInterface->disconnect();
	if (diagInterface->connect(AbstractDiagInterface::protocol_SSM2_ISO14230))
	{
		SSMP2com.setCUaddress(0x18);
		cu_resp = true;
		if (!SSMP2com.getCUdata(ssmCUdata))
		{
			SSMP2com.setCUaddress(0x01);
			cu_resp = SSMP2com.getCUdata(ssmCUdata);
		}
	}
	if (!cu_resp)
	{
		diagInterface->disconnect();
		if (diagInterface->connect(AbstractDiagInterface::protocol_SSM2_ISO15765))
		{
			SSMP2com.setCUaddress(0x7E1);
			// Read TCU data:
			cu_resp = SSMP2com.getCUdata(ssmCUdata);
		}
	}
	if (cu_resp)
	{
		dumpfile.write(libFSSM::StrToMultiLineHexstr(ssmCUdata.SYS_ID).c_str());
		dumpfile.write(libFSSM::StrToMultiLineHexstr(ssmCUdata.ROM_ID).c_str());
		dumpfile.write(libFSSM::StrToMultiLineHexstr(ssmCUdata.flagbytes).c_str());
	}
	else
		dumpfile.write("\n---\n", 5);
	diagInterface->disconnect();

	// ######## SSM1-Control-Units ########
	// Configure interface:
	if (!diagInterface->connect(AbstractDiagInterface::protocol_SSM1))
		goto end;
	// Dump all SSM1-CUs:
	for (ssm1_cu_index=SSM1_CU_Engine; ssm1_cu_index<=SSM1_CU_PwrSteer; ssm1_cu_index++)
	{
		// Select CU:
		SSMP1com.selectCU( SSM1_CUtype_dt(ssm1_cu_index) );
		// Read CU-ID(s):
		if (SSMP1com.getCUdata(0, ssmCUdata))
		{
			dumpfile.write("\n");
			// *** Convert and write data to file:
			// ID:
			dumpfile.write(libFSSM::StrToMultiLineHexstr(ssmCUdata.SYS_ID).c_str());

			if ((ssmCUdata.SYS_ID.at(0) & '\xF0') == '\xA0')
			{
				// ROM-ID:
				if (ssmCUdata.SYS_ID.at(1) == '\x10')
				{
					std::vector<unsigned int> addresses;
					for (unsigned int addr=0x01; addr<=0x05; addr++)
						addresses.push_back(addr);
					if (!SSMP1com.readAddresses(addresses, &ssmCUdata.ROM_ID))
						dumpfile.write("error\n", 4);
					else
					{
						dumpfile.write(libFSSM::StrToMultiLineHexstr(ssmCUdata.ROM_ID).c_str());
					}
				}
				// Flagbytes:
				if (SSMP1com.getCUdata(32, ssmCUdata))
				{
					dumpfile.write(libFSSM::StrToMultiLineHexstr(ssmCUdata.flagbytes).c_str());
				}
				else
					dumpfile.write("error\n", 4);
			}
		}
		else
			dumpfile.write("\n-----\n", 7);
	}
	diagInterface->disconnect();

end:
	dumpfile.close();
	delete diagInterface;	// will be closed in destructor
	_dumping = false;
}


void FreeSSM::displayErrorMsg(QString errmsg)
{
	QMessageBox msg( QMessageBox::Critical, tr("Error"), errmsg, QMessageBox::Ok, this);
	QFont msgfont = msg.font();
	msgfont.setPointSize(9);
	msg.setFont( msgfont );
	msg.show();
	msg.exec();
	msg.close();
}


void FreeSSM::keyPressEvent(QKeyEvent *event)
{
	QWidget *widget = NULL;
	QPushButton *pushbutton = NULL;
	if ((event->key() == Qt::Key_Return) || (event->key() == Qt::Key_Enter))
	{
		widget = this->focusWidget();
		pushbutton = dynamic_cast<QPushButton*>(widget);
		if (pushbutton)
			pushbutton->animateClick();
	}
	else if (event->key() == Qt::Key_Escape)
	{
		close();
	}
	else
		QMainWindow::keyPressEvent(event);
}


void FreeSSM::closeEvent(QCloseEvent *event)
{
	if (!_dumping)
		event->accept();
	else
		event->ignore();
}

