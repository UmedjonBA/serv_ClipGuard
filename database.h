#pragma once

#include <pqxx/pqxx>
#include <string>
#include "db_config.h"

class Database {
public:
    Database() {
        try {
            std::string conn_string = "host=" + std::string(DBConfig::host) +
                                    " port=" + std::string(DBConfig::port) +
                                    " dbname=" + std::string(DBConfig::dbname) +
                                    " user=" + std::string(DBConfig::user) +
                                    " password=" + std::string(DBConfig::password);
            conn = std::make_unique<pqxx::connection>(conn_string);
        } catch (const std::exception& e) {
            std::cerr << "Database connection error: " << e.what() << std::endl;
            throw;
        }
    }

    void saveClipboardEvent(const std::string& username,
                          const std::string& hostname,
                          const std::string& event_type,
                          const std::string& content = "",
                          const std::string& file_path = "",
                          const std::string& file_type = "") {
        try {
            pqxx::work txn(*conn);
            
            // Если это файл, извлекаем информацию из content
            if (event_type == "file") {
                std::string path;
                std::string type;
                
                // Ищем строки, начинающиеся с "File: " и "Type: "
                size_t filePos = content.find("File: ");
                size_t typePos = content.find("Type: ");
                
                if (filePos != std::string::npos) {
                    size_t fileEnd = content.find("\n", filePos);
                    if (fileEnd != std::string::npos) {
                        path = content.substr(filePos + 6, fileEnd - (filePos + 6));
                    }
                }
                
                if (typePos != std::string::npos) {
                    size_t typeEnd = content.find("\n", typePos);
                    if (typeEnd != std::string::npos) {
                        type = content.substr(typePos + 6, typeEnd - (typePos + 6));
                    }
                }
                
                std::string query = "INSERT INTO clipboard_events (username, hostname, event_type, content, file_path, file_type) "
                    "VALUES (" + txn.quote(username) + ", " + 
                    txn.quote(hostname) + ", " + 
                    txn.quote(event_type) + ", " + 
                    txn.quote(content) + ", " + 
                    txn.quote(path) + ", " + 
                    txn.quote(type) + ")";
                txn.exec(query).exec();
            } else {
                // Для текстовых событий
                std::string query = "INSERT INTO clipboard_events (username, hostname, event_type, content) "
                    "VALUES (" + txn.quote(username) + ", " + 
                    txn.quote(hostname) + ", " + 
                    txn.quote(event_type) + ", " + 
                    txn.quote(content) + ")";
                txn.exec(query).exec();
            }
            txn.commit();
        } catch (const std::exception& e) {
            std::cerr << "Error saving clipboard event: " << e.what() << std::endl;
            throw;
        }
    }

private:
    std::unique_ptr<pqxx::connection> conn;
}; 