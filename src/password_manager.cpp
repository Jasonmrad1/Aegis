#include <password_manager.hpp>
#include <storage.hpp>
#include <ui.hpp>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cctype>
#include <windows.h>

#ifdef max
#undef max
#endif

namespace {
    void secureZeroVector(std::vector<unsigned char>& data)
    {
        if (!data.empty()) {
            SecureZeroMemory(data.data(), data.size());
            data.clear();
        }
    }
}

static std::string toLower(const std::string& text)
{
    std::string out;
    out.reserve(text.size());

    for (unsigned char c : text) {
        out.push_back(static_cast<char>(std::tolower(c)));
    }

    return out;
}

PasswordManager::PasswordManager(AuthManager& auth) : m_auth(auth) {}

bool PasswordManager::addEntry(const std::string& website, const std::string& username, const std::string& password, const std::string& note, std::string& error) {
    m_entries.emplace_back(entry_id++ ,website, username, password, note);
    return save(error);
}

bool PasswordManager::updateEntry(int id, const std::string& website, const std::string& username, const std::string& password, const std::string& note, std::string& error)
{
    auto it = std::find_if(m_entries.begin(), m_entries.end(), [id] (const Entry& entry) { return entry.id == id; });
    if (it == m_entries.end()) {
        error = "Not found";
        return false;
    }

    Entry original = *it;
    it->website = website;
    it->username = username;
    it->password = password;
    it->note = note;

    if (!save(error)) {
        *it = original;
        return false;
    }

    return true;
}

bool PasswordManager::clearAll(std::string& error)
{
    m_entries.clear();
    entry_id = 1;
    return save(error);
}

bool PasswordManager::changeMasterPassword(const std::string& currentPassword, const std::string& newPassword, std::string& error)
{
    if (!m_auth.verifyPassword(currentPassword)) {
        error = "Current password is incorrect";
        return false;
    }

    std::vector<unsigned char> oldKey = m_auth.getKey();
    std::vector<unsigned char> oldSalt = m_auth.getSalt();

    m_auth.setMasterPassword(newPassword);
    m_auth.clearSalt();
    m_auth.clearKey();

    if (!save(error)) {
        m_auth.setKey(oldKey);
        m_auth.setSalt(oldSalt);
        m_auth.clearMasterPassword();
        secureZeroVector(oldKey);
        return false;
    }

    secureZeroVector(oldKey);

    return true;
}

bool PasswordManager::verifyMasterPassword(const std::string& password) const
{
    return m_auth.verifyPassword(password);
}

void PasswordManager::displayEntries() const {
    displayEntries(m_entries, false);
}

std::vector<Entry> PasswordManager::searchEntries(const std::string& website) const {
    if (website.empty()) {
        return m_entries;
    }

    std::vector<Entry> entries;
    std::string query = toLower(website);
    for (const auto& entry: m_entries) {
        std::string site = toLower(entry.website);
        if (site.find(query) != std::string::npos) {
            entries.push_back(entry);
        }
    }

    return entries;
}

bool PasswordManager::deleteEntry(int id, std::string& error) {
    auto before = m_entries.size();
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(), [id] (const Entry& entry) { return entry.id == id; }), m_entries.end());

    if (m_entries.size() == before) {
        error = "Not found";
        return false;
    }

    return save(error);
}

bool PasswordManager::getEntryById(int id, Entry& out) const
{
    auto it = std::find_if(m_entries.begin(), m_entries.end(), [id] (const Entry& entry) { return entry.id == id; });
    if (it == m_entries.end()) {
        return false;
    }

    out = *it;
    return true;
}


void PasswordManager::displayEntries(const std::vector<Entry>& entries) const {
    displayEntries(entries, false);
}

void PasswordManager::displayEntries(const std::vector<Entry>& entries, bool showPassword) const {
    if (entries.empty()) {
        UI::infoStatus("No entries found. Use 'add' to create one.");
        return;
    }

    for (const auto& entry: entries) {
        displayEntry(entry, showPassword);
    }

    UI::separator();
    if (entries.size() == 1) {
        UI::successStatus("1 entry found");
    } else {
        UI::successStatus(std::to_string(entries.size()) + " entries found");
    }
}

void PasswordManager::displayEntry(const Entry& entry, bool showPassword) const
{
    auto maskPassword = [] (const std::string& password) {
        size_t len = password.size();
        if (len == 0) {
            return std::string("(empty)");
        }

        return std::string(len, '*');
    };

    UI::separator();
    UI::line("Id      : " + std::to_string(entry.id));
    UI::line("Website : " + entry.website);
    UI::line("Account : " + entry.username);
    UI::line("Password: " + (showPassword ? entry.password : maskPassword(entry.password)));
    UI::line("Note    : " + entry.note);
    UI::separator();
}

void PasswordManager::load()
{
    std::string error;
    if (!load(error) && !error.empty()) {
        std::cerr << error << "\n";
    }
}

bool PasswordManager::load(std::string& error)
{
    error.clear();
    m_entries.clear();

    std::string content;
    if (!Storage::readFile("data/passwords.txt", content, error)) {
        entry_id = 1;
        return true;
    }

    std::string plaintext;
    if (!m_auth.decryptContent(content, plaintext, error)) {
        entry_id = 1;
        return false;
    }

    std::istringstream input(plaintext);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            try {
                m_entries.push_back(Entry::deserialize(line));
            } catch (...) {
                error = "Corrupted entry";
                m_entries.clear();
                entry_id = 1;
                return false;
            }
        }
    }

    int maxId = 0;
    for (const auto& e : m_entries) {
        maxId = std::max(maxId, e.id);
    }

    entry_id = maxId + 1;
    return true;
}

bool PasswordManager::save(std::string& error)
{
    std::ostringstream plaintext;
    for (const auto& e : m_entries) {
        plaintext << e.serialize() << '\n';
    }

    std::string content;
    if (!m_auth.encryptContent(plaintext.str(), content, error)) {
        return false;
    }

    if (!Storage::writeFile("data/passwords.txt", content, error)) {
        return false;
    }

    return true;
}

