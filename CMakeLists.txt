cmake_minimum_required(VERSION 3.10)
project(clipboard_monitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find PostgreSQL
find_package(PostgreSQL REQUIRED)
find_package(libpqxx REQUIRED)

# Add server executable
add_executable(server
    server.cpp
    database.cpp
)

# Add client executable
add_executable(client
    client.cpp
)

# Link libraries for server
target_link_libraries(server
    PRIVATE
    ws2_32
    pqxx
    pq
)

# Link libraries for client
target_link_libraries(client
    PRIVATE
    ws2_32
) 