/*
 * CUinfo_Engine.h - Widget for displaying Engine Control Unit information
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

#ifndef CUINFO_ENGINE_H
#define CUINFO_ENGINE_H


#include <QtGui>
#include "ui_CUinfo_Engine.h"



class CUinfo_Engine : public QWidget, private Ui::CUinfo_Engine_Form
{
	Q_OBJECT

public:
	CUinfo_Engine(QWidget * parent = 0);
	~CUinfo_Engine();
	void setEngineTypeText(QString Type);
	void setRomIDText(QString RomID);
	void setVINinfo(bool VINsupported, QString VIN);
	void setNrOfSupportedMBsSWs(unsigned int MBs, unsigned int SWs);
	void setOBD2Supported(bool sup);
	void setIntegratedCCSupported(bool sup);
	void setImmobilizerSupported(bool sup);

};



#endif
