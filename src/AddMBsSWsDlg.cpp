/*
 * AddMBsSWsDlg.cpp - Dialog for selecting/adding measuring blocks and switches
 *
 * Copyright © 2008 Comer352l
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



AddMBsSWsDlg::AddMBsSWsDlg(QWidget *parent, mbsw_dt supportedMBs[768], unsigned int nrofsupportedMBs, mbsw_dt supportedSWs[768], unsigned int nrofsupportedSWs,
                           MBSWmetadata_dt *MBSWmetaList, unsigned int *MBSWmetaList_len) : QDialog(parent)
{
	unsigned int k=0;
	unsigned int m=0;
	bool unselected = false;

	_MBSWmetaList = MBSWmetaList;
	_MBSWmetaList_len = MBSWmetaList_len;
	MBSWmetadata_dt zero_mde = {0,0};
	for (k=0; k<1536; k++) _unselectedMBsSWs_metaList[k] = zero_mde;
	_unselectedMBsSWs_metaList_len = 0;
	// Setup GUI:
	setupUi(this);
	setupUiFonts();
	// SAVE AND OUTPUT AVAILABLE (UNSELECTED) MBs/SWs:
	for (k=0; k<nrofsupportedMBs; k++)
	{
		unselected = true;
		for (m=0; m<(*_MBSWmetaList_len); m++)
		{
			if ((_MBSWmetaList[m].blockType == 0) && (_MBSWmetaList[m].nativeIndex == k))
				unselected = false;
		}
		if (unselected)
		{
			// Output MB:
			if (supportedMBs[k].unit == "")
				MBsSWs_listWidget->addItem(supportedMBs[k].title);
			else
				MBsSWs_listWidget->addItem(supportedMBs[k].title+"   ["+ supportedMBs[k].unit+"]");
			// Put MB to the list of unselected MBs/SWs:
			_unselectedMBsSWs_metaList[_unselectedMBsSWs_metaList_len].blockType = 0;
			_unselectedMBsSWs_metaList[_unselectedMBsSWs_metaList_len].nativeIndex = k;
			_unselectedMBsSWs_metaList_len++;
		}
	}
	for (k=0; k<nrofsupportedSWs; k++)
	{
		unselected = true;
		for (m=0; m<(*_MBSWmetaList_len); m++)
		{
			if ((_MBSWmetaList[m].blockType == 1) && (_MBSWmetaList[m].nativeIndex == k))
				unselected = false;
		}
		if (unselected)
		{
			// Output SW:
			MBsSWs_listWidget->addItem(supportedSWs[k].title+"   ["+ supportedSWs[k].unit+"]");
			// Put SW to the list of unselected MBs/SWs:
			_unselectedMBsSWs_metaList[_unselectedMBsSWs_metaList_len].blockType = 1;
			_unselectedMBsSWs_metaList[_unselectedMBsSWs_metaList_len].nativeIndex = k;
			_unselectedMBsSWs_metaList_len++;
		}
	}
	// Enable/disable "Add" button:
	setAddButtonEnableStatus();
	// CONNECT BUTTONS AND LIST WIDGET WITH SLOTS:
	connect(add_pushButton, SIGNAL( pressed() ), this, SLOT( add() ));
	connect(cancel_pushButton, SIGNAL( pressed() ), this, SLOT( cancel() ));
	connect(MBsSWs_listWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( setAddButtonEnableStatus() ));
}


AddMBsSWsDlg::~AddMBsSWsDlg()
{
	disconnect(add_pushButton, SIGNAL( pressed() ), this, SLOT( add() ));
	disconnect(cancel_pushButton, SIGNAL( pressed() ), this, SLOT( cancel() ));
	disconnect(MBsSWs_listWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( setAddButtonEnableStatus() ));
}


void AddMBsSWsDlg::add()
{
	int index = 0;
	int k = 0;
	disconnect(add_pushButton, SIGNAL( pressed() ), this, SLOT( add() )); // bugfix !
	QItemSelectionModel *selModel = MBsSWs_listWidget->selectionModel();
	QModelIndexList MIlist = selModel->selectedRows();
	qSort(MIlist.begin(), MIlist.end(), rowIndexLessThan);	// since Qt 4.4.1, we have to sort the the QModelIndexes...
	for (k=0; k<MIlist.size(); k++)
	{
		index = MIlist.at(k).row();
		_MBSWmetaList[*_MBSWmetaList_len] = _unselectedMBsSWs_metaList[index];
		(*_MBSWmetaList_len)++;
	}
	close();
}


void AddMBsSWsDlg::cancel()
{
	close();
};


void AddMBsSWsDlg::setAddButtonEnableStatus()
{
	QList<QListWidgetItem*> selitemslist;
	selitemslist = MBsSWs_listWidget->selectedItems();
	// NOTE: retuns the nr. of selected cells, NOT THE NR. OF ROWS ! Empty cells are not included !
	if (selitemslist.size() < 1)
		add_pushButton->setEnabled(false);
	else
		add_pushButton->setEnabled(true);
}


bool AddMBsSWsDlg::rowIndexLessThan(const QModelIndex mi_A, const QModelIndex mi_B) 
{ 
	return mi_A.row() < mi_B.row(); 
}


void AddMBsSWsDlg::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS OF ui_FreeSSM.h (made with QDesigner)
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
