// Threaded-Port-Scanner-VS-Project.cpp

//The window APIs headers for socket programming and TCP/IP functions.
#include <winsock2.h> // Include the Windows Sockets API header for socket programming on Windows.
#include <ws2tcpip.h> // Include the Windows Sockets API header for TCP/IP specific functions.

//Include necessary headers for threading, vector management, and console output.
#include <thread> // Include the thread header for using std::thread to create and manage threads for concurrent port scanning.
#include <vector> // Include the vector header for using std::vector to manage threads.

//Terminal output and synchronization.
#include <iostream> // Include the iostream header for input/output operations, such as printing to the console.
#include <mutex> // Include the mutex header for thread synchronization when printing to the console.
#include <atomic> // Include the atomic header for thread-safe operations on shared variables.
#include <string> // allowing for termal arguments to be passed as strings to convert to integers for port numbers and other parameters.

#pragma comment(lib, "ws2_32.lib") // Link against the Winsock library to resolve socket functions.

// Global variables for managing the current port being scanned and synchronizing output.
std::atomic<int> current_port;
int end_port_global;
std::mutex output_mutex;



void static scanPort(const std::string& ipAddress, int timeout_ms) {
    while (true) {
        int port = current_port.fetch_add(1);

        if (port > end_port_global)
            break;

        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  //Creates a TCP socket also SOCK_STREAM can be swapwed with SOCK_DGRAM for UDP scanning.
        if (sock == INVALID_SOCKET) {
            //std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl; //Checks if the socket was created successfully, if not it prints an error message and returns.
            continue;
        }

        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
            (char*)&timeout_ms, sizeof(timeout_ms));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
            (char*)&timeout_ms, sizeof(timeout_ms));

        sockaddr_in target{};                                    //Structure to hold the server address information, including the IP address and port number.
        target.sin_family = AF_INET;                             //Sets the address family to AF_INET for IPv4.
        target.sin_port = htons(port);                           //Converts the port number from host byte order to network byte order using htons() and stores it in the serverAddr structure.

        if (inet_pton(AF_INET, ipAddress.c_str(), &target.sin_addr) != 1) {
            closesocket(sock);
            continue;
        }


        int result = connect(sock, (sockaddr*)&target, sizeof(target)); //Attempts to connect to the specified IP address and port. If the connection is successful, it indicates that the port is open.
        if (result == 0) {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "[OPEN] Port " << port << "\n";
        }
        else {
            int error_else = WSAGetLastError();
            std::cerr << "Port" << port << " Has not returned successeded - " << result << " The error code was : " << error_else << "\n";
        }
        closesocket(sock); //Closes the socket after the scan is complete to free up system resources.
    }
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cout << "Usage: name <ip> <start_port> <end_port> <timeout_ms> <max_threads>\n";
        std::cout << "       1st          2nd  3rd          4th        5th          6th\n";
        return 0;
    }

    std::string target_ip = argv[1];
    int start_port = std::stoi(argv[2]);
    int end_port = std::stoi(argv[3]);
    int timeout_ms = std::stoi(argv[4]);
    int max_threads = std::stoi(argv[5]);

    end_port_global = end_port;
    current_port = start_port;

    if (start_port < 1 || end_port > 65535 || start_port > end_port) {
        std::cerr << "Invalid port range\n";
        return 0;
    }

    if (max_threads < 1) {
        std::cerr << "Invalid number of threads\n";
        return 0;
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 0;
    }

    std::vector<std::thread> thread_worker; // Vector to hold the threads for concurrent port scanning.

    for (int i = 0; i < max_threads; i++) {
        thread_worker.emplace_back(scanPort, target_ip, timeout_ms);
    }

    for (auto& t : thread_worker) {
        t.join();
    }

    WSACleanup();
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
