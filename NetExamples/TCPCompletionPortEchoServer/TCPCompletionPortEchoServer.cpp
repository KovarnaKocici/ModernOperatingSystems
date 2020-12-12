#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define PORT     5150
#define MSGSIZE 1024
typedef enum
{
	RECV_POSTED
}OPERATION_TYPE;       // Enumeration, indicating status

typedef struct
{
	WSAOVERLAPPED   overlap;      
	WSABUF          Buffer;        
	char            szMessage[MSGSIZE];
	DWORD           NumberOfBytesRecvd;
	DWORD           Flags;
	OPERATION_TYPE OperationType;
}PER_IO_OPERATION_DATA, *LPPER_IO_OPERATION_DATA;    // Define a structure to save IO data

DWORD WINAPI ThreadProc(
	__in  LPVOID lpParameter
	)
{
	HANDLE                   CompletionPort=(HANDLE)lpParameter;
	DWORD                    dwBytesTransferred;
	SOCKET                   sClient=INVALID_SOCKET;
	LPPER_IO_OPERATION_DATA lpPerIOData = NULL;

	while (TRUE)
	{
		if(!GetQueuedCompletionStatus( // If you can receive data, return, otherwise wait
			CompletionPort,
			&dwBytesTransferred, // Number of words returned
			(PULONG_PTR) &sClient,           // Which client socket is the response?
			(LPOVERLAPPED *)&lpPerIOData, // Get the IO information saved by the socket
			INFINITE)){						// Wait indefinitely. The kind that does not time out.
				if(WSAGetLastError()==64){
					// Connection was closed by client
					// The following three sentences can be commented out, because you will go to the closed process below
					closesocket(sClient);
					HeapFree(GetProcessHeap(), 0, lpPerIOData);        // release the structure
					continue;
				}
				else
				{
				wprintf(L"GetQueuedCompletionStatus: %ld\n", WSAGetLastError());
				return -1;
				}
		}
		if (dwBytesTransferred == 0xFFFFFFFF)
		{
			return 0;
		}

		if (lpPerIOData->OperationType == RECV_POSTED) // If you receive data
		{
			if (dwBytesTransferred == 0)
			{
				// Connection was closed by client
				closesocket(sClient);
				HeapFree(GetProcessHeap(), 0, lpPerIOData);        // release the structure
			}
			else
			{
				lpPerIOData->szMessage[dwBytesTransferred] = '\0';
				
				// return the received message
				int sendCount,currentPosition=0,count=dwBytesTransferred;
				while( count>0 && (sendCount=send(sClient ,lpPerIOData->szMessage+currentPosition,count,0))!=SOCKET_ERROR)
				{
					count-=sendCount;
					currentPosition+=sendCount;
				}
				//------------------------------------------------------
				//if(sendCount==SOCKET_ERROR)break; there is error here


				// Launch another asynchronous operation for sClient
				memset(lpPerIOData, 0, sizeof(PER_IO_OPERATION_DATA));
				lpPerIOData->Buffer.len = MSGSIZE;
				lpPerIOData->Buffer.buf = lpPerIOData->szMessage;
				lpPerIOData->OperationType = RECV_POSTED;
				WSARecv(sClient,               // Loop receiving
					&lpPerIOData->Buffer,
					1,
					&lpPerIOData->NumberOfBytesRecvd,
					&lpPerIOData->Flags,
					&lpPerIOData->overlap,
					NULL);
			}
		}
	}
	return 0;
}

int main(int argc, char* argv[])
{
	// Initialization completion port
	HANDLE CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if(CompletionPort==NULL){
		wprintf(L"CreateIoCompletionPort failed with error: %ld\n", WSAGetLastError());
		return 1;
	}

	//----------------------
	// Initialize Winsock.
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error: %ld\n", iResult);
		return 1;
	}
	//----------------------
	// Create a SOCKET for listening for
	// incoming connection requests.
	SOCKET ListenSocket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound.
	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = htonl(INADDR_ANY); //Actually 0
	addrServer.sin_port = htons(20131);


	// Bind the socket to an IP address and a port
	if (bind(ListenSocket,(SOCKADDR *) & addrServer, sizeof (addrServer)) == SOCKET_ERROR) {
		wprintf(L"bind failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// Set the socket to listen mode and wait for connection requests
	//----------------------
	// Listen for incoming connection requests.
	// on the created socket
	if (listen(ListenSocket, 5) == SOCKET_ERROR) {
		wprintf(L"listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}


	// Create a few worker threads with a few CPUs
	SYSTEM_INFO              systeminfo;
	GetSystemInfo(&systeminfo);

	for (unsigned i = 0; i < systeminfo.dwNumberOfProcessors; i++)
	{
		DWORD dwThread;
		HANDLE hThread = CreateThread(NULL,0,ThreadProc,(LPVOID)CompletionPort,0,&dwThread);
		if(hThread==NULL)
		{
			wprintf(L"Thread Creat Failed!\n");
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		CloseHandle(hThread);
	}


	SOCKADDR_IN addrClient;
	int len=sizeof(SOCKADDR);
	// Receiving client socket connections in an endless loop
	while(1)
	{
		// After the request comes, accept the connection request and return a new socket corresponding to this connection
		SOCKET AcceptSocket=accept(ListenSocket,(SOCKADDR*)&addrClient,&len);
		if(AcceptSocket  == INVALID_SOCKET)break; //Error

		// Bind this newly arrived client socket to the completion port.
		CreateIoCompletionPort((HANDLE)AcceptSocket, CompletionPort, ( ULONG_PTR)AcceptSocket, 0);
		// Bind this newly arrived client socket to the completion port.

		// Initialize structure
		LPPER_IO_OPERATION_DATA lpPerIOData = (LPPER_IO_OPERATION_DATA)HeapAlloc(
			GetProcessHeap(),
			HEAP_ZERO_MEMORY,
			sizeof(PER_IO_OPERATION_DATA));
		lpPerIOData->Buffer.len = MSGSIZE; // len=1024
		lpPerIOData->Buffer.buf = lpPerIOData->szMessage;
		lpPerIOData->OperationType = RECV_POSTED; // Operation type
		WSARecv(AcceptSocket,         // Receive messages asynchronously and return immediately.
			&lpPerIOData->Buffer, // Get received data
			1,       // The number of WSABUF structures in the lpBuffers array.
			&lpPerIOData->NumberOfBytesRecvd, // The number of bytes received, if error returns 0
			&lpPerIOData->Flags,     
			&lpPerIOData->overlap,     
			NULL);
	}

	//posts an I/O completion packet to an I/O completion port.
	PostQueuedCompletionStatus(CompletionPort, 0xFFFFFFFF, 0, NULL);
	CloseHandle(CompletionPort);
	closesocket(ListenSocket);
	WSACleanup();
	return 0;
}

