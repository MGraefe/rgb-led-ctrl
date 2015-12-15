#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include "ui_mainwindow.h"
#include "modes.h"
#include <QtSerialPort/QtSerialPort>
#include "datamodel.h"
#include "serialthread.h"
#include "ColorDisplayWidget.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

	void safeSettings();
	void readSettings();
	mode_choice_e parseActiveMode();

protected slots:
	void onModeButtonPressed(QAbstractButton *button);
	void onMaxBrightnessChanged(int value);
	void onStaticColorClicked();
	void onFadeTwoFirstColorClicked();
	void onFadeTwoSecondColorClicked();
	void onFadeTwoSpeedChanged(int value);
	void onFadeAllSpeedChanged(int value);
	void onBreatheColorClicked();
	void onBreatheSpeedChanged(int value);
	void onPortSettingsApplyClicked();
	void onOutputColorChanged(QColor color);

protected:
	void closeEvent(QCloseEvent *evt);

private:
	void writeDataToGui();
	void setActiveMode(mode_choice_e mode);
	void updateButtonColor(QPushButton *button, QColor color);
	void openButtonColorDialog(QPushButton *button, QColor &color);
	void createSerialThread();

	Ui::MainWindowClass ui;
	DataModel data;
	QList<QSerialPortInfo> m_serialPorts;
	SerialThread *m_serialThread;
	ColorDisplayWidget *m_colorDisplayWidget;
};

#endif // MAINWINDOW_H
