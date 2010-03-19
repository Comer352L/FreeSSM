/*
 * FreeSSM.h - Program main window
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

#ifndef FREESSM_H
#define FREESSM_H



#define FSSM_VERSION "v1.2.1"



#ifdef __WIN32__
    #include "windows\serialCOM.h"
#elif defined __linux__
    #include "linux/serialCOM.h"
#else
    #error "Operating system not supported !"
#endif
#include <QtGui>
#include <string>
#include "SSMprotocol2.h"
#include "SSMP2communication.h"
#include "libFSSM.h"
#include "Engine.h"
#include "Transmission.h"
#include "Preferences.h"
#include "About.h"
#include "ui_FreeSSM.h"




class FreeSSM : public QMainWindow, private Ui::FreeSSM_MainWindow
{
	Q_OBJECT

private:
	QString _progversion;
	QString _portname;
	QString _language;
	QTranslator *_qt_translator;
	QTranslator *_translator;
	serialCOM *_port;
	SSMprotocol2 *_SSMP2dev;
	QLabel *_progtitle_label;
	QAction *_dump_action;
	bool _dumping;

	void setupUiFonts();
	bool initPort(unsigned int baudrate, serialCOM *port);
	void keyPressEvent(QKeyEvent *event);
	void closeEvent(QCloseEvent *event);

public:
	FreeSSM(QApplication *app = 0);
	~FreeSSM();
 
private slots:
	void engine();
	void transmission();
	void preferences();
	void help();
	void about();
	void SSMPdevCleanup();
	void retranslate(QString newlanguage, QTranslator *newtranslator);
	void dumpCUdata();

};



#endif
