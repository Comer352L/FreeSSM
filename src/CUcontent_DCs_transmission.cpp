/*
 * CUcontent_DCs_transmission.cpp - Widget for TCU Diagnostic Codes Reading
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

#include "CUcontent_DCs_transmission.h"


CUcontent_DCs_transmission::CUcontent_DCs_transmission(QWidget *parent, SSMprotocol *SSMPdev, QString progversion) : QWidget(parent)
{
	_SSMPdev = SSMPdev;
	_progversion = progversion;
	_supportedDCgroups = 0;
	_temporaryDTCs.clear();
	_temporaryDTCdescriptions.clear();
	_memorizedDTCs.clear();
	_memorizedDTCdescriptions.clear();

	// Setup GUI:
	setupUi(this);
	setupUiFonts();
	// Set column widths:
	QHeaderView *headerview;
	temporaryDTCs_tableWidget->setColumnWidth (0, 70);
	headerview = temporaryDTCs_tableWidget->horizontalHeader();
	headerview->setResizeMode(0,QHeaderView::Interactive);
	headerview->setResizeMode(1,QHeaderView::Stretch);
	memorizedDTCs_tableWidget->setColumnWidth (0, 70);
	headerview = memorizedDTCs_tableWidget->horizontalHeader();
	headerview->setResizeMode(0,QHeaderView::Interactive);
	headerview->setResizeMode(1,QHeaderView::Stretch);
	// Install event-filter for DC-tables:
	temporaryDTCs_tableWidget->viewport()->installEventFilter(this);
	memorizedDTCs_tableWidget->viewport()->installEventFilter(this);
	// *** Set initial content ***:
	// Set provisional titles:
	temporaryDTCsTitle_label->setText( tr("Temporary Diagnostic Trouble Code(s):") );
	memorizedDTCsTitle_label->setText( tr("Memorized Diagnostic Trouble Code(s):") );
	// Disable tables and their titles:
	temporaryDTCsTitle_label->setEnabled( false );
	temporaryDTCs_tableWidget->setEnabled( false );
	memorizedDTCsTitle_label->setEnabled( false );
	memorizedDTCs_tableWidget->setEnabled( false );
	// Disable "print"-button:
	printDClist_pushButton->setDisabled(true);
}


CUcontent_DCs_transmission::~CUcontent_DCs_transmission()
{
	stopDCreading();
	disconnect(_SSMPdev, SIGNAL( startedDCreading() ), this, SLOT( callStart() ));
	disconnect(_SSMPdev, SIGNAL( stoppedDCreading() ), this, SLOT( callStop() ));
	disconnect(printDClist_pushButton, SIGNAL( pressed() ), this, SLOT( printDCprotocol() ));
	disconnect(_SSMPdev, SIGNAL( temporaryDTCs(QStringList, QStringList, bool) ), this, SLOT( updateTemporaryDTCsContent(QStringList, QStringList) ));
	disconnect(_SSMPdev, SIGNAL( memorizedDTCs(QStringList, QStringList) ), this, SLOT( updateMemorizedDTCsContent(QStringList, QStringList) ));
}


bool CUcontent_DCs_transmission::setup()
{
	bool ok = false;
	bool obd2 = true;
	bool tempDTCs_sup = false;
	bool memDTCs_sup = false;
	QString title;

	// Reset data:
	_supportedDCgroups = SSMprotocol::noDCs_DCgroup;
	_temporaryDTCs.clear();
	_temporaryDTCdescriptions.clear();
	_memorizedDTCs.clear();
	_memorizedDTCdescriptions.clear();
	// Get CU information:
	ok =_SSMPdev->getSupportedDCgroups(&_supportedDCgroups);
	if (ok)
		ok =_SSMPdev->hasOBD2(&obd2);
	if (ok)
	{
		tempDTCs_sup = (_supportedDCgroups == (_supportedDCgroups | SSMprotocol::temporaryDTCs_DCgroup));
		memDTCs_sup = (_supportedDCgroups == (_supportedDCgroups | SSMprotocol::memorizedDTCs_DCgroup));
	}
	// Set titles of the DTC-tables
	if ( obd2 )
		title = tr("Temporary Diagnostic Trouble Code(s):");
	else
		title = tr("Current Diagnostic Trouble Code(s):");
	temporaryDTCsTitle_label->setText( title );
	if ( obd2 )
		title = tr("Memorized Diagnostic Trouble Code(s):");
	else
		title = tr("Historic Diagnostic Trouble Code(s):");
	memorizedDTCsTitle_label->setText( title );
	// DC-tables and titles:
	if (ok && !tempDTCs_sup)
		setDCtableContent(temporaryDTCs_tableWidget, QStringList(""), QStringList(tr("----- Not supported by ECU -----")));
	else
		setDCtableContent(temporaryDTCs_tableWidget, QStringList(""), QStringList(""));
	temporaryDTCsTitle_label->setEnabled(tempDTCs_sup);
	temporaryDTCs_tableWidget->setEnabled(tempDTCs_sup);
	if (ok && !memDTCs_sup)
		setDCtableContent(memorizedDTCs_tableWidget, QStringList(""), QStringList(tr("----- Not supported by ECU -----")));
	else
		setDCtableContent(memorizedDTCs_tableWidget, QStringList(""), QStringList(""));
	memorizedDTCsTitle_label->setEnabled(memDTCs_sup);
	memorizedDTCs_tableWidget->setEnabled(memDTCs_sup);
	// Deactivate and disconnect "Print"-button:
	printDClist_pushButton->setEnabled(false);
	disconnect(printDClist_pushButton, SIGNAL( pressed() ), this, SLOT( printDCprotocol() ));
	// Connect start-slot:
	if (ok && (_supportedDCgroups != SSMprotocol::noDCs_DCgroup))
		connect(_SSMPdev, SIGNAL( startedDCreading() ), this, SLOT( callStart() ));
	else
		disconnect(_SSMPdev, SIGNAL( startedDCreading() ), this, SLOT( callStart() ));
	// Return result;
	return ok;
}


void CUcontent_DCs_transmission::callStart()
{
	if (!startDCreading())
		communicationError(tr("Couldn't start Diagnostic Codes Reading."));
}


void CUcontent_DCs_transmission::callStop()
{
	if (!stopDCreading())
		communicationError(tr("Couldn't stop Diagnostic Codes Reading."));
}


bool CUcontent_DCs_transmission::startDCreading()
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
		if (selDCgroups != _supportedDCgroups) // inconsistency detected !
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
	if (_supportedDCgroups == (_supportedDCgroups | SSMprotocol::temporaryDTCs_DCgroup))
	{
		updateTemporaryDTCsContent(QStringList(""), QStringList(tr("----- Reading data... Please wait ! -----")));
		connect(_SSMPdev, SIGNAL( temporaryDTCs(QStringList, QStringList, bool) ), this, SLOT( updateTemporaryDTCsContent(QStringList, QStringList) ));
	}
	if (_supportedDCgroups == (_supportedDCgroups | SSMprotocol::memorizedDTCs_DCgroup))
	{
		updateMemorizedDTCsContent(QStringList(""), QStringList(tr("----- Reading data... Please wait ! -----")));
		connect(_SSMPdev, SIGNAL( memorizedDTCs(QStringList, QStringList) ), this, SLOT( updateMemorizedDTCsContent(QStringList, QStringList) ));
	}
	// Connect and disable print-button temporary (until all memories have been read once):
	printDClist_pushButton->setDisabled(true);
	connect(printDClist_pushButton, SIGNAL( pressed() ), this, SLOT( printDCprotocol() ));
	return true;
}


bool CUcontent_DCs_transmission::stopDCreading()
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
	disconnect(_SSMPdev, SIGNAL( temporaryDTCs(QStringList, QStringList, bool) ), this, SLOT( updateTemporaryDTCsContent(QStringList, QStringList) ));
	disconnect(_SSMPdev, SIGNAL( memorizedDTCs(QStringList, QStringList) ), this, SLOT( updateMemorizedDTCsContent(QStringList, QStringList) ));
	return true;
}


void CUcontent_DCs_transmission::updateTemporaryDTCsContent(QStringList temporaryDTCs, QStringList temporaryDTCdescriptions)
{
	if ((temporaryDTCs != _temporaryDTCs) || (temporaryDTCdescriptions != _temporaryDTCdescriptions))
	{
		// Save Trouble Codes:
		_temporaryDTCs = temporaryDTCs;
		_temporaryDTCdescriptions = temporaryDTCdescriptions;
		// Output Trouble Codes:
		if ((temporaryDTCs.size() == 0) && (temporaryDTCdescriptions.size() == 0))
		{
			temporaryDTCs << "";
			temporaryDTCdescriptions << tr("----- No Trouble Codes -----");
		}
		setDCtableContent(temporaryDTCs_tableWidget, temporaryDTCs, temporaryDTCdescriptions);
		// Activate "Print" button:
		printDClist_pushButton->setEnabled(true);
	}
}


void CUcontent_DCs_transmission::updateMemorizedDTCsContent(QStringList memorizedDTCs, QStringList memorizedDTCdescriptions)
{
	if ((memorizedDTCs != _memorizedDTCs) || (memorizedDTCdescriptions != _memorizedDTCdescriptions))
	{
		// Save Trouble Codes:
		_memorizedDTCs = memorizedDTCs;
		_memorizedDTCdescriptions = memorizedDTCdescriptions;
		// Output Trouble Codes:
		if ((memorizedDTCs.size() == 0) && (memorizedDTCdescriptions.size() == 0))
		{
			memorizedDTCs << "";
			memorizedDTCdescriptions << tr("----- No Trouble Codes -----");
		}
		setDCtableContent(memorizedDTCs_tableWidget, memorizedDTCs, memorizedDTCdescriptions);
		// Activate "Print" button:
		printDClist_pushButton->setEnabled(true);
	}
}


void CUcontent_DCs_transmission::setDCtableContent(QTableWidget *tableWidget, QStringList DCs, QStringList DCdescriptions)
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


void CUcontent_DCs_transmission::setNrOfTableRows(QTableWidget *tablewidget, unsigned int nrofUsedRows)
{
	// NOTE: this function doesn't change the table's content ! Min. 1 row !
	int rowheight = 0;
	int vspace = 0;
	QHeaderView *headerview;
	unsigned int minnrofrows = 0;
	const char vspace_offset = -4;	// experimental value

	// Get available vertical space (for rows) and height per row:
	if (tablewidget->rowCount() < 1)
		tablewidget->setRowCount(1); // temporary create a row to get the row hight
	rowheight = tablewidget->rowHeight(0);
	headerview = tablewidget->horizontalHeader();
	vspace = tablewidget->height() - headerview->height() + vspace_offset;
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
	tablewidget->setVerticalScrollMode( QAbstractItemView::ScrollPerItem ); // auto-scroll is triggered; Maybe this is a Qt-Bug, we don't want that  here...
}


void CUcontent_DCs_transmission::printDCprotocol()
{
	QString datetime;
	QString CU;
	QString systype;
	QString ROM_ID;
	QStringList temporaryDTCcodes = _temporaryDTCs;
	QStringList temporaryDTCdescriptions = _temporaryDTCdescriptions;
	QStringList memorizedDTCcodes = _memorizedDTCs;
	QStringList memorizedDTCdescriptions = _memorizedDTCdescriptions;
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
	CU = tr("Transmission");
	if (!_SSMPdev->getSystemDescription(&systype))
	{
		QString SYS_ID = "";
		if (!_SSMPdev->getSysID(&SYS_ID))
		{
			printmbox.close();
			communicationError(tr("Query of the System-ID failed."));
			return;
		}
		systype = tr("Unknown (") + SYS_ID + ")";	// NOTE: SYS_ID is always available, if CU is initialized/connection is alive
		/* TODO: IMPROVE: use other functions from libID to determine system type 
			 => maybe this should be done in SSMprotocol, too		*/
	}
	if (!_SSMPdev->getROMID(&ROM_ID))		// NOTE: ROM_ID is always available, if CU is initialized/connection is alive
	{
		printmbox.close();
		communicationError(tr("Query of the ROM-ID failed."));
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
	cursor.insertTable(3, 2, tableFormat);
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
	// Move (out of the table) to the next block:
	cursor.movePosition(QTextCursor::NextBlock,QTextCursor::MoveAnchor,1);
	// Move (out of the frame) to the next block:
	cursor.movePosition(QTextCursor::NextBlock,QTextCursor::MoveAnchor,1);

	// ##### DTC-tables #####
	// Current/Temporary DTCs:
	if ( _supportedDCgroups == (_supportedDCgroups | SSMprotocol::temporaryDTCs_DCgroup) )
	{
		if (_temporaryDTCdescriptions.size() == 0)
		{
			temporaryDTCcodes << "";
			temporaryDTCdescriptions << tr("----- No Trouble Codes -----");
		}
		// Insert table with current/temporary DTCs into text document:
		insertDCtable(cursor, temporaryDTCsTitle_label->text(), temporaryDTCcodes, temporaryDTCdescriptions);
	}
	// Historic/Memorized DTCs:
	if ( _supportedDCgroups == (_supportedDCgroups | SSMprotocol::memorizedDTCs_DCgroup) )
	{
		if (_memorizedDTCdescriptions.size() == 0)
		{
			memorizedDTCcodes << "";
			memorizedDTCdescriptions << tr("----- No Trouble Codes -----");
		}
		// Insert table with historic/memorized DTCs into text document:
		insertDCtable(cursor, memorizedDTCsTitle_label->text(), memorizedDTCcodes, memorizedDTCdescriptions);
	}
	// Print created text-document:
	textDocument.print(&printer);
	// Delete status message:
	printmbox.close();
}


void CUcontent_DCs_transmission::insertDCtable(QTextCursor cursor, QString title, QStringList codes, QStringList descriptions)
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


void CUcontent_DCs_transmission::resizeEvent(QResizeEvent *event)
{
	// Calculate and set number of table rows:
	setNrOfTableRows(temporaryDTCs_tableWidget, _temporaryDTCs.size() );
	setNrOfTableRows(memorizedDTCs_tableWidget, _memorizedDTCs.size() );
	// Accept event:
	event->accept();
}


bool CUcontent_DCs_transmission::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::Wheel)
	{
		if (obj == temporaryDTCs_tableWidget->viewport())
		{
			if (temporaryDTCs_tableWidget->verticalScrollBarPolicy() ==  Qt::ScrollBarAlwaysOff)
				return true;	// filter out
			else
				return false;
		}
		else if (obj == memorizedDTCs_tableWidget->viewport())
		{
			if (memorizedDTCs_tableWidget->verticalScrollBarPolicy() ==  Qt::ScrollBarAlwaysOff)
				return true;	// filter out
			else
				return false;
		}
	}
	// Pass the event on to the parent class
	return QWidget::eventFilter(obj, event);
}


void CUcontent_DCs_transmission::communicationError(QString errstr)
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


void CUcontent_DCs_transmission::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS OF ui_FreeSSM.h (made with QDesigner)
	QFont contentfont = QApplication::font();
	contentfont.setPixelSize(12); // 9pts
	contentfont.setBold(false);
	this->setFont(contentfont);
	// Table titles:
	QFont tabletitlefont = contentfont;
	tabletitlefont.setUnderline(true);
	temporaryDTCsTitle_label->setFont(tabletitlefont);
	memorizedDTCsTitle_label->setFont(tabletitlefont);
	// Tables:
	temporaryDTCs_tableWidget->setFont(contentfont);
	memorizedDTCs_tableWidget->setFont(contentfont);
	// Info about DC-Clearing:
	DCclearingInfo_label->setFont(contentfont);
	// Print-button:
	printDClist_pushButton->setFont(contentfont);
}

