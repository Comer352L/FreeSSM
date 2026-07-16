/*
 * TPMSdialog.cpp - TPMS Control Unit dialog
 *
 * Copyright (C) 2012 L1800Turbo, 2008-2019 Comer352L, 2024 jimihimi
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

#include "TPMSdialog.h"
#include "CUcontent_DCs_stopCodes.h"


TPMSdialog::TPMSdialog(AbstractDiagInterface *diagInterface, QString language) : ControlUnitDialog(controlUnitName(), diagInterface, language)
{
	// Add information widget:
	setInfoWidget( new CUinfo_simple() );
	// Add content:
	addContent(ContentSelection::DCsMode);
	addContent(ContentSelection::LocalIdentifiersMode);
	addContent(ContentSelection::ClearMemoryFcn);
}


QString TPMSdialog::systemName()
{
	return tr("TPMS");
}


QString TPMSdialog::controlUnitName()
{
	return tr("TPMS Control Unit");
}


CUtype TPMSdialog::controlUnitType()
{
	return CUtype::TPMS;
}


bool TPMSdialog::systemRequiresManualON()
{
	return false;
}


CUcontent_DCs_abstract * TPMSdialog::allocate_DCsContentWidget()
{
	return new CUcontent_DCs_stopCodes();
}


bool TPMSdialog::displayExtendedCUinfo(SSMprotocol *SSMPdev, CUinfo_abstract *abstractInfoWidget, FSSM_ProgressDialog*)
{
	std::vector<mb_dt> supportedMBs;
	std::vector<sw_dt> supportedSWs;
	if (SSMPdev == NULL)
		return false;
	CUinfo_simple *infoWidget = dynamic_cast<CUinfo_simple*>(abstractInfoWidget);
	if (infoWidget == NULL)
		return true; // NOTE: no communication error
	// Number of supported MBs / SWs:
	if ((!SSMPdev->getSupportedMBs(&supportedMBs)) || (!SSMPdev->getSupportedSWs(&supportedSWs)))
		return false;	// commError
	infoWidget->setNrOfSupportedMBsSWs(supportedMBs.size(), supportedSWs.size());
	return true;
}
