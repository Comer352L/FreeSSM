/*
 * ControlUnitDialog.h - Template for Control Unit dialogs
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

#ifndef CONTROLUNITDIALOG_H
#define CONTROLUNITDIALOG_H


#include <QtGui>
#include "ui_ControlUnitDialog.h"
#include "CUcontent_DCs_abstract.h"
#include "CUcontent_MBsSWs.h"
#include "CUcontent_Adjustments.h"
#include "CUcontent_sysTests.h"
#include "DiagInterfaceStatusBar.h"
#include "AbstractDiagInterface.h"
#include "SSMprotocol1.h"
#include "SSMprotocol2.h"
#include "FSSMdialogs.h"



class ControlUnitDialog : public QDialog, private Ui::ControlUnit_Dialog
{
	Q_OBJECT

public:
	enum class ContentSelection {DCsMode, MBsSWsMode, AdjustmentsMode, SysTestsMode, ClearMemoryFcn, ClearMemory2Fcn};

	ControlUnitDialog(QString title, AbstractDiagInterface *diagInterface, QString language);
	~ControlUnitDialog();

protected:
	enum class Mode {None, DCs, MBsSWs, Adjustments, SysTests};

	SSMprotocol *_SSMPdev;
	bool _setup_done;
	Mode _mode;
	// Content backup parameters:
	std::vector<MBSWmetadata_dt> _lastMBSWmetaList;
	MBSWsettings_dt _MBSWsettings;
	// Content widgets:
	CUcontent_DCs_abstract *_content_DCs;
	CUcontent_MBsSWs *_content_MBsSWs;
	CUcontent_Adjustments *_content_Adjustments;
	CUcontent_sysTests *_content_SysTests;

	void addContent(ContentSelection csel);
	SSMprotocol::CUsetupResult_dt probeProtocol(SSMprotocol::CUtype_dt CUtype);
	void setInfoWidget(QWidget *infowidget);
	void setContentWidget(QString title, QWidget *contentwidget);
	QWidget * contentWidget();
	void setContentSelectionButtonEnabled(ContentSelection csel, bool enabled);
	void setContentSelectionButtonChecked(ContentSelection csel, bool checked);
	bool getParametersFromCmdLine(QStringList *cmdline_args, QString *selection_file, bool *autostart);
	bool startDCsMode();
	bool startMBsSWsMode();
	bool startAdjustmentsMode();
	bool startSystemOperationTestsMode();

private:
	QString _language;
	AbstractDiagInterface *_diagInterface;
	QWidget *_infoWidget;
	QWidget *_contentWidget;
	DiagInterfaceStatusBar *_ifstatusbar;
	QMap<ContentSelection, QPushButton*> _contentSelectionButtons;

	virtual CUcontent_DCs_abstract * allocate_DCsContentWidget() = 0;
	void runClearMemory(SSMprotocol::CMlevel_dt level);
	void saveContentSettings();
	void closeEvent(QCloseEvent *event);

protected slots:
	void clearMemory();
	void clearMemory2();
	void communicationError(QString addstr = "");

private slots:
	void switchToDCsMode();
	void switchToMBsSWsMode();
	void switchToAdjustmentsMode();
	void switchToSystemOperationTestsMode();

};



#endif
