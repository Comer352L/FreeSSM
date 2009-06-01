/*
 * CUcontent_MBsSWs_tableView.h - Widget for displaying MB/SW values in a table
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

#ifndef CUCONTENT_MBSSWS_TABLEVIEW_H
#define CUCONTENT_MBSSWS_TABLEVIEW_H



#include <QtGui>
#include "ui_CUcontent_MBsSWs_tableView.h"



class CUcontent_MBsSWs_tableView : public QWidget, private Ui::MBSWtable_Form
{
	Q_OBJECT

public:
	CUcontent_MBsSWs_tableView(QWidget *parent);
	~CUcontent_MBsSWs_tableView();
	void setMWSWlistContent(QStringList titles, QStringList values, QStringList units);
	void clearMBSWlistContent();
	void updateMBSWvalues(QStringList MBvalueStrList, QStringList MBunitStrList);
	void getSelectedTableWidgetRows(QList<unsigned int> *selectedMBSWIndexes);
	void selectMBSWtableRows(unsigned int start, unsigned int end);
	void scrollMBSWtable(unsigned int rowindex);

private:
	unsigned int _nrofMBsSWs;
	unsigned int _maxrowsvisible;

	void setupUiFonts();
	void resizeEvent(QResizeEvent *event);
	bool eventFilter(QObject *obj, QEvent *event);

private slots:
	void setMoveButtonsEnabledState();

signals:
	void moveUpButton_pressed();
	void moveDownButton_pressed();
	void itemSelectionChanged();

};



#endif
