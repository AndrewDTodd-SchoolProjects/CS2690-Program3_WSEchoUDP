// Simple Windows Sockets Echo Client and Server
// Dave Heldenbrand 6/29/12
// 
// This is the error reporting function all programming assignments
// ----------------------------------------------------------------------------

// Minimum required header files for C Winsock program
#include <stdio.h>      // for print functions
#include <stdlib.h>     // for exit() 
#include <winsock2.h>    // for WSAGetLastError() 

void DisplayFatalErr(char *errMsg)
{
	fprintf(stderr, "%s: %d\n", errMsg, WSAGetLastError());  // Returns error code from current task or thread
	WSACleanup();
	exit(1);
}