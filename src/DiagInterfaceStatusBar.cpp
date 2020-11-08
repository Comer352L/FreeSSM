/*
 * DiagInterfaceStatusBar.cpp - Status bar widget for the diagnostic interface
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

#include "DiagInterfaceStatusBar.h"



DiagInterfaceStatusBar::DiagInterfaceStatusBar(QWidget *parent, Qt::WindowFlags flags) : QWidget(parent, flags)
{
	QFont labelfont = this->font();
	labelfont.setPointSize(9);
	// Layout:
	_hboxlayout = new QHBoxLayout(this);
	_hboxlayout->setContentsMargins(0, 0, 0, 0);
	setLayout(_hboxlayout);
	// Interface name:
	_if_name_title_label = new QLabel(this);
	_if_name_title_label->setFont(labelfont);
	_if_name_title_label->setText("Interface:");
	_hboxlayout->addWidget(_if_name_title_label);
	_if_name_label = new QLabel(this);
	_if_name_label->setFont(labelfont);
	_if_name_label->setFrameShape(QFrame::Panel);
	_if_name_label->setFrameShadow(QFrame::Sunken);
	_if_name_label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	_if_name_label->setMinimumSize(220, _if_name_label->minimumHeight());
	_hboxlayout->addWidget(_if_name_label);
	// Interface version
	_if_version_title_label = new QLabel(this);
	_if_version_title_label->setFont(labelfont);
	_if_version_title_label->setText("Version:");
	_hboxlayout->addWidget(_if_version_title_label);
	_if_version_label = new QLabel(this);
	_if_version_label->setFont(labelfont);
	_if_version_label->setFrameShape(QFrame::Panel);
	_if_version_label->setFrameShadow(QFrame::Sunken);
	_if_version_label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	_if_version_label->setMinimumSize(40, _if_version_label->minimumHeight());
	_hboxlayout->addWidget(_if_version_label);
	// Horizontal spacer
	_hboxlayout->addStretch();
	// Protocol
	_protocol_name_title_label = new QLabel(this);
	_protocol_name_title_label->setFont(labelfont);
	_protocol_name_title_label->setText("Protocol:");
	_hboxlayout->addWidget(_protocol_name_title_label);
	_protocol_name_label = new QLabel(this);
	_protocol_name_label->setFont(labelfont);
	_protocol_name_label->setFrameShape(QFrame::Panel);
	_protocol_name_label->setFrameShadow(QFrame::Sunken);
	_protocol_name_label->setFixedWidth(125);
	_protocol_name_label->setAlignment(Qt::AlignHCenter);
	_hboxlayout->addWidget(_protocol_name_label);
	// Baud rate
	_baudrate_title_label = new QLabel(this);
	_baudrate_title_label->setFont(labelfont);
	_baudrate_title_label->setText("Baud rate:");
	_hboxlayout->addWidget(_baudrate_title_label);
	_baudrate_label = new QLabel(this);
	_baudrate_label->setFont(labelfont);
	_baudrate_label->setFrameShape(QFrame::Panel);
	_baudrate_label->setFrameShadow(QFrame::Sunken);
	_baudrate_label->setFixedWidth(75);
	_baudrate_label->setAlignment(Qt::AlignHCenter);
	_hboxlayout->addWidget(_baudrate_label);
}

DiagInterfaceStatusBar::~DiagInterfaceStatusBar()
{
	delete _hboxlayout;
	delete _if_name_title_label;
	delete _if_name_label;
	delete _if_version_title_label;
	delete _if_version_label;
	delete _protocol_name_title_label;
	delete _protocol_name_label;
	delete _baudrate_title_label;
	delete _baudrate_label;
}

void DiagInterfaceStatusBar::setInterfaceName(QString name, QColor textcolor)
{
	// Set text and text color:
	setTextContentAndColor(_if_name_label, name, textcolor);
}

void DiagInterfaceStatusBar::setInterfaceVersion(QString version, QColor textcolor)
{
	// Set text and text color:
	setTextContentAndColor(_if_version_label, version, textcolor);
}

void DiagInterfaceStatusBar::setProtocolName(QString name, QColor textcolor)
{
	setTextContentAndColor(_protocol_name_label, name, textcolor);
}

void DiagInterfaceStatusBar::setBaudRate(QString baudrate, QColor textcolor)
{
	setTextContentAndColor(_baudrate_label, baudrate, textcolor);
}

void DiagInterfaceStatusBar::setTextContentAndColor(QLabel *label, QString text, QColor textcolor)
{
	// Set Color
	QPalette palette = label->palette();
	palette.setColor(QPalette::Active, QPalette::WindowText, textcolor);
	palette.setColor(QPalette::Inactive, QPalette::WindowText, textcolor);
	label->setPalette(palette);
	// Set Text
	label->setText(text);
}

