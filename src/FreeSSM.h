/*
 * FreeSSM.h - Program main window
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

#ifndef FREESSM_H
#define FREESSM_H



#include <QtGui>
#include "Languages.h"
#include "AbstractDiagInterface.h"
#include "SerialPassThroughDiagInterface.h"
#include "J2534DiagInterface.h"
#include "ATcommandControlledDiagInterface.h"
#include "SSMP1communication.h"
#include "SSMP2communication.h"
#include "libFSSM.h"
#include "EngineDialog.h"
#include "TransmissionDialog.h"
#include "ABSdialog.h"
#include "CruiseControlDialog.h"
#include "AirConDialog.h"
#include "Preferences.h"
#include "About.h"
#include "ui_FreeSSM.h"
#include "SSMCUdata.h"



class FreeSSM : public QMainWindow, private Ui::FreeSSM_MainWindow
{
	Q_OBJECT

private:
	AbstractDiagInterface::interface_type _iface_type;
	QString _iface_filename;
	QString _language;
	QTranslator *_qt_translator;
	QTranslator *_translator;
	QLabel *_progtitle_label;
	QAction *_dump_action;
	bool _dumping;

	AbstractDiagInterface * initInterface();
	void displayErrorMsg(QString errmsg);
	void keyPressEvent(QKeyEvent *event);
	void closeEvent(QCloseEvent *event);

public:
	FreeSSM(QApplication *app = 0);
	~FreeSSM();

private slots:
	void engine();
	void transmission();
	void abs();
	void cruisecontrol();
	void aircon();
	void preferences();
	void help();
	void about();
	void retranslate(QString newlanguage, QTranslator *newtranslator);
	void dumpCUdata();

};



#endif
