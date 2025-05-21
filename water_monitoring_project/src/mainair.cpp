#include <windows.h>
#include <iostream>
#include <algorithm>

int main() {
    HANDLE hSerial = CreateFile("COM3", GENERIC_READ, 0, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Gagal membuka COM3!\n";
        return 1;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Gagal baca config COM3\n";
        return 1;
    }

    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity   = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Gagal set config COM3\n";
        return 1;
    }

    char incomingByte;
    DWORD bytesRead;
    std::string buffer;

    std::cout << "Listening from COM3...\n";

    while (true) {
        if (ReadFile(hSerial, &incomingByte, 1, &bytesRead, NULL)) {
            if (bytesRead == 1) {
                if (incomingByte == '\n') {
    				buffer.erase(std::remove(buffer.begin(), buffer.end(), '\r'), buffer.end());
    				if (buffer == "1") {
        				std::cout << "Air tak terdeteksi\n";
    				} else if (buffer == "0") {
        				std::cout << "Air terdeteksi!\n";
    				}
    				buffer.clear();
				}
 				else {
                    buffer += incomingByte;
                }
            }
        }
    }

    CloseHandle(hSerial);
    return 0;
}

