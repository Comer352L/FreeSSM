/*
 * Engine.cpp - Engine Control Unit dialog
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

#include "EngineDialog.h"
#include "CmdLine.h"
#include "CUcontent_DCs_engine.h"


EngineDialog::EngineDialog(AbstractDiagInterface *diagInterface, QString language) : ControlUnitDialog(controlUnitName(), diagInterface, language)
{
	// Show information-widget:
	_infoWidget = new CUinfo_Engine();
	setInfoWidget(_infoWidget);
	_infoWidget->show();
	// Setup functions:
	addContent(ContentSelection::DCsMode);
	addContent(ContentSelection::MBsSWsMode);
	addContent(ContentSelection::AdjustmentsMode);
	addContent(ContentSelection::SysTestsMode);
	addContent(ContentSelection::ClearMemoryFcn);
}


QString EngineDialog::systemName()
{
	return tr("Engine");
}


QString EngineDialog::controlUnitName()
{
	return tr("Engine Control Unit");
}


SSMprotocol::CUtype_dt EngineDialog::controlUnitType()
{
	return SSMprotocol::CUtype_Engine;
}


bool EngineDialog::systemRequiresManualON()
{
	return false;
}


CUcontent_DCs_abstract * EngineDialog::allocate_DCsContentWidget()
{
	return new CUcontent_DCs_engine();
}


void EngineDialog::displaySystemDescriptionAndID(QString description, QString ID)
{
	_infoWidget->setEngineTypeText(description);
	_infoWidget->setRomIDText(ID);
}


bool EngineDialog::fillInfoWidget(FSSM_InitStatusMsgBox *initstatusmsgbox)
{
	QString VIN = "";
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
	// Integrated Cruise Control:
	if (!_SSMPdev->hasIntegratedCC(&supported))
		return false;	// commError
	_infoWidget->setIntegratedCCSupported(supported);
	// Immobilizer:
	if (!_SSMPdev->hasImmobilizer(&supported))
		return false;	// commError
	_infoWidget->setImmobilizerSupported(supported);
	// Update status info message box:
	if (initstatusmsgbox != NULL)
	{
		initstatusmsgbox->setLabelText(tr("Reading Vehicle Ident. Number... Please wait !"));
		initstatusmsgbox->setValue(55);
	}
	// Query and output VIN, if supported:
	if (!_SSMPdev->hasVINsupport(&supported))
		return false;	// commError
	if (supported)
	{
		if (!_SSMPdev->getVIN(&VIN))
			return false;	// commError
	}
	_infoWidget->setVINinfo(supported, VIN);
	return true;
}

