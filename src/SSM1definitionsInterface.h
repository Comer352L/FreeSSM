/*
 * SSM1definitionsInterface.h - Interface to the SSM1-definitions
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

#ifndef SSM1DEFINITIONSINTERFACE_H
#define SSM1DEFINITIONSINTERFACE_H


#include <string>
#include <vector>
#include <sstream>
#include "SSMprotocol.h"
#include "tinyxml/tinyxml.h"
#include "libFSSM.h"


class attributeCondition
{
public:
	enum condition_dt {equal, smaller, larger, equalOrSmaller, equalOrLarger};

	attributeCondition(std::string name="", std::string val="", condition_dt cond=equal)
	                                        : name(name), value(val), condition(cond) {};
	
	std::string name;
	std::string value;
	condition_dt condition;
};



class SSM1definitionsInterface
{
public:
	SSM1definitionsInterface(char id[3] = NULL);
	bool selectDefinitionsFile(std::string filename);
	void selectID(char id[3]);

	bool systemDescription(std::string *description);
	bool model(std::string *name);
	bool year(std::string *yearstr);
	
	bool diagnosticCodes(std::vector<dc_defs_dt> *dcs);	// TODO !
	bool measuringBlocks(std::vector<mb_intl_dt> *mbs);	// TODO !
	bool switches(std::vector<sw_intl_dt> *sws);		// TODO !
	bool clearMemoryAddress(unsigned int *addr);
	
private:
	TiXmlDocument *_xmldoc;
	char _ID[3];
	bool _id_set;
	// Shortcuts to important nodes:
	TiXmlNode *_defs_root_node;
	TiXmlNode *_data_root_node;
	TiXmlNode *_defs_for_id_b1_node;
	TiXmlNode *_defs_for_id_b2_node;
	TiXmlNode *_defs_for_id_b3_node;

	void findIDnodes();
	std::vector<TiXmlElement*> getAllMatchingChildElements(TiXmlNode *pParent, std::string elementName, std::vector<attributeCondition> attribCond=std::vector<attributeCondition>());
	bool StrToDouble(std::string mystring, double *d);
};


#endif
