#include "libFSSM.h"


bool libFSSM::raw2scaled(unsigned int rawValue, QString scaleformula, char precision, QString *scaledValueStr)
{
	bool success = false;
	double scaledValue = 0;

	if ( scaleformula.contains('=') )
	{
		success = raw2scaledByDirectAssociation(rawValue, scaleformula, scaledValueStr);
	}
	else if ( raw2scaledByCalculation( rawValue, scaleformula, &scaledValue ) )
	{
		*scaledValueStr = QString::number(scaledValue, 'f', precision);
		success = true;
	}
	else
		return false;
	return success;
}


bool libFSSM::raw2scaledByDirectAssociation(unsigned int rawValue, QString scaleformula, QString *scaledValueStr)
{
	unsigned int nrofDAs = 0;
	QString defstr = "";
	QString rvstr = "";
	bool da_success = false;
	unsigned int m = 0;

	nrofDAs = 1 + (scaleformula.count(','));
	for (m=0; m<nrofDAs; m++)
	{
		// Get next allocation string:
		defstr = scaleformula.section(',',m,m);
		// Get scaled value part of allocation string:
		rvstr = defstr.section('=', 0, 0);
		// Check if raw value matches allocation string:
		if ( rawValue == rvstr.toUInt() )
		{
			// Get allocated value:
			(*scaledValueStr) = defstr.section('=', 1, 1);
			da_success = true;
			break;
		}
	}
	return da_success;
}


bool libFSSM::raw2scaledByCalculation(unsigned int rawValue, QString scaleformula, double *scaledValue)
{
	if (scaleformula.size() < 1) return false;
	*scaledValue = rawValue;
	// CHECK IF FORMULA BEGINS WITH A VALID DATA TYPE MODIFIER:
	if (scaleformula.startsWith("s", Qt::CaseInsensitive))
	{
		// VALIDATE BIT SIZE INFORMATION:
		int bitsize = 0;
		if (scaleformula.at(1) == '8')
		{
			bitsize = 8;
			scaleformula.remove(0,2);
		}
		else if (scaleformula.mid(1,2) == "16")
		{
			bitsize = 16;
			scaleformula.remove(0,3);
		}
		else
		{
			return false;
		}
		// INTERPRETE AS SIGNED:
		if (rawValue > pow(2,bitsize-1) - 1)	// if MSB is set
			*scaledValue = rawValue - pow(2, bitsize); // substract (half of range + 1)
	}
	// SCALE:
	return scale(*scaledValue, scaleformula, false, scaledValue);
}


bool libFSSM::scaled2raw(QString scaledValueStr, QString scaleformula, unsigned int *rawValue)
{
	bool success = false;
	double scaledValue = 0;

	if ( scaleformula.contains('=') )
	{
		success = scaled2rawByDirectAssociation(scaledValueStr, scaleformula, rawValue);
	}
	else
	{
		scaledValue = scaledValueStr.toDouble(&success);
		if (success)
			success = scaled2rawByCalculation(scaledValue, scaleformula, rawValue);
	}
	return success;
}


bool libFSSM::scaled2rawByDirectAssociation(QString scaledValueStr, QString scaleformula, unsigned int *rawValue)
{
	unsigned int nrofDAs = 0;
	QString defstr = "";
	QString svstr = "";
	bool ok = false;
	unsigned int m = 0;

	nrofDAs = 1 + (scaleformula.count(','));
	for (m=0; m<nrofDAs; m++)
	{
		// Get next allocation string:
		defstr = scaleformula.section(',',m,m);
		// Get scaled value part of allocation string:
		svstr = defstr.section('=', 1, 1);
		// Check if raw value matches allocation string:
		if ( scaledValueStr == svstr )
		{
			// Get allocated value:
			(*rawValue) = defstr.section('=', 0, 0).toUInt(&ok, 10);
			break;
		}
	}
	return ok;
}


bool libFSSM::scaled2rawByCalculation(double scaledValue, QString scaleformula, unsigned int *rawValue)
{
	double wval = scaledValue;
	int bitsize = 0;

	if (scaleformula.size() < 1) return false;
	// CHECK IF FORMULA BEGINS WITH A VALID DATA TYPE MODIFIER:
	if (scaleformula.startsWith("s", Qt::CaseInsensitive))
	{
		// VALIDATE BIT SIZE INFORMATION:
		if (scaleformula.at(1) == '8')
		{
			bitsize = 8;
			scaleformula.remove(0,2);
		}
		else if (scaleformula.mid(1,2) == "16")
		{
			bitsize = 16;
			scaleformula.remove(0,3);
		}
		else
		{
			return false;
		}
	}
	// SCALE:
	if (!scale(wval, scaleformula, true, &wval))
		return false;
	// DATA TYPE CONVERSION:
	if (bitsize > 0)
	{
		// CONVERT TO UNSIGNED RAW VALUE:
		if (wval < 0)
			wval += pow(2, bitsize);
	}
	// PROCESS/CONVERT/VALIDATE SCALED VALUE:
	wval = round(wval);
	if ((wval < 0) || (wval > 65535))
		return false;
	*rawValue = static_cast<unsigned int>(wval);
	return true;
}


bool libFSSM::scale(double value_in, QString formula, bool inverse, double * value_out)
{
	double value_scaled;
	unsigned char operatorindex[10] = {0,};
	int nrofoperators = 0;
	int calcstep = 0;
	unsigned char opindexnr;
	char opchar = 0;
	unsigned char tmpvaluestrlen = 0;
	QString tmpvaluestr("");
	double tmpvalue = 0;
	bool ok = false;
	// VALIDATE FIRST OPERATOR AND SAVE POSITION:
	if ((formula.at(0) == '+') || (formula.at(0) == '-') || (formula.at(0) == '*') || (formula.at(0) == '/'))
	{
		operatorindex[nrofoperators] = 0;
		nrofoperators++;
	}
	else
		return false;
	// CHECK REST OF THE FORMULA AND GET OPERATOR POSITIONS:
	for (unsigned char charindex=1; charindex<formula.size(); charindex++)
	{
		if ((formula.at(charindex) == '+') || (formula.at(charindex) == '-') || (formula.at(charindex) == '*') || (formula.at(charindex) == '/'))
		{
			// Check for consecutive operators
			if ((charindex - operatorindex[nrofoperators-1]) > 1)
			{
				operatorindex[nrofoperators] = charindex;
				nrofoperators++;
			}
			/* NOTE: No further checks necessary, conversion to double will fail
			 *       - if we have more then two consecutive operators
			 *       - or if the second operator is not a + or - (prefix)
			 */
		}
		else if (!formula.at(charindex).isDigit())
		{
 				if (charindex == formula.size()) return false;
				if (!( (formula.at(charindex) == '.') && formula.at(charindex-1).isDigit() && formula.at(charindex+1).isDigit() ))
					return false;
		}
	}
	// DO CALCULATION:
	value_scaled = value_in;
	for (calcstep=0; calcstep<nrofoperators; calcstep++)
	{
		if (!inverse)
			opindexnr = calcstep;
		else
			opindexnr = nrofoperators - 1 - calcstep;
		// EXTRACT NEXT OPERATOR:
		opchar = formula.at( (operatorindex[opindexnr]) ).toAscii();
		// GET LENGTH OF VALUE STRING:
		if (opindexnr == (nrofoperators - 1))	// IF LAST OPERATION
			tmpvaluestrlen = formula.size() - operatorindex[opindexnr] -1;
		else
			tmpvaluestrlen = operatorindex[opindexnr+1] - operatorindex[opindexnr] -1;
		// CHECK VALUE STRING LENGTH:
		if (tmpvaluestrlen == 0) return false;
		// EXTRACT VALUE STRING AND CONVERT TO DOUBLE:
		tmpvaluestr = formula.mid( operatorindex[opindexnr]+1, tmpvaluestrlen );
		tmpvalue = tmpvaluestr.toDouble( &ok );
		if (!ok || (tmpvalue == 0)) return false;
		// DO CALCUALTION STEP:
		if (!inverse)
		{
			switch (opchar)
			{
				case '+':
					value_scaled += tmpvalue;
					break;
				case '-':
					value_scaled -= tmpvalue;
					break;
				case '*':
					value_scaled *= tmpvalue;
					break;
				case '/':
					value_scaled /= tmpvalue;
					break;
				default:
					return false;
			}
		}
		else
		{
			switch (opchar)
			{
				case '+':
					value_scaled -= tmpvalue;
					break;
				case '-':
					value_scaled += tmpvalue;
					break;
				case '*':
					value_scaled /= tmpvalue;
					break;
				case '/':
					value_scaled *= tmpvalue;
					break;
				default:
					return false;
			}
		}
	}
	*value_out = value_scaled;
	return true;
}

