/*
 * SSM1definitionsInterface.h - Interface to the SSM1-definitions
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

#ifndef SSM1DEFINITIONSINTERFACE_H
#define SSM1DEFINITIONSINTERFACE_H


#include <string>
#include <vector>
#include <sstream>
#include "SSMprotocol.h"
#include "tinyxml/tinyxml.h"
#include "libFSSM.h"


#define		SSM1_DEFS_FORMAT_VERSION_CURRENT	"0.2.0"
#define		SSM1_DEFS_FORMAT_VERSION_MIN		"0.1.1"


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



class SSM1definitionsInterface
{
public:
	SSM1definitionsInterface(std::string lang = "en");
	bool selectDefinitionsFile(std::string filename);
	void getVersionInfos(std::string *defs_version, std::string *format_version);
	void setLanguage(std::string lang);
	bool selectID(const std::vector<char>& id);

	bool systemDescription(std::string *description);
	bool model(std::string *name);
	bool year(std::string *yearstr);
	
	bool diagnosticCodes(std::vector<dc_defs_dt> *dcs);
	bool measuringBlocks(std::vector<mb_intl_dt> *mbs);
	bool switches(std::vector<sw_intl_dt> *sws);
	bool clearMemoryData(unsigned int *address, char *value);

private:
	TiXmlDocument *_xmldoc;
	std::string _defs_version;
	std::string _defs_format_version;
	std::string _lang;
	std::vector<char> _ID;
	bool _id_set;
	// Shortcuts to important nodes:
	TiXmlNode *_defs_root_node;
	TiXmlNode *_datacommon_root_node;
	TiXmlNode *_defs_for_id_b1_node;
	TiXmlNode *_defs_for_id_b2_node;
	TiXmlNode *_defs_for_id_b3_node;

	std::vector<TiXmlElement*> getAllMatchingChildElements(TiXmlNode *pParent, std::string elementName, std::vector<attributeCondition> attribCond=std::vector<attributeCondition>());
	bool StrToDouble(std::string mystring, double *d);
	bool checkDefsFormatVersion(std::string version_str);
	bool versionStrToVersionNum(std::string version_str, unsigned long int *version_major, unsigned long int *version_minor, unsigned long int *version_bugfix);
};


#endif

