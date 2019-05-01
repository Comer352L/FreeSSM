/*
 * Transmission.h - Transmission Control Unit dialog
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

#ifndef TRANSMISSIONDIALOG_H
#define TRANSMISSIONDIALOG_H


#ifdef __WIN32__
	#include "windows\serialCOM.h"
#elif defined __linux__
	#include "linux/serialCOM.h"
#else
	#error "Operating system not supported !"
#endif
#include <QtGui>
#include "ControlUnitDialog.h"
#include "CUinfo_Transmission.h"
#include "FSSMdialogs.h"
#include "AbstractDiagInterface.h"
#include "SSMprotocol.h"



class TransmissionDialog : public ControlUnitDialog
{
	Q_OBJECT

public:
	TransmissionDialog(AbstractDiagInterface *diagInterface, QString language);
	bool setup(ContentSelection csel = ContentSelection::DCsMode, QStringList cmdline_args = QStringList());

private:
	CUinfo_Transmission *_infoWidget;

	QString systemName();
	QString controlUnitName();
	SSMprotocol::CUtype_dt controlUnitType();
	bool systemRequiresManualON();
	CUcontent_DCs_abstract * allocate_DCsContentWidget();
	bool fillInfoWidget(FSSM_InitStatusMsgBox *initstatusmsgbox);

};



#endif
