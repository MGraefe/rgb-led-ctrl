#ifndef HTTPPOSTSERVER_H__
#define HTTPPOSTSERVER_H__

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