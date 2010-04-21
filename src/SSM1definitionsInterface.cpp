/*
 * SSM1definitionsInterface.cpp - Interface to the SSM1-definitions
 *
 * Copyright (C) 2009-2010 Comer352l
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

#include <SSM1definitionsInterface.h>


SSM1definitionsInterface::SSM1definitionsInterface(std::string filename, std::string lang, char id[3])
{
	_xmldoc = NULL;
	_defs_root_node = NULL;
	_datacommon_root_node = NULL;
	_defs_for_id_b1_node = NULL;
	_defs_for_id_b2_node = NULL;
	_defs_for_id_b3_node = NULL;
	_lang = lang;
	_id_set = false;
	selectDefinitionsFile(filename);
	selectID(id);
}


bool SSM1definitionsInterface::selectDefinitionsFile(std::string filename)
{
	std::vector<TiXmlElement*> elements;
	if (!filename.size())
		goto error;
	_xmldoc = new TiXmlDocument(filename);
	TiXmlNode *root_node;
	if (!_xmldoc->LoadFile())	// current doc is closed/deleted before !
		goto error;
	// Find and save node FSSM_SSM1_DEFINITIONS
	root_node = _xmldoc->FirstChildElement("FSSM_SSM1_DEFINITIONS");
	if (!root_node)
		goto error;
	// Find and save node DEFINITIONS
	elements = getAllMatchingChildElements(root_node, "DEFINITIONS");
	if (elements.size() != 1)
		goto error;
	_defs_root_node = elements.at(0);
	// Find and save node DATA_COMMON
	elements = getAllMatchingChildElements(root_node, "DATA_COMMON");
	if (elements.size() != 1)
		goto error;
	_datacommon_root_node = elements.at(0);
	// Find and save definitions node for the current ID:
	if (_id_set)
		findIDnodes();
	return true;
	
error:
	delete _xmldoc;
	_xmldoc = NULL;
	_defs_root_node = NULL;
	_datacommon_root_node = NULL;
	_defs_for_id_b1_node = NULL;
	_defs_for_id_b2_node = NULL;
	_defs_for_id_b3_node = NULL;
	return false;
}


void SSM1definitionsInterface::setLanguage(std::string lang)
{
	_lang = lang;
}


void SSM1definitionsInterface::selectID(char id[3])
{
	if (id)
	{
		_ID[0] = id[0];
		_ID[1] = id[1];
		_ID[2] = id[2];
		_id_set = true;
		findIDnodes();
	}
	else
	{
		_id_set = false;
		_defs_for_id_b1_node = NULL;
		_defs_for_id_b2_node = NULL;
		_defs_for_id_b3_node = NULL;
	}
}


bool SSM1definitionsInterface::systemDescription(std::string *description)
{
	std::vector<TiXmlElement*> elements;
	if (!_defs_for_id_b2_node)
		return false;
	elements = getAllMatchingChildElements(_defs_for_id_b2_node, "SYSTEMDESCRIPTION");
	if (elements.size() != 1)
		return false;
	*description = elements.at(0)->GetText();
	return true;
}


bool SSM1definitionsInterface::model(std::string *name)
{
	std::vector<TiXmlElement*> elements;
	if (!_defs_for_id_b2_node)
		return false;
	elements = getAllMatchingChildElements(_defs_for_id_b2_node, "MODEL");
	if (elements.size() != 1)
		return false;
	*name = elements.at(0)->GetText();
	return true;
}


bool SSM1definitionsInterface::year(std::string *yearstr)
{
	std::vector<TiXmlElement*> elements;
	if (!_defs_for_id_b1_node)
		return false;
	elements = getAllMatchingChildElements(_defs_for_id_b1_node, "YEAR");
	if (elements.size() != 1)
		return false;
	*yearstr = elements.at(0)->GetText();
	return true;
}


bool SSM1definitionsInterface::clearMemoryData(unsigned int *address, char *value)
{
	std::vector<TiXmlElement*> elements;
	TiXmlElement *CM_element;
	TiXmlElement *addr_element;
	if (!_defs_for_id_b3_node)
		return false;
	elements = getAllMatchingChildElements(_defs_for_id_b3_node, "CLEARMEMORY");
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
	unsigned long int addr = strtoul( addr_element->GetText(), NULL, 0 );
	unsigned long int val = strtoul( elements.at(0)->GetText(), NULL, 0 );
	if ((addr > 0xffff) || (val > 0xff))
		return false;
	*address = addr;
	*value = val;
	return true;
}


bool SSM1definitionsInterface::diagnosticCodes(std::vector<dc_defs_dt> *dcs)
{
	std::vector<TiXmlElement*> DTCblock_elements;
	if (!_defs_for_id_b3_node)
		return false;
	dcs->clear();
	DTCblock_elements = getAllMatchingChildElements(_defs_for_id_b3_node, "DTCBLOCK");
	for (unsigned int b=0; b<DTCblock_elements.size(); b++)
	{
		dc_defs_dt dtcblock;
		std::vector<TiXmlElement*> tmp_elements;
		// --- Get address(es) ---:
		// Get address for current DTCs:
		attributeCondition attribCond;
		attribCond.name = "type";
		attribCond.value = "current";
		attribCond.condition = attributeCondition::equal;
		tmp_elements = getAllMatchingChildElements(DTCblock_elements.at(b), "ADDRESS", std::vector<attributeCondition>(1, attribCond));
		if (tmp_elements.size() != 1)
			continue;
		unsigned long int addr = strtoul( tmp_elements.at(0)->GetText(), NULL, 0 );
		if (addr > 0xffff)
			continue;
		dtcblock.byteAddr_currentOrTempOrLatest = addr;
		// Get address for historic DTCs:
		attribCond.value = "historic";
		tmp_elements = getAllMatchingChildElements(DTCblock_elements.at(b), "ADDRESS", std::vector<attributeCondition>(1, attribCond));
		if (tmp_elements.size() == 1)
		{
			addr = strtoul( tmp_elements.at(0)->GetText(), NULL, 0 );
			if (addr > 0xffff)
				continue;
			dtcblock.byteAddr_historicOrMemorized = addr;
		}
		else if (tmp_elements.size() == 0)
		{
			dtcblock.byteAddr_historicOrMemorized = 0xffffffff;	// NO ADDRESS
		}
		else if (tmp_elements.size() > 1)
		{
			continue;
		}
		for (unsigned char k=0; k<8; k++)
		{
			dtcblock.code[k] = "???";
			if (_lang == "de")
				dtcblock.title[k] = "UNBEKANNT (Adresse 0x";
			else
				dtcblock.title[k] = "UNKNOWN (Address 0x";
			dtcblock.title[k] += QString::number(dtcblock.byteAddr_currentOrTempOrLatest,16).toUpper();
			if (dtcblock.byteAddr_historicOrMemorized != 0xffffffff)
				dtcblock.title[k] += "/0x" + QString::number(dtcblock.byteAddr_historicOrMemorized,16).toUpper() + " Bit " + QString::number(k+1) + ")";
		}
		std::vector<TiXmlElement*> DTC_elements;
		DTC_elements = getAllMatchingChildElements(DTCblock_elements.at(b), "DTC");
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
			unsigned long int bitaddr = strtoul( tmp_elements.at(0)->GetText(), NULL, 0 );
			if ((bitaddr < 1) || (bitaddr > 8))
				continue;
			// --- Get common data ---
			// Find DTC data:
			TiXmlElement *DTCdata_element = NULL;
			attribCond.name = "id";
			attribCond.value = id;
			attribCond.condition = attributeCondition::equal;
			tmp_elements = getAllMatchingChildElements(_datacommon_root_node, "DTC", std::vector<attributeCondition>(1, attribCond));
			if (tmp_elements.size() != 1)
				continue;
			DTCdata_element = tmp_elements.at(0);
			// Get code:
			tmp_elements = getAllMatchingChildElements(DTCdata_element, "CODE");
			if (tmp_elements.size() != 1)
				continue;
			dtcblock.code[bitaddr-1] = QString::fromStdString( tmp_elements.at(0)->GetText() );
			// Get title:
			attribCond.name = "lang";
			attribCond.value = _lang;
			attribCond.condition = attributeCondition::equal;
			tmp_elements = getAllMatchingChildElements(DTCdata_element, "TITLE", std::vector<attributeCondition>(1, attribCond));
			if (tmp_elements.size() != 1)
				continue;
			dtcblock.title[bitaddr-1] = QString::fromStdString( tmp_elements.at(0)->GetText() );
		}
		// Add DTC-block to the list:
		dcs->push_back(dtcblock);
	}
	return true;
	/* NOTE: - DCs with missing definitions are displayed as UNKNOWN
		 - DCs with existing definitions and empty code- and title-fields will be ignored */
}


bool SSM1definitionsInterface::measuringBlocks(std::vector<mb_intl_dt> *mbs)
{
	std::vector<TiXmlElement*> MB_elements;
	if (!_defs_for_id_b3_node)
		return false;
	mbs->clear();
	MB_elements = getAllMatchingChildElements(_defs_for_id_b3_node, "MB");
	for (unsigned int k=0; k<MB_elements.size(); k++)
	{
		std::vector<TiXmlElement*> tmp_elements;
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
		unsigned long int addr = strtoul( tmp_elements.at(0)->GetText(), NULL, 0 );
		if (addr > 0xffff)
			continue;
		mb.adr_high = (addr & 0xffff) >> 2;
		mb.adr_low = addr & 0xff;
		// --- Get common data ---
		// Find MB data:
		TiXmlElement *MBdata_element = NULL;
		attributeCondition attribCond;
		attribCond.name = "id";
		attribCond.value = id;
		attribCond.condition = attributeCondition::equal;
		tmp_elements = getAllMatchingChildElements(_datacommon_root_node, "MB", std::vector<attributeCondition>(1, attribCond));
		if (tmp_elements.size() != 1)
			continue;
		MBdata_element = tmp_elements.at(0);
		// Get title:
		attribCond.name = "lang";
		attribCond.value = _lang;
		tmp_elements = getAllMatchingChildElements(MBdata_element, "TITLE", std::vector<attributeCondition>(1, attribCond));
		if (tmp_elements.size() != 1)
			continue;
		mb.title = QString::fromStdString( tmp_elements.at(0)->GetText() );
		// Get unit:
		tmp_elements = getAllMatchingChildElements(MBdata_element, "UNIT");
		if (tmp_elements.size() != 1)
			continue;
		mb.unit = QString::fromStdString( tmp_elements.at(0)->GetText() );
		// Get formula:
		tmp_elements = getAllMatchingChildElements(MBdata_element, "FORMULA");
		if (tmp_elements.size() != 1)
			continue;
		mb.scaleformula = QString::fromStdString( tmp_elements.at(0)->GetText() );
		// Get precision:
		tmp_elements = getAllMatchingChildElements(MBdata_element, "PRECISION");
		if (tmp_elements.size() != 1)
			continue;
		long int precision = strtol( tmp_elements.at(0)->GetText(), NULL, 0 );
		if ((precision >= -128) && (precision <= 127))
			mb.precision = precision;
		else
			continue;
		// Add MB to the list:
		mbs->push_back(mb);
	}
	return true;
}


bool SSM1definitionsInterface::switches(std::vector<sw_intl_dt> *sws)
{
	std::vector<TiXmlElement*> SWblock_elements;
	if (!_defs_for_id_b3_node)
		return false;
	sws->clear();
	SWblock_elements = getAllMatchingChildElements(_defs_for_id_b3_node, "SWBLOCK");
	for (unsigned int b=0; b<SWblock_elements.size(); b++)
	{
		sw_intl_dt sw;
		std::vector<TiXmlElement*> tmp_elements;
		// Get block address:
		tmp_elements = getAllMatchingChildElements(SWblock_elements.at(b), "ADDRESS");
		if (tmp_elements.size() != 1)
			continue;
		unsigned long int byteaddr = strtoul( tmp_elements.at(0)->GetText(), NULL, 0 );
		if (byteaddr > 0xffff)
			continue;
		sw.byteadr = byteaddr;
		// Get switches:
		std::vector<TiXmlElement*> SW_elements;
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
			unsigned long int bitaddr = strtoul( tmp_elements.at(0)->GetText(), NULL, 0 );
			if ((bitaddr < 1) || (bitaddr > 8))
				continue;
			sw.bitadr = bitaddr;
			// --- Get common data ---:
			// Find SW data:
			TiXmlElement *SWdata_element = NULL;
			attributeCondition attribCond;
			attribCond.name = "id";
			attribCond.value = id;
			attribCond.condition = attributeCondition::equal;
			tmp_elements = getAllMatchingChildElements(_datacommon_root_node, "SW", std::vector<attributeCondition>(1, attribCond));
			if (tmp_elements.size() != 1)
				continue;
			SWdata_element = tmp_elements.at(0);
			// Get title:
			attribCond.name = "lang";
			attribCond.value = _lang;
			tmp_elements = getAllMatchingChildElements(SWdata_element, "TITLE", std::vector<attributeCondition>(1, attribCond));
			if (tmp_elements.size() != 1)
				continue;
			sw.title = QString::fromStdString( tmp_elements.at(0)->GetText() );
			// Get unit:
			tmp_elements = getAllMatchingChildElements(SWdata_element, "UNIT");
			if (tmp_elements.size() != 1)
				continue;
			sw.unit = QString::fromStdString( tmp_elements.at(0)->GetText() );
			// Add SW to the list:
			sws->push_back(sw);
		}
	}
	return true;
}

// PRIVATE:

void SSM1definitionsInterface::findIDnodes()
{
	std::vector<TiXmlElement*> elements;
	std::vector<attributeCondition> attribConditions;
	attributeCondition attribCondition;

	_defs_for_id_b1_node = NULL;
	_defs_for_id_b2_node = NULL;
	_defs_for_id_b3_node = NULL;
	if (!_defs_root_node || !_id_set)
		return;
	attribCondition.name = "value";
	attribCondition.value = "0x" + libFSSM::StrToHexstr(_ID, 1);
	attribCondition.condition = attributeCondition::equal;
	elements = getAllMatchingChildElements(_defs_root_node, "ID_BYTE1", std::vector<attributeCondition>(1, attribCondition));
	if (elements.size() == 1)
	{
		_defs_for_id_b1_node = elements.at(0);
		attribCondition.value = "0x" + libFSSM::StrToHexstr(_ID+1, 1);
		elements = getAllMatchingChildElements(elements.at(0), "ID_BYTE2", std::vector<attributeCondition>(1, attribCondition));
		if (elements.size() == 1)
		{
			_defs_for_id_b2_node = elements.at(0);
			attribCondition.name = "value_start";
			attribCondition.value = "0x" + libFSSM::StrToHexstr(_ID+2, 1);
			attribCondition.condition = attributeCondition::equalOrSmaller;
			attribConditions.push_back(attribCondition);
			attribCondition.name = "value_end";
			attribCondition.condition = attributeCondition::equalOrLarger;
			attribConditions.push_back(attribCondition);
			elements = getAllMatchingChildElements(elements.at(0), "ID_BYTE3", attribConditions);
			if (elements.size() == 1)
				_defs_for_id_b3_node = elements.at(0);
		}
	}
}


std::vector<TiXmlElement*> SSM1definitionsInterface::getAllMatchingChildElements(TiXmlNode *pParent, std::string elementName, std::vector<attributeCondition> attribConditions)
{
	std::vector<TiXmlElement*> retElements;
	TiXmlNode *pChild = NULL;
	TiXmlElement *pElement = NULL;
	TiXmlAttribute* pAttrib = NULL;
	double cond_d_val = 0;
	double attr_d_val = 0;
	bool attribOK = false;
	unsigned int attribsOK = 0;
	for (pChild = pParent->FirstChildElement(elementName); pChild != 0; pChild = pChild->NextSibling(elementName)) 
	{
		pElement = pChild->ToElement();
		if (pElement)
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
					pAttrib=pAttrib->Next();
				}
			}
			if (attribsOK == attribConditions.size())
				retElements.push_back(pElement);
		}
	}
	return retElements;
}


bool SSM1definitionsInterface::StrToDouble(std::string str, double *d)
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

