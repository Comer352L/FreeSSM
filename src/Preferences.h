/*
 * Preferences.h - Adjustment of program settings
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
#include "Languages.h"
#include "AbstractDiagInterface.h"
#include "SerialPassThroughDiagInterface.h"
#include "ATcommandControlledDiagInterface.h"
#include "J2534DiagInterface.h"
#include "SSMP1communication.h"
#include "SSMP2communication.h"
#include "FSSMdialogs.h"
#include "ui_Preferences.h"




class Preferences : public QDialog, private Ui::Preferences_Dialog
{
	Q_OBJECT

private:
	QMainWindow *_FreeSSM_MainWindow;
	AbstractDiagInterface::interface_type *_r_interfacetype;
	AbstractDiagInterface::interface_type _newinterfacetype;
	QString *_r_interfacefilename;
	QString _newinterfacefilename;
	QString _language_old;
	QString _language_current;
	bool *_r_preferSSM2protocolVariantISO14230;
	bool _new_preferSSM2protocolVariantISO14230;
	QString _style_old;
	int _lastlangindex;
	bool _confirmed;
	QStringList _J2534libraryPaths;

	void displayErrorMsg(QString errormsg);
	void closeEvent(QCloseEvent *event);

public:
	Preferences(QMainWindow *parent = NULL, AbstractDiagInterface::interface_type *ifacetype = NULL, QString *ifacefilename = NULL, QString language = "", bool *preferSSM2protocolVariantISO14230 = NULL);
	~Preferences();

public slots:
	void switchLanguage(int langindex);
	void switchGUIstyle(QString style);
	void selectInterfaceType(int index);
	void selectInterfaceName(int index);
	void selectPreferredSSM2protocolVariant(int state);
	void interfacetest();
	void ok();

signals:
	void languageSelChanged(QString language, QTranslator *translator);

};



#endif
