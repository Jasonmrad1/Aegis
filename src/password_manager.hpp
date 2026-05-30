#pragma once
#include <string>
#include <vector>
#include <entry.hpp>
#include <auth_manager.hpp>

class PasswordManager {
    public:
    explicit PasswordManager(AuthManager& auth);
    ~PasswordManager() = default;
    bool addEntry(const std::string& website, const std::string& username, const std::string& password, const std::string& note, std::string& error);
    bool updateEntry(int id, const std::string& website, const std::string& username, const std::string& password, const std::string& note, std::string& error);
    bool clearAll(std::string& error);
    bool changeMasterPassword(const std::string& currentPassword, const std::string& newPassword, std::string& error);
    bool verifyMasterPassword(const std::string& password) const;
    void displayEntries() const;
    void displayEntries(const std::vector<Entry>& entries) const;
    void displayEntries(const std::vector<Entry>& entries, bool showPassword) const;
    void displayEntry(const Entry& entry, bool showPassword) const;
    std::vector<Entry> searchEntries(const std::string& website) const;
    bool deleteEntry(int id, std::string& error);
    bool getEntryById(int id, Entry& out) const;
    void load();
    bool load(std::string& error);
    bool save(std::string& error);

    private:
    AuthManager& m_auth;
    std::vector<Entry> m_entries;
    int entry_id = 1;
};