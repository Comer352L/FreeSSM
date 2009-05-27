/*
 * CUcontent_sysTests.cpp - Widget for System Test Procedures
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

#include "CUcontent_sysTests.h"


CUcontent_sysTests::CUcontent_sysTests(QWidget *parent, SSMprotocol2 *SSMP2dev) : QWidget(parent)
{
	_SSMP2dev = SSMP2dev;
	_actuatorTestTitles.clear();
	// Setup GUI:
	setupUi(this);
	setupUiFonts();
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
	_SSMP2dev->stopActuatorTesting();	// ActuatorTestDlg already stops test on delete...
	disconnect( startActuatorTest_pushButton, SIGNAL( released() ), this, SLOT( startActuatorTest() ) );
	disconnect( testImmoLine_pushButton, SIGNAL( released() ), this, SLOT( testImmobilizerLine() ) );
}


bool CUcontent_sysTests::setup()
{
	bool ok = false;
	bool AT_sup = false;
	bool immo_sup = false;
	unsigned char k = 0;

	_actuatorTestTitles.clear();
	// Get CU-informations:
	ok = _SSMP2dev->hasActuatorTests(&AT_sup);
	if (ok)
		ok = _SSMP2dev->getSupportedActuatorTests(&_actuatorTestTitles);
	if (ok)
		ok = _SSMP2dev->hasImmobilizer(&immo_sup);
	AT_sup = AT_sup && ok;
	immo_sup = immo_sup && ok;
	// Display actuator test list content:
	if (AT_sup)
	{
		// Fill cctuator list:
		for (k=0; k<_actuatorTestTitles.size(); k++)
			actuators_listWidget->addItem( _actuatorTestTitles[k] );
		// Select first actuator on the list:
		if (_actuatorTestTitles.size() > 0)
			actuators_listWidget->setCurrentRow(0);
	}
	else
		actuators_listWidget->clear();
	// Enable/Disable GUI-elements:
	actuatorlistTitle_label->setEnabled( AT_sup );
	actuators_listWidget->setEnabled( AT_sup );
	actuatorTest_arrow_label->setEnabled( AT_sup );
	startActuatorTest_pushButton->setEnabled( AT_sup );
	actuatorTestInfo_label->setEnabled( AT_sup );
	testImmoLine_pushButton->setEnabled(immo_sup);
	testImmoLineTitel_label->setEnabled(immo_sup);
	// Return result:
	return ok;
}


void CUcontent_sysTests::startActuatorTest()
{
	unsigned char actuatorTestNr = 0;
	bool testmode = false;
	QFont mbfont;

	// Create wait message for test mode connecter status check:
	FSSM_WaitMsgBox wmsgbox(this, tr("Checking test mode connector... Please wait !   "));
	wmsgbox.show();
	// Check test mode connecotor status:
	if (_SSMP2dev->isInTestMode(&testmode))
	{
		wmsgbox.close();
		if (testmode)
		{
			// Get selected actuator:
			actuatorTestNr = static_cast<unsigned char>(actuators_listWidget->currentIndex().row());
			ActuatorTestDlg AT_dlg(this, _SSMP2dev, _actuatorTestTitles.at(actuatorTestNr), actuatorTestNr);
			// Open test dialog and wait for test to finish:
			if (AT_dlg.isVisible())	// if actuator test has not been aborted in the constructor
				AT_dlg.exec();
		}
		else
		{
			// Error message: not in test mode
			QMessageBox infomsgbox( QMessageBox::Critical, tr("Actuator Test"), tr("Actuator Test couldn't be started:\n=> Test mode connector is not connected !\n\nATTENTION:\nConnect/Disconnect test mode connector\nONLY WITH IGNITION SWITCHED OFF !"), QMessageBox::Ok, this);
			mbfont = infomsgbox.font();
			mbfont.setPixelSize(12); // 9pts
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
	SSMprotocol2::immoTestResult_dt testresult = SSMprotocol2::immoNotShorted;
	QFont msgfont;
	QString resultInfo = "";
	QMessageBox::Icon msgboxicon = QMessageBox::NoIcon;

	// Run immobilizer communication line test:
	FSSM_WaitMsgBox wmsgbox(this, tr("Testing Immobilizer Communication Line... Please wait !   "));
	wmsgbox.show();
	ok = _SSMP2dev->TestImmobilizerCommLine(&testresult);
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
		if (testresult == SSMprotocol2::immoNotShorted)
		{
			// "Success"-message:
			resultInfo = tr("The Immobilizer Communication Line is not shorted.");
			msgboxicon = QMessageBox::Information;
		}
		else if (testresult == SSMprotocol2::immoShortedToGround)
		{
			// "Error" message:
			resultInfo = tr("The Immobilizer Communication Line seems\nto be shorted to ground !");
			msgboxicon = QMessageBox::Critical;
		}
		else if (testresult == SSMprotocol2::immoShortedToBattery)
		{
			// "Error" message:
			resultInfo = tr("The Immobilizer Communication Line seems\nto be shorted to battery (+) !");
			msgboxicon = QMessageBox::Critical;
		}
		QMessageBox resultmsgbox( msgboxicon, tr("Immobilizer Test"), resultInfo, QMessageBox::Ok, this);
		msgfont = resultmsgbox.font();
		msgfont.setPixelSize(12); // 9pts
		resultmsgbox.setFont( msgfont );
		resultmsgbox.show();
		resultmsgbox.exec();
		resultmsgbox.close();
	}
	else
		communicationError("=> Immobilizer Communication Line Test could not be started !");
}


void CUcontent_sysTests::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS MADE WITH QDESIGNER
	QFont contentfont = QApplication::font();
	contentfont.setPixelSize(12); // 9pts
	contentfont.setBold(false);
	this->setFont(contentfont);
	// Titles:
	QFont titlefont = contentfont;
	titlefont.setUnderline(true);
	actuatorlistTitle_label->setFont(titlefont);
	testImmoLineTitel_label->setFont(titlefont);
	// Actuator Test elements:
	actuators_listWidget->setFont(contentfont);
	actuatorTestInfo_label->setFont(contentfont);
	QFont arrowfont = contentfont;
	arrowfont.setPointSize(18);
	actuatorTest_arrow_label->setFont(arrowfont);
	// Buttons:
	startActuatorTest_pushButton->setFont(contentfont);
	testImmoLine_pushButton->setFont(contentfont);
}


void CUcontent_sysTests::communicationError(QString adstr)
{
	if (adstr.size() > 0) adstr.prepend('\n');
	QMessageBox msg( QMessageBox::Critical, tr("Communication Error"), tr("Communication Error:\n- No or invalid answer from TCU -") + adstr, QMessageBox::Ok, this);
	QFont msgfont = msg.font();
	msgfont.setPixelSize(12); // 9pts
	msg.setFont( msgfont );
	msg.show();
	msg.exec();
	msg.close();
	emit error();
}


/* TODO:
	- implement Dealer Check Mode Procedure
*/

