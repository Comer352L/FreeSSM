/*
 * ClearMemoryDlg.cpp - Provides dialogs and runs the Clear Memory procedure(s)
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

#include "ClearMemoryDlg.h"



ClearMemoryDlg::ClearMemoryDlg(QMainWindow *parent, SSMprotocol *SSMPdev, SSMprotocol::CMlevel_dt level)
{
	_parent = parent;
	_SSMPdev = SSMPdev;
	_level = level;
}


ClearMemoryDlg::CMresult_dt ClearMemoryDlg::run()
{
	bool ok = false;
	bool CMsuccess = false;
	QString SYS_ID_old;
	QString ROM_ID_old;
	SSMprotocol::state_dt CUstate_old;
	int oldDCgroups = 0;
	MBSWmetadata_dt oldMBSWmetaList[SSMP_MAX_MBSW];
	ClearMemoryDlg::CMresult_dt reconnect_result;
	bool tm = false;
	bool enginerunning = false;

	for (int k=0; k<SSMP_MAX_MBSW; k++)
	{
		oldMBSWmetaList[k].blockType = 0;
		oldMBSWmetaList[k].nativeIndex = 0;
	}
	unsigned int oldMBSWmetaList_len = 0;
	// Let the user confirm the Clear Memory procedure:
	if (!confirmClearMemory())
		return ClearMemoryDlg::CMresult_aborted;
	// Wait-messagebox:
	QString waitstr = tr("Clearing memory");
	if (_level)
		waitstr += tr(" (level 2)");
	waitstr += tr("... Please wait !   ");
	FSSM_WaitMsgBox waitmsgbox(_parent, waitstr);
	waitmsgbox.show();
	// Save CU-state, prepare for Clear Memory:
	CUstate_old = _SSMPdev->state();
	if (CUstate_old == SSMprotocol::state_DCreading)
	{
		if (!_SSMPdev->getLastDCgroupsSelection(&oldDCgroups))
			return ClearMemoryDlg::CMresult_communicationError;
		if (!_SSMPdev->stopDCreading())
			return ClearMemoryDlg::CMresult_communicationError;
	}
	else if (CUstate_old == SSMprotocol::state_MBSWreading)
	{
		if (!_SSMPdev->getLastMBSWselection(oldMBSWmetaList, &oldMBSWmetaList_len))
			return ClearMemoryDlg::CMresult_communicationError;
		if (!_SSMPdev->stopMBSWreading())
			return ClearMemoryDlg::CMresult_communicationError;
	}
	// NOTE: it's currently not possible to call this function while actuator-test is in progress, so we don't need to care about running actuator-tests
	if (!_SSMPdev->getSysID(&SYS_ID_old))
		return ClearMemoryDlg::CMresult_communicationError;
	if (!_SSMPdev->getROMID(&ROM_ID_old))
		return ClearMemoryDlg::CMresult_communicationError;
	// Clear Memory:
	ok = _SSMPdev->ClearMemory(_level, &CMsuccess);
	if (!ok || !CMsuccess)
		return ClearMemoryDlg::CMresult_communicationError;
	// Request user to switch ignition off and wait for communication error:
	waitmsgbox.setText(tr("Please switch ignition OFF and be patient...   "));
	ok = _SSMPdev->waitForIgnitionOff();
	// Close wait-message box:
	waitmsgbox.close();
	if (!ok)
		return ClearMemoryDlg::CMresult_communicationError;
	// Request user to switch ignition on and ensure that CU is still the same:
	reconnect_result = reconnect(SYS_ID_old, ROM_ID_old);
	if (reconnect_result != ClearMemoryDlg::CMresult_success)
		return reconnect_result;
	// Stop all actuators if CU is in tes-mode:
	if (!_SSMPdev->hasTestMode(&tm))
		return ClearMemoryDlg::CMresult_communicationError;
	if (tm)
	{
		// Query test mode connector status:
		if (!_SSMPdev->isInTestMode(&tm))
			return ClearMemoryDlg::CMresult_communicationError;
		if (tm)
		{
			// Check that engine is not running:
			if (!_SSMPdev->isEngineRunning(&enginerunning))
				return ClearMemoryDlg::CMresult_communicationError;
			if (!enginerunning)
			{
				// Stop all actuator tests:
				if (!_SSMPdev->stopAllActuators())
					return ClearMemoryDlg::CMresult_communicationError;
			}
		}
	}
	// Restore last CU-state:
	if (CUstate_old == SSMprotocol::state_DCreading)
	{
		if (!_SSMPdev->startDCreading(oldDCgroups))
			return ClearMemoryDlg::CMresult_communicationError;
	}
	else if (CUstate_old == SSMprotocol::state_MBSWreading)
	{
		if (!_SSMPdev->startMBSWreading(oldMBSWmetaList, oldMBSWmetaList_len))
			return ClearMemoryDlg::CMresult_communicationError;
	}
	// Return success:
	return ClearMemoryDlg::CMresult_success;
}



bool ClearMemoryDlg::confirmClearMemory()
{
	int uc = 0;
	// Create dialog
	QString winTitle = tr("Clear Memory");
	QString confirmStr = tr("Do you really want to clear the\nControl Unit's memory");
	if (_level)
	{
		winTitle.append( " 2" );
		confirmStr.append( tr(" (level 2)") );
	}
	confirmStr.append( " ?" );
	// Show dialog:
	QMessageBox ccmmsg( QMessageBox::Warning, winTitle, confirmStr, QMessageBox::NoButton, _parent);
	ccmmsg.addButton(tr("OK"), QMessageBox::AcceptRole);
	ccmmsg.addButton(tr("Cancel"), QMessageBox::RejectRole);
	QFont ccmmsgfont = ccmmsg.font();
	ccmmsgfont.setPixelSize(12);	// 9pts
	ccmmsg.setFont( ccmmsgfont );
	ccmmsg.show();
	// Wait for user choice:
	uc = ccmmsg.exec();
	// Close dialog and return result:
	ccmmsg.close();
	if (uc == QMessageBox::RejectRole)
		return false;
	else
		return true;
}


ClearMemoryDlg::CMresult_dt ClearMemoryDlg::reconnect(QString SYS_ID_old, QString ROM_ID_old)
{
	QString ID_new;
	int uc = 0;
	bool ok = false;
	bool equal = false;
	// Messagebox: Tell user to switch ignition on (or leave CU):
	QString winTitle = tr("Clear Memory");
	if (_level)
		winTitle.append( " 2" );
	QMessageBox ignonmsgbox( QMessageBox::Information, winTitle, tr("Please switch ignition ON again."), QMessageBox::NoButton, _parent);
	ignonmsgbox.addButton(tr("Continue"), QMessageBox::AcceptRole);
	ignonmsgbox.addButton(tr(" Leave Control Unit "), QMessageBox::RejectRole);
	QFont ignonmsgfont = ignonmsgbox.font();
	ignonmsgfont.setPixelSize(12);	// 9pts
	ignonmsgbox.setFont( ignonmsgfont );
	// Wait-messagebox:
	FSSM_WaitMsgBox waitmsgbox(_parent, tr("Reconnecting... Please wait !   "));
	while (!ok)
	{
		ignonmsgbox.show();
		uc = ignonmsgbox.exec();
		ignonmsgbox.hide();
		if (uc == QMessageBox::RejectRole)
			return ClearMemoryDlg::CMresult_reconnectAborted;
		// Validate CU idetification:
		waitmsgbox.show();
		ok = _SSMPdev->setupCUdata();
		if (ok)
		{
			ok = _SSMPdev->getSysID(&ID_new);
			if (ok)
			{
				equal = (ID_new == SYS_ID_old);
				if (equal)
				{
					ok = _SSMPdev->getROMID(&ID_new);
					if (ok)
						equal = (ID_new == ROM_ID_old);
				}
				// Check for communication error:
				if (!ok)
					return ClearMemoryDlg::CMresult_communicationError;
				// Check if CU is still the same:
				if (!equal)
					return ClearMemoryDlg::CMresult_reconnectFailed;	// Leave CU without error message (for now)
			}
		}
		waitmsgbox.hide();
	}
	// Return success:
	return ClearMemoryDlg::CMresult_success;
}


