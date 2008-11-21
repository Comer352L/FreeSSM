/*
 * main.cpp - Main file of the FreeSSM software
 *
 * Copyright © 2008 Comer352l
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

#include <QApplication> 
#include "FreeSSM.h"



int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	// Ensure that only one instance of FreeSSM is started:
	QSharedMemory fssm_lock("FreeSSM_smo-key_1234567890ABCDEFGHIJKLMNOPQRSTUVW");
	bool ok = fssm_lock.create(1, QSharedMemory::ReadOnly);
	if (!ok) return -1;
	// Set Window Icon:
	app.setWindowIcon( QIcon(":/icons/FreeSSM.png") );
	// Use Unicode UTF-8:
	QTextCodec::setCodecForCStrings( QTextCodec::codecForName("UTF-8") );
	// Get installed (system) fonts:
	QFontDatabase fdb;
	QStringList fontfamilies = fdb.families ( QFontDatabase::Any );
	// Select font:
	QFont appfont = app.font();
	if (fontfamilies.contains("Arial"))
		appfont.setFamily("Arial");
	else if (fontfamilies.contains("Albany AMT"))
		appfont.setFamily("Albany AMT");
	else
	{
		// Check if font "Liberation Sans" is installed:
		if (!fontfamilies.contains("Liberation Sans"))
		{
			// Load font "Liberation Sans" (no installation !)
			QString AppsPath( QCoreApplication::applicationDirPath() );
			QFontDatabase::addApplicationFont (AppsPath + "LiberationSans-Regular.ttf");
			QFontDatabase::addApplicationFont (AppsPath + "LiberationSans-Bold.ttf");
			QFontDatabase::addApplicationFont (AppsPath + "LiberationSans-Italic.ttf");
			QFontDatabase::addApplicationFont (AppsPath + "LiberationSans-BoldItalic.ttf");
		}
		appfont.setFamily("Liberation Sans");
	}
	// Use selected font:
	app.setFont( appfont );
	// Open main window:
	FreeSSM *freessm_mainwindow = new FreeSSM(&app);
	freessm_mainwindow->show();
	// Wait until main window is closed:
	int ret = app.exec();
	delete freessm_mainwindow;
	return ret;
}
