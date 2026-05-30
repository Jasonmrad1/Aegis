#include <auth_manager.hpp>
#include <crypto_service.hpp>
#include <entry.hpp>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

static void testEntryRoundTrip()
{
    Entry original(1, "site|name", "user\\name", "p@ss\nword", "note\rwith|pipe");
    std::string encoded = original.serialize();
    Entry decoded = Entry::deserialize(encoded);

    assert(decoded.id == original.id);
    assert(decoded.website == original.website);
    assert(decoded.username == original.username);
    assert(decoded.password == original.password);
    assert(decoded.note == original.note);
}

static void testCryptoRoundTrip()
{
    std::vector<unsigned char> salt;
    assert(CryptoService::randomBytes(16, salt));

    std::vector<unsigned char> key;
    assert(CryptoService::deriveKey("password", salt, key));

    std::string plaintext = "hello world";
    std::vector<unsigned char> nonce;
    std::vector<unsigned char> tag;
    std::vector<unsigned char> cipher;

    assert(CryptoService::encryptAesGcm(key, plaintext, nonce, tag, cipher));

    std::string decoded;
    assert(CryptoService::decryptAesGcm(key, nonce, tag, cipher, decoded));
    assert(decoded == plaintext);
}

static void testWrongPassword()
{
    AuthManager auth;
    auth.setMasterPassword("secret");

    std::string error;
    std::string content;
    assert(auth.encryptContent("data", content, error));

    AuthManager wrong;
    wrong.setMasterPassword("bad");
    std::string plaintext;
    bool ok = wrong.decryptContent(content, plaintext, error);
    assert(!ok);
}

static void testCorruptedVault()
{
    AuthManager auth;
    auth.setMasterPassword("secret");

    std::string plaintext;
    std::string error;
    bool ok = auth.decryptContent("PM1\nabc\n", plaintext, error);
    assert(!ok);
}

int main()
{
    testEntryRoundTrip();
    testCryptoRoundTrip();
    testWrongPassword();
    testCorruptedVault();

    std::cout << "All tests passed.\n";
    return 0;
}
