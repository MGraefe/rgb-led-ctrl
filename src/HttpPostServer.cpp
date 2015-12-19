
#include "stdafx.h"
#include "HttpPostServer.h"
#include <iostream>
#include <sstream>

#ifdef WIN32

Win32SocketInitializer::Win32SocketInitializer()
{
	WSADATA data = {0};
	int result = WSAStartup(MAKEWORD(2, 2), &data);
	if (result == 0)
		printf("WSAStartup was successfull");
	else
		printf("WSAStartup failed\n");
}

Win32SocketInitializer g_socketInitializerInstance;

#endif // WIN32


#define STATUS_OK "HTTP/1.1 200 OK\r\n" \
	"Content-Type: text/html\r\n" \
	"Content-Length: 0\r\n" \
	"Connection: keep-alive\r\n" \
	"\r\n"


HttpPostServer::HttpPostServer(std::function<void(const char*, size_t)> dataEndPoint, 
	int port, bool loopbackOnly) :
	m_dataEndPoint(dataEndPoint),
	m_port(port),
	m_loopbackOnly(loopbackOnly),
	m_listenSocket(INVALID_SOCKET),
	m_clientSocket(INVALID_SOCKET),
	m_stop(false)
{

}


void toLower(std::string &str)
{
	for (size_t i = 0; i < str.size(); i++)
		if (str[i] >= 'A' && str[i] <= 'Z')
			str[i] += ('a' - 'A');
}


bool HttpPostServer::findContentSize(const std::string &recvBuffer,
	size_t &headerSize,  size_t &contentSize)
{
	if (recvBuffer.length() > 2048)
		return false;
	size_t pos = recvBuffer.find("\r\n\r\n");
	if (pos == std::string::npos)
		return true;
	headerSize = pos + 4;
	if (recvBuffer.compare(0, 4, "POST", 4) != 0)
		return false;
	size_t lineEnd = 0, lineBegin = 0;
	while ((lineEnd = recvBuffer.find("\r\n", lineBegin)) != std::string::npos)
	{
		std::string line(recvBuffer.begin() + lineBegin, recvBuffer.begin() + lineEnd);
		toLower(line);
		if (line.compare(0, 15, "content-length:", 15) == 0)
		{
			contentSize = strtol(line.substr(15).c_str(), NULL, 10);
			break;
		}
		lineBegin = lineEnd + 2;
	}
	return contentSize != 0;
}


bool HttpPostServer::sendResult(SOCKET clientSocket, const char *result)
{
	int bytesTotal = strlen(result);
	int bytesSend = 0;
	while (bytesSend < bytesTotal)
	{
		//fd_set fd_send;
		//FD_ZERO(&fd_send);
		//FD_SET(clientSocket, &fd_send);
		//struct timeval timeout = { 1, 0 };
		//int fss = select(clientSocket + 1, NULL, &fd_send, NULL, &timeout);
		//if (fss <= 0)
		//	return false;
		int bytes = send(clientSocket, result, bytesTotal - bytesSend, NULL);
		if (bytes <= 0)
			return false;
		bytesSend += bytes;
	}
	return true;
}



bool HttpPostServer::examinePackets(std::string &recvBuffer, size_t &headerSize, size_t &contentSize)
{
	while (true)
	{
		if (headerSize == 0 || contentSize == 0) // Find Header size
			if (!findContentSize(recvBuffer, headerSize, contentSize))
				return false; // error
		if (headerSize == 0 || contentSize == 0) 
			return true; // still not (yet) known
		size_t totalSize = headerSize + contentSize;
		if (recvBuffer.size() < totalSize)
			return true; // content not yet complete
		m_dataEndPoint(recvBuffer.c_str() + headerSize, contentSize);
		recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + totalSize);
		headerSize = contentSize = 0;
		if (!sendResult(m_clientSocket, STATUS_OK))
			return false; // error
	}
}


void HttpPostServer::run()
{
	m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_listenSocket == INVALID_SOCKET)
	{
		printf("Error opening socket");
		return;
	}

	sockaddr_in sockAddrIn;
	memset(&sockAddrIn, 0, sizeof(sockAddrIn));
	sockAddrIn.sin_port = htons(m_port);
	sockAddrIn.sin_family = AF_INET;
	sockAddrIn.sin_addr.s_addr = htonl(m_loopbackOnly ? INADDR_LOOPBACK : INADDR_ANY);
	if (bind(m_listenSocket, (sockaddr*)&sockAddrIn, sizeof(sockAddrIn)) == SOCKET_ERROR)
	{
		printf("Error binding server to port %i on IPv4, already bound?\n", m_port);
		closesocket(m_listenSocket);
		m_listenSocket = INVALID_SOCKET;
		return;
	}

	if (listen(m_listenSocket, 5) != 0)
	{
		printf("Call to listen() failed\n");
		closesocket(m_listenSocket);
		m_listenSocket = INVALID_SOCKET;
		return;
	}

	while (!m_stop)
	{
		fd_set fs_read;
		FD_ZERO(&fs_read);
		FD_SET(m_listenSocket, &fs_read);

		timeval timeout = { 1, 0 };
		int fss = select(m_listenSocket + 1, &fs_read, NULL, NULL, &timeout);
		if (fss <= 0)
			continue;

		sockaddr *addr = NULL;
		socklen_t addrsize = 0;
		addr = (sockaddr*)malloc(sizeof(sockaddr_in));
		memset(addr, 0, sizeof(sockaddr_in));
		addrsize = sizeof(sockaddr_in);

		qDebug() << "Listening for cons";
		m_clientSocket = accept(m_listenSocket, addr, &addrsize);
		free(addr);
		if (m_clientSocket == INVALID_SOCKET)
		{
			printf("Error accepting client...\n");
			continue;
		}
		qDebug() << "New client...";
		
		std::string recvBuffer;
		char buf[2048];
		size_t contentSize = 0;
		size_t headerSize = 0;
		bool errorOccurred = false;
		while (!m_stop)
		{
			fd_set fs_recv;
			FD_ZERO(&fs_recv);
			FD_SET(m_clientSocket, &fs_recv);
			struct timeval timeout = { 0, 100000 };
			int fss = select(m_clientSocket + 1, &fs_recv, NULL, NULL, &timeout);
			if (fss <= 0)
				continue;
			int bytes = recv(m_clientSocket, buf, 2048, 0);
			if (bytes <= 0)
				break;
			recvBuffer.insert(recvBuffer.end(), buf, buf + bytes);
			if (!examinePackets(recvBuffer, headerSize, contentSize))
				break;
		}

		qDebug() << "Disconnected client...";

		closesocket(m_clientSocket);
		m_clientSocket = INVALID_SOCKET;
	}
}


void HttpPostServer::stop()
{
	m_stop = true;
	this->wait();
}
