/*
 * Transmission.cpp - Transmission Control Unit dialog
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

#include "TransmissionDialog.h"
#include "CmdLine.h"
#include "CUcontent_DCs_twoMemories.h"


TransmissionDialog::TransmissionDialog(AbstractDiagInterface *diagInterface, QString language) : ControlUnitDialog(controlUnitName(), diagInterface, language)
{
	// Show information-widget:
	_infoWidget = new CUinfo_Transmission();
	setInfoWidget(_infoWidget);
	_infoWidget->show();
	// Setup functions:
	addContent(ContentSelection::DCsMode);
	addContent(ContentSelection::MBsSWsMode);
	addContent(ContentSelection::AdjustmentsMode);
	addContent(ContentSelection::ClearMemoryFcn);
	addContent(ContentSelection::ClearMemory2Fcn);
}


QString TransmissionDialog::systemName()
{
	return tr("Transmission");
}


QString TransmissionDialog::controlUnitName()
{
	return tr("Transmission Control Unit");
}


SSMprotocol::CUtype_dt TransmissionDialog::controlUnitType()
{
	return SSMprotocol::CUtype_Transmission;
}


bool TransmissionDialog::systemRequiresManualON()
{
	return false;
}


CUcontent_DCs_abstract * TransmissionDialog::allocate_DCsContentWidget()
{
	return new CUcontent_DCs_twoMemories();
}


void TransmissionDialog::displaySystemDescriptionAndID(QString description, QString ID)
{
	_infoWidget->setTransmissionTypeText(description);
	_infoWidget->setRomIDText(ID);
}


bool TransmissionDialog::fillInfoWidget(FSSM_InitStatusMsgBox*)
{
	bool supported = false;
	std::vector<mb_dt> supportedMBs;
	std::vector<sw_dt> supportedSWs;
	// Number of supported MBs / SWs:
	if ((!_SSMPdev->getSupportedMBs(&supportedMBs)) || (!_SSMPdev->getSupportedSWs(&supportedSWs)))
		return false;	// commError
	_infoWidget->setNrOfSupportedMBsSWs(supportedMBs.size(), supportedSWs.size());
	// OBD2-Support:
	if (!_SSMPdev->hasOBD2system(&supported))
		return false;	// commError
	_infoWidget->setOBD2Supported(supported);
	return true;
}

