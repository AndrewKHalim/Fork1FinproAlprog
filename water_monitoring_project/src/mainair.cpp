#include <windows.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <ctime>
#include <iomanip>
#include <string>
#include <conio.h>

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

std::string getStatusMessage(const std::string& buffer) {
    std::string timestamp = getCurrentTimestamp();
    
    if (buffer == "2") {
        return timestamp + " - Critical! (Low Level)";
    }
    else if (buffer == "1") {
        return timestamp + " - Stable";
    }
    else if (buffer == "0") {
        return timestamp + " - Critical! (High Level)";
    }
    else {
        return timestamp + " - Status tidak dikenali: '" + buffer + "'";
    }
}

int getStatusCode(const std::string& buffer) {
    if (buffer == "2") return 2;  // Low Level
    if (buffer == "1") return 1;  // Stable
    if (buffer == "0") return 0;  // High Level
    return -1; // Unknown status
}

int main()
{
    std::cout << "Pilih COM Port:\n1. COM3 (mainair logic)\n2. COM7 (mainlogic logic)\nPilihan (1/2): ";
    int comChoice;
    std::cin >> comChoice;
    
    std::string comPort = (comChoice == 2) ? "COM7" : "COM3";
    
    HANDLE hSerial = CreateFile(comPort.c_str(), GENERIC_READ, 0, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hSerial == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Gagal membuka " << comPort << "!\n";
        return 1;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams))
    {
        std::cerr << "Gagal baca config " << comPort << "\n";
        return 1;
    }

    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams))
    {
        std::cerr << "Gagal set config " << comPort << "\n";
        return 1;
    }

    char incomingByte;
    DWORD bytesRead;
    std::string buffer;

    std::cout << "Pilih mode:\n1. Monitoring (default)\n2. Ekspor ke JSON\n3. Cari data berdasarkan tanggal\n4. Urutkan data berdasarkan status\nPilihan: ";
    int mode;
    std::cin >> mode;
    std::cin.ignore();
    
    if (mode == 2)
    {
        exportToJson();
        return 0;
    }
    else if (mode == 3)
    {
        int d, m, y;
        std::cout << "Masukkan tanggal (DD MM YYYY): ";
        std::cin >> d >> m >> y;
        searchByDate(d, m, y);
        return 0;
    }
    else if (mode == 4)
    {
        sortDataByStatus();
        return 0;
    }

    std::cout << "Listening from " << comPort << "...\n";
    std::cout << "Tekan 'q' untuk keluar dari monitoring.\n";

    while (true)
    {
        if (_kbhit())
        {
            char ch = _getch();
            if (ch == 'q' || ch == 'Q')
            {
                std::cout << "Keluar dari mode monitoring.\n";
                break;
            }
        }
        
        if (ReadFile(hSerial, &incomingByte, 1, &bytesRead, NULL))
        {
            if (bytesRead == 1)
            {
                if (incomingByte == '\n')
                {
                    buffer.erase(std::remove(buffer.begin(), buffer.end(), '\r'), buffer.end());
                    
                    // Menampilkan pesan status dengan timestamp
                    std::cout << getStatusMessage(buffer) << "\n";
                    
                    // Menyimpan data ke binary file
                    int statusCode = getStatusCode(buffer);
                    if (statusCode != -1) {
                        saveToBinary(time(nullptr), statusCode);
                    }
                    
                    buffer.clear();
                }
                else
                {
                    buffer += incomingByte;
                }
            }
        }
    }

    CloseHandle(hSerial);
    return 0;
}