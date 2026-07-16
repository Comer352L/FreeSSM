#include "SSMprotocol.h"

#include <QTimer>

namespace
{
QString formatByteValue(char value)
{
	const unsigned int byte = static_cast<unsigned char>(value);
	return QString("0x%1 (%2)").arg(byte, 2, 16, QChar('0')).arg(byte).toUpper();
}
}

SSM3protocolTPMS::SSM3protocolTPMS(AbstractDiagInterface *diagInterface, QString language)
	: SSMprotocol3(diagInterface, language)
{
}

SSMprotocol::CUsetupResult_dt SSM3protocolTPMS::setupCUdata(enum CUtype CU)
{
	if ((CU != CUtype::TPMS) || (_diagInterface == NULL))
		return result_invalidCUtype;

	const AbstractDiagInterface::protocol_type protocol = _diagInterface->protocolType();
	if (protocol != AbstractDiagInterface::protocol_type::SSM3_ISO14230)
		return result_invalidInterfaceConfig;

	stopKeepAlive();
	if (!startDiagnosticSession())
		return result_commError;

	std::vector<char> identifyPayload;
	identifyPayload.push_back('\x1A');
	identifyPayload.push_back('\x91');
	std::vector<char> commonIdentifyResponse;
	if (!sendRequest(identifyPayload, 0x5A, &commonIdentifyResponse) ||
	    (commonIdentifyResponse.size() < 5) ||
	    (static_cast<unsigned char>(commonIdentifyResponse.at(1)) != 0x91) ||
	    (static_cast<unsigned char>(commonIdentifyResponse.at(2)) != 0xCC) ||
	    (static_cast<unsigned char>(commonIdentifyResponse.at(3)) != 0x02) ||
	    (static_cast<unsigned char>(commonIdentifyResponse.at(4)) != 0x00))
		return result_commError;

	identifyPayload.clear();
	identifyPayload.push_back('\x1A');
	identifyPayload.push_back('\x9A');
	std::vector<char> identifyResponse;
	if (!sendRequest(identifyPayload, 0x5A, &identifyResponse) ||
	    (identifyResponse.size() < 6) ||
	    (static_cast<unsigned char>(identifyResponse.at(1)) != 0x9A))
		return result_commError;
	// TPMS 0x1A/0x91 is the common family ID; 0x1A/0x9A is module-specific.
	_ssmCUdata.SYS_ID.assign(identifyResponse.begin() + 2, identifyResponse.begin() + 5);
	_ssmCUdata.ROM_ID.assign(identifyResponse.begin() + 2, identifyResponse.begin() + 6);

	_CU = CU;
	_ifceProtocol = protocol;
	_sysDescription = "Tire Pressure Monitoring System";
	_supportedDCgroups = currentDTCs_DCgroup;
	_selectedDCgroups = noDCs_DCgroup;
	_state = state_normal;
	startKeepAlive();
	return result_success;
}

std::string SSM3protocolTPMS::getROMID() const
{
	if (_state == state_needSetup)
		return "";
	return libFSSM::StrToHexstr(_ssmCUdata.ROM_ID);
}

bool SSM3protocolTPMS::hasClearMemory(bool *CMsup)
{
	if ((_state == state_needSetup) || (CMsup == NULL))
		return false;
	*CMsup = true;
	return true;
}

bool SSM3protocolTPMS::startDCreading(int DCgroups)
{
	if ((_state != state_normal) || !(DCgroups & currentDTCs_DCgroup))
		return false;

	_selectedDCgroups = currentDTCs_DCgroup;
	_state = state_DCreading;
	stopKeepAlive();
	QTimer::singleShot(0, this, SLOT(readDiagnosticCodes()));
	return true;
}

bool SSM3protocolTPMS::restartDCreading()
{
	if (_state != state_DCreading)
		return false;

	_state = state_normal;
	return startDCreading(_selectedDCgroups);
}

bool SSM3protocolTPMS::stopDCreading()
{
	if (_state != state_DCreading)
		return false;

	_selectedDCgroups = noDCs_DCgroup;
	_state = state_normal;
	startKeepAlive();
	emit stoppedDCreading();
	return true;
}

bool SSM3protocolTPMS::clearMemory(CMlevel_dt level, bool *success)
{
	if (success != NULL)
		*success = false;
	if ((_state != state_normal) || (level != CMlevel_1))
		return false;

	stopKeepAlive();
	std::vector<char> payload;
	payload.push_back('\x14');
	payload.push_back('\xFE');
	std::vector<char> response;
	if (!sendRequest(payload, 0x54, &response) ||
	    (response.size() < 2) ||
	    (static_cast<unsigned char>(response.at(1)) != 0xFE))
	{
		if (!startDiagnosticSession() ||
		    !sendRequest(payload, 0x54, &response) ||
		    (response.size() < 2) ||
		    (static_cast<unsigned char>(response.at(1)) != 0xFE))
		{
			startKeepAlive();
			return false;
		}
	}

	if (success != NULL)
		*success = true;
	startKeepAlive();
	return true;
}


bool SSM3protocolTPMS::hasLocalIdentifierData(bool *LIsup)
{
	if ((_state == state_needSetup) || (LIsup == NULL))
		return false;
	*LIsup = true;
	return true;
}


bool SSM3protocolTPMS::readLocalIdentifierData(std::vector<local_identifier_section_dt> *sections)
{
	if ((_state != state_normal) || (sections == NULL))
		return false;
	sections->clear();

	QStringList registeredIDs;
	QStringList runtimeIDs;
	if (readTransmitterIDs(&registeredIDs, &runtimeIDs))
	{
		local_identifier_section_dt registeredSection;
		registeredSection.title = tr("Registered Transmit IDs:");
		registeredSection.columnHeaders << tr("Tire") << tr("Transmit ID");
		for (int i = 0; i < 4; i++)
			registeredSection.rows.push_back(QStringList() << QString::number(i + 1) << ((i < registeredIDs.size()) ? registeredIDs.at(i) : ""));
		sections->push_back(registeredSection);

		local_identifier_section_dt runtimeSection;
		runtimeSection.title = tr("Runtime / Reception History IDs:");
		runtimeSection.columnHeaders << tr("Slot") << tr("Transmit ID");
		for (int i = 0; i < 4; i++)
			runtimeSection.rows.push_back(QStringList() << QString::number(i + 1) << ((i < runtimeIDs.size()) ? runtimeIDs.at(i) : ""));
		sections->push_back(runtimeSection);
	}
	else
	{
		local_identifier_section_dt transmitSection;
		transmitSection.title = tr("Transmit IDs:");
		transmitSection.rawValue = tr("not supported or no response");
		sections->push_back(transmitSection);
	}

	std::vector<char> block10;
	if (!readLocalIdentifier(0x10, 10, &block10))
		return !sections->empty();

	local_identifier_section_dt block10Section;
	block10Section.title = tr("0x21 / 0x10 Live/status block (10 bytes):");
	block10Section.rawValue = QString::fromStdString(libFSSM::StrToHexstr(block10));
	block10Section.columnHeaders << tr("Byte") << tr("Value");
	for (std::size_t i = 0; i < block10.size(); i++)
		block10Section.rows.push_back(QStringList() << tr("Byte %1").arg(static_cast<unsigned int>(i)) << formatByteValue(block10.at(i)));
	sections->push_back(block10Section);

	std::vector<char> block99;
	local_identifier_section_dt block99Section;
	block99Section.title = tr("0x21 / 0x99 Live/status block (8 bytes):");
	block99Section.columnHeaders << tr("Byte") << tr("Value");
	if (readLocalIdentifier(0x99, 8, &block99))
	{
		block99Section.rawValue = QString::fromStdString(libFSSM::StrToHexstr(block99));
		for (std::size_t i = 0; i < block99.size(); i++)
			block99Section.rows.push_back(QStringList() << tr("Byte %1").arg(static_cast<unsigned int>(i)) << formatByteValue(block99.at(i)));
	}
	else
	{
		block99Section.rawValue = tr("not supported or no response");
	}
	sections->push_back(block99Section);

	return !sections->empty();
}


bool SSM3protocolTPMS::readTransmitterIDs(QStringList *registeredIDs, QStringList *runtimeIDs)
{
	if ((_state != state_normal) || (registeredIDs == NULL))
		return false;
	registeredIDs->clear();
	if (runtimeIDs != NULL)
		runtimeIDs->clear();

	std::vector<char> payload;
	payload.push_back('\x21');
	payload.push_back('\x11');
	std::vector<char> response;
	if (!sendRequest(payload, 0x61, &response) ||
	    (response.size() < 14) ||
	    (static_cast<unsigned char>(response.at(1)) != 0x11))
	{
		if (!startDiagnosticSession() ||
		    !sendRequest(payload, 0x61, &response) ||
		    (response.size() < 14) ||
		    (static_cast<unsigned char>(response.at(1)) != 0x11))
			return false;
	}

	for (unsigned int i = 0; i < 4; i++)
	{
		const std::size_t offset = 2 + (i * 3);
		registeredIDs->append(QString::fromStdString(libFSSM::StrToHexstr(&response.at(offset), 3)));
	}
	if (runtimeIDs != NULL)
	{
		for (unsigned int i = 0; i < 4; i++)
		{
			const std::size_t offset = 2 + ((i + 4) * 3);
			if ((offset + 3) <= response.size())
				runtimeIDs->append(QString::fromStdString(libFSSM::StrToHexstr(&response.at(offset), 3)));
			else
				runtimeIDs->append("");
		}
	}
	return true;
}


bool SSM3protocolTPMS::readLiveDataBlocks(QStringList *block10Values, QStringList *block99Values,
                                         QString *block10Raw, QString *block99Raw)
{
	if ((_state != state_normal) || (block10Values == NULL) || (block99Values == NULL))
		return false;
	block10Values->clear();
	block99Values->clear();
	if (block10Raw != NULL)
		block10Raw->clear();
	if (block99Raw != NULL)
		block99Raw->clear();

	std::vector<char> block10;
	std::vector<char> block99;
	if (!readLocalIdentifier(0x10, 10, &block10))
		return false;
	// Calsonic supports 0x21/0x10 and 0x21/0x11, but not ALPS' read-only
	// 0x21/0x99 block. Leave the second table blank when it is unavailable.
	readLocalIdentifier(0x99, 8, &block99);

	for (std::size_t i = 0; i < block10.size(); i++)
		block10Values->append(formatByteValue(block10.at(i)));
	for (std::size_t i = 0; i < block99.size(); i++)
		block99Values->append(formatByteValue(block99.at(i)));
	if (block10Raw != NULL)
		*block10Raw = QString::fromStdString(libFSSM::StrToHexstr(block10));
	if (block99Raw != NULL)
	{
		if (block99.empty())
			*block99Raw = "not supported or no response";
		else
			*block99Raw = QString::fromStdString(libFSSM::StrToHexstr(block99));
	}
	return true;
}


void SSM3protocolTPMS::readDiagnosticCodes()
{
	if (_state != state_DCreading)
		return;

	std::vector<char> payload;
	payload.push_back('\x17');
	payload.push_back('\xFE');
	std::vector<char> response;
	if (!sendRequest(payload, 0x57, &response) || (response.size() < 2))
	{
		if (!startDiagnosticSession() || !sendRequest(payload, 0x57, &response) || (response.size() < 2))
		{
			_selectedDCgroups = noDCs_DCgroup;
			_state = state_normal;
			startKeepAlive();
			emit commError();
			return;
		}
	}

	const unsigned int codeCount = static_cast<unsigned char>(response.at(1));
	if (response.size() < (2 + codeCount))
	{
		_selectedDCgroups = noDCs_DCgroup;
		_state = state_normal;
		emit commError();
		return;
	}

	QStringList codes;
	QStringList descriptions;
	for (unsigned int i = 0; i < codeCount; i++)
	{
		const unsigned int code = static_cast<unsigned char>(response.at(2 + i));
		codes << QString("0x%1").arg(code, 2, 16, QChar('0')).toUpper();
		descriptions << diagnosticCodeDescription(code);
	}
	emit currentOrTemporaryDTCs(codes, descriptions, false, false);
	if (_state == state_DCreading)
		QTimer::singleShot(2000, this, SLOT(readDiagnosticCodes()));
}


QString SSM3protocolTPMS::diagnosticCodeDescription(unsigned int code) const
{
	const unsigned int transmitter = code & 0x0F;
	switch (code & 0xF0)
	{
		case 0x10:
			if ((transmitter >= 1) && (transmitter <= 4))
				return tr("Tire pressure %1 is reduced").arg(transmitter);
			break;
		case 0x20:
			if ((transmitter >= 1) && (transmitter <= 4))
				return tr("Data cannot be received from transmitter %1").arg(transmitter);
			break;
		case 0x30:
			if ((transmitter >= 1) && (transmitter <= 4))
				return tr("Transmitter %1 pressure data is abnormal").arg(transmitter);
			break;
		case 0x40:
			if ((transmitter >= 1) && (transmitter <= 4))
				return tr("Transmitter %1 function code is abnormal").arg(transmitter);
			break;
		case 0x50:
			if ((transmitter >= 1) && (transmitter <= 4))
				return tr("Transmitter %1 battery voltage is low").arg(transmitter);
			break;
		case 0x60:
			if (code == 0x61)
				return tr("Vehicle speed signal is abnormal");
			break;
	}
	return tr("Unknown TPMS diagnostic code 0x%1").arg(code, 2, 16, QChar('0')).toUpper();
}
