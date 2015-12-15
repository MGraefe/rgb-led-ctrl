// src/ColorDisplayWidget.h
//----------------------------------
// RGB-LED Controller Source Code
// Copyright (c) 2015 Marius Graefe
// All rights reserved
// Contact: stuff@mgraefe.de
//----------------------------------

#pragma once
#ifndef rgbledsrc__ColorDisplayWidget_H__
#define rgbledsrc__ColorDisplayWidget_H__

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>

class ColorDisplayWidget : public QWidget
{
	Q_OBJECT
public:
	ColorDisplayWidget(QWidget *parent = 0);
	void setColor(const QColor &color);

protected:
	void paintEvent(QPaintEvent *e);

private:
	QColor m_color;
};

#endif
