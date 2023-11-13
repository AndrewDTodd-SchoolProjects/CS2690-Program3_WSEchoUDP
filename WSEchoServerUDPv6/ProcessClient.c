#include <stdio.h>
#include <winsock2.h>
#include <Ws2ipdef.h>

#define RCVBUFSIZ 1024

//void DisplayFatalErr(char* errMsg); // writes error message before abnormal termination

void ProcessClient(void* listenSocketPtr, const char* portNum)
{
	SOCKET serverSocket = *(SOCKET*)listenSocketPtr;

	char rcvBuffer[RCVBUFSIZ];
	int bytesReceived = 0;

	struct sockaddr_storage clientAddr;
	int addrLen = sizeof(clientAddr);
	char clientIP[INET6_ADDRSTRLEN];
	int clientPort;
	do
	{
		bytesReceived = recvfrom(serverSocket, rcvBuffer, RCVBUFSIZ, 0, (struct sockaddr*)&clientAddr, &addrLen);

		if (clientAddr.ss_family == AF_INET)
		{
			struct sockaddr_in* s = (struct sockaddr_in*)&clientAddr;
			inet_ntop(AF_INET, &s->sin_addr, clientIP, INET6_ADDRSTRLEN);
			clientPort = ntohs(s->sin_port);
		}
		else if (clientAddr.ss_family == AF_INET6)
		{
			struct sockaddr_in6* s = (struct sockaddr_in6*)&clientAddr;
			inet_ntop(AF_INET6, &s->sin6_addr, clientIP, INET6_ADDRSTRLEN);
			clientPort = ntohs(s->sin6_port);
		}
		else
		{
			printf("Received data from an unknown address family\n");
			closesocket(serverSocket);
			_endthreadex(0);
			return;
		}

		if (bytesReceived > 0)
		{
			int bytesSent = sendto(serverSocket, rcvBuffer, bytesReceived, 0, (struct sockaddr*)&clientAddr, addrLen);
			if (bytesSent == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				printf("sendto failed with error: %d\nClient address - \"%s\", client port: %d, server port: %s\n", error, clientIP, clientPort, portNum);
			}
		}
		else if (bytesReceived == 0)
		{
			printf("Message end...\n");
		}
		else
		{
			int error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS)
			{
				printf("recv failed with error: %d\n", error);
			}
			else
			{
				printf("Process client thread ending...\n");
			}
		}
	} while (bytesReceived > 0);

	printf("Processed the client at %s, from port: %d, on server port: %s", clientIP, clientPort, portNum);

	closesocket(serverSocket);
	_endthreadex(0);
}
