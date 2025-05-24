cmake_minimum_required(VERSION 3.10)
project(clipboard_monitor)

# Настройка vcpkg
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройки для статической сборки
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")

# Настройки для статической линковки
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")

# Явно указываем путь к pqxx, если он установлен в нестандартном месте
set(PQXX_ROOT "C:/Program Files/libpqxx" CACHE PATH "Path to libpqxx installation")
set(PQXX_STATIC_LIBRARY "C:/Program Files/libpqxx/lib/pqxx.lib" CACHE FILEPATH "Path to pqxx static library")

# Find packages
find_package(PostgreSQL REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

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
    libpqxx::pqxx_static
    libpq
)

# Link libraries for client
target_link_libraries(client
    PRIVATE
    ws2_32
)

# Установка флагов компилятора для статической сборки
if(MSVC)
    target_compile_options(server PRIVATE /MT$<$<CONFIG:Debug>:d>)
    target_compile_options(client PRIVATE /MT$<$<CONFIG:Debug>:d>)
endif() 