/*
 * Languages.h - Supported program languages
 *
 * Copyright (C) 2010 Comer352l
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

#ifndef LANGUAGES_H
#define LANGUAGES_H


#include <QVector>
#include <QStringList>


/* Supported locales (languages) */
static const QVector<QLocale> __supportedLocales = QVector<QLocale>()
	<< QLocale::English
	<< QLocale::German
	<< QLocale::Turkish;
	/* ===> ADD NEW LOCALES HERE <=== */

/* Make language names translatable */
#if 0
QT_TRANSLATE_NOOP("Language", "English")
QT_TRANSLATE_NOOP("Language", "German")
QT_TRANSLATE_NOOP("Language", "Turkish")
/* ===> ADD NEW LANGUAGE NAMES HERE <=== */
#endif


#endif
