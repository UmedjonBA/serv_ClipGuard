#pragma once

#include <string>

struct DBConfig {
    static constexpr const char* host = "localhost";
    static constexpr const char* port = "5432";
    static constexpr const char* dbname = "clipboard_monitor";
    static constexpr const char* user = "postgres";
    static constexpr const char* password = "your_password";  // Change this to your actual password
}; 