// src/HttpPostServer.h
//----------------------------------
// RGB-LED Controller Source Code
// Copyright (c) 2015 Marius Graefe
// All rights reserved
// Contact: stuff@mgraefe.de
//----------------------------------

#pragma once
#ifndef rgbledsrc__HttpPostServer_H__
#define rgbledsrc__HttpPostServer_H__

#include <QThread>
#include <functional>
#include <vector>

#ifdef WIN32
class Win32SocketInitializer
{
public:
	Win32SocketInitializer();
};
#endif

class HttpPostServer : public QThread
{
public:
	HttpPostServer(std::function<void(const char*, size_t)> dataEndPoint,
		int port, bool loopbackOnly);
	void run() override;
	void stop();

private:
	bool examinePackets(std::string &recvBuffer, size_t &headerSize, size_t &contentSize);
	bool sendResult(SOCKET clientSocket, const char *result);
	bool findContentSize(const std::string &recvBuffer, size_t &headerSize, size_t &contentSize);
	std::function<void(const char*, size_t)> m_dataEndPoint;
	int m_port;
	bool m_loopbackOnly;
	SOCKET m_listenSocket;
	SOCKET m_clientSocket;
	volatile bool m_stop;
};

#endif