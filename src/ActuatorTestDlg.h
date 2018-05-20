/*
 * ActuatorTestDlg.h - Actuator Test Dialog
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

#ifndef ACTUATORTESTDLG_H
#define ACTUATORTESTDLG_H


#include <QtGlobal>	/* required for QT_VERSION */
#if QT_VERSION < 0x050000
	#include <QtGui>
#else
	#include <QtWidgets>
#endif
#include "ui_ActuatorTestDlg.h"
#include "SSMprotocol.h"



class ActuatorTestDlg : public QDialog, private Ui::actuatortest_Dialog
{
	Q_OBJECT

public:
	ActuatorTestDlg(QWidget *parent, SSMprotocol *SSMPdev, QString actuatorTitle, unsigned char actuatorIndex);

private:
	SSMprotocol *_SSMPdev;

	void closeEvent(QCloseEvent *event);

signals:
	void error();
};



#endif
