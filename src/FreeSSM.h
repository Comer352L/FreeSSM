/*
 * FreeSSM.h - Program main window
 *
 * Copyright © 2008 Comer352l
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



#define FSSM_VERSION "v0.97.2"



#ifdef __WIN32__
    #include "windows\serialCOM.h"
#elif defined __linux__
    #include "linux/serialCOM.h"
#else
    #error "Operating system not supported !"
#endif
#include <QtGui>
#include "SSMprotocol.h"
#include "SSMPcommunication.h"
#include "Engine.h"
#include "Transmission.h"
#include "Preferences.h"
#include "About.h"
#include "ui_FreeSSM.h"




class FreeSSM : public QMainWindow, private Ui::FreeSSM_MainWindow
{
	Q_OBJECT

private:
	QString progversion;
	QString portname;
	QString language;
	QTranslator *qt_translator;
	QTranslator *translator;
	serialCOM *port;
	SSMprotocol *SSMPdev;
	QLabel *progtitle_label;
	bool dumping;

	void setupUiFonts();
	bool initPort(unsigned int baudrate, serialCOM *port);
	void StrToHexstr(char *inputstr, unsigned int nrbytes, QString *hexstr);
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
