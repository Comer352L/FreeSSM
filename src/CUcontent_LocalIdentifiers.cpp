/*
 * CUcontent_LocalIdentifiers.cpp - Widget for local identifier data
 *
 * Copyright (C) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "CUcontent_LocalIdentifiers.h"


CUcontent_LocalIdentifiers::CUcontent_LocalIdentifiers(QWidget *parent) : QWidget(parent)
{
	_SSMPdev = NULL;

	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(6);

	_sectionsLayout = new QVBoxLayout();
	_sectionsLayout->setContentsMargins(0, 0, 0, 0);
	_sectionsLayout->setSpacing(6);
	mainLayout->addLayout(_sectionsLayout);

	QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->addStretch();
	_refreshButton = new QPushButton(tr("Refresh"), this);
	_refreshButton->setMinimumSize(90, 34);
	buttonLayout->addWidget(_refreshButton);
	mainLayout->addLayout(buttonLayout);

	connect(_refreshButton, SIGNAL(released()), this, SLOT(refreshData()));
}


CUcontent_LocalIdentifiers::~CUcontent_LocalIdentifiers()
{
	disconnect(_refreshButton, SIGNAL(released()), this, SLOT(refreshData()));
}


bool CUcontent_LocalIdentifiers::setup(SSMprotocol *SSMPdev)
{
	_SSMPdev = SSMPdev;
	if (_SSMPdev == NULL)
		return false;
	return readAndDisplayData();
}


void CUcontent_LocalIdentifiers::refreshData()
{
	if (!readAndDisplayData())
	{
		emit error();
		return;
	}
}


bool CUcontent_LocalIdentifiers::readAndDisplayData()
{
	if (_SSMPdev == NULL)
		return false;

	std::vector<local_identifier_section_dt> sections;
	if (!_SSMPdev->readLocalIdentifierData(&sections))
		return false;

	clearSections();
	for (std::vector<local_identifier_section_dt>::const_iterator it = sections.begin(); it != sections.end(); ++it)
		addSection(*it);
	return true;
}


void CUcontent_LocalIdentifiers::clearSections()
{
	QLayoutItem *item = NULL;
	while ((item = _sectionsLayout->takeAt(0)) != NULL)
	{
		if (item->widget() != NULL)
			delete item->widget();
		delete item;
	}
}


void CUcontent_LocalIdentifiers::addSection(const local_identifier_section_dt& section)
{
	QFont titleFont = font();
	titleFont.setUnderline(true);

	QLabel *titleLabel = new QLabel(section.title, this);
	titleLabel->setFont(titleFont);
	_sectionsLayout->addWidget(titleLabel);

	if (!section.rawValue.isEmpty())
	{
		QLabel *rawLabel = new QLabel(tr("Raw: %1").arg(section.rawValue), this);
		_sectionsLayout->addWidget(rawLabel);
	}

	if (section.rows.empty())
		return;

	int columnCount = section.columnHeaders.size();
	if (columnCount == 0)
		columnCount = section.rows.at(0).size();
	if (columnCount == 0)
		return;

	QTableWidget *table = new QTableWidget(static_cast<int>(section.rows.size()), columnCount, this);
	setupTable(table);
	if (section.columnHeaders.size() == columnCount)
		table->setHorizontalHeaderLabels(section.columnHeaders);
	for (std::size_t row = 0; row < section.rows.size(); row++)
	{
		const QStringList& values = section.rows.at(row);
		for (int column = 0; column < columnCount; column++)
		{
			QTableWidgetItem *item = new QTableWidgetItem();
			if (column == 0)
				item->setTextAlignment(Qt::AlignCenter);
			item->setText((column < values.size()) ? values.at(column) : "");
			table->setItem(static_cast<int>(row), column, item);
		}
	}
	_sectionsLayout->addWidget(table);
}


void CUcontent_LocalIdentifiers::setupTable(QTableWidget *table)
{
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->setAlternatingRowColors(true);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->verticalHeader()->hide();
	table->setColumnWidth(0, 70);
#if QT_VERSION < 0x050000
	table->horizontalHeader()->setResizeMode(0, QHeaderView::Interactive);
	for (int column = 1; column < table->columnCount(); column++)
		table->horizontalHeader()->setResizeMode(column, QHeaderView::Stretch);
	table->verticalHeader()->setResizeMode(QHeaderView::Fixed);
#else
	table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
	for (int column = 1; column < table->columnCount(); column++)
		table->horizontalHeader()->setSectionResizeMode(column, QHeaderView::Stretch);
	table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
#endif
}
