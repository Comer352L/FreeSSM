/*
 * CUcontent_DCs_abstract.h - Abstract widget for Diagnostic Codes Reading
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

#ifndef CUCONTENT_DCS_ABSTRACT_H
#define CUCONTENT_DCS_ABSTRACT_H


#include <QtGlobal>	/* required for QT_VERSION */
#if QT_VERSION < 0x050000
	#include <QtGui>
#else
	#include <QtWidgets>
#endif
#include <QPrinter>
#include <QPrintDialog>
#include "SSMprotocol.h"


class CUcontent_DCs_abstract : public QWidget
{
	Q_OBJECT

public:
	CUcontent_DCs_abstract(QWidget *parent);
	virtual ~CUcontent_DCs_abstract();
	virtual bool setup(SSMprotocol *SSMPdev) = 0;
	bool startDCreading();
	bool stopDCreading();

protected:
	SSMprotocol * _SSMPdev;
	int _supportedDCgroups;

	virtual void connectGUIelements() = 0;
	virtual void disconnectGUIelements() = 0;
	void setDCtableContent(QTableWidget *tableWidget, QStringList DCs, QStringList DCdescriptions);
	void setNrOfTableRows(QTableWidget *tablewidget, unsigned int nrofUsedRows);
#ifndef SMALL_RESOLUTION
	virtual void createDCprintTables(QTextCursor cursor) = 0;
	void insertDCprintTable(QTextCursor cursor, QString title, QStringList codes, QStringList descriptions);
#endif
	void communicationError(QString errstr);

protected slots:
	void callStart();
	void callStop();
#ifndef SMALL_RESOLUTION
	void printDCprotocol();
#endif

signals:
	void error();

};


#endif
