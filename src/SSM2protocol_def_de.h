/*
 * SSM2protocol_def_de.h - SSM2-protocol-definitions
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


#ifndef SSM2PROTOCOL_DEF_DE_H
#define SSM2PROTOCOL_DEF_DE_H


#include <QStringList>


class SSM2protocol_def_de
{

private:
	QStringList _MB_defs_de;
	QStringList _SW_defs_de;
	QStringList _DTC_SUBARU_defs_de;
	QStringList _DTC_OBD_defs_de;
	QStringList _CC_defs_de;
	QStringList _actuator_defs_de;
	QStringList _adjustment_defs_de;

public:
	SSM2protocol_def_de();
	const QStringList MBrawDefs();
	const QStringList SWrawDefs();
	const QStringList SUBDTCrawDefs();
	const QStringList OBDDTCrawDefs();
	const QStringList CCCCrawDefs();
	const QStringList ActuatorRawDefs();
	const QStringList AdjustmentRawDefs();

};


#endif
