/*
 * CUcontent_MBsSWs.cpp - Widget for Reading of Measuring Blocks and Switches
 *
 * Copyright (C) 2008-2009 Comer352l
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

#include "CUcontent_MBsSWs.h"



CUcontent_MBsSWs::CUcontent_MBsSWs(QWidget *parent, SSMprotocol2 *SSMP2dev, bool timemode) : QWidget(parent)
{
	_SSMP2dev = SSMP2dev;
	_supportedMBs.clear();
	_supportedSWs.clear();
	_MBSWmetaList.clear();
	_timemode = timemode;
	_lastrefreshduration_ms = 0;
	_lastValues.clear();
	_minmaxData.clear();
	_rawValueIndexes.clear();

	// Setup GUI:
	setupUi(this);
	setupTimeModeUiElements();
	setupUiFonts();
	_valuesTableView = new CUcontent_MBsSWs_tableView(MBSWviews_tabWidget->widget(0));
	valuesTableView_gridLayout->addWidget(_valuesTableView);
	//_curvesTableView = new ...
	//curvesView_gridLayout->addWidget();
	// Disable all GUI-elements:
	_MBSWrefreshTimeTitle_label->setEnabled( false );
	_MBSWrefreshTimeValue_label->setEnabled( false );
	_timemode_pushButton->setEnabled( false );
	startstopmbreading_pushButton->setEnabled( false );
	mbswadd_pushButton->setEnabled( false );
	mbswdelete_pushButton->setEnabled( false );
	MBSWviews_tabWidget->setTabEnabled(1, false);
	_valuesTableView->setEnabled(false);
	// Set content of time refresh-time labels:
	if (_timemode)
		_MBSWrefreshTimeTitle_label->setText(tr("Block transfer rate:   "));
	else
		_MBSWrefreshTimeTitle_label->setText(tr("Refresh duration:"));
	_MBSWrefreshTimeValue_label->setText("---      ");
	// Connect signals and slots:
	connect( startstopmbreading_pushButton , SIGNAL( released() ), this, SLOT( startstopMBsSWsButtonPressed() ) ); 
	connect( mbswadd_pushButton , SIGNAL( released() ), this, SLOT( addMBsSWs() ) );
	connect( mbswdelete_pushButton , SIGNAL( released() ), this, SLOT( deleteMBsSWs() ) );
	connect( _valuesTableView , SIGNAL( moveUpButton_pressed() ), this, SLOT( moveupMBsSWs() ) );
	connect( _valuesTableView , SIGNAL( moveDownButton_pressed() ), this, SLOT( movedownMBsSWs() ) );
	connect( _valuesTableView , SIGNAL( resetMinMaxButton_pressed() ), this, SLOT( resetMinMax() ) );
	connect( _valuesTableView , SIGNAL( itemSelectionChanged() ), this, SLOT( setDeleteButtonEnabledState() ) );
	connect( _timemode_pushButton , SIGNAL( released() ), this, SLOT( switchTimeMode() ) );
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
}


CUcontent_MBsSWs::~CUcontent_MBsSWs()
{
	_SSMP2dev->stopMBSWreading();
	disconnect( _SSMP2dev, SIGNAL( newMBSWrawValues(std::vector<unsigned int>, int) ), this, SLOT( processMBSWRawValues(std::vector<unsigned int>, int) ) );
	disconnect( startstopmbreading_pushButton , SIGNAL( released() ), this, SLOT( startstopMBsSWsButtonPressed() ) ); 
	disconnect( mbswadd_pushButton , SIGNAL( released() ), this, SLOT( addMBsSWs() ) );
	disconnect( mbswdelete_pushButton , SIGNAL( released() ), this, SLOT( deleteMBsSWs() ) );
	disconnect( _valuesTableView , SIGNAL( moveUpButton_pressed() ), this, SLOT( moveupMBsSWs() ) );
	disconnect( _valuesTableView , SIGNAL( moveDownButton_pressed() ), this, SLOT( movedownMBsSWs() ) );
	disconnect( _valuesTableView , SIGNAL( resetMinMaxButton_pressed() ), this, SLOT( resetMinMax() ) );
	disconnect( _valuesTableView , SIGNAL( itemSelectionChanged() ), this, SLOT( setDeleteButtonEnabledState() ) );
	disconnect( _timemode_pushButton , SIGNAL(released() ), this, SLOT( switchTimeMode() ) );
	disconnect( _SSMP2dev , SIGNAL( stoppedMBSWreading() ), this, SLOT( callStop() ) );
	disconnect( _SSMP2dev , SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ) );
	delete _MBSWrefreshTimeTitle_label;
	delete _MBSWrefreshTimeValue_label;
	delete _timemode_pushButton;
	delete _valuesTableView;
}


bool CUcontent_MBsSWs::setup()
{
	bool ok;
	// Get supported MBs/SWs:
	ok = _SSMP2dev->getSupportedMBs(&_supportedMBs);
	if (ok)
		ok = _SSMP2dev->getSupportedSWs(&_supportedSWs);
	if (!ok)
	{
		// Reset CU-data:
		_supportedMBs.clear();
		_supportedSWs.clear();
	}
	_MBSWmetaList.clear();
	_rawValueIndexes.clear();
	_lastValues.clear();
	_minmaxData.clear();
	// Reset refresh time:
	_lastrefreshduration_ms = 0;
	_MBSWrefreshTimeValue_label->setText("---      ");
	// Output titles and units of the selcted MBs/SWs
	displayMBsSWs();
	// *** Enable/Disable all GUI-elements:
	// Labels + tables:
	_MBSWrefreshTimeTitle_label->setEnabled( ok );
	_MBSWrefreshTimeValue_label->setEnabled( ok );
	_valuesTableView->setEnabled( ok );
	// Values table view widget:
	_timemode_pushButton->setEnabled( ok );
	// Disable "Add"-button, if all supported MBs/SWs are already selected:
	if (_MBSWmetaList.size() < (_supportedMBs.size()+_supportedSWs.size()))
		mbswadd_pushButton->setEnabled(true);
	else
		mbswadd_pushButton->setEnabled(false);
	// Enable/disable "Delete"-button:
	setDeleteButtonEnabledState();
	// Disable "Start"-button:
	startstopmbreading_pushButton->setEnabled(false);
	// Return result:
	return ok;
}


bool CUcontent_MBsSWs::setMBSWselection(std::vector<MBSWmetadata_dt> MBSWmetaList)
{
	unsigned int k = 0;
	// Check if MBSW-reading (and monitoring !) is in progress:
	if ((mbswadd_pushButton->isEnabled() == false) && (_MBSWmetaList.size() < (_supportedMBs.size() + _supportedSWs.size())))
		return false;
	// Check if the selected MBs/SWs are available:
	for (k=0; k<MBSWmetaList.size(); k++)
	{
		if (MBSWmetaList.at(k).blockType == 0)
		{
			if (MBSWmetaList.at(k).nativeIndex > (_supportedMBs.size()-1))
				return false;
		}
		else
		{
			if (MBSWmetaList.at(k).nativeIndex > (_supportedSWs.size()-1))
				return false;
		}
	}
	// Save MB/SW-list:
	_MBSWmetaList = MBSWmetaList;
	// Clear last values:
	_lastValues.clear();
	_minmaxData.clear();
	// Update MB/SW table content:
	displayMBsSWs();
	// Clear time information:
	_MBSWrefreshTimeValue_label->setText("---      ");
	// Activate/deactivate buttons:
	if (_MBSWmetaList.size() > 0)
	{
		startstopmbreading_pushButton->setEnabled(true);
		mbswdelete_pushButton->setEnabled(true);
		connect(_SSMP2dev, SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ));
	}
	else
		disconnect(_SSMP2dev, SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ));
	if (_MBSWmetaList.size() >= (_supportedMBs.size() + _supportedSWs.size()))
		mbswadd_pushButton->setEnabled(false);	// "Add"-button aktivieren
	return true;
}


void CUcontent_MBsSWs::displayMBsSWs()
{
	QStringList titles;
	QStringList minvalues;
	QStringList values;
	QStringList maxvalues;
	QStringList units;
	unsigned int k=0;
	for (k=0; k<_MBSWmetaList.size(); k++)
	{
		// Title:
		if (_MBSWmetaList.at(k).blockType == 0)	// MB
				titles.append( _supportedMBs.at(_MBSWmetaList.at(k).nativeIndex).title );
		else	// SW
				titles.append( _supportedSWs.at(_MBSWmetaList.at(k).nativeIndex).title );
		// Value and unit strings:
		// NOTE: _lastValues can be empty !
		if (static_cast<unsigned int>(_lastValues.size()) > k)
		{
			if (_lastValues.at(k).scaledStr.isEmpty())
				values.append( QString::number(_lastValues.at(k).rawValue) );
			else
				values.append( _lastValues.at(k).scaledStr );
			units.append( _lastValues.at(k).unitStr );
		}
		else
		{
			values.append( "" );
			if (_MBSWmetaList.at(k).blockType == 0)	// MB
				units.append( _supportedMBs.at(_MBSWmetaList.at(k).nativeIndex).unit );
			else // SW
				units.append( "" );
		}
		// Last min/max value strings:
		// NOTE: _minmaxData can be empty !
		if (static_cast<unsigned int>(_minmaxData.size()) > k)
		{
			if (!_minmaxData.at(k).disabled)
			{
				if (_minmaxData.at(k).minScaledValueStr.isEmpty())
					minvalues.append( QString::number(_minmaxData.at(k).minRawValue) );
				else
					minvalues.append( _minmaxData.at(k).maxScaledValueStr );
				if (_minmaxData.at(k).maxScaledValueStr.isEmpty())
					maxvalues.append( QString::number(_minmaxData.at(k).minRawValue) );
				else
					maxvalues.append( _minmaxData.at(k).maxScaledValueStr );
			}
			else
			{
				maxvalues.append( "" );
				maxvalues.append( "" );
			}
		}
		else
		{
			minvalues.append("");
			maxvalues.append("");
		}
	}
	// Display MBs/SWs
	_valuesTableView->setMWSWlistContent(titles, values, minvalues, maxvalues, units);
}


void CUcontent_MBsSWs::startstopMBsSWsButtonPressed()
{
	if (_SSMP2dev->state() == SSMprotocol2::state_MBSWreading)
		callStop();
	else
		callStart();
}


void CUcontent_MBsSWs::callStart()
{
	if (!startMBSWreading())
		communicationError(tr("=> Couldn't stop Measuring Blocks Reading."));
}


void CUcontent_MBsSWs::callStop()
{
	if (!stopMBSWreading())
		communicationError(tr("=> Couldn't start Measuring Blocks Reading."));
}


bool CUcontent_MBsSWs::startMBSWreading()
{
	SSMprotocol2::state_dt state = SSMprotocol2::state_needSetup;
	std::vector<MBSWmetadata_dt> usedMBSWmetaList;
	unsigned int k = 0;
	bool consistent = true;
	// Check premises:
	state = _SSMP2dev->state();
	if (state == SSMprotocol2::state_normal)
	{
		if (_MBSWmetaList.empty()) return false;
		disconnect( _SSMP2dev, SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ) );
		// Start MB/SW-reading:
		if (!_SSMP2dev->startMBSWreading(_MBSWmetaList))
		{
			connect( _SSMP2dev, SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ) );
			return false;
		}
	}
	else if (state == SSMprotocol2::state_MBSWreading)
	{
		// Verify consistency:
		if (!_SSMP2dev->getLastMBSWselection(&usedMBSWmetaList))
			return false;
		if (_MBSWmetaList.size() == usedMBSWmetaList.size())
		{
			for (k=0; k<_MBSWmetaList.size(); k++)
			{
				if ((_MBSWmetaList.at(k).blockType != usedMBSWmetaList.at(k).blockType) || (_MBSWmetaList.at(k).nativeIndex != usedMBSWmetaList.at(k).nativeIndex))
				{
					consistent = false;
					break;
				}
			}
		}
		if (!consistent)	// inconsistency detected !
		{
			// Stop MBSW-reading:
			stopMBSWreading();
			return false;
		}
	}
	else
		return false;
	// Reset/Setup list with the raw value indexes:
	_rawValueIndexes.clear();
	for (k=0; k<_MBSWmetaList.size(); k++)
		_rawValueIndexes.push_back(k);
	// Connect signals and slots:
	connect( _SSMP2dev, SIGNAL( newMBSWrawValues(std::vector<unsigned int>, int) ), this, SLOT( processMBSWRawValues(std::vector<unsigned int>, int) ) );
	connect( _SSMP2dev , SIGNAL( stoppedMBSWreading() ), this, SLOT( callStop() ) );
	// Disable add/delete buttons:
	mbswdelete_pushButton->setEnabled(false);
	mbswadd_pushButton->setEnabled(false);
	// Set text+icon of start/stop-button:
	startstopmbreading_pushButton->setText(tr(" Stop  "));
	QIcon startstopmbreadingicon(QString::fromUtf8(":/icons/chrystal/32x32/player_stop.png"));
	QSize startstopmbreadingiconsize(24,24);
	startstopmbreading_pushButton->setIcon(startstopmbreadingicon);
	startstopmbreading_pushButton->setIconSize(startstopmbreadingiconsize);
	return true;
}


bool CUcontent_MBsSWs::stopMBSWreading()
{
	disconnect( _SSMP2dev , SIGNAL( stoppedMBSWreading() ), this, SLOT( callStop() ) ); // must be disconnected before stopMBSWreading is called
	if (!_SSMP2dev->stopMBSWreading())
	{
		connect( _SSMP2dev , SIGNAL( stoppedMBSWreading() ), this, SLOT( callStop() ) ); // must be disconnected before stopMBSWreading is called
		return false;
	}
	disconnect( _SSMP2dev, SIGNAL( newMBSWrawValues(std::vector<unsigned int>, int) ), this, SLOT( processMBSWRawValues(std::vector<unsigned int>, int) ) );
	connect( _SSMP2dev, SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ) );
	// Clear list with the raw value indexes:
	_rawValueIndexes.clear();
	// set text+icon of start/stop-button:
	startstopmbreading_pushButton->setText(tr(" Start  "));
	QIcon startstopmbreadingicon(QString::fromUtf8(":/icons/chrystal/32x32/player_play.png"));
	QSize startstopmbreadingiconsize(24,24);
	startstopmbreading_pushButton->setIcon(startstopmbreadingicon);
	startstopmbreading_pushButton->setIconSize(startstopmbreadingiconsize);
	// Enable add/delete button (depending on list content and selection):
	setDeleteButtonEnabledState();
	if (_MBSWmetaList.size() < (_supportedMBs.size() + _supportedSWs.size()))
		mbswadd_pushButton->setEnabled(true);
	return true;
}


void CUcontent_MBsSWs::processMBSWRawValues(std::vector<unsigned int> rawValues, int refreshduration_ms)
{
	QString defstr;
	QString rvstr;
	bool scalingSuccessful = false;
	QString scaledValueStr;
	int k = 0;
	// Min/Max comparison:
	bool noLastMinMaxValue = false;
	bool isCurrentValueNumeric = false;
	bool isLastMinValueNumeric = false;
	bool isLastMaxValueNumeric = false;
	double currentScaledValueNumeric = 0;
	double lastMinScaledValueNumeric = 0;
	double lastMaxScaledValueNumeric = 0;
	double currentMinCompValue = 0;
	double currentMaxCompValue = 0;
	double lastMinCompValue = 0;
	double lastMaxCompValue = 0;
	// List output
	QStringList minValueStrList;
	QStringList valueStrList;
	QStringList maxValueStrList;
	QStringList unitStrList;
	unsigned int rvIndex = 0;

	for (k=0; static_cast<unsigned int>(k)<_MBSWmetaList.size(); k++)	// MB/SW LOOP
	{
		// ******** SCALE MB/SW ********:
		// Get raw value index for the current MB/SW:
		rvIndex = _rawValueIndexes.at(k);
		// Scale raw values:
		if (_MBSWmetaList.at(k).blockType == 0) // if it is a MB
		{
			scalingSuccessful = libFSSM::raw2scaled( rawValues.at(rvIndex), _supportedMBs.at( _MBSWmetaList.at(k).nativeIndex ).scaleformula, _supportedMBs.at( _MBSWmetaList.at(k).nativeIndex ).precision, &scaledValueStr);
			if (scalingSuccessful)
				unitStrList.append(_supportedMBs.at( _MBSWmetaList.at(k).nativeIndex ).unit);
			else
				unitStrList.append("[RAW]");
		}
		else	// it is a SW
		{
			if (rawValues.at(rvIndex) == 0)
			{
				if (_supportedSWs.at( _MBSWmetaList.at(k).nativeIndex ).unit.contains('/'))
					scaledValueStr = _supportedSWs.at( _MBSWmetaList.at(k).nativeIndex ).unit.section('/',0,0);
				else
					scaledValueStr.clear();
				scalingSuccessful = !scaledValueStr.isEmpty();
			}
			else if (rawValues.at(rvIndex) == 1)
			{
				scaledValueStr = _supportedSWs.at( _MBSWmetaList.at(k).nativeIndex ).unit.section('/',1,1);
				scalingSuccessful = !scaledValueStr.isEmpty();
			}
			else
			{
				/* NOTE:
				 * THIS MEANS THAT WE HAVE A SEVERE BUG IN SSMprotocol2::assignMBSWRawData(...) !
				 * => handling this theoretical case would be too complicated...
				 *    => display the raw value (although it is definitely wrong)
				 *       => in combination with the display unit [BIN], the user should notice
                                 *          that something is going wrong...
				 */
				scalingSuccessful = false;
				scaledValueStr = "";
			}
			// Add unit string to the output list:
			if (!scalingSuccessful/*((rawValues.at(rvIndex)==0) || (rawValues.at(rvIndex) == 1)) && scaledValueStr.isEmpty()*/) // if we have a valid raw value but scaling failed
				unitStrList.append("[BIN]"); // display a unit, to signal that the displayed value is unscaled 
			else
				unitStrList.append("");
		}
		// Add value string to the output list:
		if (scalingSuccessful)
			valueStrList.append( scaledValueStr );
		else
			valueStrList.append( QString::number( rawValues.at(rvIndex) ) );
		// ******** CHECK FOR NEW MIN/MAX VALUE ********:
		/* NOTE:   
		 * - MB/SW scaled values can be NUMERIC VALUES or STRINGS or even BOTH MIXED (for different raw values)
		 * - scaled numeric values DO NOT necessarily grow with increasing raw values
		 * => STRATEGY:
		 * - compare scaled values only if min/max AND current values both have numeric scaled values
		 * - compare raw values in any other case (even if one of the scaled values is numeric !)
		 */
		// Check if we already have min+max values, try to convert last min/max values and current value to (scaled) numeric values:
		isLastMinValueNumeric = false;
		isLastMaxValueNumeric = false;
		if (k >= _minmaxData.size())	// if no last min/max values available
			noLastMinMaxValue = true;
		else
		{
			noLastMinMaxValue = false;
			if (scalingSuccessful && !_minmaxData.at(k).disabled) // NOTE: otherwise values aren't used
			{
				lastMinScaledValueNumeric = _minmaxData.at(k).minScaledValueStr.toDouble(&isLastMinValueNumeric);
				lastMaxScaledValueNumeric = _minmaxData.at(k).maxScaledValueStr.toDouble(&isLastMaxValueNumeric);
			}
		}
		if (isLastMinValueNumeric || isLastMaxValueNumeric || noLastMinMaxValue)
			currentScaledValueNumeric = valueStrList.at(k).toDouble(&isCurrentValueNumeric);
		// Disable min/max values for this MB/SW, if scaling failed and if we already have scaled min/max values:
		if (!scalingSuccessful && !noLastMinMaxValue && !_minmaxData.at(k).disabled  && (!_minmaxData.at(k).minScaledValueStr.isEmpty() || !_minmaxData.at(k).maxScaledValueStr.isEmpty()))
		{
			_minmaxData[k].disabled = true;
			_minmaxData[k].minScaledValueStr = "";	// important !
			_minmaxData[k].maxScaledValueStr = "";	// important !
			/* NOTE:
			 * - do not generally disable min/max values, if scaling failed. Pure raw value MBs/SWs (without any scaling information) should be allowed
			 * - maybe we can improve the min/max determination for MBs/SWs which are partially unscalable
			 *   (does it make sense to switch betweend scaled and unscaled min/max values ???)
			 */
		}
		// Update/get new min/max values:
		if (noLastMinMaxValue || !_minmaxData.at(k).disabled) // if we don't have min/max-values yet or min/max values are not disabled
		{
			if (noLastMinMaxValue)
			{
				// Set min and max value to current value:
				MinMaxMBSWvalue_dt newMinMaxDataset;
				newMinMaxDataset.minRawValue = rawValues.at(rvIndex);
				newMinMaxDataset.maxRawValue = rawValues.at(rvIndex);
				if (scalingSuccessful)
				{
					newMinMaxDataset.minScaledValueStr = valueStrList.at(k);
					newMinMaxDataset.maxScaledValueStr = valueStrList.at(k);
				}
				else
				{
					newMinMaxDataset.minScaledValueStr = "";
					newMinMaxDataset.maxScaledValueStr = "";
				}
				_minmaxData.append( newMinMaxDataset );
			}
			else
			{
				// Determine values for min comparison:
				if (scalingSuccessful && isCurrentValueNumeric && isLastMinValueNumeric)
				{
					// Use scaled values for min comparison:
					currentMinCompValue = currentScaledValueNumeric;
					lastMinCompValue = lastMinScaledValueNumeric;
				}
				else
				{
					// Use raw values for min comparison:
					currentMinCompValue = rawValues.at(rvIndex);
					lastMinCompValue = _minmaxData.at(k).minRawValue;
				}
				// Determine values for max comparison:
				if (scalingSuccessful && isCurrentValueNumeric && isLastMaxValueNumeric)
				{
					// Use scaled values for max comparison:
					currentMaxCompValue = currentScaledValueNumeric;
					lastMaxCompValue = lastMaxScaledValueNumeric;
				}
				else
				{
					// Use raw values for max comparison:
					currentMaxCompValue = rawValues.at(rvIndex);
					lastMaxCompValue = _minmaxData.at(k).maxRawValue;
				}
				/* NOTE: only compare scaled values, if BOTH (min/max and current) are numeric ! */
				// Check if we have a new min value:
				if (currentMinCompValue < lastMinCompValue)
				{
					_minmaxData[k].minRawValue = rawValues.at(rvIndex);
					if (scalingSuccessful)
						_minmaxData[k].minScaledValueStr = valueStrList.at(k);
					else
						_minmaxData[k].minScaledValueStr = "";
				}
				// Check if we have a new max value:
				if (currentMaxCompValue > lastMaxCompValue)
				{
					_minmaxData[k].maxRawValue = rawValues.at(rvIndex);
					if (scalingSuccessful)
						_minmaxData[k].maxScaledValueStr = valueStrList.at(k);
					else
						_minmaxData[k].maxScaledValueStr = "";
				}
			}
		}
		// Add min/max strings to the output list:
		if (_minmaxData.at(k).disabled)
		{
			// Don not display any min/values, if disabled:
			minValueStrList.append( "" );
			maxValueStrList.append( "" );
		}
		else if (_minmaxData.at(k).minScaledValueStr.isEmpty() || _minmaxData.at(k).maxScaledValueStr.isEmpty()) // if min/max is not disabled an we have unscaled min/max values
		{
			/* NOTE: min/max scaled value strings should BOTH be empty in this case (otherwise min/max would have been disabled before !) */
			// Display raw min/max values:
			minValueStrList.append( QString::number(_minmaxData.at(k).minRawValue) );
			maxValueStrList.append( QString::number(_minmaxData.at(k).maxRawValue) );
		}
		else
		{
			// Display scaled min/max values:
			minValueStrList.append( _minmaxData.at(k).minScaledValueStr );
			maxValueStrList.append( _minmaxData.at(k).maxScaledValueStr );
		}
		// ******** Save current value data ********:
		if (k >= _lastValues.size())
		{
			MBSWvalue_dt newValueDataset;
			newValueDataset.rawValue = rawValues.at(rvIndex);
			if (scalingSuccessful)
				newValueDataset.scaledStr = valueStrList.at(k);
			else
				newValueDataset.scaledStr = "";
			newValueDataset.unitStr = unitStrList.at(k);
			_lastValues.append( newValueDataset );
		}
		else
		{
			_lastValues[k].rawValue = rawValues.at(rvIndex);
			if (scalingSuccessful)
				_lastValues[k].scaledStr = valueStrList.at(k);
			else
				_lastValues[k].scaledStr = "";
			_lastValues[k].unitStr = unitStrList.at(k);
		}
	}
	// Display new values:
	_valuesTableView->updateMBSWvalues(valueStrList, minValueStrList, maxValueStrList, unitStrList);
	// Output refresh duration:
	updateTimeInfo(refreshduration_ms);
}


void CUcontent_MBsSWs::updateTimeInfo(int refreshduration_ms)
{
	double secs_ilen = 0;
	double datarate = 0;

	_lastrefreshduration_ms = refreshduration_ms; // save last refresh duration
	QString timeValStr = "";
	// Output refresh duration:
	secs_ilen = static_cast<double>(refreshduration_ms) / 1000;
	if (_timemode == 0)
		timeValStr = QString::number(secs_ilen, 'f', 3) + " s";
	else
	{
		datarate = _MBSWmetaList.size() / secs_ilen;
		timeValStr = QString::number(datarate, 'f', 1) + " B/s";
	}
	_MBSWrefreshTimeValue_label->setText(timeValStr);
}


void CUcontent_MBsSWs::addMBsSWs()
{
	unsigned int MBSWmetaList_len_old = _MBSWmetaList.size();
	// Open selection dialog:
	AddMBsSWsDlg *dlg = new AddMBsSWsDlg(this, _supportedMBs, _supportedSWs, &_MBSWmetaList);
	dlg->exec();
	delete dlg;
	// Update table:
	if (_MBSWmetaList.size() != MBSWmetaList_len_old)
	{
		// Clear current values:
		_lastValues.clear();
		_minmaxData.clear();
		// Update MB/SW table content:
		displayMBsSWs();
		// Clear time information:
		_MBSWrefreshTimeValue_label->setText("---      ");
		// Select new MBs/SWs:
		_valuesTableView->selectMBSWtableRows(MBSWmetaList_len_old, _MBSWmetaList.size()-1);
		// Scroll to end of the table:
		_valuesTableView->scrollMBSWtable(_MBSWmetaList.size()-1);
	}
	// Activate/deactivate buttons:
	if (_MBSWmetaList.size() > 0)
	{
		startstopmbreading_pushButton->setEnabled(true);
		mbswdelete_pushButton->setEnabled(true);
		connect(_SSMP2dev, SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ));
	}
	if (_MBSWmetaList.size() >= (_supportedMBs.size() + _supportedSWs.size()))
		mbswadd_pushButton->setEnabled(false);	// "Add"-button aktivieren
}


void CUcontent_MBsSWs::deleteMBsSWs()
{
	QList<unsigned int> selectedMBSWIndexes;
	unsigned int startindex = 0;
	unsigned int endindex = 0;
	int k = 0;
	// GET INDEXES OF SELECTED ROWS:
	_valuesTableView->getSelectedTableWidgetRows(&selectedMBSWIndexes);
	if (selectedMBSWIndexes.size() < 1) return;
	// CHECK AND CORRECT START AND END INDEXES:
	startindex = selectedMBSWIndexes.at(0);
	if (startindex > (_MBSWmetaList.size()-1)) return; // Cancel, if only empty table lines are selected
	endindex = selectedMBSWIndexes.at(selectedMBSWIndexes.size()-1);
	if (endindex > (_MBSWmetaList.size()-1))
		endindex = (_MBSWmetaList.size()-1); // correct last index, if section exceeds end of list
	// DELETE MB/SWs FROM SELECTION LIST (METALIST):
	_MBSWmetaList.erase(_MBSWmetaList.begin()+startindex, _MBSWmetaList.begin()+endindex+1);
	// DELETE LAST VALUE(S):
	for (k=0; k<selectedMBSWIndexes.size(); k++)
	{
		_lastValues.removeAt(startindex + k);
		_minmaxData.removeAt(startindex + k);
	}
	// UPDATE MB/SW TABLE CONTENT:
	displayMBsSWs();
	// Clear time information:
	_MBSWrefreshTimeValue_label->setText("---      ");
	// ACTIVATE/DEACTIVATE BUTTONS:
	if (_MBSWmetaList.empty())
	{
		startstopmbreading_pushButton->setEnabled(false);
		mbswdelete_pushButton->setEnabled(false);
		disconnect(_SSMP2dev, SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ));
	}
	if (_MBSWmetaList.size() < (_supportedMBs.size() + _supportedSWs.size()))	// if not all MBs/SWs are selected
		mbswadd_pushButton->setEnabled(true);
}


void CUcontent_MBsSWs::moveupMBsSWs()
{
	QList<unsigned int> selectedMBSWIndexes;
	int nrofSelRows = 0;
	int rowToMoveDownIndex = 0;
	int rowToMoveDownTargetIndex = 0;
	MBSWmetadata_dt datablockToMoveDown = {0, 0};
	int k = 0;
	// GET SELECTED ROWS:
	_valuesTableView->getSelectedTableWidgetRows(&selectedMBSWIndexes);
	nrofSelRows = selectedMBSWIndexes.size();
	// CHECK AND CORRECT SELECTED ROWS:
	if ((nrofSelRows < 1) || (selectedMBSWIndexes.at(0) < 1) || (1 + selectedMBSWIndexes.at(0) > _MBSWmetaList.size()))
		return;	// Cancel, if moving up is not possible
	if ((selectedMBSWIndexes.at(0) + nrofSelRows) > _MBSWmetaList.size()) // if selection exceed the end of the list
		nrofSelRows = _MBSWmetaList.size() - selectedMBSWIndexes.at(0);
	// NOTE: IN FACT WE ARE MOVING 1 ROW DOWN... 
	// GET START AND TERGET INDEX OF THE ROW THAT WILL BE MOVED:
	rowToMoveDownIndex = selectedMBSWIndexes.at(0) - 1;	
	rowToMoveDownTargetIndex = selectedMBSWIndexes.at(nrofSelRows-1);
	// MOVE MBs/SWs AT SELECTION LIST (METALIST):
	datablockToMoveDown = _MBSWmetaList.at(rowToMoveDownIndex);
	for (k=1; k<=nrofSelRows; k++)
		_MBSWmetaList.at(rowToMoveDownIndex + (k-1)) = _MBSWmetaList.at(rowToMoveDownIndex + k);
	_MBSWmetaList.at(rowToMoveDownTargetIndex) = datablockToMoveDown;
	// MOVE LAST VALUEs:
	if (_lastValues.size() > rowToMoveDownTargetIndex)
		_lastValues.move(rowToMoveDownIndex, rowToMoveDownTargetIndex);
	if (_minmaxData.size() > rowToMoveDownTargetIndex)
		_minmaxData.move(rowToMoveDownIndex, rowToMoveDownTargetIndex);
	// MOVE RAW VALUE INDEXES:
	if (_rawValueIndexes.size()>0)
	_rawValueIndexes.move(rowToMoveDownIndex, rowToMoveDownTargetIndex);
	// UPDATE MB/SW TABLE CONTENT:
	displayMBsSWs();
	// RESELECT MOVED ROWS:
	_valuesTableView->selectMBSWtableRows(rowToMoveDownIndex, rowToMoveDownTargetIndex-1);
	// SCROLL TO POSTION OF FIRST SELCTED ROW:
	_valuesTableView->scrollMBSWtable(rowToMoveDownIndex);
}


void CUcontent_MBsSWs::movedownMBsSWs()
{
	QList<unsigned int> selectedMBSWIndexes;
	int rowToMoveUpIndex = 0;
	int rowToMoveUpTargetIndex = 0;
	MBSWmetadata_dt datablockToMoveUp = {0,0};
	int k = 0;
	// GET SELECTED ROWS:
	_valuesTableView->getSelectedTableWidgetRows(&selectedMBSWIndexes);
	// CHECK AND CORRECT SELECTED ROWS:
	if ((selectedMBSWIndexes.size() < 1) | (selectedMBSWIndexes.at(selectedMBSWIndexes.size()-1)+1 >= _MBSWmetaList.size()))
		return;	// Cancle if moving is not possible
	// NOTE: IN FACT WE ARE MOVING 1 ROW UP... 
	// GET START AND TERGET INDEX OF THE ROW THAT WILL BE MOVED:
	rowToMoveUpIndex = selectedMBSWIndexes.at(selectedMBSWIndexes.size()-1)+1;
	rowToMoveUpTargetIndex = selectedMBSWIndexes.at(0);
	// MOVE MBs/SWs AT SELECTION LIST (METALIST):
	datablockToMoveUp = _MBSWmetaList.at(rowToMoveUpIndex);
	for (k=1; k<=selectedMBSWIndexes.size(); k++)
		_MBSWmetaList.at(rowToMoveUpIndex - (k-1)) = _MBSWmetaList.at(rowToMoveUpIndex - k);
	_MBSWmetaList.at(rowToMoveUpTargetIndex) = datablockToMoveUp;
	// MOVE LAST VALUES:
	if (_lastValues.size() > rowToMoveUpIndex)
		_lastValues.move(rowToMoveUpIndex, rowToMoveUpTargetIndex);
	if (_minmaxData.size() > rowToMoveUpIndex)
		_minmaxData.move(rowToMoveUpIndex, rowToMoveUpTargetIndex);
	// MOVE RAW VALUE INDEXES:
	if (_rawValueIndexes.size()>0)
	_rawValueIndexes.move(rowToMoveUpIndex, rowToMoveUpTargetIndex);
	// UPDATE MB/SW TABLE CONTENT:
	displayMBsSWs();
	// RESELECT MOVED ROWS:
	_valuesTableView->selectMBSWtableRows(rowToMoveUpTargetIndex+1, rowToMoveUpIndex);
	// SCROLL TO POSTION OF LAST SELCTED ROW:
	_valuesTableView->scrollMBSWtable(rowToMoveUpIndex);
}


void CUcontent_MBsSWs::resetMinMax()
{
	QStringList lastValueStr;
	QStringList lastUnitStr;
	MinMaxMBSWvalue_dt newMinMaxDataset;
	// Delete min/max values:
	_minmaxData.clear();
	// Setup new min/max values and output data:
	for (int k=0; k<_lastValues.size(); k++)
	{
		// Set min/max values to current value:
		newMinMaxDataset.minRawValue = _lastValues.at(k).rawValue;
		newMinMaxDataset.maxRawValue = _lastValues.at(k).rawValue;
		newMinMaxDataset.minScaledValueStr = _lastValues.at(k).scaledStr;
		newMinMaxDataset.maxScaledValueStr = _lastValues.at(k).scaledStr;
		_minmaxData.append( newMinMaxDataset );
		// Get min/max value string and unit for output:
		if (_lastValues.at(k).scaledStr.isEmpty())
			lastValueStr.append( QString::number(_lastValues.at(k).rawValue) );
		else
			lastValueStr.append( _lastValues.at(k).scaledStr );
		lastUnitStr.append( _lastValues.at(k).unitStr );
	}
	// Display last values as current/min/max values:
	_valuesTableView->updateMBSWvalues(lastValueStr, lastValueStr, lastValueStr, lastUnitStr);
}


void CUcontent_MBsSWs::setDeleteButtonEnabledState()
{
	if (_SSMP2dev->state() == SSMprotocol2::state_MBSWreading) return;
	QList<unsigned int> selectedMBSWIndexes;
	_valuesTableView->getSelectedTableWidgetRows(&selectedMBSWIndexes);
	if (selectedMBSWIndexes.size() < 1)
		mbswdelete_pushButton->setEnabled(false);
	else
		mbswdelete_pushButton->setEnabled(true);
}


void CUcontent_MBsSWs::switchTimeMode()
{
	QString timeValStr = "---      ";
	_timemode = !_timemode;
	if (_timemode)
	{
		_MBSWrefreshTimeTitle_label->setText(tr("Block transfer rate:   "));
		if (_lastrefreshduration_ms > 0)
		{
			double datarate = static_cast<double>(1000 * _MBSWmetaList.size()) / _lastrefreshduration_ms;
			timeValStr = QString::number(datarate, 'f', 1) + " B/s";
		}
	}
	else
	{
		_MBSWrefreshTimeTitle_label->setText(tr("Refresh duration:"));
		if (_lastrefreshduration_ms > 0)
		{
			double sec = static_cast<double>(_lastrefreshduration_ms) / 1000;
			timeValStr = QString::number(sec, 'f', 3) + " s";
		}
	}
	_MBSWrefreshTimeValue_label->setText(timeValStr);
}


void CUcontent_MBsSWs::getCurrentMBSWselection(std::vector<MBSWmetadata_dt> *MBSWmetaList)
{
	*MBSWmetaList = _MBSWmetaList;
}


void CUcontent_MBsSWs::getCurrentTimeMode(bool *timemode)
{
	*timemode = _timemode;
}


void CUcontent_MBsSWs::communicationError(QString addstr)
{
	if (addstr.size() > 0) addstr.prepend('\n');
	QMessageBox msg( QMessageBox::Critical, tr("Communication Error"), tr("Communication Error:\n- No or invalid answer from Control Unit -") + addstr, QMessageBox::Ok, this);
	QFont msgfont = msg.font();
	msgfont.setPixelSize(12); // 9pts
	msg.setFont( msgfont );
	msg.show();
	msg.exec();
	msg.close();
	emit error();
}


void CUcontent_MBsSWs::resizeEvent(QResizeEvent *event)
{
	_MBSWrefreshTimeTitle_label->move(width()-244, 3);
	_MBSWrefreshTimeValue_label->move(width()-100, 3);
	_timemode_pushButton->move(width()-29, 1);
	event->accept();
}


void CUcontent_MBsSWs::setupTimeModeUiElements()
{
	_MBSWrefreshTimeTitle_label = new QLabel("", this);
	_MBSWrefreshTimeTitle_label->setFixedWidth(140);
	_MBSWrefreshTimeTitle_label->setFixedHeight(16);
	_MBSWrefreshTimeValue_label = new QLabel("", this);
	_MBSWrefreshTimeValue_label->setFixedWidth(55);
	_MBSWrefreshTimeValue_label->setFixedHeight(16);
	_timemode_pushButton = new QPushButton(QIcon(QString::fromUtf8(":/icons/oxygen/16x16/chronometer.png")), "", this);
	_timemode_pushButton->setFixedWidth(20);
	_timemode_pushButton->setFixedHeight(20);
	_timemode_pushButton->setIconSize(QSize(12,12));
	_MBSWrefreshTimeTitle_label->move(width()-244, 3);
	_MBSWrefreshTimeValue_label->move(width()-100, 3);
	_timemode_pushButton->move(width()-29, 1);
	_MBSWrefreshTimeTitle_label->show();
	_MBSWrefreshTimeValue_label->show();
	_timemode_pushButton->show();
}


void CUcontent_MBsSWs::setupUiFonts()
{
	// SET FONT FAMILY AND FONT SIZE
	// OVERWRITES SETTINGS OF ui_CUcontent_MBsSWs.h (made with QDesigner)
	QFont contentfont = QApplication::font();
	contentfont.setPixelSize(12);// 9pts
	contentfont.setBold(false);
	this->setFont(contentfont);
	// Buttons:
	startstopmbreading_pushButton->setFont(contentfont);
	mbswadd_pushButton->setFont(contentfont);
	mbswdelete_pushButton->setFont(contentfont);
	_timemode_pushButton->setFont(contentfont);
	// Refresh interval labels:
	_MBSWrefreshTimeTitle_label->setFont(contentfont);
	_MBSWrefreshTimeValue_label->setFont(contentfont);
	// Tab widget:
	MBSWviews_tabWidget->setFont(contentfont);
}

