/*
 * CUcontent_LocalIdentifiers.h - Widget for local identifier data
 *
 * Copyright (C) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef CUCONTENT_LOCALIDENTIFIERS_H
#define CUCONTENT_LOCALIDENTIFIERS_H


#include <QtGlobal>
#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "LocalIdentifier.h"
#include "SSMprotocol.h"


class CUcontent_LocalIdentifiers : public QWidget
{
	Q_OBJECT

public:
	CUcontent_LocalIdentifiers(QWidget *parent = 0);
	~CUcontent_LocalIdentifiers();
	bool setup(SSMprotocol *SSMPdev);

signals:
	void error();

private slots:
	void refreshData();

private:
	SSMprotocol *_SSMPdev;
	QVBoxLayout *_sectionsLayout;
	QPushButton *_refreshButton;

	bool readAndDisplayData();
	void clearSections();
	void addSection(const local_identifier_section_dt& section);
	void setupTable(QTableWidget *table);
};


#endif
