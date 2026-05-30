#pragma once
#include <string>
#include <filesystem>

class Storage {
    public:
    static bool getAppDataDirectory(std::filesystem::path& out, std::string& error);
    static bool getVaultPath(std::filesystem::path& out, std::string& error);
    static bool createDataDirectories(std::string& error);
    static bool readFile(const std::string& path, std::string& out, std::string& error);
    static bool writeFile(const std::string& path, const std::string& content, std::string& error);
};