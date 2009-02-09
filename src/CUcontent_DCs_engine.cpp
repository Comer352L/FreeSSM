/*
 * CUcontent_DCs_engine.cpp - Widget for ECU Diagnostic Codes Reading
 *
 * Copyright © 2008-2009 Comer352l
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


CUcontent_DCs_engine::CUcontent_DCs_engine(QWidget *parent, SSMprotocol *SSMPdev, QString progversion) : QWidget(parent)
{
	_SSMPdev = SSMPdev;
	_progversion = progversion;
	_supportedDCgroups = 0;
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
	setupUiFonts();
	// Set column widths:
	QHeaderView *headerview;
	currOrTempDTCs_tableWidget->setColumnWidth (0, 70);
	headerview = currOrTempDTCs_tableWidget->horizontalHeader();
	headerview->setResizeMode(0,QHeaderView::Interactive);
	headerview->setResizeMode(1,QHeaderView::Stretch);
	histOrMemDTCs_tableWidget->setColumnWidth (0, 70);
	headerview = histOrMemDTCs_tableWidget->horizontalHeader();
	headerview->setResizeMode(0,QHeaderView::Interactive);
	headerview->setResizeMode(1,QHeaderView::Stretch);
	latestCCCCs_tableWidget->setColumnWidth (0, 70);
	headerview = latestCCCCs_tableWidget->horizontalHeader();
	headerview->setResizeMode(0,QHeaderView::Interactive);
	headerview->setResizeMode(1,QHeaderView::Stretch);
	memorizedCCCCs_tableWidget->setColumnWidth (0, 70);
	headerview = memorizedCCCCs_tableWidget->horizontalHeader();
	headerview->setResizeMode(0,QHeaderView::Interactive);
	headerview->setResizeMode(1,QHeaderView::Stretch);
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
	// Disable "Cruise Control"-tab:
	DCgroups_tabWidget->setTabEnabled(1, false);
	// Disable "print"-button:
	printDClist_pushButton->setDisabled(true);
}


CUcontent_DCs_engine::~CUcontent_DCs_engine()
{
	stopDCreading();
	disconnect(_SSMPdev, SIGNAL( startedDCreading() ), this, SLOT( callStart() ));
	disconnect(_SSMPdev, SIGNAL( stoppedDCreading() ), this, SLOT( callStop() ));
	disconnect(printDClist_pushButton, SIGNAL( pressed() ), this, SLOT( printDCprotocol() ));
	disconnect(_SSMPdev, SIGNAL( currentOrTemporaryDTCs(QStringList, QStringList, bool, bool) ), this, SLOT( updateCurrentOrTemporaryDTCsContent(QStringList, QStringList, bool, bool) ));
	disconnect(_SSMPdev, SIGNAL( historicOrMemorizedDTCs(QStringList, QStringList) ), this, SLOT( updateHistoricOrMemorizedDTCsContent(QStringList, QStringList) ));
	disconnect(_SSMPdev, SIGNAL( latestCCCCs(QStringList, QStringList) ), this, SLOT( updateCClatestCCsContent(QStringList, QStringList) ));
	disconnect(_SSMPdev, SIGNAL( memorizedCCCCs(QStringList, QStringList) ), this, SLOT( updateCCmemorizedCCsContent(QStringList, QStringList) ));
}


bool CUcontent_DCs_engine::setup()
{
	bool ok = false;
	bool TMsup = false;
	bool currOrTempDTCs_sup = false;
	bool histOrMemDTCs_sup = false;
	bool latestCCCCs_sup = false;
	bool memCCCCs_sup = false;
	QString title;

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
	// Deactivate and disconnect "Print"-button:
	printDClist_pushButton->setEnabled(false);
	disconnect(printDClist_pushButton, SIGNAL( pressed() ), this, SLOT( printDCprotocol() ));
	// Enable/disable "Cruise Control"-tab:
	if (ok && (latestCCCCs_sup || memCCCCs_sup))
		DCgroups_tabWidget->setTabEnabled(1, true);
	else
	{
		DCgroups_tabWidget->setCurrentIndex(0);
		DCgroups_tabWidget->setTabEnabled(1, false);
	}
	// Connect start-slot:
	if (ok && (_supportedDCgroups != SSMprotocol::noDCs_DCgroup))
		connect(_SSMPdev, SIGNAL( startedDCreading() ), this, SLOT( callStart() ));
	else
		disconnect(_SSMPdev, SIGNAL( startedDCreading() ), this, SLOT( callStart() ));
	// Return result;
	return ok;
}


void CUcontent_DCs_engine::callStart()
{
	if (!startDCreading())
		communicationError(tr("Couldn't start Diagnostic Codes Reading."));
}


void CUcontent_DCs_engine::callStop()
{
	if (!stopDCreading())
		communicationError(tr("Couldn't stop Diagnostic Codes Reading."));
}


bool CUcontent_DCs_engine::startDCreading()
{
	SSMprotocol::state_dt state = SSMprotocol::state_needSetup;
	int selDCgroups = 0;
	// Check if DC-group(s) selected:
	if (_supportedDCgroups == SSMprotocol::noDCs_DCgroup)
		return false;
	// Check if DC-reading is startable or already in progress:
	state = _SSMPdev->state();
	if (state == SSMprotocol::state_normal)
	{
		disconnect(_SSMPdev, SIGNAL( startedDCreading() ), this, SLOT( callStart() ));
		// Start DC-reading:
		if (!_SSMPdev->startDCreading( _supportedDCgroups ))
		{
			connect(_SSMPdev, SIGNAL( startedDCreading() ), this, SLOT( callStart() ));
			return false;
		}
	}
	else if (state == SSMprotocol::state_DCreading)
	{
		// Verify consistency:
		if (!_SSMPdev->getLastDCgroupsSelection(&selDCgroups))
			return false;
		if (selDCgroups != _supportedDCgroups)  // inconsistency detected !
		{
			// Stop DC-reading:
			stopDCreading();
			return false;
		}
	}
	else
		return false;
	// Enable notification about external DC-reading-stops:
	connect(_SSMPdev, SIGNAL( stoppedDCreading() ), this, SLOT( callStop() ));
	// DTCs:   disable tables of unsupported DTCs, initial output, connect slots:
	if (_supportedDCgroups & SSMprotocol::temporaryDTCs_DCgroup)
	{
		updateCurrentOrTemporaryDTCsContent(QStringList(""), QStringList(tr("----- Reading data... Please wait ! -----")), _testMode, false);
		connect(_SSMPdev, SIGNAL( currentOrTemporaryDTCs(QStringList, QStringList, bool, bool) ), this, SLOT( updateCurrentOrTemporaryDTCsContent(QStringList, QStringList, bool, bool) ));
	}
	if (_supportedDCgroups & SSMprotocol::memorizedDTCs_DCgroup)
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
	// Connect and disable print-button temporary (until all memories have been read once):
	printDClist_pushButton->setDisabled(true);
	connect(printDClist_pushButton, SIGNAL( pressed() ), this, SLOT( printDCprotocol() ));
	return true;
}


bool CUcontent_DCs_engine::stopDCreading()
{
	disconnect(_SSMPdev, SIGNAL( stoppedDCreading() ), this, SLOT( callStop() )); // this must be done BEFORE calling _SSMPdev->stopDCreading() !
	if (_SSMPdev->state() == SSMprotocol::state_DCreading)
	{
		if (!_SSMPdev->stopDCreading())
		{
			connect(_SSMPdev, SIGNAL( stoppedDCreading() ), this, SLOT( callStop() )); // this must be done BEFORE calling _SSMPdev->stopDCreading() !
			return false;
		}
	}
	connect(_SSMPdev, SIGNAL( startedDCreading() ), this, SLOT( callStart() ));
	disconnect(_SSMPdev, SIGNAL( currentOrTemporaryDTCs(QStringList, QStringList, bool, bool) ), this, SLOT( updateCurrentOrTemporaryDTCsContent(QStringList, QStringList, bool, bool) ));
	disconnect(_SSMPdev, SIGNAL( historicOrMemorizedDTCs(QStringList, QStringList) ), this, SLOT( updateHistoricOrMemorizedDTCsContent(QStringList, QStringList) ));
	disconnect(_SSMPdev, SIGNAL( latestCCCCs(QStringList, QStringList) ), this, SLOT( updateCClatestCCsContent(QStringList, QStringList) ));
	disconnect(_SSMPdev, SIGNAL( memorizedCCCCs(QStringList, QStringList) ), this, SLOT( updateCCmemorizedCCsContent(QStringList, QStringList) ));
	return true;
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
		// Activate "Print" button:
		printDClist_pushButton->setEnabled(true);
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
		// Activate "Print" button:
		printDClist_pushButton->setEnabled(true);
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
		// Activate "Print" button:
		printDClist_pushButton->setEnabled(true);
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
		// Activate "Print" button:
		printDClist_pushButton->setEnabled(true);
	}
}


void CUcontent_DCs_engine::setDCtableContent(QTableWidget *tableWidget, QStringList DCs, QStringList DCdescriptions)
{
	int k = 0;
	QTableWidgetItem *tableelement;
	// Delete table content:
	tableWidget->clearContents();
	// Set number of rows:
	setNrOfTableRows(tableWidget, DCdescriptions.size());
	// Fill Table:
	for (k=0; k<DCs.size(); k++)
	{
		// Output DC:
		tableelement = new QTableWidgetItem(DCs.at(k));
		tableelement->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		tableWidget->setItem(k, 0, tableelement);
		// Output DC description:
		tableelement = new QTableWidgetItem(DCdescriptions.at(k));
		tableWidget->setItem(k, 1, tableelement);
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


void CUcontent_DCs_engine::setNrOfTableRows(QTableWidget *tablewidget, unsigned int nrofUsedRows)
{
	// NOTE: this function doesn't change the table's content ! Min. 1 row !
	int rowheight = 0;
	int vspace = 0;
	QHeaderView *headerview;
	unsigned int minnrofrows = 0;
	// Get available vertical space (for rows) and height per row:
	if (tablewidget->rowCount() < 1)
		tablewidget->setRowCount(1); // temporary create a row to get the row hight
	rowheight = tablewidget->rowHeight(0);
	headerview = tablewidget->horizontalHeader();
	vspace = tablewidget->viewport()->height();
	// Temporary switch to "Scroll per Pixel"-mode to ensure auto-scroll (prevent white space between bottom of the last row and the lower table border)
	tablewidget->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
	// Calculate and set nr. of rows:
	minnrofrows = static_cast<unsigned int>(trunc((vspace-1)/rowheight) + 1);
	if (minnrofrows < nrofUsedRows)
		minnrofrows = nrofUsedRows;
	tablewidget->setRowCount(minnrofrows);
	// Set vertical scroll bar policy:
	if (minnrofrows > nrofUsedRows)
		tablewidget->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	else
		tablewidget->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	// Switch back to "Scroll per Item"-mode:
	tablewidget->setVerticalScrollMode( QAbstractItemView::ScrollPerItem ); // auto-scroll is triggered; Maybe this is a Qt-Bug, we don't want that here...
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


void CUcontent_DCs_engine::printDCprotocol()
{
	QString datetime;
	QString CU;
	QString systype;
	QString ROM_ID;
	QString VIN;
	QStringList currOrTempDTCcodes = _currOrTempDTCs;
	QStringList currOrTempDTCdescriptions = _currOrTempDTCdescriptions;
	QStringList histOrMemDTCcodes = _histOrMemDTCs;
	QStringList histOrMemDTCdescriptions = _histOrMemDTCdescriptions;
	QStringList latestCCCCcodes = _latestCCCCs;
	QStringList latestCCCCdescriptions = _latestCCCCdescriptions;
	QStringList memorizedCCCCcodes = _memorizedCCCCs;
	QStringList memorizedCCCCdescriptions = _memorizedCCCCdescriptions;
	bool VINsup = false;
	bool ok = false;
	QString errstr = "";

	// Create Printer:
	QPrinter printer(QPrinter::ScreenResolution);
	// Show print dialog:
	QPrintDialog printDialog(&printer, this);
	if (printDialog.exec() != QDialog::Accepted)
		return;
	// Show print status message:
	QMessageBox printmbox( QMessageBox::NoIcon, tr("Printing..."), "\n" + tr("Printing... Please wait !    "), QMessageBox::NoButton, this);
	printmbox.setStandardButtons(QMessageBox::NoButton);
	QPixmap printicon(QString::fromUtf8(":/icons/chrystal/32x32/fileprint"));
	printmbox.setIconPixmap(printicon);
	QFont printmboxfont = printmbox.font();
	printmboxfont.setPixelSize(13);	// 10pts
	printmboxfont.setBold(true);
	printmbox.setFont( printmboxfont );
	printmbox.show();
	// ##### GATHER CU-INFORMATIONS #####
	datetime = QDateTime::currentDateTime().toString("dddd, dd. MMMM yyyy, h:mm") + ":";
	CU = tr("Engine");
	ok = true;
	if (!_SSMPdev->getSystemDescription(&systype))
	{
		QString SYS_ID = "";
		ok = _SSMPdev->getSysID(&SYS_ID);
		if (ok)
			systype = tr("Unknown (") + SYS_ID + ")";	// NOTE: SYS_ID is always available, if CU is initialized/connection is alive
		else
			errstr = tr("Query of the System-ID failed.");
		/* TODO: IMPROVE: use other functions from libID to determine system type 
			 => maybe this should be done in SSMprotocol, too		*/
	}
	if (ok)
	{
		ok = _SSMPdev->getROMID(&ROM_ID);		// NOTE: ROM_ID is always available, if CU is initialized/connection is alive
		if (ok)
		{
			ok = _SSMPdev->hasVINsupport(&VINsup);
			if (ok)
			{
				if ( VINsup )
				{
					// Temporary stop DC-reading for VIN-Query:
					disconnect(_SSMPdev, SIGNAL( stoppedDCreading() ), this, SLOT( callStop() ));
					ok = _SSMPdev->stopDCreading();
					if (ok)
					{
						// Query VIN:
						ok = _SSMPdev->getVIN(&VIN);
						if (ok)
						{
							if (VIN.size() == 0)
								VIN = tr("not programmed yet");
							// Restart DC-reading:
							ok = _SSMPdev->restartDCreading();
							if (ok)
								connect(_SSMPdev, SIGNAL( stoppedDCreading() ), this, SLOT( callStop() ));
							else
								errstr = tr("Couldn't restart Diagnostic Codes Reading.");
						}
						else
							errstr = tr("Query of the VIN failed.");
					}
					else
						errstr = tr("Couldn't stop Diagnostic Codes Reading.");
				}
				else
					VIN = tr("not supported by ECU");
			}
			else
				errstr = tr("Couldn't determine if VIN-registration is supported.");
		}
		else
			errstr = tr("Query of the ROM-ID failed.");
	}
	// Check for communication error:
	if (!ok)
	{
		printmbox.close();
		communicationError(errstr);
		return;
	}
	// ##### CREATE TEXT DOCUMENT #####
	// Create text document and get cursor position:
	QTextDocument textDocument;
	QTextCursor cursor(&textDocument);
	// Define formats:
	QTextBlockFormat blockFormat;
	QTextCharFormat charFormat;
	QTextTableFormat tableFormat;
	// --- TITLE ---
	// Set title format:
	blockFormat.setAlignment(Qt::AlignHCenter);
	charFormat.setFontPointSize(18);
	charFormat.setFontWeight(QFont::Normal);
	charFormat.setFontUnderline(true);
	// Put title into text document:
	cursor.setBlockFormat(blockFormat);
	cursor.setCharFormat(charFormat);
	cursor.insertText("FreeSSM " + _progversion);
	cursor.insertBlock();
	// --- DATE, TIME ---
	// Format date + time:
	blockFormat.setAlignment(Qt::AlignLeft);
	blockFormat.setTopMargin(15);
	charFormat.setFontPointSize(14);
	charFormat.setFontWeight(QFont::Normal);
	charFormat.setFontUnderline(false);
	cursor.setBlockFormat(blockFormat);
	cursor.setCharFormat(charFormat);
	// Put date + time into the text document:
	cursor.insertText(datetime);
	// ----- CU-INFORMATIONS -----
	// -- Create frame for CU-Informations:
	QTextFrameFormat infoFrame;
	infoFrame.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
	infoFrame.setBorder(1);
	infoFrame.setBorderBrush(QBrush(QColor("black"), Qt::SolidPattern));
	infoFrame.setTopMargin(10);
	cursor.insertFrame(infoFrame);
	// -- TABLE OF CU-INFORMATIONS:
	// Set minimal columns widths:
	QVector<QTextLength> widthconstraints(2);
	widthconstraints[0] = QTextLength(QTextLength::PercentageLength, 50);
	widthconstraints[1] = QTextLength(QTextLength::PercentageLength, 50);
	// Set format:
	tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_None);
	tableFormat.setColumnWidthConstraints(widthconstraints);
	tableFormat.setLeftMargin(20);
	// Create and insert table:
	cursor.insertTable(4, 2, tableFormat);
	// Set font format:
	charFormat.setFontPointSize(12);
	charFormat.setFontUnderline(false);
	// Fill table cells:
	// Row 1 - Column 1:
	charFormat.setFontWeight(QFont::Bold);
	cursor.setCharFormat(charFormat);
	cursor.insertText(tr("Control Unit:"));
	cursor.movePosition(QTextCursor::NextBlock,QTextCursor::MoveAnchor,1);
	// Row 1 - Column 2:
	charFormat.setFontWeight(QFont::Normal);
	cursor.setCharFormat(charFormat);
	cursor.insertText(CU);
	cursor.movePosition(QTextCursor::NextBlock,QTextCursor::MoveAnchor,1);
	// Row 2 - Column 1:
	charFormat.setFontWeight(QFont::Bold);
	cursor.setCharFormat(charFormat);
	cursor.insertText(tr("System Type:"));
	cursor.movePosition(QTextCursor::NextBlock,QTextCursor::MoveAnchor,1);
	// Row 2 - Column 2:
	charFormat.setFontWeight(QFont::Normal);
	cursor.setCharFormat(charFormat);
	cursor.insertText(systype);
	cursor.movePosition(QTextCursor::NextBlock,QTextCursor::MoveAnchor,1);
	// Row 3 - Column 1:
	charFormat.setFontWeight(QFont::Bold);
	cursor.setCharFormat(charFormat);
	cursor.insertText(tr("ROM-ID:"));
	cursor.movePosition(QTextCursor::NextBlock,QTextCursor::MoveAnchor,1);
	// Row 3 - Column 2:
	charFormat.setFontWeight(QFont::Normal);
	cursor.setCharFormat(charFormat);
	cursor.insertText(ROM_ID);
	cursor.movePosition(QTextCursor::NextBlock,QTextCursor::MoveAnchor,1);
	// Row 4 - Column 1:
	charFormat.setFontWeight(QFont::Bold);
	cursor.setCharFormat(charFormat);
	cursor.insertText(tr("Registered VIN:"));
	cursor.movePosition(QTextCursor::NextBlock,QTextCursor::MoveAnchor,1);
	// Row 4 - Column 2:
	charFormat.setFontWeight(QFont::Normal);
	cursor.setCharFormat(charFormat);
	cursor.insertText(VIN);
	// Move (out of the table) to the next block:
	cursor.movePosition(QTextCursor::NextBlock,QTextCursor::MoveAnchor,1);
	// Move (out of the frame) to the next block:
	cursor.movePosition(QTextCursor::NextBlock,QTextCursor::MoveAnchor,1);

	// ##### DTC- and CC-tables #####
	// Current/Temporary DTCs:
	if ((_supportedDCgroups & SSMprotocol::currentDTCs_DCgroup) || (_supportedDCgroups & SSMprotocol::temporaryDTCs_DCgroup))
	{
		if (currOrTempDTCdescriptions.size() == 0)
		{
			currOrTempDTCcodes << "";
			currOrTempDTCdescriptions << tr("----- No Trouble Codes -----");
		}
		// Insert table with current/temporary DTCs into text document:
		insertDCtable(cursor, currOrTempDTCsTitle_label->text(), currOrTempDTCcodes, currOrTempDTCdescriptions);
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
		insertDCtable(cursor, histOrMemDTCsTitle_label->text(), histOrMemDTCcodes, histOrMemDTCdescriptions);
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
		insertDCtable(cursor, latestCCCCsTitle_label->text(), latestCCCCcodes, latestCCCCdescriptions);
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
		insertDCtable(cursor, memorizedCCCCsTitle_label->text(), memorizedCCCCcodes, memorizedCCCCdescriptions);
	}
	// Print created text-document:
	textDocument.print(&printer);
	// Delete status message:
	printmbox.close();
}


void CUcontent_DCs_engine::insertDCtable(QTextCursor cursor, QString title, QStringList codes, QStringList descriptions)
{
	QTextTableFormat tableFormat;
	QTextCharFormat charFormat;
	QVector<QTextLength> widthconstraints(2);
	int k = 0;

	// Insert space line:
	cursor.insertBlock();
	// Set minimal column widths:
	widthconstraints.clear();
	widthconstraints.resize(2);
	widthconstraints[0] = QTextLength(QTextLength::PercentageLength,15);
	widthconstraints[1] = QTextLength(QTextLength::PercentageLength,85);
	// Set table format:
	tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_None);
	tableFormat.setColumnWidthConstraints(widthconstraints);
	tableFormat.setLeftMargin(30);
	// Set title:
	charFormat.setFontPointSize(14);
	charFormat.setFontWeight(QFont::Bold);
	charFormat.setFontUnderline(false);
	cursor.setCharFormat(charFormat);
	cursor.insertText( title );
	// Create and insert table:
	cursor.insertTable(codes.size(), 2, tableFormat); // n rows, 2 columns
	// Set font format for DCs:
	charFormat.setFontPointSize(12);
	charFormat.setFontWeight(QFont::Normal);
	charFormat.setFontUnderline(false);
	// Fill table cells:
	for (k=0; k<codes.size(); k++)
	{
		//   Column 1: Code
		cursor.setCharFormat(charFormat);
		cursor.insertText( codes.at(k) );
		cursor.movePosition(QTextCursor::NextBlock,QTextCursor::MoveAnchor,1);
		//   Column 2: Description
		cursor.setCharFormat(charFormat);
		cursor.insertText( descriptions.at(k) );
		cursor.movePosition(QTextCursor::NextBlock,QTextCursor::MoveAnchor,1);
	}
}


void CUcontent_DCs_engine::resizeEvent(QResizeEvent *event)
{
	// Set nr of rows of all teble-widgets:
	setNrOfRowsOfAllTableWidgets();
	// accept event:
	event->accept();
	/* NOTE: Switching the tabs is a "dirty" workaround for a Qt-issue:
	 * for all tabs except the current tab, the returned table height is wrong.
	 * Fortunately, the switch is not visible...
	 */
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


void CUcontent_DCs_engine::communicationError(QString errstr)
{
	QMessageBox msg( QMessageBox::Critical, tr("Communication Error"), tr("Communication Error:") + ('\n') + errstr, QMessageBox::Ok, this);
	QFont msgfont = msg.font();
	msgfont.setPixelSize(12); // 9pts
	msg.setFont( msgfont );
	msg.show();
	msg.exec();
	msg.close();
	emit error();
}


void CUcontent_DCs_engine::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS OF ui_FreeSSM.h (made with QDesigner)
	QFont contentfont = QApplication::font();
	contentfont.setPixelSize(12);// 9pts
	contentfont.setBold(false);
	this->setFont(contentfont);
	// Tab-Widget:
	QFont tabwidgetfont = DCgroups_tabWidget->font();
	tabwidgetfont.setPixelSize(13);
	tabwidgetfont.setBold(true);
	DCgroups_tabWidget->setFont(tabwidgetfont);
	// Tabs:
	engineDTCs_tab->setFont(contentfont);
	CCCCs_tab->setFont(contentfont);
	// Table titles:
	QFont tabletitlefont = contentfont;
	tabletitlefont.setUnderline(true);
	currOrTempDTCsTitle_label->setFont(tabletitlefont);
	histOrMemDTCsTitle_label->setFont(tabletitlefont);
	latestCCCCsTitle_label->setFont(tabletitlefont);
	memorizedCCCCsTitle_label->setFont(tabletitlefont);
	// Tables:
	currOrTempDTCs_tableWidget->setFont(contentfont);
	histOrMemDTCs_tableWidget->setFont(contentfont);
	latestCCCCs_tableWidget->setFont(contentfont);
	memorizedCCCCs_tableWidget->setFont(contentfont);
	// Info about DC-Clearing:
	DCclearingInfo_label->setFont(contentfont);
	// Print-button:
	printDClist_pushButton->setFont(contentfont);
}

