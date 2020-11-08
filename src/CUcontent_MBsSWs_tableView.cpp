/*
 * CUcontent_MBsSWs_tableView.cpp - Widget for displaying MB/SW values in a table
 *
 * Copyright (C) 2009-2018 Comer352L
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
	// Disable all GUI-elements:
	mbswmoveup_pushButton->setEnabled( false );
	mbswmovedown_pushButton->setEnabled( false );
	// Set table column resize behavior:
	headerview = selectedMBsSWs_tableWidget->horizontalHeader();
#if QT_VERSION < 0x050000
	headerview->setResizeMode(static_cast<int>(Column::type), QHeaderView::ResizeToContents);
	headerview->setResizeMode(static_cast<int>(Column::title), QHeaderView::Stretch);
	headerview->setResizeMode(static_cast<int>(Column::min), QHeaderView::ResizeToContents);
	headerview->setResizeMode(static_cast<int>(Column::current), QHeaderView::ResizeToContents);
	headerview->setResizeMode(static_cast<int>(Column::max), QHeaderView::ResizeToContents);
	headerview->setResizeMode(static_cast<int>(Column::unit), QHeaderView::ResizeToContents);
#else
	headerview->setSectionResizeMode(static_cast<int>(Column::type), QHeaderView::ResizeToContents);
	headerview->setSectionResizeMode(static_cast<int>(Column::title), QHeaderView::Stretch);
	headerview->setSectionResizeMode(static_cast<int>(Column::min), QHeaderView::ResizeToContents);
	headerview->setSectionResizeMode(static_cast<int>(Column::current), QHeaderView::ResizeToContents);
	headerview->setSectionResizeMode(static_cast<int>(Column::max), QHeaderView::ResizeToContents);
	headerview->setSectionResizeMode(static_cast<int>(Column::unit), QHeaderView::ResizeToContents);
#endif
	// Set table row resize behavior:
	headerview = selectedMBsSWs_tableWidget->verticalHeader();
#if QT_VERSION < 0x050000
	headerview->setResizeMode(QHeaderView::Fixed);
#else
	headerview->setSectionResizeMode(QHeaderView::Fixed);
#endif
	/* NOTE: Current method for calculating ther nr. of needed rows
	 * assumes all rows to have the same constant height */
	// Install event-filter for MB/SW-table:
	selectedMBsSWs_tableWidget->viewport()->installEventFilter(this);
	// (Un)check min/max toggle-buttons:
	showMin_pushButton->setChecked(showMin);
	showMax_pushButton->setChecked(showMax);
	// Make min/max values columns (in)visible:
	toggleMinColumnVisible(showMin);
	toggleMaxColumnVisible(showMax);
	iconMB = QIcon(":/icons/freessm/32x32/MB.png");
	iconSW = QIcon(":/icons/freessm/32x32/SW.png");
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


void CUcontent_MBsSWs_tableView::setMBSWlistContent(const std::vector<BlockType>& types,
						    const std::vector<QString>& titles,
						    const std::vector<QString>& values,
						    const std::vector<QString>& minValues,
						    const std::vector<QString>& maxValues,
						    const std::vector<QString>& units      )
{
	int firstrowvisibleindex = 0;
	// Delete table content:
	selectedMBsSWs_tableWidget->clearContents();
	// Save nr of MBs/Sws:
	_nrofMBsSWs = std::max({ types.size(), titles.size(), values.size(), minValues.size(), maxValues.size(), units.size() });

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
	updateTypesColumn(types);
	updateMBColumn(titles, Column::title, Qt::AlignLeft | Qt::AlignVCenter);
	// current/min/max values, units:
	updateMBSWvalues(values, minValues, maxValues, units);
}

void CUcontent_MBsSWs_tableView::updateMBColumn(const std::vector<QString>& data, CUcontent_MBsSWs_tableView::Column column, Qt::Alignment alignment)
{
	const int col = static_cast<int>(column);
	const unsigned int rowcount = std::min(static_cast<unsigned int>(data.size()), _nrofMBsSWs);
	for (unsigned int row = 0; row < rowcount; ++row) {
		QTableWidgetItem* tableelement = selectedMBsSWs_tableWidget->item(row, col);
		if (tableelement)
			tableelement->setText(data.at(row));
		else
		{
			tableelement = new QTableWidgetItem(data.at(row));
			tableelement->setTextAlignment(alignment);
			selectedMBsSWs_tableWidget->setItem(row, col, tableelement);
		}
	}
}

void CUcontent_MBsSWs_tableView::updateMBSWvalues(const std::vector<QString>& valueStrList,
						  const std::vector<QString>& minValueStrList,
						  const std::vector<QString>& maxValueStrList,
						  const std::vector<QString>& unitStrList     )
{
	updateMBColumn(minValueStrList, Column::min);
	updateMBColumn(valueStrList, Column::current);
	updateMBColumn(maxValueStrList, Column::max);
	updateMBColumn(unitStrList, Column::unit, Qt::AlignLeft | Qt::AlignVCenter);
	/* NOTE: The units can change during MB/SW-reading !:
	 *       If a MB/SW cannot be scaled (e.g. due to unexpected raw values, incomplete/invalid definitions),
	 *       the raw value is displayed instead and the unit is switched to [RAW] (MBs) or [BIN] (SWs).
	 */
}

void CUcontent_MBsSWs_tableView::updateTypesColumn(const std::vector<BlockType>& types)
{
	const unsigned int rowcount = std::min(static_cast<unsigned int>(types.size()), _nrofMBsSWs);

	for (unsigned int row = 0; row < rowcount; ++row) {
		QTableWidgetItem* tableelement = selectedMBsSWs_tableWidget->item(row, Column::type);
		const QIcon& icon = types.at(row) == BlockType::MB ? iconMB : iconSW;
		if (tableelement)
			tableelement->setIcon(icon);
		else
		{
			tableelement = new QTableWidgetItem(icon, nullptr);
			//tableelement->setIcon(icon);
			selectedMBsSWs_tableWidget->setItem(row, Column::type, tableelement);
		}
	}
}


void CUcontent_MBsSWs_tableView::clearMBSWlistContent()
{
	selectedMBsSWs_tableWidget->clear();
	_nrofMBsSWs = 0;
}


void CUcontent_MBsSWs_tableView::setMoveButtonsEnabledState()
{
	const std::vector<unsigned int> selectedMBSWIndexes = getSelectedTableWidgetRows();
	if (selectedMBSWIndexes.size() > 0)
	{
		mbswmoveup_pushButton->setEnabled(selectedMBSWIndexes.front() > 0);
		mbswmovedown_pushButton->setEnabled(selectedMBSWIndexes.back() < (_nrofMBsSWs - 1));
	}
	else
	{
		mbswmovedown_pushButton->setEnabled(false);
		mbswmoveup_pushButton->setEnabled(false);
	}
}


void CUcontent_MBsSWs_tableView::toggleMinColumnVisible(bool show)
{
	if (show)
		selectedMBsSWs_tableWidget->showColumn(static_cast<int>(Column::min));
	else
		selectedMBsSWs_tableWidget->hideColumn(static_cast<int>(Column::min));
}


void CUcontent_MBsSWs_tableView::toggleMaxColumnVisible(bool show)
{
	if (show)
		selectedMBsSWs_tableWidget->showColumn(static_cast<int>(Column::max));
	else
		selectedMBsSWs_tableWidget->hideColumn(static_cast<int>(Column::max));
}


bool CUcontent_MBsSWs_tableView::minValuesEnabled() const
{
	return showMin_pushButton->isChecked();
}


bool CUcontent_MBsSWs_tableView::maxValuesEnabled() const
{
	return showMax_pushButton->isChecked();
}


std::vector<unsigned int> CUcontent_MBsSWs_tableView::getSelectedTableWidgetRows() const
{
	// GET INDEXES OF SELECTED ROWS:
	std::vector<unsigned int> selectedMBSWIndexes;
	const QList<QTableWidgetSelectionRange> ranges = selectedMBsSWs_tableWidget->selectedRanges();
	for (const QTableWidgetSelectionRange& range : ranges)
	{
		const int rows = range.rowCount();
		const int topRow = range.topRow();
		for (int m=0; m<rows; m++)
		{
			const unsigned int index = topRow + m;
			if (index < _nrofMBsSWs)
				selectedMBSWIndexes.push_back(index);
		}
	}
	std::sort(selectedMBSWIndexes.begin(), selectedMBSWIndexes.end());
	/* NOTE: This function must return sorted indexes (from min to max) !
	   At least for the QAbstractItemView::ContiguousSelction selection mode,
	   QTableWidget::selectedRanges() seems to return always sorted indexes.
	   However, Qt-Documentation doesn't tell us anything about the order of
	   the returned indexes, so we can NOT assume that they are and will
	   ever be sorted in future Qt-versions !
	 */
	return selectedMBSWIndexes;
}


void CUcontent_MBsSWs_tableView::selectMBSWtableRows(unsigned int start, unsigned int end)
{
	QTableWidgetSelectionRange selrange(start, static_cast<int>(Column::type), end, static_cast<int>(Column::unit));
	selectedMBsSWs_tableWidget->setRangeSelected(selrange , true);
}


void CUcontent_MBsSWs_tableView::scrollMBSWtable(unsigned int rowindex)
{
	const QTableWidgetItem *item = selectedMBsSWs_tableWidget->item(rowindex, static_cast<int>(Column::title));
	selectedMBsSWs_tableWidget->scrollToItem(item, QAbstractItemView::EnsureVisible);
}


void CUcontent_MBsSWs_tableView::resizeEvent(QResizeEvent *event)
{
	// Get available vertical space (for rows) and height per row:
	if (selectedMBsSWs_tableWidget->rowCount() < 1)
		selectedMBsSWs_tableWidget->setRowCount(1); // Temporary create a row to get the row hight
	const int rowheight = selectedMBsSWs_tableWidget->rowHeight(0);
	//vspace = selectedMBsSWs_tableWidget->viewport()->height(); // NOTE: Sometimes doesn't work as expected ! (Qt-Bug ?)
	const int vspace = selectedMBsSWs_tableWidget->height() - selectedMBsSWs_tableWidget->horizontalHeader()->viewport()->height() - 4;
	// Temporary switch to "Scroll per Pixel"-mode to ensure auto-scroll (prevent white space between bottom of the last row and the lower table border)
	selectedMBsSWs_tableWidget->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
	// Calculate and set nr. of rows:
	_maxrowsvisible = static_cast<unsigned int>(trunc((vspace-1)/rowheight) + 1);
	const unsigned int minnrofrows = _maxrowsvisible < _nrofMBsSWs ? _nrofMBsSWs : _maxrowsvisible;
	selectedMBsSWs_tableWidget->setRowCount(minnrofrows);
	// Set vertical scroll bar policy:
	selectedMBsSWs_tableWidget->setVerticalScrollBarPolicy( minnrofrows > _nrofMBsSWs ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded );
	// Switch back to "Scroll per item"-mode:
	selectedMBsSWs_tableWidget->setVerticalScrollMode( QAbstractItemView::ScrollPerItem ); // auto-scroll is triggered; Maybe this is a Qt-Bug, we don't want that here...
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

