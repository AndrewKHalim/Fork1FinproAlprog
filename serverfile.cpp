#include <iostream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

class TCPServer {
private:
    SOCKET server_socket;
    int port;
    bool running;

public:
    TCPServer(int p) : port(p), running(false) {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }

    ~TCPServer() {
        stop();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    bool start() {
        // Create socket
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }

        // Set socket options
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

        // Bind socket
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed" << std::endl;
            return false;
        }

        // Listen
        if (listen(server_socket, 5) == SOCKET_ERROR) {     
            std::cerr << "Listen failed" << std::endl;
            return false;
        }

        running = true;
        std::cout << "Server listening on port " << port << std::endl;
        return true;
    }

    void run() {
        while (running) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
            if (client_socket == INVALID_SOCKET) {
                if (running) {
                    std::cerr << "Accept failed" << std::endl;
                }
                continue;
            }

            // Handle client in separate thread
            std::thread client_thread(&TCPServer::handleClient, this, client_socket, client_addr);
            client_thread.detach();
        }
    }

    void stop() {
        running = false;
        if (server_socket != INVALID_SOCKET) {
            closesocket(server_socket);
        }
    }

private:
    void handleClient(SOCKET client_socket, sockaddr_in client_addr) {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        std::cout << "Client connected from " << client_ip << std::endl;

        char buffer[1024];
        while (true) {  
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytes_received <= 0) {
                break; // Client disconnected or error
            }

            buffer[bytes_received] = '\0';
            std::string data(buffer);
            
            // Process received data
            std::cout << "Received from " << client_ip << ": " << data << std::endl;
            
            // Parse JSON data here if needed
            processData(data);
            
            // Send acknowledgment
            std::string response = "OK";
            send(client_socket, response.c_str(), response.length(), 0);
        }
    }

    void processData(const std::string& jsonData) {
        // Parse sensor data
        std::cout << "Raw JSON: " << jsonData << std::endl;
        
        // Find rainValue
        size_t rain_val_pos = jsonData.find("\"rainValue\":");
        if (rain_val_pos != std::string::npos) {
            size_t start = jsonData.find(":", rain_val_pos) + 1;
            size_t end = jsonData.find("}", start);
            
            std::string rain_val_str = jsonData.substr(start, end - start);
            int rainValue = std::stoi(rain_val_str);
            
            std::cout << "Sensor Value: " << rainValue;
            
            // Show status based on your original thresholds
            if(rainValue >= 3000) {
                std::cout << " (Status: 0 - Red LED)";
            }
            else if(rainValue < 3000 && rainValue > 1500) {
                std::cout << " (Status: 1 - Green LED)";
            }
            else if(rainValue <= 1500) {
                std::cout << " (Status: 2 - Red+Blue LED)";
            }
            std::cout << std::endl;
        }
        
        std::cout << "---" << std::endl;
    }
};

int main() {
    TCPServer server(8080);
    
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    std::cout << "Press Enter to stop server..." << std::endl;
    
    // Run server in separate thread
    std::thread server_thread(&TCPServer::run, &server);
    
    // Wait for user input to stop
    std::cin.get();
    
    server.stop();
    server_thread.join();
    
    std::cout << "Server stopped" << std::endl;
    return 0;
}