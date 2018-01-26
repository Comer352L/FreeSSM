/*
 * AddMBsSWsDlg.h - Dialog for selecting/adding measuring blocks and switches
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

#ifndef ADDMBSSWSDLG_H
#define ADDMBSSWSDLG_H



#include <vector>
#include <algorithm>
#include <QtGui>
#include "ui_AddMBsSWsDlg.h"
#include "SSMprotocol2.h"



class AddMBsSWsDlg : public QDialog, private Ui::AddMBsSWs_Dialog
{
	Q_OBJECT

private:
	enum class Column { type, title, unit };

	struct Item {
		BlockType blockType;
		QString title;
		QString unit;
	};

	std::vector<MBSWmetadata_dt> *_MBSWmetaList;
	std::vector<MBSWmetadata_dt> _unselectedMBsSWs_metaList;
	QIcon iconMB;
	QIcon iconSW;

	static bool rowIndexLessThan(const QModelIndex mi_A, const QModelIndex mi_B);
	void setContent(const std::vector<Item>& items);

public:
	AddMBsSWsDlg(QWidget *parent, std::vector<mb_dt> supportedMBs, std::vector<sw_dt> supportedSWs,
	             std::vector<MBSWmetadata_dt> *MBSWmetaList);
	~AddMBsSWsDlg();

private slots:
	void add();
	void cancel();
	void setAddButtonEnableStatus();

};



#endif

