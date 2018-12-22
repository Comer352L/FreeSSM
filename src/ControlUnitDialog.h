/*
 * ControlUnitDialog.h - Template for Control Unit dialogs
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

#ifndef CONTROLUNITDIALOG_H
#define CONTROLUNITDIALOG_H


#include <QtGui>
#include "ui_ControlUnitDialog.h"
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

	QPushButton * addContent(ContentSelection csel);
	SSMprotocol::CUsetupResult_dt probeProtocol(SSMprotocol::CUtype_dt CUtype);
	void setInfoWidget(QWidget *infowidget);
	void setContentWidget(QString title, QWidget *contentwidget);
	QWidget * contentWidget();
	void setContentSelectionButtonEnabled(ContentSelection csel, bool enabled);
	void setContentSelectionButtonChecked(ContentSelection csel, bool checked);

private:
	QString _language;
	AbstractDiagInterface *_diagInterface;
	QWidget *_infoWidget;
	QWidget *_contentWidget;
	DiagInterfaceStatusBar *_ifstatusbar;
	QMap<ContentSelection, QPushButton*> _contentSelectionButtons;

	void closeEvent(QCloseEvent *event);

protected slots:
	void communicationError(QString addstr = "");

};



#endif
