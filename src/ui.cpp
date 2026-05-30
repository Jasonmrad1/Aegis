#include <ui.hpp>
#include <iostream>
#include <cstdlib>
#include <windows.h>
#include <conio.h>

namespace {
    bool g_colorEnabled = false;

    std::string wrap(const char* code, const std::string& text)
    {
        if (!g_colorEnabled) {
            return text;
        }

        return std::string("\x1b[") + code + "m" + text + "\x1b[0m";
    }
}

void UI::init()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        g_colorEnabled = false;
        return;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) {
        g_colorEnabled = false;
        return;
    }

    if (!SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        g_colorEnabled = false;
        return;
    }

    g_colorEnabled = true;
}

void UI::line(const std::string& text)
{
    std::cout << text << std::endl;
}

void UI::write(const std::string& text)
{
    std::cout << text;
}

void UI::heading(const std::string& text)
{
    line(wrap("1;36", text));
}

void UI::prompt()
{
    write(wrap("90", "vault> "));
}

std::string UI::readHiddenLine(const std::string& prompt)
{
    write(prompt);

    std::string input;
    while (true) {
        int ch = _getch();
        if (ch == '\r') {
            break;
        }

        if (ch == '\b') {
            if (!input.empty()) {
                input.pop_back();
                write("\b \b");
            }
            continue;
        }

        if (ch == 0 || ch == 224) {
            _getch();
            continue;
        }

        input.push_back(static_cast<char>(ch));
        write("*");
    }

    line("");
    return input;
}

void UI::clearScreen()
{
    std::system("cls");
}

void UI::info(const std::string& text)
{
    line(wrap("90", text));
}

void UI::success(const std::string& text)
{
    line(wrap("32", text));
}

void UI::warn(const std::string& text)
{
    line(wrap("33", text));
}

void UI::error(const std::string& text)
{
    line(wrap("31", text));
}

void UI::separator()
{
    line(wrap("90", "------------------------------"));
}

void UI::infoStatus(const std::string& text)
{
    line(wrap("90", "[i]") + " " + text);
}

void UI::successStatus(const std::string& text)
{
    line(wrap("32", "[+]") + " " + text);
}

void UI::warnStatus(const std::string& text)
{
    line(wrap("33", "[!]") + " " + text);
}

void UI::errorStatus(const std::string& text)
{
    line(wrap("31", "[x]") + " " + text);
}
