/*
 * CruiseControlDialog.cpp - Cruise Control Unit dialog
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

#include "CruiseControlDialog.h"
#include "CUcontent_DCs_stopCodes.h"


CruiseControlDialog::CruiseControlDialog(AbstractDiagInterface *diagInterface, QString language) : ControlUnitDialog(controlUnitName(), diagInterface, language)
{
	// Show information-widget:
	_infoWidget = new CUinfo_simple();
	setInfoWidget(_infoWidget);
	_infoWidget->show();
	// Setup functions:
	addContent(ContentSelection::DCsMode);
	addContent(ContentSelection::MBsSWsMode);
}


QString CruiseControlDialog::systemName()
{
	return tr("Cruise Control");
}


QString CruiseControlDialog::controlUnitName()
{
	return tr("Cruise Control Control Unit");
}


SSMprotocol::CUtype_dt CruiseControlDialog::controlUnitType()
{
	return SSMprotocol::CUtype_CruiseControl;
}


bool CruiseControlDialog::systemRequiresManualON()
{
	return true;
}


CUcontent_DCs_abstract * CruiseControlDialog::allocate_DCsContentWidget()
{
	return new CUcontent_DCs_stopCodes();
}


void CruiseControlDialog::displaySystemDescriptionAndID(QString description, QString ID)
{
	_infoWidget->setSystemTypeText(description);
	_infoWidget->setRomIDText(ID);
}


bool CruiseControlDialog::fillInfoWidget(FSSM_InitStatusMsgBox*)
{
	std::vector<mb_dt> supportedMBs;
	std::vector<sw_dt> supportedSWs;
	// Number of supported MBs / SWs:
	if ((!_SSMPdev->getSupportedMBs(&supportedMBs)) || (!_SSMPdev->getSupportedSWs(&supportedSWs)))
		return false;	// commError
	_infoWidget->setNrOfSupportedMBsSWs(supportedMBs.size(), supportedSWs.size());
	return true;
}

