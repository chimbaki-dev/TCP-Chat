#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <string>
#include <vector>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

std::vector<SOCKET> clients;
std::mutex clientsMutex;

void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    while (true) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::string message = buffer;

            // Broadcast message to all clients
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (const auto& client : clients) {
                if (client != clientSocket) {
                    send(client, message.c_str(), message.length(), 0);
                }
            }
            std::cout << "Broadcast: " << message << "\n";
        } else if (bytesReceived == 0) {
            std::cout << "Client disconnected\n";
            break;
        } else {
            std::cerr << "Receive failed with error: " << WSAGetLastError() << "\n";
            break;
        }
    }

    // Remove client from list and cleanup
    std::lock_guard<std::mutex> lock(clientsMutex);
    clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket;
    struct sockaddr_in serverAddr;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // Create socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // Bind socket
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port 8080...\n";

    while (true) {
        SOCKET clientSocket;
        struct sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);

        // Accept a client socket
        if ((clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen)) == INVALID_SOCKET) {
            std::cerr << "Accept failed with error: " << WSAGetLastError() << "\n";
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        std::cout << "Client connected\n";
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientSocket);
        }

        // Create a new thread for the client
        std::thread(handleClient, clientSocket).detach();
    }

    // Cleanup
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
