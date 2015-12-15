#ifndef COLORDISPLAYWIDGET_H__
#define COLORDISPLAYWIDGET_H__

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
