// src/modes.h
//----------------------------------
// RGB-LED Controller Source Code
// Copyright (c) 2015 Marius Graefe
// All rights reserved
// Contact: stuff@mgraefe.de
//----------------------------------

#pragma once
#ifndef rgbledsrc__modes_H__
#define rgbledsrc__modes_H__

enum mode_choice_e
{
	MODE_INVALID = 0,
	MODE_STATIC,
	MODE_FADE_TWO,
	MODE_FADE_ALL,
	MODE_BREATHE,

	MODE_LAST, //Always last
};

#endif // rgbledsrc__modes_H__