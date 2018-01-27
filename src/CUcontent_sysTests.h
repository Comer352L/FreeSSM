/*
 * CUcontent_sysTests.h - Widget for System Test Procedures
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

#ifndef CUCONTENT_SYSTESTS_H
#define CUCONTENT_SYSTESTS_H



#include <QWidget>
#include "ui_CUcontent_sysTests.h"
#include "SSMprotocol.h"
#include "ActuatorTestDlg.h"
#include "FSSMdialogs.h"



class CUcontent_sysTests : public QWidget, private Ui::sysTestsContent_Form
{
	Q_OBJECT

private:
	SSMprotocol *_SSMPdev;
	QStringList _actuatorTestTitles;

	void communicationError(QString adstr = "");

public:
	CUcontent_sysTests(QWidget *parent = 0);
	~CUcontent_sysTests();
	bool setup(SSMprotocol *SSMPdev);

private slots:
	void startActuatorTest();
	void testImmobilizerLine();

signals:
	void error();

};


#endif
