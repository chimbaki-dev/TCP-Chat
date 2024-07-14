#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <string>
#include <thread>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

void moveCursorToStartOfLine() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(hStdout, &csbi)) {
        COORD startPos = { 0, csbi.dwCursorPosition.Y };
        SetConsoleCursorPosition(hStdout, startPos);
    }
}

void saveCursorPos() {
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout, &csbi);
    COORD newPos = { 0, static_cast<SHORT>(csbi.dwCursorPosition.Y + 1) };
    SetConsoleCursorPosition(hStdout, newPos);
}

void restoreCursorPos(int length) {
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout, &csbi);
    COORD pos = { length, csbi.dwCursorPosition.Y - 1 };
    SetConsoleCursorPosition(hStdout, pos);
}

void receiveMessages(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    while (true) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            saveCursorPos();
            std::cout << "\r" << std::string(100, ' ') << "\r";
            std::cout << buffer << "\n> ";
            std::cout.flush();
            restoreCursorPos(2);
        }
        else if (bytesReceived == 0) {
            std::cout << "Server closed connection\n";
            break;
        }
        else {
            std::cerr << "Receive failed with error: " << WSAGetLastError() << "\n";
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // Create socket
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // Setup server address structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address/Address not supported\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connect failed with error: " << WSAGetLastError() << "\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server\n";

    // Get username
    std::string username;
    std::cout << "Enter your username: ";
    std::getline(std::cin, username);

    // Start a thread to receive messages
    std::thread(receiveMessages, clientSocket).detach();

    // Send messages to server
    std::string message;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, message);
        if (message == "/quit") {
            break;
        }
        std::string fullMessage = username + ": " + message;
        if (send(clientSocket, fullMessage.c_str(), fullMessage.length(), 0) == SOCKET_ERROR) {
            std::cerr << "Send failed with error: " << WSAGetLastError() << "\n";
            break;
        }

        // Move cursor to start of line and print the sent message
        moveCursorToStartOfLine();
        std::cout << fullMessage << "\n";
    }

    // Cleanup
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
