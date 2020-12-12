#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#include <io.h>
#include <Ws2tcpip.h> 
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
//#define _ADD_GROUP

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
	// Create a SOCKET 
	SOCKET senderSocket  =socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (senderSocket == INVALID_SOCKET) {
		wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// In fact, the following two options do not need to be set, and use UDPEchoClient to send multicast directly
#ifdef _ADD_GROUP 
	int bMLoop=1;// Allow this machine to receive
	if(SOCKET_ERROR==setsockopt(senderSocket,IPPROTO_IP,IP_MULTICAST_LOOP,(char*)&bMLoop,sizeof(bMLoop)))
	{
		wprintf(L"IP_MULTICAST_LOOP error! CODE is :%d\n",WSAGetLastError());
		closesocket(senderSocket);
		WSACleanup();
		return -1;
	}
	int dwRoute=12;
	if(SOCKET_ERROR==setsockopt(senderSocket,IPPROTO_IP,IP_MULTICAST_TTL,(char*)&dwRoute,sizeof(dwRoute)))
	{
		wprintf(L"IP_MULTICAST_TTL error! CODE is :%d\n",WSAGetLastError());
		closesocket(senderSocket);
		WSACleanup();
		return -1;
	}


	// Join a multicast group, you can send messages without joining
	// Used to receive multicast settings and add sockets to a multicast group
	ip_mreq mcast;// Set up multicast (multicast)
	memset(&mcast,0x00,sizeof(mcast));
	mcast.imr_multiaddr.S_un.S_addr=inet_addr( "233.25.10.72");
	mcast.imr_interface.S_un.S_addr=htonl(INADDR_ANY);
	if(SOCKET_ERROR==setsockopt(senderSocket,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char*)&mcast,sizeof(mcast))){
		wprintf(L"IP_ADD_MEMBERSHIPERROR!CODEIS:%d\n",WSAGetLastError());
		closesocket(senderSocket);
		WSACleanup();
		return 1;
	}
#endif
	char buf[2048+1];
    //----------------------
    sockaddr_in remote;
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = inet_addr( "233.25.10.72"); 
    remote.sin_port = htons(20131);
	// In an infinite loop, keep receiving input and sending it to the server
	while(gets_s(buf,2048)!=NULL)
	{
		int count = strlen(buf);// Read from standard input
		if(sendto(senderSocket, buf,count,0,(struct sockaddr *)&remote,sizeof(remote))<count)
			break;
	}
#ifdef _ADD_GROUP 
	// Leave a multicast group
	setsockopt(senderSocket,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char*)&mcast,sizeof(mcast)); 
#endif
	// End connection
	closesocket(senderSocket);
	WSACleanup();
	return 0;
}
