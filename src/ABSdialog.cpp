/*
 * ABSdialog.cpp - ABS/VDC Control Unit dialog
 *
 * Copyright (C) 2012 L1800Turbo, 2008-2019 Comer352L
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

 /*
  * TODO:
  *  FreezeFrame
  */

#include "ABSdialog.h"
#include "CUcontent_DCs_twoMemories.h"


ABSdialog::ABSdialog(AbstractDiagInterface *diagInterface, QString language) : ControlUnitDialog(controlUnitName(), diagInterface, language)
{
	// Show information-widget:
	_infoWidget = new CUinfo_simple();
	setInfoWidget(_infoWidget);
	_infoWidget->show();
	// Setup functions:
	addContent(ContentSelection::DCsMode);
	addContent(ContentSelection::MBsSWsMode);
	addContent(ContentSelection::ClearMemoryFcn);
}


QString ABSdialog::systemName()
{
	return tr("ABS/VDC");
}


QString ABSdialog::controlUnitName()
{
	return tr("ABS/VDC Control Unit");
}


SSMprotocol::CUtype_dt ABSdialog::controlUnitType()
{
	return SSMprotocol::CUtype_ABS;
}


bool ABSdialog::systemRequiresManualON()
{
	return false;
}


CUcontent_DCs_abstract * ABSdialog::allocate_DCsContentWidget()
{
	return new CUcontent_DCs_twoMemories();
}


void ABSdialog::displaySystemDescriptionAndID(QString description, QString ID)
{
	_infoWidget->setSystemTypeText(description);
	_infoWidget->setRomIDText(ID);
}


bool ABSdialog::fillInfoWidget(FSSM_InitStatusMsgBox*)
{
	std::vector<mb_dt> supportedMBs;
	std::vector<sw_dt> supportedSWs;
	// Number of supported MBs / SWs:
	if ((!_SSMPdev->getSupportedMBs(&supportedMBs)) || (!_SSMPdev->getSupportedSWs(&supportedSWs)))
		return false;	// commError
	_infoWidget->setNrOfSupportedMBsSWs(supportedMBs.size(), supportedSWs.size());
	return true;
}

