/*
 * ClearMemoryDlg.cpp - Provides dialogs and runs the Clear Memory procedure(s)
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

#include "ClearMemoryDlg.h"



ClearMemoryDlg::ClearMemoryDlg(QMainWindow *parent, SSMprotocol2 *SSMP2dev, SSMprotocol2::CMlevel_dt level)
{
	_parent = parent;
	_SSMP2dev = SSMP2dev;
	_level = level;
}


ClearMemoryDlg::CMresult_dt ClearMemoryDlg::run()
{
	CMresult_dt result = CMresult_success;
	bool ok = false;
	bool CMsuccess = false;
	std::string SYS_ID_old;
	std::string ROM_ID_old;
	SSMprotocol2::state_dt CUstate_old;
	int oldDCgroups = 0;
	std::vector<MBSWmetadata_dt> oldMBSWmetaList;
	std::vector<unsigned int> oldAdjVal;
	bool tm = false;
	bool enginerunning = false;
	std::vector<adjustment_dt> supAdj;
	QEventLoop el;

	// Let the user confirm the Clear Memory procedure:
	if (!confirmClearMemory())
		return ClearMemoryDlg::CMresult_aborted;
	// Wait-messagebox:
	QString waitstr = tr("Clearing memory");
	if (_level == SSMprotocol2::CMlevel_2)
		waitstr += tr(" (level 2)");
	waitstr += tr("... Please wait !   ");
	FSSM_WaitMsgBox waitmsgbox(_parent, waitstr);
	waitmsgbox.show();
	// Save CU-state, prepare for Clear Memory:
	CUstate_old = _SSMP2dev->state();
	if (CUstate_old == SSMprotocol2::state_DCreading)
	{
		if (!_SSMP2dev->getLastDCgroupsSelection(&oldDCgroups))
			return ClearMemoryDlg::CMresult_communicationError;
		if (!_SSMP2dev->stopDCreading())
			return ClearMemoryDlg::CMresult_communicationError;
	}
	else if (CUstate_old == SSMprotocol2::state_MBSWreading)
	{
		if (!_SSMP2dev->getLastMBSWselection(&oldMBSWmetaList))
			return ClearMemoryDlg::CMresult_communicationError;
		if (!_SSMP2dev->stopMBSWreading())
			return ClearMemoryDlg::CMresult_communicationError;
	}
	// NOTE: it's currently not possible to call this function while actuator-test is in progress, so we don't need to care about running actuator-tests
	SYS_ID_old = _SSMP2dev->getSysID();
	if (!SYS_ID_old.length())
		return ClearMemoryDlg::CMresult_communicationError;
	ROM_ID_old = _SSMP2dev->getROMID();
	if (!ROM_ID_old.length())
		return ClearMemoryDlg::CMresult_communicationError;
	if (!_SSMP2dev->getAllAdjustmentValues(&oldAdjVal))
		return ClearMemoryDlg::CMresult_communicationError;
	// Clear Memory:
	ok = _SSMP2dev->ClearMemory(_level, &CMsuccess);
	if (!ok || !CMsuccess)
		return ClearMemoryDlg::CMresult_communicationError;
	QTimer::singleShot(800, &el, SLOT( quit() ));
	el.exec();
	// Request user to switch ignition off and wait for communication error:
	waitmsgbox.setText(tr("Please switch ignition OFF and be patient...   "));
	ok = _SSMP2dev->waitForIgnitionOff();
	// Wait 5 seconds
	QTimer::singleShot(5000, &el, SLOT( quit() ));
	el.exec();
	// Close wait-message box:
	waitmsgbox.close();
	if (!ok)
		return ClearMemoryDlg::CMresult_communicationError;
	// Request user to switch ignition on and ensure that CU is still the same:
	result = reconnect(SYS_ID_old, ROM_ID_old);
	if (result != ClearMemoryDlg::CMresult_success)
		return result;
	// Stop all actuators if CU is in test-mode:
	if (!_SSMP2dev->hasTestMode(&tm))
		return ClearMemoryDlg::CMresult_communicationError;
	if (tm)
	{
		// Query test mode connector status:
		if (!_SSMP2dev->isInTestMode(&tm))
			return ClearMemoryDlg::CMresult_communicationError;
		if (tm)
		{
			// Check that engine is not running:
			if (!_SSMP2dev->isEngineRunning(&enginerunning))
				return ClearMemoryDlg::CMresult_communicationError;
			if (!enginerunning)
			{
				// Stop all actuator tests:
				if (!_SSMP2dev->stopAllActuators())
					return ClearMemoryDlg::CMresult_communicationError;
			}
		}
	}
	// Check if it makes sense to restore the adjustment values:
	if (!_SSMP2dev->getSupportedAdjustments( &supAdj ))
		return ClearMemoryDlg::CMresult_communicationError;
	ok = false;
	for (unsigned char k=0; k<supAdj.size(); k++)
	{
		// Check if old value was valid
		if ((supAdj.at(k).rawMin <= supAdj.at(k).rawMax) && ((oldAdjVal.at(k) < supAdj.at(k).rawMin) || ((oldAdjVal.at(k) > supAdj.at(k).rawMax))))
		{
			ok = false;
			break;
		}
		if ((supAdj.at(k).rawMin > supAdj.at(k).rawMax) && (oldAdjVal.at(k) < supAdj.at(k).rawMin) && (oldAdjVal.at(k) > supAdj.at(k).rawMax))
		{
			ok = false;
			break;
		}
		// Check if we have custom adjustment values:
		if (supAdj.at(k).rawDefault != oldAdjVal.at(k))
			ok = true;
	}
	// Restore adjustment values:
	if (ok)
	{
		// Let the user confirm the restoration:
		if (confirmAdjustmentValuesRestoration())
		{
			result = restoreAdjustmentValues(oldAdjVal);
			if ( result == CMresult_communicationError )
				return ClearMemoryDlg::CMresult_communicationError;
		}
	}
	// Restore last CU-state:
	if (CUstate_old == SSMprotocol2::state_DCreading)
	{
		if (!_SSMP2dev->startDCreading(oldDCgroups))
			return ClearMemoryDlg::CMresult_communicationError;
	}
	else if (CUstate_old == SSMprotocol2::state_MBSWreading)
	{
		if (!_SSMP2dev->startMBSWreading(oldMBSWmetaList))
			return ClearMemoryDlg::CMresult_communicationError;
	}
	// Return result:
	return result;
}


bool ClearMemoryDlg::confirmClearMemory()
{
	int uc = 0;
	// Create dialog
	QString winTitle = tr("Clear Memory");
	QString confirmStr = tr("The Clear Memory procedure");
	if (_level == SSMprotocol2::CMlevel_2)
		confirmStr.append( tr(" (level 2)") );
	confirmStr.append( tr("\n- clears the Diagnostic Codes") );
	confirmStr.append( tr("\n- resets all non-permanent Adjustment Values") );
	if ( _SSMP2dev->CUtype() == SSMprotocol2::CUtype_Engine || ((_SSMP2dev->CUtype() == SSMprotocol2::CUtype_Transmission) && (_level == SSMprotocol2::CMlevel_2)) )
		confirmStr.append( tr("\n- resets the Control Units' learning values") );
	confirmStr.append( tr("\n\nDo you really want to clear the Control Units' memory") );
	if (_level == SSMprotocol2::CMlevel_2)
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


bool ClearMemoryDlg::confirmAdjustmentValuesRestoration()
{
	int uc = 0;
	// Create dialog
	QString winTitle = tr("Restore Adjustment Values ?");
	QString confirmStr = tr("Shall the last Adjustment Values be restored ?");
	// Show dialog:
	QMessageBox ccmmsg( QMessageBox::Warning, winTitle, confirmStr, QMessageBox::NoButton, _parent);
	ccmmsg.addButton(tr("Restore"), QMessageBox::AcceptRole);
	ccmmsg.addButton(tr("Keep default values"), QMessageBox::RejectRole);
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


ClearMemoryDlg::CMresult_dt ClearMemoryDlg::restoreAdjustmentValues(std::vector<unsigned int> oldAdjVal)
{
	std::vector<unsigned int> currentAdjVal;
	bool ok = false;
	// Restore all adjustment values:
	FSSM_WaitMsgBox waitmsgbox(_parent, tr("Restoring Adjustment Values... Please wait !   "));
	waitmsgbox.show();
	for (unsigned char m=0; m<oldAdjVal.size(); m++)
	{
		if (!_SSMP2dev->setAdjustmentValue(m, oldAdjVal.at(m)))
		{
			waitmsgbox.close();
			return CMresult_communicationError;
			/* NOTE: we don't know if we really have a communication error... */
		}
	}
	// To be sure: read and verify value again
	ok = _SSMP2dev->getAllAdjustmentValues(&currentAdjVal);
	waitmsgbox.close();
	if (ok)
	{
		if (currentAdjVal != oldAdjVal)
		{
			QMessageBox msg( QMessageBox::Critical, tr("Error"), tr("Adjustment Value restoration failed:\nThe Control Unit didn't accept some of the values !\n\nPlease check current values !"), QMessageBox::Ok, _parent);
			QFont msgfont = msg.font();
			msgfont.setPixelSize(12); // 9pts
			msg.setFont( msgfont );
			msg.show();
			msg.exec();
			msg.close();
			return CMresult_adjValRestorationFailed;
		}
	}
	else
		return CMresult_communicationError;
	return CMresult_success;
}


ClearMemoryDlg::CMresult_dt ClearMemoryDlg::reconnect(std::string SYS_ID_old, std::string ROM_ID_old)
{
	std::string ID_new;
	int uc = 0;
	bool ok = false;
	bool equal = false;
	// Messagebox: Tell user to switch ignition on (or leave CU):
	QString winTitle = tr("Clear Memory");
	if (_level == SSMprotocol2::CMlevel_2)
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
		ok = _SSMP2dev->setupCUdata();
		if (ok)
		{
			ID_new = _SSMP2dev->getSysID();
			ok = ID_new.length();
			if (ok)
			{
				equal = (ID_new == SYS_ID_old);
				if (equal)
				{
					ID_new = _SSMP2dev->getROMID();
					ok = ID_new.length();
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


