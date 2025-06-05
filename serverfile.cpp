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
        // bikin socket TCP
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == INVALID_SOCKET) {
            std::cerr << "Gagal membuat socket." << std::endl;
            return false;
        }

        // set opsi biar socket bisa reuse alamat
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

        // ngatur info IP dan port server
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        // binding socket ke alamat dan port
        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "Gagal melakukan binding." << std::endl;
            return false;
        }

        // mulai dengerin koneksi masuk
        if (listen(server_socket, 5) == SOCKET_ERROR) {
            std::cerr << "Gagal mendengarkan koneksi." << std::endl;
            return false;
        }

        running = true;
        std::cout << "Server berhasil dijalankan di port " << port << "." << std::endl;
        return true;
    }

    void run() {
        while (running) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
            if (client_socket == INVALID_SOCKET) {
                if (running) {
                    std::cerr << "Gagal menerima koneksi dari klien." << std::endl;
                }
                continue;
            }

            // tiap client dijalanin di thread biar bisa handle banyak koneksi sekaligus
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

        std::cout << "Koneksi diterima dari " << client_ip << "." << std::endl;

        char buffer[1024];
        while (true) {
            // nerima data dari klien
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) {
                break; // klien disconnect atau error
            }

            buffer[bytes_received] = '\0';
            std::string data(buffer);

            // tampilkan data mentah
            std::cout << "Data diterima dari " << client_ip << ": " << data << std::endl;

            // kalau formatnya JSON, proses di sini
            processData(data);

            // kirim respon ke klien
            std::string response = "Data telah diterima.";
            send(client_socket, response.c_str(), response.length(), 0);
        }
    }

    void processData(const std::string& jsonData) {
        // debug print isi jsonnya biar keliatan
        std::cout << "Data mentah (JSON): " << jsonData << std::endl;

        // cari nilai sensor 'rainValue'
        size_t rain_val_pos = jsonData.find("\"rainValue\":");
        if (rain_val_pos != std::string::npos) {
            size_t start = jsonData.find(":", rain_val_pos) + 1;
            size_t end = jsonData.find("}", start);
            std::string rain_val_str = jsonData.substr(start, end - start);
            int rainValue = std::stoi(rain_val_str);

            std::cout << "Nilai sensor terdeteksi: " << rainValue;

            // tentuin status berdasarkan ambang
            if (rainValue >= 3000) {
                std::cout << " (Status: 0 - LED Merah)" << std::endl;
            } else if (rainValue > 1500) {
                std::cout << " (Status: 1 - LED Hijau)" << std::endl;
            } else {
                std::cout << " (Status: 2 - LED Merah dan Biru)" << std::endl;
            }
        }

        std::cout << "-----------------------------" << std::endl;
    }
};

int main() {
    TCPServer server(8080);

    if (!server.start()) {
        std::cerr << "Server tidak dapat dijalankan." << std::endl;
        return 1;
    }

    std::cout << "Tekan Enter untuk menghentikan server..." << std::endl;

    // jalanin server di thread baru biar bisa dengerin terus
    std::thread server_thread(&TCPServer::run, &server);

    std::cin.get();

    server.stop();
    server_thread.join();

    std::cout << "Server telah dimatikan." << std::endl;
    return 0;
}
