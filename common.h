#pragma once

#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

constexpr int DEFAULT_PORT = 12345;
constexpr int BUFFER_SIZE = 1024;
constexpr int MAX_NAME_LENGTH = 256;

// Message types
enum class MessageType {
    INIT,    // Client initialization
    REGULAR  // Regular message (clipboard content)
};

struct ClientInfo {
    char username[MAX_NAME_LENGTH];
    char hostname[MAX_NAME_LENGTH];
};

struct Message {
    ClientInfo clientInfo;
    MessageType type;
    char data[BUFFER_SIZE];
}; 