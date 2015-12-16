// src/ColorDisplayWidget.cpp
//----------------------------------
// RGB-LED Controller Source Code
// Copyright (c) 2015 Marius Graefe
// All rights reserved
// Contact: stuff@mgraefe.de
//----------------------------------


#include "stdafx.h"
#include "ColorDisplayWidget.h"

ColorDisplayWidget::ColorDisplayWidget(QWidget *parent) :
	QWidget(parent),
	m_color(100, 100, 100)
{}

void ColorDisplayWidget::setColor(const QColor &color)
{
	m_color = color;
	repaint();
}

void ColorDisplayWidget::paintEvent(QPaintEvent *e)
{
	QRect rect = e->rect();
	QPainter painter(this);
	painter.setPen(Qt::black);
	painter.setBrush(m_color);
	painter.drawRect(rect);
}
