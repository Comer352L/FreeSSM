/*
 * SSMprotocol_ID.cpp - Identification of vehicle and control unit details
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

#include "SSMprotocol_ID.h"


SSMprotocol_ID::SSMprotocol_ID()
{
	_engine_sysDefs
	<< "01;2.5L SOHC"
	<< "02;2.5L SOHC"
	<< "03;2.2L SOHC"
	<< "04;2.2L SOHC"
	<< "05;1.5L SOHC"
	<< "06;1.6L SOHC"
	<< "07;1.8L SOHC"
	<< "08;2.0L SOHC"
	<< "09;2.0L DOHC"
	<< "0A;2.5L DOHC"
	<< "0B;2.0L DOHC Turbo"
	<< "0C;2.0L DOHC Turbo"
	<< "0D;2.0L DOHC Turbo"
	<< "0E;3.0L DOHC"
	<< "0F;2.0L DOHC Turbo"
	<< "10;2.5L DOHC"
	<< "11;2.5L DOHC Turbo"
	<< "12;3.0L DOHC"
	<< "13;1.5L DOHC"
	<< "14;2.0L DOHC Turbo Diesel"
	<< "15;3.6L DOHC";

	_transmission_sysDefs
	<< "01;E-4AT"
	<< "02;E-4AT"
	<< "03;E-4AT"
	<< "04;E-4AT"
	<< "05;E-4AT"
	<< "06;E-4AT"
	<< "07;E-4AT"
	<< "08;E-4AT"
	<< "09;E-4AT"
	<< "10;E-4AT"
	<< "11;E-4AT"
	<< "12;E-4AT"
	<< "13;E-4AT"
	<< "20;E-4AT"
	<< "21;E-4AT"
	<< "22;E-5AT"
	<< "23;E-4AT"
	<< "24;E-4AT"
	<< "25;E-4AT"
	<< "30;" + QObject::tr("Center Differential");
}


bool SSMprotocol_ID::getSysDescriptionBySysID(sysIDtype_dt sysIDtype, char *sysID, QString *sysDescription)
{
	QStringList *sysDefs = NULL;
	unsigned char k = 0;
	bool ok = false;

	sysDescription->clear();
	if (sysIDtype == ECU_sysID)
	{
		sysDefs = &_engine_sysDefs;
	}
	else if (sysIDtype == TCU_sysID)
	{
		sysDefs = &_transmission_sysDefs;
	}
	else
	{
		return false;
	}
	if (sysID[1]=='\x10')
	{
		for (k=0; k<sysDefs->size(); k++)
		{
			if (sysDefs->at(k).section(';', 0, 0).toInt(&ok, 16) == sysID[2])
			{
				*sysDescription = sysDefs->at(k).section(';', 1, 1);
				return true;
			}
		}
	}
	return false;
}

