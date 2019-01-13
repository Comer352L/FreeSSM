/*
 * CmdLine.h - Command Line processing
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

#ifndef CMDLINE_H
#define CMDLINE_H


#include <QString>
#include <QStringList>


#define NOERROR			 0
#define ERROR_APPRUNNING	-1
#define ERROR_BADCMDLINEARGS	-2


class CmdLine
{
public:
	static void printHelp();
	static bool validateOptionDependencies(QStringList cmdline_args);
	static bool parseForOption(QStringList *cmdline_args, QString alias1, QString alias2, QStringList *values);
	static void printWarning(QString msg);
	static void printError(QString msg);

private:
	static const QString helpText;
	static void printToCerr(QString level, QString msg);
};

#endif




