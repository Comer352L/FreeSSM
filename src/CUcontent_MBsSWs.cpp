/*
 * CUcontent_MBsSWs.cpp - Widget for Reading of Measuring Blocks and Switches
 *
 * Copyright (C) 2008-2019 Comer352L
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
	_MBSWreading = false;

	// Setup GUI:
	setupUi(this);
	setupTimeModeUiElements();
	_valuesTableView = new CUcontent_MBsSWs_tableView(MBSWviews_tabWidget->widget(0), settings.minValuesEnabled, settings.maxValuesEnabled);
	valuesTableView_gridLayout->addWidget(_valuesTableView);
	//_curvesTableView = new ...
	//curvesView_gridLayout->addWidget();
	// Disable all GUI-elements:
	_MBSWrefreshTimeTitle_label->setEnabled( false );
	_MBSWrefreshTimeValue_label->setEnabled( false );
	_timemode_pushButton->setEnabled( false );
	labelStartStopButtonReadyForStart();
	startstopmbreading_pushButton->setEnabled( false );
	mbswsave_pushButton->setEnabled( false );
	mbswload_pushButton->setEnabled( false );
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
	connect( mbswsave_pushButton , SIGNAL( released() ), this, SLOT( saveMBsSWs() ) );
	connect( mbswload_pushButton , SIGNAL( released() ), this, SLOT( loadMBsSWs() ) );
	connect( _valuesTableView , SIGNAL( moveUpButton_pressed() ), this, SLOT( moveUpMBsSWsOnTheTable() ) );
	connect( _valuesTableView , SIGNAL( moveDownButton_pressed() ), this, SLOT( moveDownMBsSWsOnTheTable() ) );
	connect( _valuesTableView , SIGNAL( resetMinMaxButton_pressed() ), this, SLOT( resetMinMaxTableValues() ) );
	connect( _valuesTableView , SIGNAL( itemSelectionChanged() ), this, SLOT( setDeleteButtonEnabledState() ) );
	connect( _timemode_pushButton , SIGNAL( released() ), this, SLOT( switchTimeMode() ) );
	// NOTE: using released() instead of pressed() as workaround for a Qt-Bug occuring under MS Windows
	// Add some shortcuts:
	mbswadd_pushButton->setShortcut( QKeySequence("Ctrl++") );
	mbswdelete_pushButton->setShortcut( QKeySequence("Ctrl+-") );
	mbswsave_pushButton->setShortcut( QKeySequence("Ctrl+s") );
	mbswload_pushButton->setShortcut( QKeySequence("Ctrl+l") );
}


CUcontent_MBsSWs::~CUcontent_MBsSWs()
{
	if (_SSMPdev)
	{
		_SSMPdev->stopMBSWreading();
		disconnect( _SSMPdev, SIGNAL( newMBSWrawValues(const std::vector<unsigned int>&, int) ), this, SLOT( processMBSWRawValues(const std::vector<unsigned int>&, int) ) );
		disconnect( _SSMPdev , SIGNAL( stoppedMBSWreading() ), this, SLOT( stopMBSWreading() ) );
		disconnect( _SSMPdev , SIGNAL( startedMBSWreading() ), this, SLOT( startMBSWreading() ) );
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
	disconnect( mbswsave_pushButton , SIGNAL( released() ), this, SLOT( saveMBsSWs() ) );
	disconnect( mbswload_pushButton , SIGNAL( released() ), this, SLOT( loadMBsSWs() ) );
}


bool CUcontent_MBsSWs::setup(SSMprotocol *SSMPdev)
{
	bool ok = false;

	// Clean up old SSMPdev (if needed):
	if ((_SSMPdev != NULL) && (_SSMPdev != SSMPdev))
	{
		disconnect( _SSMPdev, SIGNAL( newMBSWrawValues(const std::vector<unsigned int>&, int) ), this, SLOT( processMBSWRawValues(const std::vector<unsigned int>&, int) ) );
		disconnect( _SSMPdev, SIGNAL( startedMBSWreading() ), this, SLOT( startMBSWreading() ) );
		disconnect( _SSMPdev, SIGNAL( stoppedMBSWreading() ), this, SLOT( stopMBSWreading() ) );
		if (_SSMPdev->state() == SSMprotocol::state_MBSWreading)
			_SSMPdev->stopMBSWreading();
	}
	// Reset reading state:
	_MBSWreading = false;
	// Reset MB/SW list:
	_MBSWmetaList.clear();
	_tableRowPosIndexes.clear();
	_lastValues.clear();
	_minmaxData.clear();
	// Reset refresh time:
	_lastrefreshduration_ms = 0;
	clearRefreshTime();
	// Display MB/SW list (make empty):
	displayMBsSWs();
	// Get supported MBs/SWs:
	ok = (SSMPdev != NULL);
	if (ok)
		ok = SSMPdev->getSupportedMBs(&_supportedMBs);
	if (ok)
		ok = SSMPdev->getSupportedSWs(&_supportedSWs);
	if (!ok)
	{
		// Reset CU-data:
		_supportedMBs.clear();
		_supportedSWs.clear();
	}
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
	// Disable "Delete"-button:
	mbswdelete_pushButton->setEnabled(false);
	// Reset "Start"-button:
	labelStartStopButtonReadyForStart();
	startstopmbreading_pushButton->setEnabled(false);
	// Disable "Save"-button:
	mbswsave_pushButton->setEnabled(false);
	// Enable "Load"-button if at least MB/SW is provided by the control unit:
	mbswload_pushButton->setEnabled( (_supportedMBs.size() + _supportedSWs.size() > 0) );
	// Save SSMPdev:
	if (ok)
		_SSMPdev = SSMPdev;
	else
		_SSMPdev = NULL;
	// NOTE: _SSMPdev != NULL means setup() has been done
	// Sync reading state:
	if (_SSMPdev)
	{
		if (_SSMPdev->state() == SSMprotocol::state_MBSWreading)
			startMBSWreading();	// NOTE: synchronizes MB/SW list etc.
		connect( _SSMPdev, SIGNAL( startedMBSWreading() ), this, SLOT( startMBSWreading() ) );
		connect( _SSMPdev, SIGNAL( stoppedMBSWreading() ), this, SLOT( stopMBSWreading() ) );
	}
	// Return result:
	return ok;
}


bool CUcontent_MBsSWs::validateMBSWselection(const std::vector<MBSWmetadata_dt>& MBSWmetaList)
{
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

	return true;
}


bool CUcontent_MBsSWs::setMBSWselection(const std::vector<MBSWmetadata_dt>& MBSWmetaList)
{
	// Check if setup() has already been called:
	if (_SSMPdev == NULL)
		return false;
	// Check if MB/SW monitoring is in progress:
	if (_MBSWreading)
		return false;
	// Check if the selected MBs/SWs are available:
	if (!validateMBSWselection(MBSWmetaList))
		return false;
	// Save+display the new MB/SW selection:
	setMBSWselectionUnvalidated(MBSWmetaList);

	return true;
}


void CUcontent_MBsSWs::setMBSWselectionUnvalidated(const std::vector<MBSWmetadata_dt>& MBSWmetaList)
{
	// Save MB/SW-list:
	_MBSWmetaList = MBSWmetaList;
	// Clear last values:
	_lastValues.clear();
	_minmaxData.clear();
	// Setup table position indexes:
	_tableRowPosIndexes.clear();
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
		mbswsave_pushButton->setEnabled(true);
	}
	else
	{
		startstopmbreading_pushButton->setEnabled(false);
		mbswdelete_pushButton->setEnabled(false);
		mbswsave_pushButton->setEnabled(false);
	}
	if (_MBSWmetaList.size() >= (_supportedMBs.size() + _supportedSWs.size()))
		mbswadd_pushButton->setEnabled(false);	// "Add"-button aktivieren
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


size_t CUcontent_MBsSWs::numMBsSWsSelected()
{
	return _MBSWmetaList.size();
}


void CUcontent_MBsSWs::displayMBsSWs()
{
	const size_t itemcount = _tableRowPosIndexes.size();
	std::vector<BlockType> types(itemcount, BlockType::MB);
	// Prepare strings, initialize with empty strings up to needed size:
	std::vector<QString> titles(itemcount);
	std::vector<QString> minvalues(itemcount);
	std::vector<QString> values(itemcount);
	std::vector<QString> maxvalues(itemcount);
	std::vector<QString> units(itemcount);

	// set strings for output:
	for (size_t k=0; k<_MBSWmetaList.size(); k++)
	{
		// Get MB/SW-index:
		const unsigned int listPosIndex = _tableRowPosIndexes.at(k);
		const MBSWmetadata_dt& metadata = _MBSWmetaList.at(k);
		// Title:
		switch(metadata.blockType)
		{
		case BlockType::MB:
			titles.at(listPosIndex) = _supportedMBs.at(metadata.nativeIndex).title;
			break;
		case BlockType::SW:
			titles.at(listPosIndex) = _supportedSWs.at(metadata.nativeIndex).title;
			types.at(listPosIndex) = BlockType::SW;
			break;
		}
		// Value and unit strings:
		// NOTE: _lastValues can be empty !
		if (_lastValues.size() > k)
		{
			const MBSWvalue_dt& lastValue = _lastValues.at(k);
			values.at(listPosIndex) = !lastValue.scaledStr.isEmpty() ? lastValue.scaledStr : QString::number(lastValue.rawValue);
			units.at(listPosIndex) = lastValue.unitStr;
		}
		else
		{
			if (metadata.blockType == BlockType::MB)
				units.at(listPosIndex) = _supportedMBs.at(metadata.nativeIndex).unit;
		}
		// Last min/max value strings:
		// NOTE: _minmaxData can be empty !
		if (_minmaxData.size() > k)
		{
			const MinMaxMBSWvalue_dt& minmaxData = _minmaxData.at(k);
			if (!minmaxData.disabled)
			{
				minvalues.at(listPosIndex) = !minmaxData.minScaledValueStr.isEmpty() ? minmaxData.minScaledValueStr : QString::number(minmaxData.minRawValue);
				maxvalues.at(listPosIndex) = !minmaxData.maxScaledValueStr.isEmpty() ? minmaxData.maxScaledValueStr : QString::number(minmaxData.maxRawValue);
			}
		}
	}
	// Display MBs/SWs
	_valuesTableView->setMBSWlistContent(types, titles, values, minvalues, maxvalues, units);
}


void CUcontent_MBsSWs::startstopMBsSWsButtonPressed()
{
	if (!_SSMPdev || _MBSWreading)
		stopMBSWreading();
	else
		startMBSWreading();
}


bool CUcontent_MBsSWs::startMBSWreading()
{
	SSMprotocol::state_dt state = SSMprotocol::state_needSetup;
	std::vector<MBSWmetadata_dt> usedMBSWmetaList;
	// Check if setup() has already been called:
	if (!_SSMPdev)
		return false;
	// Check premises:
	state = _SSMPdev->state(); // NOTE: we are interested in the "real" state here, not _MBSWreading
	if (state == SSMprotocol::state_normal)
	{
		if (_MBSWmetaList.empty())
			errorMsg(tr("Error"), tr("Error: failed to start MB/SW-reading:\nThe MB/SW list is empty"));
		bool sig_disc = disconnect( _SSMPdev, SIGNAL( startedMBSWreading() ), this, SLOT( startMBSWreading() ) );
		// Start MB/SW-reading:
		bool ok = _SSMPdev->startMBSWreading(_MBSWmetaList);
		if (sig_disc)
			connect( _SSMPdev, SIGNAL( startedMBSWreading() ), this, SLOT( startMBSWreading() ) );
		if (!ok)
			goto err;
	}
	else if (state == SSMprotocol::state_MBSWreading)
	{
		// NOTE: this means we are informed that someone else has called _SSMPdev->startMBSWreading()
		// Update the MB/SW selection:
		if (!_SSMPdev->getLastMBSWselection(&_MBSWmetaList))
			goto err;
		// NOTE: MB/SW list can't be empty
	}
	else
		goto err;
	// Reset old data:
	_lastValues.clear();
	_minmaxData.clear();
	// Clear values in MB/SW-table:
	displayMBsSWs();
	// Clear refresh-time-information:
	clearRefreshTime();
	// Connect signals and slots:
	connect( _SSMPdev, SIGNAL( newMBSWrawValues(const std::vector<unsigned int>&, int) ), this, SLOT( processMBSWRawValues(const std::vector<unsigned int>&, int) ) );
	// Disable add/delete buttons:
	mbswdelete_pushButton->setEnabled(false);
	mbswadd_pushButton->setEnabled(false);
	// Disable load state button
	mbswload_pushButton->setEnabled(false);
	// Set text+icon of start/stop-button:
	startstopmbreading_pushButton->setText(tr(" Stop  "));
	startstopmbreading_pushButton->setIcon( QIcon(QString::fromUtf8(":/icons/chrystal/32x32/player_stop.png")) );
	startstopmbreading_pushButton->setIconSize( QSize(24,24) );
	// Update state:
	_MBSWreading = true;
	return true;

err:
	communicationError(tr("=> Couldn't start Measuring Blocks Reading."));
	return false;
}


bool CUcontent_MBsSWs::stopMBSWreading()
{
	// Check if setup() has already been called:
	if (!_SSMPdev)
		return false;
	// Stop reading (if in progress):
	if (_SSMPdev->state() == SSMprotocol::state_MBSWreading)
	{
		bool sig_disc = disconnect( _SSMPdev , SIGNAL( stoppedMBSWreading() ), this, SLOT( stopMBSWreading() ) );
		bool ok = _SSMPdev->stopMBSWreading();
		if (sig_disc)
			connect( _SSMPdev , SIGNAL( stoppedMBSWreading() ), this, SLOT( stopMBSWreading() ) );
		if (!ok)
		{
			communicationError(tr("=> Couldn't stop Measuring Blocks Reading."));
			return false;
		}
	}
	disconnect( _SSMPdev, SIGNAL( newMBSWrawValues(const std::vector<unsigned int>&, int) ), this, SLOT( processMBSWRawValues(const std::vector<unsigned int>&, int) ) );
	// Set text+icon of start/stop-button:
	labelStartStopButtonReadyForStart();
	// Enable delete button if MBs/SWs are selected on the table:
	if (_valuesTableView->getSelectedTableWidgetRows().size() > 0)
		mbswdelete_pushButton->setEnabled(true);
	// Enable add button (if there is something left to add):
	if (_MBSWmetaList.size() < (_supportedMBs.size() + _supportedSWs.size()))
		mbswadd_pushButton->setEnabled(true);
	// Enable load button (if at least MB/SW is provided by the control unit):
	if (_supportedMBs.size() + _supportedSWs.size() > 0)
		mbswload_pushButton->setEnabled(true);
	// Update state:
	_MBSWreading = false;
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
	// Prepare strings for values-table-output (fill with empty strings up to the needed size):
	const size_t count = rawValues.size();
	std::vector<QString> valueStrList(count);
	std::vector<QString> minValueStrList(count);
	std::vector<QString> maxValueStrList(count);
	std::vector<QString> unitStrList(count);
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
			scalingSuccessful = libFSSM::raw2scaled(rawValues.at(k), mb.formula, mb.precision, &scaledValueStr);
			unitStrList.at(tablePosIndex) = scalingSuccessful ? mb.unit : "[RAW]";
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
				unitStrList.at(tablePosIndex) = "[BIN]"; // display a unit, to signal that the displayed value is unscaled
			else
				unitStrList.at(tablePosIndex) = "";
		}
		// Add value string to the output list:
		if (scalingSuccessful)
			valueStrList.at(tablePosIndex) = scaledValueStr;
		else
			valueStrList.at(tablePosIndex) = QString::number(rawValues.at(k));
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
			minValueStrList.at(tablePosIndex) = "";
			maxValueStrList.at(tablePosIndex) = "";
		}
		else if (_minmaxData.at(k).minScaledValueStr.isEmpty() || _minmaxData.at(k).maxScaledValueStr.isEmpty()) // if min/max is not disabled an we have unscaled min/max values
		{
			/* NOTE: min/max scaled value strings should BOTH be empty in this case
			         (otherwise min/max would have been disabled before !) */
			// Display raw min/max values:
			minValueStrList.at(tablePosIndex) = QString::number(_minmaxData.at(k).minRawValue);
			maxValueStrList.at(tablePosIndex) = QString::number(_minmaxData.at(k).maxRawValue);
		}
		else
		{
			// Display scaled min/max values:
			minValueStrList.at(tablePosIndex) = _minmaxData.at(k).minScaledValueStr;
			maxValueStrList.at(tablePosIndex) = _minmaxData.at(k).maxScaledValueStr;
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
	_MBSWrefreshTimeTitle_label->setText(_timemode == TimeMode::refreshDuration ? tr("Refresh duration:") : tr("Block transfer rate:"));
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
#ifdef SMALL_RESOLUTION
	dlg->showFullScreen();
#else
	dlg->show();
#endif
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
			mbswsave_pushButton->setEnabled(true);
		}
		// Scroll to end of the table:
		_valuesTableView->scrollMBSWtable(_MBSWmetaList.size()-1);
	}
	// Activate/deactivate buttons:
	if (_MBSWmetaList.size() > 0)
	{
		startstopmbreading_pushButton->setEnabled(true);
		mbswsave_pushButton->setEnabled(true);
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
	const std::vector<unsigned int> selectedMBSWIndexes = _valuesTableView->getSelectedTableWidgetRows();
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
		mbswsave_pushButton->setEnabled(false);
	}
	if (_MBSWmetaList.size() < (_supportedMBs.size() + _supportedSWs.size()))	// if not all MBs/SWs are selected
		mbswadd_pushButton->setEnabled(true);
}


bool CUcontent_MBsSWs::saveMBsSWs(QString filename)
{
	// Use ROM-ID to avoid loading MBs/SWs on a different control unit / ROM that does not support the same MBs/SWs:
	std::string ROM_ID = _SSMPdev->getROMID();
	// Get file name:
	if (!filename.size())
	{
		// Select file for saving
		filename = QFileDialog::getSaveFileName(this, tr("Save MB/SW List"),
							QCoreApplication::applicationDirPath(),
							tr("FreeSSM MB/SW list files") + " (*.list)(*.list)");
		if (!filename.size())
			return false;
	}
	// Open file:
	// NOTE: we write data in binary format, so non-human readable
	std::ofstream file (filename.toLocal8Bit(), std::ios::out | std::ios::binary | std::ios::trunc); // NOTE: out seems to imply trunc (but in+out does not)
	if (!file.is_open())
	{
		errorMsg(tr("Save Error"), tr("Error: failed to save the MB/SW list:\nCouldn't open the selected file"));
		return false;
	}
	// Save ROM-ID length:
	size_t size_of_ROMstring = ROM_ID.size();
	file.write((char*)(&size_of_ROMstring), sizeof(size_t));
	// Save ROM-ID:
	file.write((char*)(ROM_ID.data()), ROM_ID.size());
	// Save number of selected MBsSWs:
	unsigned int k = _MBSWmetaList.size();
	file.write((char*)(&k), sizeof(k));
	// Save individual MBsSWs:
	for (unsigned int i=0; i<k; i++)
	{
		file.write((char*)(&_MBSWmetaList.at(i).blockType), sizeof(_MBSWmetaList.at(i).blockType));
		file.write((char*)(&_MBSWmetaList.at(i).nativeIndex), sizeof(_MBSWmetaList.at(i).nativeIndex));
	}
	// Check for write errors:
	if (!file.good())
	{
		file.close();
		remove(filename.toLocal8Bit());
		errorMsg(tr("Save Error"), tr("Error: failed to save MB/SW list:\nA write error occured"));
		return false;
	}
	// Close the file:
	file.close();

	return true;
}


bool CUcontent_MBsSWs::loadMBsSWs(QString filename)
{
	std::string savedROM_ID = "";
	std::vector<MBSWmetadata_dt> MBSWmetaList;
	MBSWmetadata_dt tmpMBSWmd;
	unsigned int k = 0;
	// Check if setup() has already been called:
	if (_SSMPdev == NULL)
		return false;
	// Check if MB/SW monitoring is in progress:
	if (_MBSWreading)
	{
		errorMsg(tr("Load Error"), tr("Error: can't load a new MB/SW list:\nMB/SW-monitoring is in progress"));
		return false;
	}
	// Check if Control Unit provides at least one MB/SW:
	if (_supportedMBs.size() + _supportedSWs.size() < 1)
	{
		errorMsg(tr("Load Error"), tr("Error: can't load a MB/SW list:\nControl Unit provides no MBs/SWs"));
		return false;
	}
	// Get file name:
	if (!filename.size())
	{
		// Select file to load:
		QString default_filename = QCoreApplication::applicationDirPath();
		QDir dir(QCoreApplication::applicationDirPath());
		QStringList filenames = dir.entryList(QStringList("*.list"), QDir::Files | QDir::Readable, QDir::Time);
		if (filenames.size())
			default_filename += QDir::separator() + filenames.at(0);
		filename = QFileDialog::getOpenFileName(this, tr("Load MB/SW List"),
							default_filename,
							tr("FreeSSM MB/SW list files") + " (*.list)(*.list)");
		if (!filename.size())
			return false;
	}
	// Open file:
	std::ifstream file (filename.toLocal8Bit(), std::ios::in | std::ios::binary);
	if (!file.is_open())
	{
		errorMsg(tr("Load Error"), tr("Error: failed to load MB/SW list:\nCouldn't open the selected file"));
		return false;
	}
	// Get the ROM-ID of the current Control Unit:
	std::string ROM_ID = _SSMPdev->getROMID();
	if (ROM_ID.empty())
	{
		file.close();
		communicationError(tr("=> Couldn't read the ROM-ID for validating the saved MB/SW list."));
		return false;
	}
	// Read saved ROM-ID::
	size_t size_of_ROMstring;
	char *tmp_savedROM_ID = NULL;
	file.read((char*)(&size_of_ROMstring), sizeof(size_t));
	if (!file.good() || (size_of_ROMstring != ROM_ID.size()))
		goto err_invalid;
	tmp_savedROM_ID = new char[size_of_ROMstring];
	file.read((char*)(tmp_savedROM_ID), size_of_ROMstring);
	savedROM_ID = std::string(tmp_savedROM_ID, size_of_ROMstring);
	delete[] tmp_savedROM_ID;
	if (!file.good())
		goto err_invalid;
#ifdef __FSSM_DEBUG__
	std::cout << "CUcontent_MBsSWs::loadMBsSWs(): current ROM-ID: " << ROM_ID << "ROM-ID of the MB/SW list file: " << savedROM_ID << std::endl;
#endif
	// Check that the ROM-IDs match:
	if (savedROM_ID != ROM_ID)
	{
		file.close();
		errorMsg(tr("Load Error"), tr("Error: failed to load MB/SW list:\nThe saved ROM-ID does not match the Control Units ROM-ID"));
		return false;
	}
	// Read number of saved MBs/SWs:
	file.read((char*)(&k), sizeof(k));
#ifdef __FSSM_DEBUG__
	std::cout << "CUcontent_MBsSWs::loadMBsSWs(): saved MB/SW list size is " << k << std::endl;
#endif
	// Read saved MBs/SWs one by one:
	for (unsigned int i=0; i<k; i++)
	{
		file.read((char*)(&tmpMBSWmd.blockType), sizeof(tmpMBSWmd.blockType));
		file.read((char*)(&tmpMBSWmd.nativeIndex), sizeof(tmpMBSWmd.nativeIndex));
		if (file.good())
			MBSWmetaList.push_back( tmpMBSWmd );
		else
		{
#ifdef __FSSM_DEBUG__
			std::cout << "CUcontent_MBsSWs::loadMBsSWs(): error: only " << i << " of " << k << " MBs/SWs could be loaded" << std::endl;
#endif
			goto err_invalid;
		}
	}
	// Try to read 1 byte beyond the expected list end / file size:
	file.read((char*)(&tmpMBSWmd.blockType), 1);
	if (file.good())
	{
#ifdef __FSSM_DEBUG__
		std::cout << "CUcontent_MBsSWs::loadMBsSWs(): error: file is larger than expected" << std::endl;
#endif
		goto err_invalid;
	}
	// Validate loaded MBs/SWs and update table:
	if (!validateMBSWselection(MBSWmetaList))
		goto err_invalid;
	// Close the file
	file.close();
	// Save and display the new MB/SW selection:
	setMBSWselectionUnvalidated(MBSWmetaList);

	return true;

err_invalid:
	file.close();
	errorMsg(tr("Load Error"), tr("Error: failed to load MB/SW list:\nThe loaded MB/SW list is invalid"));
	return false;
}


void CUcontent_MBsSWs::labelStartStopButtonReadyForStart()
{
	// Set text+icon of start/stop-button:
	startstopmbreading_pushButton->setText(tr(" Start  "));
	QIcon startstopmbreadingicon(QString::fromUtf8(":/icons/chrystal/32x32/player_play.png"));
	QSize startstopmbreadingiconsize(24,24);
	startstopmbreading_pushButton->setIcon(startstopmbreadingicon);
	startstopmbreading_pushButton->setIconSize(startstopmbreadingiconsize);
}


void CUcontent_MBsSWs::moveUpMBsSWsOnTheTable()
{
	int nrofSelRows = 0;
	unsigned int rowToMoveDownIndex = 0;
	unsigned int rowToMoveDownTargetIndex = 0;
	// GET SELECTED ROWS:
	const std::vector<unsigned int> selectedMBSWIndexes = _valuesTableView->getSelectedTableWidgetRows();
	nrofSelRows = selectedMBSWIndexes.size();
	// CHECK AND CORRECT SELECTED ROWS:
	if ((nrofSelRows < 1) || (selectedMBSWIndexes.at(0) < 1) || (1 + selectedMBSWIndexes.at(0) > _MBSWmetaList.size()))
		return;	// Cancel, if moving up is not possible
	if ((selectedMBSWIndexes.at(0) + nrofSelRows) > _MBSWmetaList.size()) // if selection exceeds the end of the list
		nrofSelRows = _MBSWmetaList.size() - selectedMBSWIndexes.at(0);
	// NOTE: IN FACT WE ARE MOVING 1 ROW DOWN...
	// GET START AND TARGET INDEX OF THE ROW THAT WILL BE MOVED:
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
	// SCROLL TO POSTION OF FIRST SELECTED ROW:
	_valuesTableView->scrollMBSWtable(rowToMoveDownIndex);
}


void CUcontent_MBsSWs::moveDownMBsSWsOnTheTable()
{
	unsigned int rowToMoveUpIndex = 0;
	unsigned int rowToMoveUpTargetIndex = 0;
	// GET SELECTED ROWS:
	const std::vector<unsigned int> selectedMBSWIndexes = _valuesTableView->getSelectedTableWidgetRows();
	// CHECK AND CORRECT SELECTED ROWS:
	if ((selectedMBSWIndexes.size() < 1) | (selectedMBSWIndexes.at(selectedMBSWIndexes.size()-1)+1 >= _MBSWmetaList.size()))
		return;	// Cancel, if moving is not possible
	// NOTE: IN FACT WE ARE MOVING 1 ROW UP...
	// GET START AND TARGET INDEX OF THE ROW THAT WILL BE MOVED:
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
	// SCROLL TO POSITION OF LAST SELECTED ROW:
	_valuesTableView->scrollMBSWtable(rowToMoveUpIndex);
}


void CUcontent_MBsSWs::resetMinMaxTableValues()
{
	const size_t count = _lastValues.size();
	std::vector<QString> lastValueStr(count);
	std::vector<QString> lastUnitStr(count);
	// Setup new min/max values and output data:
	for (size_t k=0; k<count; ++k)
	{
		const MBSWvalue_dt& lastValue = _lastValues.at(k);
		MinMaxMBSWvalue_dt newMinMaxDataset;
		// Set min/max values to current value:
		newMinMaxDataset.minRawValue = lastValue.rawValue;
		newMinMaxDataset.maxRawValue = lastValue.rawValue;
		newMinMaxDataset.minScaledValueStr = lastValue.scaledStr;
		newMinMaxDataset.maxScaledValueStr = lastValue.scaledStr;
		_minmaxData.at(k) = newMinMaxDataset;

		const unsigned int tablePosIndex = _tableRowPosIndexes.at(k);
		lastValueStr.at(tablePosIndex) = !lastValue.scaledStr.isEmpty() ? lastValue.scaledStr : QString::number(lastValue.rawValue);
		lastUnitStr.at(tablePosIndex) = lastValue.unitStr;
	}
	// Display last values as current/min/max values:
	_valuesTableView->updateMBSWvalues(lastValueStr, lastValueStr, lastValueStr, lastUnitStr);
}


void CUcontent_MBsSWs::setDeleteButtonEnabledState()
{
	bool enable = (!_MBSWreading) && (_valuesTableView->getSelectedTableWidgetRows().size() > 0);
	mbswdelete_pushButton->setEnabled(enable);
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
	msgfont.setPointSize(9);
	msg.setFont( msgfont );
	msg.show();
	msg.exec();
	msg.close();
	emit error();
}


void CUcontent_MBsSWs::moveRefreshTimeUiElements()
{
	_MBSWrefreshTimeTitle_label->move(width() - 244, 3);
	_MBSWrefreshTimeValue_label->move(width() - 100, 3);
	_timemode_pushButton->move(width() - 29, 1);
}


void CUcontent_MBsSWs::resizeEvent(QResizeEvent *event)
{
	moveRefreshTimeUiElements();
	event->accept();
}


void CUcontent_MBsSWs::setupTimeModeUiElements()
{
	_MBSWrefreshTimeTitle_label = new QLabel("", this);
	_MBSWrefreshTimeTitle_label->setFixedWidth(140);
	_MBSWrefreshTimeTitle_label->setFixedHeight(16);
	_MBSWrefreshTimeTitle_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	_MBSWrefreshTimeValue_label = new QLabel("", this);
	_MBSWrefreshTimeValue_label->setFixedWidth(70);
	_MBSWrefreshTimeValue_label->setFixedHeight(16);
	_timemode_pushButton = new QPushButton(QIcon(QString::fromUtf8(":/icons/oxygen/16x16/chronometer.png")), "", this);
	_timemode_pushButton->setFixedWidth(20);
	_timemode_pushButton->setFixedHeight(20);
	_timemode_pushButton->setIconSize(QSize(12,12));
	moveRefreshTimeUiElements();
	_MBSWrefreshTimeTitle_label->show();
	_MBSWrefreshTimeValue_label->show();
	_timemode_pushButton->show();
}


void CUcontent_MBsSWs::errorMsg(QString title, QString message)
{
	QMessageBox msg( QMessageBox::Critical, title, message, QMessageBox::Ok, this);
	QFont msgfont = msg.font();
	msgfont.setPointSize(9);
	msg.setFont( msgfont );
	msg.show();
	msg.exec();
	msg.close();
}
