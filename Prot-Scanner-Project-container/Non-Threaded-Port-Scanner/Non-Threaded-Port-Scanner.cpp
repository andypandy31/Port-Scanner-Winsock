#include <winsock2.h> // Include the Windows Sockets API header for socket programming on Windows.
#include <ws2tcpip.h>
#include <iostream> 
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib") // Link against the Winsock library to resolve socket functions.

void static scanPort(const std::string& ipAddress, int port) {  //Function to scan a single port , takes the IP address and port number as parameters.
    std::cout << "ScanPort started\n";
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);  //Creates a TCP socket also SOCK_STREAM can be swapwed with SOCK_DGRAM for UDP scanning.
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl; //Checks if the socket was created successfully, if not it prints an error message and returns.
        return;
    }

    sockaddr_in serverAddr; //Structure to hold the server address information, including the IP address and port number.
    serverAddr.sin_family = AF_INET; //Sets the address family to AF_INET for IPv4.
    inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr); //Converts the IP address from string format to binary format and stores it in the serverAddr structure.
    serverAddr.sin_port = htons(port); //Converts the port number from host byte order to network byte order using htons() and stores it in the serverAddr structure.

    int result = connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)); //Attempts to connect to the specified IP address and port. If the connection is successful, it indicates that the port is open.
    if (result == 0) {
        std::cout << "Port " << port << " is open." << std::endl; //Prints a message indicating that the port is open.
    }
    else if (result == -1) {
        int error = WSAGetLastError();
        std::cout << "Port " << port << " has retured : " << result << std::endl; //Prints a message indicating that the port is open.
        std::cout << "the error code was - :" << error << std::endl;
    }
    else {
        int error_else = WSAGetLastError();
        std::cout << "Port " << port << " Has not returned success = 1 or failure = -1 and has somehow resulted in else catch" << result << std::endl;
        std::cout << "the error code was - :" << error_else << std::endl;
    }
    closesocket(sock); //Closes the socket after the scan is complete to free up system resources.
}

int main() {
    // Initialize Winsock (REQUIRED on Windows)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    std::cout << "test main started\n";
    scanPort("127.0.0.1", 80);  // Hardcoded test

    WSACleanup();  // Cleanup Winsock
    return 0;
}