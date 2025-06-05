#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <iomanip>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <conio.h>
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

// Struktur untuk menyimpan tiap entri data (waktu + status)
struct DataEntry {
    time_t t;
    int status;
};

// Fungsi mendapatkan timestamp saat ini dalam format "YYYY-MM-DD HH:MM:SS"
std::string getCurrentTimestamp() {
    time_t now = time(nullptr);
    tm* localTime = localtime(&now);

    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localTime);
    return std::string(buf);
}

// Simpan entri (waktu, status) ke file binary "data.bin"
void saveToBinary(time_t t, int status) {
    std::ofstream binFile("data.bin", std::ios::binary | std::ios::app);
    if (!binFile) return;
    binFile.write(reinterpret_cast<char*>(&t), sizeof(t));
    binFile.write(reinterpret_cast<char*>(&status), sizeof(status));
    binFile.close();
}

// Ekspor isi "data.bin" ke "data.json"
void exportToJson() {
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

    while (binFile.read(reinterpret_cast<char*>(&t), sizeof(t))) {
        binFile.read(reinterpret_cast<char*>(&status), sizeof(status));

        // Ubah time_t ke tm* dan format string "YYYY-MM-DD HH:MM:SS"
        tm* ltm = localtime(&t);
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ltm);

        if (!first) jsonFile << ",\n";
        jsonFile << "  {"
                 << "\"timestamp\": " << t << ", "
                 << "\"datetime\": \"" << buf << "\", "
                 << "\"status\": " << status
                 << "}";
        first = false;
    }

    jsonFile << "\n]\n";
    binFile.close();
    jsonFile.close();

    std::cout << "Data berhasil diekspor ke data.json\n";
}


// Cari entri pada tanggal tertentu (DD MM YYYY) dan tampilkan
void searchByDate(int targetDay, int targetMonth, int targetYear) {
    std::ifstream binFile("data.bin", std::ios::binary);
    if (!binFile) {
        std::cerr << "Gagal membuka data.bin.\n";
        return;
    }

    time_t t;
    int status;
    bool found = false;
    while (binFile.read(reinterpret_cast<char*>(&t), sizeof(t))) {
        binFile.read(reinterpret_cast<char*>(&status), sizeof(status));
        tm* ltm = localtime(&t);
        if (ltm->tm_mday == targetDay &&
            ltm->tm_mon + 1 == targetMonth &&
            ltm->tm_year + 1900 == targetYear)
        {
            std::cout << "Waktu: " 
                      << std::put_time(ltm, "%Y-%m-%d %H:%M:%S")
                      << ", Status: " << status << "\n";
            found = true;
        }
    }
    if (!found) {
        std::cout << "Tidak ada data pada tanggal tersebut.\n";
    }
    binFile.close();
}

// Baca semua entri, urutkan berdasarkan status, dan tampilkan
void sortDataByStatus()
{
    std::ifstream binFile("data.bin", std::ios::binary);
    if (!binFile)
    {
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

    // Bubble sort by status
    for (size_t i = 0; i < entries.size(); ++i)
    {
        for (size_t j = 0; j + 1 < entries.size() - i; ++j)
        {
            if (entries[j].status > entries[j + 1].status)
            {
                std::swap(entries[j], entries[j + 1]);
            }
        }
    }

    for (const auto &entry : entries)
    {
        tm *ltm = localtime(&entry.t);
        std::cout << "Waktu: "
                  << std::put_time(ltm, "%Y-%m-%d %H:%M:%S")
                  << ", Status: " << entry.status << "\n";
    }
}

// Mapping rainValue ke status code:
//   rainValue >= 3000   -> status 0 (High Level / Critical!)
//   1500 < rainValue < 3000 -> status 1 (Stable)
//   rainValue <= 1500   -> status 2 (Low Level / Critical!)
int mapRainValueToStatus(int rainValue) {
    if (rainValue >= 3000) return 0;
    else if (rainValue > 1500) return 1;
    else return 2;
}

// Pesan status lengkap dengan timestamp, berdasarkan status code
std::string getStatusMessage(int statusCode) {
    std::string timestamp = getCurrentTimestamp();
    switch (statusCode) {
        case 0: return timestamp + " - Critical! (Low Level)";
        case 1: return timestamp + " - Stable";
        case 2: return timestamp + " - Critical! (High Level)";
        default:
            return timestamp + " - Status tidak dikenali: '" + std::to_string(statusCode) + "'";
    }
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
        // Buat socket TCP
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == INVALID_SOCKET) {
            std::cerr << "Gagal membuat socket.\n";
            return false;
        }

        // Set opsi reuse address
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

        // Siapkan alamat server (INADDR_ANY, port)
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        // Bind ke port
        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "Bind gagal.\n";
            return false;
        }

        // Listen koneksi masuk
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

            // Jalankan handleClient di thread baru agar bisa paralel
            std::thread t(&TCPServer::handleClient, this, client_socket, client_addr);
            t.detach();
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

        std::cout << "Koneksi diterima dari " << client_ip << ".\n";

        char buffer[1024];
        while (true) {
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) {
                break; // Klien disconnect atau error
            }

            buffer[bytes_received] = '\0';
            std::string data(buffer);

            // Data diharapkan berbentuk JSON yang memuat "rainValue":<angka>
            // Contoh: {"rainValue": 2750}
            // Kita parse untuk mendapatkan rainValue
            size_t pos = data.find("\"rainValue\":");
            if (pos != std::string::npos) {
                size_t start = data.find(":", pos) + 1;
                size_t end = data.find("}", start);
                std::string valStr = data.substr(start, end - start);
                int rainValue = std::stoi(valStr);

                int statusCode = mapRainValueToStatus(rainValue);
                // Tampilkan pesan di terminal (bahasa Indonesia formal)
                std::cout << getStatusMessage(statusCode) << "\n";

                // Simpan ke data.bin
                saveToBinary(time(nullptr), statusCode);
            }

            // Kirim balasan "OK" ke klien
            std::string response = "OK";
            send(client_socket, response.c_str(), (int)response.size(), 0);
        }

        closesocket(client_socket);
    }
};

int main() {
    while (true) {
        std::cout << "=== Aplikasi Server Sensor & Terminal UI ===\n";
        std::cout << "Pilih mode:\n"
                     "1. Monitoring (terima data lewat TCP dan simpan ke data.bin)\n"
                     "2. Ekspor ke JSON (data.bin -> data.json)\n"
                     "3. Cari data berdasarkan tanggal\n"
                     "4. Urutkan data berdasarkan status\n"
                     "5. Quit\n"
                     "Pilihan: ";

        int mode;
        std::cin >> mode;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (mode == 2) {
            exportToJson();
            // Kembali ke menu (tidak keluar program)
            continue;
        }
        else if (mode == 3) {
            int d, m, y;
            std::cout << "Masukkan tanggal (DD MM YYYY): ";
            std::cin >> d >> m >> y;
            searchByDate(d, m, y);
            // Kembali ke menu
            continue;
        }
        else if (mode == 4) {
            sortDataByStatus();
            // Kembali ke menu
            continue;
        }
        else if (mode == 5) {
            // Keluar dari loop dan program
            break;
        }
        else if (mode == 1) {
            std::cout << "Mode Monitoring dipilih.\n";
            std::cout << "Menunggu koneksi TCP pada port 8080...\n";
            std::cout << "Tekan 'q' untuk keluar dari monitoring.\n";

            TCPServer server(8080);
            if (!server.start()) {
                std::cerr << "Gagal memulai server.\n";
                // Kembali ke menu (mungkin user akan memilih mode lain)
                continue;
            }

            // Jalankan server di thread terpisah
            std::thread serverThread(&TCPServer::run, &server);

            // Di thread utama, cek input 'q' untuk berhenti monitoring
        #ifdef _WIN32
            while (true) {
                if (_kbhit()) {
                    char ch = _getch();
                    if (ch == 'q' || ch == 'Q') {
                        std::cout << "Berhenti monitoring...\n";
                        server.stop();
                        break;  // Keluar dari loop _kbhit(), tapi tetap di dalam mode 1
                    }
                }
                Sleep(100); // kurangi beban CPU
            }
        #else
            // Jika bukan Windows, cukup tunggu Enter untuk menghentikan monitoring
            std::cin.get();
            server.stop();
        #endif

            serverThread.join();
            std::cout << "Server telah dimatikan.\n";

            // Setelah monitoring dihentikan, kembali ke menu utama
            continue;
        }
        else {
            std::cout << "Pilihan tidak valid. Silakan coba lagi.\n\n";
            continue;
        }
    }

    std::cout << "Terima kasih, program selesai.\n";
    return 0;
}
