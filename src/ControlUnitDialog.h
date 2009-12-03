/*
 * ControlUnitDialog.h - Template for Control Unit dialogs
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

#ifndef CONTROLUNITDIALOG_H
#define CONTROLUNITDIALOG_H


#ifdef __WIN32__
    #include "windows\serialCOM.h"
#elif defined __linux__
    #include "linux/serialCOM.h"
#else
    #error "Operating system not supported !"
#endif
#include <QtGui>
#include "ui_ControlUnitDialog.h"
#include "SSMprotocol1.h"
#include "SSMprotocol2.h"
#include "FSSMdialogs.h"



class ControlUnitDialog : public QMainWindow, private Ui::ControlUnit_Window
{
	Q_OBJECT

public:
	ControlUnitDialog(QString title, serialCOM *port, QString language);
	~ControlUnitDialog();

protected:
	SSMprotocol *_SSMPdev;

	QPushButton * addFunction(QString title, QIcon icon, bool checkable);
	bool probeProtocol(SSMprotocol::CUtype_dt CUtype);
	QGroupBox * infoGroupBox();
	QGroupBox * contentGroupBox();

private:
	QString _language;
	serialCOM *_port;
	QList<QPushButton*> _selButtons;

	bool configurePort(unsigned int baud, char parity);
	void setupUiFonts();
	void closeEvent(QCloseEvent *event);

protected slots:
	void communicationError(QString addstr = "");

};



#endif
