#include "common.h"
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <map>

struct ClientConnection {
    SOCKET socket;
    ClientInfo info;
};

std::map<int, ClientConnection> clients;  // Client ID -> connection info
std::mutex clientsMutex;
int nextClientId = 1;

void HandleClient(SOCKET clientSocket, int clientId) {
    Message msg = {};
    ClientConnection& client = clients[clientId];

    // Get client info
    int bytesReceived = recv(clientSocket, (char*)&msg, sizeof(Message), 0);
    if (bytesReceived > 0 && msg.type == MessageType::INIT) {
        client.info = msg.clientInfo;
        std::cout << "New client connected [" << clientId << "]: " 
                  << client.info.username << "@" << client.info.hostname << std::endl;
    }

    while (true) {
        bytesReceived = recv(clientSocket, (char*)&msg, sizeof(Message), 0);
        if (bytesReceived <= 0) break;

        if (msg.type == MessageType::REGULAR) {
            std::cout << "\nClipboard content from [" << clientId << "] " 
                      << client.info.username << "@" << client.info.hostname << ":\n" 
                      << msg.data << std::endl;
        }
    }

    // Remove client on disconnect
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(clientId);
    }
    std::cout << "Client [" << clientId << "] disconnected" << std::endl;
    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(DEFAULT_PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is listening on port " << DEFAULT_PORT << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        int clientId;
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clientId = nextClientId++;
            clients[clientId] = {clientSocket, {}};
        }

        std::thread clientThread(HandleClient, clientSocket, clientId);
        clientThread.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}