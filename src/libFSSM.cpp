/*
 * libFSSM.cpp - function library for FreeSSM
 *
 * Copyright (C) 2008-2014 Comer352L
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

#include "libFSSM.h"


bool libFSSM::raw2scaled(unsigned int rawValue, QString scaleformula, char precision, QString *scaledValueStr)
{
	bool success = false;
	double scaledValue = 0;

	if ( scaleformula.contains('=') )
	{
		success = raw2scaledByDirectAssociation(rawValue, scaleformula, scaledValueStr);
	}
	else if ( raw2scaledByCalculation( rawValue, scaleformula, &scaledValue ) )
	{
		*scaledValueStr = QString::number(scaledValue, 'f', precision);
		success = true;
	}
	else
		return false;
	return success;
}


bool libFSSM::raw2scaledByDirectAssociation(unsigned int rawValue, QString scaleformula, QString *scaledValueStr)
{
	unsigned int nrofDAs = 0;
	QString defstr = "";
	QString rvstr = "";
	bool da_success = false;
	unsigned int m = 0;

	nrofDAs = 1 + (scaleformula.count(','));
	for (m=0; m<nrofDAs; m++)
	{
		// Get next allocation string:
		defstr = scaleformula.section(',',m,m);
		// Get scaled value part of allocation string:
		rvstr = defstr.section('=', 0, 0);
		// Check if raw value matches allocation string:
		if ( rawValue == rvstr.toUInt() )
		{
			// Get allocated value:
			(*scaledValueStr) = defstr.section('=', 1, 1);
			da_success = true;
			break;
		}
	}
	return da_success;
}


bool libFSSM::raw2scaledByCalculation(unsigned int rawValue, QString scaleformula, double *scaledValue)
{
	if (scaleformula.size() < 1) return false;
	double value_in = rawValue;
	// CHECK IF FORMULA BEGINS WITH A VALID DATA TYPE MODIFIER:
	if (scaleformula.startsWith("s", Qt::CaseInsensitive))
	{
		// VALIDATE BIT SIZE INFORMATION:
		int bitsize = 0;
		if (scaleformula.at(1) == '8')
		{
			bitsize = 8;
			scaleformula.remove(0,2);
		}
		else if (scaleformula.mid(1,2) == "16")
		{
			bitsize = 16;
			scaleformula.remove(0,3);
		}
		else
		{
			return false;
		}
		// INTERPRET AS SIGNED:
		if ( rawValue >= static_cast<unsigned int>(1 << (bitsize - 1)) )	// if MSB is set
			value_in = static_cast<int>(rawValue) - (1 << bitsize); // substract (half of range + 1)
	}
	// SCALE:
	return scale(value_in, scaleformula, false, scaledValue);
}


bool libFSSM::scaled2raw(QString scaledValueStr, QString scaleformula, unsigned int *rawValue)
{
	bool success = false;
	double scaledValue = 0;

	if ( scaleformula.contains('=') )
	{
		success = scaled2rawByDirectAssociation(scaledValueStr, scaleformula, rawValue);
	}
	else
	{
		scaledValue = scaledValueStr.toDouble(&success);
		if (success)
			success = scaled2rawByCalculation(scaledValue, scaleformula, rawValue);
	}
	return success;
}


bool libFSSM::scaled2rawByDirectAssociation(QString scaledValueStr, QString scaleformula, unsigned int *rawValue)
{
	unsigned int nrofDAs = 0;
	QString defstr = "";
	QString svstr = "";
	bool ok = false;
	unsigned int m = 0;

	nrofDAs = 1 + (scaleformula.count(','));
	for (m=0; m<nrofDAs; m++)
	{
		// Get next allocation string:
		defstr = scaleformula.section(',',m,m);
		// Get scaled value part of allocation string:
		svstr = defstr.section('=', 1, 1);
		// Check if raw value matches allocation string:
		if ( scaledValueStr == svstr )
		{
			// Get allocated value:
			(*rawValue) = defstr.section('=', 0, 0).toUInt(&ok, 10);
			break;
		}
	}
	return ok;
}


bool libFSSM::scaled2rawByCalculation(double scaledValue, QString scaleformula, unsigned int *rawValue)
{
	double wval = scaledValue;
	int bitsize = 0;

	if (scaleformula.size() < 1) return false;
	// CHECK IF FORMULA BEGINS WITH A VALID DATA TYPE MODIFIER:
	if (scaleformula.startsWith("s", Qt::CaseInsensitive))
	{
		// VALIDATE BIT SIZE INFORMATION:
		if (scaleformula.at(1) == '8')
		{
			bitsize = 8;
			scaleformula.remove(0,2);
		}
		else if (scaleformula.mid(1,2) == "16")
		{
			bitsize = 16;
			scaleformula.remove(0,3);
		}
		else
		{
			return false;
		}
	}
	// SCALE:
	if (!scale(wval, scaleformula, true, &wval))
		return false;
	// DATA TYPE CONVERSION:
	if (bitsize > 0)
	{
		// CONVERT TO UNSIGNED RAW VALUE:
		if (wval < 0)
			wval += 1 << bitsize;
	}
	// PROCESS/CONVERT/VALIDATE SCALED VALUE:
	wval = round(wval);
	if ((wval < 0) || (wval > 65535))
		return false;
	*rawValue = static_cast<unsigned int>(wval);
	return true;
}


bool libFSSM::scale(double value_in, QString formula, bool inverse, double * value_out)
{
	double value_scaled;
	unsigned char operatorindex[10] = {0,};
	int nrofoperators = 0;
	int calcstep = 0;
	unsigned char opindexnr;
	char opchar = 0;
	unsigned char tmpvaluestrlen = 0;
	QString tmpvaluestr("");
	double tmpvalue = 0;
	bool ok = false;
	// VALIDATE FIRST OPERATOR AND SAVE POSITION:
	if ((formula.at(0) == '+') || (formula.at(0) == '-') || (formula.at(0) == '*') || (formula.at(0) == '/'))
	{
		operatorindex[nrofoperators] = 0;
		nrofoperators++;
	}
	else
		return false;
	// CHECK REST OF THE FORMULA AND GET OPERATOR POSITIONS:
	for (unsigned char charindex=1; charindex<formula.size(); charindex++)
	{
		if ((formula.at(charindex) == '+') || (formula.at(charindex) == '-') || (formula.at(charindex) == '*') || (formula.at(charindex) == '/'))
		{
			// Check for consecutive operators
			if ((charindex - operatorindex[nrofoperators-1]) > 1)
			{
				operatorindex[nrofoperators] = charindex;
				nrofoperators++;
			}
			/* NOTE: No further checks necessary, conversion to double will fail
			 *       - if we have more then two consecutive operators
			 *       - or if the second operator is not a + or - (prefix)
			 */
		}
		else if (!formula.at(charindex).isDigit())
		{
				if (charindex == formula.size()) return false;
				if (!( (formula.at(charindex) == '.') && formula.at(charindex-1).isDigit() && formula.at(charindex+1).isDigit() ))
					return false;
		}
	}
	// DO CALCULATION:
	value_scaled = value_in;
	for (calcstep=0; calcstep<nrofoperators; calcstep++)
	{
		if (!inverse)
			opindexnr = calcstep;
		else
			opindexnr = nrofoperators - 1 - calcstep;
		// EXTRACT NEXT OPERATOR:
		opchar = formula.at( (operatorindex[opindexnr]) ).toLatin1();
		// GET LENGTH OF VALUE STRING:
		if (opindexnr == (nrofoperators - 1))	// IF LAST OPERATION
			tmpvaluestrlen = formula.size() - operatorindex[opindexnr] -1;
		else
			tmpvaluestrlen = operatorindex[opindexnr+1] - operatorindex[opindexnr] -1;
		// CHECK VALUE STRING LENGTH:
		if (tmpvaluestrlen == 0) return false;
		// EXTRACT VALUE STRING AND CONVERT TO DOUBLE:
		tmpvaluestr = formula.mid( operatorindex[opindexnr]+1, tmpvaluestrlen );
		tmpvalue = tmpvaluestr.toDouble( &ok );
		if (!ok || (tmpvalue == 0)) return false;
		// DO CALCUALTION STEP:
		if (!inverse)
		{
			switch (opchar)
			{
				case '+':
					value_scaled += tmpvalue;
					break;
				case '-':
					value_scaled -= tmpvalue;
					break;
				case '*':
					value_scaled *= tmpvalue;
					break;
				case '/':
					value_scaled /= tmpvalue;
					break;
				default:
					return false;
			}
		}
		else
		{
			switch (opchar)
			{
				case '+':
					value_scaled -= tmpvalue;
					break;
				case '-':
					value_scaled += tmpvalue;
					break;
				case '*':
					value_scaled /= tmpvalue;
					break;
				case '/':
					value_scaled *= tmpvalue;
					break;
				default:
					return false;
			}
		}
	}
	*value_out = value_scaled;
	return true;
}


// hexadecimal digits
static char hexdigits[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

static char nibble_hexdigit(const unsigned char value)
{
	return hexdigits[value & 0xF];
}


std::string libFSSM::StrToHexstr(const char* inputstr, const size_t nrbytes)
{
	if (inputstr == NULL)
		return "";
	const char delimiter = ' ';
	const size_t strlength = 3 * nrbytes - 1;
	std::string s(strlength, delimiter);
	for (size_t i = 0, bc = 0; bc < nrbytes; ++bc, i += 2)
	{
		const unsigned char charval = static_cast<unsigned char>(inputstr[bc]);
		s.at(i++) = nibble_hexdigit(charval / 16);
		s.at(i) = nibble_hexdigit(charval % 16);
	}
	return s;
}


std::string libFSSM::StrToHexstr(const std::vector<char>& data)
{
	return StrToHexstr(data.data(), data.size());
}


std::string libFSSM::StrToMultiLineHexstr(const char* const data, const size_t nrbytes, const size_t bytesperline, const std::string& lineprefix)
{
	if (data == NULL || !bytesperline)
		return "";
	// estimate length for performance, not exact, can be slightly larger than necessary
	const size_t strlength = lineprefix.length() * (1 + (nrbytes / bytesperline)) + 3 * nrbytes;

	std::string s (strlength, '\0');
	s.clear();
	for (size_t current_line_bytes, i = 0; (current_line_bytes = std::min(bytesperline, nrbytes - i)) > 0; i += current_line_bytes) {
		s += lineprefix;
		s += StrToHexstr(&data[i], current_line_bytes);
		s += '\n';
	}
	return s;
}


std::string libFSSM::StrToMultiLineHexstr(const std::vector<char>& data, const size_t bytesperline, const std::string& lineprefix)
{
	return StrToMultiLineHexstr(data.data(), data.size(), bytesperline, lineprefix);
}


std::string libFSSM::StrToMultiLineHexstr(const unsigned char* const data, const size_t nrbytes, const size_t bytesperline, const std::string& lineprefix)
{
	return StrToMultiLineHexstr(reinterpret_cast<const char*>(data), nrbytes, bytesperline, lineprefix);
}


std::string libFSSM::StrToMultiLineHexstr(const std::vector<unsigned char>& data, const size_t bytesperline, const std::string& lineprefix)
{
	return StrToMultiLineHexstr(reinterpret_cast<const char*>(data.data()), data.size(), bytesperline, lineprefix);
}


void libFSSM::push_backUInt24BigEndian(std::vector<char>& v, const unsigned int value)
{
	v.push_back(value >> 16);
	v.push_back(value >> 8);
	v.push_back(value);
}


void libFSSM::push_back_UInt32BigEndian(std::vector<char>& v, const unsigned int value)
{
	v.push_back(value >> 24);
	v.push_back(value >> 16);
	v.push_back(value >> 8);
	v.push_back(value);
}


unsigned int libFSSM::parseUInt24BigEndian(const unsigned char* const data)
{
	return data[0] << 16 | data[1] << 8 | data[2];
}


unsigned int libFSSM::parseUInt24BigEndian(const char* const data)
{
	return parseUInt24BigEndian(reinterpret_cast<const unsigned char*>(data));
}


unsigned int libFSSM::parseUInt32BigEndian(const unsigned char* const data)
{
	// gcc -O2 will translate this to a single BSWAP instruction, supported since Intel 486
	return data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
}


unsigned int libFSSM::parseUInt32BigEndian(const char* const data)
{
	return parseUInt32BigEndian(reinterpret_cast<const unsigned char*>(data));
}


char libFSSM::calcchecksum(const char *message, const unsigned int nrofbytes)
{
	unsigned char cs = 0;
	for (unsigned int k=0; k<nrofbytes; k++)
		cs += message[k];
	return static_cast<char>(cs);
}
