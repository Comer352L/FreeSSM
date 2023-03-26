/*
 * SSMCUdata.h - Class to manage SYS_ID, ROM_ID and flagbytes
 *
 * Copyright (C) 2016 MartinX, 2018 Comer352L
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

#ifndef SSMCUDATA_H
#define SSMCUDATA_H


#include <vector>


enum class CUtype {Engine, Transmission, CruiseControl, AirCon, FourWheelSteering, ABS, AirSuspension, PowerSteering};


/*!
 * \brief Class to manage SYS_ID, ROM_ID and flagbytes.
 *
 * Holds control unit raw information data and provides related functionality.
 */
class SSMCUdata
{
public:
	static const std::size_t SYS_ID_size = 3;
	static const std::size_t ROM_ID_size = 5;

	SSMCUdata();
	~SSMCUdata();

	/*!
	 * \brief Clear all data.
	 */
	void clear();

	/*!
	 * \brief Parse SSM1 init data consisting of SYS_ID and flagbytes.
	 *
	 * \param data pointer to beginning of data (SYS_ID).
	 * \param size total byte count
	 */
	void from_SSMP1(const char* data, std::size_t size);

	/*!
	 * \brief Parse SSM2 init data consisting of SYS_ID, ROM_ID and flagbytes.
	 *
	 * \param data pointer to beginning of data (SYS_ID).
	 * \param size total byte count
	 */
	void from_SSMP2(const char* data, std::size_t size);

	/*!
	 * \brief get count of flagbytes
	 *
	 * usual SSM2 flagbytes sizes are: 32, 48, 96
	 * \return byte count
	 */
	std::size_t flagbytescount() const { return flagbytes.size(); }

	/*!
	 * \brief Check a specific flagbyte for a specific bit.
	 *
	 * \param byteindex 0-based byte index
	 * \param bitnr bit number in range [0:7]; example: bitnr 5 will check using bitmask 0x20
	 * \return true if flagbyte is available and bit is set, otherwise false
	 */
	bool flagbytebit(std::size_t byteindex, unsigned char bitnr) const;

	bool uses_Flagbytes() const;
	bool uses_Ax10xx_defs() const;

	// C++11: std::array<T, count> var;
	std::vector<char> SYS_ID;
	std::vector<char> ROM_ID;
	std::vector<char> flagbytes;
};


#endif // SSMCUDATA_H
