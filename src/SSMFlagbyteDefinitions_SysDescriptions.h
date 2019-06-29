/*
 * SSMFlagbyteDefinitions_SysDescriptions.h - System descriptions for Control Units using the SSM-protocol with flagbytes
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

#ifndef SSMFLAGBYTEDEFINITIONS_SYSDESCRIPTIONS_H
#define SSMFLAGBYTEDEFINITIONS_SYSDESCRIPTIONS_H


#include <QObject>
#include <QStringList>
#include <vector>


class SSMFlagbyteDefinitions_SysDescriptions
{

private:
	QStringList _engine_sysDefs;
	QStringList _transmission_sysDefs;

public:
	enum class SystemType {ECU = 1, TCU = 2};
	SSMFlagbyteDefinitions_SysDescriptions();
	bool getSysDescriptionBySysID(SystemType sysType, const std::vector<char>& sysID, QString *sysDescription);

};


#endif
