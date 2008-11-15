/*
 * Preferences.h - Adjustment of program settings
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


#ifndef PREFERENCES_H
#define PREFERENCES_H


#ifdef __WIN32__
    #include "windows\serialCOM.h"
#elif defined __linux__
    #include "linux/serialCOM.h"
#else
    #error "Operating system not supported !"
#endif
#include <QtGui>
#include "SSMPcommunication.h"
#include "FSSMdialogs.h"
#include "ui_Preferences.h"




class Preferences : public QDialog, private Ui::Preferences_Dialog
{
	Q_OBJECT

private:
	QMainWindow *FreeSSM_MainWindow;
	QString *r_portname;
	QString newportname;
	QString language_old;
	QString language_current;
	int lastlangindex;
	bool confirmed;

	void setupUiFonts();
	void closeEvent(QCloseEvent *event);

public:
	Preferences(QMainWindow *parent = 0, QString *portname = 0, QString language = 0);
	~Preferences();

public slots:
	void serialport();
	void switchLanguage(int langindex);
	void interfacetest();
	void ok();
	void cancel();

signals:
	void languageSelChanged(QString language, QTranslator *translator);

};



#endif
