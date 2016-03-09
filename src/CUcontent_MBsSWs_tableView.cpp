/*
 * CUcontent_MBsSWs_tableView.cpp - Widget for displaying MB/SW values in a table
 *
 * Copyright (C) 2009 Comer352l
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

#include "CUcontent_MBsSWs_tableView.h"



CUcontent_MBsSWs_tableView::CUcontent_MBsSWs_tableView(QWidget *parent, bool showMin, bool showMax) : QWidget(parent)
{
	QHeaderView *headerview;
	_nrofMBsSWs = 0;
	_maxrowsvisible = 0;

	// Setup GUI:
	setupUi(this);
	setupUiFonts();
	// Disable all GUI-elements:
	mbswmoveup_pushButton->setEnabled( false );
	mbswmovedown_pushButton->setEnabled( false );
	// Set table column resize behavior:
	headerview = selectedMBsSWs_tableWidget->horizontalHeader();
	headerview->setResizeMode(col_title, QHeaderView::Stretch);
	headerview->setResizeMode(col_min, QHeaderView::ResizeToContents);
	headerview->setResizeMode(col_current, QHeaderView::ResizeToContents);
	headerview->setResizeMode(col_max, QHeaderView::ResizeToContents);
	headerview->setResizeMode(col_unit, QHeaderView::ResizeToContents);
	// Set table row resize behavior:
	headerview = selectedMBsSWs_tableWidget->verticalHeader();
	headerview->setResizeMode(QHeaderView::Fixed);
	/* NOTE: Current method for calculating ther nr. of needed rows
	 * assumes all rows to have the same constsant height */
	// Install event-filter for MB/SW-table:
	selectedMBsSWs_tableWidget->viewport()->installEventFilter(this);
	// (Un)check min/max toggle-buttons:
	showMin_pushButton->setChecked(showMin);
	showMax_pushButton->setChecked(showMax);
	// Make min/max values columns (in)visible:
	toggleMinColumnVisible(showMin);
	toggleMaxColumnVisible(showMax);
	// Connect signals and slots:
	connect( mbswmoveup_pushButton , SIGNAL( released() ), this, SIGNAL( moveUpButton_pressed() ) );
	connect( mbswmovedown_pushButton , SIGNAL( released() ), this, SIGNAL( moveDownButton_pressed() ) );
	connect( resetMinMax_pushButton , SIGNAL( released() ), this, SIGNAL( resetMinMaxButton_pressed() ) );
	connect( showMin_pushButton , SIGNAL( clicked(bool) ), this, SLOT( toggleMinColumnVisible(bool) ) );
	connect( showMax_pushButton , SIGNAL( clicked(bool) ), this, SLOT( toggleMaxColumnVisible(bool) ) );
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
	connect( selectedMBsSWs_tableWidget , SIGNAL( itemSelectionChanged() ), this, SLOT( setMoveButtonsEnabledState() ) );
	connect( selectedMBsSWs_tableWidget , SIGNAL( itemSelectionChanged() ), this, SIGNAL( itemSelectionChanged() ) );
}


CUcontent_MBsSWs_tableView::~CUcontent_MBsSWs_tableView()
{
	disconnect( mbswmoveup_pushButton , SIGNAL( released() ), this, SIGNAL( moveUpButton_pressed() ) );
	disconnect( mbswmovedown_pushButton , SIGNAL( released() ), this, SIGNAL( moveDownButton_pressed() ) );
	disconnect( resetMinMax_pushButton , SIGNAL( released() ), this, SIGNAL( resetMinMaxButton_pressed() ) );
	disconnect( showMin_pushButton , SIGNAL( clicked(bool) ), this, SLOT( toggleMinColumnVisible(bool) ) );
	disconnect( showMax_pushButton , SIGNAL( clicked(bool) ), this, SLOT( toggleMaxColumnVisible(bool) ) );
	disconnect( selectedMBsSWs_tableWidget , SIGNAL( itemSelectionChanged() ), this, SLOT( setMoveButtonsEnabledState() ) );
	disconnect( selectedMBsSWs_tableWidget , SIGNAL( itemSelectionChanged() ), this, SIGNAL( itemSelectionChanged() ) );
}


void CUcontent_MBsSWs_tableView::setMBSWlistContent(QStringList titles, QStringList values, QStringList minValues, QStringList maxValues, QStringList units)
{
	int firstrowvisibleindex = 0;
	// Delete table content:
	selectedMBsSWs_tableWidget->clearContents();
	// Save nr of MBs/Sws:
	_nrofMBsSWs = qMax( qMax( qMax( qMax( values.size(), minValues.size() ), maxValues.size() ), titles.size() ), units.size() );
	// Set number of rows and vertical scroll bar policy:
	if (_nrofMBsSWs >= _maxrowsvisible)
	{
		selectedMBsSWs_tableWidget->setRowCount(_nrofMBsSWs);
		selectedMBsSWs_tableWidget->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
		// Check if get white space area at the bottom of the table:
		firstrowvisibleindex = selectedMBsSWs_tableWidget->rowAt(0);
		if (firstrowvisibleindex+_maxrowsvisible > _nrofMBsSWs)
		{
			selectedMBsSWs_tableWidget->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
			selectedMBsSWs_tableWidget->scrollToBottom();
			selectedMBsSWs_tableWidget->setVerticalScrollMode( QAbstractItemView::ScrollPerItem );
		}
	}
	else
	{
		selectedMBsSWs_tableWidget->setRowCount(_maxrowsvisible);
		selectedMBsSWs_tableWidget->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
		selectedMBsSWs_tableWidget->scrollToTop();
	}
	// *** Fill table ***:
	updateMBColumn(titles, col_title, Qt::AlignLeft | Qt::AlignVCenter);
	// current/min/max values, units:
	updateMBSWvalues(values, minValues, maxValues, units);
}

void CUcontent_MBsSWs_tableView::updateMBColumn(QStringList data, CUcontent_MBsSWs_tableView::Column col, Qt::Alignment alignment)
{
	const unsigned int rowcount = std::min(static_cast<unsigned int>(data.size()), _nrofMBsSWs);
	for (unsigned int row = 0; row < rowcount; ++row) {
		QTableWidgetItem* tableelement = selectedMBsSWs_tableWidget->item(row, col);
		if (tableelement) {
			tableelement->setText(data.at(row));
		} else {
			tableelement = new QTableWidgetItem(data.at(row));
			tableelement->setTextAlignment(alignment);
			selectedMBsSWs_tableWidget->setItem(row, col, tableelement);
		}
	}
}

void CUcontent_MBsSWs_tableView::updateMBSWvalues(QStringList valueStrList, QStringList minValueStrList, QStringList maxValueStrList, QStringList unitStrList)
{
	updateMBColumn(minValueStrList, col_min);
	updateMBColumn(valueStrList, col_current);
	updateMBColumn(maxValueStrList, col_max);
	updateMBColumn(unitStrList, col_unit, Qt::AlignLeft | Qt::AlignVCenter);
	/* NOTE: The units can change during MB/SW-reading !:
	 *       If a MB/SW cannot be scaled (e.g. due to unexpected raw values, incomplete/invalid definitions),
	 *       the raw value is displayed instead and the unit is switched to [RAW] (MBs) or [BIN] (SWs).
	 */
}


void CUcontent_MBsSWs_tableView::clearMBSWlistContent()
{
	selectedMBsSWs_tableWidget->clear();
	_nrofMBsSWs = 0;
}


void CUcontent_MBsSWs_tableView::setMoveButtonsEnabledState()
{
	QList<unsigned int> selectedMBSWIndexes;
	getSelectedTableWidgetRows(&selectedMBSWIndexes);
	if (selectedMBSWIndexes.size() < 1)
	{
		mbswmovedown_pushButton->setEnabled(false);
		mbswmoveup_pushButton->setEnabled(false);
	}
	else
	{
		if (selectedMBSWIndexes.at(0) == 0)
			mbswmoveup_pushButton->setEnabled(false);
		else
			mbswmoveup_pushButton->setEnabled(true);
		if (selectedMBSWIndexes.at(selectedMBSWIndexes.size()-1) == (_nrofMBsSWs-1))
			mbswmovedown_pushButton->setEnabled(false);
		else
			mbswmovedown_pushButton->setEnabled(true);
	}
}


void CUcontent_MBsSWs_tableView::toggleMinColumnVisible(bool show)
{
	if (show)
		selectedMBsSWs_tableWidget->showColumn(col_min);
	else
		selectedMBsSWs_tableWidget->hideColumn(col_min);
}


void CUcontent_MBsSWs_tableView::toggleMaxColumnVisible(bool show)
{
	if (show)
		selectedMBsSWs_tableWidget->showColumn(col_max);
	else
		selectedMBsSWs_tableWidget->hideColumn(col_max);
}


bool CUcontent_MBsSWs_tableView::minValuesEnabled()
{
	return showMin_pushButton->isChecked();
}


bool CUcontent_MBsSWs_tableView::maxValuesEnabled()
{
	return showMax_pushButton->isChecked();
}


void CUcontent_MBsSWs_tableView::getSelectedTableWidgetRows(QList<unsigned int> *selectedMBSWIndexes)
{
	int k=0;
	int m=0;
	int rows=0;
	// GET INDEXES OF SELECTED ROWS:
	selectedMBSWIndexes->clear();
	QList<QTableWidgetSelectionRange> selectedRanges;
	selectedRanges = selectedMBsSWs_tableWidget->selectedRanges();
	for (k=0; k<selectedRanges.size(); k++)
	{
		rows = selectedRanges.at(k).bottomRow() - selectedRanges.at(k).topRow() + 1;
		for (m=0; m<rows; m++)
		{
			if (static_cast<unsigned int>(selectedRanges.at(k).topRow() + m) < _nrofMBsSWs)
				selectedMBSWIndexes->push_back(selectedRanges.at(k).topRow() + m);
		}
	}
	qSort(selectedMBSWIndexes->begin(), selectedMBSWIndexes->end());
	/* NOTE: This function must return sorted indexes (from min to max) !
	   At least for the QAbstractItemView::ContiguousSelction selection mode,
	   QTableWidget::selectedRanges() seems to return always sorted indexes.
	   However, Qt-Documentation doesn't tell us anything about the order of
	   the returned indexes, so we can NOT assume that they are and will
	   ever be sorted in future Qt-versions !
	 */
}


void CUcontent_MBsSWs_tableView::selectMBSWtableRows(unsigned int start, unsigned int end)
{
	QTableWidgetSelectionRange selrange(start, col_title, end, col_unit);
	selectedMBsSWs_tableWidget->setRangeSelected(selrange , true);
}


void CUcontent_MBsSWs_tableView::scrollMBSWtable(unsigned int rowindex)
{
	QTableWidgetItem *item = new QTableWidgetItem;
	item = selectedMBsSWs_tableWidget->item(rowindex, 0);
	selectedMBsSWs_tableWidget->scrollToItem(item, QAbstractItemView::EnsureVisible);
}


void CUcontent_MBsSWs_tableView::resizeEvent(QResizeEvent *event)
{
	int rowheight = 0;
	int vspace = 0;
	unsigned int minnrofrows = 0;
	// Get available vertical space (for rows) and height per row:
	if (selectedMBsSWs_tableWidget->rowCount() < 1)
		selectedMBsSWs_tableWidget->setRowCount(1); // Temporary create a row to get the row hight
	rowheight = selectedMBsSWs_tableWidget->rowHeight(0);
	//vspace = selectedMBsSWs_tableWidget->viewport()->height(); // NOTE: Sometimes doesn't work as expected ! (Qt-Bug ?)
	vspace = selectedMBsSWs_tableWidget->height() - selectedMBsSWs_tableWidget->horizontalHeader()->viewport()->height() - 4;
	// Temporary switch to "Scroll per Pixel"-mode to ensure auto-scroll (prevent white space between bottom of the last row and the lower table border)
	selectedMBsSWs_tableWidget->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
	// Calculate and set nr. of rows:
	_maxrowsvisible = static_cast<unsigned int>(trunc((vspace-1)/rowheight) + 1);
	if (_maxrowsvisible < _nrofMBsSWs)
		minnrofrows = _nrofMBsSWs;
	else
		minnrofrows = _maxrowsvisible;
	selectedMBsSWs_tableWidget->setRowCount(minnrofrows);
	// Set vertical scroll bar policy:
	if (minnrofrows > _nrofMBsSWs)
		selectedMBsSWs_tableWidget->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	else
		selectedMBsSWs_tableWidget->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	// Switch back to "Scroll per item"-mode:
	selectedMBsSWs_tableWidget->setVerticalScrollMode( QAbstractItemView::ScrollPerItem ); // auto-scroll is triggered; Maybe this is a Qt-Bug, we don't want that  here...
	// Accept event:
	event->accept();
}


bool CUcontent_MBsSWs_tableView::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == selectedMBsSWs_tableWidget->viewport())
	{
		if (event->type() == QEvent::Wheel)
		{
			if (selectedMBsSWs_tableWidget->verticalScrollBarPolicy() ==  Qt::ScrollBarAlwaysOff)
			// ...or _maxrowsvisible > _nrofMBsSWs
				return true;	// filter out
			else
				return false;
		}
	}
	// Pass the event on to the parent class
	return QWidget::eventFilter(obj, event);
}


void CUcontent_MBsSWs_tableView::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS OF ui_CUcontent_MBsSWs_tableView.h (made with QDesigner)
	QFont contentfont = QApplication::font();
	contentfont.setPixelSize(12);// 9pts
	contentfont.setBold(false);
	this->setFont(contentfont);
	// Table:
	selectedMBsSWs_tableWidget->setFont(contentfont);
	// Buttons:
	mbswmoveup_pushButton->setFont(contentfont);
	mbswmovedown_pushButton->setFont(contentfont);
	resetMinMax_pushButton->setFont(contentfont);
	showMin_pushButton->setFont(contentfont);
	showMax_pushButton->setFont(contentfont);
}

