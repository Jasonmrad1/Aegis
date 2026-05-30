#include <iostream>
#include <string>
#include "password_manager.hpp"
#include "auth_manager.hpp"
#include "command_router.hpp"
#include "ui.hpp"
#include "storage.hpp"

int main() {
    UI::init();
    AuthManager auth;
    PasswordManager pm(auth);

    std::string initError;
    if (!Storage::createDataDirectories(initError)) {
        UI::error(initError.empty() ? "Failed to initialize AppData storage" : initError);
        return 1;
    }

    UI::heading("   _   ___ ___ ___ ___ ");
    UI::heading("  /_\\ | __/ __|_ _/ __|");
    UI::heading(" / _ \\| _| (_ || |\\__ \\");
    UI::heading("/_/ \\_\\___\\___|___|___/");

    UI::info("\nAegis Password Vault\n");

    const int maxAttempts = 3;
    bool loaded = false;
    std::string error;

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        auth.promptForMasterPassword();

        if (pm.load(error)) {
            loaded = true;
            break;
        }

        if (!error.empty()) {
            UI::error(error);
        }
    }

    if (!loaded) {
        UI::error("Too many failed attempts.");
        return 1;
    }

    UI::line("");
    UI::info("----------------------------------------");
    UI::info("Type 'help' or --help for commands.");
    UI::info("----------------------------------------");

    CommandRouter router(pm);
    std::string line;
    bool running = true;

    while (running) {
        UI::prompt();
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        running = router.handle(line);
        if (running) {
            UI::line("");
        }
    }

    return 0;
}