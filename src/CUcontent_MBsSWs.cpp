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



const QString CUcontent_MBsSWs::DefaultTimeValStr {"---"};

CUcontent_MBsSWs::CUcontent_MBsSWs(MBSWsettings_dt settings, QWidget *parent) : QWidget(parent)
{
	_SSMPdev = NULL;
	_timemode = settings.timeMode;
	_lastrefreshduration_ms = 0;

	// Setup GUI:
	setupUi(this);
	setupTimeModeUiElements();
	setupUiFonts();
	_valuesTableView = new CUcontent_MBsSWs_tableView(MBSWviews_tabWidget->widget(0), settings.minValuesEnabled, settings.maxValuesEnabled);
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
	updateRefreshTimeTitle();
	clearRefreshTime();
	// Connect signals and slots:
	connect( startstopmbreading_pushButton , SIGNAL( released() ), this, SLOT( startstopMBsSWsButtonPressed() ) );
	connect( mbswadd_pushButton , SIGNAL( released() ), this, SLOT( addMBsSWs() ) );
	connect( mbswdelete_pushButton , SIGNAL( released() ), this, SLOT( deleteMBsSWs() ) );
	connect( _valuesTableView , SIGNAL( moveUpButton_pressed() ), this, SLOT( moveUpMBsSWsOnTheTable() ) );
	connect( _valuesTableView , SIGNAL( moveDownButton_pressed() ), this, SLOT( moveDownMBsSWsOnTheTable() ) );
	connect( _valuesTableView , SIGNAL( resetMinMaxButton_pressed() ), this, SLOT( resetMinMaxTableValues() ) );
	connect( _valuesTableView , SIGNAL( itemSelectionChanged() ), this, SLOT( setDeleteButtonEnabledState() ) );
	connect( _timemode_pushButton , SIGNAL( released() ), this, SLOT( switchTimeMode() ) );
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
}


CUcontent_MBsSWs::~CUcontent_MBsSWs()
{
	if (_SSMPdev)
	{
		_SSMPdev->stopMBSWreading();
		disconnect( _SSMPdev, SIGNAL( newMBSWrawValues(const std::vector<unsigned int>&, int) ), this, SLOT( processMBSWRawValues(const std::vector<unsigned int>&, int) ) );
		disconnect( _SSMPdev , SIGNAL( stoppedMBSWreading() ), this, SLOT( callStop() ) );
		disconnect( _SSMPdev , SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ) );
	}
	disconnect( startstopmbreading_pushButton , SIGNAL( released() ), this, SLOT( startstopMBsSWsButtonPressed() ) );
	disconnect( mbswadd_pushButton , SIGNAL( released() ), this, SLOT( addMBsSWs() ) );
	disconnect( mbswdelete_pushButton , SIGNAL( released() ), this, SLOT( deleteMBsSWs() ) );
	disconnect( _valuesTableView , SIGNAL( moveUpButton_pressed() ), this, SLOT( moveUpMBsSWsOnTheTable() ) );
	disconnect( _valuesTableView , SIGNAL( moveDownButton_pressed() ), this, SLOT( moveDownMBsSWsOnTheTable() ) );
	disconnect( _valuesTableView , SIGNAL( resetMinMaxButton_pressed() ), this, SLOT( resetMinMaxTableValues() ) );
	disconnect( _valuesTableView , SIGNAL( itemSelectionChanged() ), this, SLOT( setDeleteButtonEnabledState() ) );
	disconnect( _timemode_pushButton , SIGNAL(released() ), this, SLOT( switchTimeMode() ) );
	delete _MBSWrefreshTimeTitle_label;
	delete _MBSWrefreshTimeValue_label;
	delete _timemode_pushButton;
	delete _valuesTableView;
}


bool CUcontent_MBsSWs::setup(SSMprotocol *SSMPdev)
{
	bool ok;
	_SSMPdev = SSMPdev;
	// Get supported MBs/SWs:
	ok = (_SSMPdev != NULL);
	if (ok)
		ok = _SSMPdev->getSupportedMBs(&_supportedMBs);
	if (ok)
		ok = _SSMPdev->getSupportedSWs(&_supportedSWs);
	if (!ok)
	{
		// Reset CU-data:
		_supportedMBs.clear();
		_supportedSWs.clear();
	}
	_MBSWmetaList.clear();
	_tableRowPosIndexes.clear();
	_lastValues.clear();
	_minmaxData.clear();
	// Reset refresh time:
	_lastrefreshduration_ms = 0;
	clearRefreshTime();
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


bool CUcontent_MBsSWs::setMBSWselection(const std::vector<MBSWmetadata_dt>& MBSWmetaList)
{
	// Check if MBSW-reading (and monitoring !) is in progress:
	if ((mbswadd_pushButton->isEnabled() == false) && (_MBSWmetaList.size() < (_supportedMBs.size() + _supportedSWs.size())))
		return false;
	// Check if the selected MBs/SWs are available:
	for (const MBSWmetadata_dt& metadata : MBSWmetaList)
	{
		unsigned int count = 0;
		switch(metadata.blockType)
		{
		case BlockType::MB:
			count = _supportedMBs.size();
			break;
		case BlockType::SW:
			count = _supportedSWs.size();
			break;
		}
		if (metadata.nativeIndex >= count)
			return false;
	}
	// Save MB/SW-list:
	_MBSWmetaList = MBSWmetaList;
	// Clear last values:
	_lastValues.clear();
	_minmaxData.clear();
	// Setup table position indexes:
	for (size_t k=0; k<MBSWmetaList.size(); k++)
		_tableRowPosIndexes.push_back(k);

	// Update MB/SW table content:
	displayMBsSWs();
	// Clear time information:
	clearRefreshTime();
	// Activate/deactivate buttons:
	if (_MBSWmetaList.size() > 0)
	{
		startstopmbreading_pushButton->setEnabled(true);
		mbswdelete_pushButton->setEnabled(true);
		connect(_SSMPdev, SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ));
	}
	else
		disconnect(_SSMPdev, SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ));
	if (_MBSWmetaList.size() >= (_supportedMBs.size() + _supportedSWs.size()))
		mbswadd_pushButton->setEnabled(false);	// "Add"-button aktivieren
	return true;
}


std::vector<MBSWmetadata_dt> CUcontent_MBsSWs::getMBSWselection() const
{
	// Return the MBSW-metalist re-ordered according to their positions on the values-table-widget:
	const size_t count = _MBSWmetaList.size();
	std::vector<MBSWmetadata_dt> orderedMBSWmetalist(count);
	for (size_t k=0; k<count; k++)
		orderedMBSWmetalist.at(_tableRowPosIndexes.at(k)) = _MBSWmetaList.at(k);
	return orderedMBSWmetalist;
}


void CUcontent_MBsSWs::displayMBsSWs()
{
	const auto itemcount = _tableRowPosIndexes.size();
	std::vector<BlockType> types(itemcount, BlockType::MB);
	// Prepare string-lists (fill with empty strings up to needed size):
	QStringList titles;
	libFSSM::fill_n_QStringList(titles, itemcount);
	QStringList minvalues(titles);
	QStringList values(titles);
	QStringList maxvalues(titles);
	QStringList units(titles);

	// Fill string-lists for output:
	for (size_t k=0; k<_MBSWmetaList.size(); k++)
	{
		// Get MB/SW-index:
		const auto listPosIndex = _tableRowPosIndexes.at(k);
		const MBSWmetadata_dt& metadata = _MBSWmetaList.at(k);
		// Title:
		switch(metadata.blockType)
		{
		case BlockType::MB:
			titles.replace( listPosIndex, _supportedMBs.at(metadata.nativeIndex).title );
			break;
		case BlockType::SW:
			titles.replace( listPosIndex, _supportedSWs.at(metadata.nativeIndex).title );
			types.at(listPosIndex) = BlockType::SW;
			break;
		}
		// Value and unit strings:
		// NOTE: _lastValues can be empty !
		if (_lastValues.size() > k)
		{
			const auto& lastValue = _lastValues.at(k);
			if (lastValue.scaledStr.isEmpty())
				values.replace( listPosIndex, QString::number(lastValue.rawValue) );
			else
				values.replace( listPosIndex, lastValue.scaledStr );
			units.replace( listPosIndex, lastValue.unitStr );
		}
		else
		{
			if (metadata.blockType == BlockType::MB)
				units.replace( listPosIndex, _supportedMBs.at(metadata.nativeIndex).unit );
		}
		// Last min/max value strings:
		// NOTE: _minmaxData can be empty !
		if (_minmaxData.size() > k)
		{
			if (!_minmaxData.at(k).disabled)
			{
				if (_minmaxData.at(k).minScaledValueStr.isEmpty())
					minvalues.replace( listPosIndex, QString::number(_minmaxData.at(k).minRawValue) );
				else
					minvalues.replace( listPosIndex, _minmaxData.at(k).minScaledValueStr );
				if (_minmaxData.at(k).maxScaledValueStr.isEmpty())
					maxvalues.replace( listPosIndex, QString::number(_minmaxData.at(k).maxRawValue) );
				else
					maxvalues.replace( listPosIndex, _minmaxData.at(k).maxScaledValueStr );
			}
		}
	}
	// Display MBs/SWs
	_valuesTableView->setMBSWlistContent(types, titles, values, minvalues, maxvalues, units);
}


void CUcontent_MBsSWs::startstopMBsSWsButtonPressed()
{
	if (!_SSMPdev || (_SSMPdev->state() == SSMprotocol::state_MBSWreading))
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
	SSMprotocol::state_dt state = SSMprotocol::state_needSetup;
	std::vector<MBSWmetadata_dt> usedMBSWmetaList;
	unsigned int k = 0;
	bool consistent = true;
	if (!_SSMPdev) return false;
	// Check premises:
	state = _SSMPdev->state();
	if (state == SSMprotocol::state_normal)
	{
		if (_MBSWmetaList.empty()) return false;
		disconnect( _SSMPdev, SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ) );
		// Start MB/SW-reading:
		if (!_SSMPdev->startMBSWreading(_MBSWmetaList))
		{
			connect( _SSMPdev, SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ) );
			return false;
		}
	}
	else if (state == SSMprotocol::state_MBSWreading)
	{
		// Verify consistency:
		if (!_SSMPdev->getLastMBSWselection(&usedMBSWmetaList))
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
	// Reset old data:
	_lastValues.clear();
	_minmaxData.clear();
	// Clear values in MB/SW-table:
	displayMBsSWs();
	// Clear refresh-time-information:
	clearRefreshTime();
	// Connect signals and slots:
	connect( _SSMPdev, SIGNAL( newMBSWrawValues(const std::vector<unsigned int>&, int) ), this, SLOT( processMBSWRawValues(const std::vector<unsigned int>&, int) ) );
	connect( _SSMPdev , SIGNAL( stoppedMBSWreading() ), this, SLOT( callStop() ) );
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
	if (_SSMPdev)
	{
		disconnect( _SSMPdev , SIGNAL( stoppedMBSWreading() ), this, SLOT( callStop() ) ); // must be disconnected before stopMBSWreading is called
		if (!_SSMPdev->stopMBSWreading())
		{
			connect( _SSMPdev , SIGNAL( stoppedMBSWreading() ), this, SLOT( callStop() ) ); // must be disconnected before stopMBSWreading is called
			return false;
		}
		disconnect( _SSMPdev, SIGNAL( newMBSWrawValues(const std::vector<unsigned int>&, int) ), this, SLOT( processMBSWRawValues(const std::vector<unsigned int>&, int) ) );
		connect( _SSMPdev, SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ) );
	}
	// Set text+icon of start/stop-button:
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


void CUcontent_MBsSWs::processMBSWRawValues(const std::vector<unsigned int>& rawValues, int refreshduration_ms)
{
	bool scalingSuccessful = false;
	QString scaledValueStr;
	unsigned int k = 0;
	// Min/Max comparison:
	bool invSWmeaning = false;
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
	bool newMin = false;
	bool newMax = false;
	// List output
	// Prepare string-lists for values-table-output (fill with empty strings up to the needed size):
	QStringList valueStrList;
	libFSSM::fill_n_QStringList(valueStrList, rawValues.size());
	QStringList minValueStrList(valueStrList);
	QStringList maxValueStrList(valueStrList);
	QStringList unitStrList(valueStrList);
	// Process raw values
	for (k=0; k<_MBSWmetaList.size(); k++)	// MB/SW LOOP
	{
		// Get table-position-index for current MB/SW:
		const unsigned int tablePosIndex = _tableRowPosIndexes.at(k);
		const size_t nativeIndex = _MBSWmetaList.at(k).nativeIndex;
		// ******** SCALE MB/SW ********:
		// Scale raw values:
		if (_MBSWmetaList.at(k).blockType == BlockType::MB)
		{
			const mb_dt& mb = _supportedMBs.at(nativeIndex);
			scalingSuccessful = libFSSM::raw2scaled(rawValues.at(k), mb.scaleformula, mb.precision, &scaledValueStr);
			unitStrList.replace(tablePosIndex, scalingSuccessful ? mb.unit : "[RAW]");
		}
		else	// it is a SW
		{
			/* NOTE: Some switches have an inverse meaning ! (e.g. 0="High", 1="Low")
			 *       => the characters / and \ tell us which of them has to be interpreted as the lower/larger one
			 *          during the min/max value determination (can't use < > in XML-files):
			 *             a/b   => a smaller than b
			 *             a\b   => a larger than b
			 * THE MEANING DOES NOT AFFECT THE SCALING PROCESS !
			 */
			const sw_dt& sw = _supportedSWs.at(nativeIndex);
			if (rawValues.at(k) == 0)
			{
				if (sw.unit.contains('/'))
				{
					scaledValueStr = sw.unit.section('/',0,0);
					invSWmeaning = false;
				}
				else if (sw.unit.contains('\\'))
				{
					scaledValueStr = sw.unit.section('\\',0,0);
					invSWmeaning = true;
				}
				else
				{
					scaledValueStr.clear();
					invSWmeaning = false;
				}
				scalingSuccessful = !scaledValueStr.isEmpty();
			}
			else if (rawValues.at(k) == 1)
			{
				if (sw.unit.contains('/'))
				{
					scaledValueStr = sw.unit.section('/',1,1);
					invSWmeaning = false;
				}
				else if (sw.unit.contains('\\'))
				{
					scaledValueStr = sw.unit.section('\\',1,1);
					invSWmeaning = true;
				}
				else
				{
					scaledValueStr.clear();
					invSWmeaning = false;
				}
				scalingSuccessful = !scaledValueStr.isEmpty();
			}
			else
			{
				/* NOTE:
				 * THIS MEANS THAT WE HAVE A SEVERE BUG IN SSMprotocol::assignMBSWRawData(...) !
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
				unitStrList.replace(tablePosIndex, "[BIN]"); // display a unit, to signal that the displayed value is unscaled
			else
				unitStrList.replace(tablePosIndex, "");
		}
		// Add value string to the output list:
		if (scalingSuccessful)
			valueStrList.replace( tablePosIndex, scaledValueStr );
		else
			valueStrList.replace( tablePosIndex, QString::number( rawValues.at(k) ) );
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
			currentScaledValueNumeric = valueStrList.at(tablePosIndex).toDouble(&isCurrentValueNumeric);
		// Disable min/max values for this MB/SW, if scaling failed and if we already have scaled min/max values:
		if (!noLastMinMaxValue && !_minmaxData.at(k).disabled)
		{
			if ( (!scalingSuccessful && (!_minmaxData.at(k).minScaledValueStr.isEmpty() || !_minmaxData.at(k).maxScaledValueStr.isEmpty())) ||
				 (scalingSuccessful && (_minmaxData.at(k).minScaledValueStr.isEmpty() || _minmaxData.at(k).maxScaledValueStr.isEmpty())) )
			{
				_minmaxData[k].disabled = true;
				_minmaxData[k].minScaledValueStr = "";	// important !
				_minmaxData[k].maxScaledValueStr = "";	// important !
				/* NOTE:
				* - do not generally disable min/max values, if scaling failed. Pure raw value MBs/SWs
				*   (without any scaling information) should be allowed
				* - maybe we can improve the min/max determination for MBs/SWs which are partially unscalable
				*   (does it make sense to switch betweend scaled and unscaled min/max values ???)
				*/
			}
		}
		// Update/get new min/max values:
		if (noLastMinMaxValue || !_minmaxData.at(k).disabled) // if we don't have min/max-values yet or min/max values are not disabled
		{
			if (noLastMinMaxValue)
			{
				// Set min and max value to current value:
				MinMaxMBSWvalue_dt newMinMaxDataset;
				newMinMaxDataset.minRawValue = rawValues.at(k);
				newMinMaxDataset.maxRawValue = rawValues.at(k);
				if (scalingSuccessful)
				{
					newMinMaxDataset.minScaledValueStr = valueStrList.at(tablePosIndex);
					newMinMaxDataset.maxScaledValueStr = valueStrList.at(tablePosIndex);
				}
				else
				{
					newMinMaxDataset.minScaledValueStr = "";
					newMinMaxDataset.maxScaledValueStr = "";
				}
				_minmaxData.push_back(newMinMaxDataset);
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
					currentMinCompValue = rawValues.at(k);
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
					currentMaxCompValue = rawValues.at(k);
					lastMaxCompValue = _minmaxData.at(k).maxRawValue;
				}
				/* NOTE: only compare scaled values, if BOTH (min/max and current) are numeric ! */
				// Compare current value with last min/max values:
				if ( (_MBSWmetaList.at(k).blockType == BlockType::SW) && scalingSuccessful && invSWmeaning)
				{
					// Inverse comparison:
					newMin = (currentMinCompValue > lastMinCompValue);
					newMax = (currentMaxCompValue < lastMaxCompValue);
				}
				else
				{
					// Normal comparison
					newMin = (currentMinCompValue < lastMinCompValue);
					newMax = (currentMaxCompValue > lastMaxCompValue);
				}
				// Check if we have a new min value:
				if (newMin)
				{
					_minmaxData[k].minRawValue = rawValues.at(k);
					if (scalingSuccessful)
						_minmaxData[k].minScaledValueStr = valueStrList.at(tablePosIndex);
					else
						_minmaxData[k].minScaledValueStr = "";
				}
				// Check if we have a new max value:
				if (newMax)
				{
					_minmaxData[k].maxRawValue = rawValues.at(k);
					if (scalingSuccessful)
						_minmaxData[k].maxScaledValueStr = valueStrList.at(tablePosIndex);
					else
						_minmaxData[k].maxScaledValueStr = "";
				}
				/* NOTE: always save "real" values for SWs with inverse meaning ! */
			}
		}
		// Add min/max strings to the output list:
		if (_minmaxData.at(k).disabled)
		{
			// Don not display any min/values, if disabled:
			minValueStrList.replace( tablePosIndex, "" );
			maxValueStrList.replace( tablePosIndex, "" );
		}
		else if (_minmaxData.at(k).minScaledValueStr.isEmpty() || _minmaxData.at(k).maxScaledValueStr.isEmpty()) // if min/max is not disabled an we have unscaled min/max values
		{
			/* NOTE: min/max scaled value strings should BOTH be empty in this case
					 (otherwise min/max would have been disabled before !) */
			// Display raw min/max values:
			minValueStrList.replace( tablePosIndex, QString::number(_minmaxData.at(k).minRawValue) );
			maxValueStrList.replace( tablePosIndex, QString::number(_minmaxData.at(k).maxRawValue) );
		}
		else
		{
			// Display scaled min/max values:
			minValueStrList.replace( tablePosIndex, _minmaxData.at(k).minScaledValueStr );
			maxValueStrList.replace( tablePosIndex, _minmaxData.at(k).maxScaledValueStr );
		}
		// ******** Save current value data ********:
		if (k >= _lastValues.size())
		{
			MBSWvalue_dt newValueDataset;
			newValueDataset.rawValue = rawValues.at(k);
			if (scalingSuccessful)
				newValueDataset.scaledStr = valueStrList.at(tablePosIndex);
			else
				newValueDataset.scaledStr = "";
			newValueDataset.unitStr = unitStrList.at(tablePosIndex);
			_lastValues.push_back(newValueDataset);
		}
		else
		{
			_lastValues[k].rawValue = rawValues.at(k);
			if (scalingSuccessful)
				_lastValues[k].scaledStr = valueStrList.at(tablePosIndex);
			else
				_lastValues[k].scaledStr = "";
			_lastValues[k].unitStr = unitStrList.at(tablePosIndex);
		}
	}
	// Display new values:
	_valuesTableView->updateMBSWvalues(valueStrList, minValueStrList, maxValueStrList, unitStrList);
	// Output refresh duration:
	updateTimeInfo(refreshduration_ms);
}


void CUcontent_MBsSWs::updateTimeInfo(int refreshduration_ms)
{
	_lastrefreshduration_ms = refreshduration_ms; // save last refresh duration
	QString timeValStr;
	// Output refresh duration:
	const double secs_ilen = static_cast<double>(refreshduration_ms) / 1000;
	switch(_timemode) {
	case TimeMode::refreshDuration:
		timeValStr = QString::number(secs_ilen, 'f', 3) + " s";
		break;
	case TimeMode::dataRate:
		double datarate = _MBSWmetaList.size() / secs_ilen;
		timeValStr = QString::number(datarate, 'f', 1) + " B/s";
		break;
	}
	_MBSWrefreshTimeValue_label->setText(timeValStr);
}


void CUcontent_MBsSWs::updateRefreshTimeTitle()
{
	_MBSWrefreshTimeTitle_label->setText(tr(_timemode == TimeMode::refreshDuration ? "Refresh duration:" : "Block transfer rate:"));
}


void CUcontent_MBsSWs::clearRefreshTime()
{
	_MBSWrefreshTimeValue_label->setText(DefaultTimeValStr);
}


void CUcontent_MBsSWs::addMBsSWs()
{
	unsigned int MBSWmetaList_len_old = _MBSWmetaList.size();
	unsigned int k = 0;
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
		// Add new table-position-indexes:
		for (k=MBSWmetaList_len_old; k<_MBSWmetaList.size(); k++)
			_tableRowPosIndexes.push_back(k);
		// Update MB/SW table content:
		displayMBsSWs();
		// Clear time information:
		clearRefreshTime();
		// Select new MBs/SWs:
		if (MBSWmetaList_len_old > 0)
		{
			_valuesTableView->selectMBSWtableRows(MBSWmetaList_len_old, _MBSWmetaList.size()-1);
			mbswdelete_pushButton->setEnabled(true);
		}
		// Scroll to end of the table:
		_valuesTableView->scrollMBSWtable(_MBSWmetaList.size()-1);
	}
	// Activate/deactivate buttons:
	if (_MBSWmetaList.size() > 0)
	{
		startstopmbreading_pushButton->setEnabled(true);
		connect(_SSMPdev, SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ));
	}
	if (_MBSWmetaList.size() >= (_supportedMBs.size() + _supportedSWs.size()))
		mbswadd_pushButton->setEnabled(false);	// "Add"-button aktivieren
}


void CUcontent_MBsSWs::deleteMBsSWs()
{
	unsigned int startindex = 0;
	unsigned int endindex = 0;
	unsigned int k = 0;
	// GET INDEXES OF SELECTED ROWS:
	const auto selectedMBSWIndexes = _valuesTableView->getSelectedTableWidgetRows();
	if (selectedMBSWIndexes.size() < 1) return;
	// CHECK AND CORRECT START AND END INDEXES:
	startindex = selectedMBSWIndexes.at(0);
	if (startindex > (_MBSWmetaList.size()-1)) return; // Cancel, if only empty table lines are selected
	endindex = selectedMBSWIndexes.at(selectedMBSWIndexes.size()-1);
	if (endindex > (_MBSWmetaList.size()-1))
		endindex = (_MBSWmetaList.size()-1); // correct last index, if section exceeds end of list
	// DELETE MBs/SWs AND DATA:
	k = _MBSWmetaList.size();
	while (k>0)
	{
		k--;
		if ((_tableRowPosIndexes.at(k) >= startindex) && (_tableRowPosIndexes.at(k) <= endindex))
		{
			// DELETE MB/SW FROM SELECTION LIST (METALIST):
			_MBSWmetaList.erase(_MBSWmetaList.begin() + k);
			// DELETE LAST VALUE, MIN-/MAX-DATA AND PLOT DATA:
			if (_lastValues.size())
				_lastValues.erase(_lastValues.begin() + k);
			if (_minmaxData.size())
				_minmaxData.erase(_minmaxData.begin() + k);
			// DELETE TABLE POSITION INDEX:
			_tableRowPosIndexes.erase(_tableRowPosIndexes.begin() + k);
		}
		else if (_tableRowPosIndexes.at(k) > endindex)
		{
			_tableRowPosIndexes[k] -= (endindex - startindex + 1);
		}
	}
	// UPDATE MB/SW TABLE CONTENT:
	displayMBsSWs();
	// Clear time information:
	clearRefreshTime();
	// ACTIVATE/DEACTIVATE BUTTONS:
	if (_MBSWmetaList.empty())
	{
		startstopmbreading_pushButton->setEnabled(false);
		mbswdelete_pushButton->setEnabled(false);
		disconnect(_SSMPdev, SIGNAL( startedMBSWreading() ), this, SLOT( callStart() ));
	}
	if (_MBSWmetaList.size() < (_supportedMBs.size() + _supportedSWs.size()))	// if not all MBs/SWs are selected
		mbswadd_pushButton->setEnabled(true);
}


void CUcontent_MBsSWs::moveUpMBsSWsOnTheTable()
{
	int nrofSelRows = 0;
	unsigned int rowToMoveDownIndex = 0;
	unsigned int rowToMoveDownTargetIndex = 0;
	// GET SELECTED ROWS:
	const auto selectedMBSWIndexes = _valuesTableView->getSelectedTableWidgetRows();
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
	// MODIFY TABLE-POSITION-INDEXES FOR OUTPUT:
	for (size_t k=0; k<_tableRowPosIndexes.size(); k++)
	{
		if ((_tableRowPosIndexes.at(k) > rowToMoveDownIndex) && (_tableRowPosIndexes.at(k) <= rowToMoveDownTargetIndex))
		{
			_tableRowPosIndexes[k] -= 1;
		}
		else if (_tableRowPosIndexes.at(k) == rowToMoveDownIndex)
		{
			_tableRowPosIndexes[k] = rowToMoveDownTargetIndex;
		}
	}
	// UPDATE MB/SW TABLE CONTENT:
	displayMBsSWs();
	// RESELECT MOVED ROWS:
	_valuesTableView->selectMBSWtableRows(rowToMoveDownIndex, rowToMoveDownTargetIndex-1);
	// SCROLL TO POSTION OF FIRST SELCTED ROW:
	_valuesTableView->scrollMBSWtable(rowToMoveDownIndex);
}


void CUcontent_MBsSWs::moveDownMBsSWsOnTheTable()
{
	unsigned int rowToMoveUpIndex = 0;
	unsigned int rowToMoveUpTargetIndex = 0;
	// GET SELECTED ROWS:
	const auto selectedMBSWIndexes = _valuesTableView->getSelectedTableWidgetRows();
	// CHECK AND CORRECT SELECTED ROWS:
	if ((selectedMBSWIndexes.size() < 1) | (selectedMBSWIndexes.at(selectedMBSWIndexes.size()-1)+1 >= _MBSWmetaList.size()))
		return;	// Cancle if moving is not possible
	// NOTE: IN FACT WE ARE MOVING 1 ROW UP...
	// GET START AND TERGET INDEX OF THE ROW THAT WILL BE MOVED:
	rowToMoveUpIndex = selectedMBSWIndexes.at(selectedMBSWIndexes.size()-1)+1;
	rowToMoveUpTargetIndex = selectedMBSWIndexes.at(0);
	// MODIFY TABLE-POSITION-INDEXES FOR OUTPUT:
	for (size_t k=0; k<_tableRowPosIndexes.size(); k++)
	{
		if ((_tableRowPosIndexes.at(k) >= rowToMoveUpTargetIndex) && (_tableRowPosIndexes.at(k) < rowToMoveUpIndex))
		{
			_tableRowPosIndexes[k] += 1;
		}
		else if (_tableRowPosIndexes.at(k) == rowToMoveUpIndex)
		{
			_tableRowPosIndexes[k] = rowToMoveUpTargetIndex;
		}
	}
	// UPDATE MB/SW TABLE CONTENT:
	displayMBsSWs();
	// RESELECT MOVED ROWS:
	_valuesTableView->selectMBSWtableRows(rowToMoveUpTargetIndex+1, rowToMoveUpIndex);
	// SCROLL TO POSTION OF LAST SELCTED ROW:
	_valuesTableView->scrollMBSWtable(rowToMoveUpIndex);
}


void CUcontent_MBsSWs::resetMinMaxTableValues()
{
	QStringList lastValueStr;
	QStringList lastUnitStr;
	lastValueStr.reserve(_lastValues.size());
	lastUnitStr.reserve(_lastValues.size());
	// Delete min/max values:
	_minmaxData.clear();
	// Setup new min/max values and output data:
	for (const MBSWvalue_dt& lastValue : _lastValues)
	{
		MinMaxMBSWvalue_dt newMinMaxDataset;
		// Set min/max values to current value:
		newMinMaxDataset.minRawValue = lastValue.rawValue;
		newMinMaxDataset.maxRawValue = lastValue.rawValue;
		newMinMaxDataset.minScaledValueStr = lastValue.scaledStr;
		newMinMaxDataset.maxScaledValueStr = lastValue.scaledStr;
		_minmaxData.push_back(newMinMaxDataset);
		// Get min/max value string and unit for output:
		lastValueStr.push_back( !lastValue.scaledStr.isEmpty() ? lastValue.scaledStr : QString::number(lastValue.rawValue));
		lastUnitStr.push_back( lastValue.unitStr );
	}
	// Display last values as current/min/max values:
	_valuesTableView->updateMBSWvalues(lastValueStr, lastValueStr, lastValueStr, lastUnitStr);
}


void CUcontent_MBsSWs::setDeleteButtonEnabledState()
{
	if (_SSMPdev->state() == SSMprotocol::state_MBSWreading)
		return;
	mbswdelete_pushButton->setEnabled( _valuesTableView->getSelectedTableWidgetRows().size() > 0 );
}


void CUcontent_MBsSWs::switchTimeMode()
{
	QString timeValStr {DefaultTimeValStr};
	TimeMode newtimemode {TimeMode::refreshDuration};
	switch(_timemode) {
	case TimeMode::refreshDuration:
		newtimemode = TimeMode::dataRate;
		if (_lastrefreshduration_ms > 0)
		{
			double datarate = static_cast<double>(1000 * _MBSWmetaList.size()) / _lastrefreshduration_ms;
			timeValStr = QString::number(datarate, 'f', 1) + " B/s";
		}
		break;
	case TimeMode::dataRate:
		newtimemode = TimeMode::refreshDuration;
		if (_lastrefreshduration_ms > 0)
		{
			double sec = static_cast<double>(_lastrefreshduration_ms) / 1000;
			timeValStr = QString::number(sec, 'f', 3) + " s";
		}
		break;
	}
	_timemode = newtimemode;
	_MBSWrefreshTimeValue_label->setText(timeValStr);
	updateRefreshTimeTitle();
}


MBSWsettings_dt CUcontent_MBsSWs::getSettings() const
{
	MBSWsettings_dt s;
	s.timeMode = _timemode;
	s.minValuesEnabled = _valuesTableView->minValuesEnabled();
	s.maxValuesEnabled = _valuesTableView->maxValuesEnabled();
	return s;
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


constexpr int offsetRightTimeTitle = 244;
constexpr int offsetRightTimeValue = 100;
constexpr int offsetRightTimeModeButton = 29;
constexpr int offsetDownTimeLabel = 3;
constexpr int offsetDownTimeModeButton = 1;
constexpr int heightLabel = 16;

void CUcontent_MBsSWs::resizeEvent(QResizeEvent *event)
{
	_MBSWrefreshTimeTitle_label->move(width() - offsetRightTimeTitle, offsetDownTimeLabel);
	_MBSWrefreshTimeValue_label->move(width() - offsetRightTimeValue, offsetDownTimeLabel);
	_timemode_pushButton->move(width() - offsetRightTimeModeButton, offsetDownTimeModeButton);
	event->accept();
}


void CUcontent_MBsSWs::setupTimeModeUiElements()
{
	_MBSWrefreshTimeTitle_label = new QLabel("", this);
	_MBSWrefreshTimeTitle_label->setFixedWidth(140);
	_MBSWrefreshTimeTitle_label->setFixedHeight(heightLabel);
	_MBSWrefreshTimeTitle_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	_MBSWrefreshTimeValue_label = new QLabel("", this);
	_MBSWrefreshTimeValue_label->setFixedWidth(70);
	_MBSWrefreshTimeValue_label->setFixedHeight(heightLabel);
	_timemode_pushButton = new QPushButton(QIcon(QString::fromUtf8(":/icons/oxygen/16x16/chronometer.png")), "", this);
	_timemode_pushButton->setFixedWidth(20);
	_timemode_pushButton->setFixedHeight(20);
	_timemode_pushButton->setIconSize(QSize(12,12));
	_MBSWrefreshTimeTitle_label->move(width() - offsetRightTimeTitle, offsetDownTimeLabel);
	_MBSWrefreshTimeValue_label->move(width() - offsetRightTimeValue, offsetDownTimeLabel);
	_timemode_pushButton->move(width() - offsetRightTimeModeButton, offsetDownTimeModeButton);
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
