#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <conio.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

void pause();

int _cdecl main() {

	#define DEFAULT_PORT "27015"
	#define DEFAULT_BUFLEN 512

	//Create a SOCKET object
	SOCKET ListenSocket = INVALID_SOCKET;

	//WSAData structure contains info about the Windows Sockets Implementation
	WSADATA wsadata;

	//WSAStartup function is called to initiate the use of WS2_32.dll
	//The MAKEWORD paramater of WSAStartup makes a request for version 2.2 of WinSock on the system and sets the passed verion of Winsock support that the calller can use.
	int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (wsaResult != 0) {
		cout << "WSAStartup Failed: " << wsaResult << "\n";
		pause();
		return 1;
	}

	/*
	The getaddrinfo function is used to determine the values in the sockaddr structure:
		- AF_INET is used to specify the IPv4 address family.
		- SOCK_STREAM is used to specify a stream socket.
		- IPPROTO_TCP is used to specify the TCP protocol.
		- AI_PASSIVE flag indicates the caller intends to use the returned socket address structure in a call to the bind function. 
		  When the AI_PASSIVE flag is set and nodename parameter to the getaddrinfo function is a NULL pointer, the IP address 
		  portion of the socket address structure is set to INADDR_ANY for IPv4 addresses or IN6ADDR_ANY_INIT for IPv6 addresses.
		- 27015 is the port number associated with the server that the client will connect to.

	The addrinfo structure is used by the getaddrinfo function.
	In this example, a TCP stream socket for IPv4 was requested with an address family of IPv4, a socket type
	of SOCK_STREAM and a protocol of IPPROTO_TCP. So an IPv4 address is requested for the ListenSocket.
	If the server application wants to listen on IPv6, then the address family needs to be
	set to AF_INET6 in the hints parameter. If a server wants to listen on both IPv6 and IPv4, 
	two listen sockets must be created, one for IPv6 and one for IPv4. These two sockets must 
	be handled separately by the application.
	Windows Vista and later offer the ability to create a single IPv6 socket that is put in 
	dual stack mode to listen on both IPv6 and IPv4.
	*/
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	/*
	Call the get adderinfo function requesting the server port and using NULL as the address.
	Resolve the local address and port to be used by the server.
	For this server application, use the first IP address returned by the call to getaddrinfo that matched the
	address family, socket type, and protocol specified in the hints parameter.
	*/
	int lookupresult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (lookupresult != 0) {
		cout << "Getaddrinfo Failed: " << lookupresult << "\n";
		WSACleanup();
		pause();
		return 1;
	}

	//Create a SOCKET for server to listen for client connections
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "Socket Error: " << WSAGetLastError() << "\n";
		freeaddrinfo(result);
		WSACleanup();
		pause();
		return 1;
	}


	// Set the exclusive address option to avoid malicious application binding to the same port
	int optval = 1;
	int setResult = setsockopt(ListenSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&optval, sizeof(optval));
	if (setResult == SOCKET_ERROR) {
		cout << "setsockopt for SO_EXCLUSIVEADDRUSE failed with error: " << WSAGetLastError() << "\n";
	}

	/*
	For the server to accept client connections, it must be bound to a network address within the system.
	The sockaddr structure holds info regarding the addres family, IP address and port number.
	Calling the bind function passes the created socket and sockaddr structure returned from the getaddrinfo function as parameters.
	*/
	int bindResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (bindResult == SOCKET_ERROR) {
		cout << "Bind Error: " << WSAGetLastError() << "\n";
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		pause();
		return 1;
	}

	//Once the bind function is called, the address information returned by the getaddrinfo function is no longer needed.
	//The freeaddrinfo function is called to free allocated memory.
	freeaddrinfo(result);

	/*
	After the socket is bound to an IP address and port on the system, the server must then listen for incoming connection requests.
	Call the listen function, passing the parameters the created socket and a value for backlog , maximum length of the queue of
	pending connections to accept. In this example, the backlog parameter was set to SOMAXCONN. This value is a special constant that
	instructs the Winsock provider for this socket to allow a maximuim reasonable number of pending connections in the queue.
	*/
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		cout << "Listen Error: " << WSAGetLastError() << "\n";
		closesocket(ListenSocket);
		WSACleanup();
		pause();
		return 1;
	}

	
	//Once the socket is listening for a connection, the program must handle connection requests on that socket.
	//For accepting connections from client create a temporary socket object.
	SOCKET ClientRecvSocket;
	ClientRecvSocket = INVALID_SOCKET;

	/*
	Normally, a server application would be designed to listen for connections from multiple clients.
	High performance servers would use multiple threads to handle multiple client connections.
	There are several different techniques for using Winsock to listen for multiple client connections.
	Using the listen function runs a continuous loop that checks for incoming connections requests.
	If the connection request occurs, the application calls the accept, AcceptEx, WSAAccept function and passes
	the work to another thread to handle the request. Several programming techniques are also possible.
	This example does not use multiple threads and listens for and accepts only a single connection.
	*/
	cout << "Started Listening on port: " << DEFAULT_PORT << "\n\n";
	ClientRecvSocket = accept(ListenSocket, NULL, NULL);
	if (ClientRecvSocket == INVALID_SOCKET) {
		cout << "Accept Failed: " << WSAGetLastError() << "\n";
		closesocket(ListenSocket);
		WSACleanup();
		pause();
		return 1;
	}

	/*
	When the client connection has been accepted, a server application would normally pass the accepted 
	client socket (the ClientSocket variable in the above sample code) to a worker thread or an I/O completion 
	port and continue accepting additional connections. In this basic example, the server continues to the next step.
	There are a number of other programming techniques that can be used to listen for and accept multiple connections. 
	These include using the select or WSAPoll functions. Examples of these can be found in the Advanced Winsock Examples.
	
	
	The send and recv functions both return an integer value of the number of bytes sent or received, respectively, or an error. 
	Each function also takes the same parameters: the active socket, a char buffer, the number of bytes to send or receive, and any flags to use.
	Receiving and sending data on a socket:
	*/
	char recvbuf[DEFAULT_BUFLEN] = "";
	int recvResult, sendResult;
	int recvbuflen = DEFAULT_BUFLEN;

	//Receive until the peer shuts down the connection


	do {
		recvResult = recv(ClientRecvSocket, recvbuf, recvbuflen, 0);
		if (recvResult > 0) {
			cout << "Bytes Received: " << recvResult << "\n";

			//Echo and print the buffer back to the sender
			sendResult = send(ClientRecvSocket, recvbuf, recvResult, 0);
			if (sendResult == SOCKET_ERROR) {
				cout << "Send Failed: " << WSAGetLastError() << "\n";
				closesocket(ListenSocket);
				WSACleanup();
				pause();
				return 1;
			}
			cout << "Bytes sent: " << sendResult << "\n";
			cout << "Received Buffer: " << recvbuf << " \n";
		}
		else if (recvResult == 0)
			cout << "Connection Closed." << "\n";
		else {
			cout << "Receive Failed: " << WSAGetLastError() << "\n";
			closesocket(ClientRecvSocket);
			WSACleanup();
			pause();
			return 1;
		}
	} while (recvResult > 0);

	/*
	Disconnecting the Server and shutting down the socket
	When the server is done sending data, the shutdown function can be called specifying SD_SEND to shutdown the sending side of the socket.
	This allows the client to release some of the resources for this socket. 
	The server can still receive data on the socket.
	*/

	//Shutdown the send half of the socket 
	int shutdownResult = shutdown(ClientRecvSocket, SD_SEND);
	if (shutdownResult == SOCKET_ERROR) {
		cout << "Shutdown Failed: " << WSAGetLastError() << "\n";
		closesocket(ClientRecvSocket);
		WSACleanup();
		pause();
		return 1;
	}

	//When the client is done receiving data, the closesocket function is called to close the socket.
	//When the application is done using the Windows Socket DLL, the WSACleanup function is callled to release resources.
	closesocket(ClientRecvSocket);
	WSACleanup();

	pause();
	return 0;
}

void pause() {
	char a;
	cout << string(2, '\n') << "Press Any Key to Continue... ";
	a = _getch();
}