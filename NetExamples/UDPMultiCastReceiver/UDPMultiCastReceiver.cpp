#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#include <Ws2tcpip.h> 
#include <time.h>

// Need to link with Ws2_32.lib
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
    // Create a SOCKET for receiving
    // incoming connection requests.
    SOCKET updReceiverSocket;
    updReceiverSocket =socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (updReceiverSocket == INVALID_SOCKET) {
        wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port for the socket that is being bound.
    sockaddr_in addrLocal;
    addrLocal.sin_family = AF_INET;
    addrLocal.sin_addr.s_addr = htonl(INADDR_ANY); // Actually 0
    addrLocal.sin_port = htons(20131);


	// Bind the socket to an IP address and a port
    if (bind(updReceiverSocket,(SOCKADDR *) & addrLocal, sizeof (addrLocal)) == SOCKET_ERROR) {
        wprintf(L"bind failed with error: %ld\n", WSAGetLastError());
        closesocket(updReceiverSocket);
        WSACleanup();
        return 1;
    }


	// Bind the socket to an IP address and a port
	ip_mreq mcast;// Used to receive multicast settings and add sockets to a multicast group
	memset(&mcast,0x00,sizeof(mcast));

    /*int minN = 0;
    int maxN = 10;
	srand(time(NULL)); // Seed the time
	int finalNum = rand() % (maxN - minN + 1) + minN; // Generate the number, assign to variable.

    const char* addr = finalNum > 5 ? "233.25.10.72" : "233.25.10.71";*/

	mcast.imr_multiaddr.S_un.S_addr=inet_addr("233.25.10.72");
	mcast.imr_interface.S_un.S_addr=htonl(INADDR_ANY);
	if(SOCKET_ERROR==setsockopt(updReceiverSocket,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char*)&mcast,sizeof(mcast))){
		wprintf(L"IP_ADD_MEMBERSHIPERROR!CODEIS:%d\n",WSAGetLastError());
        closesocket(updReceiverSocket);
        WSACleanup();
        return 1;
	}


	// Receiving client socket connections in an endless loop
	while(1)
	{
		// The size of the receive buffer is 1024 characters
		char recvBuf[2048+1];
		     
        struct sockaddr_in cliAddr;          // Define IPV4 address variable
        int cliAddrLen = sizeof(cliAddr);
        int count = recvfrom(updReceiverSocket,recvBuf,2048,0,(struct sockaddr *)&cliAddr, &cliAddrLen);
		if(count==0)break;// Closed by the other party
		if(count==SOCKET_ERROR)break;// Error count<0
		recvBuf[count]='\0';
		printf("client IP = %s:%s\n",inet_ntoa(cliAddr.sin_addr),recvBuf);  // Display the IP address of sending data
		// For the udp package can only succeed once
        //if(sendto(SrvSocket, recvBuf,count,0,(struct sockaddr *)&cliAddr,cliAddrLen)<count)
		//	break;
	}
	/*If you leave the multicast, you can leave it alone and close it*/
	setsockopt(updReceiverSocket,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char*)&mcast,sizeof(mcast)); 
	closesocket(updReceiverSocket);
	WSACleanup();
	return 0;
}

