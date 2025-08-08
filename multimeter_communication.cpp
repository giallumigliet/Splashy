    #include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

HANDLE setupSerialPort(const char* portName) {
    HANDLE hSerial = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Error during serial port connection" << std::endl;
        return nullptr;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error during port state retrieval" << std::endl;
        CloseHandle(hSerial);
        return nullptr;
    }

    dcbSerialParams.BaudRate = CBR_9600; // Multimeter Baud Rate Confifuration
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error during serial port configuration" << std::endl;
        CloseHandle(hSerial);
        return nullptr;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts)) {
        std::cerr << "Error during port timeout configuration" << std::endl;
        CloseHandle(hSerial);
        return nullptr;
    }

    return hSerial;
}

void sendCommand(HANDLE hSerial, const std::string& command) {
    DWORD bytesWritten;
    WriteFile(hSerial, command.c_str(), command.length(), &bytesWritten, NULL);
    if (bytesWritten != command.length()) {
        std::cerr << "Error during command sending to multimeter" << std::endl;
    }
}

double readMultimeterData(HANDLE hSerial) {
    char buffer[256];
    DWORD bytesRead;

    sendCommand(hSerial, "MEAS:FRESistance?\n");
    if (ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0'; // Ends received string
            std::cout << "Multimeter Data: " << buffer << std::endl; // Prints received data
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return std::stod(buffer); // Convert to double
        } else {
            std::cerr << "No data received." << std::endl;
            return 0.0;
        }
    } else {
        std::cerr << "Error during data reading. Error code: " << GetLastError() << std::endl;
        return 0.0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 50+50=100ms delay between readings
    return 0.0;
}

int main() {
    const char* portName = "COM3"; // COM Port selection
    HANDLE hSerial = setupSerialPort(portName);

    if (hSerial) {
        sendCommand(hSerial, "SYSTem:LOCal\n");
        std::this_thread::sleep_for(std::chrono::seconds(1));  // 1s pause
        sendCommand(hSerial, "SYSTem:REMote\n");
        std::this_thread::sleep_for(std::chrono::seconds(1));  // 1s pause
        readMultimeterData(hSerial); // Reads and prints data from multimeter
        
        
        // Data collection with timing
        int n_datos = 10;  // Adjust this based on your data requirements
        std::vector<double> data_r(n_datos, 0.0);
        std::vector<double> T(n_datos, 0.0);

        for (int i = 0; i < n_datos; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            
            double resistance = readMultimeterData(hSerial);
            data_r[i] = resistance;

            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Equivalent to MATLAB's pause(0.01)

            auto end = std::chrono::high_resolution_clock::now();
            T[i] = std::chrono::duration<double>(end - start).count();
        }

        // Sum of all times
        double tFor = 0.0;
        for (const auto& time : T) {
            tFor += time;
        }
        std::cout << "Total time (tFor): " << tFor << " seconds" << std::endl;

        // Accumulated time calculation
        std::vector<double> time(T.size(), 0.0);
        time[0] = T[0];
        for (size_t i = 1; i < T.size(); ++i) {
            time[i] = T[i] + time[i - 1];
        }

        // Print resistance vector and time vector
        for (size_t i = 0; i < time.size(); ++i) {
            std::cout << "Resistance reading " << i + 1 << ": " << data_r[i] << " ohm" << std::endl;
        }  

        for (size_t i = 0; i < time.size(); ++i) {
            std::cout << "Accumulated time at index " << i + 1 << ": " << time[i] << " s" << std::endl;
        }  
        

        sendCommand(hSerial, "SYSTem:LOCal\n");
        CloseHandle(hSerial); // Closes serial connection
    }



    return 0;
}
