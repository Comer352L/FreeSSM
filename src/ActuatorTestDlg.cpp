/*
 * ActuatorTestDlg.cpp - Actuator Test Dialog
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

#include "ActuatorTestDlg.h"



ActuatorTestDlg::ActuatorTestDlg(QWidget *parent, SSMprotocol *SSMPdev, QString actuatorTitle, unsigned char actuatorIndex) : QDialog(parent)
{
	bool isrunning = false;
	int dlgchoice = 0;
	QFont msgfont;

	_SSMPdev = SSMPdev;
	setupUi(this);
	exit_pushButton->setEnabled(false);
#ifndef SMALL_RESOLUTION
	this->show();
#else
	// https://bugreports.qt.io/browse/QTBUG-16034
	// Workaround for window not showing always fullscreen
	setWindowFlags( Qt::Window );
	this->showFullScreen();
#endif
	actuator_label->setText(actuatorTitle);
	status_label->setText(tr("Checking engine status..."));
	// CHECK IF ENGINE IS RUNNING:
	while (dlgchoice == 0)
	{
		if (_SSMPdev->isEngineRunning(&isrunning))
		{
			if (isrunning)
			{
				QMessageBox engineoffmsg( QMessageBox::Information, tr("Engine is running"), tr("Please turn engine off and switch ignition on."), QMessageBox::NoButton, this);
				engineoffmsg.addButton(tr("Continue"), QMessageBox::AcceptRole);
				engineoffmsg.addButton(tr("Cancel"), QMessageBox::RejectRole);
				msgfont = engineoffmsg.font();
				msgfont.setPointSize(9);
				engineoffmsg.setFont( msgfont );
				engineoffmsg.show();
				dlgchoice = engineoffmsg.exec();
				engineoffmsg.close();

				if (dlgchoice == 1)
				{
					close();
					return;
				}
			}
			else	// if engine is NOT running
				break;
		}
		else	// if engine status couldn't be determined
		{
			// Error message:
			QMessageBox errmsg( QMessageBox::Critical, tr("Communication Error"), tr("Please ensure that ignition is switched ON."), QMessageBox::NoButton, this);
			errmsg.addButton(tr("Retry"), QMessageBox::AcceptRole);
			errmsg.addButton(tr("Exit Control Unit"), QMessageBox::RejectRole);
			msgfont = errmsg.font();
			msgfont.setPointSize(9);
			errmsg.setFont( msgfont );
			errmsg.show();
			dlgchoice = errmsg.exec();
			errmsg.close();
			// close dialog if aborted
			if (dlgchoice == 1)
			{
				close();	// close actuator test dialog
				parent->close();// close engine window / control unit
				return;
			}
		}
	}
	// START ACTUATOR TEST:
	status_label->setText(tr("Starting test..."));
	if (!_SSMPdev->startActuatorTest(actuatorIndex))
	{
		// Show error message
		QMessageBox msg( QMessageBox::Critical, tr("Communication Error"), tr("Communication Error:\nActuator Test couldn't be started."), QMessageBox::Ok, this);
		QFont msgfont = msg.font();
		msgfont.setPointSize(9);
		msg.setFont( msgfont );
		msg.show();
		msg.exec();
		msg.close();
		close();
		emit error();
		return;
	}
	else
		status_label->setText(tr("Test is running..."));
	connect( exit_pushButton, SIGNAL( released() ), this, SLOT( close() ) );
	exit_pushButton->setEnabled(true);
}



void ActuatorTestDlg::closeEvent(QCloseEvent *event)
{
	exit_pushButton->setEnabled(false);
	status_label->setText(tr("Stopping test..."));
	_SSMPdev->stopActuatorTesting();
	event->accept();
}
