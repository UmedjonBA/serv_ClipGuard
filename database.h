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
            txn.exec_params(
                "INSERT INTO clipboard_events (username, hostname, event_type, content, file_path, file_type) "
                "VALUES ($1, $2, $3, $4, $5, $6)",
                username, hostname, event_type, content, file_path, file_type
            );
            txn.commit();
        } catch (const std::exception& e) {
            std::cerr << "Error saving clipboard event: " << e.what() << std::endl;
            throw;
        }
    }

private:
    std::unique_ptr<pqxx::connection> conn;
}; 