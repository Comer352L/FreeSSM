/*
 * About.cpp - Display informations about the FreeSSM software
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


#include "About.h"


About::About(QWidget *parent, QString progversion,  QString language) : QDialog(parent)
{
	// Setup UI:
	setupUi(this);
	setupUiFonts();
	// Output title/program version:
	progversion_label->setText(progversion_label->text() + " " + progversion);
	// Load licence text and changelog:
	QFile changelog_file;
	if (language == "de")
	{
		licence_textBrowser->setSource(QUrl("qrc:/licence_de.html"));
		changelog_file.setFileName(":/changelog_de.txt");
	}
	else
	{
		licence_textBrowser->setSource(QUrl("qrc:/licence_en.html"));
		changelog_file.setFileName(":/changelog_en.txt");
	}
	changelog_file.open(QIODevice::ReadOnly | QIODevice::Text);
	QString changelog_content = static_cast<QString>(changelog_file.readAll());
	changelog_textBrowser->setText(changelog_content);
	changelog_file.close();
	// *** Definitions:
	SSMprotocol_def_en ssmp_defs;
	// Output number of supported DTCs:
	int nrofDTCs_SUB = ssmp_defs.SUBDTCrawDefs().size();
	int nrofDTCs_OBD = ssmp_defs.OBDDTCrawDefs().size();
	int nrofDTCs_CC = ssmp_defs.CCCCrawDefs().size();
	QString dtcstr = QString::number( nrofDTCs_SUB ) + " / " + QString::number( nrofDTCs_OBD ) + " / " + QString::number( nrofDTCs_CC );
	nrofsupportedDTCs_label->setText( dtcstr );
	// Output number of supported measuring blocks / switches:
	int nrofMBs = ssmp_defs.MBrawDefs().size();
	int nrofSWs = ssmp_defs.SWrawDefs().size();
	QString mbswstr = QString::number( nrofMBs ) + " / " + QString::number( nrofSWs );
	nrofsupportedMBsSWs_label->setText( mbswstr );
	//Output number of supported Adjustment values:
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
	// Output number of supported system tests:
	int nrofSysTests = ssmp_defs.ActuatorRawDefs().size();
	QString systestsstr = QString::number( nrofSysTests ) + " / 1";
	nrofActuatortests_label->setText(systestsstr);
	// Connect "Close"-button:
	connect( close_pushButton, SIGNAL( released() ), this, SLOT( close() ) ); 
}


void About::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS OF ui_About.h (made with QDesigner)
	QFont appfont = QApplication::font();
	QFont font = this->font();
	font.setFamily(appfont.family());
	font.setPixelSize(12);	// 9pts
	this->setFont(font);
	font = progversion_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(27);	// 20pts
	progversion_label->setFont(font);
	font = subtitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	subtitle_label->setFont(font);
	font = main_tabWidget->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	main_tabWidget->setFont(font);
	font = aboutcontent_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	aboutcontent_label->setFont(font);
	font = modelstitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	modelstitle_label->setFont(font);
	font = models_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	models_label->setFont(font);
	font = controlunitstitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	controlunitstitle_label->setFont(font);
	font = controlunits_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	controlunits_label->setFont(font);
	font = dtcstitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	dtcstitle_label->setFont(font);
	font = nrofsupportedDTCs_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	nrofsupportedDTCs_label->setFont(font);
	font = dtcsinfo_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	dtcsinfo_label->setFont(font);
	font = mbstitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	mbstitle_label->setFont(font);
	font = nrofsupportedMBsSWs_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	nrofsupportedMBsSWs_label->setFont(font);
	font = mbsswsinfo_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	mbsswsinfo_label->setFont(font);
	font = adjustmentstitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	adjustmentstitle_label->setFont(font);
	font = nrofadjustmentvalues_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	nrofadjustmentvalues_label->setFont(font);
	font = adjustmentsinfo_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	adjustmentsinfo_label->setFont(font);
	font = systemteststitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	systemteststitle_label->setFont(font);
	font = nrofActuatortests_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	nrofActuatortests_label->setFont(font);
	font = systemtestsinfo_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	systemtestsinfo_label->setFont(font);
	font = languagestitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	languagestitle_label->setFont(font);
	font = languages_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	languages_label->setFont(font);
	font = supportedOStitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	supportedOStitle_label->setFont(font);
	font = supportedOS_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	supportedOS_label->setFont(font);
	font = infoobd2_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	infoobd2_label->setFont(font);
	font = author_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	author_label->setFont(font);
	font = email_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	email_label->setFont(font);
	font = homepagetitle_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	homepagetitle_label->setFont(font);
	font = homepageURL_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	homepageURL_label->setFont(font);
	font = gplinfo_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	gplinfo_label->setFont(font);
	font = licence_textBrowser->font();
	font.setFamily(appfont.family());
	font.setPixelSize(16);	// 12pts
	licence_textBrowser->setFont(font);
	font = changelog_textBrowser->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	changelog_textBrowser->setFont(font);
	font = close_pushButton->font();
	font.setFamily(appfont.family());
	font.setPixelSize(13);	// 10pts
	close_pushButton->setFont(font);
	font = trademarksInfo_label->font();
	font.setFamily(appfont.family());
	font.setPixelSize(11);	// 8pts
	trademarksInfo_label->setFont(font);
}


