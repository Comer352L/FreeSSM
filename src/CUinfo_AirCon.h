/*
 * CUinfo_AirCon.h - Widget for displaying Automatic Air Conditioning Control Unit information
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

#ifndef CUINFO_AIRCON_H
#define CUINFO_AIRCON_H


#include <QtGui>
#include "ui_CUinfo_AirCon.h"



class CUinfo_AirCon : public QWidget, private Ui::CUinfo_AirCon_Form
{
	Q_OBJECT

public:
	CUinfo_AirCon(QWidget * parent = 0);
	~CUinfo_AirCon();
	void setAirConTypeText(QString Type);
	void setRomIDText(QString RomID);
	void setNrOfSupportedMBsSWs(unsigned int MBs, unsigned int SWs);

private:
	void setupUiFonts();

};



#endif
