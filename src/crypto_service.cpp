#include <crypto_service.hpp>
#include <windows.h>
#include <bcrypt.h>

bool CryptoService::randomBytes(size_t size, std::vector<unsigned char>& out)
{
    out.resize(size);
    NTSTATUS status = BCryptGenRandom(nullptr, out.data(), static_cast<ULONG>(out.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return status == 0;
}

bool CryptoService::deriveKey(const std::string& password, const std::vector<unsigned char>& salt, std::vector<unsigned char>& outKey)
{
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (status != 0) {
        return false;
    }

    const ULONG iterations = 100000;
    outKey.resize(32);
    status = BCryptDeriveKeyPBKDF2(
        hAlg,
        reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())),
        static_cast<ULONG>(password.size()),
        const_cast<PUCHAR>(salt.data()),
        static_cast<ULONG>(salt.size()),
        iterations,
        outKey.data(),
        static_cast<ULONG>(outKey.size()),
        0
    );

    BCryptCloseAlgorithmProvider(hAlg, 0);
    return status == 0;
}

bool CryptoService::encryptAesGcm(const std::vector<unsigned char>& key,
                                 const std::string& plaintext,
                                 std::vector<unsigned char>& nonce,
                                 std::vector<unsigned char>& tag,
                                 std::vector<unsigned char>& cipher)
{
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_KEY_HANDLE hKey = nullptr;
    PUCHAR keyObject = nullptr;
    ULONG keyObjectSize = 0;
    ULONG dataSize = 0;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    if (status != 0) {
        return false;
    }

    status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_GCM)), sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    if (status != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&keyObjectSize), sizeof(ULONG), &dataSize, 0);
    if (status != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    keyObject = static_cast<PUCHAR>(HeapAlloc(GetProcessHeap(), 0, keyObjectSize));
    if (!keyObject) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    status = BCryptGenerateSymmetricKey(hAlg, &hKey, keyObject, keyObjectSize, const_cast<PUCHAR>(key.data()), static_cast<ULONG>(key.size()), 0);
    if (status != 0) {
        HeapFree(GetProcessHeap(), 0, keyObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    if (!CryptoService::randomBytes(12, nonce)) {
        BCryptDestroyKey(hKey);
        HeapFree(GetProcessHeap(), 0, keyObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    tag.resize(16);
    authInfo.pbNonce = nonce.data();
    authInfo.cbNonce = static_cast<ULONG>(nonce.size());
    authInfo.pbTag = tag.data();
    authInfo.cbTag = static_cast<ULONG>(tag.size());

    ULONG cipherSize = 0;
    status = BCryptEncrypt(
        hKey,
        reinterpret_cast<PUCHAR>(const_cast<char*>(plaintext.data())),
        static_cast<ULONG>(plaintext.size()),
        &authInfo,
        nullptr,
        0,
        nullptr,
        0,
        &cipherSize,
        0
    );

    if (status != 0) {
        BCryptDestroyKey(hKey);
        HeapFree(GetProcessHeap(), 0, keyObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    cipher.resize(cipherSize);
    status = BCryptEncrypt(
        hKey,
        reinterpret_cast<PUCHAR>(const_cast<char*>(plaintext.data())),
        static_cast<ULONG>(plaintext.size()),
        &authInfo,
        nullptr,
        0,
        cipher.data(),
        static_cast<ULONG>(cipher.size()),
        &cipherSize,
        0
    );

    if (status != 0) {
        BCryptDestroyKey(hKey);
        HeapFree(GetProcessHeap(), 0, keyObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    cipher.resize(cipherSize);
    BCryptDestroyKey(hKey);
    HeapFree(GetProcessHeap(), 0, keyObject);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return true;
}

bool CryptoService::decryptAesGcm(const std::vector<unsigned char>& key,
                                 const std::vector<unsigned char>& nonce,
                                 const std::vector<unsigned char>& tag,
                                 const std::vector<unsigned char>& cipher,
                                 std::string& plaintext)
{
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_KEY_HANDLE hKey = nullptr;
    PUCHAR keyObject = nullptr;
    ULONG keyObjectSize = 0;
    ULONG dataSize = 0;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    if (status != 0) {
        return false;
    }

    status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_GCM)), sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    if (status != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&keyObjectSize), sizeof(ULONG), &dataSize, 0);
    if (status != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    keyObject = static_cast<PUCHAR>(HeapAlloc(GetProcessHeap(), 0, keyObjectSize));
    if (!keyObject) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    status = BCryptGenerateSymmetricKey(hAlg, &hKey, keyObject, keyObjectSize, const_cast<PUCHAR>(key.data()), static_cast<ULONG>(key.size()), 0);
    if (status != 0) {
        HeapFree(GetProcessHeap(), 0, keyObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = const_cast<PUCHAR>(nonce.data());
    authInfo.cbNonce = static_cast<ULONG>(nonce.size());
    authInfo.pbTag = const_cast<PUCHAR>(tag.data());
    authInfo.cbTag = static_cast<ULONG>(tag.size());

    ULONG plainSize = 0;
    status = BCryptDecrypt(
        hKey,
        const_cast<PUCHAR>(cipher.data()),
        static_cast<ULONG>(cipher.size()),
        &authInfo,
        nullptr,
        0,
        nullptr,
        0,
        &plainSize,
        0
    );

    if (status != 0) {
        BCryptDestroyKey(hKey);
        HeapFree(GetProcessHeap(), 0, keyObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    if (plainSize == 0) {
        plaintext.clear();
        BCryptDestroyKey(hKey);
        HeapFree(GetProcessHeap(), 0, keyObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return true;
    }

    plaintext.resize(plainSize);
    status = BCryptDecrypt(
        hKey,
        const_cast<PUCHAR>(cipher.data()),
        static_cast<ULONG>(cipher.size()),
        &authInfo,
        nullptr,
        0,
        reinterpret_cast<PUCHAR>(&plaintext[0]),
        static_cast<ULONG>(plaintext.size()),
        &plainSize,
        0
    );

    if (status != 0) {
        BCryptDestroyKey(hKey);
        HeapFree(GetProcessHeap(), 0, keyObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    plaintext.resize(plainSize);
    BCryptDestroyKey(hKey);
    HeapFree(GetProcessHeap(), 0, keyObject);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return true;
}
