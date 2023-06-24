/*
 * libFSSM.h - function library for FreeSSM
 *
 * Copyright (C) 2008-2009 Comer352l
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

#ifndef LIBFSSM_H
#define LIBFSSM_H


#include <QString>
#include <string>
#include <vector>
#include <cmath>			// round()


/*!
 * \brief Function library
 */
class libFSSM
{
public:
	static bool raw2scaled(unsigned int rawValue, QString scaleformula, char precision, QString *scaledValueStr);
	static bool scaled2raw(QString scaledValueStr, QString scaleformula, unsigned int *rawValue);
	static std::string StrToHexstr(const char* inputstr, size_t nrbytes);
	static std::string StrToHexstr(const std::vector<char>& data);
	static std::string StrToMultiLineHexstr(const char* data, size_t nrbytes, size_t bytesperline = 16, const std::string& lineprefix = "");
	static std::string StrToMultiLineHexstr(const unsigned char* data, size_t nrbytes, size_t bytesperline = 16, const std::string& lineprefix = "");
	static std::string StrToMultiLineHexstr(const std::vector<char>& data, size_t bytesperline = 16, const std::string& lineprefix = "");
	static std::string StrToMultiLineHexstr(const std::vector<unsigned char>& data, size_t bytesperline = 16, const std::string& lineprefix = "");
	static void push_backUInt24BigEndian(std::vector<char>& v, const unsigned int value);
	static void push_back_UInt32BigEndian(std::vector<char>& v, const unsigned int value);
	static unsigned int parseUInt24BigEndian(const unsigned char* const data);
	static unsigned int parseUInt24BigEndian(const char* const data);
	static unsigned int parseUInt32BigEndian(const unsigned char* const data);
	static unsigned int parseUInt32BigEndian(const char* const data);

	/*!
	 * \brief Calculate SSM2 checksum.
	 *
	 * \param message pointer to data bytes
	 * \param nrofbytes byte count
	 * \return
	 */
	static char calcchecksum(const char *message, unsigned int nrofbytes);

private:
	static bool raw2scaledByCalculation(unsigned int rawValue, QString scaleformula, double *scaledValue);
	static bool raw2scaledByDirectAssociation(unsigned int rawValue, QString scaleformula, QString *scaledValueStr);
	static bool scaled2rawByCalculation(double scaledValue, QString scaleformula, unsigned int *rawValue);
	static bool scaled2rawByDirectAssociation(QString scaledValueStr, QString scaleformula, unsigned int *rawValue);
	static bool scale(double value_in, QString formula, bool inverse, double * value_out);
};


#endif
