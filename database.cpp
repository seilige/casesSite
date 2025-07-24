#include <sqlite3.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

class Database {
public:
    Database(const std::string &dbPath) {
        int rc = sqlite3_open(dbPath.c_str(), &db);
        if (rc != SQLITE_OK) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
            std::cerr << "Database path: " << dbPath << std::endl;
            if (db) {
                sqlite3_close(db);
            }
            db = nullptr;
            return;
        }
        std::cout << "Database opened successfully: " << dbPath << std::endl;
        initialize();
    }

    ~Database() {
        if (db) {
            sqlite3_close(db);
        }
    }

    bool registerUser(const std::string &username, const std::string &password, const std::string &avatar) {
    if (!db) {
        std::cerr << "Database not initialized" << std::endl;
        return false;
    }

        std::string sql = "INSERT INTO users (username, password, avatar) VALUES (?, ?, ?);";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, avatar.c_str(), -1, SQLITE_STATIC);

        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    bool authenticateUser(const std::string &username, const std::string &password) {
        std::string sql = "SELECT COUNT(*) FROM users WHERE username = ? AND password = ?;";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

        bool authenticated = false;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            authenticated = (sqlite3_column_int(stmt, 0) > 0);
        }
        sqlite3_finalize(stmt);
        return authenticated;
    }

    bool saveItemForUser(const std::string &username, const std::string &item) {
        std::string sql = "INSERT INTO user_items (username, item) VALUES (?, ?);";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, item.c_str(), -1, SQLITE_STATIC);

        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    // Получение данных пользователя
    std::unordered_map<std::string, std::string> getUserData(const std::string &username) {
        std::unordered_map<std::string, std::string> userData;
        std::string sql = "SELECT username, avatar FROM users WHERE username = ?;";
        sqlite3_stmt *stmt;
        
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            return userData;
        }
        
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            userData["username"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            userData["avatar"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        }
        
        sqlite3_finalize(stmt);
        return userData;
    }

    // Получение предметов пользователя
    std::vector<std::string> getUserItems(const std::string &username) {
        std::vector<std::string> items;
        std::string sql = "SELECT item FROM user_items WHERE username = ? ORDER BY id DESC;";
        sqlite3_stmt *stmt;
        
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            return items;
        }
        
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            items.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        }
        
        sqlite3_finalize(stmt);
        return items;
    }

    // Проверка существования пользователя
    bool userExists(const std::string &username) {
        std::string sql = "SELECT COUNT(*) FROM users WHERE username = ?;";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

        bool exists = false;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            exists = (sqlite3_column_int(stmt, 0) > 0);
        }
        sqlite3_finalize(stmt);
        return exists;
    }

private:
    sqlite3 *db;

    void initialize() {
        const char *createUsersTable = R"(
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                password TEXT NOT NULL,
                avatar TEXT DEFAULT '/static/images/default_avatar.jpg',
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP
            );
        )";

        const char *createUserItemsTable = R"(
            CREATE TABLE IF NOT EXISTS user_items (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT NOT NULL,
                item TEXT NOT NULL,
                obtained_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (username) REFERENCES users (username)
            );
        )";

        char *errMsg = nullptr;
        if (sqlite3_exec(db, createUsersTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "Failed to create users table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }

        if (sqlite3_exec(db, createUserItemsTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "Failed to create user_items table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
    }
};
