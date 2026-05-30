#pragma once
#include <string>

struct Entry {
    int id;
    std::string website;
    std::string username;
    std::string password;
    std::string note;

    Entry(int id, const std::string& website, const std::string& username, const std::string& password, const std::string& note) : id(id), website(website), username(username), password(password), note(note) {};
    std::string serialize() const;
    static Entry deserialize(const std::string& line);
    std::string toString() const;
};