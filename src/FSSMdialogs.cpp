/*
 * FSSMdialogs.cpp - Dialogs and Messagboxes for FreeSSM
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

#include "FSSMdialogs.h"


FSSM_ProgressDialog::FSSM_ProgressDialog(const QString & labelText, const QString & cancelButtonText,
                                             int minimum, int maximum, QWidget * parent, Qt::WindowFlags f)
                                              : QProgressDialog(labelText, cancelButtonText, minimum, maximum, parent, f)
{
	_allow_close = false;
	_parent = parent;
	setAutoClose( false );
	setAutoReset( false );
	setModal( true );
	QFont newfont = font();
	newfont.setPointSize(10);
	newfont.setBold( true );
	setFont( newfont );
}


FSSM_ProgressDialog::~FSSM_ProgressDialog()
{
	_parent->setCursor(Qt::ArrowCursor);
	setCursor(Qt::ArrowCursor);
}


void FSSM_ProgressDialog::show()
{
	QProgressDialog::show();
	_parent->setCursor(Qt::WaitCursor);
	setCursor(Qt::WaitCursor);
}


void FSSM_ProgressDialog::hide()
{
	_parent->setCursor(Qt::ArrowCursor);
	setCursor(Qt::ArrowCursor);
	QProgressDialog::hide();
}


bool FSSM_ProgressDialog::close()
{
	_allow_close = true;
	return QProgressDialog::close();
}


void FSSM_ProgressDialog::closeEvent(QCloseEvent *event)
{
	if (_allow_close)
		event->accept();
	else
		event->ignore();
}







FSSM_WaitMsgBox::FSSM_WaitMsgBox(QWidget *parent, const QString text, const QString title)
                                  : QMessageBox(QMessageBox::NoIcon, title, text + "   ", QMessageBox::NoButton, parent)
{
	_allow_close = false;
	_parent = parent;
	// Set buttons (Constructor parameter seems to be ignored. Qt-bug ???)
	setStandardButtons( QMessageBox::NoButton );
	// Set font
	QFont newfont = font();
	newfont.setPointSize(10);
	newfont.setBold( true );
	setFont( newfont );
	// Set icon:
	setIconPixmap( QPixmap( QString::fromUtf8(":/icons/oxygen/22x22/view-history.png") ) );
}


FSSM_WaitMsgBox::~FSSM_WaitMsgBox()
{
	_parent->setCursor(Qt::ArrowCursor);
	setCursor(Qt::ArrowCursor);
}


void FSSM_WaitMsgBox::show()
{
	QMessageBox::show();
	_parent->setCursor(Qt::WaitCursor);
	setCursor(Qt::WaitCursor);
}


void FSSM_WaitMsgBox::hide()
{
	_parent->setCursor(Qt::ArrowCursor);
	setCursor(Qt::ArrowCursor);
	QMessageBox::hide();
}


bool FSSM_WaitMsgBox::close()
{
	_allow_close = true;
	return QMessageBox::close();
}


void FSSM_WaitMsgBox::closeEvent(QCloseEvent *event)
{
	if (_allow_close)
		event->accept();
	else
		event->ignore();
}
