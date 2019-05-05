/*
 * CUinfo_Transmission.h - Widget for displaying Transmission Control Unit information
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

#ifndef CUINFO_TRANSMISSION_H
#define CUINFO_TRANSMISSION_H


#include <QWidget>
#include <QString>
#include "ui_CUinfo_Transmission.h"
#include "CUinfo_abstract.h"


class CUinfo_Transmission : public CUinfo_abstract, private Ui::CUinfo_Transmission_Form
{

public:
	CUinfo_Transmission(QWidget * parent = 0);
	~CUinfo_Transmission();
	void setSystemTypeText(QString Type);
	void setRomIDText(QString RomID);
	void setNrOfSupportedMBsSWs(unsigned int MBs, unsigned int SWs);
	void setOBD2Supported(bool sup);

};



#endif
