
#include "stdafx.h"
#include "ColorDisplayWidget.h"

ColorDisplayWidget::ColorDisplayWidget(QWidget *parent) :
	QWidget(parent)
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
