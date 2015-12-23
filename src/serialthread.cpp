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

template<typename T1, typename T2>
double toSeconds(const std::chrono::duration<T1, T2> &dur)
{
	using namespace std::chrono;
	return double(duration_cast<milliseconds>(dur).count()) / 1000.0;
}

std::chrono::milliseconds fromSeconds(double seconds)
{
	return std::chrono::milliseconds(int(seconds * 1000.0));
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

		{
			DataModel::Lock lock(&m_data->mutex);
			if (m_data->csgoBombTimerStatus != BOMB_NOT_PLANTED)
			{
				handleBombTimer(outColor);
			}
			else if (m_data->mode == MODE_FADE_TWO)
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
		}
		time += deltaT * pow(outSpeed/1000.0, 3.0) * 10.0;
		float gamma = 2.2f;
		QColor finalColor = outColor * (m_data->maxBrightness / 1000.0f);
		QColor gammaCorrected = pow(finalColor, gamma);

		const unsigned char syncByte = 0xAA;
		char data[] = { syncByte, gammaCorrected.red(), gammaCorrected.green(), gammaCorrected.blue() };
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


double lerp(double val, double x0, double x1, double y0, double y1)
{
	return y0 + (y1 - y0) * (val - x0) / (x1 - x0);
}



void SerialThread::handleBombTimer(QColor &outColor)
{
	auto now = DataModel::clock::now();
	double secondsPassed = toSeconds(now - m_data->csgoBombPlantedTime);
	double secondsLeft = m_data->csgoBombTimerDuration - secondsPassed;
	//double beepDist = (secondsLeft + 0.0) / m_data->csgoBombTimerDuration;
	double beepDist = lerp(secondsLeft, m_data->csgoBombTimerDuration, 0.0, 1.0, 0.1);
	auto nextBeep = m_data->csgoBombLastBeep + fromSeconds(beepDist);
	if (m_data->csgoBombTimerStatus == BOMB_DEFUSED)
	{
		if (secondsPassed < 3.0)
			outColor = QColor(30, 255, 30);
		else
			m_data->csgoBombTimerStatus = BOMB_NOT_PLANTED;
	}
	else if (m_data->csgoBombTimerStatus != BOMB_NOT_PLANTED)
	{
		if (secondsLeft > 1.0) // Normal blinking
		{
			double timeSinceBeep = toSeconds(now - m_data->csgoBombLastBeep);
			if (now > m_data->csgoBombLastBeep + fromSeconds(beepDist))
			{
				timeSinceBeep -= beepDist;
				m_data->csgoBombLastBeep += fromSeconds(beepDist);
			}
			outColor = QColor(255, 0, 0) * (1.0 - timeSinceBeep * 6.0);
		}
		else if (secondsLeft > -1.0)
		{
			outColor = QColor(255, 255, 255);
		}
		else if (m_data->csgoBombTimerStatus == BOMB_EXPLODED)
		{
			outColor = QColor(255, 100, 100) * (((sin(secondsLeft * 40.0) * 0.5 + 1.0) +
				double(rand() % 100 - 50) * 0.01 * 0.4));
		}
	}
}