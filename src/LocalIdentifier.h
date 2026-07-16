/*
 * LocalIdentifier.h - Display model for KWP/SSM local identifier data
 *
 * Copyright (C) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef LOCALIDENTIFIER_H
#define LOCALIDENTIFIER_H


#include <QString>
#include <QStringList>
#include <vector>


class local_identifier_section_dt
{
public:
	QString title;
	QString rawValue;
	QStringList columnHeaders;
	std::vector<QStringList> rows;
};


#endif
