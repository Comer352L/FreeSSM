/*
 * DiagInterfaceStatusBar.h - Status bar widget for the diagnostic interface
 *
 * Copyright (C) 2012-2018 Comer352L
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

#ifndef DIAGINTERFACESTATUSBAR_H
#define DIAGINTERFACESTATUSBAR_H


#include <QtGlobal>	/* required for QT_VERSION */
#if QT_VERSION < 0x050000
	#include <QtGui>
#else
	#include <QtWidgets>
#endif


class DiagInterfaceStatusBar : public QWidget
{
	Q_OBJECT

public:
	DiagInterfaceStatusBar(QWidget *parent = 0, Qt::WindowFlags flags = Qt::Widget);
	~DiagInterfaceStatusBar();
	void setInterfaceName(QString name, QColor textcolor = Qt::black);
	void setInterfaceVersion(QString version, QColor textcolor = Qt::black);
	void setProtocolName(QString name, QColor textcolor = Qt::black);
	void setBaudRate(QString baudrate, QColor textcolor = Qt::black);

private:
	QHBoxLayout *_hboxlayout;
	QLabel *_if_name_title_label;
	QLabel *_if_name_label;
	QLabel *_if_version_title_label;
	QLabel *_if_version_label;
	QLabel *_protocol_name_title_label;
	QLabel *_protocol_name_label;
	QLabel *_baudrate_title_label;
	QLabel *_baudrate_label;

	void setTextContentAndColor(QLabel *label, QString text, QColor textcolor);
};


#endif
