/*
 * CUcontent_MBsSWs_plotView.h - Widget for displaying MB/SW values as curve plots
 *
 * Copyright (C) 2026 Jacob Seman
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

#ifndef CUCONTENT_MBSSWS_PLOTVIEW_H
#define CUCONTENT_MBSSWS_PLOTVIEW_H



#include <QtGlobal>	/* required for QT_VERSION */
#if QT_VERSION < 0x050000
	#include <QtGui>
#else
	#include <QtWidgets>
#endif
#include <QElapsedTimer>
#include <vector>
#include "SSMprotocol.h"



class MBSWplotCurve_dt
{
public:
	BlockType type {BlockType::MB};
	unsigned int id {0};
	QString title;
	QString unit;
	QString valueStr;
	std::vector<double> values;
};



class CUcontent_MBsSWs_plotArea : public QWidget
{
public:
	CUcontent_MBsSWs_plotArea(QWidget *parent = nullptr);
	void setCurves(const std::vector<BlockType>& types, const std::vector<QString>& titles, const std::vector<QString>& units, const std::vector<unsigned int>& ids);
	void appendValues(const std::vector<double>& values, const std::vector<QString>& valueStrList, const std::vector<QString>& unitStrList);
	void clearData();
	void startNewSegment();
	void setTimeSpan(double seconds);
	bool hasData() const;
	bool writeCSV(QTextStream& out) const;

private:
	static const size_t MaxSamples;
	static const int MinStripHeight;
	static const int MaxStripHeight;

	std::vector<MBSWplotCurve_dt> _curves;
	std::vector<double> _times;
	std::vector<bool> _segmentStarts;
	QElapsedTimer _timer;
	bool _newSegment;
	double _timeSpan;

	void paintEvent(QPaintEvent *event);
	void updateMinimumHeight();
	void paintStrip(QPainter& painter, const MBSWplotCurve_dt& curve, const QRect& plotRect, double tStart, double tStop, size_t firstVisible, size_t lastVisible) const;
	static QColor blend(const QColor& a, const QColor& b, double ratio);
	static double niceStep(double rawStep);
	static QString timeLabel(double seconds);
	static QString valueLabel(double value, double step);
	static int textWidth(const QFontMetrics& fm, const QString& text);
};



class CUcontent_MBsSWs_plotView : public QWidget
{
	Q_OBJECT

public:
	CUcontent_MBsSWs_plotView(QWidget *parent = nullptr);
	~CUcontent_MBsSWs_plotView();
	void setMBSWlistContent(const std::vector<BlockType>& types, const std::vector<QString>& titles, const std::vector<QString>& units, const std::vector<unsigned int>& ids);
	void updateMBSWvalues(const std::vector<double>& values, const std::vector<QString>& valueStrList, const std::vector<QString>& unitStrList);
	void clearMBSWlistContent();
	void clearPlotData();
	void startNewSegment();

private:
	QScrollArea *_scrollArea;
	CUcontent_MBsSWs_plotArea *_plotArea;
	QComboBox *_timeSpan_comboBox;
	QPushButton *_clear_pushButton;
	QPushButton *_export_pushButton;

	void setButtonsEnabledState();

private slots:
	void timeSpanChanged(int index);
	void clearButtonPressed();
	void exportButtonPressed();

};



#endif
