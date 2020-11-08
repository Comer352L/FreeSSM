/*
 * CUcontent_DCs_engine.cpp - Widget for ECU Diagnostic Codes Reading
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

#include "CUcontent_DCs_engine.h"


CUcontent_DCs_engine::CUcontent_DCs_engine(QWidget *parent) : CUcontent_DCs_abstract(parent)
{
	_obd2DTCformat = true;
	_testMode = false;
	_DCheckActive = false;
	_currOrTempDTCs.clear();
	_currOrTempDTCdescriptions.clear();
	_histOrMemDTCs.clear();
	_histOrMemDTCdescriptions.clear();
	_latestCCCCs.clear();
	_latestCCCCdescriptions.clear();
	_memorizedCCCCs.clear();
	_memorizedCCCCdescriptions.clear();
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
	latestCCCCs_tableWidget->setColumnWidth (0, 70);
	headerview = latestCCCCs_tableWidget->horizontalHeader();
#if QT_VERSION < 0x050000
	headerview->setResizeMode(0,QHeaderView::Interactive);
	headerview->setResizeMode(1,QHeaderView::Stretch);
#else
	headerview->setSectionResizeMode(0,QHeaderView::Interactive);
	headerview->setSectionResizeMode(1,QHeaderView::Stretch);
#endif
	memorizedCCCCs_tableWidget->setColumnWidth (0, 70);
	headerview = memorizedCCCCs_tableWidget->horizontalHeader();
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
	headerview = latestCCCCs_tableWidget->verticalHeader();
#if QT_VERSION < 0x050000
	headerview->setResizeMode(QHeaderView::Fixed);
#else
	headerview->setSectionResizeMode(QHeaderView::Fixed);
#endif
	headerview = memorizedCCCCs_tableWidget->verticalHeader();
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
	latestCCCCs_tableWidget->viewport()->installEventFilter(this);
	memorizedCCCCs_tableWidget->viewport()->installEventFilter(this);
	// *** Set initial content ***:
	// Set provisional titles:
	currOrTempDTCsTitle_label->setText( tr("Temporary Diagnostic Trouble Code(s):") );
	histOrMemDTCsTitle_label->setText( tr("Memorized Diagnostic Trouble Code(s):") );
	// Disable tables and their titles:
	currOrTempDTCsTitle_label->setEnabled( false );
	currOrTempDTCs_tableWidget->setEnabled( false );
	histOrMemDTCsTitle_label->setEnabled( false );
	histOrMemDTCs_tableWidget->setEnabled( false );
	latestCCCCsTitle_label->setEnabled( false );
	latestCCCCs_tableWidget->setEnabled( false );
	memorizedCCCCsTitle_label->setEnabled( false );
	memorizedCCCCs_tableWidget->setEnabled( false );
#ifndef SMALL_RESOLUTION
	// Disable "Cruise Control"-tab:
	DCgroups_tabWidget->setTabEnabled(1, false);
	// Disable "print"-button:
	printDClist_pushButton->setDisabled(true);
#else
	// Disable "Cruise Control"-tabs:
	DCgroups_tabWidget->setTabEnabled(2, false);
	DCgroups_tabWidget->setTabEnabled(3, false);
#endif
}


CUcontent_DCs_engine::~CUcontent_DCs_engine()
{
#ifndef SMALL_RESOLUTION
	disconnect(printDClist_pushButton, SIGNAL( released() ), this, SLOT( printDCprotocol() ));
#endif
	disconnectGUIelements();
}


bool CUcontent_DCs_engine::setup(SSMprotocol *SSMPdev)
{
	bool ok = false;
	bool TMsup = false;
	bool currOrTempDTCs_sup = false;
	bool histOrMemDTCs_sup = false;
	bool latestCCCCs_sup = false;
	bool memCCCCs_sup = false;
	QString title;

	_SSMPdev = SSMPdev;
	// Reset data:
	_obd2DTCformat = true;
	_testMode = false;
	_DCheckActive = false;
	_supportedDCgroups = SSMprotocol::noDCs_DCgroup;
	_currOrTempDTCs.clear();
	_currOrTempDTCdescriptions.clear();
	_histOrMemDTCs.clear();
	_histOrMemDTCdescriptions.clear();
	_latestCCCCs.clear();
	_latestCCCCdescriptions.clear();
	_memorizedCCCCs.clear();
	_memorizedCCCCdescriptions.clear();
	// Get CU information:
	ok = (_SSMPdev != NULL);
	if (ok)
		ok =_SSMPdev->getSupportedDCgroups(&_supportedDCgroups);
	if (ok)
	{
		ok = _SSMPdev->hasTestMode(&TMsup);
		if (ok && TMsup)
			ok = _SSMPdev->isInTestMode(&_testMode); // NOTE: CURRENTLY, THIS WILL FAIL IF DC-READING IS ALREADY IN PROGRESS
	}
	if (ok)
	{
		if ((_supportedDCgroups & SSMprotocol::currentDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::historicDTCs_DCgroup))
			_obd2DTCformat = false;
		currOrTempDTCs_sup = ((_supportedDCgroups & SSMprotocol::currentDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::temporaryDTCs_DCgroup));
		histOrMemDTCs_sup = ((_supportedDCgroups & SSMprotocol::historicDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::memorizedDTCs_DCgroup));
		latestCCCCs_sup = (_supportedDCgroups & SSMprotocol::CClatestCCs_DCgroup);
		memCCCCs_sup = (_supportedDCgroups & SSMprotocol::CCmemorizedCCs_DCgroup);
	}
	// Set titles of the DTC-tables
	setTitleOfFirstDTCtable(_obd2DTCformat, _testMode);
	if ( _obd2DTCformat )
		title = tr("Memorized Diagnostic Trouble Code(s):");
	else
		title = tr("Historic Diagnostic Trouble Code(s):");
	histOrMemDTCsTitle_label->setText( title );
	// DC-tables and titles:
	if (ok && !currOrTempDTCs_sup)
		setDCtableContent(currOrTempDTCs_tableWidget, QStringList(""), QStringList(tr("----- Not supported by ECU -----")));
	else
		setDCtableContent(currOrTempDTCs_tableWidget, QStringList(""), QStringList(""));
	currOrTempDTCsTitle_label->setEnabled(currOrTempDTCs_sup);
	currOrTempDTCs_tableWidget->setEnabled(currOrTempDTCs_sup);
	if (ok && !histOrMemDTCs_sup)
		setDCtableContent(histOrMemDTCs_tableWidget, QStringList(""), QStringList(tr("----- Not supported by ECU -----")));
	else
		setDCtableContent(histOrMemDTCs_tableWidget, QStringList(""), QStringList(""));
	histOrMemDTCsTitle_label->setEnabled(histOrMemDTCs_sup);
	histOrMemDTCs_tableWidget->setEnabled(histOrMemDTCs_sup);
	if (ok && !latestCCCCs_sup)
		setDCtableContent(latestCCCCs_tableWidget, QStringList(""), QStringList(tr("----- Not supported by ECU -----")));
	else
		setDCtableContent(latestCCCCs_tableWidget, QStringList(""), QStringList(""));
	latestCCCCsTitle_label->setEnabled(latestCCCCs_sup);
	latestCCCCs_tableWidget->setEnabled(latestCCCCs_sup);
	if (ok && !memCCCCs_sup)
		setDCtableContent(memorizedCCCCs_tableWidget, QStringList(""), QStringList(tr("----- Not supported by ECU -----")));
	else
		setDCtableContent(memorizedCCCCs_tableWidget, QStringList(""), QStringList(""));
	memorizedCCCCsTitle_label->setEnabled(memCCCCs_sup);
	memorizedCCCCs_tableWidget->setEnabled(memCCCCs_sup);
#ifndef SMALL_RESOLUTION
	// Deactivate and disconnect "Print"-button:
	printDClist_pushButton->setEnabled(false);
	disconnect(printDClist_pushButton, SIGNAL( released() ), this, SLOT( printDCprotocol() ));
#endif
	// Enable/disable "Cruise Control"-tab(s):
	if (ok && (latestCCCCs_sup || memCCCCs_sup))
	{
#ifndef SMALL_RESOLUTION
		DCgroups_tabWidget->setTabEnabled(1, true);
#else
		DCgroups_tabWidget->setTabEnabled(2, true);
		DCgroups_tabWidget->setTabEnabled(3, true);
#endif
	}
	else
	{
		DCgroups_tabWidget->setCurrentIndex(0);
#ifndef SMALL_RESOLUTION
		DCgroups_tabWidget->setTabEnabled(1, false);
#else
		DCgroups_tabWidget->setTabEnabled(2, false);
		DCgroups_tabWidget->setTabEnabled(3, false);
#endif
	}
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


void CUcontent_DCs_engine::connectGUIelements()
{
	if (!_SSMPdev) return;
	// DTCs:   disable tables of unsupported DTCs, initial output, connect slots:
	if ((_supportedDCgroups & SSMprotocol::currentDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::temporaryDTCs_DCgroup))
	{
		updateCurrentOrTemporaryDTCsContent(QStringList(""), QStringList(tr("----- Reading data... Please wait ! -----")), _testMode, false);
		connect(_SSMPdev, SIGNAL( currentOrTemporaryDTCs(QStringList, QStringList, bool, bool) ), this, SLOT( updateCurrentOrTemporaryDTCsContent(QStringList, QStringList, bool, bool) ));
	}
	if ((_supportedDCgroups & SSMprotocol::historicDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::memorizedDTCs_DCgroup))
	{
		updateHistoricOrMemorizedDTCsContent(QStringList(""), QStringList(tr("----- Reading data... Please wait ! -----")));
		connect(_SSMPdev, SIGNAL( historicOrMemorizedDTCs(QStringList, QStringList) ), this, SLOT( updateHistoricOrMemorizedDTCsContent(QStringList, QStringList) ));
	}
	if (_supportedDCgroups & SSMprotocol::CClatestCCs_DCgroup)
	{
		updateCClatestCCsContent(QStringList(""), QStringList(tr("----- Reading data... Please wait ! -----")));
		connect(_SSMPdev, SIGNAL( latestCCCCs(QStringList, QStringList) ), this, SLOT( updateCClatestCCsContent(QStringList, QStringList) ));
	}
	if (_supportedDCgroups & SSMprotocol::CCmemorizedCCs_DCgroup)
	{
		updateCCmemorizedCCsContent(QStringList(""), QStringList(tr("----- Reading data... Please wait ! -----")));
		connect(_SSMPdev, SIGNAL( memorizedCCCCs(QStringList, QStringList) ), this, SLOT( updateCCmemorizedCCsContent(QStringList, QStringList) ));
	}
#ifndef SMALL_RESOLUTION
	// Connect and disable print-button temporary (until all memories have been read once):
	printDClist_pushButton->setDisabled(true);
	connect(printDClist_pushButton, SIGNAL( released() ), this, SLOT( printDCprotocol() ));
#endif
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
}


void CUcontent_DCs_engine::disconnectGUIelements()
{
	if (!_SSMPdev) return;	// avoid NULL-pointer-warning-message
	disconnect(_SSMPdev, SIGNAL( currentOrTemporaryDTCs(QStringList, QStringList, bool, bool) ), this, SLOT( updateCurrentOrTemporaryDTCsContent(QStringList, QStringList, bool, bool) ));
	disconnect(_SSMPdev, SIGNAL( historicOrMemorizedDTCs(QStringList, QStringList) ), this, SLOT( updateHistoricOrMemorizedDTCsContent(QStringList, QStringList) ));
	disconnect(_SSMPdev, SIGNAL( latestCCCCs(QStringList, QStringList) ), this, SLOT( updateCClatestCCsContent(QStringList, QStringList) ));
	disconnect(_SSMPdev, SIGNAL( memorizedCCCCs(QStringList, QStringList) ), this, SLOT( updateCCmemorizedCCsContent(QStringList, QStringList) ));
}


void CUcontent_DCs_engine::updateCurrentOrTemporaryDTCsContent(QStringList currOrTempDTCs, QStringList currOrTempDTCdescriptions, bool testMode, bool DCheckActive)
{
	// DTC-table title:
	if (testMode != _testMode)
	{
		_testMode = testMode;
		setTitleOfFirstDTCtable(_obd2DTCformat, _testMode);
	}
	// DTCs (table content):
	if ((currOrTempDTCs != _currOrTempDTCs) || (currOrTempDTCdescriptions != _currOrTempDTCdescriptions) || (DCheckActive != _DCheckActive))
	{
		if (DCheckActive)
		{
			currOrTempDTCs = QStringList("");
			currOrTempDTCdescriptions = QStringList( tr("----- SYSTEM CHECK IS NOT YET COMPLETED ! -----") );
		}
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


void CUcontent_DCs_engine::updateHistoricOrMemorizedDTCsContent(QStringList histOrMemDTCs, QStringList histOrMemDTCdescriptions)
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


void CUcontent_DCs_engine::updateCClatestCCsContent(QStringList latestCCCCs, QStringList latestCCCCdescriptions)
{
	if ((latestCCCCs != _latestCCCCs) || (latestCCCCdescriptions != _latestCCCCdescriptions))
	{
		// Save Trouble Codes:
		_latestCCCCs = latestCCCCs;
		_latestCCCCdescriptions = latestCCCCdescriptions;
		// Output Trouble Codes:
		if ((latestCCCCs.size() == 0) && (latestCCCCdescriptions.size() == 0))
		{
			latestCCCCs << "";
			latestCCCCdescriptions << tr("----- No Cancel Codes -----");
		}
		setDCtableContent(latestCCCCs_tableWidget, latestCCCCs, latestCCCCdescriptions);
#ifndef SMALL_RESOLUTION
		// Activate "Print" button:
		printDClist_pushButton->setEnabled(true);
#endif
	}
}


void CUcontent_DCs_engine::updateCCmemorizedCCsContent(QStringList memorizedCCCCs, QStringList memorizedCCCCdescriptions)
{
	if ((memorizedCCCCs != _memorizedCCCCs) || (memorizedCCCCdescriptions != _histOrMemDTCdescriptions))
	{
		// Save Trouble Codes:
		_memorizedCCCCs = memorizedCCCCs;
		_memorizedCCCCdescriptions = memorizedCCCCdescriptions;
		// Output Trouble Codes:
		if ((memorizedCCCCs.size() == 0) && (memorizedCCCCdescriptions.size() == 0))
		{
			memorizedCCCCs << "";
			memorizedCCCCdescriptions << tr("----- No Cancel Codes -----");
		}
		setDCtableContent(memorizedCCCCs_tableWidget, memorizedCCCCs, memorizedCCCCdescriptions);
#ifndef SMALL_RESOLUTION
		// Activate "Print" button:
		printDClist_pushButton->setEnabled(true);
#endif
	}
}


void CUcontent_DCs_engine::setTitleOfFirstDTCtable(bool obd2, bool testMode)
{
	QString title;
	if ( testMode )
	{
		title = tr("System-Check Diagnostic Trouble Code(s):");
	}
	else if ( obd2 )
	{
		title = tr("Temporary Diagnostic Trouble Code(s):");
	}
	else
	{
		title = tr("Current Diagnostic Trouble Code(s):");
	}
	currOrTempDTCsTitle_label->setText( title );
}


void CUcontent_DCs_engine::setNrOfRowsOfAllTableWidgets()
{
	int currentindex = DCgroups_tabWidget->currentIndex();
	// Calculate and set number of table rows:
	DCgroups_tabWidget->setCurrentIndex(0);
	setNrOfTableRows(currOrTempDTCs_tableWidget, _currOrTempDTCs.size() );
	setNrOfTableRows(histOrMemDTCs_tableWidget, _histOrMemDTCs.size() );
	DCgroups_tabWidget->setCurrentIndex(1);
	setNrOfTableRows(latestCCCCs_tableWidget, _latestCCCCs.size() );
	setNrOfTableRows(memorizedCCCCs_tableWidget, _memorizedCCCCs.size() );
	DCgroups_tabWidget->setCurrentIndex(currentindex);
	/* NOTE: Switching the tabs is a "dirty" workaround for a Qt-issue:
	 * for all tabs except the current tab, the returned table height is wrong.
	 * Fortunately, the switch is not visible...
	 */
}


#ifndef SMALL_RESOLUTION
void CUcontent_DCs_engine::createDCprintTables(QTextCursor cursor)
{
	QStringList currOrTempDTCcodes = _currOrTempDTCs;
	QStringList currOrTempDTCdescriptions = _currOrTempDTCdescriptions;
	QStringList histOrMemDTCcodes = _histOrMemDTCs;
	QStringList histOrMemDTCdescriptions = _histOrMemDTCdescriptions;
	QStringList latestCCCCcodes = _latestCCCCs;
	QStringList latestCCCCdescriptions = _latestCCCCdescriptions;
	QStringList memorizedCCCCcodes = _memorizedCCCCs;
	QStringList memorizedCCCCdescriptions = _memorizedCCCCdescriptions;
	// Current/Temporary DTCs:
	if ((_supportedDCgroups & SSMprotocol::currentDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::temporaryDTCs_DCgroup))
	{
		if (currOrTempDTCdescriptions.size() == 0)
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
		if (histOrMemDTCdescriptions.size() == 0)
		{
			histOrMemDTCcodes << "";
			histOrMemDTCdescriptions << tr("----- No Trouble Codes -----");
		}
		// Insert table with historic/memorized DTCs into text document:
		insertDCprintTable(cursor, histOrMemDTCsTitle_label->text(), histOrMemDTCcodes, histOrMemDTCdescriptions);
	}
	// Latest Cancel Codes:
	if (_supportedDCgroups & SSMprotocol::CClatestCCs_DCgroup)
	{
		if (latestCCCCdescriptions.size() == 0)
		{
			latestCCCCcodes << "";
			latestCCCCdescriptions << tr("----- No Cancel Codes -----");
		}
		// Insert table with latest CCs into text document:
		insertDCprintTable(cursor, latestCCCCsTitle_label->text(), latestCCCCcodes, latestCCCCdescriptions);
	}
	// Memorized Cancel Codes:
	if (_supportedDCgroups & SSMprotocol::CCmemorizedCCs_DCgroup)
	{
		if (memorizedCCCCdescriptions.size() == 0)
		{
			memorizedCCCCcodes << "";
			memorizedCCCCdescriptions << tr("----- No Cancel Codes -----");
		}
		// Insert table with memorized CCs into text document:
		insertDCprintTable(cursor, memorizedCCCCsTitle_label->text(), memorizedCCCCcodes, memorizedCCCCdescriptions);
	}
}
#endif


void CUcontent_DCs_engine::resizeEvent(QResizeEvent *event)
{
	// Set nr of rows of all teble-widgets:
	setNrOfRowsOfAllTableWidgets();
	// accept event:
	event->accept();
}


bool CUcontent_DCs_engine::eventFilter(QObject *obj, QEvent *event)
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
		else if (obj == latestCCCCs_tableWidget->viewport())
		{
			if (latestCCCCs_tableWidget->verticalScrollBarPolicy() ==  Qt::ScrollBarAlwaysOff)
				return true;	// filter out
			else
				return false;
		}
		else if (obj == memorizedCCCCs_tableWidget->viewport())
		{
			if (memorizedCCCCs_tableWidget->verticalScrollBarPolicy() ==  Qt::ScrollBarAlwaysOff)
				return true;	// filter out
			else
				return false;
		}
	}
	// Pass the event on to the parent class
	return QWidget::eventFilter(obj, event);
}


void CUcontent_DCs_engine::show()
{
	// call original implementation of show():
	QWidget::show();
	// Set nr of rows of all table widgets:
	setNrOfRowsOfAllTableWidgets();
	/* NOTE: this has do be done because QTableWidget::hight() returns wrong values
	 * while table-widgets were not visible yet (but already set up !)
	 * This seems to be a Qt-Bug (found in 4.4.1); This seems to happen only in combination
	 * with a QTabWidget...
	 * Alternative solution: call show() already in the constructor
	 */
}

