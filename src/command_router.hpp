#include <password_manager.hpp>

class CommandRouter {
public:
    CommandRouter(PasswordManager& m);

    bool handle(const std::string& line);

private:
    PasswordManager& manager;
};