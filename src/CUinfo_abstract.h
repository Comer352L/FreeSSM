/*
 * CUinfo_abstract.h - Abstract widget for displaying Control Unit information
 *
 * Copyright (C) 2019 Comer352L
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

#ifndef CUINFO_ABSTRACT_H
#define CUINFO_ABSTRACT_H


#include <QWidget>
#include <QString>


class CUinfo_abstract : public QWidget
{

public:
	CUinfo_abstract(QWidget * parent = 0) : QWidget(parent) {};
	virtual void setSystemTypeText(QString Type) = 0;
	virtual void setRomIDText(QString RomID) = 0;

};


#endif
