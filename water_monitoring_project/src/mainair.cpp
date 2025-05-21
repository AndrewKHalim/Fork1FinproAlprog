#include <windows.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <ctime>
#include <iomanip>
#include <string>

struct DataEntry
{
    time_t t;
    int status;
};

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

int main()
{
    HANDLE hSerial = CreateFile("COM3", GENERIC_READ, 0, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hSerial == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Gagal membuka COM3!\n";
        return 1;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams))
    {
        std::cerr << "Gagal baca config COM3\n";
        return 1;
    }

    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams))
    {
        std::cerr << "Gagal set config COM3\n";
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

    std::cout << "Listening from COM3...\n";

    while (true)
    {
        if (ReadFile(hSerial, &incomingByte, 1, &bytesRead, NULL))
        {
            if (bytesRead == 1)
            {
                if (incomingByte == '\n')
                {
                    buffer.erase(std::remove(buffer.begin(), buffer.end(), '\r'), buffer.end());
                    if (buffer == "1")
                    {
                        std::cout << "Air tak terdeteksi\n";
                        saveToBinary(time(nullptr), 0);
                    }
                    else if (buffer == "0")
                    {
                        std::cout << "Air terdeteksi!\n";
                        saveToBinary(time(nullptr), 1);
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
