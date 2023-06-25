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
#include "CUcontent_DCs_engine.h"


EngineDialog::EngineDialog(AbstractDiagInterface *diagInterface, QString language, bool prefereSSM2protocolVariantISO14230)
	: ControlUnitDialog(controlUnitName(), diagInterface, language, prefereSSM2protocolVariantISO14230)
{
	// Add information widget:
	setInfoWidget( new CUinfo_Engine() );
	// Add content:
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


CUtype EngineDialog::controlUnitType()
{
	return CUtype::Engine;
}


bool EngineDialog::systemRequiresManualON()
{
	return false;
}


CUcontent_DCs_abstract * EngineDialog::allocate_DCsContentWidget()
{
	return new CUcontent_DCs_engine();
}


bool EngineDialog::displayExtendedCUinfo(SSMprotocol *SSMPdev, CUinfo_abstract *abstractInfoWidget, FSSM_ProgressDialog *initstatusmsgbox)
{
	QString VIN = "";
	bool supported = false;
	std::vector<mb_dt> supportedMBs;
	std::vector<sw_dt> supportedSWs;
	if (SSMPdev == NULL)
		return false;
	CUinfo_Engine *infoWidget = dynamic_cast<CUinfo_Engine*>(abstractInfoWidget);
	if (infoWidget == NULL)
		return true; // NOTE: no communication error
	// Number of supported MBs / SWs:
	if ((!SSMPdev->getSupportedMBs(&supportedMBs)) || (!SSMPdev->getSupportedSWs(&supportedSWs)))
		return false;	// commError
	infoWidget->setNrOfSupportedMBsSWs(supportedMBs.size(), supportedSWs.size());
	// OBD2-Support:
	if (!SSMPdev->hasOBD2system(&supported))
		return false;	// commError
	infoWidget->setOBD2Supported(supported);
	// Integrated Cruise Control:
	if (!SSMPdev->hasIntegratedCC(&supported))
		return false;	// commError
	infoWidget->setIntegratedCCSupported(supported);
	// Immobilizer:
	if (!SSMPdev->hasImmobilizer(&supported))
		return false;	// commError
	infoWidget->setImmobilizerSupported(supported);
	// Update status info message box:
	if (initstatusmsgbox != NULL)
	{
		initstatusmsgbox->setLabelText(tr("Reading Vehicle Ident. Number... Please wait !"));
		initstatusmsgbox->setValue(55);
	}
	// Query and output VIN, if supported:
	if (!SSMPdev->hasVINsupport(&supported))
		return false;	// commError
	if (supported)
	{
		if (SSMPdev->ifceProtocolType() != AbstractDiagInterface::protocol_type::SSM2_ISO15765)
		{
			if (!SSMPdev->getVIN(&VIN))
				return false;	// commError
		}
		else // FIXME: at least some ISO-15765 ECUs fail to read the VIN
			VIN = " "; // avoid reporting as "not programmed yet"
	}
	infoWidget->setVINinfo(supported, VIN);
	return true;
}
