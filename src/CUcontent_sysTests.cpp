/*
 * CUcontent_sysTests.cpp - Widget for System Test Procedures
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

#include "CUcontent_sysTests.h"


CUcontent_sysTests::CUcontent_sysTests(QWidget *parent) : QWidget(parent)
{
	_SSMPdev = NULL;
	_actuatorTestTitles.clear();
	// Setup GUI:
	setupUi(this);
	// Disable GUI-elements:
	actuatorlistTitle_label->setEnabled(false);
	actuators_listWidget->setEnabled(false);
	actuatorTest_arrow_label->setEnabled(false);
	startActuatorTest_pushButton->setEnabled(false);
	actuatorTestInfo_label->setEnabled(false);
	testImmoLine_pushButton->setEnabled(false);
	testImmoLineTitel_label->setEnabled(false);
	// Connect signals and slots:
	connect( startActuatorTest_pushButton, SIGNAL( released() ), this, SLOT( startActuatorTest() ) );
	connect( testImmoLine_pushButton, SIGNAL( released() ), this, SLOT( testImmobilizerLine() ) );
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
}


CUcontent_sysTests::~CUcontent_sysTests()
{
	if (_SSMPdev)
		_SSMPdev->stopActuatorTesting();	// ActuatorTestDlg already stops test on delete...
	disconnect( startActuatorTest_pushButton, SIGNAL( released() ), this, SLOT( startActuatorTest() ) );
	disconnect( testImmoLine_pushButton, SIGNAL( released() ), this, SLOT( testImmobilizerLine() ) );
}


bool CUcontent_sysTests::setup(SSMprotocol *SSMPdev)
{
	bool ok = false;
	bool AT_sup = false;
	bool immotest_sup = false;

	_SSMPdev = SSMPdev;
	_actuatorTestTitles.clear();
	// Get CU-informations:
	ok = (_SSMPdev != NULL);
	if (ok)
		ok = _SSMPdev->hasActuatorTests(&AT_sup);
	if (ok && AT_sup)
		ok = _SSMPdev->getSupportedActuatorTests(&_actuatorTestTitles);
	if (ok)
		ok = _SSMPdev->hasImmobilizerTest(&immotest_sup);
	AT_sup = AT_sup && ok;
	immotest_sup = immotest_sup && ok;
	// Display actuator test list content:
	if (AT_sup)
	{
		if (_actuatorTestTitles.size() > 0)
		{
			actuators_listWidget->addItems(_actuatorTestTitles);
			actuators_listWidget->setCurrentRow(0);
		}
	}

	// Enable/Disable GUI-elements:
	if (_SSMPdev->ifceProtocolType() != AbstractDiagInterface::protocol_type::SSM2_ISO15765)
	// FIXME: enable ISO-15765 again, when SSMprotocol has been fixed
	{
		actuatorlistTitle_label->setEnabled( AT_sup );
		actuators_listWidget->setEnabled( AT_sup );
		actuatorTest_arrow_label->setEnabled( AT_sup );
		startActuatorTest_pushButton->setEnabled( AT_sup );
		actuatorTestInfo_label->setEnabled( AT_sup );
	}
	testImmoLine_pushButton->setEnabled(immotest_sup);
	testImmoLineTitel_label->setEnabled(immotest_sup);
	// Return result:
	return ok;
}


void CUcontent_sysTests::startActuatorTest()
{
	unsigned char actuatorTestNr = 0;
	bool testmode = false;
	QFont mbfont;

	if (!_SSMPdev) return;
	// Create wait message for test mode connecter status check:
	FSSM_WaitMsgBox wmsgbox(this, tr("Checking test mode connector... Please wait !"));
	wmsgbox.show();
	// Check test mode connecotor status:
	if (_SSMPdev->isInTestMode(&testmode))
	{
		wmsgbox.close();
		if (testmode)
		{
			// Get selected actuator:
			actuatorTestNr = static_cast<unsigned char>(actuators_listWidget->currentIndex().row());
			ActuatorTestDlg AT_dlg(this, _SSMPdev, _actuatorTestTitles.at(actuatorTestNr), actuatorTestNr);
			// Open test dialog and wait for test to finish:
			if (AT_dlg.isVisible())	// if actuator test has not been aborted in the constructor
				AT_dlg.exec();
		}
		else
		{
			// Error message: not in test mode
			QMessageBox infomsgbox( QMessageBox::Critical, tr("Actuator Test"), tr("Actuator Test couldn't be started:\n=> Test mode connector is not connected !") + "\n\n" + tr("ATTENTION:\nConnect/Disconnect test mode connector\nONLY WITH IGNITION SWITCHED OFF !"), QMessageBox::Ok, this);
			mbfont = infomsgbox.font();
			mbfont.setPointSize(9);
			infomsgbox.setFont( mbfont );
			infomsgbox.show();
			infomsgbox.exec();
			infomsgbox.close();
		}
	}
	else	// if error while checking test mode connector state
	{
		wmsgbox.close();
		communicationError();
	}
}


void CUcontent_sysTests::testImmobilizerLine()
{
	bool ok = false;
	SSMprotocol::immoTestResult_dt testresult = SSMprotocol::immoNotShorted;
	QFont msgfont;
	QString resultInfo = "";
	QMessageBox::Icon msgboxicon = QMessageBox::NoIcon;

	if (!_SSMPdev) return;
	// Run immobilizer communication line test:
	FSSM_WaitMsgBox wmsgbox(this, tr("Testing Immobilizer Communication Line... Please wait !"));
	wmsgbox.show();
	ok = _SSMPdev->testImmobilizerCommLine(&testresult);
	if (ok)
	{
		QEventLoop el;
		QTimer::singleShot(800, &el, SLOT( quit() ));
		el.exec();
	}
	wmsgbox.hide(); // close() doesn't work
	// Check test result:
	if (ok)
	{
		if (testresult == SSMprotocol::immoNotShorted)
		{
			// "Success"-message:
			resultInfo = tr("The Immobilizer Communication Line is not shorted.");
			msgboxicon = QMessageBox::Information;
		}
		else if (testresult == SSMprotocol::immoShortedToGround)
		{
			// "Error" message:
			resultInfo = tr("The Immobilizer Communication Line seems\nto be shorted to ground !");
			msgboxicon = QMessageBox::Critical;
		}
		else if (testresult == SSMprotocol::immoShortedToBattery)
		{
			// "Error" message:
			resultInfo = tr("The Immobilizer Communication Line seems\nto be shorted to battery (+) !");
			msgboxicon = QMessageBox::Critical;
		}
		QMessageBox resultmsgbox( msgboxicon, tr("Immobilizer Test"), resultInfo, QMessageBox::Ok, this);
		msgfont = resultmsgbox.font();
		msgfont.setPointSize(9);
		resultmsgbox.setFont( msgfont );
		resultmsgbox.show();
		resultmsgbox.exec();
		resultmsgbox.close();
	}
	else
		communicationError("=> Immobilizer Communication Line Test could not be started !");
}


void CUcontent_sysTests::communicationError(QString adstr)
{
	if (adstr.size() > 0) adstr.prepend('\n');
	QMessageBox msg( QMessageBox::Critical, tr("Communication Error"), tr("Communication Error:") + adstr, QMessageBox::Ok, this);
	QFont msgfont = msg.font();
	msgfont.setPointSize(9);
	msg.setFont( msgfont );
	msg.show();
	msg.exec();
	msg.close();
	emit error();
}


/* TODO:
	- implement Dealer Check Mode Procedure
*/

