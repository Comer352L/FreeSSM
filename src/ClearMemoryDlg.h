/*
 * ClearMemoryDlg.h - Provides dialogs and runs the Clear Memory procedure(s)
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

#ifndef CLEARMEMORYDLG_H
#define CLEARMEMORYDLG_H


#include <QtGui>
#include <string>
#include "SSMCUdata.h"
#include "SSMprotocol.h"
#include "FSSMdialogs.h"


class ClearMemoryDlg : public QObject
{
	Q_OBJECT

public:
	enum CMresult_dt {CMresult_aborted, CMresult_communicationError, CMresult_success, CMresult_adjValRestorationFailed, CMresult_reconnectAborted, CMresult_reconnectFailed};

	ClearMemoryDlg(QDialog *parent, SSMprotocol *SSMPdev, SSMprotocol::CMlevel_dt level);
	CMresult_dt run();

private:
	QDialog *_parent;
	SSMprotocol *_SSMPdev;
	SSMprotocol::CMlevel_dt _level;

	class StoppedOperation_dt
	{
	public:
		StoppedOperation_dt();
		SSMprotocol::state_dt state;
		int DCgroups;
		std::vector<MBSWmetadata_dt> MBSWmetaList;
	};

	bool confirmClearMemory(CUtype cu_type);
	bool confirmAdjustmentValuesRestoration();
	CMresult_dt stopCurrentOperation(StoppedOperation_dt *operation);
	CMresult_dt restoreStoppedOperation(const StoppedOperation_dt& operation);
	CMresult_dt runDirectClearMemory(const StoppedOperation_dt& operation, FSSM_WaitMsgBox *waitmsgbox);
	CMresult_dt runIgnitionCycleClearMemory(CUtype cu_old, const StoppedOperation_dt& operation, FSSM_WaitMsgBox *waitmsgbox);
	CMresult_dt restoreAdjustmentValues(std::vector<unsigned int> oldAdjVal);
	CMresult_dt reconnect(CUtype cu, std::string SYS_ID_old, std::string ROM_ID_old);

};


#endif

