// CS 2690 Program 1 
// Simple Windows Sockets Echo Client (IPv6)
// Last update: 2/12/19
//
// Usage: WSEchoClientv6 <server IPv6 address> <server port> <"message to echo">
// Companion server is WSEchoServerv6
// Server usage: WSEchoServerv6 <server port>
//
// This program is coded in conventional C programming style, with the 
// exception of the C++ style comments.
//
// I declare that the following source code was written by me or provided
// by the instructor. I understand that copying source code from any other 
// source or posting solutions to programming assignments (code) on public
// Internet sites constitutes cheating, and that I will receive a zero 
// on this project if I violate this policy.
// ----------------------------------------------------------------------------

#pragma comment(lib, "ws2_32.lib")

// Minimum required header files for C Winsock program
#include <stdio.h>       // for print functions
#include <stdlib.h>      // for exit() 
#include <winsock2.h>	 // for Winsock2 functions
#include <Ws2ipdef.h>    // optional - needed for MS IP Helper
#include <conio.h>
#include <process.h>     // for _beginthreadex to make ProcessClient async
#include <signal.h>
#include <WS2tcpip.h>

// #define ALL required constants HERE, not inline 
// #define is a macro, don't terminate with ';'  For example...
#define RCVBUFSIZ 50

int shouldExit = 0;

// declare any functions located in other .c files here
void DisplayFatalErr(char* errMsg); // writes error message before abnormal termination

void main(int argc, char* argv[])   // argc is # of strings following command, argv[] is array of ptrs to the strings
{
	// Declare ALL variables and structures for main() HERE, NOT INLINE (including the following...)
	WSADATA wsaData;                // contains details about WinSock DLL implementation
	//struct sockaddr_in6 serverInfo;	// standard IPv6 structure that holds server socket info

	struct addrinfo hints, * addrInfoResult, * ptrAddrInfo;
	SOCKET clientSock = INVALID_SOCKET;

	// Verify correct number of command line arguments, else do the following:
	if (argc != 4)
	{
		fprintf(stderr, "Program requires three arguments at the command line.\nExpected: IP address of server, Listening port on server, Message to send to server.\n");
		exit(1);	  // ...and terminate with abnormal termination code (1)
	}

	// Retrieve the command line arguments. (Sanity checks not required, but port and IP addr will need
	// to be converted from char to int.  See slides 11-15 & 12-3 for details.)
	char* serverIPaddr = argv[1];
	char* serverPort = argv[2];
	char* message = argv[3];
	size_t messageLen = strlen(message);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	// Initialize Winsock 2.0 DLL. Returns 0 if ok. If this fails, fprint error message to stderr as above & exit(1).
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
	{
		DisplayFatalErr("WSAStartup failed");
	}

	//resolve what kind of IP address is being used
	if (getaddrinfo(serverIPaddr, serverPort, &hints, &addrInfoResult) != 0)
	{
		DisplayFatalErr("getaddrinfo failed. Could not resolve IP address");
	}

	//Use the first address returned by getaddrinfo
	ptrAddrInfo = addrInfoResult;

	clientSock = socket(ptrAddrInfo->ai_family, ptrAddrInfo->ai_socktype, ptrAddrInfo->ai_protocol);
	if (clientSock == INVALID_SOCKET)
	{
		DisplayFatalErr("Invalid socket returned when trying to create socket");
	}

	struct sockaddr_in6 clientAddress;
	int clientAddrLen = sizeof(clientAddress);

	memset(&clientAddress, 0, clientAddrLen);
	clientAddress.sin6_family = AF_INET6;
	clientAddress.sin6_addr = in6addr_any;
	clientAddress.sin6_port = 0;

	//bind socket
	if (bind(clientSock, (struct sockaddr*)&clientAddress, clientAddrLen) == SOCKET_ERROR)
	{
		DisplayFatalErr("Failed to bind client to ip and port to listen on");
	}

	int bytesSent = sendto(clientSock, message, (int)messageLen, 0, ptrAddrInfo->ai_addr, (int)ptrAddrInfo->ai_addrlen);
	if (bytesSent != messageLen)
	{
		if (bytesSent == -1)
		{
			DisplayFatalErr("Socket Error. Failed to transfer message");
		}
		else
		{
			DisplayFatalErr("Did not send all the bytes in the message");
		}
	}

 
 	// Retrieve the message returned by server.  Be sure you've read the whole thing (could be multiple segments). 
	// Manage receive buffer to prevent overflow with a big message.
 	// Call DisplayFatalErr() if this fails.  (Lots can go wrong here, see slides.)
	char rcvBuffer[RCVBUFSIZ];
	size_t bytesReceived = 0;
	size_t totalBytesReceived = 0;

	printf("Waiting for echo from server\n");

	struct sockaddr_storage senderAddr;
	int senderAddrLen = sizeof(senderAddr);
	char ipString[INET6_ADDRSTRLEN];
	unsigned short senderPort = 0;
	fd_set readfds;
	struct timeval timeout;
	int selectResult;

	//wait for and accept client connections
	while (!shouldExit)
	{
		//check for user input on ther server
		if (_kbhit())
		{
			char ch = _getch();
			if (ch == 4) // 4 corresponds to Ctrl + D
			{
				shouldExit = 1; //exit the loop and shut down the server
			}
			else
			{
				continue;
			}
		}

		//clear the socket set
		FD_ZERO(&readfds);

		//Add the server socket to the set
		FD_SET(clientSock, &readfds);

		//Set timeout interval
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000;

		selectResult = select(0, &readfds, NULL, NULL, &timeout);
		if (selectResult == SOCKET_ERROR)
		{
			DisplayFatalErr("Error when checking activity on server socket");
		}
		else if (selectResult > 0)
		{
			do
			{
				if (FD_ISSET(clientSock, &readfds))
				{
					bytesReceived = recvfrom(clientSock, rcvBuffer, RCVBUFSIZ - 1, 0, (struct sockaddr*)&senderAddr, &senderAddrLen);
					if (bytesReceived == SOCKET_ERROR)
					{
						int error = WSAGetLastError();
						if (error == WSAECONNRESET)
						{
							continue;
						}
						printf("Error when recvfrom called: %d\n", error);
						continue;
					}

					if (senderAddr.ss_family == AF_INET)
					{
						struct sockaddr_in* s = (struct sockaddr_in*)&senderAddr;
						inet_ntop(AF_INET, &s->sin_addr, ipString, INET6_ADDRSTRLEN);
						senderPort = ntohs(s->sin_port);
					}
					else if (senderAddr.ss_family == AF_INET6)
					{
						struct sockaddr_in6* s = (struct sockaddr_in6*)&senderAddr;
						inet_ntop(AF_INET6, &s->sin6_addr, ipString, INET6_ADDRSTRLEN);
						senderPort = ntohs(s->sin6_port);
					}
					else
					{
						printf("Received data from an unknown address family\n");
						continue;
					}

					if (bytesReceived > 0)
					{
						if (memcmp(&senderAddr, ptrAddrInfo->ai_addr, senderAddrLen) == 0 && senderPort == atoi(serverPort))
						{
							totalBytesReceived += bytesReceived;
							rcvBuffer[bytesReceived] = '\0';

							printf("%s", rcvBuffer);
						}
						else
						{
							printf("Received data from an unexpected address\n");
						}
					}
					else if (bytesReceived == -1)
					{
						break;
					}
					else
					{
						DisplayFatalErr("recv failed with error: ");
						break;
					}
				}
			} while (totalBytesReceived < messageLen);

			shouldExit = 1;
		}
	}
	printf("\n");
	printf("Message received from echo server\n%zu bytes transmitted. %zu bytes received\n", messageLen, totalBytesReceived);
    
 	// Close the socket
	if (closesocket(clientSock) == SOCKET_ERROR)
	{
		DisplayFatalErr("closesocket failed with error: ");
	}
	printf("Connection closed\n");

	// Release the Winsock DLL
	WSACleanup();
	
	exit(0);
}
