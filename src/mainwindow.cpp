// src/mainwindow.cpp
//----------------------------------
// RGB-LED Controller Source Code
// Copyright (c) 2015 Marius Graefe
// All rights reserved
// Contact: stuff@mgraefe.de
//----------------------------------

#include "stdafx.h"
#include "mainwindow.h"
#include <QSettings>
#include <QtSerialPort/QtSerialPort>
#include <QColorDialog>
#include <QJsonDocument>
#include <functional>

MainWindow::MainWindow(QWidget *parent): 
	QMainWindow(parent),
	m_serialThread(NULL)
{
	ui.setupUi(this);

	// System Tray Icon
	m_trayIcon = new QSystemTrayIcon(this->windowIcon(), this);
	m_trayIcon->setToolTip(tr("RGB Led Controller"));
	QAction *trayShowHideAction = new QAction(tr("Show/Hide"), m_trayIcon);
	QAction *trayExitAction = new QAction(tr("Close"), m_trayIcon);
	connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT(onSysTrayActivated(QSystemTrayIcon::ActivationReason)));
	connect(trayShowHideAction, SIGNAL(triggered()), SLOT(onSysTrayShowHideClicked()));
	connect(trayExitAction, SIGNAL(triggered()), SLOT(onSysTrayExitClicked()));
	QMenu *trayMenu = new QMenu();
	trayMenu->addAction(trayShowHideAction);
	trayMenu->addAction(trayExitAction);
	m_trayIcon->setContextMenu(trayMenu);
	m_trayIcon->show();
	
	// Serial port settings
	m_serialPorts = QSerialPortInfo::availablePorts();
	for (const QSerialPortInfo &port : m_serialPorts)
		ui.comPortCombo->addItem(port.portName());

	ui.baudRateCombo->addItems(QStringList({"300", "600", "1200", "2400",
		"4800", "9600", "14400", "19200", "28800", "38400", "57600", "115200"}));

	// Color display widget
	m_colorDisplayWidget = new ColorDisplayWidget(this);
	m_colorDisplayWidget->setMinimumSize(QSize(0, 30));
	ui.horizontalLayout_7->addWidget(m_colorDisplayWidget);

	// Mode Radio Buttons
	QButtonGroup *group = new QButtonGroup(this);
	group->addButton(ui.staticColorRadio);
	group->addButton(ui.fadeTwoRadio);
	group->addButton(ui.fadeAllRadio);
	group->addButton(ui.breatheRadio);

	// Connect all the signals!
	connect(group, SIGNAL(buttonClicked(QAbstractButton*)), SLOT(onModeButtonPressed(QAbstractButton*)));
	connect(ui.maxBrightnessSlider, SIGNAL(valueChanged(int)), SLOT(onMaxBrightnessChanged(int)));
	connect(ui.staticColorButton, SIGNAL(clicked()), SLOT(onStaticColorClicked()));
	connect(ui.fadeTwoFirstColorButton, SIGNAL(clicked()), SLOT(onFadeTwoFirstColorClicked()));
	connect(ui.fadeTwoSecondColorButton, SIGNAL(clicked()), SLOT(onFadeTwoSecondColorClicked()));
	connect(ui.fadeTwoSpeedSlider, SIGNAL(valueChanged(int)), SLOT(onFadeTwoSpeedChanged(int)));
	connect(ui.fadeAllSpeedSlider, SIGNAL(valueChanged(int)), SLOT(onFadeAllSpeedChanged(int)));
	connect(ui.breatheColorButton, SIGNAL(clicked()), SLOT(onBreatheColorClicked()));
	connect(ui.breatheSpeedSlider, SIGNAL(valueChanged(int)), SLOT(onBreatheSpeedChanged(int)));
	connect(ui.portSettingsApplyButton, SIGNAL(clicked()), SLOT(onPortSettingsApplyClicked()));
	connect(ui.actionAbout, SIGNAL(triggered(bool)), SLOT(onAboutClicked()));

	// Read the settings from registry/file
	readSettings();

	// Create the serial thread
	createSerialThread();

	std::function<void(const char*, size_t)> dataEndPoint = std::bind(
		&MainWindow::processJsonData, this, std::placeholders::_1, std::placeholders::_2);
	m_httpServer = new HttpPostServer(dataEndPoint, 16362, true);
	if (data.csgoBombTimerFeatureEnabled)
		m_httpServer->start();
}

MainWindow::~MainWindow()
{
	if (m_httpServer)
	{
		m_httpServer->stop();
		delete m_httpServer;
	}
}

mode_choice_e MainWindow::parseActiveMode()
{
	if (ui.fadeTwoRadio->isChecked())
		return MODE_FADE_TWO;
	if (ui.fadeAllRadio->isChecked())
		return MODE_FADE_ALL;
	if (ui.breatheRadio->isChecked())
		return MODE_BREATHE;
	return MODE_STATIC;
}

void MainWindow::setActiveMode(mode_choice_e mode)
{
	data.mode = mode;
	ui.staticColorRadio->setChecked(mode == MODE_STATIC);
	ui.fadeTwoRadio->setChecked(mode == MODE_FADE_TWO);
	ui.fadeAllRadio->setChecked(mode == MODE_FADE_ALL);
	ui.breatheRadio->setChecked(mode == MODE_BREATHE);
}

void MainWindow::safeSettings()
{
	QSettings settings;
	settings.setValue("mode", (int)parseActiveMode());
	settings.setValue("maxBrightness", data.maxBrightness);
	settings.setValue("staticColorColor", data.staticColor);
	settings.setValue("fadeTwoFirstColor", data.fadeTwoFirstColor);
	settings.setValue("fadeTwoSecondColor", data.fadeTwoSecondColor);
	settings.setValue("fadeTwoFadeSpeed", data.fadeTwoSpeed);
	settings.setValue("fadeAllFadeSpeed", data.fadeAllSpeed);
	settings.setValue("breatheColor", data.breatheColor);
	settings.setValue("breatheSpeed", data.breatheSpeed);
	settings.setValue("csgoBombTimerEnabled", data.csgoBombTimerFeatureEnabled);

	settings.setValue("comPort", ui.comPortCombo->currentText());
	settings.setValue("baudRate", ui.baudRateCombo->currentText().toInt());
}

void MainWindow::readSettings()
{
	QSettings settings;
	mode_choice_e mode = (mode_choice_e)settings.value("mode", MODE_FADE_ALL).toInt();
	if (mode <= MODE_INVALID || mode >= MODE_LAST)
		mode = MODE_FADE_ALL;
	setActiveMode(mode);

	data.maxBrightness = settings.value("maxBrightness", 900).toInt();
	data.staticColor = settings.value("staticColorColor", QColor(255, 0, 0)).value<QColor>();
	data.fadeTwoFirstColor = settings.value("fadeTwoFirstColor", QColor(0, 255, 0)).value<QColor>();
	data.fadeTwoSecondColor = settings.value("fadeTwoSecondColor", QColor(0, 0, 255)).value<QColor>();
	data.fadeTwoSpeed = settings.value("fadeTwoFadeSpeed", 300).toInt();
	data.fadeAllSpeed = settings.value("fadeAllFadeSpeed", 300).toInt();
	data.breatheColor = settings.value("breatheColor", QColor(0, 0, 255)).value<QColor>();
	data.breatheSpeed = settings.value("breatheSpeed", 300).toInt();
	data.csgoBombTimerFeatureEnabled = settings.value("csgoBombTimerEnabled", true).toBool();
	data.csgoBombTimerDuration = settings.value("csgoBombTimerDuration", 40).toInt();
	data.csgoBombTimerStatus = BOMB_NOT_PLANTED;

	ui.comPortCombo->setCurrentText(settings.value("comPort", "COM3").toString());
	ui.baudRateCombo->setCurrentText(settings.value("baudRate", "9600").toString());

	writeDataToGui();
}

void MainWindow::writeDataToGui()
{
	ui.maxBrightnessSlider->setValue(data.maxBrightness);
	updateButtonColor(ui.staticColorButton, data.staticColor);
	updateButtonColor(ui.fadeTwoFirstColorButton, data.fadeTwoFirstColor);
	updateButtonColor(ui.fadeTwoSecondColorButton, data.fadeTwoSecondColor);
	ui.fadeTwoSpeedSlider->setValue(data.fadeTwoSpeed);
	ui.fadeAllSpeedSlider->setValue(data.fadeAllSpeed);
	updateButtonColor(ui.breatheColorButton, data.breatheColor);
	ui.breatheSpeedSlider->setValue(data.breatheSpeed);
}

void MainWindow::updateButtonColor(QPushButton *button, QColor color)
{
	int brightness = color.red() * 0.299f + color.green() * 0.587f + color.blue() * 0.114f;
	QColor textColor = brightness < 105 ? Qt::white : Qt::black;
	button->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; }").arg(
		color.name(), textColor.name()));
}


void MainWindow::createSerialThread()
{
	if (m_serialThread)
	{
		m_serialThread->stop();
		delete m_serialThread;
	}

	QSerialPortInfo info = m_serialPorts[ui.comPortCombo->currentIndex()];
	bool baudOk = false;
	int baud = ui.baudRateCombo->currentText().toInt(&baudOk);
	if (!baudOk)
	{
		baud = 9600;
		ui.baudRateCombo->setCurrentText("9600");
	}

	m_serialThread = new SerialThread(info, baud, &data);
	connect(m_serialThread, SIGNAL(outputChanged(QColor)), this, SLOT(onOutputColorChanged(QColor)));
	m_serialThread->start();
}


void MainWindow::openButtonColorDialog(QPushButton *button, QColor &color)
{
	QColor newColor = QColorDialog::getColor(color, this, tr("Choose color"));
	if (newColor.isValid())
	{
		color = newColor;
		updateButtonColor(button, color);
		safeSettings();
	}
}

void MainWindow::closeEvent(QCloseEvent *evt)
{
	safeSettings();
	QMainWindow::closeEvent(evt);
}

void MainWindow::changeEvent(QEvent *evt)
{
	// Hold back the minimize event to process other messages,
	// Needed to make minimize to tray work
	if (evt->type() == QEvent::WindowStateChange && windowState() & Qt::WindowMinimized)
		QTimer::singleShot(0, this, SLOT(hide()));

	QMainWindow::changeEvent(evt);
}

// gets called from different thread
void MainWindow::processJsonData(const char *data, size_t size)
{
	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(QByteArray(data, size), &parseError);
	if (parseError.error != QJsonParseError::NoError)
	{
		qDebug() << parseError.errorString();
		return;
	}

	QJsonObject json = doc.object();
	if (json["added"].toObject()["round"].toObject()["bomb"].toBool(false) &&
		json["round"].toObject()["bomb"].toString() == "planted")
	{
		qDebug() << "Bomb planted";
		DataModel::Lock lock(&this->data.mutex);
		this->data.csgoBombPlantedTime = this->data.csgoBombLastBeep = DataModel::clock::now() - std::chrono::milliseconds(500);
		this->data.csgoBombTimerStatus = BOMB_PLANTED;
	}

	if (json["previously"].toObject()["round"].toObject()["bomb"].toString() == "planted")
	{
		QString bombStatus = json["round"].toObject()["bomb"].toString();
		if (bombStatus == "defused")
		{
			qDebug() << "Bomb defused";
			DataModel::Lock lock(&this->data.mutex);
			this->data.csgoBombTimerStatus = BOMB_DEFUSED;
			this->data.csgoBombPlantedTime = DataModel::clock::now();
		}
		else if (bombStatus == "exploded")
		{
			qDebug() << "Bomb exploded";
			this->data.csgoBombTimerStatus = BOMB_EXPLODED;
		}
		else
		{
			// Bomb timer status is reset by the serial thread
			// Is this good? maybe not..
		}
	}
}

void MainWindow::onModeButtonPressed(QAbstractButton *button)
{
	data.mode = parseActiveMode();
}

void MainWindow::onMaxBrightnessChanged(int value)
{
	data.maxBrightness = value;
}

void MainWindow::onStaticColorClicked()
{
	openButtonColorDialog(ui.staticColorButton, data.staticColor);
}

void MainWindow::onFadeTwoFirstColorClicked()
{
	openButtonColorDialog(ui.fadeTwoFirstColorButton, data.fadeTwoFirstColor);
}

void MainWindow::onFadeTwoSecondColorClicked()
{
	openButtonColorDialog(ui.fadeTwoSecondColorButton, data.fadeTwoSecondColor);
}

void MainWindow::onFadeTwoSpeedChanged(int value)
{
	data.fadeTwoSpeed = value;
}

void MainWindow::onFadeAllSpeedChanged(int value)
{
	data.fadeAllSpeed = value;
}

void MainWindow::onBreatheColorClicked()
{
	openButtonColorDialog(ui.breatheColorButton, data.breatheColor);
}

void MainWindow::onBreatheSpeedChanged(int value)
{
	data.breatheSpeed = value;
}

void MainWindow::onPortSettingsApplyClicked()
{
	createSerialThread();
}

void MainWindow::onOutputColorChanged(QColor color)
{
	m_colorDisplayWidget->setColor(color);
}

void MainWindow::onAboutClicked()
{
	QMessageBox::about(this, tr("About RGB Led Controller"),
		QString::fromLatin1("RGB Led Controller\n\nCopyright (c) 2015 Marius Gr\344fe"));
}

void MainWindow::onSysTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::DoubleClick)
		onSysTrayShowHideClicked();
}

void MainWindow::onSysTrayShowHideClicked()
{
	if (isVisible())
	{
		hide();
	}
	else
	{
		show();
		setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
	}
}

void MainWindow::onSysTrayExitClicked()
{
	close();
}