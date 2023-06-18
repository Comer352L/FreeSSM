/*
 * AbstractSSMcommunication.h - Abstract Communication class for the SSM protocol communication
 *
 * Copyright (C) 2023 Comer352L
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

#ifndef ABSTRACTSSMCOMMUNICATION_H
#define ABSTRACTSSMCOMMUNICATION_H


#include <vector>
#include <cstddef>	// NULL
#include <QThread>
#include <QMutex>
#include <QEventLoop>


class AbstractSSMcommunication : public QThread
{
	Q_OBJECT

public:
	AbstractSSMcommunication();
	virtual ~AbstractSSMcommunication();

	void setRetriesOnError(unsigned char retries);

	virtual bool readAddress(unsigned int addr, char * databyte) = 0;
	virtual bool readAddresses(const std::vector<unsigned int>& addr, std::vector<char> * data) = 0;
	virtual bool readAddress_permanent(unsigned int addr, int delay = 0) = 0;
	virtual bool readAddresses_permanent(std::vector<unsigned int> addr, int delay = 0) = 0;
	virtual bool writeAddress(unsigned int addr, char databyte, char *databytewritten = NULL) = 0;
	virtual bool writeAddresses(std::vector<unsigned int> addr, std::vector<char> data, std::vector<char> *databyteswritten = NULL) = 0;
	virtual bool writeAddress_permanent(unsigned int addr, char databyte, int delay = 0) = 0;
	virtual bool writeAddresses_permanent(std::vector<unsigned int> addr, std::vector<char> data, int delay = 0) = 0;

	virtual bool stopCommunication() = 0;

protected:
	unsigned char _errRetries;
	QMutex _mutex;
	QEventLoop _el;
	bool _result;
	bool _abort;
	bool _delay;

	virtual void run() = 0;

signals:
	void receivedData(const std::vector<char>& rawdata, int duration_ms);
	void commError();
};


#endif

