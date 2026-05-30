#pragma once
#include <string>
#include <vector>

class AuthManager {
    public:
    void setMasterPassword(const std::string& password);
    void clearMasterPassword();
    void promptForMasterPassword();
    bool verifyPassword(const std::string& password) const;
    std::vector<unsigned char> getSalt() const;
    void setSalt(const std::vector<unsigned char>& salt);
    void clearSalt();
    std::vector<unsigned char> getKey() const;
    void setKey(const std::vector<unsigned char>& key);
    void clearKey();
    bool decryptContent(const std::string& content, std::string& plaintext, std::string& error);
    bool encryptContent(const std::string& plaintext, std::string& content, std::string& error);

    private:
    std::string m_masterPassword;
    std::vector<unsigned char> m_salt;
    std::vector<unsigned char> m_key;
};
