/*
 * CUcontent_DCs_twoMemories.cpp - Widget for Diagnostic Codes Reading with two memories
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

#include "CUcontent_DCs_twoMemories.h"


CUcontent_DCs_twoMemories::CUcontent_DCs_twoMemories(QWidget *parent) : CUcontent_DCs_abstract(parent)
{
	_currOrTempDTCs.clear();
	_currOrTempDTCdescriptions.clear();
	_histOrMemDTCs.clear();
	_histOrMemDTCdescriptions.clear();
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
	histOrMemDTCs_tableWidget->setColumnWidth (0, 70);
	headerview = histOrMemDTCs_tableWidget->horizontalHeader();
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
	headerview = histOrMemDTCs_tableWidget->verticalHeader();
#if QT_VERSION < 0x050000
	headerview->setResizeMode(QHeaderView::Fixed);
#else
	headerview->setSectionResizeMode(QHeaderView::Fixed);
#endif
	/* NOTE: Current method for calculating ther nr. of needed rows
	 * assumes all rows to have the same constsant height */
	// Install event-filter for DC-tables:
	currOrTempDTCs_tableWidget->viewport()->installEventFilter(this);
	histOrMemDTCs_tableWidget->viewport()->installEventFilter(this);
	// *** Set initial content ***:
#ifndef SMALL_RESOLUTION
	// Set provisional titles:
	currOrTempDTCsTitle_label->setText( tr("Temporary Diagnostic Trouble Code(s):") );
	histOrMemDTCsTitle_label->setText( tr("Memorized Diagnostic Trouble Code(s):") );
	// Disable tables titles:
	currOrTempDTCsTitle_label->setEnabled( false );
	histOrMemDTCsTitle_label->setEnabled( false );
#else
	// Set provisional titles:
	DTCtables_tabWidget->setTabText(0, tr("Temporary DTCs"));
	DTCtables_tabWidget->setTabText(1, tr("Memorized DTCs"));
	// Disable table tabs:
	DTCtables_tabWidget->setTabEnabled(0, false);
	DTCtables_tabWidget->setTabEnabled(1, false);
#endif
	// Disable the DTC tables:
	currOrTempDTCs_tableWidget->setEnabled( false );
	histOrMemDTCs_tableWidget->setEnabled( false );
#ifndef SMALL_RESOLUTION
	// Disable "print"-button:
	printDClist_pushButton->setDisabled(true);
#endif
}


CUcontent_DCs_twoMemories::~CUcontent_DCs_twoMemories()
{
#ifndef SMALL_RESOLUTION
	disconnect(printDClist_pushButton, SIGNAL( released() ), this, SLOT( printDCprotocol() ));
#endif
	disconnectGUIelements();
}


bool CUcontent_DCs_twoMemories::setup(SSMprotocol *SSMPdev)
{
	bool ok = false;
	bool obd2DTCformat = true;
	bool currOrTempDTCs_sup = false;
	bool histOrMemDTCs_sup = false;
	QString title;

	_SSMPdev = SSMPdev;
	// Reset data:
	_supportedDCgroups = SSMprotocol::noDCs_DCgroup;
	_currOrTempDTCs.clear();
	_currOrTempDTCdescriptions.clear();
	_histOrMemDTCs.clear();
	_histOrMemDTCdescriptions.clear();
	// Get CU information:
	ok = (_SSMPdev != NULL);
	if (ok)
		ok =_SSMPdev->getSupportedDCgroups(&_supportedDCgroups);
	if (ok)
	{
		if ((_supportedDCgroups & SSMprotocol::currentDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::historicDTCs_DCgroup))
			obd2DTCformat = false;
		currOrTempDTCs_sup = ((_supportedDCgroups & SSMprotocol::currentDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::temporaryDTCs_DCgroup));
		histOrMemDTCs_sup = ((_supportedDCgroups & SSMprotocol::historicDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::memorizedDTCs_DCgroup));
	}
#ifndef SMALL_RESOLUTION
	// DTC-table titles:
	if ( obd2DTCformat )
		title = tr("Temporary Diagnostic Trouble Code(s):");
	else
		title = tr("Current Diagnostic Trouble Code(s):");
	currOrTempDTCsTitle_label->setText( title );
	if ( obd2DTCformat )
		title = tr("Memorized Diagnostic Trouble Code(s):");
	else
		title = tr("Historic Diagnostic Trouble Code(s):");
	histOrMemDTCsTitle_label->setText( title );
	currOrTempDTCsTitle_label->setEnabled(currOrTempDTCs_sup);
	histOrMemDTCsTitle_label->setEnabled(histOrMemDTCs_sup);
#else
	// DTC-table-tab titles:
	if ( obd2DTCformat )
		title = tr("Temporary DTCs");
	else
		title = tr("Current DTCs");
	DTCtables_tabWidget->setTabText(0, title);
	if ( obd2DTCformat )
		title = tr("Memorized DTCs");
	else
		title = tr("Historic DTCs");
	DTCtables_tabWidget->setTabText(1, title);
	DTCtables_tabWidget->setTabEnabled(0, currOrTempDTCs_sup);
	DTCtables_tabWidget->setTabEnabled(1, histOrMemDTCs_sup);
#endif
	// DTC-tables:
	if (ok && !currOrTempDTCs_sup)
		setDCtableContent(currOrTempDTCs_tableWidget, QStringList(""), QStringList(tr("----- Not supported by ECU -----")));
	else
		setDCtableContent(currOrTempDTCs_tableWidget, QStringList(""), QStringList(""));
	currOrTempDTCs_tableWidget->setEnabled(currOrTempDTCs_sup);
	if (ok && !histOrMemDTCs_sup)
		setDCtableContent(histOrMemDTCs_tableWidget, QStringList(""), QStringList(tr("----- Not supported by ECU -----")));
	else
		setDCtableContent(histOrMemDTCs_tableWidget, QStringList(""), QStringList(""));
	histOrMemDTCs_tableWidget->setEnabled(histOrMemDTCs_sup);
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


void CUcontent_DCs_twoMemories::connectGUIelements()
{
	if (!_SSMPdev) return;
	// DTCs:   disable tables of unsupported DTCs, initial output, connect slots:
	if ((_supportedDCgroups & SSMprotocol::currentDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::temporaryDTCs_DCgroup))
	{
		updateCurrentOrTemporaryDTCsContent(QStringList(""), QStringList(tr("----- Reading data... Please wait ! -----")));
		connect(_SSMPdev, SIGNAL( currentOrTemporaryDTCs(QStringList, QStringList, bool, bool) ), this, SLOT( updateCurrentOrTemporaryDTCsContent(QStringList, QStringList) ));
	}
	if ((_supportedDCgroups & SSMprotocol::historicDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::memorizedDTCs_DCgroup))
	{
		updateHistoricOrMemorizedDTCsContent(QStringList(""), QStringList(tr("----- Reading data... Please wait ! -----")));
		connect(_SSMPdev, SIGNAL( historicOrMemorizedDTCs(QStringList, QStringList) ), this, SLOT( updateHistoricOrMemorizedDTCsContent(QStringList, QStringList) ));
	}
#ifndef SMALL_RESOLUTION
	// Connect and disable print-button temporary (until all memories have been read once):
	printDClist_pushButton->setDisabled(true);
	connect(printDClist_pushButton, SIGNAL( released() ), this, SLOT( printDCprotocol() ));
#endif
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
}


void CUcontent_DCs_twoMemories::disconnectGUIelements()
{
	if (!_SSMPdev) return;	// avoid NULL-pointer-warning-message
	disconnect(_SSMPdev, SIGNAL( currentOrTemporaryDTCs(QStringList, QStringList, bool, bool) ), this, SLOT( updateCurrentOrTemporaryDTCsContent(QStringList, QStringList) ));
	disconnect(_SSMPdev, SIGNAL( historicOrMemorizedDTCs(QStringList, QStringList) ), this, SLOT( updateHistoricOrMemorizedDTCsContent(QStringList, QStringList) ));
}


void CUcontent_DCs_twoMemories::updateCurrentOrTemporaryDTCsContent(QStringList currOrTempDTCs, QStringList currOrTempDTCdescriptions)
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
			currOrTempDTCdescriptions << tr("----- No Trouble Codes -----");
		}
		setDCtableContent(currOrTempDTCs_tableWidget, currOrTempDTCs, currOrTempDTCdescriptions);
#ifndef SMALL_RESOLUTION
		// Activate "Print" button:
		printDClist_pushButton->setEnabled(true);
#endif

	}
}


void CUcontent_DCs_twoMemories::updateHistoricOrMemorizedDTCsContent(QStringList histOrMemDTCs, QStringList histOrMemDTCdescriptions)
{
	if ((histOrMemDTCs != _histOrMemDTCs) || (histOrMemDTCdescriptions != _histOrMemDTCdescriptions))
	{
		// Save Trouble Codes:
		_histOrMemDTCs = histOrMemDTCs;
		_histOrMemDTCdescriptions = histOrMemDTCdescriptions;
		// Output Trouble Codes:
		if ((histOrMemDTCs.size() == 0) && (histOrMemDTCdescriptions.size() == 0))
		{
			histOrMemDTCs << "";
			histOrMemDTCdescriptions << tr("----- No Trouble Codes -----");
		}
		setDCtableContent(histOrMemDTCs_tableWidget, histOrMemDTCs, histOrMemDTCdescriptions);
#ifndef SMALL_RESOLUTION
		// Activate "Print" button:
		printDClist_pushButton->setEnabled(true);
#endif
	}
}


#ifndef SMALL_RESOLUTION
void CUcontent_DCs_twoMemories::createDCprintTables(QTextCursor cursor)
{
	QStringList currOrTempDTCcodes = _currOrTempDTCs;
	QStringList currOrTempDTCdescriptions = _currOrTempDTCdescriptions;
	QStringList histOrMemDTCcodes = _histOrMemDTCs;
	QStringList histOrMemDTCdescriptions = _histOrMemDTCdescriptions;
	// Current/Temporary DTCs:
	if ((_supportedDCgroups & SSMprotocol::currentDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::temporaryDTCs_DCgroup))
	{
		if (_currOrTempDTCdescriptions.size() == 0)
		{
			currOrTempDTCcodes << "";
			currOrTempDTCdescriptions << tr("----- No Trouble Codes -----");
		}
		// Insert table with current/temporary DTCs into text document:
		insertDCprintTable(cursor, currOrTempDTCsTitle_label->text(), currOrTempDTCcodes, currOrTempDTCdescriptions);
	}
	// Historic/Memorized DTCs:
	if ((_supportedDCgroups & SSMprotocol::historicDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::memorizedDTCs_DCgroup))
	{
		if (_histOrMemDTCdescriptions.size() == 0)
		{
			histOrMemDTCcodes << "";
			histOrMemDTCdescriptions << tr("----- No Trouble Codes -----");
		}
		// Insert table with historic/memorized DTCs into text document:
		insertDCprintTable(cursor, histOrMemDTCsTitle_label->text(), histOrMemDTCcodes, histOrMemDTCdescriptions);
	}
}
#endif


void CUcontent_DCs_twoMemories::resizeEvent(QResizeEvent *event)
{
	// Calculate and set number of table rows:
	setNrOfTableRows(currOrTempDTCs_tableWidget, _currOrTempDTCs.size() );
	setNrOfTableRows(histOrMemDTCs_tableWidget, _histOrMemDTCs.size() );
	// Accept event:
	event->accept();
}


bool CUcontent_DCs_twoMemories::eventFilter(QObject *obj, QEvent *event)
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
		else if (obj == histOrMemDTCs_tableWidget->viewport())
		{
			if (histOrMemDTCs_tableWidget->verticalScrollBarPolicy() ==  Qt::ScrollBarAlwaysOff)
				return true;	// filter out
			else
				return false;
		}
	}
	// Pass the event on to the parent class
	return QWidget::eventFilter(obj, event);
}
