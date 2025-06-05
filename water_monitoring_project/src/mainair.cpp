#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <ctime>
#include <iomanip>
#include <string>
#include <thread>
#include <limits>
#include <conio.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
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
    #define Sleep(x) usleep((x)*1000)
#endif

struct DataEntry
{
    time_t t;
    int status;
};

std::string getCurrentTimestamp() {
    time_t now = time(0);
    tm* localTime = localtime(&now);

    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localTime);
    return std::string(buf);
}

void saveToBinary(time_t t, int status)
{
    std::ofstream binFile("data.bin", std::ios::binary | std::ios::app);
    if (binFile)
    {
        binFile.write(reinterpret_cast<char *>(&t), sizeof(t));
        binFile.write(reinterpret_cast<char *>(&status), sizeof(status));
        binFile.close();
    }
}

void exportToJson()
{
    std::ifstream binFile("data.bin", std::ios::binary);
    std::ofstream jsonFile("data.json");
    if (!binFile || !jsonFile) {
        std::cerr << "Gagal membuka file data.bin atau membuat data.json.\n";
        return;
    }
    
    jsonFile << "[\n";
    time_t t;
    int status;
    bool first = true;
    while (binFile.read(reinterpret_cast<char *>(&t), sizeof(t)))
    {
        binFile.read(reinterpret_cast<char *>(&status), sizeof(status));
        if (!first)
            jsonFile << ",\n";
        jsonFile << "  {\"timestamp\": " << t << ", \"status\": " << status << "}";
        first = false;
    }
    jsonFile << "\n]\n";
    binFile.close();
    jsonFile.close();
    std::cout << "Data berhasil diekspor ke data.json\n";
}

void searchByDate(int targetDay, int targetMonth, int targetYear)
{
    std::ifstream binFile("data.bin", std::ios::binary);
    if (!binFile) {
        std::cerr << "Gagal membuka data.bin.\n";
        return;
    }
    
    time_t t;
    int status;
    bool found = false;
    while (binFile.read(reinterpret_cast<char *>(&t), sizeof(t)))
    {
        binFile.read(reinterpret_cast<char *>(&status), sizeof(status));
        tm *ltm = localtime(&t);
        if (ltm->tm_mday == targetDay && ltm->tm_mon + 1 == targetMonth && ltm->tm_year + 1900 == targetYear)
        {
            std::cout << "Waktu: " << std::put_time(ltm, "%Y-%m-%d %H:%M:%S") << ", Status: " << status << "\n";
            found = true;
        }
    }
    if (!found)
        std::cout << "Tidak ada data pada tanggal tersebut.\n";
    binFile.close();
}

void sortDataByStatus()
{
    std::ifstream binFile("data.bin", std::ios::binary);
    if (!binFile) {
        std::cerr << "Gagal membuka data.bin.\n";
        return;
    }
    
    std::vector<DataEntry> entries;
    time_t t;
    int status;
    while (binFile.read(reinterpret_cast<char *>(&t), sizeof(t)))
    {
        binFile.read(reinterpret_cast<char *>(&status), sizeof(status));
        entries.push_back({t, status});
    }
    binFile.close();

    std::sort(entries.begin(), entries.end(), [](const DataEntry &a, const DataEntry &b)
              {
                  return a.status < b.status; // urutkan berdasarkan status
              });

    for (const auto &entry : entries)
    {
        std::cout << "Waktu: " << std::put_time(localtime(&entry.t), "%Y-%m-%d %H:%M:%S") << ", Status: " << entry.status << "\n";
    }
}

std::string getStatusMessage(int statusCode) {
    std::string timestamp = getCurrentTimestamp();
    
    if (statusCode == 2) {
        return timestamp + " - Critical! (Low Level)";
    }
    else if (statusCode == 1) {
        return timestamp + " - Stable";
    }
    else if (statusCode == 0) {
        return timestamp + " - Critical! (High Level)";
    }
    else {
        return timestamp + " - Status tidak dikenali: " + std::to_string(statusCode);
    }
}

int getStatusFromRainValue(int rainValue) {
    if (rainValue >= 3000) {
        return 0; // High Level (Red LED)
    }
    else if (rainValue < 3000 && rainValue > 1500) {
        return 1; // Stable (Green LED)
    }
    else if (rainValue <= 1500) {
        return 2; // Low Level (Red+Blue LED)
    }
    return -1; // Unknown
}

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
            std::cerr << "Gagal membuat socket.\n";
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
            std::cerr << "Bind gagal.\n";
            return false;
        }

        // Listen
        if (listen(server_socket, 5) == SOCKET_ERROR) {     
            std::cerr << "Gagal mendengarkan koneksi.\n";
            return false;
        }

        running = true;
        std::cout << "Server berjalan di port " << port << ".\n";
        return true;
    }

    void run() {
        while (running) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
            if (client_socket == INVALID_SOCKET) {
                if (running) {
                    std::cerr << "Gagal menerima koneksi dari klien.\n";
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
            server_socket = INVALID_SOCKET;
        }
    }

private:
    void handleClient(SOCKET client_socket, sockaddr_in client_addr) {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        std::cout << "Koneksi diterima dari " << client_ip << ".\n";

        char buffer[1024];
        while (running) {  
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytes_received <= 0) {
                break; // Client disconnected or error
            }

            buffer[bytes_received] = '\0';
            std::string data(buffer);
            
            // Process received data
            processData(data);
            
            // Send acknowledgment
            std::string response = "OK";
            send(client_socket, response.c_str(), response.length(), 0);
        }
        
        std::cout << "Klien " << client_ip << " terputus.\n";
        closesocket(client_socket);
    }

    void processData(const std::string& jsonData) {
        // Find rainValue in JSON
        size_t rain_val_pos = jsonData.find("\"rainValue\":");
        if (rain_val_pos != std::string::npos) {
            size_t start = jsonData.find(":", rain_val_pos) + 1;
            size_t end = jsonData.find_first_of(",}", start);
            
            if (end != std::string::npos) {
                std::string rain_val_str = jsonData.substr(start, end - start);
                
                // Remove any whitespace
                rain_val_str.erase(std::remove_if(rain_val_str.begin(), rain_val_str.end(), ::isspace), rain_val_str.end());
                
                try {
                    int rainValue = std::stoi(rain_val_str);
                    int statusCode = getStatusFromRainValue(rainValue);
                    
                    // Display status message with timestamp (same format as original)
                    std::cout << getStatusMessage(statusCode) << std::endl;
                    
                    // Save data to binary file (same as original)
                    if (statusCode != -1) {
                        saveToBinary(time(nullptr), statusCode);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing rainValue: " << e.what() << std::endl;
                }
            }
        } else {
            std::cout << "rainValue tidak ditemukan dalam data JSON.\n";
        }
    }
};

int main()
{
    TCPServer* server = nullptr;
    std::thread* serverThread = nullptr;
    bool serverRunning = false;

    while (true) {
        std::cout << "\n=== Aplikasi Server Sensor & Terminal UI ===\n";
        std::cout << "Pilih mode:\n"
                     "1. Monitoring (terima data lewat TCP dan simpan ke data.bin)\n"
                     "2. Ekspor ke JSON (data.bin -> data.json)\n"
                     "3. Cari data berdasarkan tanggal\n"
                     "4. Urutkan data berdasarkan status\n"
                     "5. Keluar dari program\n"
                     "Pilihan: ";

        int mode;
        std::cin >> mode;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (mode == 1) {
            // Start monitoring mode
            if (serverRunning) {
                std::cout << "Server sudah berjalan. Tekan 'q' untuk kembali ke menu.\n";
            } else {
                std::cout << "Mode Monitoring dipilih.\n";
                std::cout << "Memulai server TCP pada port 8080...\n";
                
                server = new TCPServer(8080);
                if (!server->start()) {
                    std::cerr << "Gagal memulai server.\n";
                    delete server;
                    server = nullptr;
                    continue;
                }
                
                // Jalankan server di thread terpisah
                serverThread = new std::thread(&TCPServer::run, server);
                serverRunning = true;
                std::cout << "Server berjalan. Tekan 'q' untuk kembali ke menu.\n";
            }

            // Monitor untuk input 'q'
            #ifdef _WIN32
                while (true) {
                    if (_kbhit()) {
                        char ch = _getch();
                        if (ch == 'q' || ch == 'Q') {
                            std::cout << "Kembali ke menu utama...\n";
                            break;
                        }
                    }
                    Sleep(100);
                }
            #else
                std::cout << "Tekan Enter untuk kembali ke menu.\n";
                std::cin.get();
            #endif
        }
        else if (mode == 2) {
            exportToJson();
        }
        else if (mode == 3) {
            int d, m, y;
            std::cout << "Masukkan tanggal (DD MM YYYY): ";
            std::cin >> d >> m >> y;
            searchByDate(d, m, y);
        }
        else if (mode == 4) {
            sortDataByStatus();
        }
        else if (mode == 5) {
            // Stop server and exit program
            if (serverRunning && server) {
                std::cout << "Menghentikan server...\n";
                server->stop();
                if (serverThread) {
                    serverThread->join();
                    delete serverThread;
                    serverThread = nullptr;
                }
                delete server;
                server = nullptr;
                serverRunning = false;
            }
            std::cout << "Keluar dari program. Selamat tinggal!\n";
            break; // Exit the while loop to end the program
        }
        else {
            std::cout << "Pilihan tidak valid. Silakan pilih 1-5.\n";
        }
    }

    return 0;
}