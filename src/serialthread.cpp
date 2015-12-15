
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
		clamp(c.blueF() * f), clamp(c.alphaF() * f));
}

QColor operator+(const QColor &a, const QColor &b)
{
	return QColor::fromRgbF(clamp(a.redF() + b.redF()), clamp(a.greenF() + b.greenF()),
		clamp(a.blueF() + b.blueF()), clamp(a.alphaF() + b.alphaF()));
}

QColor mixColor(const QColor &col0, const QColor &col1, float factor)
{
	return col0 * factor + col1 * (1.0f - factor);
}

void SerialThread::run()
{
	QSerialPort serialPort(m_portinfo);
	serialPort.setBaudRate(m_baudrate);
	if (!serialPort.open(QIODevice::WriteOnly))
	{
		QMessageBox::critical(NULL, tr("Error opening serial port"),
			tr("Could not open the specified serial port. Is it already in use?"));
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
		
		time += deltaT * (outSpeed / 1000.0);
		QColor finalColor = outColor * (m_data->maxBrightness / 1000.0f);
		finalColor.setAlphaF(1.0);

		const unsigned char syncByte = 0xAA;
		unsigned char data[4] = { syncByte, finalColor.red(), finalColor.green(), finalColor.blue() };
		qint64 bytesWritten = serialPort.write((char*)data, 4);
		if (bytesWritten == -1)
		{
			QMessageBox::critical(NULL, tr("Error writing to serial port"),
				tr("Could not write to the specified serial port."));
			return;
		}

		emit outputChanged(finalColor);

		msleep(50);
	}
}
