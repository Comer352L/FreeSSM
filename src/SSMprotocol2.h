/*
 * SSMprotocol2.h - Application Layer for the new Subaru SSM protocol
 *
 * Copyright (C) 2008-2023 Comer352L
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

#ifndef SSMPROTOCOL2_H
#define SSMPROTOCOL2_H



#include <QString>
#include <vector>
#include "AbstractDiagInterface.h"
#include "SSMCUdata.h"
#include "SSMprotocol.h"
#include "SSMP2communication.h"
#include "SSMFlagbyteDefinitionsInterface.h"



class SSMprotocol2 : public SSMprotocol
{
	Q_OBJECT

public:
	SSMprotocol2(AbstractDiagInterface *diagInterface, QString language="en");
	~SSMprotocol2();
	// NON-COMMUNICATION-FUNCTIONS:
	CUsetupResult_dt setupCUdata(enum CUtype CU);
	CUsetupResult_dt setupCUdata(enum CUtype CU, bool ignoreIgnitionOFF=false);
	protocol_dt protocolType() { return SSM2; }
	bool hasVINsupport(bool *VINsup);
	bool hasIntegratedCC(bool *CCsup);
	bool hasClearMemory2(bool *CM2sup);
	// COMMUNICATION BASED FUNCTIONS:
	bool getVIN(QString *VIN);

private:
	// *** CONTROL UNIT BASIC DATA (SUPPORTED FEATURES) ***:
	bool _has_integratedCC;
	bool _has_VINsupport;

	bool validateVIN(char VIN[17]);

public slots:
	void resetCUdata();

};



#endif
