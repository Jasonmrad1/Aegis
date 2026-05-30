#pragma once
#include <string>

namespace UI {
    void init();
    void line(const std::string& text);
    void write(const std::string& text);
    void heading(const std::string& text);
    void prompt();
    std::string readHiddenLine(const std::string& prompt);
    void clearScreen();
    void info(const std::string& text);
    void success(const std::string& text);
    void warn(const std::string& text);
    void error(const std::string& text);
    void separator();
    void infoStatus(const std::string& text);
    void successStatus(const std::string& text);
    void warnStatus(const std::string& text);
    void errorStatus(const std::string& text);
}