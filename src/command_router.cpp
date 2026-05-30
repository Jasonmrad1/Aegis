#include <command_router.hpp>
#include <clipboard.hpp>
#include <crypto_service.hpp>
#include <sstream>
#include <vector>
#include <ui.hpp>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <windows.h>
enum Command {
    HELP,
    VERSION,
    LIST,
    ADD,
    EDIT,
    SHOW,
    DELETE_ENTRY,
    PASSWD,
    LOCK,
    CLEAR,
    CLEAR_ALL,
    GEN,
    COPY,
    EXIT,
    UNKNOWN
};

namespace {
    const char* kAppName = "Aegis";
    const char* kAppVersion = "1.0.0";
}

std::vector<std::string> split(const std::string& line)
{
    std::stringstream ss(line);
    std::string word;
    std::vector<std::string> tokens;

    while(ss >> word) {
        tokens.push_back(word);
    }

    return tokens;
}

Command parseCommand(const std::string& line)
{
    auto tokens = split(line);

    if(tokens.empty()) return UNKNOWN;

    if(tokens[0] == "-h" || tokens[0] == "--help") return HELP;
    if(tokens[0] == "help") return HELP;
    if(tokens[0] == "-v" || tokens[0] == "--version" || tokens[0] == "version") return VERSION;
    if(tokens[0] == "list") return LIST;
    if(tokens[0] == "add") return ADD;
    if(tokens[0] == "edit") return EDIT;
    if(tokens[0] == "show") return SHOW;
    if(tokens[0] == "search") return SHOW;
    if(tokens[0] == "delete") return DELETE_ENTRY;
    if(tokens[0] == "passwd") return PASSWD;
    if(tokens[0] == "lock") return LOCK;
    if(tokens[0] == "clear") return CLEAR;
    if(tokens[0] == "clear-all") return CLEAR_ALL;
    if(tokens[0] == "gen") return GEN;
    if(tokens[0] == "copy") return COPY;
    if(tokens[0] == "exit") return EXIT;

    return UNKNOWN;
}

CommandRouter::CommandRouter(PasswordManager& m) : manager(m) {}

bool tryParseInt(const std::string& text, int& out)
{
    if (text.empty()) {
        return false;
    }

    std::istringstream iss(text);
    iss >> out;
    return !iss.fail() && iss.eof();
}

bool getFlagValue(const std::vector<std::string>& tokens, const std::string& flag, std::string& out)
{
    for (size_t i = 1; i + 1 < tokens.size(); ++i) {
        if (tokens[i] == flag) {
            out = tokens[i + 1];
            return true;
        }
    }

    return false;
}

static bool isFlagToken(const std::string& token)
{
    return !token.empty() && token[0] == '-';
}

static std::string findMissingFlagValue(const std::vector<std::string>& tokens, const std::vector<std::string>& flags)
{
    for (const auto& flag : flags) {
        for (size_t i = 1; i < tokens.size(); ++i) {
            if (tokens[i] == flag) {
                if (i + 1 >= tokens.size() || isFlagToken(tokens[i + 1])) {
                    return flag;
                }
            }
        }
    }

    return "";
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

static bool containsIgnoreCase(const std::string& text, const std::string& needle)
{
    if (needle.empty()) {
        return true;
    }

    std::string textLower = toLower(text);
    std::string needleLower = toLower(needle);
    return textLower.find(needleLower) != std::string::npos;
}

static bool generatePassword(size_t length, bool useSymbols, std::string& out)
{
    const std::string letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const std::string digits = "0123456789";
    const std::string symbols = "!@#$%^&*()-_=+[]{};:,.?";
    std::string charset = letters + digits + (useSymbols ? symbols : "");

    if (charset.empty() || length == 0) {
        return false;
    }

    std::vector<unsigned char> rnd;
    if (!CryptoService::randomBytes(length, rnd)) {
        return false;
    }

    out.clear();
    out.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        out.push_back(charset[rnd[i] % charset.size()]);
    }

    return true;
}

static void secureZeroString(std::string& text)
{
    if (!text.empty()) {
        SecureZeroMemory(&text[0], text.size());
        text.clear();
    }
}

bool CommandRouter::handle(const std::string& line)
{
    auto tokens = split(line);

    switch(parseCommand(line))
    {
        case HELP:
        {
            bool showExamples = false;
            bool showAll = false;
            std::string topic;
            for (size_t i = 1; i < tokens.size(); ++i) {
                std::string tokenLower = toLower(tokens[i]);
                if (tokenLower == "-e" || tokenLower == "--examples") {
                    showExamples = true;
                } else if (tokenLower == "all" || tokenLower == "--all") {
                    showAll = true;
                } else if (topic.empty()) {
                    topic = tokenLower;
                }
            }

            if (!topic.empty() && topic != "all") {
                if (topic == "gen") {
                    UI::heading("[ gen ]");
                    UI::line("  gen [-l <len>] [--no-symbols]");
                    UI::line("  Generates a password and prints it to the screen.");
                    UI::line("  -l <len>       Set length (default 20)");
                    UI::line("  --no-symbols   Letters + digits only");
                    UI::line("  Tip: use 'copy <id>' after saving an entry.");
                    UI::line("");
                    UI::heading("[ Examples ]");
                    UI::line("  gen");
                    UI::line("  gen -l 32");
                    UI::line("  gen -l 16 --no-symbols");
                    return true;
                }

                if (topic == "show") {
                    UI::heading("[ show ]");
                    UI::line("  show <id>");
                    UI::line("  show -i <id>");
                    UI::line("  show [-a <account>] [-w <website>] [-n <note>]");
                    UI::line("  You can combine filters: show -w site -a user");
                    UI::line("");
                    UI::heading("[ Examples ]");
                    UI::line("  show 2");
                    UI::line("  show -w site");
                    UI::line("  show -w site -a user");
                    UI::line("  show -n note");
                    return true;
                }

                if (topic == "add") {
                    UI::heading("[ add ]");
                    UI::line("  add <website> <account> <pass> <note>");
                    UI::line("  add -w <website> -a <account> -p <pass> [-n <note>]");
                    return true;
                }

                if (topic == "edit") {
                    UI::heading("[ edit ]");
                    UI::line("  edit <id> <website> <account> <pass> <note>");
                    UI::line("  edit <id> -w <website> -a <account> -p <pass> [-n <note>]");
                    return true;
                }

                if (topic == "list") {
                    UI::heading("[ list ]");
                    UI::line("  list [-p]");
                    UI::line("  -p shows passwords in the list");
                    return true;
                }

                if (topic == "delete") {
                    UI::heading("[ delete ]");
                    UI::line("  delete <id>");
                    return true;
                }

                if (topic == "copy") {
                    UI::heading("[ copy ]");
                    UI::line("  copy <id>");
                    UI::line("  Copies the entry password to clipboard.");
                    return true;
                }

                if (topic == "passwd") {
                    UI::heading("[ passwd ]");
                    UI::line("  passwd");
                    UI::line("  Changes the master password (current + new twice).");
                    return true;
                }

                if (topic == "lock") {
                    UI::heading("[ lock ]");
                    UI::line("  lock");
                    UI::line("  Locks the app and requires master password to continue.");
                    return true;
                }

                if (topic == "clear") {
                    UI::heading("[ clear ]");
                    UI::line("  clear");
                    UI::line("  Clears the screen only.");
                    return true;
                }

                if (topic == "clear-all") {
                    UI::heading("[ clear-all ]");
                    UI::line("  clear-all");
                    UI::line("  Deletes all entries after confirmation.");
                    return true;
                }

                if (topic == "exit") {
                    UI::heading("[ exit ]");
                    UI::line("  exit");
                    UI::line("  Exits the app.");
                    return true;
                }

                UI::warn("Unknown help topic. Try: help all");
                return true;
            }

            auto printHelp = [] (const std::string& command, const std::string& description) {
                const int columnWidth = 62;
                std::ostringstream oss;
                if (static_cast<int>(command.size()) >= columnWidth) {
                    oss << "  " << command << "  " << description;
                } else {
                    oss << "  " << std::left << std::setw(columnWidth) << command << description;
                }
                UI::line(oss.str());
            };

            if (!showAll) {

                UI::heading("[ Core ]");
                printHelp("add <website> <account> <pass> <note>", "Add a new entry (positional)");
                printHelp("add -w <website> -a <account> -p <pass> [-n <note>]", "Add a new entry (flags)");
                printHelp("edit <id> <website> <account> <pass> <note>", "Update an entry by id");
                printHelp("edit <id> -w/-a/-p/-n <value>", "Update specific fields by id");
                printHelp("list [-p]", "List entries; -p shows passwords");
                printHelp("show <id>", "Show entry by id");
                printHelp("show -i <id> | -w/-a/-n <value>", "Show by id or search (flags can be combined)");
                printHelp("delete <id>", "Delete entry by id");

                UI::line("");
                UI::heading("[ Search ]");
                printHelp("show [-a <account>] [-w <website>] [-n <note>]", "Search entries (flags can be combined)");

                UI::line("");
                UI::heading("[ Advanced ]");
                printHelp("gen [-l <len>] [--no-symbols]", "Generate a password (prints to screen)");
                printHelp("copy <id>", "Copy entry password to clipboard");
                printHelp("passwd", "Change master password");
                printHelp("lock", "Lock the app");
                printHelp("clear", "Clear the screen only");
                printHelp("clear-all", "Delete all entries");
                printHelp("--version", "Print version information");
                printHelp("exit", "Exit the app");

                UI::info("Tip: help all | help <command> | help -e");
                if (!showExamples) {
                    return true;
                }
            }

            UI::heading("[ Full Reference ]");
            printHelp("add <website> <account> <pass> <note>", "Add a new entry (positional)");
            printHelp("add -w <website> -a <account> -p <pass> [-n <note>]", "Add a new entry (flags)");
            printHelp("edit <id> <website> <account> <pass> <note>", "Update an entry by id (positional)");
            printHelp("edit <id> -w <website> -a <account> -p <pass> [-n <note>]", "Update by id (flags)");
            printHelp("list [-p]", "List all entries; add -p to show passwords");
            printHelp("show <id>", "Show a single entry by id");
            printHelp("show -i <id>", "Show a single entry by id (flag form)");
            printHelp("show [-a <account>] [-w <website>] [-n <note>]", "Filter entries (flags can be combined)");
            printHelp("delete <id>", "Delete an entry by id");
            printHelp("copy <id>", "Copy an entry password to clipboard");
            printHelp("gen [-l <len>] [--no-symbols]", "Generate a password (prints to screen)");
            printHelp("Use -l to set length (default 20), --no-symbols to exclude symbols", "");
            printHelp("passwd", "Change master password (asks current + new twice)");
            printHelp("lock", "Lock the app (re-enter master password to continue)");
            printHelp("clear", "Clear the screen only");
            printHelp("clear-all", "Delete all entries (requires YES)");
            printHelp("--version", "Print version information");
            printHelp("exit", "Exit the app");

            if (showExamples) {
                UI::line("");
                UI::heading("[ Examples ]");
                UI::line("  add site user@example.com pass123 note");
                UI::line("  add -w site -a user@example.com -p pass123 -n note");
                UI::line("  edit 2 -w site -a user@example.com -p pass456 -n updated");
                UI::line("  show 2");
                UI::line("  show -i 2");
                UI::line("  show -a user@example.com");
                UI::line("  show -w site");
                UI::line("  show -w site -a user");
                UI::line("  list -p");
                UI::line("  copy 2");
                UI::line("  gen");
                UI::line("  gen -l 32");
                UI::line("  gen -l 16 --no-symbols");
            }
            return true;
        }

        case VERSION:
            UI::line(std::string(kAppName) + " v" + kAppVersion);
            return true;

        case LIST:
        {
            bool showPassword = false;
            for (const auto& token : tokens) {
                if (token == "-p" || token == "--password" || token == "--show") {
                    showPassword = true;
                    break;
                }
            }

            manager.displayEntries(manager.searchEntries(""), showPassword);
            return true;
        }

        case ADD:
        {
            std::string website, user, pass, note;
            bool hasFlag = false;

            std::string missing = findMissingFlagValue(tokens, {"-w", "-a", "-p", "-n"});
            if (!missing.empty()) {
                UI::warn("Missing value for " + missing);
                return true;
            }

            hasFlag = getFlagValue(tokens, "-w", website) || hasFlag;
            hasFlag = getFlagValue(tokens, "-a", user) || hasFlag;
            hasFlag = getFlagValue(tokens, "-p", pass) || hasFlag;
            hasFlag = getFlagValue(tokens, "-n", note) || hasFlag;

            if (hasFlag)
            {
                if (website.empty() || user.empty() || pass.empty())
                {
                    UI::warn("Usage: add -w <website> -a <account> -p <pass> [-n <note>]");
                    return true;
                }

                std::string error;
                if (manager.addEntry(website, user, pass, note, error)) {
                    UI::success("Added.");
                } else {
                    UI::error(error.empty() ? "Failed to add" : error);
                }
                return true;
            }

            if(tokens.size() >= 5)
            {
                std::string error;
                if (manager.addEntry(tokens[1], tokens[2], tokens[3], tokens[4], error)) {
                    UI::success("Added.");
                } else {
                    UI::error(error.empty() ? "Failed to add" : error);
                }
                return true;
            }

            if (tokens.size() >= 2) {
                website = tokens[1];
            }
            if (tokens.size() >= 3) {
                user = tokens[2];
            }
            if (tokens.size() >= 4) {
                pass = tokens[3];
            }

            if (website.empty()) {
                UI::write("Website: ");
                std::getline(std::cin >> std::ws, website);
            }

            if (user.empty()) {
                UI::write("Account: ");
                std::getline(std::cin >> std::ws, user);
            }

            if (pass.empty()) {
                UI::write("Password: ");
                std::getline(std::cin >> std::ws, pass);
            }

            {
                std::string error;
                if (manager.addEntry(website, user, pass, note, error)) {
                    UI::success("Added.");
                } else {
                    UI::error(error.empty() ? "Failed to add" : error);
                }
            }
            return true;
        }

        case EDIT:
        {
            if(tokens.size() < 2)
            {
                UI::warn("Usage: edit <id> <website> <account> <pass> <note>");
                return true;
            }

            int id = 0;
            if (!tryParseInt(tokens[1], id))
            {
                UI::warn("Invalid id");
                return true;
            }

            std::string website, user, pass, note;
            bool hasFlag = false;

            std::string missing = findMissingFlagValue(tokens, {"-w", "-a", "-p", "-n"});
            if (!missing.empty()) {
                UI::warn("Missing value for " + missing);
                return true;
            }

            hasFlag = getFlagValue(tokens, "-w", website) || hasFlag;
            hasFlag = getFlagValue(tokens, "-a", user) || hasFlag;
            hasFlag = getFlagValue(tokens, "-p", pass) || hasFlag;
            hasFlag = getFlagValue(tokens, "-n", note) || hasFlag;

            if (!hasFlag && tokens.size() >= 6) {
                website = tokens[2];
                user = tokens[3];
                pass = tokens[4];
                note = tokens[5];
            }

            if (hasFlag) {
                Entry current(0, "", "", "", "");
                if (!manager.getEntryById(id, current)) {
                    UI::warn("Not found");
                    return true;
                }

                if (website.empty()) {
                    website = current.website;
                }
                if (user.empty()) {
                    user = current.username;
                }
                if (pass.empty()) {
                    pass = current.password;
                }
                if (note.empty()) {
                    note = current.note;
                }
            }

            if (website.empty()) {
                UI::write("Website: ");
                std::getline(std::cin >> std::ws, website);
            }

            if (user.empty()) {
                UI::write("Account: ");
                std::getline(std::cin >> std::ws, user);
            }

            if (pass.empty()) {
                UI::write("Password: ");
                std::getline(std::cin >> std::ws, pass);
            }

            if (note.empty()) {
                UI::write("Note: ");
                std::getline(std::cin >> std::ws, note);
            }

            {
                std::string error;
                if (manager.updateEntry(id, website, user, pass, note, error)) {
                    UI::success("Updated.");
                } else {
                    UI::error(error.empty() ? "Failed to update" : error);
                }
            }
            return true;
        }

        case SHOW:
        {
            std::string website;
            std::string username;
            std::string note;
            std::string idText;

            std::string missing = findMissingFlagValue(tokens, {"-w", "-a", "-n", "-i"});
            if (!missing.empty()) {
                UI::warn("Missing value for " + missing);
                return true;
            }

            getFlagValue(tokens, "-w", website);
            getFlagValue(tokens, "-a", username);
            getFlagValue(tokens, "-n", note);
            getFlagValue(tokens, "-i", idText);

            bool isSearchAlias = !tokens.empty() && tokens[0] == "search";
            if (isSearchAlias && website.empty() && tokens.size() >= 2) {
                website = tokens[1];
            }

            if (idText.empty() && website.empty() && username.empty() && note.empty() && tokens.size() >= 2) {
                idText = tokens[1];
            }

            if (!idText.empty()) {
                int id = 0;
                if (!tryParseInt(idText, id)) {
                    UI::warn("Invalid id");
                    return true;
                }

                Entry entry(0, "", "", "", "");
                if (!manager.getEntryById(id, entry))
                {
                    UI::warn("Not found");
                    return true;
                }

                manager.displayEntry(entry, true);
                return true;
            }

            if (!username.empty() || !website.empty() || !note.empty()) {
                std::vector<Entry> entries;
                for (const auto& entry : manager.searchEntries("")) {
                    bool match = true;
                    if (!website.empty()) {
                        match = match && containsIgnoreCase(entry.website, website);
                    }
                    if (!username.empty()) {
                        match = match && containsIgnoreCase(entry.username, username);
                    }
                    if (!note.empty()) {
                        match = match && containsIgnoreCase(entry.note, note);
                    }
                    if (match) {
                        entries.push_back(entry);
                    }
                }

                manager.displayEntries(entries, true);
                return true;
            }

            UI::warn("Usage: show <id> | show -i <id> | show -a <account> | show -w <website> | show -n <note>");
            return true;
        }

        case DELETE_ENTRY:
        {
            if(tokens.size() < 2)
            {
                UI::warn("Usage: delete <id>");
                return true;
            }

            int id = 0;
            if (!tryParseInt(tokens[1], id))
            {
                UI::warn("Invalid id");
                return true;
            }

            {
                std::string error;
                if (manager.deleteEntry(id, error)) {
                    UI::success("Deleted.");
                } else {
                    UI::warn(error.empty() ? "Not found" : error);
                }
            }
            return true;
        }

        case COPY:
        {
            if(tokens.size() < 2)
            {
                UI::warn("Usage: copy <id>");
                return true;
            }

            int id = 0;
            if (!tryParseInt(tokens[1], id))
            {
                UI::warn("Invalid id");
                return true;
            }

            Entry entry(0, "", "", "", "");
            if (!manager.getEntryById(id, entry))
            {
                UI::warn("Not found");
                return true;
            }

            std::string error;
            if (Clipboard::copyText(entry.password, error)) {
                UI::successStatus("Password copied to clipboard");
            } else {
                UI::error(error.empty() ? "Failed to copy" : error);
            }
            return true;
        }

        case GEN:
        {
            size_t length = 20;
            bool useSymbols = true;

            std::string missing = findMissingFlagValue(tokens, {"-l"});
            if (!missing.empty()) {
                UI::warn("Missing value for " + missing);
                return true;
            }

            std::string lenText;
            if (getFlagValue(tokens, "-l", lenText)) {
                int len = 0;
                if (!tryParseInt(lenText, len) || len <= 0) {
                    UI::warn("Invalid length");
                    return true;
                }
                length = static_cast<size_t>(len);
            }

            for (const auto& token : tokens) {
                if (token == "--no-symbols") {
                    useSymbols = false;
                }
            }

            std::string password;
            if (!generatePassword(length, useSymbols, password)) {
                UI::error("Failed to generate password");
                return true;
            }

            UI::line(password);
            return true;
        }

        case PASSWD:
        {
            std::string current;
            std::string next;
            std::string confirm;

            current = UI::readHiddenLine("Current password: ");
            next = UI::readHiddenLine("New password: ");
            confirm = UI::readHiddenLine("Confirm new password: ");

            if (next != confirm) {
                secureZeroString(current);
                secureZeroString(next);
                secureZeroString(confirm);
                UI::warn("Passwords do not match");
                return true;
            }

            std::string error;
            if (manager.changeMasterPassword(current, next, error)) {
                UI::success("Master password updated.");
            } else {
                UI::error(error.empty() ? "Failed to change password" : error);
            }
            secureZeroString(current);
            secureZeroString(next);
            secureZeroString(confirm);
            return true;
        }

        case LOCK:
        {
            UI::infoStatus("Locked");
            while (true) {
                std::string password = UI::readHiddenLine("Re-enter master password to unlock: ");

                if (manager.verifyMasterPassword(password)) {
                    secureZeroString(password);
                    UI::successStatus("Unlocked");
                    break;
                }

                secureZeroString(password);
                UI::warn("Incorrect password");
            }
            return true;
        }

        case CLEAR:
            UI::clearScreen();
            return true;

        case CLEAR_ALL:
        {
            std::string confirm;
            UI::write("Type YES to confirm: ");
            std::getline(std::cin >> std::ws, confirm);

            if (confirm != "YES") {
                UI::infoStatus("Canceled");
                return true;
            }

            std::string error;
            if (manager.clearAll(error)) {
                UI::success("All entries deleted.");
            } else {
                UI::error(error.empty() ? "Failed to clear" : error);
            }
            return true;
        }

        case EXIT:
            return false;

        default:
            UI::warn("Unknown command. type help");
            return true;
    }
}