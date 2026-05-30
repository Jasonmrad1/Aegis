#pragma once
#include <cstddef>
#include <string>
#include <vector>

class CryptoService {
    public:
    static bool randomBytes(size_t size, std::vector<unsigned char>& out);
    static bool deriveKey(const std::string& password, const std::vector<unsigned char>& salt, std::vector<unsigned char>& outKey);
    static bool encryptAesGcm(const std::vector<unsigned char>& key,
                              const std::string& plaintext,
                              std::vector<unsigned char>& nonce,
                              std::vector<unsigned char>& tag,
                              std::vector<unsigned char>& cipher);
    static bool decryptAesGcm(const std::vector<unsigned char>& key,
                              const std::vector<unsigned char>& nonce,
                              const std::vector<unsigned char>& tag,
                              const std::vector<unsigned char>& cipher,
                              std::string& plaintext);
};
