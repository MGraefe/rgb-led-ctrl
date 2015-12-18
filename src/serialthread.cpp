// src/serialthread.cpp
//----------------------------------
// RGB-LED Controller Source Code
// Copyright (c) 2015 Marius Graefe
// All rights reserved
// Contact: stuff@mgraefe.de
//----------------------------------


#include "stdafx.h"
#include "serialthread.h"

SerialThread::SerialThread(QSerialPortInfo portinfo, int baudrate, DataModel *data) :
	QThread(NULL),
	m_baudrate(baudrate),
	m_data(data),
	m_stop(false)
{
	m_portinfo = portinfo;
}

void SerialThread::stop()
{
	m_stop = true;
	wait();
}

float clamp(float val, float min = 0.0f, float max = 1.0f)
{
	return val < min ? min : val > max ? max : val;
}

QColor operator*(const QColor &c, float f)
{
	return QColor::fromRgbF(clamp(c.redF() * f), clamp(c.greenF() * f),
		clamp(c.blueF() * f));
}

QColor operator+(const QColor &a, const QColor &b)
{
	return QColor::fromRgbF(clamp(a.redF() + b.redF()), clamp(a.greenF() + b.greenF()),
		clamp(a.blueF() + b.blueF()));
}

QColor mixColor(const QColor &col0, const QColor &col1, float factor)
{
	return col0 * factor + col1 * (1.0f - factor);
}

QColor pow(const QColor &c, float exp)
{
	return QColor::fromRgbF(clamp(pow(c.redF(), exp)),
		clamp(pow(c.greenF(), exp)), clamp(pow(c.blueF(), exp)));
}

void SerialThread::run()
{
	QSerialPort serialPort;
	serialPort.setPortName(m_portinfo.portName());
	if (!serialPort.open(QIODevice::WriteOnly))
	{
		//QMessageBox::critical(NULL, tr("Error opening serial port"),
		//	tr("Could not open the specified serial port. Is it already in use?"));
		qDebug() << "Error opening serial port: " << serialPort.errorString() << "is it already in use?";
		return;
	}
	if (!serialPort.setBaudRate(m_baudrate) ||
		!serialPort.setDataBits(QSerialPort::Data8) ||
		!serialPort.setParity(QSerialPort::NoParity) ||
		!serialPort.setStopBits(QSerialPort::OneStop) ||
		!serialPort.setFlowControl(QSerialPort::NoFlowControl) ||
		!serialPort.setDataTerminalReady(true) ||
		!serialPort.setRequestToSend(true))
	{
		qDebug() << "Error settings port parameters";
		return;
	}

	double time = 0.0;
	while (!m_stop)
	{
		double deltaT = 0.02;
		float clampedTime = (float)fmod(time, 2.0 * M_PI);
		QColor outColor;
		int outSpeed;

		if (m_data->mode == MODE_FADE_TWO)
		{
			float fadeFactor = sin(clampedTime * 2.0f * M_PI) * 0.5f + 0.5f;
			outColor = mixColor(m_data->fadeTwoFirstColor, m_data->fadeTwoSecondColor, fadeFactor);
			outSpeed = m_data->fadeTwoSpeed;
		}
		else if (m_data->mode == MODE_FADE_ALL)
		{
			float angle = (float)fmod(time, 1.0);
			outColor = QColor::fromHsvF(angle, 1.0f, 1.0f);
			outSpeed = m_data->fadeAllSpeed;
		}
		else if (m_data->mode == MODE_BREATHE)
		{
			float factor = sin(clampedTime * 2.0f * M_PI) * 0.5f + 0.5f;
			outColor = m_data->breatheColor * factor;
			outSpeed = m_data->breatheSpeed;
		}
		else // MODE_STATIC
		{
			outColor = m_data->staticColor;
			outSpeed = 0;
		}
		
		time += deltaT * pow(outSpeed/1000.0, 3.0) * 10.0;
		float gamma = 2.2f;
		QColor finalColor = pow(outColor * (m_data->maxBrightness / 1000.0f), gamma);

		const unsigned char syncByte = 0xAA;
		char data[] = { syncByte, finalColor.red(), finalColor.green(), finalColor.blue() };
		if (serialPort.isWritable())
		{
			qint64 bytesWritten = serialPort.write(data, 4);
			if (bytesWritten == -1)
			{
				QMessageBox::critical(NULL, tr("Error writing to serial port"),
					tr("Could not write to the specified serial port."));
				return;
			}
			serialPort.waitForBytesWritten(100);
		}

		emit outputChanged(finalColor);

		msleep(20);
	}
}
