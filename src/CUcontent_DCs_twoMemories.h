/*
 * CUcontent_DCs_twoMemories.h - Widget for Diagnostic Codes Reading with two memories
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

#ifndef CUCONTENT_DCS_TWOMEMORIES_H
#define CUCONTENT_DCS_TWOMEMORIES_H


#include <QtGui>
#include <string>
#include "CUcontent_DCs_abstract.h"
#include "ui_CUcontent_DCs_twoMemories.h"
#include "SSMprotocol.h"


class CUcontent_DCs_twoMemories : public CUcontent_DCs_abstract, private Ui::DCcontent_twoMemories_Form
{
	Q_OBJECT

public:
	CUcontent_DCs_twoMemories(QWidget *parent = 0);
	~CUcontent_DCs_twoMemories();
	bool setup(SSMprotocol *SSMPdev);

private:
	QStringList _currOrTempDTCs;
	QStringList _currOrTempDTCdescriptions;
	QStringList _histOrMemDTCs;
	QStringList _histOrMemDTCdescriptions;

	void connectGUIelements();
	void disconnectGUIelements();
	void resizeEvent(QResizeEvent *event);
	bool eventFilter(QObject *obj, QEvent *event);
#ifndef SMALL_RESOLUTION
	void createDCprintTables(QTextCursor cursor);
#endif

private slots:
	void updateCurrentOrTemporaryDTCsContent(QStringList currOrTempDTCs, QStringList currOrTempDTCdescriptions);
	void updateHistoricOrMemorizedDTCsContent(QStringList histOrMemDTCs, QStringList histOrMemDTCdescriptions);

};


#endif
