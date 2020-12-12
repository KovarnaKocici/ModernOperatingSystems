#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#include <io.h>
#include <chrono>
#include <iostream>
#include <fstream>
#include <random>
#include <iosfwd>
#include <minwindef.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

//#define BUF_SIZE 1024*1024*2 // 2MB
#define BUF_SIZE 1024
size_t constexpr microseconds_in_a_second = 1000 * 1000;

#define RUN_TEST 0

#if RUN_TEST
size_t const  rounds = 100;
#undef max
#undef min

const std::string kTestFileName = "test.bin";

inline bool FileExists(const std::string& name) {
	std::ifstream f(name.c_str());
	return f.good();
}

void GenerateData(const int& kBytes) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(std::numeric_limits<uint8_t>::min(), std::numeric_limits<uint8_t>::max());

	std::cout << "Starting data generation" << std::endl;

	if (!FileExists(kTestFileName)) {
		std::ofstream test_file;
		test_file.open(kTestFileName, std::ios::out | std::ios::binary);

		if (test_file.is_open()) {
			for (int i = 0; i < kBytes; i++) {
				test_file << (unsigned char)distrib(gen);
			}
			test_file.close();
		}
	}

	std::cout << "Data generation finished" << std::endl;
}

void Measurement(SOCKET& ConnectSocket, const int& kBytes = 1'000'000) {

    char* input_data = new char[kBytes];
    if (FileExists(kTestFileName)) {
        std::ifstream input(kTestFileName.c_str(), std::ios::in | std::ios::binary);
        if (input.is_open()) {
            for (int i = 0; i < kBytes; i++) {
                input >> input_data[i];
            }
        }
    }
    else {
        std::cout << "Couldn't find testing file" << std::endl;
        exit(1);
    }

	int  round = 0;

	printf("Test mode is active.\n");
	int count = BUF_SIZE;// Read from standard input

	auto const& before_send = std::chrono::high_resolution_clock::now();
	while (round < rounds)
	{
		if (count <= 0)break;
		int sendCount, currentPosition = 0;

		while (count > 0 && (sendCount = send(ConnectSocket, input_data + currentPosition, count, 0)) != SOCKET_ERROR)
		{
			count -= sendCount;
			currentPosition += sendCount;
		}
		if (sendCount == SOCKET_ERROR)break;

		count = recv(ConnectSocket, input_data, BUF_SIZE, 0);

		round++;
	};
	auto const& after_receive = std::chrono::high_resolution_clock::now();

	printf(
		"Send and receive on %u bytes took %.6lfs in average\n",
		BUF_SIZE,
		static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(after_receive - before_send).count())
		/ static_cast<double>(rounds * microseconds_in_a_second));

	//Wait for input
	char input;
	std::cin >> input;

}
#endif //RUN_TEST

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
    // Create a SOCKET for connecting to server
    SOCKET ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError() );
        WSACleanup();
        return 1;
    }
    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port for the socket that is being bound.
    sockaddr_in addrServer;
    addrServer.sin_family = AF_INET;
    addrServer.sin_addr.s_addr = inet_addr( "127.0.0.1" );
    addrServer.sin_port = htons(20131);

	//----------------------
    // Connect to server.
    iResult = connect( ConnectSocket, (SOCKADDR*) &addrServer, sizeof(addrServer) );
    if ( iResult == SOCKET_ERROR) {
        closesocket (ConnectSocket);
        printf("Unable to connect to server: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

#if RUN_TEST
	GenerateData(BUF_SIZE);
    Measurement(ConnectSocket, BUF_SIZE);

#else
	// In an infinite loop, continuously receive input and send it to the server
	char* buf = new char[BUF_SIZE + 1];

	printf("Default mode is active. Please, make input\n");
	while(1)
	{
		int count = _read (0, buf, BUF_SIZE);// Read from standard input
		int init_count = count;
		if(count<=0)break;
		int sendCount,currentPosition=0;

        auto const& before_send = std::chrono::high_resolution_clock::now();

		while( count>0 && (sendCount=send(ConnectSocket ,buf+currentPosition,count,0))!=SOCKET_ERROR)
		{
			count-=sendCount;
			currentPosition+=sendCount;
		}
		if(sendCount==SOCKET_ERROR)break;
		
        count = recv(ConnectSocket, buf, BUF_SIZE, 0);

        auto const& after_receive = std::chrono::high_resolution_clock::now();

		printf(
			"Send and receive on %u bytes took %.6lfs\n",
			init_count,
			static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(after_receive - before_send).count())
			/ static_cast<double>(microseconds_in_a_second));

		if(count==0)break;// Closed by the other party
		if(count==SOCKET_ERROR)break;// Error count<0
		buf[count]='\0';
		printf("%s",buf);
	}
#endif //RUN_TEST

	// End connection
	closesocket(ConnectSocket);
	WSACleanup();
	return 0;
}

