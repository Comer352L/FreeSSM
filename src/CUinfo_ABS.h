/*
 * CUinfo_ABS.h - Widget for displaying ABS Control Unit information
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

#ifndef CUINFO_ABS_H
#define CUINFO_ABS_H


#include <QtGui>
#include "ui_CUinfo_ABS.h"



class CUinfo_ABS : public QWidget, private Ui::CUinfo_ABS_Form
{
	Q_OBJECT

public:
	CUinfo_ABS(QWidget * parent = 0);
	~CUinfo_ABS();
	void setABSTypeText(QString Type);
	void setRomIDText(QString RomID);
	void setNrOfSupportedMBsSWs(unsigned int MBs, unsigned int SWs);
	void setOBD2Supported(bool sup);

private:
	void setupUiFonts();

};



#endif
