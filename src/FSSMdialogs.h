/*
 * FSSMdialogs.h - Dialogs and Messagboxes for FreeSSM
 *
 * Copyright (C) 2008-2014 Comer352L
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

#ifndef FSSMDIALOGS_H
#define FSSMDIALOGS_H


#include <QtGlobal>	/* required for QT_VERSION */
#if QT_VERSION < 0x050000
	#include <QtGui>
#else
	#include <QtWidgets>
#endif


class FSSM_ProgressDialog : public QProgressDialog
{
public:
	FSSM_ProgressDialog(const QString & labelText, const QString & cancelButtonText, int minimum, int maximum, QWidget * parent = 0, Qt::WindowFlags f = Qt::Widget);
	~FSSM_ProgressDialog();
	void show();
	void hide();
	bool close();

private:
	QWidget *_parent;
	bool _allow_close;
	void closeEvent(QCloseEvent *event);
};



class FSSM_WaitMsgBox : public QMessageBox
{
	Q_OBJECT
public:
	FSSM_WaitMsgBox(QWidget *parent, const QString text, const QString title = tr("Please wait..."));
	~FSSM_WaitMsgBox();
	void show();
	void hide();
	bool close();

private:
	QWidget *_parent;
	bool _allow_close;
	void closeEvent(QCloseEvent *event);

};


#endif
