#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <fcntl.h>
#include <io.h>

// Function to convert UTF-16 to UTF-8 using WinAPI
std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return std::string();
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), NULL, 0, NULL, NULL);
    std::string utf8(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), &utf8[0], size_needed, NULL, NULL);
    return utf8;
}

// Function to get file information from clipboard
void ProcessFileClipboard() {
    HDROP hDrop = (HDROP)GetClipboardData(CF_HDROP);
    if (hDrop != NULL) {
        UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
        
        for (UINT i = 0; i < fileCount; i++) {
            TCHAR szFile[MAX_PATH];
            if (DragQueryFile(hDrop, i, szFile, MAX_PATH)) {
                std::wstring widePath(szFile);
                std::string utf8Path = WideToUtf8(widePath);
                std::cout << "File: " << utf8Path << std::endl;
                
                // Get file extension
                size_t dotPos = widePath.find_last_of(L".");
                if (dotPos != std::wstring::npos) {
                    std::wstring wideExt = widePath.substr(dotPos + 1);
                    std::string utf8Ext = WideToUtf8(wideExt);
                    std::cout << "Type: " << utf8Ext << std::endl;
                }
            }
        }
    }
}

// Function to get text from clipboard
void ProcessTextClipboard() {
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData != NULL) {
        wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
        if (pszText != NULL) {
            std::wstring wideText(pszText);
            std::string utf8Text = WideToUtf8(wideText);
            std::cout << "Text content: " << utf8Text << std::endl;
            GlobalUnlock(hData);
        }
    }
}

int main() {
    // Set console to use UTF-8
    SetConsoleOutputCP(CP_UTF8);
    
    // Test Chinese characters output
    std::cout << "Testing UTF-8 output with Chinese characters:" << std::endl;
    std::cout << "你好世界 (Hello World)" << std::endl;
    std::cout << "中国 (China)" << std::endl;
    std::cout << "北京 (Beijing)" << std::endl;
    std::cout << "------------------------" << std::endl;
    
    std::cout << "Clipboard Monitor Started. Press Ctrl+C to exit." << std::endl;
    
    // Previous clipboard sequence number
    DWORD lastSequence = GetClipboardSequenceNumber();
    
    while (true) {
        // Check if clipboard content has changed
        DWORD currentSequence = GetClipboardSequenceNumber();
        
        if (currentSequence != lastSequence) {
            lastSequence = currentSequence;
            
            if (OpenClipboard(NULL)) {
                // Check for files
                if (IsClipboardFormatAvailable(CF_HDROP)) {
                    std::cout << "\nFiles detected in clipboard:" << std::endl;
                    ProcessFileClipboard();
                }
                
                // Check for text
                if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
                    std::cout << "\nText detected in clipboard:" << std::endl;
                    ProcessTextClipboard();
                }
                
                CloseClipboard();
            }
        }
        
        Sleep(100); // Check every 100ms
    }
    
    return 0;
} 