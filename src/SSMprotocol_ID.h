/*
 * SSMprotocol_ID.h - Identification of vehicle and control unit details
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

#ifndef SSMPROTOCOL_ID_H
#define SSMPROTOCOL_ID_H


#include <QObject>
#include <QStringList>


class SSMprotocol_ID
{

private:
	QStringList _engine_sysDefs;
	QStringList _transmission_sysDefs;

public:
	enum sysIDtype_dt {ECU_sysID=1, TCU_sysID=2};
	SSMprotocol_ID();
	bool getSysDescriptionBySysID(sysIDtype_dt sysIDtype, char *sysID, QString *sysDescription);

};


#endif
