/*
 * CUinfo_simple.cpp - Basic widget for displaying Control Unit information
 *
 * Copyright (C) 2008-2012 Comer352L
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

#include "CUinfo_simple.h"


CUinfo_simple::CUinfo_simple(QString sysTitle, QWidget * parent) : QWidget(parent)
{
	setupUi(this);
	setupUiFonts();
	transmissiontypetitle_label->setText(sysTitle + " " + tr("Type") + ":");
}


CUinfo_simple::~CUinfo_simple()
{
}


void CUinfo_simple::setSystemTypeText(QString Type)
{
	transmissiontype_label->setText(Type);
}


void CUinfo_simple::setRomIDText(QString RomID)
{
	romID_label->setText(RomID);
}


void CUinfo_simple::setNrOfSupportedMBsSWs(unsigned int MBs, unsigned int SWs)
{
	nrofdatambs_label->setText( QString::number(MBs, 10) );
	nrofswitches_label->setText( QString::number(SWs, 10) );
}


void CUinfo_simple::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS OF ui_CUinfo_Transmission.h (made with QDesigner)
	QFont appfont = QApplication::font();
	QFont font = this->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	this->setFont(font);
	font = transmissiontypetitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	transmissiontypetitle_label->setFont(font);
	font = transmissiontype_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	transmissiontype_label->setFont(font);
	font = romIDtitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	romIDtitle_label->setFont(font);
	font = romID_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	romID_label->setFont(font);
	font = nrofmbsswstitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	nrofmbsswstitle_label->setFont(font);
	font = nrofdatambstitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	nrofdatambstitle_label->setFont(font);
	font = nrofdatambs_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	nrofdatambs_label->setFont(font);
	font = nrofswitchestitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	nrofswitchestitle_label->setFont(font);
	font = nrofswitches_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	nrofswitches_label->setFont(font);
}

