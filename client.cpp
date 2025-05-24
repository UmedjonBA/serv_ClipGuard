#include "common.h"
#include <iostream>
#include <thread>
#include <string>
#include <Lmcons.h>
#include <windows.h>

// Функция для получения информации о файлах из буфера обмена
std::wstring ProcessFileClipboard() {
    std::wstring result;  // Строка для хранения результата
    HDROP hDrop = (HDROP)GetClipboardData(CF_HDROP);  // Получаем дескриптор данных буфера обмена для файлов
    if (hDrop != NULL) {  // Если в буфере есть файлы
        UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);  // Получаем количество файлов
        
        for (UINT i = 0; i < fileCount; i++) {  // Перебираем все файлы
            TCHAR szFile[MAX_PATH];  // Буфер для пути к файлу
            if (DragQueryFile(hDrop, i, szFile, MAX_PATH)) {  // Получаем путь к текущему файлу
                std::wstring widePath(szFile);  // Преобразуем путь в широкую строку
                result += L"File: " + widePath + L"\n";  // Добавляем информацию о файле
                
                // Получаем расширение файла
                size_t dotPos = widePath.find_last_of(L".");  // Ищем последнюю точку в пути
                if (dotPos != std::wstring::npos) {  // Если точка найдена
                    std::wstring wideExt = widePath.substr(dotPos + 1);  // Извлекаем расширение
                    result += L"Type: " + wideExt + L"\n";  // Добавляем информацию о типе файла
                }
            }
        }
    }
    return result;  // Возвращаем собранную информацию
}

// Функция для получения текста из буфера обмена
std::wstring ProcessTextClipboard() {
    std::wstring result;  // Строка для хранения результата
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);  // Получаем дескриптор текстовых данных
    if (hData != NULL) {  // Если в буфере есть текст
        wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));  // Блокируем память и получаем указатель на текст
        if (pszText != NULL) {  // Если указатель получен успешно
            result = L"Text content: " + std::wstring(pszText);  // Формируем строку с текстом
            GlobalUnlock(hData);  // Разблокируем память
        }
    }
    return result;  // Возвращаем текст
}

// Функция для отправки длинных сообщений по частям
void SendLargeMessage(SOCKET sock, const std::wstring& content) {
    // Преобразуем широкую строку в UTF-8
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, NULL, 0, NULL, NULL);  // Получаем необходимый размер буфера
    if (size_needed <= 0) return;  // Если преобразование не удалось, выходим

    std::vector<char> utf8_str(size_needed);  // Создаем буфер для UTF-8 строки
    WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, utf8_str.data(), size_needed, NULL, NULL);  // Выполняем преобразование

    // Отправляем сообщение по частям
    size_t total_size = strlen(utf8_str.data());  // Получаем общий размер сообщения
    size_t sent = 0;  // Счетчик отправленных байт
    bool is_first = true;  // Флаг первого чанка

    while (sent < total_size) {  // Пока не отправили все сообщение
        Message msg = {};  // Создаем структуру сообщения
        msg.type = is_first ? MessageType::REGULAR : MessageType::REGULAR_CHUNK;  // Определяем тип сообщения
        // Вычисляем размер текущего чанка (берем минимум из оставшегося размера и размера буфера)
        size_t chunk_size = (total_size - sent < (size_t)BUFFER_SIZE - 1) ? (total_size - sent) : (BUFFER_SIZE - 1);
        
        strncpy_s(msg.data, utf8_str.data() + sent, chunk_size);  // Копируем часть сообщения в буфер
        send(sock, (char*)&msg, sizeof(Message), 0);  // Отправляем чанк
        
        sent += chunk_size;  // Увеличиваем счетчик отправленных байт
        is_first = false;  // Сбрасываем флаг первого чанка
    }

    // Отправляем сообщение о завершении
    Message end_msg = {};  // Создаем структуру для конечного сообщения
    end_msg.type = MessageType::REGULAR_END;  // Устанавливаем тип сообщения
    send(sock, (char*)&end_msg, sizeof(Message), 0);  // Отправляем сообщение о завершении
}

// Функция мониторинга буфера обмена
void MonitorClipboard(SOCKET sock) {
    DWORD lastSequence = GetClipboardSequenceNumber();  // Получаем начальный номер последовательности
    
    while (true) {  // Бесконечный цикл мониторинга
        DWORD currentSequence = GetClipboardSequenceNumber();  // Получаем текущий номер последовательности
        
        if (currentSequence != lastSequence) {  // Если содержимое буфера изменилось
            lastSequence = currentSequence;  // Обновляем номер последовательности
            
            if (OpenClipboard(NULL)) {  // Открываем буфер обмена
                std::wstring clipboardContent;  // Строка для хранения содержимого
                
                // Проверяем наличие файлов
                if (IsClipboardFormatAvailable(CF_HDROP)) {
                    clipboardContent = L"Files detected in clipboard:\n" + ProcessFileClipboard();
                }
                // Проверяем наличие текста
                else if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
                    clipboardContent = L"Text detected in clipboard:\n" + ProcessTextClipboard();
                }
                
                if (!clipboardContent.empty()) {  // Если есть содержимое
                    SendLargeMessage(sock, clipboardContent);  // Отправляем его
                }
                
                CloseClipboard();  // Закрываем буфер обмена
            }
        }
        
        Sleep(100);  // Пауза 100 мс между проверками
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <server_ip>" << std::endl;
        return 1;
    }

    WSADATA wsaData;  // Структура для инициализации WinSock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {  // Инициализируем WinSock
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);  // Создаем сокет
    if (clientSocket == INVALID_SOCKET) {  // Проверяем успешность создания
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;  // Структура адреса сервера
    serverAddr.sin_family = AF_INET;  // Устанавливаем семейство адресов
    serverAddr.sin_port = htons(DEFAULT_PORT);  // Устанавливаем порт
    inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);  // Устанавливаем IP-адрес из аргумента командной строки

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {  // Подключаемся к серверу
        std::cerr << "Failed to connect to server" << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Получаем имя пользователя
    char username[UNLEN + 1];
    DWORD usernameLen = UNLEN + 1;
    GetUserNameA(username, &usernameLen);

    // Получаем имя компьютера
    char hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD hostnameLen = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameA(hostname, &hostnameLen);

    // Отправляем инициализационное сообщение
    Message initMsg = {};
    initMsg.type = MessageType::INIT;
    strncpy_s(initMsg.clientInfo.username, username, MAX_NAME_LENGTH - 1);  // Копируем имя пользователя
    strncpy_s(initMsg.clientInfo.hostname, hostname, MAX_NAME_LENGTH - 1);  // Копируем имя компьютера
    send(clientSocket, (char*)&initMsg, sizeof(Message), 0);  // Отправляем сообщение

    std::cout << "Connected to server. Monitoring clipboard..." << std::endl;

    // Запускаем мониторинг буфера обмена в отдельном потоке
    std::thread clipboardThread(MonitorClipboard, clientSocket);
    clipboardThread.join();  // Ждем завершения потока

    closesocket(clientSocket);  // Закрываем сокет
    WSACleanup();  // Очищаем WinSock
    return 0;
} 