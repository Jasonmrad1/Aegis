#include <auth_manager.hpp>
#include <crypto_service.hpp>
#include <sstream>
#include <ui.hpp>
#include <iostream>
#include <windows.h>

namespace {
    void secureZeroString(std::string& text)
    {
        if (!text.empty()) {
            SecureZeroMemory(&text[0], text.size());
            text.clear();
        }
    }

    void secureZeroVector(std::vector<unsigned char>& data)
    {
        if (!data.empty()) {
            SecureZeroMemory(data.data(), data.size());
            data.clear();
        }
    }

    bool constantTimeEquals(const std::vector<unsigned char>& left, const std::vector<unsigned char>& right)
    {
        if (left.size() != right.size()) {
            return false;
        }

        unsigned char diff = 0;
        for (size_t i = 0; i < left.size(); ++i) {
            diff |= static_cast<unsigned char>(left[i] ^ right[i]);
        }

        return diff == 0;
    }
}

static std::string bytesToHex(const std::vector<unsigned char>& data)
{
    static const char* kHex = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);

    for (unsigned char b : data) {
        out.push_back(kHex[(b >> 4) & 0x0F]);
        out.push_back(kHex[b & 0x0F]);
    }

    return out;
}

static bool hexToBytes(const std::string& hex, std::vector<unsigned char>& out)
{
    if (hex.size() % 2 != 0) {
        return false;
    }

    out.clear();
    out.reserve(hex.size() / 2);

    auto fromHex = [] (char c, unsigned char& v) {
        if (c >= '0' && c <= '9') { v = static_cast<unsigned char>(c - '0'); return true; }
        if (c >= 'a' && c <= 'f') { v = static_cast<unsigned char>(c - 'a' + 10); return true; }
        if (c >= 'A' && c <= 'F') { v = static_cast<unsigned char>(c - 'A' + 10); return true; }
        return false;
    };

    for (size_t i = 0; i < hex.size(); i += 2) {
        unsigned char hi = 0;
        unsigned char lo = 0;
        if (!fromHex(hex[i], hi) || !fromHex(hex[i + 1], lo)) {
            return false;
        }
        out.push_back(static_cast<unsigned char>((hi << 4) | lo));
    }

    return true;
}

void AuthManager::setMasterPassword(const std::string& password)
{
    secureZeroString(m_masterPassword);
    m_masterPassword = password;
    clearKey();
}

void AuthManager::clearMasterPassword()
{
    secureZeroString(m_masterPassword);
}

bool AuthManager::verifyPassword(const std::string& password) const
{
    if (!m_key.empty() && !m_salt.empty()) {
        std::vector<unsigned char> candidate;
        if (!CryptoService::deriveKey(password, m_salt, candidate)) {
            return false;
        }

        bool matches = constantTimeEquals(candidate, m_key);
        secureZeroVector(candidate);
        return matches;
    }

    return m_masterPassword == password;
}

std::vector<unsigned char> AuthManager::getSalt() const
{
    return m_salt;
}

void AuthManager::setSalt(const std::vector<unsigned char>& salt)
{
    m_salt = salt;
}

void AuthManager::clearSalt()
{
    m_salt.clear();
}

std::vector<unsigned char> AuthManager::getKey() const
{
    return m_key;
}

void AuthManager::setKey(const std::vector<unsigned char>& key)
{
    clearKey();
    m_key = key;
}

void AuthManager::clearKey()
{
    secureZeroVector(m_key);
}

void AuthManager::promptForMasterPassword()
{
    std::string master = UI::readHiddenLine("Enter master password: ");
    setMasterPassword(master);
    secureZeroString(master);
}

bool AuthManager::decryptContent(const std::string& content, std::string& plaintext, std::string& error)
{
    plaintext.clear();
    error.clear();

    if (content.empty()) {
        return true;
    }

    std::istringstream input(content);
    std::string line;
    if (!std::getline(input, line)) {
        return true;
    }

    if (line != "PM1") {
        error = "Unsupported or unencrypted vault format";
        return false;
    }

    std::string saltHex;
    std::string nonceHex;
    std::string tagHex;
    std::string cipherHex;

    if (!std::getline(input, saltHex) ||
        !std::getline(input, nonceHex) ||
        !std::getline(input, tagHex) ||
        !std::getline(input, cipherHex)) {
        error = "Corrupted file";
        return false;
    }

    if (!hexToBytes(saltHex, m_salt)) {
        error = "Invalid salt";
        return false;
    }

    if (m_key.empty()) {
        if (m_masterPassword.empty()) {
            error = "Master password not set";
            return false;
        }

        std::vector<unsigned char> key;
        if (!CryptoService::deriveKey(m_masterPassword, m_salt, key)) {
            error = "Failed to derive key";
            return false;
        }
        setKey(key);
        secureZeroVector(key);
        clearMasterPassword();
    }

    std::vector<unsigned char> nonce;
    std::vector<unsigned char> tag;
    std::vector<unsigned char> cipher;
    if (!hexToBytes(nonceHex, nonce) || !hexToBytes(tagHex, tag) || !hexToBytes(cipherHex, cipher)) {
        error = "Invalid encrypted data";
        return false;
    }

    if (!CryptoService::decryptAesGcm(m_key, nonce, tag, cipher, plaintext)) {
        error = "Failed to decrypt (wrong password?)";
        return false;
    }

    return true;
}

bool AuthManager::encryptContent(const std::string& plaintext, std::string& content, std::string& error)
{
    content.clear();
    error.clear();

    if (m_key.empty()) {
        if (m_masterPassword.empty()) {
            error = "Master password not set";
            return false;
        }

        if (m_salt.empty()) {
            if (!CryptoService::randomBytes(16, m_salt)) {
                error = "Failed to generate salt";
                return false;
            }
        }

        std::vector<unsigned char> key;
        if (!CryptoService::deriveKey(m_masterPassword, m_salt, key)) {
            error = "Failed to derive key";
            return false;
        }
        setKey(key);
        secureZeroVector(key);
        clearMasterPassword();
    }

    std::vector<unsigned char> nonce;
    std::vector<unsigned char> tag;
    std::vector<unsigned char> cipher;
    if (!CryptoService::encryptAesGcm(m_key, plaintext, nonce, tag, cipher)) {
        error = "Failed to encrypt";
        return false;
    }

    std::ostringstream out;
    out << "PM1\n";
    out << bytesToHex(m_salt) << "\n";
    out << bytesToHex(nonce) << "\n";
    out << bytesToHex(tag) << "\n";
    out << bytesToHex(cipher) << "\n";

    content = out.str();
    return true;
}
