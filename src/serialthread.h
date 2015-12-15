// src/serialthread.h
//----------------------------------
// RGB-LED Controller Source Code
// Copyright (c) 2015 Marius Graefe
// All rights reserved
// Contact: stuff@mgraefe.de
//----------------------------------

#pragma once
#ifndef rgbledsrc__serialthread_H__
#define rgbledsrc__serialthread_H__

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

#endif // rgbledsrc__serialthread_H__