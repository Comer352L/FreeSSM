/*
 * Engine.h - Engine Control Unit dialog
 *
 * Copyright (C) 2008-2009 Comer352l
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

#ifndef ENGINE_H
#define ENGINE_H


#ifdef __WIN32__
    #include "windows\serialCOM.h"
#elif defined __linux__
    #include "linux/serialCOM.h"
#else
    #error "Operating system not supported !"
#endif
#include <QtGui>
#include "ControlUnitDialog.h"
#include "CUinfo_Engine.h"
#include "CUcontent_DCs_engine.h"
#include "CUcontent_MBsSWs.h"
#include "CUcontent_Adjustments.h"
#include "CUcontent_sysTests.h"
#include "ClearMemoryDlg.h"
#include "FSSMdialogs.h"
#include "SSMprotocol1.h"
#include "SSMprotocol2.h"



class Engine : public ControlUnitDialog
{
	Q_OBJECT

public:
	Engine(serialCOM *port, QString language);

private:
	enum mode_dt {DCs_mode=1, MBsSWs_mode=2, Adaptions_mode=3, SysTests_mode};

	// Content backup parameters:
	std::vector<MBSWmetadata_dt> _lastMBSWmetaList;
	MBSWsettings_dt _MBSWsettings;
	// Info- and content-widgets:
	CUinfo_Engine *_infoWidget;
	CUcontent_DCs_engine *_content_DCs;
	CUcontent_Adjustments *_content_Adjustments;
	// Current content/mode:
	mode_dt _mode;

	void setup();

private slots:
	void DCs();
	void measuringblocks();
	void adjustments();
	void systemoperationtests();
	void clearMemory();

};



#endif
