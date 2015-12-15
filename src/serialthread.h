#ifndef SERIALTHREAD_H__
#define SERIALTHREAD_H__

#include <QTHread>
#include <QtSerialPort/QtSerialPort>
#include "datamodel.h"

class SerialThread : public QThread
{
	Q_OBJECT
public:
	SerialThread(QSerialPortInfo portinfo, int baudrate, DataModel *data);
	void stop();
	virtual void run();

signals:
	void outputChanged(QColor color);

private:
	QSerialPortInfo m_portinfo;
	int m_baudrate;
	DataModel *m_data;
	volatile bool m_stop;
};

#endif // SERIALTHREAD_H__