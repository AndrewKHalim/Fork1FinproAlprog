#include <windows.h>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <string>

std::string getCurrentTimestamp() {
    time_t now = time(0);
    tm* localTime = localtime(&now);

    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localTime);
    return std::string(buf);
}

int main() {
    HANDLE hSerial = CreateFile("COM7", GENERIC_READ, 0, NULL,				// ganti COM7 sesuai COM masing2
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Gagal membuka COM7!\n";
        return 1;
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    GetCommState(hSerial, &dcb);
    dcb.BaudRate = CBR_9600; dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT; dcb.Parity = NOPARITY;
    SetCommState(hSerial, &dcb);

    std::cout << "Listening from COM7...\n";
    std::string buffer;
    char incoming;
    DWORD bytesRead;

    while (true) {
        if (ReadFile(hSerial, &incoming, 1, &bytesRead, NULL) && bytesRead == 1) {
            if (incoming == '\n') {
                buffer.erase(std::remove(buffer.begin(), buffer.end(), '\r'), buffer.end());

                std::string timestamp = getCurrentTimestamp();

                if (buffer == "2") {
                    std::cout << timestamp << " - Critical! (Low Level)\n";
                }
                else if (buffer == "1") {
                    std::cout << timestamp << " - Stable\n";
                }
                else if (buffer == "0") {
                    std::cout << timestamp << " - Critical! (High Level)\n";
                }
                else {
                    std::cout << timestamp << " - Status tidak dikenali: '" << buffer << "'\n";
                }

                buffer.clear();
            } else {
                buffer += incoming;
            }
        }
    }

    CloseHandle(hSerial);
    return 0;
}
