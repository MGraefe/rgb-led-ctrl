// src/main.cpp
//----------------------------------
// RGB-LED Controller Source Code
// Copyright (c) 2015 Marius Graefe
// All rights reserved
// Contact: stuff@mgraefe.de
//----------------------------------

#include "stdafx.h"
#include "mainwindow.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QCoreApplication::setOrganizationName("Marius Graefe");
	QCoreApplication::setOrganizationDomain("mgraefe.de");
	QCoreApplication::setApplicationName("RGB-LED-Controller");

	bool startMinimized = false;
	for (int i = 1; i < argc; i++)
		if (strcmp(argv[i], "-minimized") == 0)
			startMinimized = true;

	MainWindow w;
	if (!startMinimized)
		w.show();

	return a.exec();
}
