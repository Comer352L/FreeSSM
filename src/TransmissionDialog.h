/*
 * Transmission.h - Transmission Control Unit dialog
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
#include "CUcontent_DCs_twoMemories.h"
#include "CUcontent_MBsSWs.h"
#include "CUcontent_Adjustments.h"
#include "ClearMemoryDlg.h"
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
	// Content backup parameters:
	std::vector<MBSWmetadata_dt> _lastMBSWmetaList;
	MBSWsettings_dt _MBSWsettings;
	// Info- and content-widgets:
	CUinfo_Transmission *_infoWidget;
	CUcontent_DCs_twoMemories *_content_DCs;
	CUcontent_MBsSWs *_content_MBsSWs;
	CUcontent_Adjustments *_content_Adjustments;

	bool startDCsMode();
	bool startMBsSWsMode();
	bool startAdjustmentsMode();
	void runClearMemory(SSMprotocol::CMlevel_dt level);
	void saveContentSettings();

private slots:
	void switchToDCsMode();
	void switchToMBsSWsMode();
	void switchToAdjustmentsMode();
	void clearMemory();
	void clearMemory2();

};



#endif
