/*
 * main.cpp - Main file of the FreeSSM software
 *
 * Copyright (C) 2008-2018 Comer352L
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


#define FSSM_VERSION "(devel)"


#include <QtGui>
#include "FreeSSM.h"


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	// Ensure that only one instance of FreeSSM is started:
	QSharedMemory fssm_lock("FreeSSM_smo-key_1234567890ABCDEFGHIJKLMNOPQRSTUVW");
	if ( fssm_lock.attach() )
	{
		/* NOTE: With the current implementation of QSharedMemory, shared memory objects
		 * survive on UNIX-systems if the attached process is killed (or crashed) !    */
		fssm_lock.detach();	// trick: this also detaches all dead processes (why ?!) !
		if ( fssm_lock.attach() ) // another instance of FreeSSM is still running
			return -1;
	}
	fssm_lock.create(1, QSharedMemory::ReadOnly);
	// Set application version:
	app.setApplicationVersion(FSSM_VERSION);
	// Set window icon:
	app.setWindowIcon( QIcon(":/icons/freessm/32x32/FreeSSM.png") );
#if QT_VERSION < 0x050000
	// Use Unicode UTF-8:
	QTextCodec::setCodecForCStrings( QTextCodec::codecForName("UTF-8") );
#endif
	// Get installed (system) fonts:
	QFontDatabase fdb;
	QStringList fontfamilies = fdb.families ( QFontDatabase::Any );
	// Check if font "Liberation Sans" is installed:
	if (!fontfamilies.contains("Liberation Sans"))
	{
		// Load font "Liberation Sans" (no installation !)
		QString AppsPath( QCoreApplication::applicationDirPath() );
		QFontDatabase::addApplicationFont (AppsPath + "/LiberationSans-Regular.ttf");
		QFontDatabase::addApplicationFont (AppsPath + "/LiberationSans-Bold.ttf");
		QFontDatabase::addApplicationFont (AppsPath + "/LiberationSans-Italic.ttf");
		QFontDatabase::addApplicationFont (AppsPath + "/LiberationSans-BoldItalic.ttf");
	}
	// Select and use font:
	QFont appfont = app.font();
	appfont.setFamily("Liberation Sans");
	app.setFont( appfont );
	// Open main window:
	FreeSSM *freessm_mainwindow = new FreeSSM(&app);
	freessm_mainwindow->show();
	// Wait until main window is closed:
	int ret = app.exec();
	delete freessm_mainwindow;
	return ret;
}
