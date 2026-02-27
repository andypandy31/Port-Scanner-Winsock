// winsock2.h and ws2tcpip.h are the main headers for using Winsock functions in C++.
#include <winsock2.h>
#include <ws2tcpip.h>



// C++ standard library headers for threading, synchronization, and timing.
#include <thread>
#include <vector>
#include <atomic>

// C++ standard library headers for string manipulation and input/output.
#include <mutex>
#include <string>
#include <iostream>

// C++ standard library header for timing the scan duration.
#include <chrono>

// Link against the Winsock library.
#pragma comment(lib, "ws2_32.lib")

// Global variables for managing the scanning state and results.
std::atomic<int> current_port;
int end_port_global;
std::string target_ip;

// Atomic counters for tracking the number of open, closed, and filtered ports.
std::atomic<int> open_count(0);
std::atomic<int> closed_count(0);
std::atomic<int> filtered_count(0);

// Mutex for synchronizing output to the console.
std::mutex output_mutex;

// Rate limiting
int rate_limit = 0; // connections per second
std::atomic<int> connections_this_second(0);

void rateLimiter() // This thread will reset the connection count every second to enforce the rate limit.
{
    while (current_port <= end_port_global)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        connections_this_second = 0;
    }
}

void scanPort(int timeout_ms) // Each thread will execute this function, which will continuously fetch the next port to scan until we exceed the end port.
{
    while (true)
    {
		int port = current_port.fetch_add(1); // Atomically get the next port to scan.
        if (port > end_port_global)
            break;

		// Rate limiting, limit the number of connections per second if rate_limit is set.
        if (rate_limit > 0)
        {
            while (connections_this_second >= rate_limit)
				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Sleep briefly to avoid busy-waiting.

            connections_this_second++;
        }

		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create a TCP socket.
        if (sock == INVALID_SOCKET)
            continue;

		// Set up the sockaddr_in structure for the target IP and port.
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, target_ip.c_str(), &addr.sin_addr);

		// Set timeout, this is crucial to avoid hanging on filtered ports.
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout_ms, sizeof(timeout_ms)); // Set receive timeout, SO_RCVTIMOE is used for recv() timeout.
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout_ms, sizeof(timeout_ms)); // Set send timeout, SO_SNDTIMEO is used for send() timeout.

		// Attempt to connect to the target port.
        int result = connect(sock, (sockaddr*)&addr, sizeof(addr));

        if (result == 0)
        {
            open_count++;

            // Banner grabbing
            char buffer[1024] = { 0 };
			int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0); // recv() syntax is recv(int sockfd, void *buf, size_t len, int flags), this returns the number of bytes received, or -1 on error.

			std::lock_guard<std::mutex> lock(output_mutex); // Ensure thread-safe output.
			std::cout << "[OPEN] Port " << port;

			if (bytes > 0) // If we received data, print the banner.
            {
                buffer[bytes] = '\0';
                std::cout << " | Banner: " << buffer;
            }

            std::cout << std::endl; 
        }
        else
        {
			int err = WSAGetLastError(); // Get the specific error code.

			if (err == WSAECONNREFUSED) // Connection refused.
            {
                closed_count++;
            }
			else if (err == WSAETIMEDOUT) // Connection timed out.
            {
                filtered_count++;
            }
			else // Other errors (e.g., network issues) can be treated as filtered or ignored.
            {
                filtered_count++;
            }
        }

		closesocket(sock); // Clean up the socket.
    }
}

int main(int argc, char* argv[])
{
    if (argc < 6)
    {
        std::cout << "Usage:\n";
        std::cout << "scanner.exe <target_ip> <start_port> <end_port> <threads> <timeout_ms> [rate_limit]\n";
        return 1;
    }

    target_ip = argv[1];
    int start_port = std::stoi(argv[2]);
    end_port_global = std::stoi(argv[3]);
    int thread_count = std::stoi(argv[4]);
    int timeout_ms = std::stoi(argv[5]);

    if (argc >= 7)
        rate_limit = std::stoi(argv[6]);

    current_port = start_port;

	WSADATA wsa; // Initialize Winsock.
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) // WSAStartup() syntax is int WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData), this initializes the Winsock library and fills the WSADATA structure with details about the implementation.
    {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }
	
    // Start timing the scan.
    auto start_time = std::chrono::high_resolution_clock::now();

	// Create and launch threads for scanning ports.
    std::vector<std::thread> threads;

    // Start rate limiter thread if needed
    std::thread limiter;
    if (rate_limit > 0)
        limiter = std::thread(rateLimiter);

    for (int i = 0; i < thread_count; i++)
		threads.emplace_back(scanPort, timeout_ms); // Each thread will execute the scanPort function with the specified timeout.

	for (auto& t : threads) // Wait for all scanning threads to finish.
        t.join();

    if (rate_limit > 0)
        limiter.join();

	// End timing and calculate elapsed time.
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

	// Print the results of the scan.
    std::cout << "\n===== Scan Complete =====\n";
    std::cout << "Open: " << open_count << "\n";
    std::cout << "Closed: " << closed_count << "\n";
    std::cout << "Filtered: " << filtered_count << "\n";
    std::cout << "Time Elapsed: " << elapsed.count() << " seconds\n";

	WSACleanup(); // Clean up Winsock resources.
    return 0;
}