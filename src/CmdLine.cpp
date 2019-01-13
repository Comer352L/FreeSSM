/*
 * CmdLine.cpp - Command Line processing
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

#include "CmdLine.h"
#include <QCoreApplication>
#include <iostream>


const QString CmdLine::helpText =
"Usage:\n\
    FreeSSM [-c | --controlunit <CONTROLUNIT> [-f | --function <FUNCTION> [-p | --parameters <PARAMETERS>]]] | [-h | --help]\n\
\n\
Options:\n\
    -c, --controlunit <CONTROLUNIT>\n\
        Select control unit dialog.\n\
        Supported values for <CONTROLUNIT> are:\n\
        	engine\n\
        	transmission\n\
        	absvdc\n\
        	cruisectrl\n\
        	aircon\n\
\n\
    -f, --function <FUNCTION>\n\
        Select control unit function.\n\
        Can only be specified if a control unit is selected, too.\n\
        Supported values for <FUNCTION> depend on the selected control unit:\n\
        	dcs\n\
        	mbssws\n\
        	adjustments	(only engine and transmission control units)\n\
        	systests	(only engine control units)\n\
        	clearmemory\n\
        	clearmemory2	(only transmission control units)\n\
\n\
    -p, --parameters <PARAMETERS>\n\
        Select parameters to be used for the selected control unit function.\n\
        Can only be specified if a control unit and a function is selected, too.\n\
        Supported values for <PARAMETERS> depend on the selected control unit and function:\n\
        	selectionfile=<FILENAME>	(only mbssws function)\n\
        	autostart			(only mbssws function; requires selectionfile=<filename>, too)\n\
        Multiple parameters can be specified.\n\
\n\
    -h, --help\n\
        Show this help.\n\
\n \
Example:\n\
    FreeSSM -c transmission -f mbssws -p selectionfile=\"/home/users/myMBSWselection_4.list\" autostart\n\
\n\
        Starts FreeSSM and the transmission control unit dialog in MB/SW reading mode,\n\
        loads a MB/SW selection from the file /home/users/myMBSWselection_4.list and\n\
        starts reading the values from the control unit.\n";


void CmdLine::printHelp()
{
	std::cout << helpText.toStdString() << std::endl;
}


bool CmdLine::validateOptionDependencies(QStringList cmdline_args)
{
	bool error = false;

	bool opt_param = parseForOption(&cmdline_args, "-p", "--parameters", NULL);
	bool opt_fcn = parseForOption(&cmdline_args, "-f", "--function", NULL);
	bool opt_cu = parseForOption(&cmdline_args, "-c", "--controlunit", NULL);

	if (!opt_cu && (opt_fcn || opt_param))
	{
		error = true;
		QString errstr = "";
		if (opt_fcn)
			errstr += "option -f/--function";
		if (opt_param)
		{
			if (errstr.size())
				errstr += " and ";
			errstr += "option -p/--parameters";
		}
		printError("option -c/--controlunit required for " + errstr);
	}
	if (!opt_fcn && opt_param)
	{
		error = true;
		printError("option -f/--function required for option -p/--parameters");
	}

	return !error;
}


bool CmdLine::parseForOption(QStringList *cmdline_args, QString alias1, QString alias2, QStringList *values)
{
	QStringList args = *cmdline_args;
	QStringList opt_values;
	QString opt_descr;
	int i=0;
	int opt_count = 0;

	if (values != NULL)
		values->clear();
	if (cmdline_args == NULL)
		return false;
	opt_descr = alias1;
	if (alias1.size() && alias2.size())
		opt_descr += '/';
	opt_descr += alias2;
	// Validate number of occurences of the option
	if (alias1.size())
		opt_count = args.count(alias1);
	if (alias2.size() && (alias2 != alias1))
		opt_count += args.count(alias2);
	if (opt_count < 1)
		return false;
	if (opt_count > 1)
		printWarning("option " + opt_descr + " specified " + QString::number(opt_count) + " times - merging values");
	// Extract option values:
	while (i < args.size())
	{
		if ((alias1.size() && (args.at(i) == alias1)) ||
		    (alias2.size() && (args.at(i) == alias2))   )
		{
			args.removeAt(i);
			while ((i < args.size()) && (!args.at(i).startsWith('-')))
			{
				if (opt_values.contains(args.at(i)))
					printError("option " + opt_descr + " specified with duplicate value " + args.at(i) + " - eliminating");
				else
					opt_values.append(args.at(i));
				args.removeAt(i);
			}
		}
		else
			i++;	
	}

	*cmdline_args = args;
	if (values != NULL)
		*values = opt_values;
	return opt_count;
}


void CmdLine::printToCerr(QString level, QString msg)
{
	std::cerr << QCoreApplication::applicationName().toStdString() << ": " << level.toStdString() << ": " << msg.toStdString() << std::endl;
}


void CmdLine::printWarning(QString msg)
{
	printToCerr("warning", msg);
}


void CmdLine::printError(QString msg)
{
	printToCerr("error", msg);
}



