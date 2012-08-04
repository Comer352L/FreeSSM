/*
 * CUinfo_ABS.cpp - Widget for displaying ABS Control Unit information
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

#include "CUinfo_ABS.h"


CUinfo_ABS::CUinfo_ABS(QWidget * parent) : QWidget(parent)
{
	setupUi(this);
	setupUiFonts();
}


CUinfo_ABS::~CUinfo_ABS()
{
}


void CUinfo_ABS::setABSTypeText(QString Type)
{
	abstype_label->setText(Type);
}


void CUinfo_ABS::setRomIDText(QString RomID)
{
	romID_label->setText(RomID);
}


void CUinfo_ABS::setNrOfSupportedMBsSWs(unsigned int MBs, unsigned int SWs)
{
	nrofdatambs_label->setText( QString::number(MBs, 10) );
	nrofswitches_label->setText( QString::number(SWs, 10) );
}


void CUinfo_ABS::setOBD2Supported(bool sup)
{
	if (sup)
		obd2system_label->setPixmap(QPixmap(QString::fromUtf8(":/icons/chrystal/22x22/ok.png")));
	else
		obd2system_label->setPixmap(QPixmap(QString::fromUtf8(":/icons/chrystal/22x22/editdelete.png")));
}


void CUinfo_ABS::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS OF ui_CUinfo_ABS.h (made with QDesigner)
	QFont appfont = QApplication::font();
	QFont font = this->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	this->setFont(font);
	font = abstypetitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	abstypetitle_label->setFont(font);
	font = abstype_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	abstype_label->setFont(font);
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
	font = obd2systemTitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	obd2systemTitle_label->setFont(font);
}

