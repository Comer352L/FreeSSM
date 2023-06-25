/*
 * SSMLegacyDefinitionsInterface.h - Interface to the SSM legacy definitions
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

#ifndef SSMLEGACYDEFINITIONSINTERFACE_H
#define SSMLEGACYDEFINITIONSINTERFACE_H


#include <string>
#include <vector>
#include <QString>
#include <QStringList>
#include "SSMDefinitionsInterface.h"
#include "tinyxml2/tinyxml2.h"


#define		SSM1_DEFS_FORMAT_VERSION_CURRENT	"0.6.0"
#define		SSM1_DEFS_FORMAT_VERSION_MIN		"0.4.0"


using namespace tinyxml2;


class attributeCondition
{
public:
	enum condition_dt {equal, smaller, larger, equalOrSmaller, equalOrLarger};

	attributeCondition(std::string name="", std::string val="", condition_dt cond=equal)
		: name(name), value(val), condition(cond) {}

	std::string name;
	std::string value;
	condition_dt condition;
};


class SSMLegacyDefinitionsInterface : public SSMDefinitionsInterface
{
public:
	SSMLegacyDefinitionsInterface(QString language = "en");
	~SSMLegacyDefinitionsInterface();

	bool selectDefinitionsFile(std::string filename);
	bool getVersionInfos(std::string *defs_version, std::string *format_version);
	bool selectID(const std::vector<char>& id);

	bool systemDescription(std::string *description);
	bool model(std::string *name);
	bool year(std::string *yearstr);

	bool getDCblockData(std::vector<dc_block_dt> *block_data);
	bool measuringBlocks(std::vector<mb_intl_dt> *mbs);
	bool switches(std::vector<sw_intl_dt> *sws);
	bool adjustments(std::vector<adjustment_intl_dt> *adj);
	bool actuatorTests(std::vector<actuator_dt> *act);
	bool clearMemoryData(unsigned int *address, char *value);
	bool MBdata_engineRunning(mb_enginespeed_data_dt *mb_enginespeed_data);
	bool SWdata_testModeState(sw_stateindication_data_dt *sw_testmode_data);
	bool SWdata_DCheckState(sw_stateindication_data_dt *sw_dcheckactive_data);
	bool SWdata_ignitionState(sw_stateindication_data_dt *sw_ignitionstate_data);

	void getDCcontent(unsigned int address, char databyte, QStringList *codes, QStringList *titles);

private:
	tinyxml2::XMLDocument *_xmldoc; // NOTE: avoid namespace collision with XMLDocument from msxml.h (included by windows.h)
	std::string _filename;
	std::string _defs_version;
	std::string _defs_format_version;
	// Shortcuts to important nodes:
	XMLElement *_defs_root_element;
	XMLElement *_datacommon_root_element;
	XMLElement *_defs_for_id_b1_element;
	XMLElement *_defs_for_id_b2_element;
	XMLElement *_defs_for_id_b3_element;

	std::vector<XMLElement*> getAllMatchingChildElements(XMLElement *pParent, std::string elementName, std::vector<attributeCondition> attribCond=std::vector<attributeCondition>());
	std::vector<XMLElement*> getAllMultilevelElements(std::string name);
	bool getMultilevelElementWithHighestPriority(std::string name, XMLElement **element);
	bool getAttributeStr(XMLElement *elem, std::string attr_name, std::string *attr_str);

	bool checkDefsFormatVersion(std::string version_str);
	bool versionStrToVersionNum(std::string version_str, unsigned long int *version_major, unsigned long int *version_minor, unsigned long int *version_bugfix);

	XMLElement* searchForMatchingIDelement(XMLElement *parentElement, unsigned char IDbyte_number, char IDbyte_value);
	bool getCommonDataElementWithMatchingID(std::string elementName, std::string id, XMLElement **element);
	bool getAddressElementValue(XMLElement *parentElement, unsigned int *address);
	bool getBitElementValue(XMLElement *parentElement, unsigned char *bit);
	bool getLanguageDependentElementString(XMLElement *parent_elem, std::string elem_name, QString *elem_str);
	bool getElementString(XMLElement *parent_elem, std::string elem_name, QString *str);
	bool getPrecisionElementValue(XMLElement *parent_elems, char *precision);
	bool getRawValueElementValue(XMLElement *parent_elem, std::string elem_name, unsigned int *rawValue);

	bool taggedMeasuringBlocks(std::vector<mb_intl_dt> *mbs, std::string tag_str = "");
	bool taggedSwitches(std::vector<sw_intl_dt> *sws, std::string tag_str = "");

	bool SWdata_xxxState(sw_stateindication_data_dt *sw_state_data, std::string tag_str);

	void rawbyteToAssignmentListDCs(unsigned int address, char databyte, XMLElement *DCaddr_elem,
	                                std::string assignment_elem_name, std::string assignment_elem_id_name,
                                        void(SSMLegacyDefinitionsInterface::*rawbyteScalingFcn)(XMLElement*, unsigned int, char, QStringList*, QStringList*),
                                        QStringList *codes, QStringList *titles);
	void rawbyteToBitwiseDCs(XMLElement *bitfield_element, unsigned int address, char databyte, QStringList *codes, QStringList *titles);
	void rawbyteToListDC(XMLElement *list_element, unsigned int address, char databyte, QStringList *codes, QStringList *titles);

	XMLElement *getDCaddressElementForAddress(unsigned int address);
	XMLElement *getAssignmentListElement(XMLElement *DCblock_elem, std::string assignmentList_name, bool addr_has_assignmentListID, std::string addr_assignmentListID_value);
	bool getDCcodeFromDCdataElement(XMLElement* DCdata_element, QString *code);
	bool getDCtitleFromDCdataElement(XMLElement* DCdata_element, QString *title);

	bool scalingAttribStrToScaling(std::string scaling_str, dc_addr_dt::Scaling *scaling);
	void rawbyteToSingleSubstitudeDC (char databyte, QStringList *codes, QStringList *titles);

	bool StrToDouble(std::string mystring, double *d);

};


#endif

