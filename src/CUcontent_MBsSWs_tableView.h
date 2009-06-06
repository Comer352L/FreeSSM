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
	CUcontent_MBsSWs_tableView(QWidget *parent, bool showMin=true, bool showMax=true);
	~CUcontent_MBsSWs_tableView();
	void setMWSWlistContent(QStringList titles, QStringList values, QStringList minValues, QStringList maxValues, QStringList units);
	void updateMBSWvalues(QStringList valueStrList, QStringList minValueStrList, QStringList maxValueStrList, QStringList unitStrList);
	void clearMBSWlistContent();
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
	void toggleMinColumnVisible(bool show);
	void toggleMaxColumnVisible(bool show);

signals:
	void moveUpButton_pressed();
	void moveDownButton_pressed();
	void resetMinMaxButton_pressed();
	void itemSelectionChanged();

};



#endif
