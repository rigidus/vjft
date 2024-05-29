#include "database.hpp"
#include <iostream>

Database::Database(const std::string& db_name) {
    if (sqlite3_open(db_name.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db_) << "\n";
        sqlite3_close(db_);
    }

    std::string create_messages_table = "CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY, username TEXT, message TEXT);";
    executeQuery(create_messages_table);
}

Database::~Database() {
    sqlite3_close(db_);
}

bool Database::storeMessage(const std::string& username, const std::string& message) {
    std::string query = "INSERT INTO messages (username, message) VALUES ('" + username + "', '" + message + "');";
    return executeQuery(query);
}

std::vector<std::string> Database::getMessages(const std::string& username) {
    std::vector<std::string> messages;
    std::string query = "SELECT message FROM messages WHERE username = '" + username + "';";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            messages.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        }
        sqlite3_finalize(stmt);
    }
    return messages;
}

    bool Database::executeQuery(const std::string& query) {
        char* errMsg = nullptr;
        if (sqlite3_exec(db_, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "SQL error: " << errMsg << "\n";
            sqlite3_free(errMsg);
            return false;
        }
        return true;
    }
