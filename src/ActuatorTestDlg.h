/*
 * ActuatorTestDlg.h - Actuator Test Dialog
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

#ifndef ACTUATORTESTDLG_H
#define ACTUATORTESTDLG_H


#include <QtGui>
#include "ui_ActuatorTestDlg.h"
#include "SSMprotocol2.h"



class ActuatorTestDlg : public QDialog, private Ui::actuatortest_Dialog
{
	Q_OBJECT

public:
	ActuatorTestDlg(QWidget *parent, SSMprotocol2 *SSMP2dev, QString actuatorTitle, unsigned char actuatorIndex);

private:
	SSMprotocol2 *_SSMP2dev;

	void setupUiFonts();
	void closeEvent(QCloseEvent *event);

signals:
	void error();
};



#endif
