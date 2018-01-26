/*
 * CUcontent_Adjustments.h - Widget for Control Unit Adjustments
 *
 * Copyright (C) 2008-2018 Comer352L
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

#ifndef CUCONTENT_ADAPTIONS_H
#define CUCONTENT_ADAPTIONS_H



#include <QWidget>
#include <vector>
#include <cmath>		// pow()
#include "ui_CUcontent_Adjustments.h"
#include "SSMprotocol.h"
#include "libFSSM.h"
#include "FSSMdialogs.h"



class ModQDoubleSpinBox : public QDoubleSpinBox
{
	Q_OBJECT

public:
	ModQDoubleSpinBox(QWidget * parent = 0);
	void setDiscreteValuesModeBaseValue(double baseVal);
	void setDiscreteValuesModeEnabled(bool enable);
	void setDecimals ( int prec );
	void setSingleStep ( double val );

private:
	double _DVMbaseValue;
	bool _discreteValuesMode;
	double _precCorrVal;
	void stepBy(int steps);
	double roundToNextDiscreteValue(double inValue);
	void recalcPrecCorrValue();

private slots:
	void roundEditedValue();
	void setValue ( double val );

};
/* NOTE: about QDoubleSpinBox:
 - singleStep my be smaller than decimals !:
	=> the "step-up" and "step-down"-buttons always add the real singleStep-value, so it is possible
	  that the displayed value doesn't change if singleStep < decimals
	=> the precision of value() may be higher than the displayed precision if singleStep < decimals and the values are changed using the up/down-buttons
 - when setting a value with a precission > decimals (e.g. 2 decimals, value = 0.004), the value is rounded to decimals !
*/



class QIdPushButton : public QPushButton
{
	Q_OBJECT

public:
	QIdPushButton(const QString text, unsigned int indentifier, QWidget *parent = 0);

private:
	unsigned int _indentifier;

private slots:
	void emitPressed();
	void emitReleased();

signals:
	void pressed(unsigned int indentifier);
	void released(unsigned int indentifier);
};




class CUcontent_Adjustments : public QWidget, private Ui::Adjustments_Form
{
	Q_OBJECT

public:
	CUcontent_Adjustments(QWidget *parent = 0);
	bool setup(SSMprotocol *SSMPdev);

private:
	SSMprotocol *_SSMPdev;
	std::vector<adjustment_dt> _supportedAdjustments;
	std::vector<bool> _newValueSelWidgetType;
	unsigned int _maxrowsvisible;

	void setupAdjustmentsTable();
	void displayCurrentValue(unsigned char adjustment_index, QString currentValueStr, QString unit);
	void setupNewValueSelWidgetTypes();
	void getSelectableScaledValueStrings(QString formulaStr, QStringList *selectableScaledValueStr);
	void resizeEvent(QResizeEvent *event);
	bool eventFilter(QObject *obj, QEvent *event);
	void communicationError(QString errstr);
	void calculationError(QString errstr);
	void errorMsg(QString title, QString errstr);

private slots:
	void saveAdjustmentValue(unsigned int index);
	void resetAllAdjustmentValues();

signals:
	void communicationError();
	void calculationError();

};


#endif
