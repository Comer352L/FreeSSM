/*
 * CUcontent_MBsSWs.h - Widget for Reading of Measuring Blocks and Switches
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

#ifndef CUCONTENT_MBSSWS_H
#define CUCONTENT_MBSSWS_H



#include <QtGui>
#include <vector>
#include "ui_CUcontent_MBsSWs.h"
#include "CUcontent_MBsSWs_tableView.h"
#include "AddMBsSWsDlg.h"
#include "SSMprotocol.h"
#include "libFSSM.h"


class MBSWvalue_dt
{
public:
	MBSWvalue_dt() { rawValue=0; };
	unsigned int rawValue;
	QString scaledStr;
	QString unitStr;
};



class MinMaxMBSWvalue_dt
{
public:
	MinMaxMBSWvalue_dt() { disabled=false; minRawValue=0; minRawValue=0; };
	bool disabled;
	double minRawValue;
	double maxRawValue;
	QString minScaledValueStr;
	QString maxScaledValueStr;
};



class MBSWsettings_dt
{
public:
	MBSWsettings_dt() { timeMode=0; minValuesEnabled=1; maxValuesEnabled=1; };
	bool timeMode;
	bool minValuesEnabled;
	bool maxValuesEnabled;
};



class CUcontent_MBsSWs : public QWidget, private Ui::MBSWcontent_Form
{
	Q_OBJECT

public:
	CUcontent_MBsSWs(MBSWsettings_dt options = MBSWsettings_dt(), QWidget *parent = 0);
	~CUcontent_MBsSWs();
	bool setup(SSMprotocol *SSMPdev);
	bool startMBSWreading();
	bool stopMBSWreading();
	bool setMBSWselection(std::vector<MBSWmetadata_dt> MBSWmetaList);
	void getMBSWselection(std::vector<MBSWmetadata_dt> *MBSWmetaList);
	void getSettings(MBSWsettings_dt *settings);

private:
	SSMprotocol *_SSMPdev;
	QLabel *_MBSWrefreshTimeTitle_label;
	QLabel *_MBSWrefreshTimeValue_label;
	QPushButton *_timemode_pushButton;
	CUcontent_MBsSWs_tableView *_valuesTableView;
	std::vector<mb_dt> _supportedMBs;
	std::vector<sw_dt> _supportedSWs;
	std::vector<MBSWmetadata_dt> _MBSWmetaList;
	bool _timemode;
	int _lastrefreshduration_ms;
	QList<MBSWvalue_dt> _lastValues;
	QList<MinMaxMBSWvalue_dt> _minmaxData;
	QList<unsigned int> _tableRowPosIndexes; /* index of the row at which the MB/SW is displayed in the values-table-widget */
	
	void setupTimeModeUiElements();
	void setupUiFonts();
	void displayMBsSWs();
	void updateTimeInfo(int refreshduration_ms);
	void communicationError(QString addstr);
	void resizeEvent(QResizeEvent *event);

private slots:
	void startstopMBsSWsButtonPressed();
	void callStart();
	void callStop();
	void processMBSWRawValues(std::vector<unsigned int> rawValues, int refreshduration_ms);
	void addMBsSWs();
	void deleteMBsSWs();
	void moveUpMBsSWsOnTheTable();
	void moveDownMBsSWsOnTheTable();
	void resetMinMaxTableValues();
	void setDeleteButtonEnabledState();
	void switchTimeMode();

signals:
	void error();

};



#endif
