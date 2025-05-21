#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

const int DEFAULT_PORT = 12345;
const int BUFFER_SIZE = 1024;  // Keep buffer size smaller for better memory usage
const int MAX_NAME_LENGTH = 256;

// Message types
enum class MessageType {
    INIT,           // Client initialization
    REGULAR,        // Regular message (clipboard content)
    REGULAR_CHUNK,  // Part of a large message
    REGULAR_END     // End of a large message
};

struct ClientInfo {
    char username[MAX_NAME_LENGTH];
    char hostname[MAX_NAME_LENGTH];
};

struct Message {
    MessageType type;
    union {
        ClientInfo clientInfo;
        char data[BUFFER_SIZE];
    };
}; 