/*
 * SSMLegacyDefinitionsInterface.cpp - Interface to the SSM legacy definitions
 *
 * Copyright (C) 2009-2019 Comer352L
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

#include <SSMLegacyDefinitionsInterface.h>


SSMLegacyDefinitionsInterface::SSMLegacyDefinitionsInterface(std::string lang)
{
	_xmldoc = NULL;
	_defs_root_element = NULL;
	_datacommon_root_element = NULL;
	_defs_for_id_b1_element = NULL;
	_defs_for_id_b2_element = NULL;
	_defs_for_id_b3_element = NULL;
	_lang = lang;
	_id_set = false;
}


SSMLegacyDefinitionsInterface::~SSMLegacyDefinitionsInterface()
{
	if (_xmldoc != NULL)
	{
		_xmldoc->Clear();
		delete _xmldoc;
	}
}


bool SSMLegacyDefinitionsInterface::selectDefinitionsFile(std::string filename)
{
	std::vector<XMLElement*> elements;
	const XMLAttribute* pAttrib = NULL;
	XMLElement *root_element;
	bool restored = false;
	if (!filename.size())
		goto error;
	if (_xmldoc == NULL)
		_xmldoc = new XMLDocument();
	if (_xmldoc->LoadFile(filename.c_str()) != XML_SUCCESS)
	// NOTE: resets the current document content, opens the new file, reads the content and closes the file
	{
		// Try to reopen last document:
		if (!_filename.size() || (_xmldoc->LoadFile(_filename.c_str()) != XML_SUCCESS))
			goto error;
		restored = true;
	}
	else
		_filename = filename;
	// Find root element FSSM_SSM1_DEFINITIONS:
	root_element = _xmldoc->FirstChildElement("FSSM_SSM1_DEFINITIONS");
	if (!root_element)
		goto error;
	// Find and save version infos:
	_defs_version.clear();
	_defs_format_version.clear();
	pAttrib = root_element->FirstAttribute();
	while (pAttrib)
	{
		if (std::string(pAttrib->Name()) == "version")
		{
			if (_defs_version.empty())
				_defs_version = pAttrib->Value();
			else // attribute specified multiple times
				goto error;
		}
		else if (std::string(pAttrib->Name()) == "format_version")
		{
			if (_defs_format_version.empty())
				_defs_format_version = pAttrib->Value();
			else // attribute specified multiple times
				goto error;
		}
		pAttrib=pAttrib->Next();
	}
	if (_defs_version.empty() || _defs_format_version.empty())
		goto error;
	if (!checkDefsFormatVersion(_defs_format_version))
		goto error;
	// Find and save element DEFINITIONS:
	elements = getAllMatchingChildElements(root_element, "DEFINITIONS");
	if (elements.size() != 1)
		goto error;
	_defs_root_element = elements.at(0);
	// Find and save element DATA_COMMON:
	elements = getAllMatchingChildElements(root_element, "DATA_COMMON");
	if (elements.size() != 1)
		goto error;
	_datacommon_root_element = elements.at(0);
	// Find and save definition element for the current ID:
	if (_id_set)
		selectID(_ID);

	return !restored;

error:
	if (_xmldoc != NULL)
	{
		_xmldoc->Clear();
		delete _xmldoc;
	}
	_xmldoc = NULL;
	_defs_version.clear();
	_defs_format_version.clear();
	_defs_root_element = NULL;
	_datacommon_root_element = NULL;
	_defs_for_id_b1_element = NULL;
	_defs_for_id_b2_element = NULL;
	_defs_for_id_b3_element = NULL;
	_filename.clear();
	return false;
}


bool SSMLegacyDefinitionsInterface::getVersionInfos(std::string *defs_version, std::string *format_version)
{
	if (!_id_set)
		return false;
	*defs_version = _defs_version;
	*format_version = _defs_format_version;
	return true;
}


void SSMLegacyDefinitionsInterface::setLanguage(std::string lang)
{
	_lang = lang;
}


bool SSMLegacyDefinitionsInterface::selectID(const std::vector<char>& id)
{
	std::vector<XMLElement*> elements;
	std::vector<attributeCondition> attribConditions;
	attributeCondition attribCondition;

	_id_set = false;
	_defs_for_id_b1_element = NULL;
	_defs_for_id_b2_element = NULL;
	_defs_for_id_b3_element = NULL;
	if (!_defs_root_element)
		return false;
	attribCondition.name = "value";
	attribCondition.value = "0x" + libFSSM::StrToHexstr(&id.at(0), 1);
	attribCondition.condition = attributeCondition::equal;
	elements = getAllMatchingChildElements(_defs_root_element, "ID_BYTE1", std::vector<attributeCondition>(1, attribCondition));
	if (elements.size() == 1)
	{
		_defs_for_id_b1_element = elements.at(0);
		attribCondition.value = "0x" + libFSSM::StrToHexstr(&id.at(1), 1);
		elements = getAllMatchingChildElements(elements.at(0), "ID_BYTE2", std::vector<attributeCondition>(1, attribCondition));
		if (elements.size() == 1)
		{
			_defs_for_id_b2_element = elements.at(0);
			attribCondition.name = "value_start";
			attribCondition.value = "0x" + libFSSM::StrToHexstr(&id.at(2), 1);
			attribCondition.condition = attributeCondition::equalOrSmaller;
			attribConditions.push_back(attribCondition);
			attribCondition.name = "value_end";
			attribCondition.condition = attributeCondition::equalOrLarger;
			attribConditions.push_back(attribCondition);
			elements = getAllMatchingChildElements(elements.at(0), "ID_BYTE3", attribConditions);
			if (elements.size() == 1)
			{
				_defs_for_id_b3_element = elements.at(0);
				_ID = id;
				_id_set = true;
				return true;
			}
		}
	}
	return false;
}


bool SSMLegacyDefinitionsInterface::systemDescription(std::string *description)
{
	std::vector<XMLElement*> elements;

	if (!_id_set)
		return false;
	if (_defs_for_id_b3_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b3_element, "SYSTEMDESCRIPTION");
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_for_id_b2_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b2_element, "SYSTEMDESCRIPTION");
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_for_id_b1_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b1_element, "SYSTEMDESCRIPTION");
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_root_element)
		elements = getAllMatchingChildElements(_defs_root_element, "SYSTEMDESCRIPTION");
	if (elements.size() != 1)
		return false;
	const char *str = elements.at(0)->GetText();
	if (str == NULL)
		return false;
	*description = std::string(str);
	return true;
}


bool SSMLegacyDefinitionsInterface::model(std::string *name)
{
	std::vector<XMLElement*> elements;

	if (!_id_set)
		return false;
	if (_defs_for_id_b3_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b3_element, "MODEL");
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_for_id_b2_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b2_element, "MODEL");
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_for_id_b1_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b1_element, "MODEL");
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_root_element)
		elements = getAllMatchingChildElements(_defs_root_element, "MODEL");
	if (elements.size() != 1)
		return false;
	const char *str = elements.at(0)->GetText();
	if (str == NULL)
		return false;
	*name = std::string(str);
	return true;
}


bool SSMLegacyDefinitionsInterface::year(std::string *yearstr)
{
	std::vector<XMLElement*> elements;

	if (!_id_set)
		return false;
	if (_defs_for_id_b3_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b3_element, "YEAR");
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_for_id_b2_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b2_element, "YEAR");
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_for_id_b1_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b1_element, "YEAR");
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_root_element)
		elements = getAllMatchingChildElements(_defs_root_element, "YEAR");
	if (elements.size() != 1)
		return false;
	const char *str = elements.at(0)->GetText();
	if (str == NULL)
		return false;
	*yearstr = std::string(str);
	return true;
}


bool SSMLegacyDefinitionsInterface::clearMemoryData(unsigned int *address, char *value)
{
	std::vector<XMLElement*> elements;
	XMLElement *CM_element;
	XMLElement *addr_element;
	const char *str = NULL;

	if (!_id_set)
		return false;
	if (_defs_for_id_b3_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b3_element, "CLEARMEMORY");
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_for_id_b2_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b2_element, "CLEARMEMORY");
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_for_id_b1_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b1_element, "CLEARMEMORY");
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_root_element)
		elements = getAllMatchingChildElements(_defs_root_element, "CLEARMEMORY");
	if (elements.size() != 1)
		return false;
	CM_element = elements.at(0);
	elements = getAllMatchingChildElements(CM_element, "ADDRESS");
	if (elements.size() < 1)
		return false;
	addr_element = elements.at(0);
	// NOTE: multiple CM-addresses may be defined and vaild, but only one of them is needed
	elements = getAllMatchingChildElements(CM_element, "VALUE");
	if (elements.size() != 1)
		return false;
	str = addr_element->GetText();
	if (str == NULL)
		return false;
	unsigned long int addr = strtoul( str, NULL, 0 );
	str = elements.at(0)->GetText();
	if (str == NULL)
		return false;
	unsigned long int val = strtoul( str, NULL, 0 );
	if ((addr > 0xffff) || (val > 0xff))
		return false;
	*address = addr;
	*value = val;
	return true;
}


bool SSMLegacyDefinitionsInterface::diagnosticCodes(std::vector<dc_defs_dt> *dcs)
{
	std::vector<XMLElement*> DTCblock_elements;
	std::vector<XMLElement*> DTCblock_elements2;
	const char *str = NULL;

	if (!_id_set)
		return false;
	dcs->clear();
	if (_defs_root_element)
		DTCblock_elements = getAllMatchingChildElements(_defs_root_element, "DTCBLOCK");
	if (_defs_for_id_b1_element)
	{
		DTCblock_elements2 = getAllMatchingChildElements(_defs_for_id_b1_element, "DTCBLOCK");
		DTCblock_elements.insert(DTCblock_elements.end(), DTCblock_elements2.begin(), DTCblock_elements2.end());
	}
	if (_defs_for_id_b2_element)
	{
		DTCblock_elements2 = getAllMatchingChildElements(_defs_for_id_b2_element, "DTCBLOCK");
		DTCblock_elements.insert(DTCblock_elements.end(), DTCblock_elements2.begin(), DTCblock_elements2.end());
	}
	if (_defs_for_id_b3_element)
	{
		DTCblock_elements2 = getAllMatchingChildElements(_defs_for_id_b3_element, "DTCBLOCK");
		DTCblock_elements.insert(DTCblock_elements.end(), DTCblock_elements2.begin(), DTCblock_elements2.end());
	}
	for (unsigned int b=0; b<DTCblock_elements.size(); b++)
	{
		unsigned long int addr = MEMORY_ADDRESS_NONE;
		dc_defs_dt dtcblock;
		dtcblock.byteAddr_currentOrTempOrLatest = MEMORY_ADDRESS_NONE;
		dtcblock.byteAddr_historicOrMemorized = MEMORY_ADDRESS_NONE;
		std::vector<XMLElement*> tmp_elements;
		bool duplicate = false;
		// --- Get address(es) ---:
		/* NOTE: DTCs with the same address must be defined in the same DTCBLOCK */
		// Get address for current DTCs:
		attributeCondition attribCond;
		attribCond.name = "type";
		attribCond.value = "current";
		attribCond.condition = attributeCondition::equal;
		tmp_elements = getAllMatchingChildElements(DTCblock_elements.at(b), "ADDRESS", std::vector<attributeCondition>(1, attribCond));
		if (tmp_elements.size() == 1)
		{
			str = tmp_elements.at(0)->GetText();
			if (str == NULL)
				continue;
			addr = strtoul( str, NULL, 0 );
			if (addr > 0xffff)
				continue;
			// Search for duplicate block address:
			for (unsigned int d=0; d<dcs->size(); d++)
			{
				if ((addr == dcs->at(d).byteAddr_currentOrTempOrLatest) || (addr == dcs->at(d).byteAddr_historicOrMemorized))
				{
					duplicate = true;
					dcs->erase( dcs->begin() + d );
					break;
				}
			}
			if (duplicate)
				continue;
			dtcblock.byteAddr_currentOrTempOrLatest = addr;
		}
		// Get address for historic DTCs:
		attribCond.value = "historic";
		tmp_elements = getAllMatchingChildElements(DTCblock_elements.at(b), "ADDRESS", std::vector<attributeCondition>(1, attribCond));
		if (tmp_elements.size() == 1)
		{
			str = tmp_elements.at(0)->GetText();
			if (str == NULL)
				continue;
			addr = strtoul( str, NULL, 0 );
			if (addr > 0xffff)
				continue;
			// Search for duplicate block address:
			for (unsigned int d=0; d<dcs->size(); d++)
			{
				if ((addr == dcs->at(d).byteAddr_currentOrTempOrLatest) || (addr == dcs->at(d).byteAddr_historicOrMemorized))
				{
					duplicate = true;
					dcs->erase( dcs->begin() + d );
					break;
				}
			}
			if (duplicate)
				continue;
			dtcblock.byteAddr_historicOrMemorized = addr;
		}
		if ((dtcblock.byteAddr_currentOrTempOrLatest == MEMORY_ADDRESS_NONE) && (dtcblock.byteAddr_historicOrMemorized == MEMORY_ADDRESS_NONE))
			continue;
		for (unsigned char k=0; k<8; k++)
		{
			dtcblock.code[k] = "???";
			dtcblock.title[k] = "     ???     (0x";
			if (dtcblock.byteAddr_currentOrTempOrLatest != MEMORY_ADDRESS_NONE)
				dtcblock.title[k] += QString::number(dtcblock.byteAddr_currentOrTempOrLatest,16).toUpper();
			if (dtcblock.byteAddr_historicOrMemorized != MEMORY_ADDRESS_NONE)
			{
				if (dtcblock.byteAddr_currentOrTempOrLatest != MEMORY_ADDRESS_NONE)
					dtcblock.title[k] += "/0x";
				dtcblock.title[k] += QString::number(dtcblock.byteAddr_historicOrMemorized,16).toUpper();
			}
			dtcblock.title[k] += " Bit " + QString::number(k+1) + ")";
			/* NOTE: see comments at the end of the function */
		}
		std::vector<XMLElement*> DTC_elements;
		DTC_elements = getAllMatchingChildElements(DTCblock_elements.at(b), "DTC");
		char assignedBits = 0;
		for (unsigned int k=0; k<DTC_elements.size(); k++)
		{
			// Get ID:
			std::string id;
			id = DTC_elements.at(k)->Attribute("id");
			if (!id.size())
				continue;
			// Get bit address:
			tmp_elements = getAllMatchingChildElements(DTC_elements.at(k), "BIT");
			if (tmp_elements.size() != 1)
				continue;
			str = tmp_elements.at(0)->GetText();
			if (str == NULL)
				continue;
			unsigned long int bitaddr = strtoul( str, NULL, 0 );
			if ((bitaddr < 1) || (bitaddr > 8))
				continue;
			// Search for duplicate DTCs:
			if (assignedBits == (assignedBits | static_cast<char>(1 << (bitaddr-1))))
			{
				// Display DTC as UNKNOWN
				dtcblock.code[bitaddr-1] = "???";
				dtcblock.title[bitaddr-1] = "     ???     (0x";
				if (dtcblock.byteAddr_currentOrTempOrLatest != MEMORY_ADDRESS_NONE)
					dtcblock.title[bitaddr-1] += QString::number(dtcblock.byteAddr_currentOrTempOrLatest,16).toUpper();
				if (dtcblock.byteAddr_historicOrMemorized != MEMORY_ADDRESS_NONE)
				{
					if (dtcblock.byteAddr_currentOrTempOrLatest != MEMORY_ADDRESS_NONE)
						dtcblock.title[bitaddr-1] += "/0x";
					dtcblock.title[bitaddr-1] += QString::number(dtcblock.byteAddr_historicOrMemorized,16).toUpper();
				}
				dtcblock.title[k] += " Bit " + QString::number(bitaddr) + ")";
				/* NOTE: see comments at the end of the function */
				continue;
			}
			// --- Get common data ---
			// Find DTC data:
			XMLElement *DTCdata_element = NULL;
			attribCond.name = "id";
			attribCond.value = id;
			attribCond.condition = attributeCondition::equal;
			tmp_elements = getAllMatchingChildElements(_datacommon_root_element, "DTC", std::vector<attributeCondition>(1, attribCond));
			if (tmp_elements.size() != 1)
				continue;
			DTCdata_element = tmp_elements.at(0);
			// Get code:
			tmp_elements = getAllMatchingChildElements(DTCdata_element, "CODE");
			if (tmp_elements.size() != 1)
				continue;
			str = tmp_elements.at(0)->GetText();
			if (str == NULL)
				continue;
			dtcblock.code[bitaddr-1] = QString( str );
			// Get title:
			attribCond.name = "lang";
			attribCond.value = _lang;
			attribCond.condition = attributeCondition::equal;
			tmp_elements = getAllMatchingChildElements(DTCdata_element, "TITLE", std::vector<attributeCondition>(1, attribCond));
			if (tmp_elements.size() != 1)
			{
				if (tmp_elements.size() < 1 && (_lang != "en")) // fall back to english language:
				{
					attribCond.value = "en";
					tmp_elements = getAllMatchingChildElements(DTCdata_element, "TITLE", std::vector<attributeCondition>(1, attribCond));
					if (tmp_elements.size() != 1)
						continue;
				}
				else
					continue;
			}
			str = tmp_elements.at(0)->GetText();
			if (str == NULL)
				continue;
			dtcblock.title[bitaddr-1] = QString( str );
			assignedBits |= static_cast<char>(1 << (bitaddr-1));
		}
		// Add DTC-block to the list:
		dcs->push_back(dtcblock);
	}
	return true;
	/* NOTE: - DCs with missing definitions are displayed with address byte + bit in the title field
		 - DCs with existing definitions and empty code- and title-fields are ignored */
}


bool SSMLegacyDefinitionsInterface::measuringBlocks(std::vector<mb_intl_dt> *mbs)
{
	std::vector<XMLElement*> MB_elements;
	std::vector<XMLElement*> MB_elements2;
	const char *str = NULL;

	if (!_id_set)
		return false;
	mbs->clear();
	if (_defs_root_element)
		MB_elements = getAllMatchingChildElements(_defs_root_element, "MB");
	if (_defs_for_id_b1_element)
	{
		MB_elements2 = getAllMatchingChildElements(_defs_for_id_b1_element, "MB");
		MB_elements.insert(MB_elements.end(), MB_elements2.begin(), MB_elements2.end());
	}
	if (_defs_for_id_b2_element)
	{
		MB_elements2 = getAllMatchingChildElements(_defs_for_id_b2_element, "MB");
		MB_elements.insert(MB_elements.end(), MB_elements2.begin(), MB_elements2.end());
	}
	if (_defs_for_id_b3_element)
	{
		MB_elements2 = getAllMatchingChildElements(_defs_for_id_b3_element, "MB");
		MB_elements.insert(MB_elements.end(), MB_elements2.begin(), MB_elements2.end());
	}
	for (unsigned int k=0; k<MB_elements.size(); k++)
	{
		std::vector<XMLElement*> tmp_elements;
		mb_intl_dt mb;
		// Get ID:
		std::string id;
		id = MB_elements.at(k)->Attribute("id");
		if (!id.size())
			continue;
		// Get address:
		tmp_elements = getAllMatchingChildElements(MB_elements.at(k), "ADDRESS");
		if (tmp_elements.size() != 1)
			continue;
		str = tmp_elements.at(0)->GetText();
		if (str == NULL)
			continue;
		unsigned long int addr = strtoul( str, NULL, 0 );
		if (addr > 0xffff)
			continue;
		// Check for duplicate definitions (addresses):
		bool duplicate = false;
		for (unsigned int m=0; m<mbs->size(); m++)
		{
			if ((addr == mbs->at(m).addr_low) || (addr == mbs->at(m).addr_high))
			{
				duplicate = true;
				mbs->erase(mbs->begin() + m);
				break;
			}
		}
		if (duplicate)
			continue;
		mb.addr_low = addr;
		mb.addr_high = MEMORY_ADDRESS_NONE;
		// --- Get common data ---
		// Find MB data:
		XMLElement *MBdata_element = NULL;
		attributeCondition attribCond;
		attribCond.name = "id";
		attribCond.value = id;
		attribCond.condition = attributeCondition::equal;
		tmp_elements = getAllMatchingChildElements(_datacommon_root_element, "MB", std::vector<attributeCondition>(1, attribCond));
		if (tmp_elements.size() != 1)
			continue;
		MBdata_element = tmp_elements.at(0);
		// Get title:
		attribCond.name = "lang";
		attribCond.value = _lang;
		tmp_elements = getAllMatchingChildElements(MBdata_element, "TITLE", std::vector<attributeCondition>(1, attribCond));
		if (tmp_elements.size() != 1)
		{
			if (tmp_elements.size() < 1 && (_lang != "en")) // fall back to english language:
			{
				attribCond.value = "en";
				tmp_elements = getAllMatchingChildElements(MBdata_element, "TITLE", std::vector<attributeCondition>(1, attribCond));
				if (tmp_elements.size() != 1)
					continue;
			}
			else
				continue;
		}
		mb.title = QString( tmp_elements.at(0)->GetText() );
		// Get unit:
		attribCond.value = "all";
		tmp_elements = getAllMatchingChildElements(MBdata_element, "UNIT", std::vector<attributeCondition>(1, attribCond));
		if (tmp_elements.size() != 1)
		{
			attribCond.value = _lang;
			tmp_elements = getAllMatchingChildElements(MBdata_element, "UNIT", std::vector<attributeCondition>(1, attribCond));
			if (tmp_elements.size() != 1)
			{
				if (tmp_elements.size() < 1 && (_lang != "en")) // fall back to english language:
				{
					attribCond.value = "en";
					tmp_elements = getAllMatchingChildElements(MBdata_element, "UNIT", std::vector<attributeCondition>(1, attribCond));
					if (tmp_elements.size() != 1)
						continue;
				}
				else
					continue;
			}
		}
		mb.unit = QString( tmp_elements.at(0)->GetText() );
		// Get formula:
		tmp_elements = getAllMatchingChildElements(MBdata_element, "FORMULA");
		if (tmp_elements.size() != 1)
			continue;
		str = tmp_elements.at(0)->GetText();
		if (str == NULL)
			continue;
		mb.scaleformula = QString( str );
		// Get precision:
		tmp_elements = getAllMatchingChildElements(MBdata_element, "PRECISION");
		if (tmp_elements.size() != 1)
			continue;
		str = tmp_elements.at(0)->GetText();
		if (str == NULL)
			continue;
		long int precision = strtol( str, NULL, 0 );
		if ((precision >= -128) && (precision <= 127))
			mb.precision = precision;
		else
			continue;
		// Add MB to the list:
		mbs->push_back(mb);
	}
	return true;
}


bool SSMLegacyDefinitionsInterface::switches(std::vector<sw_intl_dt> *sws)
{
	std::vector<XMLElement*> SWblock_elements;
	std::vector<XMLElement*> SWblock_elements2;
	const char *str = NULL;

	if (!_id_set)
		return false;
	sws->clear();
	if (_defs_root_element)
		SWblock_elements = getAllMatchingChildElements(_defs_root_element, "SWBLOCK");
	if (_defs_for_id_b1_element)
	{
		SWblock_elements2 = getAllMatchingChildElements(_defs_for_id_b1_element, "SWBLOCK");
		SWblock_elements.insert(SWblock_elements.end(), SWblock_elements2.begin(), SWblock_elements2.end());
	}
	if (_defs_for_id_b2_element)
	{
		SWblock_elements2 = getAllMatchingChildElements(_defs_for_id_b2_element, "SWBLOCK");
		SWblock_elements.insert(SWblock_elements.end(), SWblock_elements2.begin(), SWblock_elements2.end());
	}
	if (_defs_for_id_b3_element)
	{
		SWblock_elements2 = getAllMatchingChildElements(_defs_for_id_b3_element, "SWBLOCK");
		SWblock_elements.insert(SWblock_elements.end(), SWblock_elements2.begin(), SWblock_elements2.end());
	}
	for (unsigned int b=0; b<SWblock_elements.size(); b++)
	{
		sw_intl_dt sw;
		std::vector<XMLElement*> tmp_elements;
		// Get block address:
		tmp_elements = getAllMatchingChildElements(SWblock_elements.at(b), "ADDRESS");
		if (tmp_elements.size() != 1)
			continue;
		str = tmp_elements.at(0)->GetText();
		if (str == NULL)
			continue;
		unsigned long int byteaddr = strtoul( str, NULL, 0 );
		if (byteaddr > 0xffff)
			continue;
		sw.byteAddr = byteaddr;
		// Get switches:
		std::vector<XMLElement*> SW_elements;
		SW_elements = getAllMatchingChildElements(SWblock_elements.at(b), "SW");
		for (unsigned int k=0; k<SW_elements.size(); k++)
		{
			// Get ID:
			std::string id;
			id = SW_elements.at(k)->Attribute("id");
			if (!id.size())
				continue;
			// Get bit address:
			tmp_elements = getAllMatchingChildElements(SW_elements.at(k), "BIT");
			if (tmp_elements.size() != 1)
				continue;
			str = tmp_elements.at(0)->GetText();
			if (str == NULL)
				continue;
			unsigned long int bitaddr = strtoul( str, NULL, 0 );
			if ((bitaddr < 1) || (bitaddr > 8))
				continue;
			// Check for duplicate definitions (address + bit):
			bool duplicate = false;
			for (unsigned int s=0; s<sws->size(); s++)
			{
				if ((byteaddr == sws->at(s).byteAddr) && (bitaddr == sws->at(s).bitAddr))
				{
					duplicate = true;
					sws->erase(sws->begin() + s);
					break;
				}
			}
			if (duplicate)
				continue;
			/* NOTE: switches with the same address can be defined across multiple SWBLOCKs */
			sw.bitAddr = bitaddr;
			// --- Get common data ---:
			// Find SW data:
			XMLElement *SWdata_element = NULL;
			attributeCondition attribCond;
			attribCond.name = "id";
			attribCond.value = id;
			attribCond.condition = attributeCondition::equal;
			tmp_elements = getAllMatchingChildElements(_datacommon_root_element, "SW", std::vector<attributeCondition>(1, attribCond));
			if (tmp_elements.size() != 1)
				continue;
			SWdata_element = tmp_elements.at(0);
			// Get title:
			attribCond.name = "lang";
			attribCond.value = _lang;
			tmp_elements = getAllMatchingChildElements(SWdata_element, "TITLE", std::vector<attributeCondition>(1, attribCond));
			if (tmp_elements.size() != 1)
			{
				if (tmp_elements.size() < 1 && (_lang != "en")) // fall back to english language:
				{
					attribCond.value = "en";
					tmp_elements = getAllMatchingChildElements(SWdata_element, "TITLE", std::vector<attributeCondition>(1, attribCond));
					if (tmp_elements.size() != 1)
						continue;
				}
				else
					continue;
			}
			sw.title = QString( tmp_elements.at(0)->GetText() );
			// Get unit:
			tmp_elements = getAllMatchingChildElements(SWdata_element, "UNIT", std::vector<attributeCondition>(1, attribCond));
			if (tmp_elements.size() != 1)
			{
				attribCond.value = "all";
				tmp_elements = getAllMatchingChildElements(SWdata_element, "UNIT", std::vector<attributeCondition>(1, attribCond));
				if (tmp_elements.size() != 1)
				{
					if (tmp_elements.size() < 1 && (_lang != "en")) // fall back to english language:
					{
						attribCond.value = "en";
						tmp_elements = getAllMatchingChildElements(SWdata_element, "UNIT", std::vector<attributeCondition>(1, attribCond));
						if (tmp_elements.size() != 1)
							continue;
					}
					else
						continue;
				}
			}
			sw.unit = QString( tmp_elements.at(0)->GetText() );
			// Add SW to the list:
			sws->push_back(sw);
		}
	}
	return true;
}

// PRIVATE:

std::vector<XMLElement*> SSMLegacyDefinitionsInterface::getAllMatchingChildElements(XMLElement *pParent, std::string elementName, std::vector<attributeCondition> attribConditions)
{
	std::vector<XMLElement*> retElements;
	XMLElement *pElement = NULL;
	const XMLAttribute* pAttrib = NULL;
	double cond_d_val = 0;
	double attr_d_val = 0;
	bool attribOK = false;
	unsigned int attribsOK = 0;
	for (pElement = pParent->FirstChildElement(elementName.c_str()); pElement != NULL; pElement = pElement->NextSiblingElement(elementName.c_str()))
	{
		attribsOK = false;
		// Check attribute conditions:
		for (unsigned int c=0; c<attribConditions.size(); c++)
		{
			pAttrib = pElement->FirstAttribute();
			while (pAttrib)
			{
				if (pAttrib->Name() == attribConditions.at(c).name)
				{
					attribOK = false;
					if (StrToDouble(attribConditions.at(c).value, &cond_d_val) && StrToDouble(pAttrib->Value(), &attr_d_val))
					{
						// Compare doubles:
						if (attribConditions.at(c).condition == attributeCondition::equal)
						{
							attribOK = (attr_d_val == cond_d_val);
						}
						else if (attribConditions.at(c).condition == attributeCondition::smaller)
						{
							attribOK = (attr_d_val < cond_d_val);
						}
						else if (attribConditions.at(c).condition == attributeCondition::larger)
						{
							attribOK = (attr_d_val > cond_d_val);
						}
						else if (attribConditions.at(c).condition == attributeCondition::equalOrSmaller)
						{
							attribOK = (attr_d_val <= cond_d_val);
						}
						else if (attribConditions.at(c).condition == attributeCondition::equalOrLarger)
						{
							attribOK = (attr_d_val >= cond_d_val);
						}
					}
					else
					{
						// Compare strings:
						if (attribConditions.at(c).condition == attributeCondition::equal)
						{
							attribOK = (pAttrib->Value() == attribConditions.at(c).value);
						}
						else if (attribConditions.at(c).condition == attributeCondition::smaller)
						{
							attribOK = (pAttrib->Value() < attribConditions.at(c).value);
						}
						else if (attribConditions.at(c).condition == attributeCondition::larger)
						{
							attribOK = (pAttrib->Value() > attribConditions.at(c).value);
						}
						else if (attribConditions.at(c).condition == attributeCondition::equalOrSmaller)
						{
							attribOK = (pAttrib->Value() <= attribConditions.at(c).value);
						}
						else if (attribConditions.at(c).condition == attributeCondition::equalOrLarger)
						{
							attribOK = (pAttrib->Value() >= attribConditions.at(c).value);
						}
					}
					if (attribOK)
					{
						attribsOK++;
						break;
					}
				}
				pAttrib = pAttrib->Next();
			}
		}
		if (attribsOK == attribConditions.size())
			retElements.push_back(pElement);
	}
	return retElements;
}


bool SSMLegacyDefinitionsInterface::StrToDouble(std::string str, double *d)
{
	double dbl = 0;
	int i = 0;
	std::stringstream sstr;
	while (str.size() && (str.at(0) == ' '))
		str = str.substr(1);
	if (!str.size())
		return false;
	sstr << str;
	if (str.find("0x") == 0 || (str.find("-0x") == 0) || (str.find("+0x") == 0))
	{
		sstr >> std::hex >> i;
		dbl = i;
	}
	else
		sstr >> dbl;
	if (sstr.eof())
	{
		*d = dbl;
		return true;
	}
	return false;
}


bool SSMLegacyDefinitionsInterface::checkDefsFormatVersion(std::string version_str)
{
	unsigned long int version_major = 0, version_major_min = 0, version_major_current = 0;
	unsigned long int version_minor = 0, version_minor_min = 0, version_minor_current = 0;
	unsigned long int version_bugfix = 0, version_bugfix_min = 0, version_bugfix_current = 0;
	// Convert version strings to numeric data:
	if (!versionStrToVersionNum(SSM1_DEFS_FORMAT_VERSION_MIN, &version_major_min, &version_minor_min, &version_bugfix_min))
		return false;
	if (!versionStrToVersionNum(SSM1_DEFS_FORMAT_VERSION_CURRENT, &version_major_current, &version_minor_current, &version_bugfix_current))
		return false;
	if (!versionStrToVersionNum(version_str, &version_major, &version_minor, &version_bugfix))
		return false;
	// Check against minimum supported version:
	if (version_major < version_major_min)
		return false;
	if (version_major == version_major_min)
	{
		if (version_minor < version_minor_min)
			return false;
		if (version_minor == version_minor_min)
		{
			if (version_bugfix < version_bugfix_min)
				return false;
		}
	}
	// Check against current version:
	if (version_major > version_major_current)
		return false;
	if (version_major == version_major_current)
	{
		if (version_minor > version_minor_current)
			return false;
		if (version_minor == version_minor_current)
		{
			if (version_bugfix > version_bugfix_current)
				return false;
		}
	}
	return true;
}


bool SSMLegacyDefinitionsInterface::versionStrToVersionNum(std::string version_str, unsigned long int *version_major, unsigned long int *version_minor, unsigned long int *version_bugfix)
{
	size_t dot_pos[2];
	unsigned char num_dots = 0;
	for (unsigned int k=0; k<version_str.size(); k++)
	{
		if (version_str[k] == '.')
		{
			if (num_dots > 1)
				return false;
			dot_pos[num_dots] = k;
			num_dots++;
		}
		else if ((version_str[k] < '0')  && (version_str[k] > '9'))
			return false;
	}
	if (num_dots != 2)
		return false;
	std::string version_str_major = version_str.substr(0, dot_pos[0]);
	std::string version_str_minor = version_str.substr(dot_pos[0]+1, dot_pos[1]-dot_pos[0]-1);
	std::string version_str_bugfix = version_str.substr(dot_pos[1]+1);
	*version_major = strtoul(version_str_major.data(), NULL, 0);
	*version_minor = strtoul(version_str_minor.data(), NULL, 0);
	*version_bugfix = strtoul(version_str_bugfix.data(), NULL, 0);
	return true;
}

