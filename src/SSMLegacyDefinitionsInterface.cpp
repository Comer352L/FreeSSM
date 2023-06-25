/*
 * SSMLegacyDefinitionsInterface.cpp - Interface to the SSM legacy definitions
 *
 * Copyright (C) 2009-2023 Comer352L
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

#include "SSMLegacyDefinitionsInterface.h"

#include <sstream>
#include "libFSSM.h"


SSMLegacyDefinitionsInterface::SSMLegacyDefinitionsInterface(QString language) : SSMDefinitionsInterface(language)
{
	_xmldoc = NULL;
	_defs_root_element = NULL;
	_datacommon_root_element = NULL;
	_defs_for_id_b1_element = NULL;
	_defs_for_id_b2_element = NULL;
	_defs_for_id_b3_element = NULL;
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
		selectID(_ssmCUdata.SYS_ID);

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
	if ((defs_version != NULL) && (format_version != NULL))
		return false;
	if (!_id_set)
		return false;
	if (defs_version != NULL)
		*defs_version = _defs_version;
	if (format_version != NULL)
		*format_version = _defs_format_version;
	return true;
}


bool SSMLegacyDefinitionsInterface::selectID(const std::vector<char>& id)
{
	XMLElement *defs_for_id_b1_element = NULL;
	XMLElement *defs_for_id_b2_element = NULL;
	XMLElement *defs_for_id_b3_element = NULL;

	if (!_defs_root_element)
		return false;

	defs_for_id_b1_element = SSMLegacyDefinitionsInterface::searchForMatchingIDelement(_defs_root_element, 1, id.at(0));
	if (defs_for_id_b1_element == NULL)
		return false;
	defs_for_id_b2_element = SSMLegacyDefinitionsInterface::searchForMatchingIDelement(defs_for_id_b1_element, 2, id.at(1));
	if (defs_for_id_b2_element == NULL)
		return false;
	defs_for_id_b3_element = SSMLegacyDefinitionsInterface::searchForMatchingIDelement(defs_for_id_b2_element, 3, id.at(2));
	if (defs_for_id_b3_element == NULL)
		return false;

	_ssmCUdata.SYS_ID = id;
	_defs_for_id_b1_element = defs_for_id_b1_element;
	_defs_for_id_b2_element = defs_for_id_b2_element;
	_defs_for_id_b3_element = defs_for_id_b3_element;
	_id_set = true;
	return true;
}


bool SSMLegacyDefinitionsInterface::systemDescription(std::string *description)
{
	XMLElement* element = NULL;
	const char *str = NULL;

	if (description == NULL)
		return false;
	description->clear();
	if (!_id_set)
		return false;
	if (getMultilevelElementWithHighestPriority("SYSTEMDESCRIPTION", &element))
	{
		str = element->GetText();
		if (str != NULL)
			*description = std::string(str);
	}

	return true;
}


bool SSMLegacyDefinitionsInterface::model(std::string *name)
{
	XMLElement* element = NULL;
	const char *str = NULL;

	if (name == NULL)
		return false;
	name->clear();
	if (!_id_set)
		return false;
	if (getMultilevelElementWithHighestPriority("MODEL", &element))
	{
		str = element->GetText();
		if (str != NULL)
			*name = std::string(str);
	}

	return true;
}


bool SSMLegacyDefinitionsInterface::year(std::string *yearstr)
{
	XMLElement* element = NULL;
	const char *str = NULL;

	if (yearstr == NULL)
		return false;
	yearstr->clear();
	if (!_id_set)
		return false;
	if (getMultilevelElementWithHighestPriority("YEAR", &element))
	{
		str = element->GetText();
		if (str != NULL)
			*yearstr = std::string(str);
	}

	return true;
}


bool SSMLegacyDefinitionsInterface::getDCblockData(std::vector<dc_block_dt> *block_data)
{
	std::vector<XMLElement*> DCblock_elements;
	std::vector<unsigned int> _dirty_addr_values;

	if (block_data == NULL)
		return false;
	block_data->clear();
	if (!_id_set)
		return false;
	DCblock_elements = SSMLegacyDefinitionsInterface::getAllMultilevelElements("DCBLOCK");
	for (unsigned int b = 0; b < DCblock_elements.size(); b++)
	{
		XMLElement* current_xml_block_element = DCblock_elements.at(b);
		dc_block_dt new_block_data;

		// for all ADDRESS elements:
		for (XMLElement *addr_xml_element = current_xml_block_element->FirstChildElement("ADDRESS"); addr_xml_element != NULL; addr_xml_element = addr_xml_element->NextSiblingElement("ADDRESS"))
		{
next_addr_elem:
			dc_addr_dt new_addr;
			const XMLAttribute *pAttrib = NULL;
			std::string attrib_value;
			const char *str = NULL;
			unsigned long int addr_val;

			// Get and check address value:
			str = addr_xml_element->GetText();
			if (str == NULL)
				continue;
			addr_val = strtoul( str, NULL, 0 );
			if ((addr_val > 0) && (addr_val <= 0xFFFF))
				new_addr.address = addr_val;
			else
				continue;

			// Check if address is blacklisted (due to already detected invalid/ambiguous definitions):
			if (std::find(begin(_dirty_addr_values), end(_dirty_addr_values), addr_val) != std::end(_dirty_addr_values))
				continue;

			// Check for duplicate address values (invalid/ambiguous definitions):
			for (unsigned int b2 = 0; b2 < block_data->size(); b2++)
			{
				dc_block_dt *p_current_bd = &block_data->at(b2);
				for (unsigned int a2 = 0; a2 < p_current_bd->addresses.size(); a2++)
				{
					dc_addr_dt *p_current_ad = &p_current_bd->addresses.at(a2);
					if (p_current_ad->address == addr_val)
					{
						// Delete address from block data:
						p_current_bd->addresses.erase(p_current_bd->addresses.begin() + a2);
						// Delete block data, if no address remain:
						if (p_current_bd->addresses.size() < 1)
							block_data->erase(block_data->begin() + b2);
						// Blacklist address to avoid adding it again later:
						_dirty_addr_values.push_back(addr_val);
						goto next_addr_elem;
					}
				}
			}

			// Get and check address type attribute:
			pAttrib = addr_xml_element->FindAttribute("type");
			if (pAttrib != NULL)
			{
				attrib_value = pAttrib->Value();
				if (attrib_value == "currentOrTempOrLatest")
					new_addr.type = dc_addr_dt::Type::currentOrTempOrLatest;
				else if (attrib_value == "historicOrMemorized")
					new_addr.type = dc_addr_dt::Type::historicOrMemorized;
				else
				{
					_dirty_addr_values.push_back(addr_val);
					continue;
				}
			}
			else
			{
				_dirty_addr_values.push_back(addr_val);
				continue;
			}

			// Get and check address scaling attribute:
			pAttrib = addr_xml_element->FindAttribute("scaling");
			if (pAttrib != NULL)
			{
				attrib_value = pAttrib->Value();
				if (attrib_value == "bitwise")
					new_addr.scaling = dc_addr_dt::Scaling::bitwise;
					/* NOTE: Attribute "bitfield_id" may or may not be present.
					 *       We don't check if a corresponding valid BITFIELD elemement exists.
					 *       Even if it doesn't exist, we nevertheless want to report active DCs with generic code+title strings.
					 */
				else if (attrib_value == "list")
					new_addr.scaling = dc_addr_dt::Scaling::list;
					/* NOTE: Attribute "list_id" may or may not be present.
					 *       We don't check if a corresponding valid LIST elemement exists.
					 *       Even if it doesn't exist, we nevertheless want to report active DCs with generic code+title strings.
					 */
				else
				{
					_dirty_addr_values.push_back(addr_val);
					continue;
				}
			}
			else
			{
				_dirty_addr_values.push_back(addr_val);
				continue;
			}

			// Add address data to address block data:
			new_block_data.addresses.push_back(new_addr);
		}

		// Add address block data to list of address blocks:
		if (new_block_data.addresses.size() > 0)
			block_data->push_back(new_block_data);
	}

	return true;
}


bool SSMLegacyDefinitionsInterface::measuringBlocks(std::vector<mb_intl_dt> *mbs)
{
	return taggedMeasuringBlocks(mbs);
}


bool SSMLegacyDefinitionsInterface::switches(std::vector<sw_intl_dt> *sws)
{
	return taggedSwitches(sws);
}


bool SSMLegacyDefinitionsInterface::adjustments(std::vector<adjustment_intl_dt> *adjustments)
{
	std::vector<XMLElement*> ADJ_elements;

	if (adjustments == NULL)
		return false;
	adjustments->clear();
	if (!_id_set)
		return false;
	ADJ_elements = getAllMultilevelElements("ADJ");
	for (unsigned int k=0; k<ADJ_elements.size(); k++)
	{
		std::vector<XMLElement*> tmp_elements;
		adjustment_intl_dt adj;
		// Get ID:
		std::string id;
		id = ADJ_elements.at(k)->Attribute("id");
		if (!id.size())
			continue;
		// Get address:
		unsigned int addr;
		if (!getAddressElementValue(ADJ_elements.at(k), &addr))
			continue;
		// Check for duplicate definitions (addresses):
		bool duplicate = false;
		for (unsigned int a=0; a<adjustments->size(); a++)
		{
			if ((addr == adjustments->at(a).addrLow) || (addr == adjustments->at(a).addrHigh))
			{
				duplicate = true;
				adjustments->erase(adjustments->begin() + a);
				break;
			}
		}
		if (duplicate)
			continue;
		adj.addrLow = addr;
		adj.addrHigh = MEMORY_ADDRESS_NONE;
		// --- Get common data ---
		// Find ADJ data:
		XMLElement *ADJdata_element = NULL;
		if (!getCommonDataElementWithMatchingID("ADJ", id, &ADJdata_element))
			continue;
		// Get title:
		if (!getLanguageDependentElementString(ADJdata_element, "TITLE", &adj.title))
			continue;
		// Get unit:
		if (!getLanguageDependentElementString(ADJdata_element, "UNIT", &adj.unit))
			continue;
		// Get formula:
		if (!getElementString(ADJdata_element, "FORMULA", &adj.formula))
			continue;
		// Get default raw value:
		if (!getRawValueElementValue(ADJdata_element, "DEFAULT_RAW_VALUE", &adj.rawDefault))
			continue;
		// Get lower raw value limit:
		if (!getRawValueElementValue(ADJdata_element, "MIN_RAW_VALUE", &adj.rawMin))
			continue;
		// Get upper raw value limit:
		if (!getRawValueElementValue(ADJdata_element, "MAX_RAW_VALUE", &adj.rawMax))
			continue;
		// Check if min/max/default raw values are consistent/valid:
		if ((adj.rawDefault < adj.rawMin) || (adj.rawMax < adj.rawDefault))
			continue;
		// Get precision:
		if (!getPrecisionElementValue(ADJdata_element, &adj.precision))
			continue;
		// Add Adjustment Value to the list:
		adjustments->push_back(adj);
	}
	return true;
}


bool SSMLegacyDefinitionsInterface::actuatorTests(std::vector<actuator_dt> *actuators)
{
	std::vector<XMLElement*> ACT_elements;

	if (actuators == NULL)
		return false;
	actuators->clear();
	if (!_id_set)
		return false;
	ACT_elements = getAllMultilevelElements("ACT");
	for (unsigned int k=0; k<ACT_elements.size(); k++)
	{
		std::vector<XMLElement*> tmp_elements;
		actuator_dt act;
		// Get ID:
		std::string id;
		id = ACT_elements.at(k)->Attribute("id");
		if (!id.size())
			continue;
		// Get address:
		unsigned int addr;
		if (!getAddressElementValue(ACT_elements.at(k), &addr))
			continue;
		// Get bit:
		unsigned char bit;
		if (!getBitElementValue(ACT_elements.at(k), &bit))
			continue;
		// Check for duplicate definitions (addresses):
		bool duplicate = false;
		for (unsigned int a=0; a<actuators->size(); a++)
		{
			if ((addr == actuators->at(a).byteAddr) && (bit == actuators->at(a).bitAddr))
			{
				duplicate = true;
				actuators->erase(actuators->begin() + a);
				break;
			}
		}
		if (duplicate)
			continue;
		act.byteAddr = addr;
		act.bitAddr = bit;
		// --- Get common data ---
		XMLElement *ACTdata_element = NULL;
		if (!getCommonDataElementWithMatchingID("ACT", id, &ACTdata_element))
			continue;
		// Get title:
		if (!getLanguageDependentElementString(ACTdata_element, "TITLE", &act.title))
			continue;
		// Add Actuator Test to the list:
		actuators->push_back(act);
	}
	return true;
}


bool SSMLegacyDefinitionsInterface::clearMemoryData(unsigned int *address, char *value)
{
	std::vector<XMLElement*> elements;
	XMLElement *CM_element = NULL;
	const char *str = NULL;
	unsigned int addr;
	unsigned long int val;

	if ((address != NULL) && (value != NULL))
		return false;
	if (address != NULL)
		*address = MEMORY_ADDRESS_NONE;
	if (value != NULL)
		*value = '\x00';
	if (!_id_set)
		return false;
	if (!getMultilevelElementWithHighestPriority("CLEARMEMORY", &CM_element))
		return true;
	if (!getAddressElementValue(CM_element, &addr))
		return true;
	elements = getAllMatchingChildElements(CM_element, "VALUE");
	if (elements.size() != 1)
		return true;
	str = elements.at(0)->GetText();
	if (str == NULL)
		return true;
	val = strtoul( str, NULL, 0 );
	if (val > 0xff)
		return true;
	if (address != NULL)
		*address = addr;
	if (value != NULL)
		*value = val;
	return true;
}


bool SSMLegacyDefinitionsInterface::MBdata_engineRunning(mb_enginespeed_data_dt *mb_enginespeed_data)
{
	std::vector<mb_intl_dt> mbs;

	if (mb_enginespeed_data == NULL)
		return false;
	*mb_enginespeed_data = mb_enginespeed_data_dt();
	if (!taggedMeasuringBlocks(&mbs, "ENGINESPEED"))
		return false;
	if (mbs.size() == 1)
	{
		mb_enginespeed_data->addr_low = mbs.at(0).addrLow;
		mb_enginespeed_data->addr_high = mbs.at(0).addrHigh;
		mb_enginespeed_data->scaling_formula = mbs.at(0).formula;
	}
	return true;
}


bool SSMLegacyDefinitionsInterface::SWdata_testModeState(sw_stateindication_data_dt *sw_testmode_data)
{
	return SWdata_xxxState(sw_testmode_data, "TESTMODESTATE");
}


bool SSMLegacyDefinitionsInterface::SWdata_DCheckState(sw_stateindication_data_dt *sw_dcheckactive_data)
{
	return SWdata_xxxState(sw_dcheckactive_data, "DCHECKSTATE");
}


bool SSMLegacyDefinitionsInterface::SWdata_ignitionState(sw_stateindication_data_dt *sw_ignitionstate_data)
{
	return SWdata_xxxState(sw_ignitionstate_data, "IGNITIONSTATE");
}


void SSMLegacyDefinitionsInterface::getDCcontent(unsigned int address, char databyte, QStringList *codes, QStringList *titles)
{
	/* NOTE: This function should actually never be called with an invalid address or address for which
	 *       no valid+unambiguous definitions exist, because getDCblockData(...) doesn't report such addresses.
	 */
	if ((codes == NULL) && (titles == NULL))
		return;
	if (codes != NULL)
		codes->clear();
	if (titles != NULL)
		titles->clear();
	if (!_id_set)
		return rawbyteToSingleSubstitudeDC (databyte, codes, titles);

	// Get address element:
	XMLElement *DCaddr_elem = NULL;
	DCaddr_elem = getDCaddressElementForAddress(address);
	if (DCaddr_elem == NULL)
		return rawbyteToSingleSubstitudeDC (databyte, codes, titles);

	// Get scaling attribute from address data:
	std::string scaling_str;
	if (!getAttributeStr(DCaddr_elem, "scaling", &scaling_str))
		return rawbyteToSingleSubstitudeDC (databyte, codes, titles);

	// Convert scaling attribute to scaling type:
	dc_addr_dt::Scaling scaling;
	if (!scalingAttribStrToScaling(scaling_str, &scaling))
		return rawbyteToSingleSubstitudeDC (databyte, codes, titles);

	// Get DC code(s) + title(s):
	if (scaling == dc_addr_dt::Scaling::bitwise)
	{
		rawbyteToAssignmentListDCs(address, databyte, DCaddr_elem, "BITFIELD", "bitfield_id", &SSMLegacyDefinitionsInterface::rawbyteToBitwiseDCs, codes, titles);
	}
	else if (scaling == dc_addr_dt::Scaling::list)
	{
		rawbyteToAssignmentListDCs(address, databyte, DCaddr_elem, "LIST", "list_id", &SSMLegacyDefinitionsInterface::rawbyteToListDC, codes, titles);
	}
	else
		return rawbyteToSingleSubstitudeDC (databyte, codes, titles);
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


std::vector<XMLElement*> SSMLegacyDefinitionsInterface::getAllMultilevelElements(std::string name)
{
	std::vector<XMLElement*> DCblock_elements;
	std::vector<XMLElement*> DCblock_elements2;
	if (_defs_root_element)
		DCblock_elements = getAllMatchingChildElements(_defs_root_element, name);
	if (_defs_for_id_b1_element)
	{
		DCblock_elements2 = getAllMatchingChildElements(_defs_for_id_b1_element, name);
		DCblock_elements.insert(DCblock_elements.end(), DCblock_elements2.begin(), DCblock_elements2.end());
	}
	if (_defs_for_id_b2_element)
	{
		DCblock_elements2 = getAllMatchingChildElements(_defs_for_id_b2_element, name);
		DCblock_elements.insert(DCblock_elements.end(), DCblock_elements2.begin(), DCblock_elements2.end());
	}
	if (_defs_for_id_b3_element)
	{
		DCblock_elements2 = getAllMatchingChildElements(_defs_for_id_b3_element, name);
		DCblock_elements.insert(DCblock_elements.end(), DCblock_elements2.begin(), DCblock_elements2.end());
	}
	return DCblock_elements;
}


bool SSMLegacyDefinitionsInterface::getMultilevelElementWithHighestPriority(std::string name, XMLElement **element)
{
	*element = NULL;
	std::vector<XMLElement*> elements;
	if (_defs_for_id_b3_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b3_element, name);
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_for_id_b2_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b2_element, name);
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_for_id_b1_element)
	{
		elements = getAllMatchingChildElements(_defs_for_id_b1_element, name);
		if (elements.size() > 1)
			return false;
	}
	if (!elements.size() && _defs_root_element)
		elements = getAllMatchingChildElements(_defs_root_element, name);
	if (elements.size() != 1)
		return false;
	*element = elements.at(0);
	return true;
}


bool SSMLegacyDefinitionsInterface::getAttributeStr(XMLElement *elem, std::string attr_name, std::string *attr_str)
{
	const XMLAttribute *pAttrib = NULL;
	bool has_attr = false;

	attr_str->clear();
	pAttrib = elem->FindAttribute( attr_name.c_str() );
	has_attr = (pAttrib != NULL);
	if (has_attr)
		*attr_str = pAttrib->Value();
	return has_attr;
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


XMLElement* SSMLegacyDefinitionsInterface::searchForMatchingIDelement(XMLElement *parentElement, unsigned char IDbyte_number, char IDbyte_value)
{
	std::string IDelement_name;
	attributeCondition attribCondition;
	std::vector<attributeCondition> attribConditions;
	std::vector<XMLElement*> value_elements;
	std::vector<XMLElement*> value_range_elements;

	IDelement_name = "ID_BYTE" + std::to_string(IDbyte_number);

	// Search for elements with attribute "value":
	attribCondition.name = "value";
	attribCondition.value = "0x" + libFSSM::StrToHexstr(&IDbyte_value, 1);
	attribCondition.condition = attributeCondition::equal;
	value_elements = getAllMatchingChildElements(parentElement, IDelement_name, std::vector<attributeCondition>(1, attribCondition));
	if (value_elements.size() > 1)
		return NULL;
	else if (value_elements.size() == 1)
	{
		// Check if matching element it also contains value_start and/or value_end attributes:
		if ((value_elements.at(0)->FindAttribute("value_start") != NULL) || (value_elements.at(0)->FindAttribute("value_end") != NULL))
			return NULL;
	}

	// Search for elements with attributes "value_start" AND "value_range":
	attribConditions.clear();
	attribCondition.name = "value_start";
	attribCondition.value = "0x" + libFSSM::StrToHexstr(&IDbyte_value, 1);
	attribCondition.condition = attributeCondition::equalOrSmaller;
	attribConditions.push_back(attribCondition);
	attribCondition.name = "value_end";
	attribCondition.condition = attributeCondition::equalOrLarger;
	attribConditions.push_back(attribCondition);
	value_range_elements = getAllMatchingChildElements(parentElement, IDelement_name, attribConditions);
	if (value_range_elements.size() > 1)
		return NULL;
	else if (value_range_elements.size() == 1)
	{
		// Check if matching element it also contains value attribute:
		if (value_range_elements.at(0)->FindAttribute("value") != NULL)
			return NULL;
	}

	// Check if we have separate matching elements (1 element matching with value and 1 element matching with value_start/value_end):
	if (value_elements.size() == value_range_elements.size())
		return NULL;

	// Return matching element:
	if (value_elements.size())
		return value_elements.at(0);
	else
		return value_range_elements.at(0);
}


bool SSMLegacyDefinitionsInterface::getCommonDataElementWithMatchingID(std::string elementName, std::string id, XMLElement **element)
{
	std::vector<XMLElement*> tmp_elements;
	attributeCondition attribCond;

	attribCond.name = "id";
	attribCond.value = id;
	attribCond.condition = attributeCondition::equal;
	tmp_elements = getAllMatchingChildElements(_datacommon_root_element, elementName, std::vector<attributeCondition>(1, attribCond));
	if (tmp_elements.size() != 1)
		return false;
	*element = tmp_elements.at(0);
	return true;
}


bool SSMLegacyDefinitionsInterface::getAddressElementValue(XMLElement *parentElement, unsigned int *address)
{
	std::vector<XMLElement*> tmp_elements;
	const char *str = NULL;

	tmp_elements = getAllMatchingChildElements(parentElement, "ADDRESS");
	if (tmp_elements.size() != 1)
		return false;
	str = tmp_elements.at(0)->GetText();
	if (str == NULL)
		return false;
	unsigned long int addr = strtoul( str, NULL, 0 );
	if (addr > 0xffff)
		return false;
	*address = addr;
	return true;
}


bool SSMLegacyDefinitionsInterface::getBitElementValue(XMLElement *parentElement, unsigned char *bit)
{
	std::vector<XMLElement*> tmp_elements;
	const char *str = NULL;

	tmp_elements = getAllMatchingChildElements(parentElement, "BIT");
	if (tmp_elements.size() != 1)
		return false;
	str = tmp_elements.at(0)->GetText();
	if (str == NULL)
		return false;
	unsigned long int bitval = strtoul( str, NULL, 0 );
	if ((bitval < 1) || (bitval > 8))
		return false;
	*bit = bitval;
	return true;
}


bool SSMLegacyDefinitionsInterface::getLanguageDependentElementString(XMLElement *parent_elem, std::string elem_name, QString *elem_str)
{
	std::vector<XMLElement*> tmp_elements;
	attributeCondition attribCond;
	const char *str = NULL;

	// WARNING: do not overwrite elem_str until element found !
	attribCond.name = "lang";
	attribCond.value = "all";
	attribCond.condition = attributeCondition::equal;
	tmp_elements = getAllMatchingChildElements(parent_elem, elem_name, std::vector<attributeCondition>(1, attribCond));
	if (tmp_elements.size() != 1)
	{
		attribCond.value = _language.toStdString();
		tmp_elements = getAllMatchingChildElements(parent_elem, elem_name, std::vector<attributeCondition>(1, attribCond));
		if ((tmp_elements.size() == 0) && (_language != "en")) // fall back to english language:
		{
			attribCond.value = "en";
			tmp_elements = getAllMatchingChildElements(parent_elem, elem_name, std::vector<attributeCondition>(1, attribCond));
		}
	}

	if (tmp_elements.size() == 1)
	{
		str = tmp_elements.at(0)->GetText();
		if (str != NULL)
			*elem_str = QString( str );
		else
			elem_str->clear();
		return true;
	}

	return false;
}


bool SSMLegacyDefinitionsInterface::getElementString(XMLElement *parent_elem, std::string elem_name, QString *str)
{
	std::vector<XMLElement*> tmp_elements;
	const char *cstr = NULL;

	tmp_elements = getAllMatchingChildElements(parent_elem, elem_name);
	if (tmp_elements.size() != 1)
		return false;
	cstr = tmp_elements.at(0)->GetText();
	if (cstr == NULL)
		return false;
	*str = QString( cstr );
	return true;
}


bool SSMLegacyDefinitionsInterface::getPrecisionElementValue(XMLElement *parent_elem, char *precision)
{
	std::vector<XMLElement*> tmp_elements;
	const char *str = NULL;

	tmp_elements = getAllMatchingChildElements(parent_elem, "PRECISION");
	if (tmp_elements.size() != 1)
		return false;
	str = tmp_elements.at(0)->GetText();
	if (str == NULL)
		return false;
	long int prec = strtol( str, NULL, 0 );
	if ((prec < -128) || (prec > 127))
		return false;
	*precision = prec;
	return true;
}


bool SSMLegacyDefinitionsInterface::getRawValueElementValue(XMLElement *parent_elem, std::string elem_name, unsigned int *rawValue)
{
	std::vector<XMLElement*> tmp_elements;
	const char *str = NULL;

	tmp_elements = getAllMatchingChildElements(parent_elem, elem_name);
	if (tmp_elements.size() != 1)
		return false;
	str = parent_elem->GetText();
	if (str == NULL)
		return false;
	unsigned long int raw_value = strtoul( str, NULL, 0 );
	if (raw_value > 0xff)
		return false;
	*rawValue = raw_value;
	return true;
}


bool SSMLegacyDefinitionsInterface::taggedMeasuringBlocks(std::vector<mb_intl_dt> *mbs, std::string tag_str)
{
	std::vector<XMLElement*> MB_elements;

	if (mbs == NULL)
		return false;
	mbs->clear();
	if (!_id_set)
		return false;
	MB_elements = getAllMultilevelElements("MB");
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
		unsigned int addr;
		if (!getAddressElementValue(MB_elements.at(k), &addr))
			continue;
		// Check for duplicate definitions (addresses):
		bool duplicate = false;
		for (unsigned int m=0; m<mbs->size(); m++)
		{
			if ((addr == mbs->at(m).addrLow) || (addr == mbs->at(m).addrHigh))
			{
				duplicate = true;
				mbs->erase(mbs->begin() + m);
				break;
			}
		}
		if (duplicate)
			continue;
		mb.addrLow = addr;
		mb.addrHigh = MEMORY_ADDRESS_NONE;
		// --- Get common data ---
		// Find MB data:
		XMLElement *MBdata_element = NULL;
		if (!getCommonDataElementWithMatchingID("MB", id, &MBdata_element))
			continue;
		// Check if tag string matches (if passed):
		if (tag_str.size())
		{
			QString str;
			if (!getElementString(MBdata_element, "TAG", &str))
				continue;
			if (tag_str != str.toStdString())
				continue;
		}
		// Get title:
		if (!getLanguageDependentElementString(MBdata_element, "TITLE", &mb.title))
			continue;
		// Get unit:
		if (!getLanguageDependentElementString(MBdata_element, "UNIT", &mb.unit))
			continue;
		// Get formula:
		if (!getElementString(MBdata_element, "FORMULA", &mb.formula))
			continue;
		// Get precision:
		if (!getPrecisionElementValue(MBdata_element, &mb.precision))
			continue;
		// Add MB to the list:
		mbs->push_back(mb);
	}
	return true;
}


bool SSMLegacyDefinitionsInterface::taggedSwitches(std::vector<sw_intl_dt> *sws, std::string tag_str)
{
	std::vector<XMLElement*> SWblock_elements;

	if (sws == NULL)
		return false;
	sws->clear();
	if (!_id_set)
		return false;
	SWblock_elements = getAllMultilevelElements("SWBLOCK");
	for (unsigned int b=0; b<SWblock_elements.size(); b++)
	{
		sw_intl_dt sw;
		std::vector<XMLElement*> tmp_elements;
		// Get block address:
		unsigned int byteaddr;
		if (!getAddressElementValue(SWblock_elements.at(b), &byteaddr))
			continue;
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
			unsigned char bitaddr;
			if (!getBitElementValue(SW_elements.at(k), &bitaddr))
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
			// NOTE: switches with the same address can be defined across multiple SWBLOCKs
			sw.byteAddr = byteaddr;
			sw.bitAddr = bitaddr;
			// --- Get common data ---:
			// Find SW data:
			XMLElement *SWdata_element = NULL;
			if (!getCommonDataElementWithMatchingID("SW", id, &SWdata_element))
				continue;
			// Check if tag string matches (if passed):
			if (tag_str.size())
			{
				QString str;
				if (!getElementString(SWdata_element, "TAG", &str))
					continue;
				if (tag_str != str.toStdString())
					continue;
			}
			// Get title:
			if (!getLanguageDependentElementString(SWdata_element, "TITLE", &sw.title))
				continue;
			// Get unit:
			if (!getLanguageDependentElementString(SWdata_element, "UNIT", &sw.unit))
				continue;
			// Add SW to the list:
			sws->push_back(sw);
		}
	}
	return true;
}


bool SSMLegacyDefinitionsInterface::SWdata_xxxState(sw_stateindication_data_dt *sw_state_data, std::string tag_str)
{
	std::vector<sw_intl_dt> sws;

	if (sw_state_data == NULL)
		return false;
	*sw_state_data = sw_stateindication_data_dt();
	if (!taggedSwitches(&sws, tag_str))
		return false;
	if (sws.size() == 1)
	{
		sw_state_data->addr = sws.at(0).byteAddr;
		sw_state_data->bit = sws.at(0).bitAddr;
		sw_state_data->inverted = sws.at(0).unit.contains('\\');
	}
	return true;
}


void SSMLegacyDefinitionsInterface::rawbyteToAssignmentListDCs(unsigned int address, char databyte, XMLElement *DCaddr_elem,
                                                               std::string assignment_elem_name, std::string assignment_elem_id_name,
                                                               void(SSMLegacyDefinitionsInterface::*rawbyteScalingFcn)(XMLElement*, unsigned int, char, QStringList*, QStringList*),
                                                               QStringList *codes, QStringList *titles)
{
	// NOTE: assignment_elem_name: BITFIELD or LIST; assignment_elem_id_name: bitfield_id, list_id
	bool addr_has_assignment_id = false;
	std::string assignment_list_id_str;
	XMLElement *DCblock_elem = NULL;
	XMLElement *assignment_list_element = NULL;

	// Determine if ADDRESS element has assignment list ID attribute:
	addr_has_assignment_id = getAttributeStr(DCaddr_elem, assignment_elem_id_name, &assignment_list_id_str);

	// Get DCBLOCK element:
	if ((DCaddr_elem->Parent() == NULL) || (DCaddr_elem->Parent()->ToElement() == NULL))
		(this->*rawbyteScalingFcn)(NULL, address, databyte, codes, titles); // set codes and titles of active DCs to generic/default strings
	DCblock_elem = DCaddr_elem->Parent()->ToElement();

	// Get assignment list element:
	assignment_list_element = getAssignmentListElement(DCblock_elem, assignment_elem_name, addr_has_assignment_id, assignment_list_id_str);
	if (assignment_list_element == NULL)
		(this->*rawbyteScalingFcn)(NULL, address, databyte, codes, titles); // set codes and titles of active DCs to generic/default strings
	else
		(this->*rawbyteScalingFcn)(assignment_list_element, address, databyte, codes, titles);
}


void SSMLegacyDefinitionsInterface::rawbyteToBitwiseDCs(XMLElement *bitfield_element, unsigned int address, char databyte, QStringList *codes, QStringList *titles)
{
	// NOTE: bitfield_element=NULL and/or DClist_element=NULL can be passed/used to assign default/substitude DC codes and descriptions
	// NOTE: codes OR titles may be NULL !
	/* NOTE: - for DCs with missing definitions, default strings are assigned (containing address byte + bit in the title)
	 *       - for DCs with ambiguous definitions (e.g. duplicate or incomplete definitions), default string(s) are assigned for the ambiguous parts (code and/or title)
	 *       - for DCs with existing unambiguous definitions, defined strings are assigned unconditionally (empty strings are assigned, too)
	 *         => DCs with empty code string AND empty title string are ignored to be ignored
	 */
	QStringList local_codes;
	QStringList local_titles;

	if (codes != NULL)
		codes->clear();
	if (titles != NULL)
		titles->clear();

	// Set default DC contents for all 8 bits:
	for (unsigned char b = 0; b < 8; b++)
	{
		local_codes.push_back(  "???" );
		QString title = "     ???     (0x" + QString::number(address, 16).toUpper() + " Bit " + QString::number(b + 1) + ")";
		local_titles.push_back(title);
	}

	// Evaluate all "DC" sub-elements and assign DC contents:
	if (bitfield_element != NULL)
	{
		std::vector<XMLElement*> DC_elements;
		DC_elements = getAllMatchingChildElements(bitfield_element, "DC");
		char assignedBits = 0;
		for (unsigned int d = 0; d < DC_elements.size(); d++)
		{
			const char *str = NULL;
			std::vector<XMLElement*> tmp_elements;

			// --- Get DC data (id and assigned bit) ---
			// Get ID:
			str = DC_elements.at(d)->Attribute("id");
			if (str == NULL)
				continue;
			std::string id_attr_str = std::string(str);
			if (!id_attr_str.size())
				continue;
			// Get bit address:
			tmp_elements = getAllMatchingChildElements(DC_elements.at(d), "BIT");
			if (tmp_elements.size() != 1)
				continue;
			str = tmp_elements.at(0)->GetText();
			if (str == NULL)
				continue;
			unsigned long int bitaddr = strtoul( str, NULL, 0 );
			if ((bitaddr < 1) || (bitaddr > 8))
				continue;

			// Check if corresponding bit is set in databyte:
			if (!(databyte & (1 << (bitaddr - 1))))
				continue;

			// Search for duplicate DCs:
			if (assignedBits == (assignedBits | static_cast<char>(1 << (bitaddr-1))))
			{
				// Display DC as UNKNOWN
				QString title = "     ???     (0x" + QString::number(address, 16).toUpper() + " Bit " + QString::number(bitaddr) + ")";
				local_codes.replace(bitaddr - 1, "???");
				local_titles.replace(bitaddr - 1, title);
				continue;
			}

			// --- Get DC content (code and title) from common data ---
			// Find DC data:
			XMLElement *DCdata_element = NULL;
			attributeCondition attribCond;
			attribCond.name = "id";
			attribCond.value = id_attr_str;
			attribCond.condition = attributeCondition::equal;
			tmp_elements = getAllMatchingChildElements(_datacommon_root_element, "DC", std::vector<attributeCondition>(1, attribCond));
			if (tmp_elements.size() != 1)
				continue;
			DCdata_element = tmp_elements.at(0);
			// Mark bit as assigned:
			assignedBits |= static_cast<char>(1 << (bitaddr - 1));
			// Check if DC is supposed to be ignored:
			tmp_elements = getAllMatchingChildElements(DCdata_element, "IGNORE");
			if (tmp_elements.size() > 0)
			{
				local_codes.replace(bitaddr - 1, "");
				local_titles.replace(bitaddr - 1, "");
				continue;
			}
			// Get code:
			QString code;
			if (getDCcodeFromDCdataElement(DCdata_element, &code))
				local_codes.replace(bitaddr - 1, QString( code ));
			// Get title:
			QString title;
			if (getDCtitleFromDCdataElement(DCdata_element, &title))
				local_titles.replace(bitaddr - 1, QString( title ));
		}
	}

	// Remove all DCs whose bit is not set (DC inactive) or whose code AND title are empty (DC shall be ignored):
	for (char bit = 7; bit >= 0; bit--)
	{
		if (!(databyte & static_cast<char>(1 << bit))
		    || (local_codes.at(bit).isEmpty() && local_titles.at(bit).isEmpty()))
		{
			local_codes.removeAt(bit);
			local_titles.removeAt(bit);
		}
	}

	if (codes != NULL)
		*codes = local_codes;
	if (titles != NULL)
		*titles = local_titles;
}


void SSMLegacyDefinitionsInterface::rawbyteToListDC(XMLElement *list_element, unsigned int address, char databyte, QStringList *codes, QStringList *titles)
{
	// NOTE: list_element=NULL and/or DClist_element=NULL can be passed/used to assign default/substitude DC codes and descriptions
	// NOTE: codes OR titles may be NULL !
	/* NOTE: - for DCs with missing definitions, default strings are assigned (containing the address byte in the title)
	 *       - for DCs with ambiguous definitions (e.g. duplicate or incomplete definitions), default string(s) are assigned for the ambiguous parts (code and/or title)
	 *       - for DCs with existing unambiguous definitions, defined strings are assigned unconditionally (empty strings are assigned, too)
	 *         => DCs with empty code string AND empty title string are ignored to be ignored
	 */
	QStringList local_codes;
	QStringList local_titles;

	if (codes != NULL)
		codes->clear();
	if (titles != NULL)
		titles->clear();

	// Set default DC contents:
	QString title = "     ???     (0x" + QString::number(address, 16).toUpper() + ")";
	local_codes.push_back(  "???" );
	local_titles.push_back(title);

	// Evaluate all "DC" sub-elements and assign DC contents:
	if (list_element != NULL)
	{
		std::vector<XMLElement*> DC_elements;
		DC_elements = getAllMatchingChildElements(list_element, "DC");
		bool DC_assigned = false;
		for (unsigned int d = 0; d < DC_elements.size(); d++)
		{
			const char *str = NULL;
			std::vector<XMLElement*> tmp_elements;
			unsigned long int value = 0;

			// --- Get DC data (id and assigned bit) ---
			// Get ID:
			str = DC_elements.at(d)->Attribute("id");
			if (str == NULL)
				continue;
			std::string id_attr_str = std::string(str);
			if (!id_attr_str.size())
				continue;
			// Get list value:
			tmp_elements = getAllMatchingChildElements(DC_elements.at(d), "VALUE");
			if (tmp_elements.size() != 1)
				continue;
			str = tmp_elements.at(0)->GetText();
			if (str == NULL)
				continue;
			value = strtoul( str, NULL, 0 );
			if (value > 255)
				continue;
			// Check if list databyte matches the value:
			if (value != static_cast<unsigned char>(databyte))
				continue;
			// Search for duplicate DCs:
			if (DC_assigned)
			{
				// Display DC as UNKNOWN
				QString title = "     ???     (0x" + QString::number(address, 16).toUpper() + ")";
				local_codes.replace(0, "???");
				local_titles.replace(0, title);
				continue;
			}

			// --- Get DC content (code and title) from common data ---
			// Find DC data:
			XMLElement *DCdata_element = NULL;
			attributeCondition attribCond;
			attribCond.name = "id";
			attribCond.value = id_attr_str;
			attribCond.condition = attributeCondition::equal;
			tmp_elements = getAllMatchingChildElements(_datacommon_root_element, "DC", std::vector<attributeCondition>(1, attribCond));
			if (tmp_elements.size() != 1)
				continue;
			DCdata_element = tmp_elements.at(0);
			// Mark DC as assigned:
			DC_assigned = true;
			// Check if DC is supposed to be ignored:
			tmp_elements = getAllMatchingChildElements(DCdata_element, "IGNORE");
			if (tmp_elements.size() > 0)
			{
				local_codes.clear();
				local_titles.clear();
				continue;
			}
			// Get code:
			QString code;
			if (getDCcodeFromDCdataElement(DCdata_element, &code))
				local_codes.replace(0, code);
			// Get title:
			QString title;
			if (getDCtitleFromDCdataElement(DCdata_element, &title))
				local_titles.replace(0, title);
		}
	}

	if (codes != NULL)
		*codes = local_codes;
	if (titles != NULL)
		*titles = local_titles;
}


XMLElement *SSMLegacyDefinitionsInterface::getDCaddressElementForAddress(unsigned int address)
{
	std::vector <XMLElement*> DCblock_elements;
	XMLElement *DCaddr_elem = NULL;

	DCblock_elements = getAllMultilevelElements("DCBLOCK");
	for (unsigned int b = 0; b < DCblock_elements.size(); b++)
	{
		XMLElement* current_DCblock_element = NULL;
		std::vector<XMLElement*> addr_elements;

		current_DCblock_element = DCblock_elements.at(b);

		// Get all ADDRESS elements in the current DCBLOCK:
		addr_elements = getAllMatchingChildElements(current_DCblock_element, "ADDRESS");

		// Evaluate all ADDRESS elements in the current DCBLOCK:
		for (unsigned int a = 0; a < addr_elements.size(); a++)
		{
			XMLElement *current_addr_element = NULL;
			unsigned int addr_val = 0;
			const char *str = NULL;
			std::string addr_assignmentListID_value;

			// Get address value of the ADDRESS element and check if it matches:
			current_addr_element = addr_elements.at(a);
			str = current_addr_element->GetText();
			if (str == NULL)
				continue;
			addr_val = strtoul( str, NULL, 0 );
			if (addr_val != address)
				continue; // go on with next ADDRESS
			if (DCaddr_elem != NULL) // element already assigned => ambiguous definitions
				return NULL;

			DCaddr_elem = current_addr_element;
		}
	}

	return DCaddr_elem;
}


XMLElement *SSMLegacyDefinitionsInterface::getAssignmentListElement(XMLElement *DCblock_elem, std::string assignmentList_name,
                                                                    bool addr_has_assignmentListID, std::string addr_assignmentListID_value)
{
	XMLElement* assignmentList_elem = NULL;

	std::vector<XMLElement*> assignmentList_elements = getAllMatchingChildElements(DCblock_elem, assignmentList_name);
		/* NOTE: We want ALL elements here;
		 *       We could instead retrieve only the element(s) with the assignment list ID specified
		 *       by the ADDRESS element, but that wouldn't allow us to detect ambiguous definitions.
		 */

	// Evaluate all assignment list elements in the current DCBLOCK:
	if (assignmentList_elements.size() > 0)
	{
		// Search fo matching assignment list:
		for (unsigned int al_el_idx = 0; al_el_idx < assignmentList_elements.size(); al_el_idx++)
		{
			XMLElement* current_assignmentList_element = NULL;
			const XMLAttribute *pAttrib_assignmentListID = NULL;
			bool assignmentList_has_id;
			std::string assignmentListID_value;

			// Determine the assignment lists ID usage and value:
			current_assignmentList_element = assignmentList_elements.at(al_el_idx);
			pAttrib_assignmentListID = current_assignmentList_element->FindAttribute("id");
			assignmentList_has_id = (pAttrib_assignmentListID != NULL);
			if (assignmentList_has_id)
				assignmentListID_value = pAttrib_assignmentListID->Value();

			// Match assignment list element with ADDRESS element:
			if (assignmentList_has_id == addr_has_assignmentListID)
			{
				if (!assignmentList_has_id || ((assignmentList_has_id) && (addr_assignmentListID_value == assignmentListID_value)))
				{
					if (assignmentList_elem != NULL)
						return NULL;
					assignmentList_elem = current_assignmentList_element;
					/* NOTE: We nevertheless continue. Further valid+matching definitons may be defined,
					 *       which would mean that the defintions are invalid / ambiguous.
					 *       We need to detect this case to avoid delivering potentially wrong (unsafe) DC codes+descriptions.
					 */
				}
			}
			else // NOTE: we generally consider scaling invalid / ambiguous, if ADDRESS and assignment list elements with and without index are mixed in any way
				return NULL;
		}
	}
	else // no assignment list defined, so definitions are invalid (incomplete)
		return NULL;

	return assignmentList_elem;
}


bool SSMLegacyDefinitionsInterface::getDCcodeFromDCdataElement(XMLElement* DCdata_element, QString *code)
{
	std::vector<XMLElement*> tmp_elements;
	attributeCondition attribCond;
	const char *str = NULL;

	// Get code:
	tmp_elements = getAllMatchingChildElements(DCdata_element, "CODE");
	if (tmp_elements.size() == 1)
	{
		str = tmp_elements.at(0)->GetText();
		if (str != NULL)
		{
			*code = QString( str );
			return true;
		}
	}
	return false;
}


bool SSMLegacyDefinitionsInterface::getDCtitleFromDCdataElement(XMLElement* DCdata_element, QString *title)
{
	return getLanguageDependentElementString(DCdata_element, "TITLE", title);
}


bool SSMLegacyDefinitionsInterface::scalingAttribStrToScaling(std::string scaling_str, dc_addr_dt::Scaling *scaling)
{
	if (scaling_str == "bitwise")
		*scaling = dc_addr_dt::Scaling::bitwise;
	else if (scaling_str == "list")
		*scaling = dc_addr_dt::Scaling::list;
	else
		return false;

	return true;
}


void SSMLegacyDefinitionsInterface::rawbyteToSingleSubstitudeDC(char databyte, QStringList *codes, QStringList *titles)
{
	if (codes != NULL)
	{
		codes->clear();
		codes->push_back( "     ???     " );
	}
	if (titles != NULL)
	{
		titles->clear();
		titles->push_back( "0x" + QString::number(static_cast<unsigned char>(databyte), 16) + " [raw]" );
	}
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

