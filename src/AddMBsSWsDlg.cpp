/*
 * AddMBsSWsDlg.cpp - Dialog for selecting/adding measuring blocks and switches
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

#include "AddMBsSWsDlg.h"



AddMBsSWsDlg::AddMBsSWsDlg(QWidget *parent, std::vector<mb_dt> supportedMBs, std::vector<sw_dt> supportedSWs,
                           std::vector<MBSWmetadata_dt> *MBSWmetaList) : QDialog(parent)
{
	bool unselected = false;
	MBSWmetadata_dt tmpMBSWmd;

	_MBSWmetaList = MBSWmetaList;
	_unselectedMBsSWs_metaList.clear();
	// Setup GUI:
	setupUi(this);
	// enable maximize and minimize buttons
	//   GNOME 3 at least: this also enables fast window management e.g. "View split on left" (Super-Left), "... right" (Super-Right)
	setWindowFlags( Qt::Window );

	iconMB = QIcon(":/icons/freessm/32x32/MB.png");
	iconSW = QIcon(":/icons/freessm/32x32/SW.png");
	QHeaderView* headerview = MBsSWs_tableWidget->horizontalHeader();
#if QT_VERSION < 0x050000
	headerview->setResizeMode(static_cast<int>(Column::type), QHeaderView::ResizeToContents);
	headerview->setResizeMode(static_cast<int>(Column::title), QHeaderView::Stretch);
	headerview->setResizeMode(static_cast<int>(Column::unit), QHeaderView::ResizeToContents);
#else
	headerview->setSectionResizeMode(static_cast<int>(Column::type), QHeaderView::ResizeToContents);
	headerview->setSectionResizeMode(static_cast<int>(Column::title), QHeaderView::Stretch);
	headerview->setSectionResizeMode(static_cast<int>(Column::unit), QHeaderView::ResizeToContents);
#endif
	// Set table row resize behavior:
	headerview = MBsSWs_tableWidget->verticalHeader();
#if QT_VERSION < 0x050000
	headerview->setResizeMode(QHeaderView::Fixed);
#else
	headerview->setSectionResizeMode(QHeaderView::Fixed);
#endif

	std::vector<Item> items;
	// FIND AVAILABLE (UNSELECTED) MBs:
	tmpMBSWmd.blockType = BlockType::MB;
	for (size_t k = 0; k < supportedMBs.size(); ++k)
	{
		unselected = true;
		for (size_t m = 0; m < _MBSWmetaList->size(); ++m)
		{
			const MBSWmetadata_dt& metadata = _MBSWmetaList->at(m);
			if (metadata.blockType == BlockType::MB && metadata.nativeIndex == k)
			{
				unselected = false;
				break;
			}
		}
		if (unselected)
		{
			const mb_dt& mb = supportedMBs.at(k);
			// Output MB:
			items.push_back(Item { BlockType::MB, mb.title, mb.unit });

			// Put MB to the list of unselected MBs/SWs:
			tmpMBSWmd.nativeIndex = k;
			_unselectedMBsSWs_metaList.push_back( tmpMBSWmd );
		}
	}
	// FIND AVAILABLE (UNSELECTED) SWs:
	tmpMBSWmd.blockType = BlockType::SW;
	for (size_t k=0; k < supportedSWs.size(); ++k)
	{
		unselected = true;
		for (size_t m = 0; m < _MBSWmetaList->size(); ++m)
		{
			const MBSWmetadata_dt& metadata = _MBSWmetaList->at(m);
			if (metadata.blockType == BlockType::SW && metadata.nativeIndex == k)
			{
				unselected = false;
				break;
			}
		}
		if (unselected)
		{
			// Output SW:
			sw_dt& sw = supportedSWs.at(k);
			items.push_back(Item { BlockType::SW, sw.title, sw.unit.replace('\\','/') });

			// Put SW to the list of unselected MBs/SWs:
			tmpMBSWmd.nativeIndex = k;
			_unselectedMBsSWs_metaList.push_back( tmpMBSWmd );
		}
	}

	setContent(items);
	// Enable/disable "Add" button:
	setAddButtonEnableStatus();
	// CONNECT BUTTONS AND LIST WIDGET WITH SLOTS:
	connect(add_pushButton, SIGNAL( released() ), this, SLOT( add() ));
	connect(cancel_pushButton, SIGNAL( released() ), this, SLOT( cancel() ));
	connect(MBsSWs_tableWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( setAddButtonEnableStatus() ));
}


AddMBsSWsDlg::~AddMBsSWsDlg()
{
	disconnect(add_pushButton, SIGNAL( released() ), this, SLOT( add() ));
	disconnect(cancel_pushButton, SIGNAL( released() ), this, SLOT( cancel() ));
	disconnect(MBsSWs_tableWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( setAddButtonEnableStatus() ));
}


void AddMBsSWsDlg::add()
{
	disconnect(add_pushButton, SIGNAL( pressed() ), this, SLOT( add() )); // bugfix !
	QItemSelectionModel *selModel = MBsSWs_tableWidget->selectionModel();
	QModelIndexList MIlist = selModel->selectedRows();
	std::sort(MIlist.begin(), MIlist.end(), rowIndexLessThan);	// since Qt 4.4.1, we have to sort the QModelIndexes...
	for (const QModelIndex& mi : MIlist)
	{
		int index = mi.row();
		_MBSWmetaList->push_back( _unselectedMBsSWs_metaList.at(index) );
	}
	close();
}


void AddMBsSWsDlg::cancel()
{
	close();
}


void AddMBsSWsDlg::setAddButtonEnableStatus()
{
	const QList<QTableWidgetItem*> selitemslist = MBsSWs_tableWidget->selectedItems();
	// NOTE: returns the nr. of selected cells, NOT THE NR. OF ROWS ! Empty cells are not included !
	add_pushButton->setEnabled(selitemslist.size() > 0);
}


bool AddMBsSWsDlg::rowIndexLessThan(const QModelIndex mi_A, const QModelIndex mi_B)
{
	return mi_A.row() < mi_B.row();
}


void AddMBsSWsDlg::setContent(const std::vector<Item>& items)
{
	MBsSWs_tableWidget->clearContents();
	const size_t count = items.size();
	MBsSWs_tableWidget->setRowCount(count);

	for (size_t row = 0; row < count; ++row)
	{
		const Item& item = items.at(row);
		QTableWidgetItem* tableelement;

		const QIcon& icon = item.blockType == BlockType::MB ? iconMB : iconSW;
		tableelement = new QTableWidgetItem(icon, nullptr);
		MBsSWs_tableWidget->setItem(row, static_cast<int>(Column::type), tableelement);

		tableelement = new QTableWidgetItem(item.title);
		//tableelement->setTextAlignment(alignment);
		MBsSWs_tableWidget->setItem(row, static_cast<int>(Column::title), tableelement);

		tableelement = new QTableWidgetItem(item.unit);
		//tableelement->setTextAlignment(alignment);
		MBsSWs_tableWidget->setItem(row, static_cast<int>(Column::unit), tableelement);
	}
}
