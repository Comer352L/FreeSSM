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
#include "CUcontent_DCs_twoMemories.h"


TransmissionDialog::TransmissionDialog(AbstractDiagInterface *diagInterface, QString language, bool prefereSSM2protocolVariantISO14230)
	: ControlUnitDialog(controlUnitName(), diagInterface, language, prefereSSM2protocolVariantISO14230)
{
	// Add information widget:
	setInfoWidget( new CUinfo_Transmission() );
	// Add content:
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


CUtype TransmissionDialog::controlUnitType()
{
	return CUtype::Transmission;
}


bool TransmissionDialog::systemRequiresManualON()
{
	return false;
}


CUcontent_DCs_abstract * TransmissionDialog::allocate_DCsContentWidget()
{
	return new CUcontent_DCs_twoMemories();
}


bool TransmissionDialog::displayExtendedCUinfo(SSMprotocol *SSMPdev, CUinfo_abstract *abstractInfoWidget, FSSM_ProgressDialog*)
{
	bool supported = false;
	std::vector<mb_dt> supportedMBs;
	std::vector<sw_dt> supportedSWs;
	if (SSMPdev == NULL)
		return false;
	CUinfo_Transmission *infoWidget = dynamic_cast<CUinfo_Transmission*>(abstractInfoWidget);
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
	return true;
}

