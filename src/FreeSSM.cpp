/*
 * FreeSSM.cpp - Program main window
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

#include "FreeSSM.h"



FreeSSM::FreeSSM(QApplication *app)
{
	_qt_translator = NULL;
	_translator = NULL;
	_portname = "";
	_language = "";
	_dumping = false;
	QString appsPath( QCoreApplication::applicationDirPath() );
	// SETUP GUI:
	setupUi(this);
	setWindowFlags( windowFlags() & ~Qt::WindowMaximizeButtonHint );	// only necessary for MS Windows
	setupUiFonts();
	// LOAD BACKGROUND PICTURE:
	background_label->setPixmap(appsPath + "/background.png");
	// SHOW PROGRAM TITEL + VERSION:
	QFont titlefont = this->font();
	titlefont.setPixelSize(27); // 20pts
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
	// LOAD PREFERENCES FROM FILE:
	QString savedportname = "";
	QString savedlanguage = "";
	QString savedGUIstyle = "";
	QFile prefsfile(QDir::homePath() + "/FreeSSM.prefs");
	if (prefsfile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QByteArray line;
		if (!prefsfile.atEnd())
		{
			// Load port settings:
			line = prefsfile.readLine();
			line.truncate(line.length()-1);	// truncate newline-character
			savedportname = static_cast<QString>(line);
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
			// Load language settings:
			line = prefsfile.readLine();
			line.truncate(line.length()-1);
			savedGUIstyle = static_cast<QString>(line);
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
	// CHECK SAVED LANGUAGE AND CORRECT IF NECESSARY:
	if ((savedlanguage == "en") | (savedlanguage == "de"))
		_language = savedlanguage;
	else
	{
		// TRY TO GET SYSTEM LANGUAGE:
		if ((QLocale::system().language() == QLocale::English))
		{
			_language = "en";
		}
		else if ((QLocale::system().language() == QLocale::German))
		{
			_language = "de";
		}
		else	// if system language is not available/supported
		{
			_language = "en";
		}
	}
	// SET TRANSLATOR AND RETRANSLATE:
	_translator = new QTranslator;
	bool langfileerror = false;
	if (_language == "de")
	{
		langfileerror = !_translator->load("FreeSSM_de.qm", appsPath);
	}
	if ((_language == "en") | (langfileerror))
	{
		_language = "en";
		langfileerror = !_translator->load("FreeSSM_en.qm", appsPath);
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
		QMessageBox msg( QMessageBox::Critical, tr("Error"), tr("Error:\n- Language file missing or damaged -"), QMessageBox::Ok, this);
		QFont msgfont = msg.font();
		msgfont.setPixelSize(12); // 9pts
		msg.setFont( msgfont );
		msg.show();
		msg.exec();
		msg.close();
	}
	// SET Qt-TRANSLATOR (if necessary and available):
	QString qt_ts_path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
	if (qt_ts_path.isEmpty())
		qt_ts_path = QCoreApplication::applicationDirPath();
	if (_language == "de")
	{
		_qt_translator = new QTranslator;
		if (_qt_translator->load("qt_de_DE", qt_ts_path))
			app->installTranslator(_qt_translator);
		else
		{
			delete _qt_translator;
			_qt_translator = NULL;
		}
	}
	// GET AVAILABLE PORTS:
	std::vector<std::string> portlist;
	portlist = serialCOM::GetAvailablePorts();
	// CHECK SAVED PREFERENCES AND CORRECT IF NECESSARY:
	bool pnvalid = false;
	for (unsigned int k=0; k<portlist.size(); k++)
	{
		if (savedportname == portlist[k].c_str())
		{
			pnvalid = true;
			break;
		}
	}
	if (pnvalid == true)
		_portname = savedportname;
	else
	{
		if (portlist.size()>0)
			_portname = (QString)portlist[0].c_str();
		// otherwise portname remains empty
	}
	// CREATE ACTION FOR DUMPING CONTROL UNID ID-DATA TO FILE:
	_dump_action = new QAction(this);
	_dump_action->setShortcut( QKeySequence("Ctrl+Alt+Return") );
	this->addAction(_dump_action);
	// CONNECT SIGNALS/SLOTS:
	connect( engine_pushButton, SIGNAL( released() ), this, SLOT( engine() ) );
	connect( transmission_pushButton, SIGNAL( released() ), this, SLOT( transmission() ) );
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
	disconnect( preferences_pushButton, SIGNAL( released() ), this, SLOT( preferences() ) ); 
	disconnect( help_pushButton, SIGNAL( released() ), this, SLOT( help() ) ); 
	disconnect( about_pushButton, SIGNAL( released() ), this, SLOT( about() ) ); 
	disconnect( exit_pushButton, SIGNAL( released() ), this, SLOT( close() ) );
	delete _dump_action;
	delete _progtitle_label;
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
	SerialPassThroughDiagInterface *diagInterface = initInterface();
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
	SerialPassThroughDiagInterface *diagInterface = initInterface();
	if (diagInterface)
	{
		TransmissionDialog *transmissiondialog = new TransmissionDialog(diagInterface, _language);
		if (!transmissiondialog->isHidden())
			transmissiondialog->exec();
		delete transmissiondialog;
		delete diagInterface;
	}
}


void FreeSSM::preferences()
{
	if (_dumping) return;
	Preferences *preferencesdlg = new Preferences(this, &_portname, _language);
	preferencesdlg->show();
	connect(preferencesdlg, SIGNAL( languageSelChanged(QString, QTranslator*) ),
					this, SLOT( retranslate(QString, QTranslator*) ));
	preferencesdlg->exec();
	delete preferencesdlg;
}


void FreeSSM::help()
{
	// Get working directory:
	QString helpurlstr( QCoreApplication::applicationDirPath() );
	// Select help file:
	if (_language == "de")
		helpurlstr += "/doc/help_de.html";
	else
		helpurlstr += "/doc/help_en.html";
	// Open help file in standard browswer:
	QDesktopServices::openUrl( QUrl(helpurlstr) );
}


void FreeSSM::about()
{
	About *aboutdlg = new About(this, _language);
	aboutdlg->show();
	aboutdlg->exec();
	delete aboutdlg;
}


SerialPassThroughDiagInterface * FreeSSM::initInterface()
{
	// IF NO PORT IS SELECTED YET: SELECT FIRST AVAILABLE PORT
	if (_portname == "")
	{
		// Get available ports:
		std::vector<std::string> portlist;
		portlist = serialCOM::GetAvailablePorts();
		if (portlist.size() > 0)
			_portname = (QString)portlist[0].c_str();
		else
		{
			QMessageBox msg( QMessageBox::Critical, tr("Error"),tr("No serial port available !"), QMessageBox::Ok, this);
			QFont msgfont = msg.font();
			msgfont.setPixelSize(12); // 9pts
			msg.setFont( msgfont );
			msg.show();
			msg.exec();
			msg.close();
			return false;
		}
	}
	// Open interface:
	SerialPassThroughDiagInterface *diagInterface = new SerialPassThroughDiagInterface;
	if (diagInterface->open(_portname.toStdString()))
		return diagInterface;
	// Return error:
	QMessageBox msg( QMessageBox::Critical, tr("Error"),tr("Couldn't open serial port !" "\n" "Maybe port is already in use by another application..."), QMessageBox::Ok, this);
	QFont msgfont = msg.font();
	msgfont.setPixelSize(12); // 9pts
	msg.setFont( msgfont );
	msg.show();
	msg.exec();
	msg.close();
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
	QString qt_ts_path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
	if (qt_ts_path.isEmpty())
		qt_ts_path = QCoreApplication::applicationDirPath();
	if (newlanguage=="de")
	{
		_qt_translator = new QTranslator;
		if (_qt_translator->load("qt_de_DE", qt_ts_path))
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
	QFile dumpfile;
	char SYS_ID[3] = {0};
	char ROM_ID[5] = {0};
	char flagbytes[96] = {0};
	unsigned char nrofflagbytes = 0;
	std::string hexstr = "";
	unsigned char k = 0;
	unsigned int dataaddr[17] = {0};
	char data[17] = {0};
	int ssm1_cu_index = SSM1_CU_Engine;

	if (_dumping) return;
	// Initialize and configure serial port:
	SerialPassThroughDiagInterface *diagInterface = initInterface();
	if (diagInterface)
	{
		if (!diagInterface->connect(AbstractDiagInterface::protocol_SSM2))
		{
			delete diagInterface;
			return;
		}
	}
	else
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
	// Create SSMP1-Communication-object:
	SSMP1communication SSMP1com(diagInterface, SSM1_CU_Engine, 0);
	// Create SSMP2-Communication-object:
	SSMP2communication SSMP2com(diagInterface, '\x10', 0);
	// ######## SSM2-Control-Units ########
	// **************** ECU ***************
	// Read ECU data:
	if (SSMP2com.getCUdata(SYS_ID, ROM_ID, flagbytes, &nrofflagbytes))
	{
		// Open/Create File:
		if (!dumpfile.open(QIODevice::WriteOnly | QIODevice::Text))
			goto end;
		// *** Convert and write data to file:
		// Sys-ID:
		hexstr = libFSSM::StrToHexstr(SYS_ID, 3);
		hexstr.push_back('\n');
		dumpfile.write(hexstr.data(), hexstr.length());
		// ROM-ID:
		hexstr = libFSSM::StrToHexstr(ROM_ID, 5);
		hexstr.push_back('\n');
		dumpfile.write(hexstr.data(), hexstr.length());
		// Flagbytes:
		for (k=1; k<=ceil(static_cast<float>(nrofflagbytes)/16); k++)
		{
			hexstr.clear();
			if (16*k <= nrofflagbytes)
				hexstr = libFSSM::StrToHexstr(flagbytes+((k-1)*16), 16);
			else
				hexstr = libFSSM::StrToHexstr(flagbytes+((k-1)*16), (nrofflagbytes%16));
			hexstr.push_back('\n');
			dumpfile.write(hexstr.data(), hexstr.length());
		}
		// VIN:
		if (flagbytes[36] == (flagbytes[36] | 0x01))
		{
			dataaddr[0] = 0xDA;
			dataaddr[1] = 0xDB;
			dataaddr[2] = 0xDC;
			if (!SSMP2com.readMultipleDatabytes(0x0, dataaddr, 3, data))
				goto end;
			hexstr = libFSSM::StrToHexstr(data, 3);
			hexstr.insert(0, "\n");
			hexstr.push_back('\n');
			dumpfile.write(hexstr.data(), hexstr.length());
			dataaddr[0] = (65536 * static_cast<unsigned char>(data[0]))
					+ (256 * static_cast<unsigned char>(data[1]))
					+ (static_cast<unsigned char>(data[2]));
			for (k=1; k<17; k++)
				dataaddr[k] = dataaddr[0]+k;
			if (!SSMP2com.readMultipleDatabytes(0x0, dataaddr, 17, data))
				goto end;
			hexstr = libFSSM::StrToHexstr(data, 17);
			hexstr.push_back('\n');
			dumpfile.write(hexstr.data(), hexstr.size());
		}
	}
	else if (dumpfile.isOpen())
		dumpfile.write("\n---\n", 5);
	// **************** TCU ***************
	SSMP2com.setCUaddress('\x18');
	// Read TCU data:
	if (SSMP2com.getCUdata(SYS_ID, ROM_ID, flagbytes, &nrofflagbytes))
	{
		if (!dumpfile.isOpen()) // if file is not opened yet
		{
			// Open/Create File:
			if (!dumpfile.open(QIODevice::WriteOnly | QIODevice::Text))
				goto end;
			dumpfile.write("\n---\n", 5);
		}
		// *** Convert and write data to file:
		// Sys-ID:
		hexstr = libFSSM::StrToHexstr(SYS_ID, 3);
		hexstr.insert(0, "\n");
		hexstr.push_back('\n');
		dumpfile.write(hexstr.data(), hexstr.length());
		// ROM-ID:
		hexstr = libFSSM::StrToHexstr(ROM_ID, 5);
		hexstr.push_back('\n');
		dumpfile.write(hexstr.data(), hexstr.length());
		// Flagbytes:
		for (k=1; k<=ceil(static_cast<float>(nrofflagbytes)/16); k++)
		{
			hexstr.clear();
			if (16*k <= nrofflagbytes)
				hexstr = libFSSM::StrToHexstr(flagbytes+((k-1)*16), 16);
			else
				hexstr = libFSSM::StrToHexstr(flagbytes+((k-1)*16), (nrofflagbytes%16));
			hexstr.push_back('\n');
			dumpfile.write(hexstr.data(), hexstr.length());
		}
	}
	else if (dumpfile.isOpen())
		dumpfile.write("\n---\n", 5);
	diagInterface->disconnect();
	// ######## SSM1-Control-Units ########
	// Configure interface:
	if (!diagInterface->connect(AbstractDiagInterface::protocol_SSM1))
		goto end;
	// Dump all SSM1-CUs:
	for (ssm1_cu_index=SSM1_CU_Engine; ssm1_cu_index<=SSM1_CU_FourWS; ssm1_cu_index++)
	{
		// Select CU:
		SSMP1com.selectCU( SSM1_CUtype_dt(ssm1_cu_index) );
		// Read CU-ID:
		if (SSMP1com.readID(SYS_ID))
		{
			if (!dumpfile.isOpen()) // if file is not opened yet
			{
				// Open/Create File:
				if (!dumpfile.open(QIODevice::WriteOnly | QIODevice::Text))
					goto end;

			for (k=0; k<(ssm1_cu_index+2); k++)
				dumpfile.write("\n-----\n", 7);

			}
			// *** Convert and write data to file:
			// ID:
			hexstr = libFSSM::StrToHexstr(SYS_ID, 3);
			hexstr.insert(0, "\n");
			hexstr.push_back('\n');
			dumpfile.write(hexstr.data(), hexstr.length());
		}
		else if (dumpfile.isOpen())
			dumpfile.write("\n-----\n", 7);
	}
	diagInterface->disconnect();

end:
	dumpfile.close();
	delete diagInterface;	// will be closed in destructor
	_dumping = false;
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


void FreeSSM::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS OF ui_FreeSSM.h (made with QDesigner)
	QFont appfont = QApplication::font();
	QFont font = this->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	this->setFont(font);
	font = controlunits_groupBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	controlunits_groupBox->setFont(font);
	font = engine_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	engine_pushButton->setFont(font);
	font = transmission_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	transmission_pushButton->setFont(font);
	font = program_groupBox->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	program_groupBox->setFont(font);
	font = help_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	help_pushButton->setFont(font);
	font = about_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	about_pushButton->setFont(font);
	font = preferences_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	preferences_pushButton->setFont(font);
	font = exit_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	exit_pushButton->setFont(font);
}


