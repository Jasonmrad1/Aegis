#include <storage.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <windows.h>
#include <shlobj.h>

namespace {
    std::filesystem::path resolvePath(const std::string& path, std::string& error)
    {
        std::filesystem::path filePath(path);
        if (filePath.is_absolute()) {
            return filePath;
        }

        std::filesystem::path base;
        if (!Storage::getAppDataDirectory(base, error)) {
            return {};
        }

        return base / filePath;
    }
}

bool Storage::getAppDataDirectory(std::filesystem::path& out, std::string& error)
{
    error.clear();
    PWSTR appDataPath = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &appDataPath);
    if (FAILED(hr) || !appDataPath) {
        error = "AppData is unavailable";
        return false;
    }

    out = std::filesystem::path(appDataPath) / "Aegis";
    CoTaskMemFree(appDataPath);
    return true;
}

bool Storage::getVaultPath(std::filesystem::path& out, std::string& error)
{
    std::filesystem::path base;
    if (!getAppDataDirectory(base, error)) {
        return false;
    }

    out = base / "data" / "passwords.txt";
    return true;
}

bool Storage::createDataDirectories(std::string& error)
{
    error.clear();
    std::filesystem::path base;
    if (!getAppDataDirectory(base, error)) {
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(base / "data", ec);
    if (ec) {
        error = "Failed to create AppData data directory";
        return false;
    }

    std::filesystem::create_directories(base / "config", ec);
    if (ec) {
        error = "Failed to create AppData config directory";
        return false;
    }

    std::filesystem::create_directories(base / "backups", ec);
    if (ec) {
        error = "Failed to create AppData backups directory";
        return false;
    }

    return true;
}

bool Storage::readFile(const std::string& path, std::string& out, std::string& error)
{
    error.clear();
    std::filesystem::path filePath = resolvePath(path, error);
    if (filePath.empty()) {
        return false;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    out = ss.str();
    return true;
}

bool Storage::writeFile(const std::string& path, const std::string& content, std::string& error)
{
    error.clear();
    std::filesystem::path filePath = resolvePath(path, error);
    if (filePath.empty()) {
        return false;
    }

    if (!createDataDirectories(error)) {
        return false;
    }

    std::filesystem::path dir = filePath.parent_path();
    if (!dir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec) {
            error = "Failed to create data directory";
            return false;
        }
    }

    std::filesystem::path tempPath = filePath;
    tempPath += ".tmp";

    std::ofstream file(tempPath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        error = "Failed to open temp file";
        return false;
    }

    file << content;
    file.close();

    std::error_code ec;
    std::filesystem::rename(tempPath, filePath, ec);
    if (ec) {
        std::filesystem::remove(filePath, ec);
        ec.clear();
        std::filesystem::rename(tempPath, filePath, ec);
    }

    if (ec) {
        std::filesystem::remove(tempPath, ec);
        error = "Failed to save vault";
        return false;
    }

    return true;
}