/*
 * CUcontent_DCs_stopCodes.cpp - Widget for Diagnostic Codes Reading with Stop Codes
 *
 * Copyright (C) 2012 L1800Turbo, 2008-2018 Comer352L
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

#include "CUcontent_DCs_stopCodes.h"


CUcontent_DCs_stopCodes::CUcontent_DCs_stopCodes(QWidget *parent) : CUcontent_DCs_abstract(parent)
{
	_currOrTempDTCs.clear();
	_currOrTempDTCdescriptions.clear();
	// Setup GUI:
	setupUi(this);
	// Set column widths:
	QHeaderView *headerview;
	currOrTempDTCs_tableWidget->setColumnWidth (0, 70);
	headerview = currOrTempDTCs_tableWidget->horizontalHeader();
#if QT_VERSION < 0x050000
	headerview->setResizeMode(0,QHeaderView::Interactive);
	headerview->setResizeMode(1,QHeaderView::Stretch);
#else
	headerview->setSectionResizeMode(0,QHeaderView::Interactive);
	headerview->setSectionResizeMode(1,QHeaderView::Stretch);
#endif
	// Set table row resize behavior:
	headerview = currOrTempDTCs_tableWidget->verticalHeader();
#if QT_VERSION < 0x050000
	headerview->setResizeMode(QHeaderView::Fixed);
#else
	headerview->setSectionResizeMode(QHeaderView::Fixed);
#endif
	/* NOTE: Current method for calculating ther nr. of needed rows
	 * assumes all rows to have the same constsant height */
	// Install event-filter for DC-tables:
	currOrTempDTCs_tableWidget->viewport()->installEventFilter(this);
	// *** Set initial content ***:
	// Set provisional titles:
	currOrTempDTCsTitle_label->setText( tr("Current Stop Code:") );
	// Disable tables and their titles:
	currOrTempDTCsTitle_label->setEnabled( false );
	currOrTempDTCs_tableWidget->setEnabled( false );
#ifndef SMALL_RESOLUTION
	// Disable "print"-button:
	printDClist_pushButton->setDisabled(true);
#endif
}


CUcontent_DCs_stopCodes::~CUcontent_DCs_stopCodes()
{
#ifndef SMALL_RESOLUTION
	disconnect(printDClist_pushButton, SIGNAL( released() ), this, SLOT( printDCprotocol() ));
#endif
	disconnectGUIelements();
}


bool CUcontent_DCs_stopCodes::setup(SSMprotocol *SSMPdev)
{
	bool ok = false;
	bool currOrTempDTCs_sup = false;
	QString title;

	_SSMPdev = SSMPdev;
	// Reset data:
	_supportedDCgroups = SSMprotocol::noDCs_DCgroup;
	_currOrTempDTCs.clear();
	_currOrTempDTCdescriptions.clear();

	// Get CU information:
	ok = (_SSMPdev != NULL);
	if (ok)
		ok =_SSMPdev->getSupportedDCgroups(&_supportedDCgroups);
	if (ok)
	{
		currOrTempDTCs_sup = ((_supportedDCgroups & SSMprotocol::currentDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::temporaryDTCs_DCgroup));
	}
	// Set titles of the DTC-tables
	title = tr("Current Stop Code:");
	currOrTempDTCsTitle_label->setText( title );

	// DC-tables and titles:
	if (ok && !currOrTempDTCs_sup)
		setDCtableContent(currOrTempDTCs_tableWidget, QStringList(""), QStringList(tr("----- Not supported by ECU -----")));
	else
		setDCtableContent(currOrTempDTCs_tableWidget, QStringList(""), QStringList(""));
	currOrTempDTCsTitle_label->setEnabled(currOrTempDTCs_sup);
	currOrTempDTCs_tableWidget->setEnabled(currOrTempDTCs_sup);
#ifndef SMALL_RESOLUTION
	// Deactivate and disconnect "Print"-button:
	printDClist_pushButton->setEnabled(false);
	disconnect(printDClist_pushButton, SIGNAL( released() ), this, SLOT( printDCprotocol() ));
#endif
	// Connect start-slot:
	if (_SSMPdev)
	{
		if (ok && (_supportedDCgroups != SSMprotocol::noDCs_DCgroup))
			connect(_SSMPdev, SIGNAL( startedDCreading() ), this, SLOT( callStart() ));
		else
			disconnect(_SSMPdev, SIGNAL( startedDCreading() ), this, SLOT( callStart() ));
	}
	// Return result;
	return ok;
}


void CUcontent_DCs_stopCodes::connectGUIelements()
{
	if (!_SSMPdev) return;
	// DTCs:   disable tables of unsupported DTCs, initial output, connect slots:
	if ((_supportedDCgroups & SSMprotocol::currentDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::temporaryDTCs_DCgroup))
	{
		updateCurrentOrTemporaryDTCsContent(QStringList(""), QStringList(tr("----- Reading data... Please wait ! -----")));
		connect(_SSMPdev, SIGNAL( currentOrTemporaryDTCs(QStringList, QStringList, bool, bool) ), this, SLOT( updateCurrentOrTemporaryDTCsContent(QStringList, QStringList) ));
	}
#ifndef SMALL_RESOLUTION
	// Connect and disable print-button temporary (until all memories have been read once):
	printDClist_pushButton->setDisabled(true);
	connect(printDClist_pushButton, SIGNAL( released() ), this, SLOT( printDCprotocol() ));
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
#endif
}


void CUcontent_DCs_stopCodes::disconnectGUIelements()
{
	if (!_SSMPdev) return;	// avoid NULL-pointer-warning-message
	disconnect(_SSMPdev, SIGNAL( currentOrTemporaryDTCs(QStringList, QStringList, bool, bool) ), this, SLOT( updateCurrentOrTemporaryDTCsContent(QStringList, QStringList) ));
}


void CUcontent_DCs_stopCodes::updateCurrentOrTemporaryDTCsContent(QStringList currOrTempDTCs, QStringList currOrTempDTCdescriptions)
{
	if ((currOrTempDTCs != _currOrTempDTCs) || (currOrTempDTCdescriptions != _currOrTempDTCdescriptions))
	{
		// Save Trouble Codes:
		_currOrTempDTCs = currOrTempDTCs;
		_currOrTempDTCdescriptions = currOrTempDTCdescriptions;
		// Output Trouble Codes:
		if ((currOrTempDTCs.size() == 0) && (currOrTempDTCdescriptions.size() == 0))
		{
			currOrTempDTCs << "";
			currOrTempDTCdescriptions << tr("----- No valid Stop Codes -----");
		}
		setDCtableContent(currOrTempDTCs_tableWidget, currOrTempDTCs, currOrTempDTCdescriptions);
#ifndef SMALL_RESOLUTION
		// Activate "Print" button:
		printDClist_pushButton->setEnabled(true);
#endif
	}
}


#ifndef SMALL_RESOLUTION
void CUcontent_DCs_stopCodes::createDCprintTables(QTextCursor cursor)
{
	QStringList currOrTempDTCcodes = _currOrTempDTCs;
	QStringList currOrTempDTCdescriptions = _currOrTempDTCdescriptions;
	// Current/Temporary DTCs:
	if ((_supportedDCgroups & SSMprotocol::currentDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::temporaryDTCs_DCgroup))
	{
		if (_currOrTempDTCdescriptions.size() == 0)
		{
			currOrTempDTCcodes << "";
			currOrTempDTCdescriptions << tr("----- No valid Stop Codes -----");
		}
		// Insert table with current/temporary DTCs into text document:
		insertDCprintTable(cursor, currOrTempDTCsTitle_label->text(), currOrTempDTCcodes, currOrTempDTCdescriptions);
	}
}
#endif


void CUcontent_DCs_stopCodes::resizeEvent(QResizeEvent *event)
{
	// Calculate and set number of table rows:
	setNrOfTableRows(currOrTempDTCs_tableWidget, _currOrTempDTCs.size() );
	// Accept event:
	event->accept();
}


bool CUcontent_DCs_stopCodes::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::Wheel)
	{
		if (obj == currOrTempDTCs_tableWidget->viewport())
		{
			if (currOrTempDTCs_tableWidget->verticalScrollBarPolicy() ==  Qt::ScrollBarAlwaysOff)
				return true;	// filter out
			else
				return false;
		}
	}
	// Pass the event on to the parent class
	return QWidget::eventFilter(obj, event);
}

