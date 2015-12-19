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
#include <chrono>
#include <mutex>

enum BombStatus 
{
	BOMB_NOT_PLANTED,
	BOMB_PLANTED,
	BOMB_EXPLODED,
	BOMB_DEFUSED
};

class DataModel
{
public:
	typedef std::chrono::high_resolution_clock clock;
	typedef QMutexLocker Lock;

	QMutex mutex;

	mode_choice_e mode;
	int maxBrightness;
	QColor staticColor;
	QColor fadeTwoFirstColor;
	QColor fadeTwoSecondColor;
	int fadeTwoSpeed;
	int fadeAllSpeed;
	QColor breatheColor;
	int breatheSpeed;

	bool csgoBombTimerFeatureEnabled;
	BombStatus csgoBombTimerStatus;
	int csgoBombTimerDuration;
	clock::time_point csgoBombPlantedTime;
	clock::time_point csgoBombLastBeep;
};

#endif