/*
 * CUcontent_MBsSWs_plotView.cpp - Widget for displaying MB/SW values as curve plots
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

#include "CUcontent_MBsSWs_plotView.h"
#include <algorithm>
#include <climits>
#include <cmath>



const size_t CUcontent_MBsSWs_plotArea::MaxSamples = 36000;
const int CUcontent_MBsSWs_plotArea::MinStripHeight = 96;
const int CUcontent_MBsSWs_plotArea::MaxStripHeight = 240;


static double chooseTimeStep(double span, int widthPx)
{
	static const double steps[] = {1, 2, 5, 10, 15, 30, 60, 120, 300, 600, 900, 1800, 3600, 7200};
	const size_t count = sizeof(steps) / sizeof(steps[0]);
	for (size_t k=0; k<count; k++)
	{
		if (steps[k] / span * widthPx >= 70)
			return steps[k];
	}
	return steps[count-1];
}


static void drawPoints(QPainter& painter, std::vector<QPointF>& points)
{
	if (points.size() >= 2)
		painter.drawPolyline(&points[0], static_cast<int>(points.size()));
	else if (points.size() == 1)
		painter.drawPoint(points[0]);
	points.clear();
}


static QString csvQuote(QString text)
{
	text.replace('"', "\"\"");
	return QString("\"") + text + "\"";
}



CUcontent_MBsSWs_plotArea::CUcontent_MBsSWs_plotArea(QWidget *parent) : QWidget(parent)
{
	_newSegment = true;
	_timeSpan = 60;
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
	updateMinimumHeight();
}


void CUcontent_MBsSWs_plotArea::setCurves(const std::vector<BlockType>& types, const std::vector<QString>& titles, const std::vector<QString>& units, const std::vector<unsigned int>& ids)
{
	const size_t count = ids.size();
	std::vector<MBSWplotCurve_dt> newCurves(count);
	// Keep the recorded data if the same MBs/SWs are displayed (e.g. rows have been moved):
	bool sameSet = (count > 0) && (count == _curves.size());
	if (sameSet)
	{
		std::vector<unsigned int> oldIds;
		std::vector<unsigned int> newIds(ids);
		for (const MBSWplotCurve_dt& curve : _curves)
			oldIds.push_back(curve.id);
		std::sort(oldIds.begin(), oldIds.end());
		std::sort(newIds.begin(), newIds.end());
		sameSet = (oldIds == newIds);
	}
	for (size_t row=0; row<count; row++)
	{
		MBSWplotCurve_dt& curve = newCurves.at(row);
		curve.id = ids.at(row);
		if (row < types.size())
			curve.type = types.at(row);
		if (row < titles.size())
			curve.title = titles.at(row);
		if (row < units.size())
			curve.unit = units.at(row);
		if (sameSet)
		{
			for (MBSWplotCurve_dt& oldCurve : _curves)
			{
				if (oldCurve.id == curve.id)
				{
					curve.values.swap(oldCurve.values);
					curve.valueStr = oldCurve.valueStr;
					break;
				}
			}
		}
	}
	_curves.swap(newCurves);
	if (!sameSet)
	{
		_times.clear();
		_segmentStarts.clear();
		_newSegment = true;
	}
	updateMinimumHeight();
	update();
}


void CUcontent_MBsSWs_plotArea::appendValues(const std::vector<double>& values, const std::vector<QString>& valueStrList, const std::vector<QString>& unitStrList)
{
	if (_curves.empty() || (values.size() != _curves.size()))
		return;
	// Make sure all curves are in sync with the time base:
	for (const MBSWplotCurve_dt& curve : _curves)
	{
		if (curve.values.size() != _times.size())
		{
			clearData();
			break;
		}
	}
	if (_times.empty())
		_timer.start();
	_times.push_back(_times.empty() ? 0.0 : static_cast<double>(_timer.elapsed()) / 1000);
	_segmentStarts.push_back(_newSegment);
	_newSegment = false;
	for (size_t row=0; row<_curves.size(); row++)
	{
		MBSWplotCurve_dt& curve = _curves.at(row);
		curve.values.push_back(values.at(row));
		if (row < valueStrList.size())
			curve.valueStr = valueStrList.at(row);
		if (row < unitStrList.size())
			curve.unit = unitStrList.at(row);
	}
	// Limit memory usage by dropping the oldest samples:
	if (_times.size() > MaxSamples)
	{
		const size_t drop = MaxSamples / 10;
		_times.erase(_times.begin(), _times.begin() + drop);
		_segmentStarts.erase(_segmentStarts.begin(), _segmentStarts.begin() + drop);
		_segmentStarts.front() = true;
		for (MBSWplotCurve_dt& curve : _curves)
			curve.values.erase(curve.values.begin(), curve.values.begin() + drop);
	}
	update();
}


void CUcontent_MBsSWs_plotArea::clearData()
{
	_times.clear();
	_segmentStarts.clear();
	for (MBSWplotCurve_dt& curve : _curves)
		curve.values.clear();
	_newSegment = true;
	update();
}


void CUcontent_MBsSWs_plotArea::startNewSegment()
{
	_newSegment = true;
}


void CUcontent_MBsSWs_plotArea::setTimeSpan(double seconds)
{
	_timeSpan = seconds;
	update();
}


bool CUcontent_MBsSWs_plotArea::hasData() const
{
	return !_times.empty();
}


bool CUcontent_MBsSWs_plotArea::writeCSV(QTextStream& out) const
{
	out << "time_s";
	for (const MBSWplotCurve_dt& curve : _curves)
	{
		QString column = curve.title;
		if (!curve.unit.isEmpty())
			column += " [" + curve.unit + "]";
		out << ',' << csvQuote(column);
	}
	out << '\n';
	for (size_t i=0; i<_times.size(); i++)
	{
		out << QString::number(_times.at(i), 'f', 3);
		for (const MBSWplotCurve_dt& curve : _curves)
		{
			out << ',';
			if (i < curve.values.size())
				out << QString::number(curve.values.at(i), 'g', 10);
		}
		out << '\n';
	}
	return (out.status() == QTextStream::Ok);
}


void CUcontent_MBsSWs_plotArea::updateMinimumHeight()
{
	const int axisBand = fontMetrics().height() + 8;
	setMinimumHeight(static_cast<int>(_curves.size()) * MinStripHeight + axisBand);
}


void CUcontent_MBsSWs_plotArea::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	const QColor surface = palette().color(QPalette::Base);
	const QColor ink = palette().color(QPalette::Text);
	const QColor mutedColor = blend(surface, ink, 0.6);
	const QFontMetrics fm = painter.fontMetrics();
	painter.fillRect(rect(), surface);
	const int count = static_cast<int>(_curves.size());
	if (count < 1)
	{
		painter.setPen(mutedColor);
		painter.drawText(rect(), Qt::AlignCenter, CUcontent_MBsSWs_plotView::tr("No Measuring Blocks / Switches selected"));
		return;
	}
	// Geometry:
	const int leftGutter = 60;
	const int rightMargin = 28;
	const int axisBand = fm.height() + 8;
	const int headerHeight = fm.height() + 4;
	const int stripGap = 8;
	int stripHeight = (height() - axisBand) / count;
	stripHeight = std::max(MinStripHeight, std::min(MaxStripHeight, stripHeight));
	const int plotLeft = leftGutter;
	const int plotWidth = width() - rightMargin - plotLeft;
	if (plotWidth < 10)
		return;
	// Visible time window:
	const double tEnd = _times.empty() ? 0.0 : _times.back();
	double tStart = 0;
	double tStop = 0;
	if (_timeSpan > 0)
	{
		tStart = std::max(0.0, tEnd - _timeSpan);
		tStop = tStart + _timeSpan;
	}
	else
		tStop = std::max(tEnd, 1.0);
	// Visible samples (plus one sample before the window, so that the curve enters from the left):
	size_t firstVisible = std::lower_bound(_times.begin(), _times.end(), tStart) - _times.begin();
	if (firstVisible > 0)
		firstVisible--;
	const size_t lastVisible = _times.size();
	// Strips:
	for (int row=0; row<count; row++)
	{
		const MBSWplotCurve_dt& curve = _curves.at(row);
		const int y0 = row * stripHeight;
		const QRect plotRect(plotLeft, y0 + headerHeight, plotWidth, stripHeight - headerHeight - stripGap);
		paintStrip(painter, curve, plotRect, tStart, tStop, firstVisible, lastVisible);
		// Header: title on the left, current value on the right
		QString valueText = curve.valueStr;
		if (!curve.unit.isEmpty())
			valueText += " " + curve.unit;
		const int valueWidth = textWidth(fm, valueText);
		const QRect headerRect(4, y0, width() - 4 - rightMargin, headerHeight);
		painter.setPen(ink);
		painter.drawText(headerRect, Qt::AlignRight | Qt::AlignVCenter, valueText);
		painter.drawText(headerRect, Qt::AlignLeft | Qt::AlignVCenter, fm.elidedText(curve.title, Qt::ElideRight, headerRect.width() - valueWidth - 16));
	}
	// Time axis labels:
	const int bandY = count * stripHeight;
	const double xScale = plotWidth / (tStop - tStart);
	const double xStep = chooseTimeStep(tStop - tStart, plotWidth);
	painter.setPen(mutedColor);
	for (long k = static_cast<long>(std::ceil(tStart / xStep)); k * xStep <= tStop + xStep * 1e-6; k++)
	{
		const int x = plotLeft + qRound((k * xStep - tStart) * xScale);
		painter.drawText(QRect(x - 40, bandY + 3, 80, fm.height()), Qt::AlignHCenter | Qt::AlignTop, timeLabel(k * xStep));
	}
}


void CUcontent_MBsSWs_plotArea::paintStrip(QPainter& painter, const MBSWplotCurve_dt& curve, const QRect& plotRect, double tStart, double tStop, size_t firstVisible, size_t lastVisible) const
{
	const QColor surface = palette().color(QPalette::Base);
	const QColor ink = palette().color(QPalette::Text);
	const bool darkSurface = (surface.lightness() < 128);
	const QColor seriesColor = darkSurface ? QColor(0x39, 0x87, 0xe5) : QColor(0x2a, 0x78, 0xd6);
	const QColor gridColor = blend(surface, ink, 0.12);
	const QColor axisColor = blend(surface, ink, 0.3);
	const QColor mutedColor = blend(surface, ink, 0.6);
	const QFontMetrics fm = painter.fontMetrics();
	const double span = tStop - tStart;
	const double xScale = plotRect.width() / span;
	// Value range of the visible samples:
	double vMin = 0;
	double vMax = 1;
	double yStep = 1;
	if (curve.type == BlockType::SW)
	{
		vMin = -0.25;
		vMax = 1.25;
	}
	else
	{
		bool any = false;
		for (size_t i=firstVisible; (i<lastVisible) && (i<curve.values.size()); i++)
		{
			const double v = curve.values.at(i);
			if (!std::isfinite(v))
				continue;
			if (!any)
			{
				vMin = v;
				vMax = v;
				any = true;
			}
			else
			{
				vMin = std::min(vMin, v);
				vMax = std::max(vMax, v);
			}
		}
		if (any)
		{
			double pad = (vMax - vMin) * 0.08;
			if (pad < 1e-9)
				pad = std::max(std::fabs(vMin) * 0.05, 0.5);
			vMin -= pad;
			vMax += pad;
		}
		yStep = niceStep((vMax - vMin) / 3);
	}
	const double yScale = plotRect.height() / (vMax - vMin);
	painter.setRenderHint(QPainter::Antialiasing, false);
	// Horizontal grid lines and value labels:
	for (long k = static_cast<long>(std::ceil(vMin / yStep)); k * yStep <= vMax; k++)
	{
		const double v = k * yStep;
		const int y = plotRect.bottom() - qRound((v - vMin) * yScale);
		if ((y < plotRect.top()) || (y > plotRect.bottom()))
			continue;
		painter.setPen(gridColor);
		painter.drawLine(plotRect.left(), y, plotRect.right(), y);
		QRect labelRect(0, y - fm.height() / 2, plotRect.left() - 6, fm.height());
		if (labelRect.top() < plotRect.top() - 4)
			labelRect.moveTop(plotRect.top() - 4);
		if (labelRect.bottom() > plotRect.bottom() + 8)
			labelRect.moveBottom(plotRect.bottom() + 8);
		painter.setPen(mutedColor);
		painter.drawText(labelRect, Qt::AlignRight | Qt::AlignVCenter, valueLabel(v, yStep));
	}
	// Vertical grid lines:
	const double xStep = chooseTimeStep(span, plotRect.width());
	painter.setPen(gridColor);
	for (long k = static_cast<long>(std::ceil(tStart / xStep)); k * xStep <= tStop + xStep * 1e-6; k++)
	{
		const int x = plotRect.left() + qRound((k * xStep - tStart) * xScale);
		painter.drawLine(x, plotRect.top(), x, plotRect.bottom());
	}
	// Baseline:
	painter.setPen(axisColor);
	painter.drawLine(plotRect.left(), plotRect.bottom(), plotRect.right(), plotRect.bottom());
	// Curve:
	if (curve.values.empty() || (firstVisible >= lastVisible) || (lastVisible > curve.values.size()))
		return;
	painter.save();
	painter.setClipRect(plotRect.adjusted(0, -1, 1, 1));
	painter.setRenderHint(QPainter::Antialiasing, true);
	QPen pen(seriesColor, 2);
	pen.setCapStyle(Qt::RoundCap);
	pen.setJoinStyle(Qt::RoundJoin);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	const bool decimate = ((lastVisible - firstVisible) > static_cast<size_t>(2 * plotRect.width()));
	std::vector<QPointF> points;
	int column = INT_MIN;
	double columnMin = 0;
	double columnMax = 0;
	for (size_t i=firstVisible; i<lastVisible; i++)
	{
		const double v = curve.values.at(i);
		const bool valid = std::isfinite(v);
		const bool segmentBreak = (i > firstVisible) && _segmentStarts.at(i);
		if (segmentBreak || !valid)
		{
			if (column != INT_MIN)
			{
				points.push_back(QPointF(column, plotRect.bottom() - (columnMin - vMin) * yScale));
				points.push_back(QPointF(column, plotRect.bottom() - (columnMax - vMin) * yScale));
				column = INT_MIN;
			}
			drawPoints(painter, points);
			if (!valid)
				continue;
		}
		const double x = plotRect.left() + (_times.at(i) - tStart) * xScale;
		const double y = plotRect.bottom() - (v - vMin) * yScale;
		if (decimate)
		{
			const int c = qRound(x);
			if (c != column)
			{
				if (column != INT_MIN)
				{
					points.push_back(QPointF(column, plotRect.bottom() - (columnMin - vMin) * yScale));
					points.push_back(QPointF(column, plotRect.bottom() - (columnMax - vMin) * yScale));
				}
				column = c;
				columnMin = v;
				columnMax = v;
			}
			else
			{
				columnMin = std::min(columnMin, v);
				columnMax = std::max(columnMax, v);
			}
		}
		else
		{
			if ((curve.type == BlockType::SW) && !points.empty())
				points.push_back(QPointF(x, points.back().y()));
			points.push_back(QPointF(x, y));
		}
	}
	if (column != INT_MIN)
	{
		points.push_back(QPointF(column, plotRect.bottom() - (columnMin - vMin) * yScale));
		points.push_back(QPointF(column, plotRect.bottom() - (columnMax - vMin) * yScale));
	}
	drawPoints(painter, points);
	// Marker at the latest sample:
	const size_t last = lastVisible - 1;
	const double lastValue = curve.values.at(last);
	if (std::isfinite(lastValue) && (_times.at(last) >= tStart))
	{
		const QPointF center(plotRect.left() + (_times.at(last) - tStart) * xScale, plotRect.bottom() - (lastValue - vMin) * yScale);
		painter.setPen(Qt::NoPen);
		painter.setBrush(surface);
		painter.drawEllipse(center, 6.0, 6.0);
		painter.setBrush(seriesColor);
		painter.drawEllipse(center, 4.0, 4.0);
	}
	painter.restore();
}


QColor CUcontent_MBsSWs_plotArea::blend(const QColor& a, const QColor& b, double ratio)
{
	return QColor(qRound(a.red() + (b.red() - a.red()) * ratio),
		      qRound(a.green() + (b.green() - a.green()) * ratio),
		      qRound(a.blue() + (b.blue() - a.blue()) * ratio));
}


double CUcontent_MBsSWs_plotArea::niceStep(double rawStep)
{
	if (!(rawStep > 0))
		return 1;
	const double base = std::pow(10.0, std::floor(std::log10(rawStep)));
	const double fraction = rawStep / base;
	double nice = 10;
	if (fraction <= 1)
		nice = 1;
	else if (fraction <= 2)
		nice = 2;
	else if (fraction <= 5)
		nice = 5;
	return nice * base;
}


QString CUcontent_MBsSWs_plotArea::timeLabel(double seconds)
{
	const long total = static_cast<long>(std::floor(seconds + 0.5));
	const long hours = total / 3600;
	const long minutes = (total % 3600) / 60;
	const long secs = total % 60;
	QString label = QString::number(minutes) + ":" + QString("%1").arg(secs, 2, 10, QChar('0'));
	if (hours > 0)
		label = QString::number(hours) + ":" + QString("%1").arg(minutes, 2, 10, QChar('0')) + ":" + QString("%1").arg(secs, 2, 10, QChar('0'));
	return label;
}


QString CUcontent_MBsSWs_plotArea::valueLabel(double value, double step)
{
	int decimals = 0;
	if (step < 1)
		decimals = static_cast<int>(std::ceil(-std::log10(step) - 1e-9));
	if (std::fabs(value) < step * 1e-6)
		value = 0;
	return QString::number(value, 'f', decimals);
}


int CUcontent_MBsSWs_plotArea::textWidth(const QFontMetrics& fm, const QString& text)
{
#if QT_VERSION < 0x050B00
	return fm.width(text);
#else
	return fm.horizontalAdvance(text);
#endif
}



CUcontent_MBsSWs_plotView::CUcontent_MBsSWs_plotView(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	_scrollArea = new QScrollArea(this);
	_scrollArea->setWidgetResizable(true);
	_scrollArea->setFrameShape(QFrame::StyledPanel);
	_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_plotArea = new CUcontent_MBsSWs_plotArea();
	_scrollArea->setWidget(_plotArea);
	mainLayout->addWidget(_scrollArea);
	QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->setContentsMargins(0, 8, 0, 0);
	buttonLayout->setSpacing(10);
	QLabel *timeSpanLabel = new QLabel(tr("Time span:"), this);
	_timeSpan_comboBox = new QComboBox(this);
	_timeSpan_comboBox->addItem(tr("10 s"), 10);
	_timeSpan_comboBox->addItem(tr("30 s"), 30);
	_timeSpan_comboBox->addItem(tr("1 min"), 60);
	_timeSpan_comboBox->addItem(tr("2 min"), 120);
	_timeSpan_comboBox->addItem(tr("5 min"), 300);
	_timeSpan_comboBox->addItem(tr("10 min"), 600);
	_timeSpan_comboBox->addItem(tr("30 min"), 1800);
	_timeSpan_comboBox->addItem(tr("All"), 0);
	_timeSpan_comboBox->setCurrentIndex(2);
	_clear_pushButton = new QPushButton(tr(" Clear "), this);
	_clear_pushButton->setMinimumHeight(34);
	_export_pushButton = new QPushButton(tr(" Export CSV... "), this);
	_export_pushButton->setMinimumHeight(34);
	buttonLayout->addWidget(timeSpanLabel);
	buttonLayout->addWidget(_timeSpan_comboBox);
	buttonLayout->addStretch();
	buttonLayout->addWidget(_clear_pushButton);
	buttonLayout->addWidget(_export_pushButton);
	mainLayout->addLayout(buttonLayout);
	setButtonsEnabledState();
	// Connect signals and slots:
	connect( _timeSpan_comboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( timeSpanChanged(int) ) );
	connect( _clear_pushButton, SIGNAL( released() ), this, SLOT( clearButtonPressed() ) );
	connect( _export_pushButton, SIGNAL( released() ), this, SLOT( exportButtonPressed() ) );
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
}


CUcontent_MBsSWs_plotView::~CUcontent_MBsSWs_plotView()
{
	disconnect( _timeSpan_comboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( timeSpanChanged(int) ) );
	disconnect( _clear_pushButton, SIGNAL( released() ), this, SLOT( clearButtonPressed() ) );
	disconnect( _export_pushButton, SIGNAL( released() ), this, SLOT( exportButtonPressed() ) );
}


void CUcontent_MBsSWs_plotView::setMBSWlistContent(const std::vector<BlockType>& types, const std::vector<QString>& titles, const std::vector<QString>& units, const std::vector<unsigned int>& ids)
{
	_plotArea->setCurves(types, titles, units, ids);
	setButtonsEnabledState();
}


void CUcontent_MBsSWs_plotView::updateMBSWvalues(const std::vector<double>& values, const std::vector<QString>& valueStrList, const std::vector<QString>& unitStrList)
{
	_plotArea->appendValues(values, valueStrList, unitStrList);
	setButtonsEnabledState();
}


void CUcontent_MBsSWs_plotView::clearMBSWlistContent()
{
	_plotArea->setCurves(std::vector<BlockType>(), std::vector<QString>(), std::vector<QString>(), std::vector<unsigned int>());
	setButtonsEnabledState();
}


void CUcontent_MBsSWs_plotView::clearPlotData()
{
	_plotArea->clearData();
	setButtonsEnabledState();
}


void CUcontent_MBsSWs_plotView::startNewSegment()
{
	_plotArea->startNewSegment();
}


void CUcontent_MBsSWs_plotView::setButtonsEnabledState()
{
	const bool hasData = _plotArea->hasData();
	_clear_pushButton->setEnabled(hasData);
	_export_pushButton->setEnabled(hasData);
}


void CUcontent_MBsSWs_plotView::timeSpanChanged(int index)
{
	_plotArea->setTimeSpan(_timeSpan_comboBox->itemData(index).toDouble());
}


void CUcontent_MBsSWs_plotView::clearButtonPressed()
{
	clearPlotData();
}


void CUcontent_MBsSWs_plotView::exportButtonPressed()
{
	QString filename = QFileDialog::getSaveFileName(this, tr("Export Plot Data"), QDir::homePath(), tr("CSV files") + " (*.csv)");
	if (filename.isEmpty())
		return;
	if (!filename.endsWith(".csv", Qt::CaseInsensitive))
		filename += ".csv";
	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
	{
		QMessageBox::critical(this, tr("Export Error"), tr("Error: failed to export the plot data:\nCouldn't open the selected file"));
		return;
	}
	QTextStream out(&file);
#if QT_VERSION < 0x060000
	out.setCodec("UTF-8");
#endif
	const bool ok = _plotArea->writeCSV(out);
	out.flush();
	file.close();
	if (!ok || (file.error() != QFile::NoError))
		QMessageBox::critical(this, tr("Export Error"), tr("Error: failed to export the plot data:\nA write error occured"));
}
