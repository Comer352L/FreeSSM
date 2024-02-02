/*
 * CUcontent_Adjustments.cpp - Widget for Control Unit Adjustments
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

#include "CUcontent_Adjustments.h"



ModQDoubleSpinBox::ModQDoubleSpinBox(QWidget * parent) : QDoubleSpinBox(parent)
{
	_DVMbaseValue = 0;
	_discreteValuesMode = false;
	_precCorrVal = 0;
	recalcPrecCorrValue();
}


void ModQDoubleSpinBox::stepBy(int steps)
{
	double cpValue = 0, crValue = 0, newValue = 0; double delta = 0;
	if (_discreteValuesMode)
	{
		// Get current round value:
		crValue = pow(10,-decimals()) * round( value()*pow(10,decimals()) );
		if (singleStep() < pow(10, -decimals()))
		{
			// Calculate new value:
			newValue = crValue + (steps * pow(10, -decimals()));
			newValue = roundToNextDiscreteValue( newValue );
		}
		else
		{
			// Get current precise value:
			cpValue = roundToNextDiscreteValue( value() );
			// Correct nr of steps (if necessary):
			if (steps > 0)
				delta = cpValue - crValue;
			else // < 0 (= 0 ???)
				delta = crValue - cpValue;
			if (delta >= (pow(10, -decimals()) - _precCorrVal))
				steps -= steps/abs(steps);
			// Calculate new value:
			newValue = cpValue + (steps * singleStep());
		}
		// Set new value (automatic round to decimals):
		QDoubleSpinBox::setValue( newValue );
	}
	else
		QDoubleSpinBox::stepBy(steps);
}


void ModQDoubleSpinBox::setDiscreteValuesModeBaseValue(double baseVal)
{
	double crValue = 0;
	_DVMbaseValue = baseVal;
	recalcPrecCorrValue();
	if (_discreteValuesMode)
	{
		crValue = pow(10,-decimals()) * round( value()*pow(10,decimals()) );
		setValue( roundToNextDiscreteValue( crValue ) );
	}
}


void ModQDoubleSpinBox::setDiscreteValuesModeEnabled(bool enable)
{
	double crValue = 0;
	_discreteValuesMode = enable;
	if (_discreteValuesMode)
	{
		crValue = pow(10,-decimals()) * round( value()*pow(10,decimals()) );
		setValue( roundToNextDiscreteValue( crValue ) );
		connect(this, SIGNAL( editingFinished() ), this, SLOT( roundEditedValue() ) );
	}
	else
		disconnect(this, SIGNAL( editingFinished() ), this, SLOT( roundEditedValue() ) );
}


void ModQDoubleSpinBox::roundEditedValue()
{
	if (_discreteValuesMode) // to be sure...
		QDoubleSpinBox::setValue( roundToNextDiscreteValue( value() ) );
	/* NOTE: we don't have to round value() here, because it is already round to decimals() */
}


double ModQDoubleSpinBox::roundToNextDiscreteValue(double inValue)
{
	double d_new = 0;
	double kcorr = 0;
	if (singleStep() != 0)
	{
		if ((inValue >= 0))
			kcorr = _precCorrVal;
		if ((inValue < 0))
			kcorr = - _precCorrVal;
		d_new = singleStep() * round(((inValue - _DVMbaseValue + kcorr) / singleStep())) + _DVMbaseValue;
	}
	else
		d_new = inValue;
	return d_new;
}


void ModQDoubleSpinBox::recalcPrecCorrValue()
{
	int base_decimals = static_cast<int>(-floor(log10(_DVMbaseValue)));
	int singleStep_decimals = static_cast<int>(-floor(log10(singleStep())));
	int corrVal_decimals = 1 + std::max(decimals(), std::max(base_decimals, singleStep_decimals));
	_precCorrVal = pow(10,-corrVal_decimals);
}


void ModQDoubleSpinBox::setDecimals(int prec)
{
	double crValue = 0;
	QDoubleSpinBox::setDecimals(prec);
	recalcPrecCorrValue();
	if (_discreteValuesMode)
	{
		crValue = pow(10,-decimals()) * round( value()*pow(10,decimals()) );
		setValue( roundToNextDiscreteValue( crValue ) );
	}
}


void ModQDoubleSpinBox::setSingleStep(double val)
{
	double crValue = 0;
	QDoubleSpinBox::setSingleStep(val);
	recalcPrecCorrValue();
	if (_discreteValuesMode)
	{
		crValue = pow(10,-decimals()) * round( value()*pow(10,decimals()) );
		setValue( roundToNextDiscreteValue( crValue ) );
	}
}


void ModQDoubleSpinBox::setValue( double val )
{
	if (_discreteValuesMode)
		val = roundToNextDiscreteValue( val );
	QDoubleSpinBox::setValue(val);
}







QIdPushButton::QIdPushButton(const QString text, unsigned int indentifier, QWidget *parent) : QPushButton(text, parent)
{
	_indentifier = indentifier;
	connect (this, SIGNAL( pressed() ), this, SLOT( emitPressed() ));
	connect (this, SIGNAL( released() ), this, SLOT( emitReleased() ));
}


void QIdPushButton::emitPressed()
{
	emit pressed(_indentifier);
}


void QIdPushButton::emitReleased()
{
	emit released(_indentifier);
}






CUcontent_Adjustments::CUcontent_Adjustments(QWidget *parent) : QWidget(parent)
{
	QHeaderView *headerview;
	_SSMPdev = NULL;
	_num_rows_used = 0;
	_num_volatile_adj = 0;
	_num_perment_adj = 0;
	// Setup GUI:
	setupUi(this);
	// Set column widths:
	adjustments_tableWidget->setColumnWidth (1, 88);
	adjustments_tableWidget->setColumnWidth (2, 106);
	adjustments_tableWidget->setColumnWidth (3, 70);
	headerview = adjustments_tableWidget->horizontalHeader();
#if QT_VERSION < 0x050000
	headerview->setResizeMode(0,QHeaderView::Stretch);
	headerview->setResizeMode(1,QHeaderView::Fixed);
	headerview->setResizeMode(2,QHeaderView::Fixed);
	headerview->setResizeMode(3,QHeaderView::Fixed);
#else
	headerview->setSectionResizeMode(0,QHeaderView::Stretch);
	headerview->setSectionResizeMode(1,QHeaderView::Fixed);
	headerview->setSectionResizeMode(2,QHeaderView::Fixed);
	headerview->setSectionResizeMode(3,QHeaderView::Fixed);
#endif
	// Set table row resize behavior:
	headerview = adjustments_tableWidget->verticalHeader();
#if QT_VERSION < 0x050000
	headerview->setResizeMode(QHeaderView::Fixed);
#else
	headerview->setSectionResizeMode(QHeaderView::Fixed);
#endif
	/* NOTE: Current method for calculating ther nr. of needed rows
	 * assumes all rows to have the same constsant height */
	// Install event-filter for adjustments-table:
	adjustments_tableWidget->viewport()->installEventFilter(this);
	// Disable GUI-elements:
	title_label->setEnabled(false);
	adjustments_tableWidget->setEnabled(false);
}


bool CUcontent_Adjustments::setup(SSMprotocol *SSMPdev)
{
	std::vector<unsigned int> rawValues;
	unsigned char k = 0;
	bool ok = false;
	bool calcerror = false;
	std::vector<adjustment_dt> supportedAdjustments;

	_SSMPdev = SSMPdev;
	// Reset data:
	_adjustmentData.clear();
	_num_perment_adj = 0;
	_num_volatile_adj = 0;
	clearTable();
	// Get supported adjustments:
	ok = (_SSMPdev != NULL);
	if (ok)
		ok = _SSMPdev->getSupportedAdjustments(&supportedAdjustments);
	if (ok && !supportedAdjustments.empty())
	{
		// Setup the adjustment adjustment data:
		setupAdjustmentData(supportedAdjustments);
		// Setup adjustments table (without current values):
		setupAdjustmentsTable();
		// Query current adjustment values from CU:
		ok = _SSMPdev->getAllAdjustmentValues(&rawValues);
		if (ok)
		{
			// Scale raw values:
			QString scaledValueString = "";
			for (k = 0; k < supportedAdjustments.size(); k++)
			{
				if (libFSSM::raw2scaled(rawValues.at(k), supportedAdjustments.at(k).formula, supportedAdjustments.at(k).precision, &scaledValueString))
					displayCurrentValue(k, scaledValueString, supportedAdjustments.at(k).unit);
				else
				{
					calcerror = true;
					displayCurrentValue(k, QString::number(rawValues.at(k)), tr("[RAW]"));
				}
			}
		}
	}
	// Enable/Disable GUI-elements:
	title_label->setEnabled( ok );
	adjustments_tableWidget->setEnabled( ok );
	// Check for calculation error(s):
	if (calcerror)
		calculationError(tr("One or more current values couldn't be scaled."));
	// Return result:
	return ok;
}


void CUcontent_Adjustments::setupAdjustmentData(std::vector<adjustment_dt> supportedAdjustments)
{
	AdjustmentData adjData;

	_adjustmentData.clear();
	_num_volatile_adj = 0;
	_num_perment_adj = 0;
	for (size_t i = 0; i < supportedAdjustments.size(); i++)
	{
		adjData.data = supportedAdjustments.at(i);
		adjData.non_numeric = scaledValueAreNonNumeric(supportedAdjustments.at(i).formula);
		adjData.rowIndex = -1; // not yet displayed

		_adjustmentData.push_back(adjData);

		if (supportedAdjustments.at(i).permanent)
			_num_perment_adj++;
		else
			_num_volatile_adj++;
	}
}


bool CUcontent_Adjustments::scaledValueAreNonNumeric(QString formula)
{
	int m = 0;
	QString defstr = "";
	QString svstr = "";
	bool ok = false;

	if (formula.contains('='))
	{
		for (m = 0; m <= formula.count(','); m++)
		{
			// Get next allocation string:
			defstr = formula.section(',', m, m);
			// Get scaled part of the allocation string:
			svstr = defstr.section('=', 1, 1);
			// Try to convert scaled part to double:
			svstr.toDouble(&ok);
			if (!ok)
				return true;
		}
	}
	return false;
}


void CUcontent_Adjustments::clearTable()
{
	// Clear Table:
	_num_rows_used = 0;
	adjustments_tableWidget->clearContents(); // NOTE: table dimensions stay the same
	adjustments_tableWidget->setRowCount(0);
	resizeTableToMinimumRows();
}


void CUcontent_Adjustments::setupAdjustmentsTable()
{
	bool enable = false;
	unsigned char k = 0;
	QTableWidgetItem *tableItem = NULL;
	QPushButton *resetButton = NULL;
	QPixmap leftIcon, rightIcon;
	QPixmap mergedIcon(54, 22);
	bool calcerror = false;

	// Create "Reset"-icon:
	leftIcon.load( QString::fromUtf8(":/icons/oxygen/22x22/go-first.png") );
	mergedIcon.fill(Qt::transparent);
	QPainter painter(&mergedIcon);
	painter.drawTiledPixmap( 0, 0, 22, 22, leftIcon );
	painter.drawTiledPixmap( 32, 0, 22, 22, rightIcon );
	QIcon resetButton_icon( mergedIcon );

	// Clear Table:
	clearTable();

	// Enable/Disable GUI-elements:
	title_label->setEnabled( enable );
	adjustments_tableWidget->setEnabled( enable );

	// Add section with all non-permanent adjustments to the table:
	if (_num_volatile_adj)
	{
		// Add title row:
		addTextToTableRow(tr("Non-Permanent Adjustments:"), _num_rows_used, true, true);
		_num_rows_used++;

		// Add ajustment value rows:
		for (k = 0; k < _adjustmentData.size(); k++)
		{
			if (!_adjustmentData.at(k).data.permanent)
			{
				if (!addAdjustmenValueToTable(_adjustmentData.at(k), k, _num_rows_used))
					calcerror = true; // NOTE: item has nevertheless been added to the table
				_adjustmentData.at(k).rowIndex = _num_rows_used;
				_num_rows_used++;
			}
		}

		// Add information row:
		addTextToTableRow(tr("=> NOTE:") + "   " +
		                  tr("Clearing the ECU's memory or disconnecting from the power supply will reset all these adjustments to default values !"),
				  _num_rows_used, false, true);
		_num_rows_used++;
	}

	// Add section with all permanent adjustments to the table:
	if (_num_perment_adj)
	{
		if (_num_volatile_adj)
		{
			// Empty line:
			if (adjustments_tableWidget->rowCount() < (_num_rows_used + 1))
				adjustments_tableWidget->insertRow(_num_rows_used);
			_num_rows_used++;
		}

		// Add title row:
		addTextToTableRow(tr("Permanent Adjustments:"), _num_rows_used, true, true);
		_num_rows_used++;

		// Add ajustment value rows:
		for (k = 0; k < _adjustmentData.size(); k++)
		{
			if (_adjustmentData.at(k).data.permanent)
			{
				if (!addAdjustmenValueToTable(_adjustmentData.at(k), k, _num_rows_used))
					calcerror = true; // NOTE: item has nevertheless been added to the table
				_adjustmentData.at(k).rowIndex = _num_rows_used;
				_num_rows_used++;
			}
		}

		// Add information row:
		addTextToTableRow(tr("=> NOTE:") + "   " +
		                  tr("These adjustments will survive clearing the ECU's memory and disconnection from the power supply !"),
				  _num_rows_used, false, true);
		_num_rows_used++;
	}

	// Setup "Reset all"-elements:
	if (_adjustmentData.size() > 0)
	{
		// Empty line:
		if (adjustments_tableWidget->rowCount() < (_num_rows_used + 1))
			adjustments_tableWidget->insertRow(_num_rows_used);
		_num_rows_used++;

		if (adjustments_tableWidget->rowCount() < (_num_rows_used + 1))
			adjustments_tableWidget->insertRow(_num_rows_used);

		// Title:
		tableItem = new QTableWidgetItem( tr("Reset all: ") );
		tableItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
		adjustments_tableWidget->setItem(_num_rows_used, 2, tableItem);

		// "Reset all"-button:
		resetButton = new QPushButton(adjustments_tableWidget);
		resetButton->setIcon( resetButton_icon );
		resetButton->setIconSize( QSize(54, 22) );
		connect (resetButton, SIGNAL( released() ), this, SLOT( resetAllAdjustmentValues() ));
		// NOTE: using released() instead of pressed() for buttons as workaround for a Qt-Bug occurring under MS Windows
		adjustments_tableWidget->setCellWidget(_num_rows_used, 3, resetButton);

		_num_rows_used++;
	}

	resizeTableToMinimumRows();

	// Check for calculation error(s):
	if (calcerror)
		calculationError(tr("One or more values will not be adjustable to prevent\nwrong data being written to the Control Unit."));
}


void CUcontent_Adjustments::addTextToTableRow(QString text, int row, bool underline, bool bold)
{
	if (adjustments_tableWidget->rowCount() < (row + 1))
		adjustments_tableWidget->insertRow(row);

	QTableWidgetItem *tableItem = new QTableWidgetItem( text );
	QFont font = tableItem->font();
	font.setUnderline(underline);
	font.setBold(bold);
	tableItem->setFont(font);
	adjustments_tableWidget->setItem(row, 0, tableItem);
	adjustments_tableWidget->setSpan(row, 0, 1, adjustments_tableWidget->columnCount());
}


bool CUcontent_Adjustments::addAdjustmenValueToTable(AdjustmentData adjustment, int adj_index, int row)
{
	bool ok = false;
	QPixmap leftIcon, rightIcon;
	QPixmap mergedIcon(54, 22);
	QTableWidgetItem *tableItem = NULL;
	ModQDoubleSpinBox *spinBox = NULL;
	QComboBox *comboBox = NULL;
	QIdPushButton *saveButton = NULL;

	QStringList selectableScaledValueStr;
	QString helpScaledValueStr = 0;
	double minScaledValue = 0;
	double maxScaledValue = 0;
	double defaultScaledValue = 0;
	double minSingleStepFromPrecision = 0;
	double minSingleStepFromRaw = 0;

	// Create "Save"-icon:
	leftIcon.load( QString::fromUtf8(":/icons/oxygen/22x22/go-next.png") );
	rightIcon.load( QString::fromUtf8(":/icons/oxygen/22x22/drive-harddisk.png") );
	mergedIcon.fill(Qt::transparent);
	QPainter painter(&mergedIcon);
	painter.drawTiledPixmap(0, 0, 22, 22, leftIcon);
	painter.drawTiledPixmap(32, 0, 22, 22, rightIcon);
	QIcon saveButton_icon( mergedIcon );

	// Add table row, if required:
	if (adjustments_tableWidget->rowCount() < (row + 1))
		adjustments_tableWidget->insertRow(row);

	// Title:
	tableItem = new QTableWidgetItem(adjustment.data.title);
	adjustments_tableWidget->setItem(row, 0, tableItem);

	// Current Value:
	tableItem = new QTableWidgetItem( "???" );
	tableItem->setTextAlignment(Qt::AlignCenter);
	adjustments_tableWidget->setItem(row, 1, tableItem);

	// New Value:
	if (adjustment.non_numeric)
	{
		// Get selectable scaled values:
		selectableScaledValueStr.clear();
		getSelectableScaledValueStrings(adjustment.data.formula, &selectableScaledValueStr);
		// Setup and insert selection-Combobox:
		comboBox = new QComboBox();
		comboBox->addItems(selectableScaledValueStr);
		adjustments_tableWidget->setCellWidget(row, 2, comboBox);
	}
	else
	{
		// Calculate and set min/max:
		ok = libFSSM::raw2scaled(adjustment.data.rawMin, adjustment.data.formula, adjustment.data.precision, &helpScaledValueStr);
		if (ok)
			minScaledValue = helpScaledValueStr.toDouble(&ok);
		if (ok)
			ok = libFSSM::raw2scaled(adjustment.data.rawMax, adjustment.data.formula, adjustment.data.precision, &helpScaledValueStr);
		if (ok)
			maxScaledValue = helpScaledValueStr.toDouble(&ok);
		if (ok)
			ok = libFSSM::raw2scaled(adjustment.data.rawDefault, adjustment.data.formula, adjustment.data.precision, &helpScaledValueStr);
		if (ok)
			defaultScaledValue = helpScaledValueStr.toDouble(&ok);
		if (!ok)
			return false; // calculation error

		// Put spinbox into the table:
		spinBox = new ModQDoubleSpinBox();
		adjustments_tableWidget->setCellWidget(row, 2, spinBox);
		/* NOTE: we do this here, because some spinbox functions don't work as expected (Qt-bugs ?) if spinBox is not visible yet */

		// Set adjustable range:
		if (minScaledValue > maxScaledValue)
			spinBox->setRange(maxScaledValue, minScaledValue);
		else
			spinBox->setRange(minScaledValue, maxScaledValue);

		// Calculate and set step size:
		minSingleStepFromPrecision = pow(10, (-1 * adjustment.data.precision));
		minSingleStepFromRaw = (maxScaledValue - minScaledValue) / (adjustment.data.rawMax - adjustment.data.rawMin);
		/* NOTE: this only works for constant step size ! */
		if (minSingleStepFromRaw > minSingleStepFromPrecision)
			spinBox->setSingleStep(minSingleStepFromRaw);
		else
			spinBox->setSingleStep(minSingleStepFromPrecision);

		// Set base value for "discrete values mode":
		spinBox->setDiscreteValuesModeBaseValue(defaultScaledValue);

		// Enable "discrete values mode":
		spinBox->setDiscreteValuesModeEnabled(true);

		// Set decimals:
		spinBox->setDecimals(adjustment.data.precision);

		// Set suffix (unit):
		spinBox->setSuffix(" " + adjustment.data.unit);

		// Set alignement:
		spinBox->setAlignment(Qt::AlignCenter);
	}

	// "Save" button:
	saveButton = new QIdPushButton("", adj_index, adjustments_tableWidget);
	saveButton->setIcon(saveButton_icon);
	saveButton->setIconSize(QSize(54,22));
	connect (saveButton, SIGNAL( released(unsigned int) ), this, SLOT( saveAdjustmentValue(unsigned int) ));
	adjustments_tableWidget->setCellWidget (row, 3, saveButton);

	// Return success:
	return true;
}


void CUcontent_Adjustments::getSelectableScaledValueStrings(QString formulaStr, QStringList *selectableScaledValueStr)
{
	int k = 0;
	QString defstr = "";
	QString svstr = "";
	selectableScaledValueStr->clear();
	if (formulaStr.contains('='))
	{
		for (k=0; k<=formulaStr.count(','); k++)
		{
			// Get next allocation string:
			defstr = formulaStr.section(',', k, k);
			// Get scaled part of the allocation string:
			svstr = defstr.section('=', 1, 1);
			// Add value to the list of selectable scaled values:
			selectableScaledValueStr->append(svstr);
		}
	}
}


void CUcontent_Adjustments::displayCurrentValue(unsigned char adjustment_index, QString currentValueStr, QString unit)
{
	QTableWidgetItem *tableItem;
	QString outputStr = "";
	QDoubleSpinBox *spinbox = NULL;
	QComboBox *combobox = NULL;
	QWidget *cellWidget = NULL;
	double sbvalue = 0;
	bool ok = false;

	if (_adjustmentData.at(adjustment_index).rowIndex < 0) // should not happen
		return;
	if (currentValueStr.isEmpty())
		outputStr = "???";
	else
		outputStr = currentValueStr;
	// Append unit (if available):
	if ( unit != "" )
		outputStr += " " + unit;
	// Display value+unit:
	tableItem = new QTableWidgetItem( outputStr );
	tableItem->setTextAlignment(Qt::AlignCenter);
	adjustments_tableWidget->setItem(_adjustmentData.at(adjustment_index).rowIndex, 1, tableItem);
	// Set value of the selection-elements to the current value:
	if (!currentValueStr.isEmpty())
	{
		cellWidget = adjustments_tableWidget->cellWidget(_adjustmentData.at(adjustment_index).rowIndex, 2);
		if (_adjustmentData.at(adjustment_index).non_numeric)
		{
			combobox = dynamic_cast<QComboBox*>(cellWidget);
			int index = combobox->findText(currentValueStr);
			if (index > -1)
				combobox->setCurrentIndex(index);
		}
		else	// Spinbox
		{
			spinbox = dynamic_cast<ModQDoubleSpinBox*>(cellWidget);
			sbvalue = currentValueStr.toDouble(&ok);
			if (ok)
				spinbox->setValue( sbvalue );
		}
	}
}


void CUcontent_Adjustments::saveAdjustmentValue(unsigned int adjustment_index)
{
	QWidget *cellWidget = NULL;
	QDoubleSpinBox *spinbox = NULL;
	QComboBox *combobox = NULL;
	double newvalue_scaledDouble = 0;
	QString newvalue_scaledStr = "";
	unsigned int newvalue_raw = 0;
	unsigned int controlValue_raw = 0;
	bool ok = false;

	if (!_SSMPdev)
		return;
	if (_adjustmentData.at(adjustment_index).rowIndex < 0) // should not happen
		return;
	// Show wait-message:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Saving adjustment value to Electronic Control Unit... Please wait !"));
	waitmsgbox.show();
	// Get selected Value from table:
	cellWidget = adjustments_tableWidget->cellWidget(_adjustmentData.at(adjustment_index).rowIndex, 2);
	if (_adjustmentData.at(adjustment_index).non_numeric)
	{
		combobox = dynamic_cast<QComboBox*>(cellWidget);
		newvalue_scaledStr = combobox->currentText();
	}
	else	// Spinbox
	{
		spinbox = dynamic_cast<ModQDoubleSpinBox*>(cellWidget);
		newvalue_scaledDouble = spinbox->value();
		newvalue_scaledStr = QString::number(newvalue_scaledDouble, 'f', _adjustmentData.at(adjustment_index).data.precision);
	}
	// Convert scaled value string to raw value:
	if (!libFSSM::scaled2raw(newvalue_scaledStr, _adjustmentData.at(adjustment_index).data.formula, &newvalue_raw))
	{
		calculationError(tr("The new adjustment value couldn't be scaled."));
		return;
	}
	// Save new ajustment value to control unit:
	ok = _SSMPdev->setAdjustmentValue(adjustment_index, newvalue_raw);
	// To be sure: read and verify value again
	if (ok)
		ok = _SSMPdev->getAdjustmentValue(adjustment_index, &controlValue_raw);
	if (!ok)
	{
		communicationError(tr("No or invalid answer from Control Unit."));
		return;
	}
	// Scale and display the current value:
	ok = libFSSM::raw2scaled(controlValue_raw, _adjustmentData.at(adjustment_index).data.formula, _adjustmentData.at(adjustment_index).data.precision, &newvalue_scaledStr);
	if (ok)
		displayCurrentValue(adjustment_index, newvalue_scaledStr, _adjustmentData.at(adjustment_index).data.unit);
	else
		displayCurrentValue(adjustment_index, QString::number(controlValue_raw, 10), tr("[RAW]"));
	// Close wait-messagebox
	waitmsgbox.hide();
	// Check if the CU accepted the new value:
	if (controlValue_raw != newvalue_raw)
		errorMsg(tr("Error"), tr("Error:\nThe Control Unit didn't accept the new value !"));
	// Check for calculation error:
	if (!ok)
		calculationError(tr("The current value couldn't be scaled."));
}


void CUcontent_Adjustments::resetAllAdjustmentValues()
{
	int uc = 0;
	unsigned char k = 0;
	bool calcerror = false;

	if (!_SSMPdev) return;
	// Show "Confirm"-dialog:
	QMessageBox confirmmsg( QMessageBox::Question, tr("Continue ?"), tr("Do you really want to reset all adjustment values ?"), QMessageBox::NoButton, this);
	confirmmsg.addButton(tr("OK"), QMessageBox::AcceptRole);
	confirmmsg.addButton(tr("Cancel"), QMessageBox::RejectRole);
	QFont confirmmsgfont = confirmmsg.font();
	confirmmsgfont.setPointSize(9);
	confirmmsg.setFont( confirmmsgfont );
	confirmmsg.show();
	uc = confirmmsg.exec();
	confirmmsg.close();
	if (uc != QMessageBox::AcceptRole)
		return;
	// Wait-messagebox:
	FSSM_WaitMsgBox waitmsgbox(this, tr("Resetting all adjustment values... Please wait !"));
	waitmsgbox.show();
	// Reset all adjustment values:
	for (k = 0; k < _adjustmentData.size(); k++)
	{
		if (!_SSMPdev->setAdjustmentValue(k, _adjustmentData.at(k).data.rawDefault))
		{
			communicationError(tr("No or invalid answer from Control Unit."));
			return;
		}
	}
	// Scale raw default values and display them:
	QString scaledValueString = "";
	for (k = 0; k < _adjustmentData.size(); k++)
	{
		if (!libFSSM::raw2scaled(_adjustmentData.at(k).data.rawDefault, _adjustmentData.at(k).data.formula, _adjustmentData.at(k).data.precision, &scaledValueString))
			calcerror = true;
		displayCurrentValue(k, scaledValueString, _adjustmentData.at(k).data.unit);
	}
	// Close wait-messagebox:
	waitmsgbox.close();
	// Check for calculation error(s):
	if (calcerror)
		calculationError(tr("One or more values couldn't be scaled."));
}


void CUcontent_Adjustments::resizeTableToMinimumRows()
{
	int rowheight = 0;
	int vspace = 0;
	int maxrowsvisible = 0;
	int minnrofrows = 0;
	// Get available vertical space (for rows) and height per row:
	if (adjustments_tableWidget->rowCount() < 1)
		adjustments_tableWidget->setRowCount(1); // temporary create a row to get the row hight
	rowheight = adjustments_tableWidget->rowHeight(0);
	//vspace = adjustments_tableWidget->viewport()->height(); // NOTE: Sometimes doesn't work as expected ! (Qt-Bug ?)
	vspace = adjustments_tableWidget->height() - adjustments_tableWidget->horizontalHeader()->viewport()->height() - 4;
	// Temporary switch to "Scroll per Pixel"-mode to ensure auto-scroll (prevent white space between bottom of the last row and the lower table border)
	adjustments_tableWidget->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
	// Calculate and set nr. of rows:
	maxrowsvisible = static_cast<unsigned int>(trunc((vspace-1)/rowheight) + 1);
	if (maxrowsvisible < _num_rows_used)
		minnrofrows = _num_rows_used;
	else
		minnrofrows = maxrowsvisible;
	adjustments_tableWidget->setRowCount(minnrofrows);
	// Set vertical scroll bar policy:
	if (minnrofrows > _num_rows_used)
		adjustments_tableWidget->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	else
		adjustments_tableWidget->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	// Switch back to "Scroll per Item"-mode:
	adjustments_tableWidget->setVerticalScrollMode( QAbstractItemView::ScrollPerItem ); // auto-scroll is triggered; Maybe this is a Qt-Bug, we don't want that here...
}


void CUcontent_Adjustments::resizeEvent(QResizeEvent *event)
{
	resizeTableToMinimumRows();
	// Accept event:
	event->accept();
}


bool CUcontent_Adjustments::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == adjustments_tableWidget->viewport())
	{
		if (event->type() == QEvent::Wheel)
		{
			if (adjustments_tableWidget->verticalScrollBarPolicy() ==  Qt::ScrollBarAlwaysOff)
			// ...or minnrofrows > _nrofsupportedAdjustments
				return true;	// filter out
			else
				return false;
		}
	}
	// pass the event on to the parent class
	return QWidget::eventFilter(obj, event);
}


void CUcontent_Adjustments::communicationError(QString errstr)
{
	errstr = tr("Communication Error:") + '\n' + errstr;
	errorMsg(tr("Communication Error"), errstr);
	emit communicationError();
}


void CUcontent_Adjustments::calculationError(QString errstr)
{
	errstr = tr("Calculation Error:") + '\n' + errstr;
	errorMsg(tr("Calculation Error"), errstr);
	emit calculationError();
}


void CUcontent_Adjustments::errorMsg(QString title, QString errstr)
{
	QMessageBox msg( QMessageBox::Critical, title, errstr, QMessageBox::Ok, this);
	QFont msgfont = msg.font();
	msgfont.setPointSize(9);
	msg.setFont( msgfont );
	msg.show();
	msg.exec();
	msg.close();
}

