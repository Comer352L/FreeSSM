/*
 * About.h - Display informations about the FreeSSM software
 *
 * Copyright © 2008-2009 Comer352l
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

#ifndef ABOUT_H
#define ABOUT_H



#include <QtGui>
#include "ui_About.h"
#include "SSMprotocol_def_en.h"



class About : public QDialog, private Ui::about_Dialog
{
	Q_OBJECT

public:
	About(QWidget *parent = 0, QString version = "", QString language = "");

private:
	void setupUiFonts();

};



#endif
