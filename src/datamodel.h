// src/datamodel.h
//----------------------------------
// RGB-LED Controller Source Code
// Copyright (c) 2015 Marius Graefe
// All rights reserved
// Contact: stuff@mgraefe.de
//----------------------------------

#pragma once
#ifndef rgbledsrc__datamodel_H__
#define rgbledsrc__datamodel_H__

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