/*
 * AddMBsSWsDlg.cpp - Dialog for selecting/adding measuring blocks and switches
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

#include "AddMBsSWsDlg.h"



AddMBsSWsDlg::AddMBsSWsDlg(QWidget *parent, std::vector<mb_dt> supportedMBs, std::vector<sw_dt> supportedSWs,
						   std::vector<MBSWmetadata_dt> *MBSWmetaList) : QDialog(parent)
{
	unsigned int k=0;
	unsigned int m=0;
	bool unselected = false;
	MBSWmetadata_dt tmpMBSWmd;

	_MBSWmetaList = MBSWmetaList;
	_unselectedMBsSWs_metaList.clear();
	// Setup GUI:
	setupUi(this);
	setupUiFonts();
	// enable maximize and minimize buttons
	//   GNOME 3 at least: this also enables fast window management e.g. "View split on left" (Super-Left), "... right" (Super-Right)
	setWindowFlags( Qt::Window );
	// SAVE AND OUTPUT AVAILABLE (UNSELECTED) MBs:
	tmpMBSWmd.blockType = blockType_MB;
	for (k=0; k<supportedMBs.size(); k++)
	{
		unselected = true;
		for (m=0; m<(_MBSWmetaList->size()); m++)
		{
			if ((_MBSWmetaList->at(m).blockType == blockType_MB) && (_MBSWmetaList->at(m).nativeIndex == k))
			{
				unselected = false;
				break;
			}
		}
		if (unselected)
		{
			const auto& mb = supportedMBs.at(k);
			// Output MB:
			const QString label = !mb.unit.isEmpty() ? mb.title + "   [" + mb.unit + "]" : mb.title;
			MBsSWs_listWidget->addItem(label);
			// Put MB to the list of unselected MBs/SWs:
			tmpMBSWmd.nativeIndex = k;
			_unselectedMBsSWs_metaList.push_back( tmpMBSWmd );
		}
	}
	// SAVE AND OUTPUT AVAILABLE (UNSELECTED) SWs:
	tmpMBSWmd.blockType = blockType_SW;
	for (k=0; k<supportedSWs.size(); k++)
	{
		unselected = true;
		for (m=0; m<(_MBSWmetaList->size()); m++)
		{
			if ((_MBSWmetaList->at(m).blockType == blockType_SW) && (_MBSWmetaList->at(m).nativeIndex == k))
			{
				unselected = false;
				break;
			}
		}
		if (unselected)
		{
			// Output SW:
			auto& sw = supportedSWs.at(k);
			MBsSWs_listWidget->addItem(sw.title+"   ["+ sw.unit.replace('\\','/') +"]");
			// Put SW to the list of unselected MBs/SWs:
			tmpMBSWmd.nativeIndex = k;
			_unselectedMBsSWs_metaList.push_back( tmpMBSWmd );
		}
	}
	// Enable/disable "Add" button:
	setAddButtonEnableStatus();
	// CONNECT BUTTONS AND LIST WIDGET WITH SLOTS:
	connect(add_pushButton, SIGNAL( released() ), this, SLOT( add() ));
	connect(cancel_pushButton, SIGNAL( released() ), this, SLOT( cancel() ));
	connect(MBsSWs_listWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( setAddButtonEnableStatus() ));
}


AddMBsSWsDlg::~AddMBsSWsDlg()
{
	disconnect(add_pushButton, SIGNAL( released() ), this, SLOT( add() ));
	disconnect(cancel_pushButton, SIGNAL( released() ), this, SLOT( cancel() ));
	disconnect(MBsSWs_listWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( setAddButtonEnableStatus() ));
}


void AddMBsSWsDlg::add()
{
	disconnect(add_pushButton, SIGNAL( pressed() ), this, SLOT( add() )); // bugfix !
	QItemSelectionModel *selModel = MBsSWs_listWidget->selectionModel();
	QModelIndexList MIlist = selModel->selectedRows();
	std::sort(MIlist.begin(), MIlist.end(), rowIndexLessThan);	// since Qt 4.4.1, we have to sort the QModelIndexes...
	for (const auto& mi : MIlist)
	{
		int index = mi.row();
		_MBSWmetaList->push_back( _unselectedMBsSWs_metaList.at(index) );
	}
	close();
}


void AddMBsSWsDlg::cancel()
{
	close();
};


void AddMBsSWsDlg::setAddButtonEnableStatus()
{
	const QList<QListWidgetItem*> selitemslist = MBsSWs_listWidget->selectedItems();
	// NOTE: returns the nr. of selected cells, NOT THE NR. OF ROWS ! Empty cells are not included !
	add_pushButton->setEnabled(selitemslist.size() > 0);
}


bool AddMBsSWsDlg::rowIndexLessThan(const QModelIndex mi_A, const QModelIndex mi_B)
{
	return mi_A.row() < mi_B.row();
}


void AddMBsSWsDlg::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS OF ui_AddMBsSWsDlg.h (made with QDesigner)
	QFont appfont = QApplication::font();
	QFont font = this->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	this->setFont(font);
	font = title_label->font();
	font.setBold(true);
	font.setPixelSize(13);	// 10pts
	title_label->setFont(font);
	font = MBsSWs_listWidget->font();
	font.setPixelSize(12);	// 9pts
	MBsSWs_listWidget->setFont(font);
	font = add_pushButton->font();
	font.setPixelSize(13);	// 10pts
	add_pushButton->setFont(font);
	font = cancel_pushButton->font();
	font.setPixelSize(13);	// 10pts
	cancel_pushButton->setFont(font);
}
