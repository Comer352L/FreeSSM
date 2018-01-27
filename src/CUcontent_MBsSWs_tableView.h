/*
 * CUcontent_MBsSWs_tableView.h - Widget for displaying MB/SW values in a table
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

#ifndef CUCONTENT_MBSSWS_TABLEVIEW_H
#define CUCONTENT_MBSSWS_TABLEVIEW_H



#include <QtGui>
#include <QtGlobal>
#include "ui_CUcontent_MBsSWs_tableView.h"
#include "SSMprotocol.h"



class CUcontent_MBsSWs_tableView : public QWidget, private Ui::MBSWtable_Form
{
	Q_OBJECT

public:
	CUcontent_MBsSWs_tableView(QWidget *parent, bool showMin=true, bool showMax=true);
	~CUcontent_MBsSWs_tableView();
	void setMBSWlistContent(const std::vector<BlockType>& types, const std::vector<QString>& titles, const std::vector<QString>& values,
				const std::vector<QString>& minValues, const std::vector<QString>& maxValues, const std::vector<QString>& units);
	void updateMBSWvalues(const std::vector<QString>& valueStrList, const std::vector<QString>& minValueStrList,
			      const std::vector<QString>& maxValueStrList, const std::vector<QString>& unitStrList  );
	void clearMBSWlistContent();
	bool minValuesEnabled() const;
	bool maxValuesEnabled() const;
	std::vector<unsigned int> getSelectedTableWidgetRows() const;
	void selectMBSWtableRows(unsigned int start, unsigned int end);
	void scrollMBSWtable(unsigned int rowindex);

private:
	enum Column { type, title, min, current, max, unit };

	QIcon iconMB;
	QIcon iconSW;
	unsigned int _nrofMBsSWs;
	unsigned int _maxrowsvisible;

	void resizeEvent(QResizeEvent *event);
	bool eventFilter(QObject *obj, QEvent *event);
	void updateMBColumn(const std::vector<QString>& data, CUcontent_MBsSWs_tableView::Column col, Qt::Alignment alignment = Qt::AlignCenter);
	void updateTypesColumn(const std::vector<BlockType>& types);

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
