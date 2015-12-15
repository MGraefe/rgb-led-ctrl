#ifndef DATAMODEL_H__
#define DATAMODEL_H__

#include <QColor>
#include "modes.h"

class DataModel
{
public:
	mode_choice_e mode;
	int maxBrightness;
	QColor staticColor;
	QColor fadeTwoFirstColor;
	QColor fadeTwoSecondColor;
	int fadeTwoSpeed;
	int fadeAllSpeed;
	QColor breatheColor;
	int breatheSpeed;
};

#endif