/*
 * About.cpp - Display informations about the FreeSSM software
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


#include "About.h"


About::About(QWidget *parent, QString language) : QDialog(parent)
{
	_language = language;
	// Setup UI:
	setupUi(this);
#ifdef SMALL_RESOLUTION
	// https://bugreports.qt.io/browse/QTBUG-16034
	// Workaround for window not showing always fullscreen
	setWindowFlags( Qt::Window );
	this->showFullScreen();
#endif
	// Display title/program version:
	progversion_label->setText(progversion_label->text() + " " + QApplication::applicationVersion());
	// Load licence text and changelog:
	QFile changelog_file;
	if (language == "de")
		changelog_file.setFileName(":/changelog_de.txt");
	else
		changelog_file.setFileName(":/changelog_en.txt");
	changelog_file.open(QIODevice::ReadOnly | QIODevice::Text);
	QString changelog_content = static_cast<QString>(changelog_file.readAll());
	changelog_textBrowser->setText(changelog_content);
	changelog_file.close();
	// *** Definitions:
	SSMprotocol2_def_en ssmp_defs;
	// Display number of supported DTCs:
	int nrofDTCs_SUB = ssmp_defs.SUBDTCrawDefs().size();
	int nrofDTCs_OBD = ssmp_defs.OBDDTCrawDefs().size();
	int nrofDTCs_CC = ssmp_defs.CCCCrawDefs().size();
	QString dtcstr = QString::number( nrofDTCs_SUB ) + " / " + QString::number( nrofDTCs_OBD ) + " / " + QString::number( nrofDTCs_CC );
	nrofsupportedDTCs_label->setText( dtcstr );
	// Display number of supported measuring blocks / switches:
	int nrofMBs = ssmp_defs.MBrawDefs().size();
	int nrofSWs = ssmp_defs.SWrawDefs().size();
	QString mbswstr = QString::number( nrofMBs ) + " / " + QString::number( nrofSWs );
	nrofsupportedMBsSWs_label->setText( mbswstr );
	// Display number of supported Adjustment values:
	int ecu_adjustments = 0;
	int tcu_adjustments = 0;
	QStringList adjustmentdefs = ssmp_defs.AdjustmentRawDefs();
	for (int k=0; k< adjustmentdefs.size(); k++)
	{
		if (adjustmentdefs.at(k).section(';', 1, 1).toInt() == 0)
		{
			ecu_adjustments++;
		}
		else if (adjustmentdefs.at(k).section(';', 1, 1).toInt() == 1)
		{
			tcu_adjustments++;
		}
	}
	QString adjustmentsstr = QString::number( ecu_adjustments ) + " / " + QString::number( tcu_adjustments );
	nrofadjustmentvalues_label->setText( adjustmentsstr );
	// Display number of supported system tests:
	int nrofSysTests = ssmp_defs.ActuatorRawDefs().size();
	QString systestsstr = QString::number( nrofSysTests ) + " / 1";
	nrofActuatortests_label->setText(systestsstr);
	// Display supported program languages:
	QString langstr;
	for (int k=0; k<__supportedLocales.size(); k++)
	{
		QLocale locale = __supportedLocales.at(k);
		QString langname = QLocale::languageToString( locale.language() );
		QString langname_tr = QCoreApplication::translate( "Language", langname.toUtf8() );
		if (k > 0)
			langstr.append(", ");
		langstr.append(langname_tr);
	}
	languages_label->setText( langstr );
	// Connect buttons:
	connect( showlicense_pushButton, SIGNAL( released() ), this, SLOT( showLicense() ) );
	connect( close_pushButton, SIGNAL( released() ), this, SLOT( close() ) );
}


void About::showLicense()
{
	QTextBrowser *license_textBrowser = new QTextBrowser;
	license_textBrowser->setWindowTitle("FreeSSM license text");
	license_textBrowser->setAttribute(Qt::WA_DeleteOnClose, true);
	// Place window in the center of the screen and resize:
	license_textBrowser->resize(700, 500);
	QDesktopWidget desktop;
	int x = (desktop.width() - license_textBrowser->size().width()) / 2;
	int y = (desktop.height() - license_textBrowser->size().height()) / 2 - 50;
	license_textBrowser->move ( x, y );
	// Display license text:
	if (_language == "de")
		license_textBrowser->setSource(QUrl("qrc:/license_de.html"));
	else
		license_textBrowser->setSource(QUrl("qrc:/license_en.html"));
	license_textBrowser->show();
}

