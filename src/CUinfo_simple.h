/*
 * CUinfo_simple.h - Basic widget for displaying Control Unit information
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

#ifndef CUINFO_SIMPLE_H
#define CUINFO_SIMPLE_H


#include <QtGui>
#include "ui_CUinfo_simple.h"



class CUinfo_simple : public QWidget, private Ui::CUinfo_simple_Form
{
	Q_OBJECT

public:
	CUinfo_simple(QWidget * parent = 0);
	~CUinfo_simple();
	void setSystemTypeText(QString Type);
	void setRomIDText(QString RomID);
	void setNrOfSupportedMBsSWs(unsigned int MBs, unsigned int SWs);

};



#endif
