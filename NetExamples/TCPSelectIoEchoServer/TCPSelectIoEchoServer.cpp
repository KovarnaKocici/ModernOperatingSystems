#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
// Need to link with Ws2_32.lib
/*http://blog.csdn.net/mlite/article/details/699340*/
#pragma comment (lib, "Ws2_32.lib")

int main(int argc, char* argv[])
{
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
    SOCKET ListenSocket;
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
    addrServer.sin_addr.s_addr = htonl(INADDR_ANY); //实际上是0
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

	// select model process
    // 1) Initialize a socket set fdSocket, and add the listener socket handle to this set
    fd_set fdSocket;        // Collection of all available sockets
    FD_ZERO(&fdSocket);  
    FD_SET(ListenSocket, &fdSocket); 

	//In an endless loop, keep checking whether there are any elements in the collection that have not been processed
	while(fdSocket.fd_count>0)
	{
		// 2) Pass a copy fdRead of the fdSocket collection to the select function,
        // When an event occurs, the select function removes the socket handles that have no pending I/O operations in the fdRead collection, and then returns.
		fd_set fdRead = fdSocket;  
        int nRet = ::select(0, &fdRead, NULL, NULL, NULL);  
        if(nRet > 0)  
        {  
            // 3) By comparing the original fdSocket collection with the fdRead collection processed by select,
            // Determine which sockets have pending I/O, and further process these I/Os.
            for(int i=0; i<(int)fdSocket.fd_count; i++)  
            {  
                if(FD_ISSET(fdSocket.fd_array[i], &fdRead))  
                {  
                    if(fdSocket.fd_array[i] == ListenSocket)     // (1) The listening socket receives a new connection
                    {  
                        if(fdSocket.fd_count < FD_SETSIZE)  
                        {  
                            sockaddr_in addrRemote;  
                            int nAddrLen = sizeof(addrRemote);  
                            SOCKET sNew = ::accept(ListenSocket, (SOCKADDR*)&addrRemote, &nAddrLen);  
							if(sNew  == INVALID_SOCKET)//error
							{
								FD_CLR(ListenSocket, &fdSocket);
							}
							else{
								FD_SET(sNew, &fdSocket);  
								printf("Connection received % s \n", ::inet_ntoa(addrRemote.sin_addr));  
							}
                        }  
                        else  
                        {  
                            printf(" Too much connections! \n");  
                            continue;  
                        }  
                    }  
                    else  
                    {  
                        char szText[256];  
                        int nRecv = ::recv(fdSocket.fd_array[i], szText, 255, 0);
						szText[nRecv] = '\0'; 
                        if(nRecv > 0)                        // (2) Readable
                        {
							int sendCount,currentPosition=0;
							//write can also be selected here to simplify processing
							while( nRecv>0 && (sendCount=send(fdSocket.fd_array[i] ,szText+currentPosition,nRecv,0))!=SOCKET_ERROR)
							{
								nRecv-=sendCount;
								currentPosition+=sendCount;
							}
							if(sendCount==SOCKET_ERROR){
								::closesocket(fdSocket.fd_array[i]);  
								FD_CLR(fdSocket.fd_array[i], &fdSocket);
							}								;
                            printf("Data received: % s \n", szText);  
                        }  
                        else                                // The connection is closed, restarted or interrupted =0 <0 SOCKET_ERROR
                        {
							printf("close a client\n"); 
                            ::closesocket(fdSocket.fd_array[i]);  
                            FD_CLR(fdSocket.fd_array[i], &fdSocket);  
                        }  
                    }  
                }  
            }  
        }  
        else  
        {  
            printf(" Failed select() \n");  
            break;  
        }  
	}

	closesocket(ListenSocket);
	WSACleanup();
	return 0;
}

