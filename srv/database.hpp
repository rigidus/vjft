#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <sqlite3.h>
#include <string>
#include <vector>

class Database {
public:
    Database(const std::string& db_name);
    ~Database();

    bool addUser(const std::string& username, const std::string& password);
    bool authenticateUser(const std::string& username, const std::string& password);
    bool storeMessage(const std::string& username, const std::string& message);
    std::vector<std::string> getMessages(const std::string& username);

private:
    sqlite3* db_;
    bool executeQuery(const std::string& query);
};

#endif // DATABASE_HPP
