/*
 * CUcontent_MBsSWs.h - Widget for Reading of Measuring Blocks and Switches
 *
 * Copyright (C) 2008-2019 Comer352L
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



#include <QtGlobal>	/* required for QT_VERSION */
#if QT_VERSION < 0x050000
	#include <QtGui>
#else
	#include <QtWidgets>
#endif
#include <vector>
#include "ui_CUcontent_MBsSWs.h"
#include "CUcontent_MBsSWs_tableView.h"
#include "AddMBsSWsDlg.h"
#include "SSMprotocol.h"
#include "libFSSM.h"

// To get stdout for my errors
#include <iostream>
// To do binary file operations storing the current setup
#include <fstream>

class MBSWvalue_dt
{
public:
	unsigned int rawValue {0};
	QString scaledStr;
	QString unitStr;
};



class MinMaxMBSWvalue_dt
{
public:
	bool disabled {false};
	double minRawValue {0};
	double maxRawValue {0};
	QString minScaledValueStr;
	QString maxScaledValueStr;
};



enum class TimeMode {refreshDuration, dataRate};


class MBSWsettings_dt
{
public:
	TimeMode timeMode {TimeMode::refreshDuration};
	bool minValuesEnabled {true};
	bool maxValuesEnabled {true};
};



class CUcontent_MBsSWs : public QWidget, private Ui::MBSWcontent_Form
{
	Q_OBJECT

public:
	CUcontent_MBsSWs(MBSWsettings_dt options = MBSWsettings_dt(), QWidget *parent = nullptr);
	~CUcontent_MBsSWs();
	bool setup(SSMprotocol *SSMPdev);
	bool setMBSWselection(const std::vector<MBSWmetadata_dt>& MBSWmetaList);
	std::vector<MBSWmetadata_dt> getMBSWselection() const;
	size_t numMBsSWsSelected();
	MBSWsettings_dt getSettings() const;

private:
	const static QString DefaultTimeValStr;

	SSMprotocol *_SSMPdev;
	QLabel *_MBSWrefreshTimeTitle_label;
	QLabel *_MBSWrefreshTimeValue_label;
	QPushButton *_timemode_pushButton;
	CUcontent_MBsSWs_tableView *_valuesTableView;
	std::vector<mb_dt> _supportedMBs;
	std::vector<sw_dt> _supportedSWs;
	std::vector<MBSWmetadata_dt> _MBSWmetaList;
	TimeMode _timemode;
	int _lastrefreshduration_ms;
	std::vector<MBSWvalue_dt> _lastValues;
	std::vector<MinMaxMBSWvalue_dt> _minmaxData;
	std::vector<unsigned int> _tableRowPosIndexes; /* index of the row at which the MB/SW is displayed in the values-table-widget */
	bool _MBSWreading;

	void setupTimeModeUiElements();
	bool validateMBSWselection(const std::vector<MBSWmetadata_dt>& MBSWmetaList);
	void setMBSWselectionUnvalidated(const std::vector<MBSWmetadata_dt>& MBSWmetaList);
	void displayMBsSWs();
	void updateRefreshTimeTitle();
	void updateTimeInfo(int refreshduration_ms);
	void clearRefreshTime();
	void labelStartStopButtonReadyForStart();
	void communicationError(QString addstr);
	void moveRefreshTimeUiElements();
	void resizeEvent(QResizeEvent *event);
	void errorMsg(QString title, QString message);

public slots:
	bool startMBSWreading();
	bool loadMBsSWs(QString filename = "");

private slots:
	void startstopMBsSWsButtonPressed();
	bool stopMBSWreading();
	void processMBSWRawValues(const std::vector<unsigned int>& rawValues, int refreshduration_ms);
	void addMBsSWs();
	void deleteMBsSWs();
	bool saveMBsSWs(QString filename = "");
	void moveUpMBsSWsOnTheTable();
	void moveDownMBsSWsOnTheTable();
	void resetMinMaxTableValues();
	void setDeleteButtonEnabledState();
	void switchTimeMode();

signals:
	void error();

};



#endif
