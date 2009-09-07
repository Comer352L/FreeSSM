/*
 * SSM1definitionsInterface.cpp - Interface to the SSM1-definitions
 *
 * Copyright (C) 2009 Comer352l
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


SSM1definitionsInterface::SSM1definitionsInterface(char id[3])
{
	_xmldoc = NULL;
	_defs_root_node = NULL;
	_data_root_node = NULL;
	_defs_for_id_b1_node = NULL;
	_defs_for_id_b2_node = NULL;
	_defs_for_id_b3_node = NULL;
	selectID(id);
}


bool SSM1definitionsInterface::selectDefinitionsFile(std::string filename)
{
	_xmldoc = new TiXmlDocument(filename);
	TiXmlNode *root_node;
	std::vector<TiXmlElement*> elements;
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
	// Find and save node FEATURES_DATA
	elements = getAllMatchingChildElements(root_node, "DATA");
	if (elements.size() != 1)
		goto error;
	_data_root_node = elements.at(0);
	// Find and save definitions node for the current ID:
	if (_id_set)
		findIDnodes();
	return true;
	
error:
	delete _xmldoc;
	_xmldoc = NULL;
	_defs_root_node = NULL;
	_data_root_node = NULL;
	_defs_for_id_b1_node = NULL;
	_defs_for_id_b2_node = NULL;
	_defs_for_id_b3_node = NULL;
	return false;
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
	if (!_defs_for_id_b2_node) return false;
	elements = getAllMatchingChildElements(_defs_for_id_b2_node, "SYSDESCRIPTION");
	if (elements.size() != 1) return false;
	*description = elements.at(0)->GetText();
	return true;
}


bool SSM1definitionsInterface::model(std::string *name)
{
	std::vector<TiXmlElement*> elements;
	if (!_defs_for_id_b3_node) return false;
	elements = getAllMatchingChildElements(_defs_for_id_b3_node, "MODELNAME");
	if (elements.size() != 1) return false;
	*name = elements.at(0)->GetText();
	return true;
}


bool SSM1definitionsInterface::year(std::string *yearstr)
{
	std::vector<TiXmlElement*> elements;
	if (!_defs_for_id_b1_node) return false;
	elements = getAllMatchingChildElements(_defs_for_id_b1_node, "YEAR");
	if (elements.size() != 1) return false;
	*yearstr = elements.at(0)->GetText();
	return true;
}


bool SSM1definitionsInterface::clearMemoryAddresses(std::vector<unsigned int> *addr)
{
	std::vector<TiXmlElement*> elements;
	if (!_xmldoc || !_defs_for_id_b3_node) return false;
	elements = getAllMatchingChildElements(_defs_for_id_b3_node, "CLEARMEMORYADDRESS");
	addr->clear();
	for (unsigned int k=0; k<elements.size(); k++)
		addr->push_back( strtoul( elements.at(k)->GetText(), NULL, 0 ) );
	return true;
}


bool SSM1definitionsInterface::diagnosticCodes(std::vector<dc_defs_dt> *dcs)
{
	return false;	// TODO !
}


bool SSM1definitionsInterface::measuringBlocks(std::vector<mb_intl_dt> *mbs)
{
	return false;	// TODO !
}


bool SSM1definitionsInterface::switches(std::vector<sw_intl_dt> *sws)
{
	return false;	// TODO !
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
	if (!_defs_root_node || !_id_set) return;
	attribCondition.name = "value";
	attribCondition.value = "0x" + libFSSM::StrToHexstr(_ID, 1);
	attribCondition.condition = attributeCondition::equal;
	elements = getAllMatchingChildElements(_defs_root_node, "ID_BYTE1", std::vector<attributeCondition>(1,attribCondition));
	if (elements.size() == 1)
	{
		_defs_for_id_b1_node = elements.at(0);
		attribCondition.value = "0x" + libFSSM::StrToHexstr(_ID+1, 1);
		elements = getAllMatchingChildElements(elements.at(0), "ID_BYTE2", std::vector<attributeCondition>(1,attribCondition));
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
	for ( pChild = pParent->FirstChildElement(elementName); pChild != 0; pChild = pChild->NextSibling(elementName)) 
	{
		pElement = pChild->ToElement();
		if (pElement)
		{
			attribsOK = 0;
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
	while (str.size() && (str.at(0)==' '))
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

