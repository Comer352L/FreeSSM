/*
 * SSMFlagbyteDefinitions_en.h - SSM flagbyte definitions (en)
 *
 * Copyright (C) 2008-2019 Comer352L
 * Copyright (C) 2019 madanadam (Turkish translation)
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


#ifndef SSMFLAGBYTEDEFINITIONS_TR_H
#define SSMFLAGBYTEDEFINITIONS_TR_H


#include <QStringList>


class SSMFlagbyteDefinitions_tr
{

private:
	static const QStringList _MB_defs_tr;
	static const QStringList _SW_defs_tr;
	static const QStringList _DTC_SUBARU_defs_tr;
	static const QStringList _DTC_OBD_defs_tr;
	static const QStringList _CC_defs_tr;
	static const QStringList _actuator_defs_tr;
	static const QStringList _adjustment_defs_tr;

public:
	const QStringList MBrawDefs();
	const QStringList SWrawDefs();
	const QStringList SUBDTCrawDefs();
	const QStringList OBDDTCrawDefs();
	const QStringList CCCCrawDefs();
	const QStringList ActuatorRawDefs();
	const QStringList AdjustmentRawDefs();

};


#endif
